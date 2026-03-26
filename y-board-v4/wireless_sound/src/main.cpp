#include "yboard.h"
#include "yboard_helper.h"

// Classic kid-friendly sound effects stored on the SD card as /<name>.wav
const char *SOUNDS[] = {
    "laser",
    "explosion",
    "coin",
    "powerup",
    "gameover",
    "boing",
    "siren",
    "tada",
    "bark",
    "beep",
    "surprise",
};
const int NUM_SOUNDS = sizeof(SOUNDS) / sizeof(SOUNDS[0]);

int selected_sound = 0;

// Written by the ESP-NOW WiFi task callback, consumed by loop().
// Keep the callback short — only copy data and set the flag.
volatile bool sound_pending = false;
char pending_sound[33];

void on_string_received(const char *str) {
    strncpy(pending_sound, str, 32);
    pending_sound[32] = '\0';
    sound_pending = true;
}

// Draw the transmitter screen showing the currently selected sound.
void draw_tx_screen() {
    Yboard.display.clearDisplay();

    // Mode badge (top-left)
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(0, 0);
    Yboard.display.print("[TX]");

    // Sound index counter (top-right), e.g. "3/10"
    char idx_str[8];
    snprintf(idx_str, sizeof(idx_str), "%d/%d", selected_sound + 1, NUM_SOUNDS);
    Yboard.display.setCursor(Yboard.display_width - (int)strlen(idx_str) * 6, 0);
    Yboard.display.print(idx_str);

    // Sound name, large and centered
    Yboard.display.setTextSize(2);
    int name_w = strlen(SOUNDS[selected_sound]) * 12; // 12px per char at size 2
    int x = (Yboard.display_width - name_w) / 2;
    if (x < 0) {
        x = 0;
    }
    Yboard.display.setCursor(x, 24);
    Yboard.display.print(SOUNDS[selected_sound]);

    // Button hint (bottom)
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(0, 56);
    Yboard.display.print("^v:cycle  o:send");

    Yboard.display.display();
}

// Draw the receiver screen. Pass "Waiting..." when idle or a sound name when playing.
void draw_rx_screen(const char *label) {
    Yboard.display.clearDisplay();

    // Mode badge (top-left)
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(0, 0);
    Yboard.display.print("[RX]");

    // Status label, large and centered
    Yboard.display.setTextSize(2);
    int w = strlen(label) * 12;
    int x = (Yboard.display_width - w) / 2;
    if (x < 0) {
        x = 0;
    }
    Yboard.display.setCursor(x, 24);
    Yboard.display.print(label);

    Yboard.display.display();
}

bool prev_sw1 = false;
bool prev_up = false;
bool prev_down = false;
bool prev_center = false;

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(1);
    Yboard.set_led_brightness(100);

    yboard_radio_begin();
    yboard_radio_set_group(42);
    yboard_radio_on_string(on_string_received);

    prev_sw1 = Yboard.get_switch(1);
    if (prev_sw1) {
        draw_tx_screen();
    } else {
        draw_rx_screen("Waiting...");
    }
}

void loop() {
    bool sw1 = Yboard.get_switch(1);

    // Redraw and reset state whenever the mode switch changes
    if (sw1 != prev_sw1) {
        Yboard.set_all_leds_color(0, 0, 0);
        prev_up = false;
        prev_down = false;
        prev_center = false;
        if (sw1) {
            draw_tx_screen();
        } else {
            draw_rx_screen("Waiting...");
        }
    }

    if (sw1) {
        // ----- TX MODE -----
        bool up = Yboard.get_button(YBoardV4::button_up);
        bool down = Yboard.get_button(YBoardV4::button_down);
        bool center = Yboard.get_button(YBoardV4::button_center);

        if (up && !prev_up) {
            selected_sound = (selected_sound - 1 + NUM_SOUNDS) % NUM_SOUNDS;
            draw_tx_screen();
        } else if (down && !prev_down) {
            selected_sound = (selected_sound + 1) % NUM_SOUNDS;
            draw_tx_screen();
        }

        if (center && !prev_center) {
            yboard_radio_send_string(SOUNDS[selected_sound]);

            // Flash all LEDs green to confirm the send
            Yboard.set_all_leds_color(0, 200, 0);
            delay(200);
            Yboard.set_all_leds_color(0, 0, 0);
            draw_tx_screen();
        }

        prev_up = up;
        prev_down = down;
        prev_center = center;
    } else {
        // ----- RX MODE -----
        if (sound_pending) {
            sound_pending = false;

            // Show what's playing
            draw_rx_screen(pending_sound);

            // Light up LEDs blue while playing
            Yboard.set_all_leds_color(0, 100, 255);

            // Play the sound file from the SD card
            char filepath[48];
            snprintf(filepath, sizeof(filepath), "/%s.wav", pending_sound);
            Yboard.set_sound_file_volume(8);
            Yboard.play_sound_file(filepath);

            Yboard.set_all_leds_color(0, 0, 0);
            draw_rx_screen("Waiting...");
        }
    }

    prev_sw1 = sw1;
    delay(20);
}
