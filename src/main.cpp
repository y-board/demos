#include "hardware_test/hardware_test.h"
#include "wifi_sniffer/wifi_sniffer.h"

void setup() {
    // wifi_sniffer_init();
    hardware_test_init();
}

void loop() {
    // wifi_sniffer_loop();
    hardware_test_loop();
}
