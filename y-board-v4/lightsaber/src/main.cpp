#include <cmath>

#include "yboard.h"
#include "yboard_helper.h"

// Saber colors selected by switch combo: SW1 and SW2
// SW1 off, SW2 off = green | SW1 on, SW2 off = blue
// SW1 off, SW2 on  = red   | SW1 on, SW2 on  = purple
struct SaberColor {
    uint8_t r, g, b;
};

static const SaberColor COLOR_GREEN = {0, 255, 0};
static const SaberColor COLOR_BLUE = {0, 80, 255};
static const SaberColor COLOR_RED = {255, 0, 0};
static const SaberColor COLOR_PURPLE = {180, 0, 255};

// Accelerometer state for motion detection
float prev_magnitude = 0;
bool saber_on = false;

// Thresholds for swing detection (in mG)
const float SWING_THRESHOLD = 400;
const float HARD_SWING_THRESHOLD = 800;

// Timing
unsigned long last_hum_time = 0;
unsigned long swing_end_time = 0;
const unsigned long HUM_INTERVAL_MS = 1200;

SaberColor get_saber_color() {
    bool sw1 = Yboard.get_switch(1);
    bool sw2 = Yboard.get_switch(2);

    if (sw1 && sw2) {
        return COLOR_PURPLE;
    }
    if (sw2) {
        return COLOR_RED;
    }
    return sw1 ? COLOR_BLUE : COLOR_GREEN;
}

// Try to play a sound file, return true if successful
bool try_play_file(const char *name, bool background = false) {
    char path[48];

    snprintf(path, sizeof(path), "/lightsaber/%s.wav", name);
    Serial.printf("Trying: %s\n", path);
    if (background ? Yboard.play_sound_file_background(path) : Yboard.play_sound_file(path)) {
        Serial.println("  -> OK");
        return true;
    }

    snprintf(path, sizeof(path), "/lightsaber/%s.mp3", name);
    Serial.printf("Trying: %s\n", path);
    if (background ? Yboard.play_sound_file_background(path) : Yboard.play_sound_file(path)) {
        Serial.println("  -> OK");
        return true;
    }

    Serial.printf("  -> Not found: %s\n", name);
    return false;
}

void play_hum() { try_play_file("hum", true); }

void play_swing() { try_play_file("swing", true); }

void play_hard_swing() { try_play_file("clash", true); }

void play_power_on() { try_play_file("on", true); }

void play_power_off() { try_play_file("off", true); }

// Animate LEDs turning on one by one (saber extending)
void saber_extend(SaberColor color) {
    for (int i = 1; i <= Yboard.num_leds; i++) {
        Yboard.set_led_color(i, color.r, color.g, color.b);
        delay(40);
    }
}

// Animate LEDs turning off one by one (saber retracting)
void saber_retract() {
    for (int i = Yboard.num_leds; i >= 1; i--) {
        Yboard.set_led_color(i, 0, 0, 0);
        delay(40);
    }
}

// Flash LEDs brighter briefly for a swing effect
void swing_flash(SaberColor color, bool hard) {
    uint8_t boost = hard ? 255 : 200;
    uint8_t r = min((int)color.r + 60, (int)boost);
    uint8_t g = min((int)color.g + 60, (int)boost);
    uint8_t b = min((int)color.b + 60, (int)boost);

    for (int i = 1; i <= Yboard.num_leds; i++) {
        Yboard.set_led_color(i, r, g, b);
    }
}

void draw_display(const char *status) {
    Yboard.display.clearDisplay();

    Yboard.display.setTextSize(2);
    int w = (int)strlen("LIGHTSABER") * 12;
    int x = (Yboard.display_width - w) / 2;
    Yboard.display.setCursor(x > 0 ? x : 0, 0);
    Yboard.display.print("LIGHTSABER");

    Yboard.display.setTextSize(1);
    w = (int)strlen(status) * 6;
    x = (Yboard.display_width - w) / 2;
    Yboard.display.setCursor(x > 0 ? x : 0, 28);
    Yboard.display.print(status);

    Yboard.display.setCursor(0, 48);
    Yboard.display.print("Knob btn: on/off");
    Yboard.display.setCursor(0, 56);
    Yboard.display.print("Switches: color");

    Yboard.display.display();
}

bool prev_knob_btn = false;

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.set_sound_file_volume(8);
    Yboard.set_knob(150);
    Yboard.set_led_brightness(150);
    Yboard.set_all_leds_color(0, 0, 0);

    draw_display("Press knob to start");

    if (Yboard.accelerometer_available()) {
        prev_magnitude = yboard_accel_magnitude();
    }

    prev_knob_btn = Yboard.get_knob_button();
}

void loop() {
    // Knob controls LED brightness (0–200)
    int64_t knob = Yboard.get_knob();
    if (knob < 0) {
        knob = 0;
        Yboard.set_knob(0);
    } else if (knob > 200) {
        knob = 200;
        Yboard.set_knob(200);
    }
    Yboard.set_led_brightness((uint8_t)knob);

    bool knob_btn = Yboard.get_knob_button();

    // Toggle saber on/off with knob button press
    if (knob_btn && !prev_knob_btn) {
        if (!saber_on) {
            saber_on = true;
            SaberColor color = get_saber_color();
            draw_display("~ vwooom ~");
            play_power_on();
            saber_extend(color);
            last_hum_time = millis();
        } else {
            saber_on = false;
            Yboard.stop_audio();
            draw_display("Press knob to start");
            play_power_off();
            saber_retract();
        }
    }
    prev_knob_btn = knob_btn;

    if (!saber_on) {
        delay(50);
        return;
    }

    SaberColor color = get_saber_color();

    // Read accelerometer and detect motion
    if (Yboard.accelerometer_available()) {
        float magnitude = yboard_accel_magnitude();
        float delta = fabsf(magnitude - prev_magnitude);
        prev_magnitude = magnitude;

        if (delta > HARD_SWING_THRESHOLD) {
            // Hard swing / clash
            Yboard.stop_audio();
            play_hard_swing();
            swing_flash(color, true);
            swing_end_time = millis() + 300;
            draw_display("!! CLASH !!");
        } else if (delta > SWING_THRESHOLD) {
            // Normal swing
            Yboard.stop_audio();
            play_swing();
            swing_flash(color, false);
            swing_end_time = millis() + 200;
            draw_display("~ swoosh ~");
        }
    }

    // Restore normal saber glow after swing flash
    if (swing_end_time > 0 && millis() > swing_end_time) {
        for (int i = 1; i <= Yboard.num_leds; i++) {
            Yboard.set_led_color(i, color.r, color.g, color.b);
        }
        swing_end_time = 0;
        draw_display("~ vwooom ~");
    }

    // Keep the idle hum going — only restart after the interval to avoid hammering the audio system
    if (millis() - last_hum_time > HUM_INTERVAL_MS && millis() > swing_end_time + 100) {
        play_hum();
        last_hum_time = millis();
    }

    // Subtle LED flicker for ambiance
    if (swing_end_time == 0) {
        int flicker = random(-15, 15);
        for (int i = 1; i <= Yboard.num_leds; i++) {
            uint8_t r = constrain((int)color.r + flicker, 0, 255);
            uint8_t g = constrain((int)color.g + flicker, 0, 255);
            uint8_t b = constrain((int)color.b + flicker, 0, 255);
            Yboard.set_led_color(i, r, g, b);
        }
    }

    delay(30);
}
