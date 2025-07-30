#include "Arduino.h"
#include "colors.h"
#include <yboard.h>

static const std::string fight_song =
    "! O5 F D. C#8 D. C#8 D8 B8- O4 G8 F F8 G8 G8- F O5 B- C B- B2- A2";
static const std::string come_come_ye_saints = "!T100 O5 "
                                               "G4 A8 B8 C4 D4. E8 D4 C4 B4 A2 "
                                               "G4 G8 A8 B4 C4. D8 E4 D4 C4 B2 "
                                               "A4 A8 G8 F#4 G4. A8 B4 C4 D2 "
                                               "G4 A8 B8 C4 D4. E8 D4 C4 B4 A2 "
                                               "G4 A8 B8 C4 D4. E8 D4 C4 B4 A2 "
                                               "G4 G8 A8 B4 C4. D8 E4 D4 C4 B2 "
                                               "A4 A8 G8 F#4 G4. A8 B4 C4 D2 "
                                               "G4 A8 B8 C4 D1";
static int rainbow_cycle_state = 0;
static int running_lights_state = 0;
static int color_wipe_state = 0;
static int rgb_loop_state = 0;
static int rgb_loop_direction = 1;
static int delay_time = 4;

void rainbow_cycle(int delay_time);
void running_lights(byte red, byte green, byte blue, int WaveDelay);
void color_wipe(byte red, byte green, byte blue, int SpeedDelay);
void rgb_loop(int delay_time);

void setup() {
    Serial.begin(9600);
    Yboard.setup();
}

void loop() {
    // Play flight song when button is pressed
    if (Yboard.get_button(2)) {
        if (!Yboard.is_audio_playing()) {
            Yboard.play_notes_background(fight_song);
        } else {
            Yboard.stop_audio();
        }

        // Wait until button is released
        while (Yboard.get_button(2)) {
            delay(10);
        }
    }

    // Decrease delay_time when button 1 is pressed
    if (Yboard.get_button(1)) {
        if (delay_time > 1) {
            delay_time--;
        }

        // Wait until button is released
        while (Yboard.get_button(1)) {
            delay(10);
        }
    }

    // Increase delay_time when button 3 is pressed
    if (Yboard.get_button(3)) {
        delay_time++;

        // Wait until button is released
        while (Yboard.get_button(3)) {
            delay(10);
        }
    }

    // Play different LED effects based on switch states
    if (Yboard.get_switch(1) && Yboard.get_switch(2)) {
        rainbow_cycle(delay_time * 2);
    } else if (Yboard.get_switch(1) && !Yboard.get_switch(2)) {
        running_lights(255, 255, 255, delay_time * 8);
    } else if (!Yboard.get_switch(1) && Yboard.get_switch(2)) {
        color_wipe(0, 0, 255, delay_time * 5);
    } else {
        rgb_loop(delay_time * 2);
    }

    // Update brightness of LEDs based on knob
    int brightness = map(Yboard.get_knob(), 0, 100, 1, 255);
    Yboard.set_led_brightness(brightness);

    Yboard.loop_speaker();
}

void rainbow_cycle(int delay_time) {
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

    delay(delay_time);
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

void rgb_loop(int delay_time) {
    Yboard.set_all_leds_color(0, 0, rgb_loop_state);

    // Switch between increasing and decreasing
    if (rgb_loop_state >= 255 || rgb_loop_state <= 0) {
        rgb_loop_direction = !rgb_loop_direction;
    }

    if (rgb_loop_direction == 0) {
        rgb_loop_state++;
    } else {
        rgb_loop_state--;
    }

    delay(delay_time);
}
