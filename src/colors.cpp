#include "colors.h"

// used by rainbowCycle and theaterChaseRainbow
uint8_t *color_wheel(uint8_t wheel_pos) {
    static uint8_t c[3];

    if (wheel_pos < 85) {
        c[0] = wheel_pos * 3;
        c[1] = 255 - wheel_pos * 3;
        c[2] = 0;
    } else if (wheel_pos < 170) {
        wheel_pos -= 85;
        c[0] = 255 - wheel_pos * 3;
        c[1] = 0;
        c[2] = wheel_pos * 3;
    } else {
        wheel_pos -= 170;
        c[0] = 0;
        c[1] = wheel_pos * 3;
        c[2] = 255 - wheel_pos * 3;
    }

    return c;
}
