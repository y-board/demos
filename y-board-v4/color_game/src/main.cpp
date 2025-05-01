
#include "Arduino.h"
// #include "ResponsiveAnalogRead.h"
#include "yboard.h"

typedef struct {
  int r, g, b;
} Color;

// ResponsiveAnalogRead analog(Yboard.knob_pin, true);

Color secret_color;
Color user_color = {0, 0, 0};

typedef enum { S_RANDOMIZE = 0, S_SHOW_SECRET, S_DISPLAY } State;

State state = S_RANDOMIZE;

void setup() {
  Serial.begin(9600);

  Yboard.setup();
}

void randomizeSecretColor() {
  int *c[3] = {&secret_color.r, &secret_color.g, &secret_color.b};

  // Randomly shuffle c
  for (int i = 0; i < 3; i++) {
    int j = random(0, 3);
    int *temp = c[i];
    c[i] = c[j];
    c[j] = temp;
  }

  // Primary
  *c[0] = random(200, 256);
  // Secondary
  *c[1] = random(0, 100);
  // Tertiary
  *c[2] = random(0, 100);
}

void showSecretColor() {
  Yboard.set_all_leds_color(secret_color.r, secret_color.g, secret_color.b);
}

void showUserColor() {
  Yboard.set_all_leds_color(user_color.r, user_color.g, user_color.b);
}

int last_button = 0;

const int BUTTON_SECRET = 3;
const int BUTTON_RED = 1;
const int BUTTON_BLUE = 2;
const int BUTTON_GREEN = 4;

const int scaler = 3;

void loop() {

  switch (state) {
  case S_RANDOMIZE:
    randomizeSecretColor();
    Yboard.set_led_brightness(100);
    Yboard.display.clearDisplay();
    state = S_SHOW_SECRET;
    break;
  case S_SHOW_SECRET:
    Yboard.set_all_leds_color(secret_color.r, secret_color.g, secret_color.b);
    delay(1000);
    state = S_DISPLAY;
    break;
  case S_DISPLAY:
    if (Yboard.get_button(BUTTON_SECRET)) {
      showSecretColor();
      last_button = BUTTON_SECRET;
      return;
    }

    // Draw RGB characters along bottom of screen, then bar graphs with RGB
    // values
    Yboard.display.setCursor(0, 0);
    Yboard.display.setTextSize(1);
    Yboard.display.setTextColor(WHITE);
    Yboard.display.print("R: ");
    Yboard.display.drawRect(0, 10, 20, user_color.r, 0xF800);

    showUserColor();
    if (Yboard.get_button(BUTTON_RED)) {
      if (last_button == BUTTON_RED) {
        int knob = Yboard.get_knob();
        user_color.r = user_color.r + knob * scaler;
        user_color.r = constrain(user_color.r, 0, 255);
      }
      last_button = BUTTON_RED;
    }

    else if (Yboard.get_button(BUTTON_GREEN)) {
      if (last_button == BUTTON_GREEN) {
        int knob = Yboard.get_knob();
        user_color.g = user_color.g + knob * scaler;
        user_color.g = constrain(user_color.g, 0, 255);
      }
      last_button = BUTTON_GREEN;
    }

    else if (Yboard.get_button(BUTTON_BLUE)) {
      if (last_button == BUTTON_BLUE) {
        int knob = Yboard.get_knob();
        user_color.b = user_color.b + knob * scaler;
        user_color.b = constrain(user_color.b, 0, 255);
      }
      last_button = BUTTON_BLUE;
    }

    else {
      last_button = 0;
    }
    Yboard.reset_knob();
    break;
  }
}