#include "yboard.h"

int start_time = 0;
int done_time = 0;

void setup() {
    Yboard.setup();

    //////////////////// Queue up a bunch of notes to play ///////////

    // Place scale in Octave 5
    Yboard.play_notes("O5 T150 AA#BCC#DD#EFF#GG#");
    Serial.println("Done playing scale");

    // Yboard.set_sound_file_volume(10);
    Yboard.play_sound_file("/light_game/sm64_key_get.wav");
    Serial.println("Done playing song");

    Serial.println("Queueing up notes to play in the background");

    // Repeat the scale using the > notation
    Yboard.play_notes_background("O4 T150 A>A#>B>C>C#> O5 DD#EFF#GG#");

    // Rest, then play a few of the longest possible notes
    Yboard.play_notes_background("R1 T40 A1 B1 C1");

    // Play mary had a little lamb
    Yboard.play_notes_background("O4 T240 V2 EDCDEEE2DDD2EGG2EDCDEEEEDDEDC1");

    start_time = millis();
}

bool stopped = false;
bool played_after_stopping = false;
bool played_after_done = false;

void loop() {
    Yboard.loop_speaker();

    // Uncomment this to stop the audio after 5 seconds to test the stop functionality
    // and then start mary had a little lamb again 3 seconds later
    //
    // if ((millis() - start_time > 5000) && !stopped) {
    //     Yboard.stop_audio();
    //     stopped = true;
    // }
    // if ((millis() - start_time > 8000) & !played_after_stopping) {
    //     Yboard.play_notes("O4 T240 V2 EDCDEEE2DDD2EGG2EDCDEEEEDDEDC1");
    //     // Yboard.play_sound_file("/light_game/sm64_key_get.wav");

    //     played_after_stopping = true;
    // }

    // Uncomment this to test the is_audio_playing function
    //
    // // Wait for 5 seconds after the audio stops playing, and then play mary had a little lamb
    // again if (!Yboard.is_audio_playing() && done_time == 0) {
    //     Serial.println("Audio done playing");
    //     done_time = millis();
    // }
    // if ((millis() - done_time > 5000) && done_time != 0 && !played_after_done) {
    //     Serial.println("Playing again");
    //     Yboard.play_notes("O4 T240 V2 EDCDEEE2DDD2EGG2EDCDEEEEDDEDC1");
    //     played_after_done = true;
    // }
}
