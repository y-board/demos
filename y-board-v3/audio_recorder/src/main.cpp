
#include "Arduino.h"
#include "yboard.h"

static const std::string FILE_NAME = "/audio_recorder_demo.wav";
static const float smoothness_pts = 500;    // larger=slower change in brightness
static const float smoothness_gamma = 0.14; // affects the width of peak (more or less darkness)
static const float smoothness_beta = 0.5;   // shifts the gaussian to be symmetric

unsigned long previousMillis = 0; // Stores last time the action was performed
unsigned int smoothness_index = 0;

void setup() {
    Serial.begin(9600);
    Yboard.setup();
    Yboard.set_sound_file_volume(10);
    Yboard.set_recording_volume(3);
    Yboard.set_all_leds_color(255, 255, 255);
}

void loop() {
    unsigned long currentMillis = millis();

    if (Yboard.get_button(1)) {
        bool started_recording = Yboard.start_recording(FILE_NAME);
        while (Yboard.get_button(1)) {
            if (started_recording) {
                Yboard.set_all_leds_color(255, 0, 0);
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

        Yboard.set_all_leds_color(0, 0, 0);
    }

    if (Yboard.get_button(2)) {
        Yboard.set_all_leds_color(0, 0, 255);
        if (!Yboard.play_sound_file(FILE_NAME)) {
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
