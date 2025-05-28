#include <yboard.h>

#include "Arduino.h"
#include "colors.h"

static int rainbow_cycle_state = 0;
static int running_lights_state = 0;
static int color_wipe_state = 0;
static int rgb_loop_state = 0;
static int rgb_loop_direction = 1;

void rainbow_cycle();
void running_lights(byte red, byte green, byte blue, int WaveDelay);
void color_wipe(byte red, byte green, byte blue, int SpeedDelay);
void rgb_loop();

void setup() {
    Serial.begin(9600);
    Yboard.setup();
    Yboard.set_knob(120);
}

void loop() {
    // Update brightness of LEDs based on knob
    int brightness = Yboard.get_knob();
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 255) {
        brightness = 255;
    }
    Yboard.set_led_brightness(brightness);

    if (Yboard.get_switch(1) && Yboard.get_switch(2)) {
        rainbow_cycle();
    } else if (Yboard.get_switch(1) && !Yboard.get_switch(2)) {
        running_lights(255, 255, 255, 20);
    } else if (!Yboard.get_switch(1) && Yboard.get_switch(2)) {
        color_wipe(0, 0, 255, 25);
    } else {
        rgb_loop();
    }
}

void rainbow_cycle() {
    RGBColor c;

    for (int i = 1; i < Yboard.led_count + 1; i++) {
        c = color_wheel(((i * 256 / Yboard.led_count) + rainbow_cycle_state) & 255);
        Yboard.set_led_color(i, c.red, c.green, c.blue);
    }

    if (rainbow_cycle_state >= 256) {
        rainbow_cycle_state = 0;
    } else {
        rainbow_cycle_state++;
    }
}

void running_lights(byte red, byte green, byte blue, int WaveDelay) {
    running_lights_state++; // = 0; //Position + Rate;

    for (int i = 1; i < Yboard.led_count + 1; i++) {
        Yboard.set_led_color(i, ((sin(i + running_lights_state) * 127 + 128) / 255) * red,
                             ((sin(i + running_lights_state) * 127 + 128) / 255) * green,
                             ((sin(i + running_lights_state) * 127 + 128) / 255) * blue);
    }

    delay(WaveDelay);
}

void color_wipe(byte red, byte green, byte blue, int SpeedDelay) {
    if (color_wipe_state < Yboard.led_count + 1) {
        Yboard.set_led_color(color_wipe_state, red, green, blue);
    } else {
        Yboard.set_led_color(color_wipe_state - Yboard.led_count, 255, 255, 255);
    }

    if (color_wipe_state >= (Yboard.led_count * 2)) {
        color_wipe_state = 1;
    } else {
        color_wipe_state++;
    }

    delay(SpeedDelay);
}

void rgb_loop() {
    Yboard.set_all_leds_color(0, rgb_loop_state, rgb_loop_state);

    // Switch between increasing and decreasing
    if (rgb_loop_state >= 255 || rgb_loop_state <= 0) {
        rgb_loop_direction = !rgb_loop_direction;
    }

    if (rgb_loop_direction == 0) {
        rgb_loop_state++;
    } else {
        rgb_loop_state--;
    }

    delay(3);
}
