/*
 * ESP-NOW Walkie Talkie — real-time streaming
 *
 * Push-to-talk: hold center button to talk, release to stop.
 * Audio is streamed live over ESP-NOW with no file I/O.
 *
 * Pipeline:
 *   TX: mic I2S  →  240-byte raw PCM chunk  →  ESP-NOW broadcast
 *   RX: ESP-NOW callback  →  FreeRTOS StreamBuffer  →  speaker I2S
 *
 * Audio format: 16 kHz, 16-bit, mono (telephone quality)
 *   240 bytes = 120 samples = 7.5 ms per packet
 *   ~133 packets/sec — well within ESP-NOW bandwidth
 *
 * Controls (IDLE state):
 *   Center  – hold to transmit, release to stop
 *   Up/Down – change channel (1-13); both boards must match
 *
 * Display (retro walkie-talkie style, 128×64 OLED):
 *   ┌──────────────────────────┐
 *   │ WALKIE-TALKIE       /|\  │  title + antenna
 *   ├──────────────────────────┤
 *   │ CH: 07    SIG: ||||      │  channel + RSSI bars + dBm
 *   ╠══════════════════════════╣
 *   │          IDLE            │  large status word
 *   ├──────────────────────────┤
 *   │   ^v=CH  [CTR]=TALK      │  hint / packet stats
 *   └──────────────────────────┘
 */

#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "yboard.h"
#include <string.h>

// ── Audio format ───────────────────────────────────────────────────────────────
// 16 kHz matches the speaker's native I2S rate; mic is reconfigured to match.
static constexpr int SAMPLE_RATE = 16000;
static constexpr int BITS = 16;
static constexpr int BYTES_PER_SMP = BITS / 8;

// ── Packet layout ──────────────────────────────────────────────────────────────
// Byte 0-1: magic 'W','T'
// Byte 2  : session   (new value = new transmission)
// Byte 3  : seq       (rolling 0-255)
// Byte 4  : flags     (0x01 = last packet in transmission)
// Bytes 5+: raw 16-bit PCM samples, little-endian
static constexpr int PKT_HDR = 5;
static constexpr int PKT_AUDIO = 240;                 // 120 samples × 2 bytes = 7.5 ms
static constexpr int PKT_TOTAL = PKT_HDR + PKT_AUDIO; // 245 ≤ 250 ✓
static_assert(PKT_TOTAL <= 250, "ESP-NOW max payload is 250 bytes");

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ── I2S pin definitions (Y-Board V4) ──────────────────────────────────────────
static constexpr int MIC_PIN_WS = 41;
static constexpr int MIC_PIN_DATA = 40;
static constexpr int MIC_I2S_PORT = 0;

static constexpr int SPK_PIN_WS = 47;
static constexpr int SPK_PIN_BCK = 21;
static constexpr int SPK_PIN_DATA = 14;
static constexpr int SPK_I2S_PORT = 1;

// ── State ──────────────────────────────────────────────────────────────────────
enum WTState : uint8_t { ST_IDLE, ST_TX, ST_RX };
static volatile WTState g_state = ST_IDLE;
static volatile int8_t g_rssi = 0; // dBm; 0 = no packets seen
static int g_channel = 1;

// ── TX bookkeeping ────────────────────────────────────────────────────────────
static uint8_t g_tx_session = 0;
static uint8_t g_tx_seq = 0;
static volatile uint32_t g_tx_pkt_count = 0; // for display

// ── RX bookkeeping ────────────────────────────────────────────────────────────
// StreamBuffer bridges ESP-NOW callback (core 0) ↔ speaker task (core 0).
// Capacity: 16 packets = 120 ms of audio — enough to absorb ESP-NOW jitter.
static constexpr int SBUF_SIZE = PKT_AUDIO * 16;
static StreamBufferHandle_t g_audio_sbuf = nullptr;
static volatile uint32_t g_rx_pkt_count = 0;

// Flag set by speaker task when the stream dries up; main loop acts on it.
static volatile bool g_rx_ended = false;

// ─────────────────────────────────────────────────────────────────────────────
// ESP-NOW callbacks
// ─────────────────────────────────────────────────────────────────────────────

static void espnow_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len < PKT_HDR) {
        return;
    }
    if (data[0] != 'W' || data[1] != 'T') {
        return;
    }
    if (g_state == ST_TX) {
        return; // ignore our own echoes while transmitting
    }

    (void)mac_addr;

    int audio_len = len - PKT_HDR;
    if (audio_len > 0) {
        // Push raw PCM audio into the stream buffer.
        // If full, the oldest audio is simply dropped — the stream buffer is
        // FIFO so the oldest data falls off the front naturally (non-overwrite
        // mode). A full buffer means the speaker can't keep up; we just drop.
        xStreamBufferSendFromISR(g_audio_sbuf, data + PKT_HDR, audio_len, nullptr);
        g_rx_pkt_count++;
        g_state = ST_RX;
    }
}

