#include <Arduino.h>

#include "yboard.h"

/////////////////////////////// Constants //////////////////////////////////////

constexpr int REFRESH_RATE_MS = 50;

constexpr int PADDING = 2;
constexpr int TEXT_SIZE = 1;
constexpr int TEXT_LINE_HEIGHT = 8 * TEXT_SIZE;

constexpr int SWITCH_X = PADDING;
constexpr int SWITCH_Y = 15;
constexpr int SWITCH_WIDTH = 10;
constexpr int SWITCH_HEIGHT = 24;

constexpr int BUTTON_X = 58;
constexpr int BUTTON_Y = TEXT_LINE_HEIGHT + PADDING;
constexpr int BUTTON_WIDTH = 11; // Should be odd, since inner circle is odd width
constexpr int BUTTON_HEIGHT = BUTTON_WIDTH;
constexpr int BUTTON_GAP = 1;

constexpr int KNOB_RADIUS = 8;
constexpr int KNOB_X = Yboard.display_width - KNOB_RADIUS - PADDING * 2;
constexpr int KNOB_Y = 25;

constexpr int ACCEL_Y_TITLE = Yboard.display_height - TEXT_LINE_HEIGHT * 2;
constexpr int ACCEL_Y_DATA = ACCEL_Y_TITLE + TEXT_LINE_HEIGHT;

constexpr int ON = 1;
constexpr int OFF = 0;

/////////////////////////////// Globals ////////////////////////////////////////
int knob_x, knob_y;

/////////////////////////////// Forward Declarations ///////////////////////////
static void draw_buttons_state();
static void draw_switches_state();
static void draw_knob_state();
static void draw_accelerometer_data();
static void draw_text_titles();

/////////////////////////////// Main Functions /////////////////////////////////
void setup() {
    Serial.begin(115200);
    Yboard.setup();

    // Initialize variables
    int knob = Yboard.get_knob() * 5;
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;

    // Display setup
    delay(1000); // Display needs time to initialize
    Yboard.display.clearDisplay();
    Yboard.display.setTextColor(ON, OFF);
    Yboard.display.setTextWrap(false);
    Yboard.display.setTextSize(TEXT_SIZE);

    draw_text_titles();

    Yboard.set_led_brightness(70);
}

void loop() {
    // Update display
    draw_switches_state();
    draw_buttons_state();
    draw_knob_state();
    draw_accelerometer_data();
    Yboard.display.display();

    Yboard.set_all_leds_color(255, 255, 255);

    // Check if only center button is pressed
    if (Yboard.get_buttons() == (1 << (Yboard.button_center - 1))) {
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.println("Recording...");
        Yboard.display.display();

        bool started_recording = Yboard.start_recording("/hardware_test.wav");

        while (Yboard.get_button(Yboard.button_center)) {
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
            Yboard.display.setCursor(0, 0);
            Yboard.display.println("Playing...  ");
            Yboard.display.display();
            Yboard.set_all_leds_color(0, 255, 0);
            Yboard.play_sound_file("/hardware_test.wav");
            Yboard.set_all_leds_color(0, 0, 0);
            draw_text_titles();
            Yboard.display.display();
        }
    }

    // Refresh rate
    delay(REFRESH_RATE_MS);
}

/////////////////////////////// Helper Functions ///////////////////////////////
static void draw_buttons_state() {
    // Update button display states
    for (int i = 1; i <= Yboard.num_buttons; i++) {
        int pressed = Yboard.get_button(i);
        int x = BUTTON_X;
        int y = BUTTON_Y;
        if (i != Yboard.button_left) {
            x += BUTTON_WIDTH + BUTTON_GAP;
        }
        if (i == Yboard.button_right) {
            x += BUTTON_WIDTH + BUTTON_GAP;
        }
        if (i != Yboard.button_up) {
            y += BUTTON_HEIGHT + BUTTON_GAP;
        }
        if (i == Yboard.button_down) {
            y += BUTTON_HEIGHT + BUTTON_GAP;
        }

        Yboard.display.drawRect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, ON);

        if (pressed) {
            Yboard.display.fillCircle(x + BUTTON_WIDTH / 2, y + BUTTON_HEIGHT / 2,
                                      BUTTON_WIDTH / 2 - 2, ON);
        } else {
            Yboard.display.fillCircle(x + BUTTON_WIDTH / 2, y + BUTTON_HEIGHT / 2,
                                      BUTTON_WIDTH / 2 - 2, OFF);
        }
        Yboard.display.drawCircle(x + BUTTON_WIDTH / 2, y + BUTTON_HEIGHT / 2, BUTTON_WIDTH / 2 - 2,
                                  ON);
    }
}

static void draw_switches_state() {
    int current_switches = Yboard.get_switches();

    int inner_w = SWITCH_WIDTH - 4;
    int inner_h = SWITCH_HEIGHT * 0.3;

    for (int i = 0; i < Yboard.num_switches; i++) {
        int switch_state = (current_switches >> i) & 1;

        Yboard.display.drawRoundRect(SWITCH_X + (i * (SWITCH_WIDTH + PADDING)), SWITCH_Y,
                                     SWITCH_WIDTH, SWITCH_HEIGHT, 3, ON);

        int up_color = BLACK;
        int down_color = WHITE;

        if (switch_state) {
            up_color = WHITE;
            down_color = BLACK;
        }

        Yboard.display.fillRoundRect(SWITCH_X + 2 + (i * (SWITCH_WIDTH + PADDING)),
                                     SWITCH_Y + SWITCH_HEIGHT - inner_h - PADDING, inner_w, inner_h,
                                     2, down_color);
        Yboard.display.fillRoundRect(SWITCH_X + 2 + (i * (SWITCH_WIDTH + PADDING)),
                                     SWITCH_Y + PADDING, inner_w, inner_h, 2, up_color);
    }
}

static void draw_knob_state() {
    // Update knob display
    Yboard.display.fillCircle(knob_x, knob_y, 2, OFF);
    int knob = Yboard.get_knob() * 5;
    knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
    knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;

    if (Yboard.get_knob_button()) {
        Yboard.display.fillCircle(KNOB_X, KNOB_Y, KNOB_RADIUS - 2, ON);
    } else {
        Yboard.display.fillCircle(KNOB_X, KNOB_Y, KNOB_RADIUS - 2, OFF);
    }
    Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
    Yboard.display.fillCircle(knob_x, knob_y, 2, ON);
}

static void draw_accelerometer_data() {
    accelerometer_data accel_data = {0, 0, 0};

    // Update accelerometer data
    if (Yboard.accelerometer_available()) {
        accel_data = Yboard.get_accelerometer();
    }

    Yboard.display.setCursor(0, ACCEL_Y_DATA);
    Yboard.display.printf("x:%-5i", (int)accel_data.x);
    Yboard.display.setCursor(42, ACCEL_Y_DATA);
    Yboard.display.printf("y:%-5i", (int)accel_data.y);
    Yboard.display.setCursor(85, ACCEL_Y_DATA);
    Yboard.display.printf("z:%-5i", (int)accel_data.z);
}

static void draw_text_titles() {
    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Switches Buttons Knob");

    Yboard.display.setCursor(0, ACCEL_Y_TITLE);
    Yboard.display.println("Accelerometer (mg):");
}
