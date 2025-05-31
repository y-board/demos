#include "yboard.h"

#define NUM_BUTTONS 5

decode_results learned_codes[NUM_BUTTONS]; // Stores learned IR codes for each button

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
    Yboard.display.setTextWrap(true);
}

void loop() {
    if (Yboard.get_switch(1)) {
        print_to_display("IR Receiver Mode");

        if (Yboard.recv_ir()) {
            decode_results &results = Yboard.ir_results;
            Serial.println(resultToHumanReadableBasic(&results));
            print_to_display(resultToHumanReadableBasic(&results).c_str());

            // My own custom protocol to talk with another YBoard that is in TX mode
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
    } else if (Yboard.get_switch(2)) {
        print_to_display("IR Transmitter Mode");
        Yboard.set_all_leds_color(0, 0, 0);

        if (Yboard.get_buttons()) {
            Yboard.set_all_leds_color(255, 0, 0);
            Yboard.ir_send.sendNEC(Yboard.get_buttons(), 32);

            while (Yboard.get_buttons()) {
                delay(10);
            }
        }
    } else if (Yboard.get_switch(3)) {
        print_to_display("IR Learning Mode\n\nPut a remote in front of the IR receiver and press a "
                         "button on the remote.");

        if (Yboard.recv_ir()) {
            if (Yboard.ir_results.decode_type != UNKNOWN) {
                Yboard.play_notes("T240 O5 C32 E32 G32 C>32");
                Serial.println(resultToHumanReadableBasic(&Yboard.ir_results));
                print_to_display("IR Learning Mode\n\nPress the button you want to program.");

                uint8_t buttons = 0;
                while (!(buttons = Yboard.get_buttons())) {
                    delay(10);
                }
                Serial.printf("Button pressed: %d\n", buttons);

                // Save the learned IR code to the corresponding button
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    if (buttons & (1 << i)) {
                        learned_codes[i] = Yboard.ir_results;
                        Serial.printf("Learned code for button %d\n", i + 1);
                    }
                }
            }
            delay(200);
            Yboard.clear_ir();
        }
    } else if (Yboard.get_switch(4)) {
        print_to_display("IR TX Learned Mode\n\nPress a button to replay the learned code.");

        if (Yboard.get_buttons()) {
            uint8_t buttons = Yboard.get_buttons();
            for (int i = 0; i < NUM_BUTTONS; ++i) {
                if ((buttons & (1 << i)) && learned_codes[i].decode_type != UNUSED) {
                    // Play back the learned code for this button
                    Yboard.set_all_leds_color(0, 255, 0);
                    bool result =
                        Yboard.ir_send.send(learned_codes[i].decode_type, learned_codes[i].value,
                                            learned_codes[i].bits, 0);

                    Serial.printf("Replaying code for button %d: ", i + 1);
                    Serial.println(resultToHumanReadableBasic(&learned_codes[i]));

                    if (!result) {
                        Serial.printf("Failed to replay code for button %d\n", i + 1);
                    }
                }
            }
            delay(1000);
            Yboard.set_all_leds_color(0, 0, 0);
        }

    } else {
        print_to_display("Flip switch 1, 2, 3, 4 to start");
        Yboard.set_all_leds_color(0, 0, 0);
    }
}
