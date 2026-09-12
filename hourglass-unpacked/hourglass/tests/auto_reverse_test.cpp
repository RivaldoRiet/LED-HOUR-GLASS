#include "test_support.h"

extern int sensedGravity;
extern bool autoReverseWaiting;

int main() {
  fakeAnalogX = 330;
  fakeAnalogY = 410;
  setup();

  while (countParticles(getTopMatrix()) > 0) loop();
  for (int i = 0; i < 100 && !autoReverseWaiting; ++i) loop();
  if (!autoReverseWaiting)
    fail("completed hourglass did not start the auto-reverse timer");

  int oldGravity = gravity;
  int oldBottom = getBottomMatrix();
  for (int i = 0; i < 1799; ++i) loop();
  if (gravity != oldGravity)
    fail("hourglass auto-reversed before three minutes elapsed");

  loop();
  if (gravity == oldGravity)
    fail("hourglass did not auto-reverse after three minutes");
  if (getTopMatrix() != oldBottom)
    fail("auto-reverse did not make the full bulb the new top");
  if (sensedGravity != oldGravity)
    fail("auto-reverse corrupted the physical sensor orientation");
  int particlesBefore = countParticles(getTopMatrix());
  for (int i = 0; i < 20; ++i) loop();
  if (countParticles(getTopMatrix()) >= particlesBefore)
    fail("sand did not resume after automatic reversal");

  fakeAnalogY = 200;
  for (int i = 0; i < 3; ++i) loop();
  fakeAnalogY = 410;
  for (int i = 0; i < 3; ++i) loop();
  if (gravity != 180 || sensedGravity != 180)
    fail("physical movement did not retake control after auto-reverse");

  std::cout << "PASS: a completed hourglass auto-reverses after three minutes\n";
  return 0;
}
