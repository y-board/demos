#include "Arduino.h"
#include <yboard.h>

void displayWrappedText(const char *text, int maxLineLength, int startY);

constexpr int default_volume = 2;
constexpr int min_volume = 1;
constexpr int max_volume = 8;
static int last_volume = default_volume;

constexpr CRGB color1 = CRGB::White;
constexpr CRGB color2 = CRGB::Blue;

// Data structure for BYU fight song lyrics and timestamps
struct LyricLine {
    uint32_t time_ms; // Time in milliseconds from start of song
    const char *text;
};

const LyricLine fight_song_lyrics[] = {
    {6000, "Rise all loyal Cougars and hurl your challenge to the foe."},
    {12000, "You will fight, day or night, rain or snow."},
    {19000, "Loyal, strong, and true Wear the white and blue."},
    {25000, "While we sing, get set to spring. Come on Cougars it's up to you."},
    {30500, "Oh! Rise and shout, the Cougars are out along the trail to fame and glory."},
    {37000, "Rise and shout, our cheers will ring out As you unfold your victr'y story."},
    {43000, "On you go to vanquish the foe for Alma Mater's sons and daughters."},
    {49000, "As we join in song, in praise of you, our faith is strong."},
    {54000, "We'll raise our colors high in the blue And cheer our Cougars of BYU."},
    {61000, ""}};

const char instructions[] =
    "Use the knob to adjust volume. Press the center button to play/pause the BYU fight song.";

const size_t fight_song_lyrics_count = sizeof(fight_song_lyrics) / sizeof(fight_song_lyrics[0]);
static uint32_t song_start_time = 0;
static int current_lyric_index = -1;
static bool prev_center = false;

void setup() {
    Serial.begin(9600);
    Yboard.setup();

    // Set up display
    Yboard.display.clearDisplay();
    Yboard.display.setTextWrap(true);
    Yboard.display.setTextSize(1);

    // Set up speaker
    Yboard.set_sound_file_volume(default_volume);

    // Set up knob
    Yboard.set_knob(default_volume);

    Yboard.display.setCursor(0, 0);
    displayWrappedText(instructions, 20, 0);
    Yboard.display.display();
}

void loop() {
    // Deal with volume
    int new_volume = constrain(Yboard.get_knob(), min_volume, max_volume);
    if (new_volume != Yboard.get_knob()) {
        Yboard.set_knob(new_volume);
    }
    if (new_volume != last_volume) {
        Yboard.set_sound_file_volume(new_volume);
        last_volume = new_volume;
    }

    // Play fight song when button is pressed (edge-triggered)
    bool center = Yboard.get_button(Yboard.button_center);
    if (center && !prev_center) {
        if (!Yboard.is_audio_playing()) {
            Yboard.play_sound_file_background("fight_song.mp3");
            song_start_time = millis();
            current_lyric_index = -1;
            Yboard.display.clearDisplay();
            Yboard.display.display();
            Yboard.set_all_leds_color(0, 0, 255);
        } else {
            Yboard.stop_audio();
        }
    }
    prev_center = center;

    // Update lyrics display based on current playback time
    if (Yboard.is_audio_playing()) {
        uint32_t current_time = millis() - song_start_time;

        for (size_t i = 0; i < fight_song_lyrics_count; ++i) {
            if (current_time >= fight_song_lyrics[i].time_ms &&
                (i == fight_song_lyrics_count - 1 ||
                 current_time < fight_song_lyrics[i + 1].time_ms)) {
                if ((int)i != current_lyric_index) {
                    current_lyric_index = (int)i;
                    Yboard.display.clearDisplay();
                    displayWrappedText(fight_song_lyrics[i].text, 20, 0);
                    Yboard.display.display();
                }
                break;
            }
        }
    }

    // Control lights
    if (!Yboard.is_audio_playing()) {
        uint8_t wave = beatsin8(20, 0, 255);
        CRGB blended = blend(color1, color2, wave);
        fill_solid(Yboard.leds, Yboard.num_leds, blended);
        FastLED.show();
    }
}

// Helper function to wrap text at word boundaries for the display
void displayWrappedText(const char *text, int maxLineLength, int startY) {
    int len = strlen(text);
    int lineStart = 0;
    int y = startY;
    while (lineStart < len) {
        int lineEnd = lineStart + maxLineLength;
        if (lineEnd >= len) {
            lineEnd = len;
        } else {
            // Try to break at the last space within the line
            int lastSpace = -1;
            for (int i = lineStart; i < lineEnd; ++i) {
                if (text[i] == ' ') {
                    lastSpace = i;
                }
            }
            if (lastSpace > lineStart) {
                lineEnd = lastSpace;
            }
        }
        char buf[64];
        int copyLen = lineEnd - lineStart;
        strncpy(buf, text + lineStart, copyLen);
        buf[copyLen] = '\0';
        Yboard.display.setCursor(0, y);
        Yboard.display.println(buf);
        y += 8; // Move down for next line (adjust if needed for your font)
        // Skip any spaces at the start of the next line
        lineStart = lineEnd;
        while (text[lineStart] == ' ' && lineStart < len) {
            ++lineStart;
        }
    }
}
