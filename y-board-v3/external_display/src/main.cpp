#include <yboard.h>

int old_knob_value = 0;

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(2);
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

    if (knob_value != old_knob_value) {
        // Only update the display if something has changed
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 10);
        Yboard.display.printf("Knob: %d\n", knob_value);
        Yboard.display.display();

        Serial.printf("Knob value: %d\n", knob_value);

        old_knob_value = knob_value;
    }

    delay(50);
}
