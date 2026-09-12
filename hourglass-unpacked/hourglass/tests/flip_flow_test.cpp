#include "test_support.h"

int main() {
  // Begin upright in the 180-degree sensor orientation.
  fakeAnalogX = 330;
  fakeAnalogY = 410;
  setup();

  for (int i = 0; i < 800; ++i) loop();

  int oldTop = getTopMatrix();
  int oldBottom = getBottomMatrix();
  if (countParticles(oldTop) != 0 || countParticles(oldBottom) != 60)
    fail("sand did not finish in the first direction");

  // Turn the hourglass upside down.
  fakeAnalogY = 200;
  for (int i = 0; i < 3; ++i) loop();

  if (getTopMatrix() != oldBottom)
    fail("after a 180-degree flip, the previous bottom is not the new top");

  for (int i = 0; i < 20; ++i) loop();
  if (countParticles(oldBottom) >= 60)
    fail("sand does not resume flowing after the hourglass is flipped");

  std::cout << "PASS: sand reverses and resumes after a 180-degree flip\n";
  return 0;
}
