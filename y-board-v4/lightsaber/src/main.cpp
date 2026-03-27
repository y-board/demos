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
    if (background ? Yboard.play_sound_file_background(path)
                   : Yboard.play_sound_file(path)) {
        return true;
    }

    snprintf(path, sizeof(path), "/lightsaber/%s.mp3", name);
    if (background ? Yboard.play_sound_file_background(path)
                   : Yboard.play_sound_file(path)) {
        return true;
    }

    return false;
}

// Play the idle hum sound. Tries a sound file first, falls back to notes.
void play_hum() {
    if (try_play_file("hum", true)) {
        return;
    }
    // Low continuous buzz as fallback
    Yboard.play_notes_background("V3 O4 T60 C2 C2 C2 C2");
}

// Play a swing sound. Tries a sound file first, falls back to notes.
void play_swing() {
    if (try_play_file("swing", true)) {
        return;
    }
    Yboard.play_notes_background("V5 O5 T240 C+D+F+G+A+");
}

// Play a hard/clash swing sound.
void play_hard_swing() {
    if (try_play_file("clash", true)) {
        return;
    }
    Yboard.play_notes_background("V7 O6 T240 E+F+G+A+B+");
}

// Play power-on sound
void play_power_on() {
    if (try_play_file("on", false)) {
        return;
    }
    Yboard.play_notes("V5 O4 T120 C8 E8 G8 O5 C4");
}

// Play power-off sound
void play_power_off() {
    if (try_play_file("off", false)) {
        return;
    }
    Yboard.play_notes("V5 O5 T120 C8 O4 G8 E8 C4");
}

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
    Yboard.set_led_brightness(150);
    Yboard.set_all_leds_color(0, 0, 0);

    draw_display("Press knob to start");

    if (Yboard.accelerometer_available()) {
        prev_magnitude = yboard_accel_magnitude();
    }

    prev_knob_btn = Yboard.get_knob_button();
}

void loop() {
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
        float delta = abs(magnitude - prev_magnitude);
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

    // Keep the idle hum going
    if (!Yboard.is_audio_playing() || (millis() - last_hum_time > HUM_INTERVAL_MS)) {
        if (millis() > swing_end_time + 100) {
            play_hum();
            last_hum_time = millis();
        }
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
