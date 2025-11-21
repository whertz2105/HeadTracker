# Troubleshooting Guide

Use this guide whenever something unexpected happens during assembly, flashing, or flight testing. Start with the symptom table, then follow the detailed steps.

---

## 1. LED Status Codes (Drone Node)

| Pattern | Meaning | Action |
|---------|---------|--------|
| Slow blink (1 Hz) | Booting / initializing IMU | Wait 3 seconds |
| Double blink | HC-12 link active, receiving packets | Normal operation |
| Rapid blink (5 Hz) | Packet loss failsafe active | Check radio link |
| Solid on | Error (IMU or radio failure) | Connect USB and read serial log |

The headset LED blinks at 2 Hz during normal operation and goes solid when the recenter button is held.

---

## 2. Symptom Matrix

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| No serial output | Wrong COM port or driver missing | Install CP210x/CH340 drivers, reselect port |
| HC-12 does not respond to AT commands | SET pin not grounded before power | Power cycle with SET held low |
| Servos twitch randomly | Missing ground reference or noisy power | Tie all grounds, add 470 µF capacitor |
| Camera drifts slowly | IMU not calibrated | Follow [`docs/calibration.md`](calibration.md) |
| Mode switch ignored | PWM wire loose or wrong AUX mapping | Revisit [`docs/betaflight_setup.md`](betaflight_setup.md) |
| Servo slams to stop | Failsafe triggered | Check packet LED pattern, antenna orientation |

---

## 3. Serial Console Commands

Both firmware builds provide a simple command interface at 115200 baud:

| Key | Effect |
|-----|--------|
| `r` | Print raw IMU data |
| `o` | Print current quaternion and Euler angles |
| `p` | Print packet statistics |
| `f` | Force failsafe for testing |
| `c` | Re-run calibration routine |

If no response, ensure the serial monitor sends newline characters (LF) and that you selected the correct COM port.

---

## 4. Radio Link Checklist

1. Confirm both HC-12 modules share the same `AT+Cxxx` channel and `AT+FU3` mode.
2. Inspect antennas for secure solder joints.
3. Keep the headset antenna vertical; mount the drone antenna away from carbon fiber.
4. Run the `test/loopback_headset_pc.py` tool to watch packet sequence numbers. Drops over 10% indicate interference or misaligned antennas.

---

## 5. Servo Health Check

1. Disconnect the servos and connect them to a standalone servo tester.
2. Sweep through the full range to confirm smooth motion.
3. If a servo chatters only on the drone, verify the BEC can supply 2–3 A and that the ground is solid.
4. Re-enable the servos in firmware and monitor the serial log for "slew limit" warnings, which indicate the command is changing faster than permitted.

---

## 6. IMU Noise Troubleshooting

* If the gyro noise exceeds 0.05 °/s RMS, add foam padding beneath the board.
* Route power wires away from the IMU to avoid magnetic interference.
* Use shielded twisted pair for SDA/SCL if noise persists.

---

## 7. Recovery Steps for Corrupted NVS

1. Hold the boot button while powering the ESP32 to enter bootloader mode.
2. In PlatformIO, run `pio run -t erase` for the target environment.
3. Reflash the firmware and redo the calibration.

---

## 8. Getting Help

* Re-read the relevant documentation files—they contain the most common answers.
* Search the PlatformIO community forums for driver issues.
* If you file an issue, include:
  * PlatformIO environment (`pio system info` output)
  * Serial console log (copy/paste)
  * Steps that reproduce the problem

Documenting your findings helps future pilots build with confidence.

