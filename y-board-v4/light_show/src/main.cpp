#include <yboard.h>

#define FRAMES_PER_SECOND 120
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define SHOW_PATTERN_NAME_TIME 1500

void rainbow();
void rainbowWithGlitter();
void addGlitter(fract8 chanceOfGlitter);
void confetti();
void sinelon();
void juggle();
void bpm();
void nextPattern();
void previousPattern();
void showPatternName();
void waitForButtonRelease();

// List of patterns to cycle through. Each is defined as a separate function below.
typedef void (*PatternFunction)();
struct Pattern {
    PatternFunction func;
    const char *name;
};

Pattern gPatterns[] = {{rainbow, "Rainbow"},   {rainbowWithGlitter, "Rainbow + Glitter"},
                       {confetti, "Confetti"}, {sinelon, "Sinelon"},
                       {juggle, "Juggle"},     {bpm, "BPM"}};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // rotating "base color" used by many of the patterns
bool clearDisplay = false;         // Flag to clear the display on the next update
unsigned long displayTime = 0;     // Time to clear the display after showing pattern name

void setup() {
    Serial.begin(9600);
    Yboard.setup();
    Yboard.set_knob(120);
    Yboard.display.setTextSize(2);
    Yboard.display.setTextWrap(true);

    showPatternName();
}

void loop() {

    // Check if the display should be cleared after showing the pattern name
    if (clearDisplay && (millis() - displayTime > SHOW_PATTERN_NAME_TIME)) {
        Yboard.display.clearDisplay();
        Yboard.display.display();
        clearDisplay = false;
    }

    // Update brightness of LEDs based on knob
    int brightness = Yboard.get_knob();
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 255) {
        brightness = 255;
    }
    Yboard.set_led_brightness(brightness);

    gPatterns[gCurrentPatternNumber].func();
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    EVERY_N_MILLISECONDS(20) { gHue++; }

    if (Yboard.get_button(2)) {
        nextPattern();
        showPatternName();
        waitForButtonRelease();
    }
    if (Yboard.get_button(1)) {
        previousPattern();
        showPatternName();
        waitForButtonRelease();
    }
}

void showPatternName() {
    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.println(gPatterns[gCurrentPatternNumber].name);
    Yboard.display.display();
    clearDisplay = true;
    displayTime = millis();
}

void waitForButtonRelease() {
    while (Yboard.get_button(2) || Yboard.get_button(1)) {
        delay(10);
    }
}

void nextPattern() {
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void previousPattern() {
    // subtract one from the current pattern number, and wrap around at the beginning
    if (gCurrentPatternNumber == 0) {
        gCurrentPatternNumber = ARRAY_SIZE(gPatterns) - 1;
    } else {
        gCurrentPatternNumber--;
    }
}

void rainbow() {
    // FastLED's built-in rainbow generator
    fill_rainbow(Yboard.leds, Yboard.led_count, gHue, 7);
}

void rainbowWithGlitter() {
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter) {
    if (random8() < chanceOfGlitter) {
        Yboard.leds[random16(Yboard.led_count)] += CRGB::White;
    }
}

void confetti() {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(Yboard.leds, Yboard.led_count, 10);
    int pos = random16(Yboard.led_count);
    Yboard.leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(Yboard.leds, Yboard.led_count, 20);
    int pos = beatsin16(13, 0, Yboard.led_count - 1);
    Yboard.leds[pos] += CHSV(gHue, 255, 192);
}

void bpm() {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < Yboard.led_count; i++) { // 9948
        Yboard.leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
}

void juggle() {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(Yboard.leds, Yboard.led_count, 20);
    uint8_t dothue = 0;
    for (int i = 0; i < 8; i++) {
        Yboard.leds[beatsin16(i + 7, 0, Yboard.led_count - 1)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
}
