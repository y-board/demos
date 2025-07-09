
#include "Arduino.h"
#include "yboard.h"

//////////////////////////////// Types /////////////////////////////////////////

typedef struct {
  int r, g, b;
} Color;

typedef enum {
  S_INIT = 0,
  S_SHOW_INSTRUCTIONS,
  S_GUESSING,
  S_SHOW_SCORE,
  S_DONE
} State;

//////////////////////////////// Configuration Constants ///////////////////////
const int scaler = 3; // How much to change color by per knob turn

const int BUTTON_SECRET = Yboard.button_up;
const int BUTTON_RED = Yboard.button_left;
const int BUTTON_BLUE = Yboard.button_right;
const int BUTTON_GREEN = Yboard.button_center;

//////////////////////////////// Globals ///////////////////////////////////////
Color secret_color;   // Secret color to match
Color user_color;     // User's currently guessed color
State state = S_INIT; // FSM state
int last_button = 0;  // Previous button state

//////////////////////////////// Forward Declarations //////////////////////////
static void state_init();
static void state_instructions();
static void state_guessing();
static void state_show_score();
static void randomizeSecretColor();
static void showSecretColor();
static void showUserColor();

//////////////////////////////// FSM ///////////////////////////////////////////

void setup() {
  Serial.begin(9600);

  Yboard.setup();
}

void loop() {

  switch (state) {
  case S_INIT:
    state_init();
    state = S_SHOW_INSTRUCTIONS;
    break;

  case S_SHOW_INSTRUCTIONS:
    state_instructions();

    // Transition on knob press and release
    if (Yboard.get_knob_button()) {
      while (Yboard.get_knob_button())
        ;
      state = S_GUESSING;
    }
    break;

  case S_GUESSING:
    state_guessing();

    // Transition on knob press and release
    if (Yboard.get_knob_button()) {
      while (Yboard.get_knob_button())
        ;
      state = S_SHOW_SCORE;
    }
    break;

  case S_SHOW_SCORE:
    state_show_score();

    // Transition on knob press and release
    if (Yboard.get_knob_button()) {
      while (Yboard.get_knob_button())
        ;
      state = S_INIT;
    }
  }
}

//////////////////////////////// State Functions ///////////////////////////////
static void state_init() {
  randomizeSecretColor();
  user_color = {10, 10, 10};
  Yboard.set_led_brightness(100);
  Yboard.display.clearDisplay();
}

static void state_instructions() {
  Yboard.set_all_leds_color(secret_color.r, secret_color.g, secret_color.b);

  Yboard.display.clearDisplay();
  Yboard.display.setTextSize(1);
  Yboard.display.setCursor(0, 0);
  Yboard.display.print(
      "Match the color!\n"
      "Hold L, C, or R\nbutton while turning\nknob to guess.\nPress "
      "knob to guess.\nPress UP to see the\ngoal color again.\n= Press knob "
      "now. =");
  Yboard.display.display();
}

static void state_guessing() {
  if (Yboard.get_button(BUTTON_SECRET)) {
    showSecretColor();
    last_button = BUTTON_SECRET;
    return;
  }

  // Draw RGB characters along bottom of screen, then bar graphs with RGB
  // values
  Yboard.display.clearDisplay();
  Yboard.display.setCursor(0, 0);
  Yboard.display.setTextSize(2);
  Yboard.display.print("R:\nG:\nB:\n");
  Yboard.display.fillRect(25, 0, 90 * (user_color.r / 255.0), 15, WHITE);
  Yboard.display.fillRect(25, 16, 90 * (user_color.g / 255.0), 15, WHITE);
  Yboard.display.fillRect(25, 32, 90 * (user_color.b / 255.0), 15, WHITE);
  Yboard.display.display();

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
  } else {
    last_button = 0;
  }
  Yboard.reset_knob();
}

static void state_show_score() {
  Yboard.display.clearDisplay();

  // Calculate score from 0 to 100 based on distance from secret color
  int score = 100 - (abs(secret_color.r - user_color.r) +
                     abs(secret_color.g - user_color.g) +
                     abs(secret_color.b - user_color.b)) /
                        3;
  score = constrain(score, 0, 100);

  Yboard.display.setTextSize(3);
  Yboard.display.setCursor(0, 0);
  Yboard.display.print("Score:\n");
  Yboard.display.setTextSize(5);

  // Center the score horizontally
  int16_t x1, y1;
  uint16_t w, h;
  char scoreStr[4];
  sprintf(scoreStr, "%d", score);
  Yboard.display.getTextBounds(scoreStr, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (Yboard.display.width() - w) / 2;
  Yboard.display.setCursor(x, 30);
  Yboard.display.print(score);
  Yboard.display.display();
}

//////////////////////////////// Helper Functions //////////////////////////////

static void randomizeSecretColor() {
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

static void showSecretColor() {
  Yboard.set_all_leds_color(secret_color.r, secret_color.g, secret_color.b);
}

static void showUserColor() {
  Yboard.set_all_leds_color(user_color.r, user_color.g, user_color.b);
}