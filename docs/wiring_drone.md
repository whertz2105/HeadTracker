# Drone Node Wiring Guide

This guide explains how to integrate the drone-side ESP32 with your F722 flight controller, pan/tilt servos, and camera mount. Work slowly and double-check polarity before powering up.

---

## 1. Parts Checklist

| Qty | Item | Notes |
|-----|------|-------|
| 1 | ESP32-WROOM development board | Main controller |
| 1 | MPU-6050 breakout | Mounted near the gimbal pivot |
| 1 | HC-12 wireless module | Receives head orientation |
| 2 | 12 g digital metal-gear servos | Pan and tilt axes |
| 1 | 5 V BEC rated ≥3 A | Powers both servos and HC-12 |
| 1 | 470 µF, 10 V electrolytic capacitor | Across servo rail |
| 1 | 3-position gimbal mount | Printed or purchased |
| 1 | Connection to F722 AUX PWM output | Mode selection |
| misc | Servo extension wires, heatshrink, zip ties | Cable management |

---

## 2. Pin Assignments

| ESP32 Pin | Signal | Connects To | Notes |
|-----------|--------|-------------|-------|
| VIN (5 V) | Power in | BEC 5 V output | The ESP32 regulator feeds 3.3 V rail |
| GND | Ground | BEC ground, servo ground, FC ground | Tie all grounds together |
| 21 | I²C SDA | MPU-6050 SDA | Keep short |
| 22 | I²C SCL | MPU-6050 SCL | Keep short |
| 17 | UART TX | HC-12 RX | Data to radio |
| 16 | UART RX | HC-12 TX | Not used but keep connected |
| 4 | HC-12 SET | HC-12 SET | Needed for AT config |
| 25 | Pan servo PWM | Servo signal wire | LEDC channel 0 |
| 26 | Tilt servo PWM | Servo signal wire | LEDC channel 1 |
| 27 | Mode PWM input | F722 AUX output | Uses ESP32 RMT peripheral |
| 2 | Status LED | LED anode via 220 Ω to GND | Blink patterns for status |

---

## 3. Power Distribution

```text
              +------------- 5 V BEC -------------+
              |                                    |
         +----+----+                           +----+----+
         | ESP32  |                           | Servos |
         | VIN/GND|                           | +5/GND |
         +----+----+                           +----+----+
              |                                    |
          Common ground ----------------------------+
```

* Install the 470 µF capacitor between the servo +5 V and GND terminals to absorb spikes.
* Keep servo wires twisted to reduce EMI into the IMU.

---

## 4. Servo Connections

1. Set both servos to their neutral 1500 µs position using a servo tester.
2. Mount the servos into the gimbal frame with arms aligned for level camera orientation.
3. Connect servo signal wires directly to GPIO25 (pan) and GPIO26 (tilt).
4. Run the servo power wires to the 5 V BEC output, not to the ESP32 board.

---

## 5. Flight Controller Integration

* The F722 AUX channel must output a standard 1000–2000 µs PWM signal.
* Route the signal wire to ESP32 GPIO27.
* Tie the flight controller ground to the ESP32 ground; without this the PWM signal will be unreadable.
* See [`docs/betaflight_setup.md`](betaflight_setup.md) for screenshots of the Betaflight configuration tabs.

---

## 6. HC-12 Placement

* Mount the HC-12 away from carbon fiber and ESC wiring.
* Use the included spring antenna or an external half-wave whip.
* Keep the SET pin accessible; you will use a jumper to pull it low for AT mode.

---

## 7. Mounting the IMU

* Attach the MPU-6050 to the gimbal baseplate so it measures the drone body motion, not the gimbal carriage.
* Align the board so that the printed X arrow points forward and the Z arrow points up.
* Secure the board with foam tape and zip ties to eliminate vibration.

---

## 8. Pre-Flight Checklist

1. Verify 5 V power at the servo rail with the BEC running.
2. Plug in the ESP32 USB cable and open the serial monitor at 115200 baud.
3. Confirm the IMU initializes without errors.
4. Move the drone; you should see yaw/pitch/roll updates along with servo commands.
5. Toggle the transmitter three-position switch; the serial log should show the selected mode.
6. Disconnect USB, power from the BEC, and re-run the tests.

If something fails, visit [`docs/troubleshooting.md`](troubleshooting.md) for LED blink meanings and fault isolation steps.

