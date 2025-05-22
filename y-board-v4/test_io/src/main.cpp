#include <stdint.h>

#include "Arduino.h"
#include "yboard.h"

void setup() {
  Serial.begin(9600);
  Yboard.setup();
}

#define USE_BIT_PACKED_METHODS 0

#if USE_BIT_PACKED_METHODS
uint8_t buttons_old = Yboard.get_buttons();
uint8_t switches_old = Yboard.get_switches();
uint8_t dsw_old = Yboard.get_dip_switches();
bool knob_button_old = Yboard.get_knob_button();

void loop() {
  // Buttons
  uint8_t buttons = Yboard.get_buttons();
  if (buttons != buttons_old) {
    for (int i = 0; i < 5; i++) {
      if ((buttons & (1 << i)) != (buttons_old & (1 << i))) {
        Serial.printf("Button %d: %s\r\n", i + 1,
                      (buttons & (1 << i)) ? "pressed" : "released");
      }
    }
    buttons_old = buttons;
  }

  // Switches
  uint8_t switches = Yboard.get_switches();
  if (switches != switches_old) {
    for (int i = 0; i < 4; i++) {
      if ((switches & (1 << i)) != (switches_old & (1 << i))) {
        Serial.printf("Switch %d: %s\r\n", i + 1,
                      (switches & (1 << i)) ? "on" : "off");
      }
    }
    switches_old = switches;
  }

  // DIP switches
  uint8_t dsw = Yboard.get_dip_switches();
  if (dsw != dsw_old) {
    for (int i = 0; i < 6; i++) {
      if ((dsw & (1 << i)) != (dsw_old & (1 << i))) {
        Serial.printf("DIP Switch %d: %s\r\n", i + 1,
                      (dsw & (1 << i)) ? "on" : "off");
      }
    }
    dsw_old = dsw;
  }

  // Knob button
  bool knob_button = Yboard.get_knob_button();
  if (knob_button != knob_button_old) {
    Serial.printf("Knob button: %s\r\n", knob_button ? "pressed" : "released");
    knob_button_old = knob_button;
  }
}

#else

bool buttons_old[5] = {Yboard.get_button(1), Yboard.get_button(2),
                       Yboard.get_button(3), Yboard.get_button(4),
                       Yboard.get_button(5)};
bool switches_old[4] = {Yboard.get_switch(1), Yboard.get_switch(2),
                        Yboard.get_switch(3), Yboard.get_switch(4)};
bool dsw_old[6] = {Yboard.get_dip_switch(1), Yboard.get_dip_switch(2),
                   Yboard.get_dip_switch(3), Yboard.get_dip_switch(4),
                   Yboard.get_dip_switch(5), Yboard.get_dip_switch(6)};
bool knob_button_old = Yboard.get_knob_button();

void loop() {
  if (!USE_BIT_PACKED_METHODS) {
    // Buttons
    for (int i = 0; i < 5; i++) {
      bool button = Yboard.get_button(i + 1);
      if (button != buttons_old[i]) {
        Serial.printf("Button %d: %s\r\n", i + 1,
                      button ? "pressed" : "released");
        buttons_old[i] = button;
      }
    }

    // Switches
    for (int i = 0; i < 4; i++) {
      bool sw = Yboard.get_switch(i + 1);
      if (sw != switches_old[i]) {
        Serial.printf("Switch %d: %s\r\n", i + 1, sw ? "on" : "off");
        switches_old[i] = sw;
      }
    }

    // DIP switches
    for (int i = 0; i < 6; i++) {
      bool dsw = Yboard.get_dip_switch(i + 1);
      if (dsw != dsw_old[i]) {
        Serial.printf("DIP Switch %d: %s\r\n", i + 1, dsw ? "on" : "off");
        dsw_old[i] = dsw;
      }
    }

    // Knob button
    bool knob_button = Yboard.get_knob_button();
    if (knob_button != knob_button_old) {
      Serial.printf("Knob button: %s\r\n",
                    knob_button ? "pressed" : "released");
      knob_button_old = knob_button;
    }
  }
}
#endif