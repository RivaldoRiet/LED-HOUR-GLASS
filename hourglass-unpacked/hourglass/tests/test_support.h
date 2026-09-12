#ifndef HOURGLASS_TEST_SUPPORT_H
#define HOURGLASS_TEST_SUPPORT_H

#include <cstdlib>
#include <iostream>

#include "Arduino.h"
#include "LedControl.h"

extern void setup();
extern void loop();
extern void alarm();
extern void updateAlarm();
extern int countParticles(int addr);
extern int getTopMatrix();
extern int getBottomMatrix();
extern int getGravity();
extern LedControl lc;
extern int gravity;

[[noreturn]] inline void fail(const char* message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

#endif
