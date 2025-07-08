#include <Arduino.h>

#include "yboard.h"

// Screen Constants
#define BRIGHTNESS_DAMPER 0.8 // 0 is brightest
#define REFRESH_RATE 50       // Measured in ms
#define SCREEN_WIDTH 128
#define ZERO_DEG 0
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
#define KNOB_X SCREEN_WIDTH - KNOB_RADIUS - PADDING * 2
#define KNOB_Y 20

#define ON 1
#define OFF 0

#define NUM_SWITCHES 4
#define NUM_BUTTONS 5

int x, y, z, knob, temp, knob_x, knob_y;
int switches[NUM_SWITCHES] = {0};
int buttons[NUM_BUTTONS] = {0};
int prev_switches = 0;
int prev_buttons = 0;

void update_display() {
    // Read current state of all inputs
    int current_switches = Yboard.get_switches();
    int current_buttons = Yboard.get_buttons();
    int current_knob = map(Yboard.get_knob(), 0, 100, 0, 255);

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

    // // Update knob display
    Yboard.display.fillCircle(knob_x, knob_y, 2, OFF);
    knob = Yboard.get_knob();
    Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;
    Yboard.display.fillCircle(knob_x, knob_y, 2, ON);

    Yboard.display.display();
}

void setup() {
    Serial.begin(9600);
    Yboard.setup();
    x = 0;
    y = 0;
    z = 0;

    // // Initialize buttons and switches using batch functions
    // prev_buttons = Yboard.get_buttons();
    // prev_switches = Yboard.get_switches();

    // // Extract individual button and switch states
    // for (int i = 0; i < NUM_BUTTONS; i++) {
    //     buttons[i] = (prev_buttons >> i) & 1;
    // }
    // for (int i = 0; i < NUM_SWITCHES; i++) {
    //     switches[i] = (prev_switches >> i) & 1;
    // }

    knob = map(Yboard.get_knob(), 0, 100, 0, 255);
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;
    temp = 0;

    // Display setup
    delay(1000); // Display needs time to initialize
    Yboard.display.setTextWrap(false);
    uint8_t text_size = 1;
    Yboard.display.setTextSize(text_size);
    Yboard.display.setCursor(0, 0);
    Yboard.display.printf("x:%i", x);
    Yboard.display.setCursor(42, 0);
    Yboard.display.printf("y:%i", y);
    Yboard.display.setCursor(85, 0);
    Yboard.display.printf("z:%i", z);

    update_display();

    // // Draw switch 1
    // Yboard.display.drawRoundRect(SWITCH_X, SWITCH_Y, SWITCH_WIDTH, SWITCH_HEIGHT, 3, ON);
    // if (switches[0]) {
    //     Yboard.display.fillRoundRect(SWITCH_X + PADDING, SWITCH_Y + PADDING, 8, 8, 3, ON);
    // } else {
    //     Yboard.display.fillRoundRect(SWITCH_X + PADDING, SWITCH_Y + PADDING + SWITCH_HEIGHT / 2,
    //     8,
    //                                  8, 3, ON);
    // }

    // // Draw switch 2
    // Yboard.display.drawRoundRect(SWITCH_X + SWITCH_WIDTH + PADDING, SWITCH_Y, SWITCH_WIDTH,
    //                              SWITCH_HEIGHT, 3, ON);
    // if (switches[1]) {
    //     Yboard.display.fillRoundRect(SWITCH_X + SWITCH_WIDTH + PADDING * 2, SWITCH_Y + PADDING,
    //     8,
    //                                  8, 3, ON);
    // } else {
    //     Yboard.display.fillRoundRect(SWITCH_X + SWITCH_WIDTH + PADDING * 2,
    //                                  SWITCH_Y + PADDING + SWITCH_HEIGHT / 2, 8, 8, 3, ON);
    // }

    // // Draw button 1
    // Yboard.display.drawRect(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, ON);
    // if (buttons[0]) {
    //     Yboard.display.fillCircle(BUTTON_X + BUTTON_WIDTH / 2, BUTTON_Y + BUTTON_HEIGHT / 2 - 1,
    //     6,
    //                               ON);
    // } else {
    //     Yboard.display.drawCircle(BUTTON_X + BUTTON_WIDTH / 2, BUTTON_Y + BUTTON_HEIGHT / 2, 6,
    //     ON);
    // }

    // // Draw button 2
    // Yboard.display.drawRect(BUTTON_X + BUTTON_WIDTH + PADDING, BUTTON_Y, BUTTON_WIDTH,
    //                         BUTTON_HEIGHT, ON);
    // if (buttons[1]) {
    //     Yboard.display.fillCircle(BUTTON_X + BUTTON_WIDTH + PADDING + BUTTON_WIDTH / 2,
    //                               BUTTON_Y + BUTTON_HEIGHT / 2, 6, ON);
    // } else {
    //     Yboard.display.drawCircle(BUTTON_X + BUTTON_WIDTH + PADDING + BUTTON_WIDTH / 2,
    //                               BUTTON_Y + BUTTON_HEIGHT / 2, 6, ON);
    // }

    // Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
    // Yboard.display.display();
}

