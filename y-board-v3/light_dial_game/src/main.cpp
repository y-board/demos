
#include "Arduino.h"
#include "yboard.h"

#define CODE_LENGTH 3

typedef struct {
    unsigned char r, g, b;
} Color;

void generate_secret_code();
void play_correct_guess();
void play_bad_guess();
void play_win();
int filter(int value);
Color map_to_color(int value, int min_value, int max_value);
void delay_with_audio(int ms);

int secret_code[CODE_LENGTH];
int success_count = 0;

void setup() {
    Serial.begin(9600);

    Yboard.setup();
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.set_led_brightness(100);

    generate_secret_code();
}

void loop() {
    int led_value = map(filter(Yboard.get_knob()), 0, 100, 20, 1);

    for (int i = 1; i <= Yboard.led_count; i++) {
        if (i == led_value) {
            Color c = map_to_color(abs(secret_code[success_count] - i), 0, 10);
            Yboard.set_led_color(i, c.r, c.g, c.b);
        } else {
            Yboard.set_led_color(i, 0, 0, 0);
        }
    }

    if (Yboard.get_button(1) && led_value == secret_code[success_count]) {
        success_count++;
        play_correct_guess();

        if (success_count == CODE_LENGTH) {
            play_win();

            // Reset the game
            success_count = 0;
            generate_secret_code();
        }

        while (Yboard.get_button(1))
            ;

    } else if (Yboard.get_button(1)) {
        play_bad_guess();
        success_count = 0;
    }
}

void generate_secret_code() {
    for (int i = 0; i < CODE_LENGTH; i++) {
        secret_code[i] = random(1, 21);
    }
}

void play_correct_guess() {
    char file_name[50];
    snprintf(file_name, 50, "/light_game/sm64_red_coin_%d.wav", success_count % (CODE_LENGTH + 1));
    Serial.println(file_name);

    Yboard.play_song_from_sd(file_name);
    Yboard.set_all_leds_color(0, 255, 0);
    delay_with_audio(800);
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_bad_guess() {
    Yboard.play_song_from_sd("/light_game/sm64_thwomp.wav");
    delay_with_audio(100);
    Yboard.set_all_leds_color(255, 0, 0);
    delay_with_audio(900);
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_win() {
    Yboard.play_song_from_sd("/light_game/sm64_key_get.wav");
    for (int i = 0; i < CODE_LENGTH; i++) {
        Yboard.set_led_color(secret_code[i], 0, 255, 0);
        delay_with_audio(250);
        Yboard.set_led_color(secret_code[i], 0, 0, 0);
        delay_with_audio(250);
    }

    for (int i = 1; i <= Yboard.led_count; i++) {
        Yboard.set_led_color(i, 0, 255, 0);
        delay_with_audio(80);
    }
    delay_with_audio(500);
    Yboard.set_all_leds_color(0, 0, 0);
}

// Alpha filter for knob value
int filter(int value) {
    float alpha = 0.5;
    static float filtered_value = 0;

    filtered_value = alpha * value + (1 - alpha) * filtered_value;
    return round(filtered_value);
}

Color map_to_color(int value, int min_value, int max_value) {
    Color color;

    if (value < min_value) {
        value = min_value;
    }
    if (value > max_value) {
        value = max_value;
    }

    double range = max_value - min_value;
    double position = (value - min_value) / range;

    color.r = (unsigned char)((1 - position) * 255);
    color.g = 0;
    color.b = (unsigned char)(position * 255);

    return color;
}

void delay_with_audio(int ms) {
    unsigned long start_time = millis();
    while (millis() - start_time < ms) {
        Yboard.loop_speaker();
    }
}
