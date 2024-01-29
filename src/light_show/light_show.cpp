#include "light_show.h"
#include "colors.h"

static int rainbow_cycle_state = 0;
static int running_lights_state = 0;
static int color_wipe_state = 0;
static int rgb_loop_state = 0;
static int rgb_loop_direction = 1;

void light_show_init() {
    Serial.begin(9600);
    leds_init();
    timer_init();
}

void light_show_loop() {
    // Update brightness of LEDs based on knob
    int brightness = map(knob_get(), 0, 100, 0, 255);
    leds_set_brightness(brightness);

    if (switches_get(1) && switches_get(2)) {
        rainbow_cycle();
    } else if (switches_get(1) && !switches_get(2)) {
        running_lights(255, 255, 255, 20);
    } else if (!switches_get(1) && switches_get(2)) {
        color_wipe(0, 0, 255, 25);
    } else {
        rgb_loop();
    }
}

void rainbow_cycle() {
    RGBColor c;

    for (int i = 1; i < LED_COUNT + 1; i++) {
        c = color_wheel(((i * 256 / LED_COUNT) + rainbow_cycle_state) & 255);
        leds_set_color(i, c.red, c.green, c.blue);
    }

    if (rainbow_cycle_state >= 256) {
        rainbow_cycle_state = 0;
    } else {
        rainbow_cycle_state++;
    }
}

void running_lights(byte red, byte green, byte blue, int WaveDelay) {
    running_lights_state++; // = 0; //Position + Rate;

    for (int i = 1; i < LED_COUNT + 1; i++) {
        leds_set_color(i, ((sin(i + running_lights_state) * 127 + 128) / 255) * red,
                       ((sin(i + running_lights_state) * 127 + 128) / 255) * green,
                       ((sin(i + running_lights_state) * 127 + 128) / 255) * blue);
    }

    delay(WaveDelay);
}

void color_wipe(byte red, byte green, byte blue, int SpeedDelay) {
    if (color_wipe_state < LED_COUNT + 1) {
        leds_set_color(color_wipe_state, red, green, blue);
    } else {
        leds_set_color(color_wipe_state - LED_COUNT, 255, 255, 255);
    }

    if (color_wipe_state >= (LED_COUNT * 2)) {
        color_wipe_state = 1;
    } else {
        color_wipe_state++;
    }

    delay(SpeedDelay);
}

void rgb_loop() {
    all_leds_set_color(0, rgb_loop_state, rgb_loop_state);

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
