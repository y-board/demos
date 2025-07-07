#include <Arduino.h>

#include "yboard.h"

// Screen Constants
#define BRIGHTNESS_DAMPER 0.8 // 0 is brightest
#define REFRESH_RATE 50       // Measured in ms
#define SCREEN_WIDTH 128
#define ZERO_DEG 0
#define PADDING 2 
#define SWITCH_X 0+PADDING*2
#define SWITCH_Y 8
#define SWITCH_WIDTH 12
#define SWITCH_HEIGHT 24
#define BUTTON_Y SWITCH_Y+5
#define BUTTON_WIDTH 17
#define BUTTON_HEIGHT 17
#define BUTTON_X SCREEN_WIDTH/2-BUTTON_WIDTH
#define KNOB_RADIUS 8
#define KNOB_X SCREEN_WIDTH-KNOB_RADIUS-PADDING*2
#define KNOB_Y 20

#define ON 1
#define OFF 0

int x,y,z,but1,but2,sw1,sw2,knob,temp,knob_x,knob_y;

void setup() {
  Serial.begin(9600);
  Yboard.setup();
  x = 0;
  y = 0;
  z = 0;
  but1 = Yboard.get_button(1);
  but2 = Yboard.get_button(2);
  sw1 = Yboard.get_switch(1);
  sw2 = Yboard.get_switch(2);
  knob = map(Yboard.get_knob(), 0, 100, 0, 255);
  knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
  knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;
  temp = 0;

  // Display setup
  delay(1000); // Display needs time to initialize
  Yboard.display.clearDisplay();
  Yboard.display.setTextColor(ON,OFF);
  Yboard.display.setRotation(ZERO_DEG); // Can be 0, 90, 180, or 270
  Yboard.display.setTextWrap(false);
  uint8_t text_size = 1;
  Yboard.display.setTextSize(text_size);
  Yboard.display.setCursor(0, 0);
  Yboard.display.printf("x:%i",x);
  Yboard.display.setCursor(42, 0);
  Yboard.display.printf("y:%i", y);
  Yboard.display.setCursor(85, 0);
  Yboard.display.printf("z:%i", z);
  Yboard.display.drawRoundRect(SWITCH_X, SWITCH_Y, SWITCH_WIDTH, SWITCH_HEIGHT, 3, ON);
  if (sw1) {
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING, 8, 8, 3, ON);
  } else {
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, ON);
  }
  Yboard.display.drawRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING, SWITCH_Y,SWITCH_WIDTH, SWITCH_HEIGHT, 3, ON);
  if (sw2) {
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING, 8, 8, 3, ON);
  } else {
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, ON);
  }
  Yboard.display.drawRect(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, ON);
  if (but1) {
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2-1, 6, ON);
  }else {
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
  }
  Yboard.display.drawRect(BUTTON_X+BUTTON_WIDTH+PADDING, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, ON);
  if (but2) {
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
  } else {
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
  }
  Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
  Yboard.display.display();
  
}

