#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "yboard.h"

typedef struct {
    unsigned frame_ctrl : 16;
    unsigned duration_id : 16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

// Callback for when a packet is received
void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);
CRGB valueToColor(uint8_t val);
void update_display();

int current_channel = 1;
bool sniffed_packet = false;
int rssi = 0;

void setup() {
    Serial.begin(115200);

    // Set up WiFi hardware
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_rx_packet);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    Yboard.setup();
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.set_led_brightness(100);

    Yboard.display.setTextWrap(false);
    Yboard.display.println("The lights blink\n"
                           "for every WiFi packet\n"
                           "Turn the knob to\n"
                           "adjust brightness.\n"
                           "Press up/down to\n"
                           "change channel.\n"
                           "Press a btn to start\n");
    Yboard.display.display();

    while (!Yboard.get_buttons()) {
        delay(50);
    }

    while (Yboard.get_buttons()) {
        delay(50);
    }

    Yboard.set_knob(100);
    update_display();
}

void loop() {
    // Update brightness of LEDs based on knob
    int brightness = constrain(Yboard.get_knob(), 0, 200);
    Yboard.set_led_brightness(brightness);

    if (Yboard.get_button(Yboard.button_up) || Yboard.get_button(Yboard.button_down)) {
        // If either button is pressed, change channels
        if (Yboard.get_button(Yboard.button_down)) {
            current_channel--;
            if (current_channel < 1) {
                current_channel = 11;
            }
        }

        if (Yboard.get_button(Yboard.button_up)) {
            current_channel++;
            if (current_channel > 11) {
                current_channel = 1;
            }
        }

        Serial.printf("Switching to channel %d\n", current_channel);
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

        update_display();

        while (Yboard.get_buttons() != 0) {
            delay(10);
        }
    }

    if (sniffed_packet) {
        sniffed_packet = false;

        // Map RSSI to 0 and 255
        int value = map(rssi, -90, -50, 0, 255);
        value = constrain(value, 0, 255);
        Serial.printf("RSSI: %d, Mapped value: %d\n", rssi, value);

        // Map value to rainbow color
        CRGB color = valueToColor(value);

        // Light up LEDs
        Yboard.set_all_leds_color(color.red, color.green, color.blue);
    } else {
        // Turn off LEDs
        Yboard.set_all_leds_color(0, 0, 0);
    }
}

void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
    sniffed_packet = true;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // Parse radiotap header
    wifi_pkt_rx_ctrl_t header = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;
    rssi = header.rssi;
}

CRGB valueToColor(uint8_t val) {
    // val: 0 → blue, 255 → red
    uint8_t r = val;       // increases from 0 → 255
    uint8_t g = 0;         // stays at 0
    uint8_t b = 255 - val; // decreases from 255 → 0
    return CRGB(r, g, b);
}

void update_display() {
    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.printf("Channel: %d", current_channel);
    Yboard.display.display();
}
