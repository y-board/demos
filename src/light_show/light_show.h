#ifndef LIGHT_SHOW_H
#define LIGHT_SHOW_H

#include "Arduino.h"
#include "ybadge.h"

// Required functions
void light_show_init();
void light_show_loop();

// Helper functions
void rainbow_cycle();
void running_lights(byte red, byte green, byte blue, int WaveDelay);
void color_wipe(byte red, byte green, byte blue, int SpeedDelay);
void rgb_loop();

#endif /* LIGHT_SHOW_H */