void loop() {

  // Test switches
  temp = Yboard.get_switch(1);
  if (temp && !sw1) {
    sw1 = 1;
    Yboard.set_led_color(1, 255, 255, 255);
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, OFF);
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING, 8, 8, 3, ON);
    Yboard.display.display();
  } else if (!temp && sw1) {
    sw1 = 0;
    Yboard.set_led_color(1, 0, 0, 0);
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING, 8, 8, 3, OFF);
    Yboard.display.fillRoundRect(SWITCH_X+PADDING, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, ON);
    Yboard.display.display();
  }

  temp = Yboard.get_switch(2);
  if (temp && !sw2) {
    sw2 = 1;
    Yboard.set_led_color(2, 255, 255, 255);
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, OFF);
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING, 8, 8, 3, ON);
    Yboard.display.display();
  } else if (!temp && sw2) {
    sw2 = 0;
    Yboard.set_led_color(2, 0, 0, 0);
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING, 8, 8, 3, OFF);
    Yboard.display.fillRoundRect(SWITCH_X+SWITCH_WIDTH+PADDING*2, SWITCH_Y+PADDING+SWITCH_HEIGHT/2, 8, 8, 3, ON);
    Yboard.display.display();
  }

  // Test buttons
  temp = Yboard.get_button(1);
  if (temp && !but1) {
    but1 = 1;
    // Yboard.set_led_color(3, 255, 255, 255);
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, OFF);
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2-1, 6, ON);
    Yboard.display.display();
    Yboard.set_all_leds_color(255, 255, 255);
    Yboard.play_notes("O5 F D. C#8 D. C#8 D8 B8- O4 G8 F"); // Play fight song
    Yboard.set_all_leds_color(0, 0, 0);
  } else if (!temp && but1){
    but1 = 0;
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2-1, 6, OFF);
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
    Yboard.display.display();
  }

  temp = Yboard.get_button(2);
  if (temp && !but2) {
    but2 = 1;
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, OFF);
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
    Yboard.display.display();
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
  } else if (!temp && but2) {
    but2 = 0;
    Yboard.set_all_leds_color(0, 0, 0);
    Yboard.display.fillCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, OFF);
    Yboard.display.drawCircle(BUTTON_X+BUTTON_WIDTH+PADDING+BUTTON_WIDTH/2, BUTTON_Y+BUTTON_HEIGHT/2, 6, ON);
    Yboard.display.display();
  }

  // Test Knob
  Yboard.display.fillCircle(knob_x, knob_y, 2, OFF);
  knob = map(Yboard.get_knob(), 0, 100, 0, 255);
  Yboard.set_led_color(5, knob, knob, knob);
  Yboard.display.drawCircle(KNOB_X, KNOB_Y, KNOB_RADIUS, ON);
  knob_x = cos(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_X;
  knob_y = sin(knob * PI / 180 + PI / 4) * KNOB_RADIUS + KNOB_Y;
  Yboard.display.fillCircle(knob_x, knob_y, 2, ON);

  // Test accelerometer
  if (Yboard.accelerometer_available()) {
    accelerometer_data accel_data = Yboard.get_accelerometer();
    temp = accel_data.x;
    if (temp != x) {
      x = temp;
      if (x > 999) {
        x = 999;
      } else if (x < -999) {
        x = -999;
      }
      if (x < 0) {
        Yboard.set_led_color(6, map(x, 0, -999, 0, 255), 0, 0);
      } else {
        Yboard.set_led_color(6, 0, map(x, 0, 999, 0, 255),
                           map(x, 0, 999, 0, 255));
      }
      Yboard.display.setCursor(10, 0);
      Yboard.display.printf("%04i", x);
    }

    temp = accel_data.y;
    if (temp != y) {
      y = temp;
      if (y > 999) {
        y = 999;
      } else if (y < -999) {
        y = -999;
      }
      if (y < 0) {
        Yboard.set_led_color(7, map(y, 0, -999, 0, 255), 0,
                            map(y, 0, -999, 0, 255));
      } else {
        Yboard.set_led_color(7, 0, map(y, 0, 999, 0, 255), 0);
      }
      Yboard.display.setCursor(42+10, 0);
      Yboard.display.printf("%04i", y);
    }

    temp = accel_data.z;
    if (temp != z) {
      z = temp;
      if (z > 999) {
        z = 999;
      } else if (z < -999) {
        z = -999;
      }
      if (z < 0) {
        Yboard.set_led_color(8, map(z, 0, -999, 0, 255),
                            map(z, 0, -999, 0, 255), 0);
      } else {
        Yboard.set_led_color(8, 0, 0, map(z, 0, 999, 0, 255));
      }
      Yboard.display.setCursor(85+10, 0);
      Yboard.display.printf("%04i", z);
    }
  }
  Yboard.display.display(); // Draw on display
}
