
#include "Arduino.h"
#include "yboard.h"

constexpr int num_bars = 16;
constexpr int bar_width = 6;
constexpr int max_bar_height = Yboard.display_height - 5;
constexpr float smoothness_pts = 500;    // larger=slower change in brightness
constexpr float smoothness_gamma = 0.14; // affects the width of peak (more or less darkness)
constexpr float smoothness_beta = 0.5;   // shifts the gaussian to be symmetric
const std::string file_name = "/audio_recorder_demo.wav";

unsigned long previousMillis = 0; // Stores last time the action was performed
unsigned int smoothness_index = 0;

void draw_animation();

void setup() {
    Serial.begin(9600);
    Yboard.setup();
    Yboard.set_sound_file_volume(10);
    Yboard.set_recording_volume(8);
    Yboard.set_all_leds_color(255, 255, 255);

    Yboard.display.println("Press the left button\n"
                           "to record and press\n"
                           "the right button to\n"
                           "play the recording.");
    Yboard.display.display();
}

void loop() {
    unsigned long currentMillis = millis();

    if (Yboard.get_button(1)) {
        bool started_recording = Yboard.start_recording(file_name);
        while (Yboard.get_button(1)) {
            if (started_recording) {
                Yboard.set_all_leds_color(255, 0, 0);
                draw_animation();
                delay(100);
            } else {
                Yboard.set_all_leds_color(255, 0, 0);
                delay(100);
                Yboard.set_all_leds_color(0, 0, 0);
                delay(100);
            }
        }

        if (started_recording) {
            delay(200); // Don't stop the recording immediately
            Yboard.stop_recording();
        }

        Yboard.display.clearDisplay();
        Yboard.display.display();
        Yboard.set_all_leds_color(0, 0, 0);
    }

    if (Yboard.get_button(2)) {
        Yboard.set_all_leds_color(0, 0, 255);
        if (!Yboard.play_sound_file(file_name)) {
            for (int i = 0; i < 3; i++) {
                Yboard.set_all_leds_color(255, 0, 0);
                delay(100);
                Yboard.set_all_leds_color(0, 0, 0);
                delay(100);
            }
        }
    }

    if (currentMillis - previousMillis >= 5) {
        // Save the last time you performed the action
        previousMillis = currentMillis;

        float pwm_val =
            255.0 *
            (exp(-(pow(((smoothness_index / smoothness_pts) - smoothness_beta) / smoothness_gamma,
                       2.0)) /
                 2.0));
        Yboard.set_all_leds_color(int(pwm_val), int(pwm_val), int(pwm_val));

        // Increment the index and reset if it exceeds the number of points
        smoothness_index++;
        if (smoothness_index >= smoothness_pts) {
            smoothness_index = 0;
        }
    }
}

void draw_animation() {
    Yboard.display.clearDisplay();

    Yboard.display.setTextSize(1);
    Yboard.display.fillCircle(3, 3, 2, WHITE);
    Yboard.display.drawChar(8, 0, 'R', WHITE, BLACK, 1);
    Yboard.display.drawChar(16, 0, 'E', WHITE, BLACK, 1);
    Yboard.display.drawChar(24, 0, 'C', WHITE, BLACK, 1);

    // Draw equalizer bars with random heights to simulate sound levels
    for (int i = 0; i < num_bars; i++) {
        int barHeight = random(5, max_bar_height); // Random height between 5 and max height
        int x = i * (bar_width + 2);               // Calculate x position of the bar

        // Draw the bar (bottom-up)
        Yboard.display.fillRect(x, Yboard.display_height - barHeight, bar_width, barHeight, WHITE);
    }

    Yboard.display.display();
}
