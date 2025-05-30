#include "yboard.h"

int brightness = 120;

void print_to_display(const char *message) {
    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.println(message);
    Yboard.display.display();
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    Yboard.setup();
    Yboard.set_led_brightness(brightness);
}

void loop() {
    if (Yboard.get_switch(1)) {
        print_to_display("IR Receiver Mode");

        if (Yboard.recv_ir()) {
            decode_results &results = Yboard.ir_results;
            Serial.println(resultToHumanReadableBasic(&results));
            print_to_display(resultToHumanReadableBasic(&results).c_str());

            // My own custom protocol
            if (results.value == 0x01) {
                Yboard.set_all_leds_color(255, 255, 255);
            } else if (results.value == 0x02) {
                Yboard.set_all_leds_color(255, 0, 255);
            } else if (results.value == 0x04) {
                Yboard.set_all_leds_color(0, 255, 255);
            } else if (results.value == 0x08) {
                Yboard.set_all_leds_color(255, 255, 0);
            } else if (results.value == 0x10) {
                Yboard.set_all_leds_color(0, 0, 255);
            }
            // Random remote
            else if (results.value == 0xFF906F) {
                Yboard.set_all_leds_color(255, 0, 0);
            } else if (results.value == 0xFF10EF) {
                Yboard.set_all_leds_color(0, 255, 0);
            } else if (results.value == 0xFF50AF) {
                Yboard.set_all_leds_color(0, 0, 255);
            } else if (results.value == 0xFF609F) {
                Yboard.set_all_leds_color(0, 0, 0);
            } else if (results.value == 0xFFE01F) {
                Yboard.set_all_leds_color(255, 255, 255);
            } else if (results.value == 0xFFA05F) {
                // Add brightness
                brightness += 10;
                if (brightness > 255) {
                    brightness = 255;
                }
                Yboard.set_led_brightness(brightness);
            } else if (results.value == 0xFF20DF) {
                // Subtract brightness
                brightness -= 10;
                if (brightness < 0) {
                    brightness = 0;
                }
                Yboard.set_led_brightness(brightness);
            }

            delay(1000);
            Yboard.clear_ir();
        }
    } else {
        Yboard.set_all_leds_color(0, 0, 0);

        // Print as binary
        // Serial.println(Yboard.get_buttons() & 0x1F, BIN);
        // delay(200);

        if (Yboard.get_buttons() & 0x1F) {
            Yboard.set_all_leds_color(255, 0, 0);
            Yboard.ir_send.sendNEC(Yboard.get_buttons() & 0x1F, 32);

            while (Yboard.get_buttons() & 0x1F) {
                delay(10);
            }
        }
    }
}
