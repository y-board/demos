
#include "Arduino.h"
#include "yboard.h"

void setup() {
    Serial.begin(9600);
    Yboard.setup();
}

void loop() {
    if (Yboard.get_switch(1)) {
        Yboard.set_led_color(1, 255, 0, 0);
    } else {
        Yboard.set_led_color(1, 0, 0, 0);
    }

    if (Yboard.get_switch(2)) {
        Yboard.set_led_color(2, 255, 0, 0);
    } else {
        Yboard.set_led_color(2, 0, 0, 0);
    }

    if (Yboard.get_button(1)) {
        Yboard.set_all_leds_color(255, 255, 255);
        Yboard.play_notes("O5 T150 AA#B");
        Yboard.set_all_leds_color(0, 0, 0);
    }

    if (Yboard.get_button(2)) {
        Yboard.set_recording_volume(3);
        bool started_recording = Yboard.start_recording("/hardware_test.wav");
        while (Yboard.get_button(2)) {
            if (started_recording) {
                Yboard.set_all_leds_color(255, 0, 0);
                delay(100);
            } else {
                Yboard.set_all_leds_color(100, 100, 100);
                delay(100);
                Yboard.set_all_leds_color(0, 0, 0);
                delay(100);
            }
        }

        if (started_recording) {
            delay(200); // Don't stop the recording immediately
            Yboard.stop_recording();
            delay(100);
            Yboard.set_all_leds_color(0, 255, 0);
            Yboard.set_sound_file_volume(10);
            Yboard.play_sound_file("/hardware_test.wav");
            Yboard.set_all_leds_color(0, 0, 0);
            Yboard.set_sound_file_volume(5);
        }
    }

    Yboard.set_led_color(5, map(Yboard.get_knob(), 0, 100, 0, 255), 0, 0);

    if (Yboard.accelerometer_available()) {
        accelerometer_data accel_data = Yboard.get_accelerometer();
        Serial.printf("x: %f, y: %f, z: %f\n", accel_data.x, accel_data.y, accel_data.z);
        Yboard.set_led_color(6, map(accel_data.x, -1000, 1000, 0, 255), 0, 0);
        Yboard.set_led_color(7, map(accel_data.y, -1000, 1000, 0, 255), 0, 0);
        Yboard.set_led_color(8, map(accel_data.z, -1000, 1000, 0, 255), 0, 0);
    }

    temperature_data temp_data = Yboard.get_temperature();
    if (temp_data.temperature > 28) {
        Yboard.set_led_color(9, 255, 0, 0);
    } else {
        Yboard.set_led_color(9, 0, 0, 0);
    }

    if (temp_data.humidity > 50) {
        Yboard.set_led_color(10, 255, 0, 0);
    } else {
        Yboard.set_led_color(10, 0, 0, 0);
    }
    Serial.printf("temp: %f, humidity: %f\n", temp_data.temperature, temp_data.humidity);
}
