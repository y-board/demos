#include <yboard.h>

#define FRAMES_PER_SECOND 120
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define SHOW_PATTERN_NAME_TIME 1500

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING 55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

CRGBPalette16 pacifica_palette_1 = {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212,
                                    0x000114, 0x000117, 0x000019, 0x00001C, 0x000026, 0x000031,
                                    0x00003B, 0x000046, 0x14554B, 0x28AA50};
CRGBPalette16 pacifica_palette_2 = {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212,
                                    0x000114, 0x000117, 0x000019, 0x00001C, 0x000026, 0x000031,
                                    0x00003B, 0x000046, 0x0C5F52, 0x19BE5F};
CRGBPalette16 pacifica_palette_3 = {0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927,
                                    0x000B2D, 0x000C33, 0x000E39, 0x001040, 0x001450, 0x001860,
                                    0x001C70, 0x002080, 0x1040BF, 0x2060FF};

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
void pacifica();
void pacifica_add_whitecaps();
void pacifica_deepen_colors();
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri,
                        uint16_t ioff);
void fire();

// List of patterns to cycle through. Each is defined as a separate function below.
typedef void (*PatternFunction)();
struct Pattern {
    PatternFunction func;
    const char *name;
};

Pattern gPatterns[] = {{rainbow, "Rainbow"},   {rainbowWithGlitter, "Rainbow + Glitter"},
                       {confetti, "Confetti"}, {sinelon, "Sinelon"},
                       {juggle, "Juggle"},     {bpm, "BPM"},
                       {pacifica, "Pacifica"}, {fire, "Fire"}};

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

void pacifica() {
    // Increment the four "color index start" counters, one for each wave layer.
    // Each is incremented at a different speed, and the speeds vary over time.
    static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
    static uint32_t sLastms = 0;
    uint32_t ms = GET_MILLIS();
    uint32_t deltams = ms - sLastms;
    sLastms = ms;
    uint16_t speedfactor1 = beatsin16(3, 179, 269);
    uint16_t speedfactor2 = beatsin16(4, 179, 269);
    uint32_t deltams1 = (deltams * speedfactor1) / 256;
    uint32_t deltams2 = (deltams * speedfactor2) / 256;
    uint32_t deltams21 = (deltams1 + deltams2) / 2;
    sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
    sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
    sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
    sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

    // Clear out the LED array to a dim background blue-green
    fill_solid(Yboard.leds, Yboard.led_count, CRGB(2, 6, 10));

    // Render each of four layers, with different scales and speeds, that vary over time
    pacifica_one_layer(pacifica_palette_1, sCIStart1, beatsin16(3, 11 * 256, 14 * 256),
                       beatsin8(10, 70, 130), 0 - beat16(301));
    pacifica_one_layer(pacifica_palette_2, sCIStart2, beatsin16(4, 6 * 256, 9 * 256),
                       beatsin8(17, 40, 80), beat16(401));
    pacifica_one_layer(pacifica_palette_3, sCIStart3, 6 * 256, beatsin8(9, 10, 38),
                       0 - beat16(503));
    pacifica_one_layer(pacifica_palette_3, sCIStart4, 5 * 256, beatsin8(8, 10, 28), beat16(601));

    // Add brighter 'whitecaps' where the waves lines up more
    pacifica_add_whitecaps();

    // Deepen the blues and greens a bit
    pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri,
                        uint16_t ioff) {
    uint16_t ci = cistart;
    uint16_t waveangle = ioff;
    uint16_t wavescale_half = (wavescale / 2) + 20;
    for (uint16_t i = 0; i < Yboard.led_count; i++) {
        waveangle += 250;
        uint16_t s16 = sin16(waveangle) + 32768;
        uint16_t cs = scale16(s16, wavescale_half) + wavescale_half;
        ci += cs;
        uint16_t sindex16 = sin16(ci) + 32768;
        uint8_t sindex8 = scale16(sindex16, 240);
        CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
        Yboard.leds[i] += c;
    }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps() {
    uint8_t basethreshold = beatsin8(9, 55, 65);
    uint8_t wave = beat8(7);

    for (uint16_t i = 0; i < Yboard.led_count; i++) {
        uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
        wave += 7;
        uint8_t l = Yboard.leds[i].getAverageLight();
        if (l > threshold) {
            uint8_t overage = l - threshold;
            uint8_t overage2 = qadd8(overage, overage);
            Yboard.leds[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
        }
    }
}

// Deepen the blues and greens
void pacifica_deepen_colors() {
    for (uint16_t i = 0; i < Yboard.led_count; i++) {
        Yboard.leds[i].blue = scale8(Yboard.leds[i].blue, 145);
        Yboard.leds[i].green = scale8(Yboard.leds[i].green, 200);
        Yboard.leds[i] |= CRGB(2, 5, 7);
    }
}

void fire() {
    // Array of temperature readings at each simulation cell
    static uint8_t heat[Yboard.led_count];

    // Step 1.  Cool down every cell a little
    for (int i = 0; i < Yboard.led_count; i++) {
        heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / Yboard.led_count) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = Yboard.led_count - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKING) {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < Yboard.led_count; j++) {
        CRGB color = HeatColor(heat[j]);
        Yboard.leds[j] = color;
    }
}
