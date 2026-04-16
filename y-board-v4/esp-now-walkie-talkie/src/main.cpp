/*
 * ESP-NOW Walkie Talkie
 *
 * Push-to-talk audio over ESP-NOW broadcast.
 * Hold the center button to record, release to compress and send.
 * Nearby boards on the same channel automatically receive and play the audio.
 *
 * Controls (IDLE):
 *   Center button  – hold to record, release to transmit
 *   Up / Down      – change channel (1-13)
 *
 * Audio pipeline:
 *   Record  → 44100 Hz 16-bit mono WAV on SD card
 *   Compress → decimate 6× to 7350 Hz, quantize to 8-bit unsigned
 *   Transmit → 250-byte ESP-NOW broadcast packets
 *   Receive  → reassemble, expand to 16-bit WAV at 7350 Hz, play
 *
 * Display (retro walkie-talkie style, 128×64 OLED):
 *   ┌──────────────────────────┐
 *   │ WALKIE-TALKIE       /|\  │  ← title + antenna icon
 *   ├──────────────────────────┤
 *   │ CH: 07    SIG: ||||      │  ← channel + RSSI bars
 *   ╠══════════════════════════╣
 *   │                          │
 *   │          IDLE            │  ← large status word
 *   │                          │
 *   ├──────────────────────────┤
 *   │  [■■■■■■■■■■■       ]    │  ← progress bar  -or-  hint text
 *   └──────────────────────────┘
 */

#include "yboard.h"
#include "SD.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

// ── Audio compression ──────────────────────────────────────────────────────────
static constexpr int SRC_RATE  = 44100;
static constexpr int DECIMATE  = 6;              // 44100 / 6 = 7350 Hz
static constexpr int DST_RATE  = SRC_RATE / DECIMATE;
static constexpr int MAX_REC_S = 5;
static constexpr int MAX_COMP  = DST_RATE * MAX_REC_S;  // 36 750 bytes

// ── ESP-NOW packet layout ──────────────────────────────────────────────────────
//   Byte 0-1 : magic 'W','T'
//   Byte 2   : session (random per transmission)
//   Byte 3   : seq (0-indexed)
//   Byte 4   : total packets
//   Bytes 5+ : compressed audio payload (up to 245 bytes)
static constexpr int     PKT_HDR  = 5;
static constexpr int     PKT_BODY = 245;
static const uint8_t     BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

struct [[gnu::packed]] WTPkt {
    uint8_t magic[2];
    uint8_t session;
    uint8_t seq;
    uint8_t total;
    uint8_t data[PKT_BODY];
};
static_assert(sizeof(WTPkt) == 250, "packet must be exactly 250 bytes");

// ── State machine ──────────────────────────────────────────────────────────────
enum WTState : uint8_t {
    ST_IDLE, ST_RECORDING, ST_SENDING, ST_RECEIVING, ST_PLAYING
};
static volatile WTState g_state   = ST_IDLE;
static volatile int8_t  g_rssi    = 0;   // dBm from last received packet; 0 = none
static          int     g_channel = 1;   // current ESP-NOW channel (1-13)

// ── RX storage ────────────────────────────────────────────────────────────────
static constexpr int MAX_PKTS = 255;
static uint8_t g_rx_data[MAX_PKTS][PKT_BODY];   // ~62 KB in BSS
static uint8_t g_rx_len [MAX_PKTS];
static bool    g_rx_got [MAX_PKTS];

static volatile uint8_t g_rx_session  = 0xFF;
static volatile uint8_t g_rx_total    = 0;
static volatile int     g_rx_count    = 0;
static volatile bool    g_rx_complete = false;

// ── TX buffer (heap-allocated; prefer PSRAM) ──────────────────────────────────
static uint8_t *g_tx_buf = nullptr;
static int      g_tx_len = 0;

// ─────────────────────────────────────────────────────────────────────────────
// ESP-NOW callbacks
// ─────────────────────────────────────────────────────────────────────────────

