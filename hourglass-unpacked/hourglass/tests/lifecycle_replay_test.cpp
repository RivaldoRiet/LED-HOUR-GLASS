#include "test_support.h"

static void assertParticleConservation() {
  if (countParticles(0) + countParticles(1) != 60)
    fail("particle count changed during lifecycle replay");
}

static void flipTo(int y) {
  const int transition[][2] = {
    {310, 350}, {285, 340}, {330, 330}, {300, 345}
  };
  for (const auto& reading : transition) {
    fakeAnalogX = reading[0];
    fakeAnalogY = reading[1];
    loop();
    assertParticleConservation();
  }
  fakeAnalogX = 330;
  fakeAnalogY = y;
  for (int i = 0; i < 3; ++i) {
    loop();
    assertParticleConservation();
  }
}

int main(int argc, char** argv) {
  if (argc > 1) fakeAnalogSeed = std::atoi(argv[1]);
  fakeAnalogX = 330;
  fakeAnalogY = 410;
  setup();

  for (int cycle = 0; cycle < 12; ++cycle) {
    int particlesAtStart = countParticles(getTopMatrix());
    unsigned long startedAt = fakeMillis;
    int loops = 0;
    while (countParticles(getTopMatrix()) > 0 && loops < 800) {
      loop();
      assertParticleConservation();
      loops++;
    }
    if (countParticles(getTopMatrix()) != 0)
      fail("a replay cycle did not finish");
    unsigned long elapsed = fakeMillis - startedAt;
    unsigned long expected = (unsigned long)particlesAtStart * 1000UL;
    if (elapsed + 1000UL < expected || elapsed > expected + 1500UL) {
      std::cerr << "cycle=" << cycle << " elapsed=" << elapsed << " loops=" << loops << '\n';
      fail("a one-minute cycle finished outside its timing tolerance");
    }

    int disconnectedDevice = cycle & 1;
    fakeResetMax7219(disconnectedDevice);
    for (int i = 0; i < 10; ++i) loop();
    if (fakeMax7219[disconnectedDevice].registers[12] != 1 ||
        fakeMax7219[disconnectedDevice].registers[11] != 7)
      fail("display did not recover during the completion alarm");

    int previousBottom = getBottomMatrix();
    flipTo(gravity == 180 ? 200 : 410);
    if (getTopMatrix() != previousBottom)
      fail("matrix roles did not swap during repeated lifecycle replay");

    int before = countParticles(getTopMatrix());
    for (int i = 0; i < 20; ++i) loop();
    if (countParticles(getTopMatrix()) >= before)
      fail("sand did not resume after a replay flip");
  }

  std::cout << "PASS: 12 full timed cycles conserve particles and recover cleanly"
            << " (seed " << fakeAnalogSeed << ")\n";
  return 0;
}
