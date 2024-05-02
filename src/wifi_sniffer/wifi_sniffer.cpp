#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <StateMachine.h>

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

int current_channel = 1;
bool sniffed_packet = false;
int rssi = 0;

const int num_readings = 150;
int readings[num_readings];
int read_index = 0;
int total = 0;
int average = 0;

StateMachine machine = StateMachine();
State *SniffState = machine.addState(&sniff_state);
State *AdjustChannelState = machine.addState(&adjust_channel_state);
State *IncreaseChannelState = machine.addState(&increase_channel_state);
State *DecreaseChannelState = machine.addState(&decrease_channel_state);

void wifi_sniffer_init() {
    Serial.begin(9600);
    leds_init();
    timer_init();

    // Set up WiFi hardware
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_rx_packet);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    // Set up state machine transitions
    SniffState->addTransition([]() { return switches_get(1); }, AdjustChannelState);
    AdjustChannelState->addTransition([]() { return buttons_get(3); }, IncreaseChannelState);
    AdjustChannelState->addTransition([]() { return buttons_get(1); }, DecreaseChannelState);
    AdjustChannelState->addTransition([]() { return !switches_get(1); }, SniffState);
    IncreaseChannelState->addTransition([]() { return !buttons_get(3); }, AdjustChannelState);
    DecreaseChannelState->addTransition([]() { return !buttons_get(1); }, AdjustChannelState);
}

void wifi_sniffer_loop() { machine.run(); }

void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
    sniffed_packet = true;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // Parse radiotap header
    wifi_pkt_rx_ctrl_t header = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;
    rssi = header.rssi;
}

int smooth(int value) {
    total = total - readings[read_index];
    readings[read_index] = value;
    total = total + readings[read_index];

    read_index = read_index + 1;

    if (read_index >= num_readings) {
        read_index = 0;
    }

    return total / num_readings;
}

void sniff_state() {
    // Only run this once
    if (machine.executeOnce) {
        // Update WiFi channel
        Serial.printf("Switching to channel %d\n", current_channel);
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    }

    // Update brightness of LEDs based on knob
    int brightness = map(knob_get(), 0, 100, 0, 255);
    leds_set_brightness(brightness);

    if (switches_get(2)) {
        // If the first switch is set, blink the LDS for every frame sniffed

        if (sniffed_packet) {
            sniffed_packet = false;

            // Map RSSI to 0 and 255
            int value = map(rssi, -90, -65, 0, 255);

            // Map value to rainbow color
            RGBColor color = color_wheel(value);

            // Light up LEDs
            all_leds_set_color(color.red, color.green, color.blue);
        } else {
            // Turn off LEDs
            all_leds_set_color(0, 0, 0);
        }
    } else {
        // If the first switch is not set, use the color as an indication of signal strength
        if (sniffed_packet) {
            sniffed_packet = false;

            // Come up with a value for the RSSI
            int smoothed_rssi = smooth(rssi);
            int value = map(smoothed_rssi, -85, -65, 0, 255);
            Serial.printf("Smoothed value: %d (%d)\n", smoothed_rssi, value);

            // Map value to rainbow color
            RGBColor color = blue_to_red(value);

            // Light up LEDs
            all_leds_set_color(color.red, color.green, color.blue);
        }
    }
}

void adjust_channel_state() {
    // Only run this state once
    if (!machine.executeOnce) {
        return;
    }
    Serial.println("Adjust channel state");

    // Light up the LEDs based on the current channel
    for (int i = 1; i < LED_COUNT + 1; i++) {
        if (i <= current_channel) {
            leds_set_color(i, 255, 255, 255);
        } else {
            leds_set_color(i, 0, 0, 0);
        }
    }
}

void increase_channel_state() {
    // Only run this state once
    if (!machine.executeOnce) {
        return;
    }

    Serial.println("Increase channel state");

    current_channel++;
    if (current_channel > 11) {
        current_channel = 11;
    }
}

void decrease_channel_state() {
    // Only run this state once
    if (!machine.executeOnce) {
        return;
    }

    Serial.println("Decrease channel state");

    current_channel--;
    if (current_channel < 1) {
        current_channel = 1;
    }
}
