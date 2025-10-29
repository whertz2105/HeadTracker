# Headset Node Wiring Guide

This document walks you through building the headset node harness. Follow the steps in order and check every connection with a multimeter before powering up.

---

## 1. Parts Checklist

| Qty | Item | Notes |
|-----|------|-------|
| 1 | ESP32-WROOM development board (micro USB) | Provides processing power and USB connector |
| 1 | MPU-6050 breakout board | Accelerometer + gyroscope sensor |
| 1 | HC-12 433 MHz wireless module | Long-range serial radio |
| 1 | Toggle button (optional) | Recenter camera orientation |
| 1 | 5 V USB power bank | Supplies the headset |
| 1 | Breadboard or perfboard | Optional for tidier layout |
| ~10 | Female-to-female jumper wires | For quick prototyping |
| 1 | Status LED + 220 Ω resistor | Visual heartbeat |

All part numbers and suppliers are listed in [`hardware/bom_headset.csv`](../hardware/bom_headset.csv).

---

## 2. Pin Assignments

| ESP32 Pin | Signal | Connects To | Why |
|-----------|--------|-------------|-----|
| 3V3 | Power | MPU-6050 VCC | Sensor power |
| GND | Ground | MPU-6050 GND, HC-12 GND, button GND | Common reference |
| 21 | I²C SDA | MPU-6050 SDA | Sensor data |
| 22 | I²C SCL | MPU-6050 SCL | Sensor clock |
| 17 | UART TX | HC-12 RX | Send packets |
| 16 | UART RX | HC-12 TX | Receive acknowledgments (unused) |
| 4 | HC-12 SET | HC-12 SET pin | Enter AT mode for configuration |
| 2 | Status LED | LED anode (through 220 Ω) | Blinks for activity |
| 0 | Recenter button | Button to ground | Optional recenter |

> **Tip:** Keep the HC-12 antenna at least 5 cm away from the ESP32 antenna to avoid interference.

---

## 3. Wiring Diagram

```text
               +-------------------------------+
               |         ESP32 Dev Board       |
               |                               |
      3V3 o----+----o VCC  MPU-6050            |
      GND o----+----o GND  MPU-6050            |
      SDA o----+21  o SDA                       |
      SCL o----+22  o SCL                       |
      TX  o----+17  o RX   HC-12                |
      RX  o----+16  o TX   HC-12                |
      SET o----+4   o SET  HC-12                |
      LED o----+2   o-->|--[220Ω]---> GND       |
      BTN o----+0   o Button ----> GND          |
               |                               |
               +-------------------------------+
```

---

## 4. Step-by-Step Assembly

1. **Mount the ESP32** on a breadboard or printed headset mount.
2. **Connect the MPU-6050** using short wires to minimize noise.
   * Keep SDA/SCL twisted or side-by-side to reduce EMI.
   * Tie INT pin to nothing; we use polling.
3. **Attach the HC-12 module** using female jumpers for quick disconnection.
   * Route antenna away from USB cable.
   * Solder a 470 µF capacitor across VCC/GND near the module for clean power.
4. **Install the status LED.** Connect the anode to GPIO2 via a 220 Ω resistor, cathode to ground.
5. **Wire the optional recenter button.** One leg to GPIO0, the other to ground. The firmware enables the internal pull-up so no resistor is required.
6. **Bundle the wires** with zip ties or braided sleeving. Label each connector using the printable cards from [`hardware/printable_label_cards.pdf`](../hardware/printable_label_cards.pdf).
7. **Perform continuity checks** with a multimeter to ensure no shorts between 3V3 and GND.

---

## 5. Powering the Headset

* Use a USB power bank or a 5 V regulator capable of at least 500 mA.
* The ESP32 has an onboard 3.3 V regulator that powers the MPU-6050 and HC-12.
* Confirm the HC-12 sees 3.3 V; do not feed it 5 V directly.

---

## 6. Pre-Flight Checklist

1. Plug the headset ESP32 into your PC via USB.
2. Open the PlatformIO serial monitor at 115200 baud (`pio device monitor`).
3. Press the reset button; you should see messages indicating MPU-6050 and HC-12 status.
4. Move the headset – the serial console should show orientation updates.
5. Press the optional recenter button; the yaw should reset to 0°.

If any of these steps fail, consult [`docs/troubleshooting.md`](troubleshooting.md) for targeted fixes.

