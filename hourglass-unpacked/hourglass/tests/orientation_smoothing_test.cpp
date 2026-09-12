#include "test_support.h"

struct Reading {
  int x;
  int y;
};

int main() {
  fakeAnalogX = 330;
  fakeAnalogY = 410;
  setup();
  loop();

  const Reading flipTrace[] = {
    {310, 380}, {295, 350}, {285, 340}, {298, 345},
    {315, 335}, {330, 295}, {330, 302}, {330, 292},
    {330, 285}, {330, 280}
  };

  int changes = 0;
  int previous = gravity;
  for (const Reading& reading : flipTrace) {
    fakeAnalogX = reading.x;
    fakeAnalogY = reading.y;
    loop();
    if (gravity == 90 || gravity == 270)
      fail("a vertical flip briefly rotates through a sideways orientation");
    if (gravity != previous) {
      changes++;
      previous = gravity;
    }
  }

  if (gravity != 0)
    fail("the new upright orientation was not accepted");
  if (changes != 1)
    fail("a single physical flip caused multiple orientation changes");

  for (int i = 0; i < 1000; ++i) {
    fakeAnalogX = (i % 11 == 0) ? 0 : 330;
    fakeAnalogY = (i & 1) ? 359 : 361;
    loop();
    if (gravity != 0)
      fail("threshold noise or a rail reading caused orientation chatter");
  }

  fakeAnalogX = 330;
  fakeAnalogY = 410;
  for (int i = 0; i < 3; ++i) loop();
  if (gravity != 180)
    fail("orientation filter did not recover after extended sensor noise");

  std::cout << "PASS: noisy flips and 1000 unstable readings stay smooth\n";
  return 0;
}
