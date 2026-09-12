#include "Arduino.h"
#include "LedControl.h"
#include "Delay.h"

#define  MATRIX_A  1
#define MATRIX_B  0

// Values are 260/330/400
#define ACC_THRESHOLD_LOW 300
#define ACC_THRESHOLD_HIGH 360
#define ACC_FAST_FLIP_LOW 280
#define ACC_FAST_FLIP_HIGH 380
#define ACC_READING_MIN 150
#define ACC_READING_MAX 550
#define ORIENTATION_STABLE_MS 200UL

// Matrix
#define PIN_DATAIN 5
#define PIN_CLK 4
#define PIN_LOAD 6

// Accelerometer
#define PIN_X A1
#define PIN_Y A2

#define PIN_BUZZER 14

// This takes into account how the matrixes are mounted
#define ROTATION_OFFSET 90

#define DELAY_FRAME 100
// Original steady brightness; high enough to make each grain visibly distinct.
#define DISPLAY_INTENSITY 1
#define DISPLAY_RESYNC_INTERVAL 1000UL
#define ALARM_BEEP_COUNT 5
#define ALARM_BEEP_INTERVAL 1000UL
#define AUTO_REVERSE_DELAY 180000UL

#define DEBUG_OUTPUT 0

byte delayHours = 0;
byte delayMinutes = 1;
int gravity;
int sensedGravity;
int pendingGravity = -1;
unsigned long pendingGravitySince = 0;
LedControl lc = LedControl(PIN_DATAIN, PIN_CLK, PIN_LOAD, 2);
NonBlockDelay d;
bool alarmWentOff = false;
byte alarmBeepsRemaining = 0;
unsigned long nextAlarmBeepAt = 0;
unsigned long lastDisplaySyncAt = 0;
bool autoReverseWaiting = false;
unsigned long autoReverseStartedAt = 0;


/**
 * Get delay between particle drops (in seconds)
 */
unsigned long getDelayDrop() {
  return (unsigned long)delayMinutes + (unsigned long)delayHours * 60UL;
}


#if DEBUG_OUTPUT
void printmatrix() {
  Serial.println(" 0123-4567 ");
  for (int y = 0; y<8; y++) {
    if (y == 4) {
      Serial.println("|----|----|");
    }
    Serial.print(y);
    for (int x = 0; x<8; x++) {
      if (x == 4) {
        Serial.print("|");
      }
      Serial.print(lc.getXY(0,x,y) ? "X" :" ");
    }
    Serial.println("|");
  }
  Serial.println("-----------");
}
#endif


coord getDown(int x, int y) {
  coord xy;
  xy.x = x-1;
  xy.y = y+1;
  return xy;
}
coord getLeft(int x, int y) {
  coord xy;
  xy.x = x-1;
  xy.y = y;
  return xy;
}
coord getRight(int x, int y) {
  coord xy;
  xy.x = x;
  xy.y = y+1;
  return xy;
}

bool canGoLeft(int addr, int x, int y) {
  if (x == 0) return false;
  return !lc.getXY(addr, getLeft(x, y));
}
bool canGoRight(int addr, int x, int y) {
  if (y == 7) return false;
  return !lc.getXY(addr, getRight(x, y));
}
bool canGoDown(int addr, int x, int y) {
  if (y == 7) return false;
  if (x == 0) return false;
  if (!canGoLeft(addr, x, y)) return false;
  if (!canGoRight(addr, x, y)) return false;
  return !lc.getXY(addr, getDown(x, y));
}

void goDown(int addr, int x, int y) {
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getDown(x,y), true);
}
void goLeft(int addr, int x, int y) {
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getLeft(x,y), true);
}
void goRight(int addr, int x, int y) {
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getRight(x,y), true);
}

int countParticles(int addr) {
  int c = 0;
  for (byte y=0; y<8; y++) {
    for (byte x=0; x<8; x++) {
      if (lc.getXY(addr, x, y)) {
        c++;
      }
    }
  }
  return c;
}

