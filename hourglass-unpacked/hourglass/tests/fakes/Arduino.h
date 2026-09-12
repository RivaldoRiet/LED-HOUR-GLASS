#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <vector>

#define ARDUINO 10819
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define MSBFIRST 1
#define A0 14
#define A1 15
#define A2 16

#define B00000000 0x00
#define B00000001 0x01
#define B00001000 0x08
#define B00001101 0x0D
#define B00001110 0x0E
#define B00011101 0x1D
#define B00011111 0x1F
#define B00110000 0x30
#define B00110011 0x33
#define B00110111 0x37
#define B00111101 0x3D
#define B01000000 0x40
#define B01000111 0x47
#define B01001111 0x4F
#define B01011011 0x5B
#define B01011111 0x5F
#define B01100111 0x67
#define B01101101 0x6D
#define B01110000 0x70
#define B01111001 0x79
#define B01111011 0x7B
#define B01111110 0x7E
#define B01111111 0x7F
#define B10000000 0x80
#define B01110111 0x77
#define B00010101 0x15

using byte = std::uint8_t;
using boolean = bool;

struct FakeMax7219 {
  byte registers[16] = {};
};

inline FakeMax7219 fakeMax7219[2];
inline std::vector<byte> fakeShiftedBytes;
inline unsigned long fakeMillis = 0;
inline int fakeChipSelect = HIGH;
inline int fakeAnalogX = 400;
inline int fakeAnalogY = 330;
inline int fakeAnalogSeed = 123;
inline int fakeToneCount = 0;

inline void fakeResetMax7219() {
  std::memset(fakeMax7219, 0, sizeof(fakeMax7219));
}

inline void fakeResetMax7219(int device) {
  std::memset(&fakeMax7219[device], 0, sizeof(fakeMax7219[device]));
}

inline void pinMode(int, int) {}

inline void digitalWrite(int pin, int value) {
  if (pin != 6) return;
  if (value == LOW) {
    fakeShiftedBytes.clear();
  } else if (fakeChipSelect == LOW && fakeShiftedBytes.size() == 4) {
    for (int device = 0; device < 2; ++device) {
      const std::size_t offset = std::size_t(device) * 2;
      const byte opcode = fakeShiftedBytes[offset];
      const byte data = fakeShiftedBytes[offset + 1];
      if (opcode < 16 && opcode != 0) fakeMax7219[1 - device].registers[opcode] = data;
    }
  }
  fakeChipSelect = value;
}

inline void shiftOut(int, int, int, byte value) {
  fakeShiftedBytes.push_back(value);
}

inline unsigned long millis() { return fakeMillis; }
inline void delay(unsigned long duration) { fakeMillis += duration; }
inline int analogRead(int pin) {
  if (pin == A1) return fakeAnalogX;
  if (pin == A2) return fakeAnalogY;
  return fakeAnalogSeed;
}
inline void randomSeed(unsigned long seed) { std::srand(seed); }
inline long random(long maximum) { return maximum ? std::rand() % maximum : 0; }
inline void tone(int, unsigned int, unsigned long = 0) { fakeToneCount++; }

struct FakeSerial {
  void begin(long) {}
  void println(const char*) {}
  void println(int) {}
  void print(const char*) {}
  void print(int) {}
};

inline FakeSerial Serial;

#endif
