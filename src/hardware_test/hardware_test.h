#ifndef HARDWARE_TEST_H
#define HARDWARE_TEST_H

#include "Arduino.h"
#include "ybadge.h"

// Required functions
void hardware_test_init();
void hardware_test_loop();

int get_brightness();
bool check_switches();

#endif /* HARDWARE_TEST_H */
