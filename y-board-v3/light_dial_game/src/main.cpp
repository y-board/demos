
#include "Arduino.h"
#include "ResponsiveAnalogRead.h"
#include "yboard.h"

typedef struct {
    unsigned char r, g, b;
} Color;

int generate_random_number();
void play_correct_guess();
void play_bad_guess();
void play_win();
int filter(int value);
Color map_to_color(int value, int min_value, int max_value);
void delay_with_audio(int ms);
void wait_for_audio();

const int code_length = 5;
int secret_number;
int success_count = 0;
int level = 0;
int max_levels = 10;
int level_delays[] = {8000, 5000, 3000, 2000, 1000, 500, 400, 300, 200, 100};
ResponsiveAnalogRead analog(Yboard.knob_pin, true);

void secret_changing_task(void *pvParameters) {
    int previous_success_count = success_count;

    while (1) {
        if (Yboard.get_switch(1)) {
            delay(level_delays[level]);
            if (Yboard.get_switch(1) && previous_success_count == success_count) {
                secret_number = generate_random_number();
            }
            previous_success_count = success_count;
        }
        delay(50);
    }
}

void setup() {
    Serial.begin(9600);

    Yboard.setup();
    Yboard.set_sound_file_volume(2);
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.set_led_brightness(100);

    secret_number = generate_random_number();
    xTaskCreate(secret_changing_task, "Secret Changing Task", 1000, NULL, 1, NULL);
}

void loop() {
    analog.update();
    int led_value = map(analog.getValue(), 0, 2888, 20, 1);

    for (int i = 1; i <= Yboard.led_count; i++) {
        if (i == led_value) {
            Color c = map_to_color(abs(secret_number - i), 0, 10);
            Yboard.set_led_color(i, c.r, c.g, c.b);
        } else {
            Yboard.set_led_color(i, 0, 0, 0);
        }
    }

    if (Yboard.get_button(1) && led_value == secret_number) {
        success_count++;
        play_correct_guess();
        secret_number = generate_random_number();

        if (success_count == code_length) {
            play_win();
            level = min(level + 1, max_levels);

            // Reset the game
            success_count = 0;
        }

        while (Yboard.get_button(1))
            ;

    } else if (Yboard.get_button(1)) {
        play_bad_guess();
        success_count = 0;
    }

    if (Yboard.accelerometer_available()) {
        accelerometer_data data = Yboard.get_accelerometer();

        if (abs(data.x) > 1900 || abs(data.y) > 1900 || abs(data.z) > 1900) {
            play_bad_guess();
            success_count = 0;
        }
    }
}

int generate_random_number() {
    int new_rand;

    do {
        new_rand = random(1, Yboard.led_count + 1);
    } while (new_rand == secret_number);

    return new_rand;
}

void play_correct_guess() {
    char file_name[50];
    snprintf(file_name, 50, "/light_game/sm64_red_coin_%d.wav", success_count % (code_length + 1));
    Serial.println(file_name);

    if (!Yboard.play_sound_file_background(file_name)) {
        Yboard.play_notes_background("V2 T240 CE");
    }
    delay_with_audio(100);
    Yboard.set_all_leds_color(0, 255, 0);
    wait_for_audio();
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_bad_guess() {
    if (!Yboard.play_sound_file_background("/light_game/sm64_thwomp.wav")) {
        Yboard.play_notes_background("V2 T240 AC");
    }
    delay_with_audio(100);
    Yboard.set_all_leds_color(255, 0, 0);
    wait_for_audio();
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_win() {
    if (!Yboard.play_sound_file_background("/light_game/sm64_key_get.wav")) {
        Yboard.play_notes_background("V2 T240 CEGAFEDC");
    }

    for (int i = 1; i <= Yboard.led_count; i++) {
        Yboard.set_led_color(i, 0, 255, 0);
        delay_with_audio(100);
    }

    Yboard.set_all_leds_color(0, 0, 0);
    delay_with_audio(250);
    Yboard.set_all_leds_color(0, 255, 0);

    wait_for_audio();
    Yboard.set_all_leds_color(0, 0, 0);
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

void wait_for_audio() {
    while (Yboard.is_audio_playing()) {
        Yboard.loop_speaker();
    }
}
