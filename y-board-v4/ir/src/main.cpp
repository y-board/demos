#include "yboard.h"
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>

#define IR_TX_PIN 7
#define IR_RX_PIN 36

IRrecv irrecv(IR_RX_PIN);
decode_results results;
IRsend irsend(IR_TX_PIN);

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    delay(1000); // Wait for Serial to initialize

    irrecv.enableIRIn();
    Serial.printf("IR Rx started\n");
    irsend.begin();
    Serial.printf("IR TX started\n");
}

void loop() {
    if (Yboard.get_switch(1)) {
        Yboard.set_all_leds_color(0, 255, 0);

        if (irrecv.decode(&results)) {
            Serial.println(resultToHumanReadableBasic(&results));
            Yboard.display.clearDisplay();
            Yboard.display.setCursor(0, 0);
            Yboard.display.println(resultToHumanReadableBasic(&results));
            Yboard.display.display();

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
            }

            delay(1000);
            Yboard.set_all_leds_color(0, 255, 0);

            irrecv.resume(); // Receive the next value
        }
    } else {
        Yboard.set_all_leds_color(0, 0, 0);

        // Print as binary
        // Serial.println(Yboard.get_buttons() & 0x1F, BIN);
        // delay(200);

        if (Yboard.get_buttons() & 0x1F) {
            Yboard.set_all_leds_color(255, 0, 0);
            irsend.sendNEC(Yboard.get_buttons() & 0x1F, 32);

            while (Yboard.get_buttons() & 0x1F) {
                delay(10);
            }
        }
    }
}
