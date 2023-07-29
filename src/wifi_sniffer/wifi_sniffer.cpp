#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "colors.h"
#include "wifi_sniffer.h"

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

bool sniffed_packet = false;
int rssi = 0;

void wifi_sniffer_init() {
    Serial.begin(9600);
    leds_init();
    timer_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_rx_packet);

    // Set primary and secondary channel(not used)
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_loop() {
    if (sniffed_packet) {
        sniffed_packet = false;
        wifi_sniffer_sniff_action();
    } else {
        all_leds_set_color(0, 0, 0);
    }

    int brightness = map(knob_get(), 0, 4095, 0, 255);
    leds_set_brightness(brightness);
}

void wifi_sniffer_sniff_action() {
    // Depending on the state of switch 2, either turn on the LEDs to white
    // or map RSSI to a color
    if (switches_get(2)) {
        // Turn on LEDs
        all_leds_set_color(255, 255, 255);
    } else {
        // Map RSSI to 0 and 255
        int value = map(rssi, -90, -65, 0, 255);
        // Serial.printf("RSSI: %d, value: %d\n", rssi, value);

        // Map value to rainbow color
        uint8_t *c = color_wheel(value);

        // Turn LEDs on
        all_leds_set_color(c[0], c[1], c[2]);
    }
}

void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
    sniffed_packet = true;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // Parse radiotap header
    wifi_pkt_rx_ctrl_t header = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;
    rssi = header.rssi;
}
