#include <Arduino.h>

#include "yboard.h"

// Screen Constants
#define REFRESH_RATE 50 // Measured in ms
#define SCREEN_WIDTH 128
#define PADDING 2
#define SWITCH_X 0 + PADDING * 2
#define SWITCH_Y 10
#define SWITCH_WIDTH 12
#define SWITCH_HEIGHT 24
#define BUTTON_Y SWITCH_Y + 30
#define BUTTON_WIDTH 17
#define BUTTON_HEIGHT 17
#define BUTTON_X SWITCH_X
#define KNOB_RADIUS 8
#define KNOB_X SCREEN_WIDTH - KNOB_RADIUS - PADDING * 4
#define KNOB_Y 25

#define ON 1
#define OFF 0

#define NUM_SWITCHES 4
#define NUM_BUTTONS 5

int x, y, z, knob, temp, knob_x, knob_y;

void update_display() {
    // Read current state of all inputs
    int current_switches = Yboard.get_switches();
    int current_buttons = Yboard.get_buttons();

    // Update switch display states
    for (int i = 0; i < NUM_SWITCHES; i++) {
        int switch_state = (current_switches >> i) & 1;

        Yboard.display.drawRoundRect(SWITCH_X + (i * (SWITCH_WIDTH + PADDING)), SWITCH_Y,
                                     SWITCH_WIDTH, SWITCH_HEIGHT, 3, ON);

        if (switch_state) {
            Yboard.display.fillRoundRect(SWITCH_X + PADDING + (i * (SWITCH_WIDTH + PADDING)),
                                         SWITCH_Y + PADDING + SWITCH_HEIGHT / 2, 8, 8, 3, OFF);
            Yboard.display.fillRoundRect(SWITCH_X + PADDING + (i * (SWITCH_WIDTH + PADDING)),
                                         SWITCH_Y + PADDING, 8, 8, 3, ON);
        } else {
            Yboard.display.fillRoundRect(SWITCH_X + PADDING + (i * (SWITCH_WIDTH + PADDING)),
                                         SWITCH_Y + PADDING, 8, 8, 3, OFF);
            Yboard.display.fillRoundRect(SWITCH_X + PADDING + (i * (SWITCH_WIDTH + PADDING)),
                                         SWITCH_Y + PADDING + SWITCH_HEIGHT / 2, 8, 8, 3, ON);
        }
    }

    // Update button display states
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int button_state = (current_buttons >> i) & 1;

        Yboard.display.drawRect(BUTTON_X + (i * (BUTTON_WIDTH + PADDING)), BUTTON_Y, BUTTON_WIDTH,
                                BUTTON_HEIGHT, ON);

        if (button_state) {
            Yboard.display.fillCircle(BUTTON_X + (i * (BUTTON_WIDTH + PADDING)) + BUTTON_WIDTH / 2,
                                      BUTTON_Y + BUTTON_HEIGHT / 2, 6, ON);
        } else {
            Yboard.display.fillCircle(BUTTON_X + (i * (BUTTON_WIDTH + PADDING)) + BUTTON_WIDTH / 2,
                                      BUTTON_Y + BUTTON_HEIGHT / 2, 6, OFF);
        }

        Yboard.display.drawCircle(BUTTON_X + (i * (BUTTON_WIDTH + PADDING)) + BUTTON_WIDTH / 2,
                                  BUTTON_Y + BUTTON_HEIGHT / 2, 6, ON);
    }

    // Update knob display
    Yboard.display.fillCircle(knob_x, knob_y, 2, OFF);
    knob = Yboard.get_knob() * 5;
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;

    if (Yboard.get_knob_button()) {
        Yboard.display.fillCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
    } else {
        Yboard.display.fillCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, OFF);
    }
    Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
    Yboard.display.fillCircle(knob_x, knob_y, 2, ON);

    // Update accelerometer data
    if (Yboard.accelerometer_available()) {
        accelerometer_data accel_data = Yboard.get_accelerometer();

        // Update X axis
        if (accel_data.x != x) {
            x = accel_data.x;
            Yboard.display.setCursor(0, 0);
            Yboard.display.printf("x:%i", x);
        }

        // Update Y axis
        if (accel_data.y != y) {
            y = accel_data.y;
            Yboard.display.setCursor(42, 0);
            Yboard.display.printf("y:%i", y);
        }

        // Update Z axis
        if (accel_data.z != z) {
            z = accel_data.z;
            Yboard.display.setCursor(85, 0);
            Yboard.display.printf("z:%i", z);
        }
    }

    Yboard.display.display();
}

void setup() {
    Serial.begin(9600);
    Yboard.setup();

    // Initialize variables
    x = y = z = temp = 0;
    knob = Yboard.get_knob() * 5;
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;

    // Display setup
    delay(1000); // Display needs time to initialize
    Yboard.display.clearDisplay();
    Yboard.display.setTextColor(ON, OFF);
    Yboard.display.setTextWrap(false);
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(0, 0);
    Yboard.display.printf("x:%i", x);
    Yboard.display.setCursor(42, 0);
    Yboard.display.printf("y:%i", y);
    Yboard.display.setCursor(85, 0);
    Yboard.display.printf("z:%i", z);

    Yboard.set_led_brightness(70);
}

void loop() {
    update_display();
    Yboard.set_all_leds_color(255, 255, 255);

    if (Yboard.get_button(4)) {
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.println("Recording...");
        Yboard.display.display();

        bool started_recording = Yboard.start_recording("/hardware_test.wav");

        while (Yboard.get_button(4)) {
            if (started_recording) {
                Yboard.set_all_leds_color(255, 0, 0);
                delay(100);
            } else {
                Yboard.set_all_leds_color(100, 100, 100);
                delay(100);
                Yboard.set_all_leds_color(0, 0, 0);
                delay(100);
            }
        }

        if (started_recording) {
            Yboard.stop_recording();
            delay(100);
            Yboard.set_all_leds_color(0, 255, 0);
            Yboard.play_sound_file("/hardware_test.wav");
            Yboard.set_all_leds_color(0, 0, 0);
        }
    }

    // Refresh rate
    delay(REFRESH_RATE);
}
