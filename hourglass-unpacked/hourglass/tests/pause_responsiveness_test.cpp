#include "test_support.h"

extern bool autoReverseWaiting;

int main() {
  fakeAnalogX = 330;
  fakeAnalogY = 410;
  setup();

  while (countParticles(getTopMatrix()) > 0) loop();
  for (int i = 0; i < 100 && !autoReverseWaiting; ++i) loop();
  if (!autoReverseWaiting)
    fail("completed hourglass did not enter its three-minute wait");

  int previousBottom = getBottomMatrix();
  fakeAnalogY = 200;
  unsigned long before = fakeMillis;
  loop();

  if (fakeMillis - before > 100UL)
    fail("physical flip blocked the main loop");
  if (getTopMatrix() != previousBottom)
    fail("physical flip during the pause was not applied on the next frame");
  if (autoReverseWaiting)
    fail("physical flip did not cancel the three-minute timer");

  std::cout << "PASS: a physical turn interrupts the pause on the next frame\n";
  return 0;
}