void loop() {
    update_display();
    // // Get current state of all switches and buttons
    // int current_switches = Yboard.get_switches();
    // int current_buttons = Yboard.get_buttons();

    // // Test switches using for loop
    // for (int i = 0; i < NUM_SWITCHES; i++) {
    //     int current_switch = (current_switches >> i) & 1;
    //     if (current_switch && !switches[i]) {
    //         // Switch turned on
    //         Yboard.set_led_color(i + 1, 255, 255, 255);
    //     } else if (!current_switch && switches[i]) {
    //         // Switch turned off
    //         Yboard.set_led_color(i + 1, 0, 0, 0);
    //     }
    // }

    // // Test buttons using for loop
    // for (int i = 0; i < NUM_BUTTONS; i++) {
    //     int current_button = (current_buttons >> i) & 1;
    //     if (current_button && !buttons[i]) {
    //         // Button pressed
    //         // Handle button 1 (index 0)
    //         if (i == 0) {
    //             Yboard.set_all_leds_color(255, 255, 255);
    //             Yboard.play_notes("O5 F D. C#8 D. C#8 D8 B8- O4 G8 F"); // Play fight song
    //             Yboard.set_all_leds_color(0, 0, 0);
    //         }
    //         // Handle button 2 (index 1)
    //         else if (i == 1) {
    //             Yboard.set_recording_volume(3);
    //             bool started_recording = Yboard.start_recording("/hardware_test.wav");
    //             while ((Yboard.get_buttons() >> 1) & 1) { // Check if button 2 is still pressed
    //                 if (started_recording) {
    //                     Yboard.set_all_leds_color(255, 0, 0);
    //                     delay(100);
    //                 } else {
    //                     Yboard.set_all_leds_color(100, 100, 100);
    //                     delay(100);
    //                     Yboard.set_all_leds_color(0, 0, 0);
    //                     delay(100);
    //                 }
    //             }

    //             if (started_recording) {
    //                 delay(200); // Don't stop the recording immediately
    //                 Yboard.stop_recording();
    //                 delay(100);
    //                 Yboard.set_all_leds_color(0, 255, 0);
    //                 Yboard.set_sound_file_volume(10);
    //                 Yboard.play_sound_file("/hardware_test.wav");
    //                 Yboard.set_all_leds_color(0, 0, 0);
    //                 Yboard.set_sound_file_volume(5);
    //             }
    //         }
    //     } else if (!current_button && buttons[i]) {
    //         // Button released
    //         // Handle button 1 (index 0)
    //         if (i == 0) {
    //             Yboard.set_all_leds_color(0, 0, 0);
    //         }
    //         // Handle button 2 (index 1)
    //         else if (i == 1) {
    //             Yboard.set_all_leds_color(0, 0, 0);
    //         }
    //     }
    // }

    // // Update knob LED
    // int current_knob = map(Yboard.get_knob(), 0, 100, 0, 255);
    // Yboard.set_led_color(5, current_knob, current_knob, current_knob);

    // // Update display with current states
    // update_display();

    // // Test accelerometer
    // if (Yboard.accelerometer_available()) {
    //     accelerometer_data accel_data = Yboard.get_accelerometer();
    //     temp = accel_data.x;
    //     if (temp != x) {
    //         x = temp;
    //         if (x > 999) {
    //             x = 999;
    //         } else if (x < -999) {
    //             x = -999;
    //         }
    //         if (x < 0) {
    //             Yboard.set_led_color(6, map(x, 0, -999, 0, 255), 0, 0);
    //         } else {
    //             Yboard.set_led_color(6, 0, map(x, 0, 999, 0, 255), map(x, 0, 999, 0, 255));
    //         }
    //         Yboard.display.setCursor(10, 0);
    //         Yboard.display.printf("%04i", x);
    //     }

    //     temp = accel_data.y;
    //     if (temp != y) {
    //         y = temp;
    //         if (y > 999) {
    //             y = 999;
    //         } else if (y < -999) {
    //             y = -999;
    //         }
    //         if (y < 0) {
    //             Yboard.set_led_color(7, map(y, 0, -999, 0, 255), 0, map(y, 0, -999, 0, 255));
    //         } else {
    //             Yboard.set_led_color(7, 0, map(y, 0, 999, 0, 255), 0);
    //         }
    //         Yboard.display.setCursor(42 + 10, 0);
    //         Yboard.display.printf("%04i", y);
    //     }

    //     temp = accel_data.z;
    //     if (temp != z) {
    //         z = temp;
    //         if (z > 999) {
    //             z = 999;
    //         } else if (z < -999) {
    //             z = -999;
    //         }
    //         if (z < 0) {
    //             Yboard.set_led_color(8, map(z, 0, -999, 0, 255), map(z, 0, -999, 0, 255), 0);
    //         } else {
    //             Yboard.set_led_color(8, 0, 0, map(z, 0, 999, 0, 255));
    //         }
    //         Yboard.display.setCursor(85 + 10, 0);
    //         Yboard.display.printf("%04i", z);
    //     }
    // }
    // Yboard.display.display(); // Draw on display
}