static void espnow_recv(const esp_now_recv_info_t *info,
                        const uint8_t *data, int len) {
    if (len < PKT_HDR) return;
    if (data[0] != 'W' || data[1] != 'T') return;
    if (g_state == ST_RECORDING || g_state == ST_SENDING) return;

    g_rssi = (int8_t)info->rx_ctrl->rssi;

    uint8_t sess  = data[2];
    uint8_t seq   = data[3];
    uint8_t total = data[4];
    int     plen  = len - PKT_HDR;

    if (sess != g_rx_session) {
        memset(g_rx_got, 0, sizeof(g_rx_got));
        g_rx_session  = sess;
        g_rx_total    = total;
        g_rx_count    = 0;
        g_rx_complete = false;
        g_state       = ST_RECEIVING;
    }

    if (seq >= MAX_PKTS || g_rx_got[seq] || plen <= 0) return;

    memcpy(g_rx_data[seq], data + PKT_HDR, plen);
    g_rx_len[seq] = (uint8_t)plen;
    g_rx_got[seq] = true;
    g_rx_count++;

    if (g_rx_count >= g_rx_total) g_rx_complete = true;
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
    if (ch < 1)  ch = 13;
    if (ch > 13) ch = 1;
    g_channel = ch;
    esp_wifi_set_channel((uint8_t)g_channel, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Channel → %d\n", g_channel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio compression: WAV on SD → 8-bit 7350 Hz buffer
// ─────────────────────────────────────────────────────────────────────────────

static int compress_wav(const char *path, uint8_t *out, int max_out) {
    File f = SD.open(path, FILE_READ);
    if (!f) { Serial.println("compress: open failed"); return -1; }

    uint8_t hdr[12];
    if (f.read(hdr, 12) < 12 ||
        memcmp(hdr,     "RIFF", 4) != 0 ||
        memcmp(hdr + 8, "WAVE", 4) != 0) {
        Serial.println("compress: not a WAV");
        f.close(); return -1;
    }

    bool found = false;
    while (f.available()) {
        char id[4]; uint32_t sz;
        if (f.read((uint8_t *)id, 4) < 4) break;
        if (f.read((uint8_t *)&sz, 4) < 4) break;
        if (memcmp(id, "data", 4) == 0) { found = true; break; }
        if (!f.seek(f.position() + sz + (sz & 1))) break;
    }
    if (!found) { Serial.println("compress: no data chunk"); f.close(); return -1; }

    constexpr int CHUNK = 512;
    uint8_t  buf[CHUNK];
    int out_idx = 0, samp_ctr = 0;

    while (f.available() && out_idx < max_out) {
        int avail   = f.available();
        int to_read = (avail < CHUNK) ? avail : CHUNK;
        to_read    &= ~1;
        if (to_read <= 0) break;

        int got = f.read(buf, to_read);
        int n   = got / 2;
        for (int i = 0; i < n && out_idx < max_out; i++) {
            if (samp_ctr % DECIMATE == 0) {
                int16_t s; memcpy(&s, buf + i * 2, 2);
                out[out_idx++] = (uint8_t)((s >> 8) + 128);
            }
            samp_ctr++;
        }
    }

    f.close();
    Serial.printf("compress: %d raw samples → %d bytes (%.2f s)\n",
                  samp_ctr, out_idx, (float)out_idx / DST_RATE);
    return out_idx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio decompression: 8-bit buffer → WAV on SD → play
// ─────────────────────────────────────────────────────────────────────────────

static bool decompress_and_play(const uint8_t *data, int len) {
    File f = SD.open("/wt_rx.wav", FILE_WRITE);
    if (!f) { Serial.println("decompress: cannot create /wt_rx.wav"); return false; }

    uint32_t pcm_bytes = (uint32_t)len * 2;

    auto w32 = [&](uint32_t v) { f.write((uint8_t *)&v, 4); };
    auto w16 = [&](uint16_t v) { f.write((uint8_t *)&v, 2); };

    f.write((const uint8_t *)"RIFF", 4);  w32(36 + pcm_bytes);
    f.write((const uint8_t *)"WAVE", 4);
    f.write((const uint8_t *)"fmt ", 4);  w32(16);
    w16(1); w16(1);
    w32((uint32_t)DST_RATE);
    w32((uint32_t)DST_RATE * 2);
    w16(2); w16(16);
    f.write((const uint8_t *)"data", 4);  w32(pcm_bytes);

    constexpr int WR = 256;
    uint8_t tmp[WR * 2];
    for (int base = 0; base < len; base += WR) {
        int n = len - base; if (n > WR) n = WR;
        for (int i = 0; i < n; i++) {
            int16_t s = (int16_t)((int)data[base + i] - 128) << 8;
            tmp[i*2]   = (uint8_t)(s & 0xFF);
            tmp[i*2+1] = (uint8_t)((uint16_t)s >> 8);
        }
        f.write(tmp, n * 2);
    }
    f.close();

    Yboard.set_sound_file_volume(8);
    return Yboard.play_sound_file("/wt_rx.wav");
}

// ─────────────────────────────────────────────────────────────────────────────
// Display — retro walkie-talkie style
// ─────────────────────────────────────────────────────────────────────────────

// Small antenna icon at top-right: vertical mast + two V-shaped waves
static void draw_antenna(int x, int y) {
    // Mast
    Yboard.display.drawFastVLine(x + 4, y, 5, WHITE);
    // Wave 1 (inner)
    Yboard.display.drawLine(x + 4, y + 2, x + 1, y + 5, WHITE);
    Yboard.display.drawLine(x + 4, y + 2, x + 7, y + 5, WHITE);
    // Wave 2 (outer)
    Yboard.display.drawLine(x + 4, y,     x,     y + 5, WHITE);
    Yboard.display.drawLine(x + 4, y,     x + 8, y + 5, WHITE);
    // Base
    Yboard.display.drawFastHLine(x + 2, y + 5, 5, WHITE);
}

// 4-bar RSSI icon (width=22, drawn starting at given x,y; 8 px tall)
static void draw_sig_bars(int x, int y) {
    int filled = 0;
    if      (g_rssi >= -60) filled = 4;
    else if (g_rssi >= -70) filled = 3;
    else if (g_rssi >= -80) filled = 2;
    else if (g_rssi != 0)   filled = 1;

    for (int i = 0; i < 4; i++) {
        int h  = (i + 1) * 2;
        int bx = x + i * 5;
        int by = y + (8 - h);
        if (i < filled) Yboard.display.fillRect(bx, by, 4, h, WHITE);
        else            Yboard.display.drawRect(bx, by, 4, h, WHITE);
    }
}

// Full screen redraw — retro walkie-talkie layout
// prog_total > 0  → show progress bar + packet count
// prog_total == 0 → show hint text
static void draw_screen(const char *status,
                        int prog_done  = 0,
                        int prog_total = 0,
                        const char *hint = nullptr) {
    Yboard.display.clearDisplay();
    Yboard.display.setTextColor(WHITE);

    // ── Outer border ──────────────────────────────────────────────
    Yboard.display.drawRect(0, 0, 128, 64, WHITE);

    // ── Row 1: title bar (y=2..9) ─────────────────────────────────
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(3, 2);
    Yboard.display.print("WALKIE-TALKIE");

    // Antenna icon at x=114, y=1
    draw_antenna(114, 1);

    // Separator under title
    Yboard.display.drawFastHLine(1, 11, 126, WHITE);

    // ── Row 2: channel + signal (y=13..20) ───────────────────────
    char ch_str[10];
    snprintf(ch_str, sizeof(ch_str), "CH: %02d", g_channel);
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(4, 13);
    Yboard.display.print(ch_str);

    // "SIG:" label + bars
    Yboard.display.setCursor(60, 13);
    Yboard.display.print("SIG:");
    draw_sig_bars(84, 13);

    // dBm value, right-aligned
    if (g_rssi != 0) {
        char dbm[8];
        snprintf(dbm, sizeof(dbm), "%ddB", (int)g_rssi);
        Yboard.display.setCursor(107, 13);
        Yboard.display.print(dbm);
    }

    // Double separator (retro feel)
    Yboard.display.drawFastHLine(1, 22, 126, WHITE);
    Yboard.display.drawFastHLine(1, 24, 126, WHITE);

    // ── Row 3: large status word (y=28..43) ───────────────────────
    Yboard.display.setTextSize(2);
    int sw = (int)strlen(status) * 12;
    int sx = (128 - sw) / 2;
    if (sx < 1) sx = 1;
    Yboard.display.setCursor(sx, 28);
    Yboard.display.print(status);

    // Separator above footer
    Yboard.display.drawFastHLine(1, 46, 126, WHITE);

    // ── Row 4: footer (y=49..62) ──────────────────────────────────
    if (prog_total > 0) {
        // Progress bar
        Yboard.display.drawRect(3, 49, 122, 7, WHITE);
        int filled_px = (prog_done * 120) / prog_total;
        if (filled_px > 0) Yboard.display.fillRect(4, 50, filled_px, 5, WHITE);

        char txt[20];
        snprintf(txt, sizeof(txt), "%d/%d pkts", prog_done, prog_total);
        Yboard.display.setTextSize(1);
        int tw = (int)strlen(txt) * 6;
        // Show packet count to the right of bar if it fits, else below
        Yboard.display.setCursor((128 - tw) / 2, 57);
        Yboard.display.print(txt);
    } else {
        const char *h = hint ? hint : "^v=CH  [CTR]=TALK";
        Yboard.display.setTextSize(1);
        int hw = (int)strlen(h) * 6;
        Yboard.display.setCursor((128 - hw) / 2, 53);
        Yboard.display.print(h);
    }

    Yboard.display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// LED helpers
// ─────────────────────────────────────────────────────────────────────────────

static void leds_breath_blue() {
    static uint32_t t = 0; static int bv = 0, dir = 1;
    if (millis() - t < 30) return;
    t = millis();
    bv += dir * 2;
    if (bv >= 100) dir = -1;
    if (bv <= 0)   dir =  1;
    Yboard.set_all_leds_color(0, 0, (uint8_t)bv);
}

static void leds_pulse_red() {
    static uint32_t t = 0; static bool on = true;
    if (millis() - t < 350) return;
    t = millis(); on = !on;
    Yboard.set_all_leds_color(on ? 200 : 50, 0, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// setup
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextColor(WHITE);

    radio_init();

    g_tx_buf = (uint8_t *)heap_caps_malloc(MAX_COMP,
                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_tx_buf) g_tx_buf = (uint8_t *)malloc(MAX_COMP);
    if (!g_tx_buf) Serial.println("ERROR: TX buffer alloc failed");

    Yboard.set_led_brightness(80);
    draw_screen("IDLE");
}

// ─────────────────────────────────────────────────────────────────────────────
// loop
// ─────────────────────────────────────────────────────────────────────────────

void loop() {
    bool btn    = Yboard.get_button(YBoardV4::button_center);
    bool btn_up = Yboard.get_button(YBoardV4::button_up);
    bool btn_dn = Yboard.get_button(YBoardV4::button_down);

    switch (g_state) {

    // ── IDLE ──────────────────────────────────────────────────────────────────
    case ST_IDLE: {
        leds_breath_blue();

        // Channel selection
        static bool prev_up = false, prev_dn = false;
        if (btn_up && !prev_up) { set_channel(g_channel + 1); draw_screen("IDLE"); }
        if (btn_dn && !prev_dn) { set_channel(g_channel - 1); draw_screen("IDLE"); }
        prev_up = btn_up;
        prev_dn = btn_dn;

        // Refresh every 500 ms so RSSI updates show
        static uint32_t last_draw = 0;
        if (millis() - last_draw > 500) {
            last_draw = millis();
            draw_screen("IDLE");
        }

        if (btn) {
            g_state = ST_RECORDING;
            Yboard.set_all_leds_color(180, 0, 0);
            draw_screen("REC", 0, 0, "Release to send");
            Yboard.set_recording_volume(8);
            Yboard.start_recording("/wt_tx.wav");
        }
        break;
    }

    // ── RECORDING ─────────────────────────────────────────────────────────────
    case ST_RECORDING: {
        leds_pulse_red();

        if (!btn) {
            Yboard.stop_recording();
            delay(300);   // let SD flush and close

            g_state = ST_SENDING;
            Yboard.set_all_leds_color(0, 160, 0);
            draw_screen("SENDING");

            if (!g_tx_buf) {
                Serial.println("No TX buffer");
                g_state = ST_IDLE;
                Yboard.set_all_leds_color(0, 0, 0);
                draw_screen("IDLE");
                break;
            }

            g_tx_len = compress_wav("/wt_tx.wav", g_tx_buf, MAX_COMP);
            if (g_tx_len <= 0) {
                Serial.println("Compression failed");
                g_state = ST_IDLE;
                Yboard.set_all_leds_color(0, 0, 0);
                draw_screen("IDLE");
                break;
            }

            int total_pkts = (g_tx_len + PKT_BODY - 1) / PKT_BODY;
            if (total_pkts > 255) total_pkts = 255;

            uint8_t session_id = (uint8_t)(millis() & 0xFF);
            WTPkt pkt;
            pkt.magic[0] = 'W'; pkt.magic[1] = 'T';
            pkt.session  = session_id;
            pkt.total    = (uint8_t)total_pkts;

            for (int seq = 0; seq < total_pkts; seq++) {
                pkt.seq = (uint8_t)seq;
                int offset = seq * PKT_BODY;
                int chunk  = g_tx_len - offset;
                if (chunk > PKT_BODY) chunk = PKT_BODY;
                memcpy(pkt.data, g_tx_buf + offset, chunk);

                esp_now_send(BCAST, (uint8_t *)&pkt, PKT_HDR + chunk);
                delay(4);   // ~250 pkt/s

                if (seq % 8 == 0) {
                    draw_screen("SENDING", seq + 1, total_pkts);
                    int li = map(seq, 0, total_pkts - 1, 0, 19);
                    Yboard.set_all_leds_color(0, 0, 0);
                    Yboard.set_led_color(constrain(li, 0, 19), 0, 200, 0);
                }
            }

            draw_screen("SENDING", total_pkts, total_pkts);
            delay(400);
            g_state = ST_IDLE;
            Yboard.set_all_leds_color(0, 0, 0);
            draw_screen("IDLE");
        }
        break;
    }

    // ── RECEIVING ─────────────────────────────────────────────────────────────
    case ST_RECEIVING: {
        static uint32_t rx_t = 0;
        if (millis() - rx_t > 120) {
            rx_t = millis();
            draw_screen("RECEIVE", g_rx_count, (int)g_rx_total);
            int li = (g_rx_total > 0)
                     ? map(g_rx_count, 0, (int)g_rx_total, 0, 19) : 0;
            Yboard.set_all_leds_color(0, 0, 0);
            Yboard.set_led_color(constrain(li, 0, 19), 0, 80, 220);
        }

        if (g_rx_complete) {
            int total_bytes = 0;
            for (int i = 0; i < (int)g_rx_total; i++)
                if (g_rx_got[i]) total_bytes += g_rx_len[i];

            uint8_t *audio = (uint8_t *)heap_caps_malloc(
                total_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!audio) audio = (uint8_t *)malloc(total_bytes);

            if (audio) {
                int pos = 0;
                for (int i = 0; i < (int)g_rx_total; i++) {
                    if (g_rx_got[i]) {
                        memcpy(audio + pos, g_rx_data[i], g_rx_len[i]);
                        pos += g_rx_len[i];
                    }
                }
                g_state = ST_PLAYING;
                draw_screen("PLAYING");
                Yboard.set_all_leds_color(0, 140, 180);
                decompress_and_play(audio, total_bytes);
                free(audio);
            } else {
                Serial.println("RECV: malloc failed for reassembly");
            }

            g_rx_complete = false;
            g_rx_count    = 0;
            g_state       = ST_IDLE;
            Yboard.set_all_leds_color(0, 0, 0);
            draw_screen("IDLE");
        }
        break;
    }

    default: break;
    }

    delay(10);
}
