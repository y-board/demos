#include "hardware_test/hardware_test.h"
#include "light_show/light_show.h"
#include "wifi_sniffer/wifi_sniffer.h"

void setup() {
    // wifi_sniffer_init();
    // hardware_test_init();
    light_show_init();
}

void loop() {
    // wifi_sniffer_loop();
    // hardware_test_loop();
    light_show_loop();
}
