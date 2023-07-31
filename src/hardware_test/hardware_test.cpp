#include "hardware_test.h"

void hardware_test_init() {
    Serial.begin(9600);
    leds_init();
    timer_init();
}

/*
 * This function tests all of the features of the badge.
 * It will display different colors depending on which button is pressed.
 * It will also play different notes depending on which button is pressed IF at least one of the two
 * switches is on It will also change the brightness of the LEDs depending on the position of the
 * knob.
 */
void hardware_test_loop() {
    if (buttons_get(1)) {
        while (buttons_get(1)) {
            all_leds_set_color(255, 0, 0);
            leds_set_brightness(get_brightness());
            if (check_switches()) {
                speaker_play_note(NOTE_C4, 10);
                delay(10);
            }
        }
    } else if (buttons_get(2)) {
        while (buttons_get(2)) {
            all_leds_set_color(255, 255, 0);
            leds_set_brightness(get_brightness());
            if (check_switches()) {
                speaker_play_note(NOTE_D4, 10);
                delay(10);
            }
        }
    } else if (buttons_get(3)) {
        while (buttons_get(3)) {
            all_leds_set_color(0, 255, 0);
            leds_set_brightness(get_brightness());
            if (check_switches()) {
                speaker_play_note(NOTE_E4, 10);
                delay(10);
            }
        }
    } else {
        all_leds_set_color(255, 255, 255);
        leds_set_brightness(get_brightness());
    }
}

// This function converts the knob's output (1-100) to a brightness value (0-255)
int get_brightness() { return map(knob_get(), 0, 100, 0, 255); }

// This function checks if either of the switches are on
bool check_switches() { return switches_get(1) || switches_get(2); }