bool moveParticle(int addr, int x, int y) {
  if (!lc.getXY(addr,x,y)) {
    return false;
  }

  bool can_GoLeft = canGoLeft(addr, x, y);
  bool can_GoRight = canGoRight(addr, x, y);

  if (!can_GoLeft && !can_GoRight) {
    return false;
  }

  bool can_GoDown = canGoDown(addr, x, y);

  if (can_GoDown) {
    goDown(addr, x, y);
  } else if (can_GoLeft && !can_GoRight) {
    goLeft(addr, x, y);
  } else if (can_GoRight && !can_GoLeft) {
    goRight(addr, x, y);
  } else if (random(2) == 1) {
    goLeft(addr, x, y);
  } else {
    goRight(addr, x, y);
  }
  return true;
}

void fill(int addr, int maxcount) {
  int n = 8;
  byte x,y;
  int count = 0;
  for (byte slice = 0; slice < 2*n-1; ++slice) {
    byte z = slice<n ? 0 : (byte)(slice-n + 1);
    for (byte j = z; j <= slice-z; ++j) {
      y = 7-j;
      x = (slice-j);
      lc.setXY(addr, x, y, (++count <= maxcount));
    }
  }
}

bool readGravityCandidate(int &candidate, bool *strongReading = 0) {
  int x = analogRead(PIN_X);
  int y = analogRead(PIN_Y);

  if (x < ACC_READING_MIN || x > ACC_READING_MAX ||
      y < ACC_READING_MIN || y > ACC_READING_MAX) return false;

  if (y < ACC_THRESHOLD_LOW) candidate = 0;
  else if (y > ACC_THRESHOLD_HIGH) candidate = 180;
  else return false;
  if (strongReading != 0) {
    *strongReading = (y < ACC_FAST_FLIP_LOW || y > ACC_FAST_FLIP_HIGH);
  }
  return true;
}

int getGravity() {
  int candidate;
  bool strongReading = false;
  if (!readGravityCandidate(candidate, &strongReading)) {
    pendingGravity = -1;
    return sensedGravity;
  }

  // Once the sand is settled there is nothing to visually destabilize, so a
  // real turn must override the three-minute timer on the first valid sample.
  if (autoReverseWaiting && strongReading && candidate != sensedGravity) {
    pendingGravity = -1;
    sensedGravity = candidate;
    return sensedGravity;
  }

  if (candidate == sensedGravity) {
    pendingGravity = -1;
    return sensedGravity;
  }

  if (candidate != pendingGravity) {
    pendingGravity = candidate;
    pendingGravitySince = millis();
    return sensedGravity;
  }

  if ((unsigned long)(millis() - pendingGravitySince) < ORIENTATION_STABLE_MS) return sensedGravity;

  pendingGravity = -1;
  sensedGravity = candidate;
  return sensedGravity;
}

void resyncDisplaysIfNeeded() {
  unsigned long now = millis();
  if ((unsigned long)(now - lastDisplaySyncAt) < DISPLAY_RESYNC_INTERVAL) return;

  lc.resync(DISPLAY_INTENSITY);
  lastDisplaySyncAt = now;
}

int getTopMatrix() {
  return (gravity < 180) ? MATRIX_A : MATRIX_B;
}
int getBottomMatrix() {
  return (gravity < 180) ? MATRIX_B : MATRIX_A;
}

void resetTime() {
  for (byte i=0; i<2; i++) {
    lc.clearDisplay(i);
  }
  fill(getTopMatrix(), 60);
  d.Delay(getDelayDrop() * 1000);
}

