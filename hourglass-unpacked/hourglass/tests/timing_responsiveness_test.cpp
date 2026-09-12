#include "test_support.h"
#include "Delay.h"

int main() {
  NonBlockDelay timer;

  fakeMillis = 1000;
  timer.Delay(1000);
  fakeMillis = 1999;
  if (timer.Timeout()) fail("drop timer expires before its deadline");
  fakeMillis = 2000;
  if (!timer.Timeout()) fail("drop timer misses its exact deadline");

  fakeMillis = 0xFFFFFF00UL;
  timer.Delay(500);
  if (timer.Timeout()) fail("drop timer expires immediately across millis rollover");
  fakeMillis += 499;
  if (timer.Timeout()) fail("rollover timer expires one millisecond early");
  fakeMillis += 1;
  if (!timer.Timeout()) fail("rollover timer misses its deadline");

  fakeMillis = 5000;
  fakeToneCount = 0;
  alarm();
  if (fakeMillis != 5000)
    fail("completion alarm blocks the main loop");

  for (int second = 0; second < 5; ++second) {
    updateAlarm();
    fakeMillis += 1000;
  }
  if (fakeToneCount != 5)
    fail("nonblocking completion alarm does not emit five beeps");

  std::cout << "PASS: timers survive rollover and the alarm remains responsive\n";
  return 0;
}
