# LED Hourglass Firmware

Firmware for an Arduino Nano (ATmega328P), two daisy-chained MAX7219 8×8
matrices, an analog accelerometer, and a passive buzzer.

## Behavior

- Simulates 60 conserved sand particles with a one-minute nominal cycle.
- Reverses once the opposite vertical orientation is stable for 200 ms.
- Automatically reverses a completed hourglass after three minutes without a
  physical flip; later physical movement immediately retakes control.
- A valid physical turn during the three-minute pause interrupts it on the
  next 100 ms frame; debounce remains active only while sand is moving.
- Ignores sideways transitions, threshold chatter, and implausible rail
  readings from a disconnected accelerometer.
- Replays MAX7219 configuration and buffered pixels every second so a display
  can recover after being disconnected without resetting the Nano.
- Plays five nonblocking completion beeps; sensing and display recovery remain
  active throughout the alarm.
- Uses the original steady MAX7219 intensity so individual grain movement
  remains clearly visible.

## Pins

| Function | Nano pin |
| --- | --- |
| MAX7219 data | D5 |
| MAX7219 clock | D4 |
| MAX7219 load/CS | D6 |
| Accelerometer X | A1 |
| Accelerometer Y | A2 |
| Buzzer | A0 / D14 |

## Verification

Run the host replay suite from this directory:

```powershell
.\tests\run_reconnect_test.ps1
```

The suite covers display reconnection, full reverse flow, noisy orientation
changes, timer rollover, nonblocking alarms, sensor disconnection/reconnection,
particle conservation, and 60 complete cycles across five random seeds.
It also verifies the three-minute automatic reverse and physical takeover.

Compile and upload with Arduino CLI:

```powershell
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 .
arduino-cli upload -p COM5 --fqbn arduino:avr:nano:cpu=atmega328 .
```

The matrices are physically monochrome; changing LED color requires RGB
matrix hardware. Firmware can reject obvious disconnected-sensor rail values,
but no analog-only check can identify every possible floating voltage. Secure
connectors remain the hardware-level protection.
