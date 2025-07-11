#include "yboard.h"

typedef struct {
    unsigned char r, g, b;
} Color;

static const unsigned char PROGMEM image_checked_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x1c, 0x80, 0x38, 0xc0, 0x70,
    0xe0, 0xe0, 0x71, 0xc0, 0x3b, 0x80, 0x1f, 0x00, 0x0e, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};

int generate_random_number();
void play_correct_guess();
void play_bad_guess();
void play_win();
int filter(int value);
Color map_to_color(int value, int min_value, int max_value);
void wait_for_audio();
bool button_pressed();
void update_display();

const int code_length = 5;
int secret_number;
int success_count = 0;
int level = 0;
int max_levels = 10;
int level_delays[] = {8000, 5000, 3000, 2000, 1000, 500, 400, 300, 200, 100};

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
    Serial.begin(115200);

    Yboard.setup();
    Yboard.set_sound_file_volume(2);
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.set_led_brightness(100);

    secret_number = generate_random_number();
    xTaskCreate(secret_changing_task, "Secret Changing Task", 2048, NULL, 1, NULL);

    Yboard.display.setTextSize(2);
    update_display();
}

void loop() {
    int knob_value = -1 * Yboard.get_knob();
    int led_value = (knob_value % Yboard.num_leds + Yboard.num_leds) % Yboard.num_leds + 1;

    for (int i = 1; i <= Yboard.num_leds; i++) {
        int dist = abs(secret_number - i);
        int wrap_dist = (Yboard.num_leds - 1) - dist;
        int min_dist = min(dist, wrap_dist);
        if (i == led_value) {
            Color c = map_to_color(min_dist, 0, (Yboard.num_leds - 1) / 2);
            Yboard.set_led_color(i, c.r, c.g, c.b);
        } else {
            Yboard.set_led_color(i, 0, 0, 0);
        }
    }

    if (button_pressed()) {
        if (led_value == secret_number) {
            success_count++;
            play_correct_guess();
            secret_number = generate_random_number();
            update_display();

            if (success_count == code_length) {
                play_win();
                level = min(level + 1, max_levels);

                // Reset the game
                success_count = 0;
            }
        } else {
            play_bad_guess();
            success_count = 0;
        }
        update_display();

        while (button_pressed())
            ;
    }

    if (Yboard.accelerometer_available()) {
        accelerometer_data data = Yboard.get_accelerometer();

        if (abs(data.x) > 1800 || abs(data.y) > 1800 || abs(data.z) > 1800) {
            play_bad_guess();
            success_count = 0;
            update_display();
        }
    }
}

bool button_pressed() { return Yboard.get_button(4) || Yboard.get_knob_button(); }

void update_display() {
    Yboard.display.clearDisplay();

    Yboard.display.setCursor(0, 0);
    Yboard.display.printf("Level %d", level + 1);

    for (int i = 0; i < success_count; i++) {
        Yboard.display.drawBitmap(i * 20, 22, image_checked_bits, 14, 16, 1);
    }

    Yboard.display.display();
}

int generate_random_number() {
    int new_rand;

    do {
        new_rand = random(1, Yboard.num_leds);
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
    delay(100);
    Yboard.set_all_leds_color(0, 255, 0);
    wait_for_audio();
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_bad_guess() {
    if (!Yboard.play_sound_file_background("/light_game/sm64_thwomp.wav")) {
        Yboard.play_notes_background("V2 T240 AC");
    }
    delay(100);
    Yboard.set_all_leds_color(255, 0, 0);
    wait_for_audio();
    Yboard.set_all_leds_color(0, 0, 0);
}

void play_win() {
    if (!Yboard.play_sound_file_background("/light_game/sm64_key_get.wav")) {
        Yboard.play_notes_background("V2 T240 CEGAFEDC");
    }

    for (int i = 1; i <= Yboard.num_leds; i++) {
        Yboard.set_led_color(i, 0, 255, 0);
        delay(50);
    }

    Yboard.set_all_leds_color(0, 0, 0);
    delay(250);
    Yboard.set_all_leds_color(0, 255, 0);
    delay(250);
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

void wait_for_audio() {
    while (Yboard.is_audio_playing()) {
        delay(1);
    }
}
