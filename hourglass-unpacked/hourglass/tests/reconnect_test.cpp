#include "test_support.h"

static void assertDisplayRecovered(int device) {
    if (fakeMax7219[device].registers[12] != 1)
      fail("reconnected MAX7219 remains in shutdown mode");
    if (fakeMax7219[device].registers[11] != 7)
      fail("reconnected MAX7219 scan limit was not restored");
    if (fakeMax7219[device].registers[9] != 0)
      fail("reconnected MAX7219 decode mode was not restored");
    if (fakeMax7219[device].registers[10] != 1)
      fail("reconnected MAX7219 intensity was not restored");

    for (int row = 0; row < 8; ++row) {
      byte expected = 0;
      for (int column = 0; column < 8; ++column) {
        if (lc.getRawXY(device, column, row)) expected |= byte(0x80 >> column);
      }
      if (fakeMax7219[device].registers[row + 1] != expected)
        fail("reconnected MAX7219 display does not match firmware state");
    }
}

int main() {
  setup();

  for (int disconnectedDevice = 0; disconnectedDevice < 2; ++disconnectedDevice) {
    fakeResetMax7219(disconnectedDevice);
    for (int i = 0; i < 20; ++i) loop();
    assertDisplayRecovered(disconnectedDevice);
  }

  fakeAnalogX = 330;
  fakeAnalogY = 410;
  for (int i = 0; i < 3; ++i) {
    fakeMillis += 100;
    gravity = getGravity();
  }
  if (gravity != 180)
    fail("accelerometer did not establish the starting orientation");

  const int disconnectedReadings[][2] = {
    {0, 0}, {1023, 1023}, {0, 1023}, {1023, 0}, {0, 0}
  };
  for (const auto& reading : disconnectedReadings) {
    fakeAnalogX = reading[0];
    fakeAnalogY = reading[1];
    for (int repeat = 0; repeat < 4; ++repeat) {
      fakeMillis += 100;
      gravity = getGravity();
      if (gravity != 180)
        fail("floating accelerometer input changed orientation while disconnected");
    }
  }

  fakeAnalogX = 330;
  fakeAnalogY = 200;
  for (int i = 0; i < 3; ++i) {
    fakeMillis += 100;
    gravity = getGravity();
  }
  if (gravity != 0)
    fail("accelerometer did not recover after reconnection");

  std::cout << "PASS: components recover after a disconnect/reconnect\n";
  return 0;
}
