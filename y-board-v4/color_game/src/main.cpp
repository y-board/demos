
#include "Arduino.h"
// #include "ResponsiveAnalogRead.h"
#include "yboard.h"

typedef struct {
  unsigned char r, g, b;
} Color;

// ResponsiveAnalogRead analog(Yboard.knob_pin, true);

void setup() {
  Serial.begin(9600);

  Yboard.setup();
}

void loop() {}
