
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <yboard.h>

Adafruit_SSD1306 display(128, 64); // Create display

int old_knob_value = 0;
int char_count = 0;

void setup() {
    Serial.begin(115200);
    Yboard.setup();

    Serial.println("Drawing test on display");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
        Serial.println("Error initializing display");
        return;
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setRotation(90);
    display.setTextWrap(true);

    display.setTextSize(1);
    display.clearDisplay();
    display.setCursor(20, 40);

    display.drawRoundRect(0, 0, 128, 64, 5, WHITE);
    display.display();

    Serial.println("Done drawing test on display");
}

int filter(int value) {
    static int previous_value = 0;

    if (abs(value - previous_value) > 1) {
        previous_value = value;
        return value;
    } else {
        return previous_value;
    }
}

int get_knob_value() {
    int knob_values[10];
    int sum = 0;

    for (int i = 0; i < 10; i++) {
        knob_values[i] = analogRead(Yboard.knob_pin);
        sum += knob_values[i];
        delay(10);
    }

    int average_knob_value = sum / 10;
    return average_knob_value;
}

void loop() {
    int knob_value = get_knob_value();

    // Remove oscillation of knob value by filtering

    if (knob_value != old_knob_value) {
        // Only update the display if something has changed
        display.drawChar(char_count * 8 + 4, 4, (char)knob_value, WHITE, BLACK, 1);
        display.display();

        Serial.printf("Knob value: %d\n", knob_value);

        old_knob_value = knob_value;
    }

    if (Yboard.get_button(1)) {
        char_count++;

        while (Yboard.get_button(1)) {
            // Wait for button to be released
            delay(10);
        }
    }

    delay(50);
}