static void espnow_send(const uint8_t *, esp_now_send_status_t) {}

// ─────────────────────────────────────────────────────────────────────────────
// Radio
// ─────────────────────────────────────────────────────────────────────────────

static void radio_init() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_recv_cb(espnow_recv);
    esp_now_register_send_cb(espnow_send);

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, BCAST, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    esp_wifi_set_channel((uint8_t)g_channel, WIFI_SECOND_CHAN_NONE);
}

static void set_channel(int ch) {
    if (ch < 1) {
        ch = 13;
    }
    if (ch > 13) {
        ch = 1;
    }
    g_channel = ch;
    esp_wifi_set_channel((uint8_t)g_channel, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Channel → %d\n", g_channel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio stream setup
// Reconfigure the mic I2S from 44100 Hz (default) down to SAMPLE_RATE.
// The speaker is already at 16 kHz; we just re-start it to ensure a clean state.
// ─────────────────────────────────────────────────────────────────────────────

static void audio_init() {
    // ── Microphone ────────────────────────────────────────────────
    // get_microphone_stream() returns the raw I2SStream used by the yboard mic.
    // We call begin() with a fresh config to change the sample rate.
    auto &mic = Yboard.get_microphone_stream();
    auto mic_cfg = mic.defaultConfig(RX_MODE);
    mic_cfg.sample_rate = SAMPLE_RATE;
    mic_cfg.channels = 1;
    mic_cfg.bits_per_sample = BITS;
    // PDM signal type required for the on-board mic — defaultConfig() drops it
    mic_cfg.signal_type = PDM;
    mic_cfg.i2s_format = I2S_STD_FORMAT;
    mic_cfg.is_master = true;
    mic_cfg.pin_ws = MIC_PIN_WS;
    mic_cfg.pin_data = MIC_PIN_DATA;
    mic_cfg.port_no = MIC_I2S_PORT;
    mic.begin(mic_cfg);

    // ── Speaker ───────────────────────────────────────────────────
    // get_speaker_stream() returns the raw I2SStream; we write PCM directly,
    // bypassing the yboard WAV/MP3 decoder chain entirely.
    auto &spk = Yboard.get_speaker_stream();
    auto spk_cfg = spk.defaultConfig(TX_MODE);
    spk_cfg.sample_rate = SAMPLE_RATE;
    spk_cfg.channels = 1;
    spk_cfg.bits_per_sample = BITS;
    spk_cfg.pin_ws = SPK_PIN_WS;
    spk_cfg.pin_bck = SPK_PIN_BCK;
    spk_cfg.pin_data = SPK_PIN_DATA;
    spk_cfg.port_no = SPK_I2S_PORT;
    spk.begin(spk_cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Speaker task — runs on core 0, alongside the WiFi/ESP-NOW stack
// Drains g_audio_sbuf into the I2S speaker hardware in real time.
// Times out after 300 ms of silence (= end of transmission).
// ─────────────────────────────────────────────────────────────────────────────

static void speaker_task(void *) {
    auto &spk = Yboard.get_speaker_stream();

    uint8_t buf[PKT_AUDIO];
    constexpr TickType_t SILENCE_TIMEOUT = pdMS_TO_TICKS(300);

    while (true) {
        size_t got = xStreamBufferReceive(g_audio_sbuf, buf, sizeof(buf), SILENCE_TIMEOUT);
        if (got > 0) {
            spk.write(buf, got);
        } else {
            // 300 ms of silence: the transmission has ended
            if (g_state == ST_RX) {
                g_rx_ended = true; // main loop will clean up display/LEDs
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TX — called every iteration while button is held.
// Reads one packet-worth of PCM from the mic I2S (blocks ~7.5 ms) and sends it.
// ─────────────────────────────────────────────────────────────────────────────

static void transmit_packet(bool last) {
    auto &mic = Yboard.get_microphone_stream();

    uint8_t pkt[PKT_TOTAL];
    pkt[0] = 'W';
    pkt[1] = 'T';
    pkt[2] = g_tx_session;
    pkt[3] = g_tx_seq++;
    pkt[4] = last ? 0x01 : 0x00;

    uint8_t *audio = pkt + PKT_HDR;

    // readBytes blocks until PKT_AUDIO bytes are available from I2S DMA.
    // At 16 kHz 16-bit, this naturally paces transmission to ~7.5 ms/packet.
    size_t got = mic.readBytes(audio, PKT_AUDIO);
    if (got < PKT_AUDIO) {
        // Partial read: zero-pad to keep timing correct
        memset(audio + got, 0, PKT_AUDIO - got);
    }

    // Raw PDM samples are quiet; apply software gain (VolumeStream is bypassed
    // because we read I2S directly). Saturate on overflow.
    constexpr int MIC_GAIN = 8;
    int16_t *samples = reinterpret_cast<int16_t *>(audio);
    int n_samples = PKT_AUDIO / 2;
    for (int i = 0; i < n_samples; i++) {
        int32_t s = (int32_t)samples[i] * MIC_GAIN;
        if (s > 32767)
            s = 32767;
        else if (s < -32768)
            s = -32768;
        samples[i] = (int16_t)s;
    }

    esp_now_send(BCAST, pkt, PKT_TOTAL);
    g_tx_pkt_count++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Display — retro walkie-talkie style
// ─────────────────────────────────────────────────────────────────────────────

static void draw_antenna(int x, int y) {
    Yboard.display.drawFastVLine(x + 4, y, 5, WHITE);           // mast
    Yboard.display.drawLine(x + 4, y + 2, x + 1, y + 5, WHITE); // inner-L
    Yboard.display.drawLine(x + 4, y + 2, x + 7, y + 5, WHITE); // inner-R
    Yboard.display.drawLine(x + 4, y, x, y + 5, WHITE);         // outer-L
    Yboard.display.drawLine(x + 4, y, x + 8, y + 5, WHITE);     // outer-R
    Yboard.display.drawFastHLine(x + 2, y + 5, 5, WHITE);       // base
}

static void draw_sig_bars(int x, int y) {
    int filled = 0;
    if (g_rssi >= -60) {
        filled = 4;
    } else if (g_rssi >= -70) {
        filled = 3;
    } else if (g_rssi >= -80) {
        filled = 2;
    } else if (g_rssi != 0) {
        filled = 1;
    }

    for (int i = 0; i < 4; i++) {
        int h = (i + 1) * 2;
        int bx = x + i * 5;
        int by = y + (8 - h);
        if (i < filled) {
            Yboard.display.fillRect(bx, by, 4, h, WHITE);
        } else {
            Yboard.display.drawRect(bx, by, 4, h, WHITE);
        }
    }
}

// stat_str: optional small string in the footer (e.g. packet count)
static void draw_screen(const char *status, const char *footer = nullptr) {
    Yboard.display.clearDisplay();
    Yboard.display.setTextColor(WHITE);

    // Outer border
    Yboard.display.drawRect(0, 0, 128, 64, WHITE);

    // ── Title row (y 2-9) ─────────────────────────────────────────
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(3, 2);
    Yboard.display.print("WALKIE-TALKIE");
    draw_antenna(114, 1);
    Yboard.display.drawFastHLine(1, 11, 126, WHITE);

    // ── Channel + signal row (y 13-20) ────────────────────────────
    char ch_str[10];
    snprintf(ch_str, sizeof(ch_str), "CH: %02d", g_channel);
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(4, 13);
    Yboard.display.print(ch_str);

    Yboard.display.setCursor(60, 13);
    Yboard.display.print("SIG:");
    draw_sig_bars(84, 13);

    if (g_rssi != 0) {
        char dbm[8];
        snprintf(dbm, sizeof(dbm), "%ddB", (int)g_rssi);
        Yboard.display.setCursor(107, 13);
        Yboard.display.print(dbm);
    }

    // Double separator (retro feel)
    Yboard.display.drawFastHLine(1, 22, 126, WHITE);
    Yboard.display.drawFastHLine(1, 24, 126, WHITE);

    // ── Large status word (y 28-43) ───────────────────────────────
    Yboard.display.setTextSize(2);
    int sw = (int)strlen(status) * 12;
    int sx = (128 - sw) / 2;
    if (sx < 1) {
        sx = 1;
    }
    Yboard.display.setCursor(sx, 28);
    Yboard.display.print(status);

    // Separator above footer
    Yboard.display.drawFastHLine(1, 46, 126, WHITE);

    // ── Footer (y 49-62) ──────────────────────────────────────────
    const char *h = footer ? footer : "^v=CH  [CTR]=TALK";
    Yboard.display.setTextSize(1);
    int hw = (int)strlen(h) * 6;
    Yboard.display.setCursor((128 - hw) / 2, 53);
    Yboard.display.print(h);

    Yboard.display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// LED helpers
// ─────────────────────────────────────────────────────────────────────────────

static void leds_breath_blue() {
    static uint32_t t = 0;
    static int bv = 0, dir = 1;
    if (millis() - t < 30) {
        return;
    }
    t = millis();
    bv += dir * 2;
    if (bv >= 100) {
        dir = -1;
    }
    if (bv <= 0) {
        dir = 1;
    }
    Yboard.set_all_leds_color(0, 0, (uint8_t)bv);
}

static void leds_pulse_red() {
    static uint32_t t = 0;
    static bool on = true;
    if (millis() - t < 300) {
        return;
    }
    t = millis();
    on = !on;
    Yboard.set_all_leds_color(on ? 200 : 50, 0, 0);
}

static void leds_pulse_blue() {
    static uint32_t t = 0;
    static bool on = true;
    if (millis() - t < 300) {
        return;
    }
    t = millis();
    on = !on;
    Yboard.set_all_leds_color(0, 0, on ? 200 : 50);
}

// ─────────────────────────────────────────────────────────────────────────────
// setup
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextColor(WHITE);

    radio_init();
    audio_init();

    g_audio_sbuf = xStreamBufferCreate(SBUF_SIZE, 1);
    if (!g_audio_sbuf) {
        Serial.println("ERROR: stream buffer alloc failed");
    }

    // Speaker task runs on core 0 (alongside ESP-NOW WiFi stack)
    xTaskCreatePinnedToCore(speaker_task, "wt_spk", 4096, nullptr, 5, nullptr, 0);

    g_tx_session = (uint8_t)(esp_random() & 0xFF);

    Yboard.set_led_brightness(80);
    Yboard.set_recording_volume(8);
    draw_screen("IDLE");
}

// ─────────────────────────────────────────────────────────────────────────────
// loop
// ─────────────────────────────────────────────────────────────────────────────

void loop() {
    bool btn = Yboard.get_button(YBoardV4::button_center);
    bool btn_up = Yboard.get_button(YBoardV4::button_up);
    bool btn_dn = Yboard.get_button(YBoardV4::button_down);

    // ── RX-ended flag (set by speaker task on silence timeout) ────────────────
    if (g_rx_ended) {
        g_rx_ended = false;
        if (g_state == ST_RX) {
            g_state = ST_IDLE;
            Yboard.set_all_leds_color(0, 0, 0);
            draw_screen("IDLE");
        }
    }

    switch (g_state) {

    // ── IDLE ──────────────────────────────────────────────────────────────────
    case ST_IDLE: {
        leds_breath_blue();

        // Channel selection on edge trigger
        static bool prev_up = false, prev_dn = false;
        if (btn_up && !prev_up) {
            set_channel(g_channel + 1);
            draw_screen("IDLE");
        }
        if (btn_dn && !prev_dn) {
            set_channel(g_channel - 1);
            draw_screen("IDLE");
        }
        prev_up = btn_up;
        prev_dn = btn_dn;

        // Refresh RSSI indicator every 500 ms
        static uint32_t last_draw = 0;
        if (millis() - last_draw > 500) {
            last_draw = millis();
            draw_screen("IDLE");
        }

        if (btn) {
            g_state = ST_TX;
            g_tx_session = (uint8_t)(esp_random() & 0xFF);
            g_tx_seq = 0;
            g_tx_pkt_count = 0;
            Yboard.set_all_leds_color(180, 0, 0);
            draw_screen("TX", "Release to stop");
        }
        break;
    }

    // ── TX ────────────────────────────────────────────────────────────────────
    // mic.readBytes() blocks ~7.5 ms per call, naturally pacing the loop.
    case ST_TX: {
        if (!btn) {
            // Button released: send a silent "last" packet as end-of-transmission
            // marker, then return to idle.
            uint8_t end_pkt[PKT_HDR] = {'W', 'T', g_tx_session, g_tx_seq++, 0x01};
            esp_now_send(BCAST, end_pkt, PKT_HDR);

            g_state = ST_IDLE;
            Yboard.set_all_leds_color(0, 0, 0);
            draw_screen("IDLE");
            break;
        }

        transmit_packet(false);
        leds_pulse_red();

        // Update footer stats roughly every 20 packets (~150 ms)
        static uint32_t last_tx_draw = 0;
        if (millis() - last_tx_draw > 150) {
            last_tx_draw = millis();
            char footer[24];
            snprintf(footer, sizeof(footer), "pkts: %lu", g_tx_pkt_count);
            draw_screen("TX", footer);
        }
        break;
    }

    // ── RX ────────────────────────────────────────────────────────────────────
    // The speaker task drains g_audio_sbuf continuously.
    // We just update the display and LEDs here.
    case ST_RX: {
        leds_pulse_blue();

        static uint32_t last_rx_draw = 0;
        if (millis() - last_rx_draw > 150) {
            last_rx_draw = millis();
            char footer[24];
            snprintf(footer, sizeof(footer), "pkts: %lu", g_rx_pkt_count);
            draw_screen("RX", footer);
        }
        break;
    }
    }

    // No delay — the mic I2S read in transmit_packet() rate-limits TX naturally.
    // In IDLE/RX, add a small yield so other tasks (speaker, WiFi) get CPU time.
    if (g_state != ST_TX) {
        delay(10);
    }
}