bool updateMatrix() {
  int n = 8;
  bool somethingMoved = false;
  byte x,y;
  bool direction;
  for (byte slice = 0; slice < 2*n-1; ++slice) {
    direction = (random(2) == 1);
    byte z = slice<n ? 0 : (byte)(slice-n + 1);
    for (byte j = z; j <= slice-z; ++j) {
      y = direction ? (7-j) : (7-(slice-j));
      x = direction ? (slice-j) : j;
      if (moveParticle(MATRIX_B, x, y)) {
        somethingMoved = true;
      };
      if (moveParticle(MATRIX_A, x, y)) {
        somethingMoved = true;
      }
    }
  }
  return somethingMoved;
}

boolean dropParticle() {
  if (d.Timeout()) {
    d.Delay(getDelayDrop() * 1000);
    if (gravity == 0 || gravity == 180) {
      if ((lc.getRawXY(MATRIX_A, 0, 0) && !lc.getRawXY(MATRIX_B, 7, 7)) ||
          (!lc.getRawXY(MATRIX_A, 0, 0) && lc.getRawXY(MATRIX_B, 7, 7))
      ) {
        lc.invertRawXY(MATRIX_A, 0, 0);
        lc.invertRawXY(MATRIX_B, 7, 7);
        tone(PIN_BUZZER, 440, 10);
        return true;
      }
    }
  }
  return false;
}

void alarm() {
  alarmBeepsRemaining = ALARM_BEEP_COUNT;
  nextAlarmBeepAt = millis();
}

void updateAlarm() {
  if (alarmBeepsRemaining == 0) return;
  if ((long)(millis() - nextAlarmBeepAt) >= 0) {
    tone(PIN_BUZZER, 440, 200);
    alarmBeepsRemaining--;
    nextAlarmBeepAt += ALARM_BEEP_INTERVAL;
  }
}

void cancelAlarm() {
  alarmBeepsRemaining = 0;
}

void startAutoReverseTimer() {
  autoReverseWaiting = true;
  autoReverseStartedAt = millis();
}

void cancelAutoReverseTimer() {
  autoReverseWaiting = false;
}

void autoReverseIfNeeded() {
  if (!autoReverseWaiting) return;
  if ((unsigned long)(millis() - autoReverseStartedAt) < AUTO_REVERSE_DELAY) return;

  gravity = (gravity == 0) ? 180 : 0;
  autoReverseWaiting = false;
  alarmWentOff = false;
  cancelAlarm();
  d.Delay(getDelayDrop() * 1000UL);
}

/**
 * Setup
 */
void setup() {
#if DEBUG_OUTPUT
  Serial.begin(9600);
#endif
  randomSeed((unsigned long)analogRead(A0));

  for (byte i=0; i<2; i++) {
    lc.shutdown(i,false);
    lc.setIntensity(i,DISPLAY_INTENSITY);
  }

  int initialGravity;
  if (readGravityCandidate(initialGravity)) gravity = initialGravity;
  sensedGravity = gravity;
  lc.setRotation((ROTATION_OFFSET + gravity) % 360);
  resetTime();
  lastDisplaySyncAt = millis();
}

/**
 * Main loop
 */
void loop() {
  delay(DELAY_FRAME);

  int previousSensedGravity = sensedGravity;
  int currentSensedGravity = getGravity();
  if (currentSensedGravity != previousSensedGravity) {
    gravity = currentSensedGravity;
    alarmWentOff = false;
    cancelAlarm();
    cancelAutoReverseTimer();
  }
  autoReverseIfNeeded();
  lc.setRotation((ROTATION_OFFSET + gravity) % 360);
  resyncDisplaysIfNeeded();

  bool moved = updateMatrix();
  bool dropped = dropParticle();

  if (dropped) {
    lc.resync(DISPLAY_INTENSITY);
    lastDisplaySyncAt = millis();
  }

  if (moved || dropped) {
    alarmWentOff = false;
    cancelAlarm();
    cancelAutoReverseTimer();
  }
  if (!moved && !dropped && !alarmWentOff && (countParticles(getTopMatrix()) == 0)) {
    alarmWentOff = true;
    alarm();
    startAutoReverseTimer();
  }
  updateAlarm();
}
