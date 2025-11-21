# Betaflight Setup for Mode Switching

This walkthrough shows you how to map the Radiomaster Pocket three-position switch to the F722 AUX output that feeds the drone ESP32. Screenshots are represented as ASCII blocks so you can follow along without external images.

---

## 1. Prerequisites

* Betaflight Configurator 10.9 or newer installed on your PC.
* Radiomaster transmitter bound to the ELRS receiver on your drone.
* USB cable connected between the flight controller and the PC.

---

## 2. Assign the Switch to an AUX Channel

1. Power on the transmitter and connect the flight controller to Betaflight Configurator.
2. Open the **Receiver** tab.
3. Flip the three-position switch; note which AUX channel moves (e.g., AUX3).
4. If nothing moves, configure the transmitter to map the switch to a channel.

```text
Receiver Tab View (example)
+--------------------------------------------------+
| Channel | Value | Notes                          |
| AUX1    | 1500  | Arm switch                     |
| AUX2    | 1000  | Turtle mode                    |
| AUX3    | 1000/1500/2000  ← three-position switch|
+--------------------------------------------------+
```

---

## 3. Configure a Servo Output

1. Go to the **Outputs** tab.
2. Enable a free servo channel (e.g., SERVO 1).
3. Assign **AUX3** as the source.
4. Set the output range to 1000–2000 µs.
5. Save and reboot the flight controller when prompted.

```text
Outputs Tab (simplified)
+---------+--------+---------+---------+
| Servo # | Source | Min µs  | Max µs  |
| Servo1  | AUX3   | 1000    | 2000    |
+---------+--------+---------+---------+
```

---

## 4. Map the Output Pin

1. Open the **Ports** tab and verify that the desired pad (e.g., S5) is assigned to the servo output.
2. Refer to your F722 wiring diagram to identify the physical pad.
3. Solder a signal wire from the pad to ESP32 GPIO27.

---

## 5. Verify PWM Levels

1. Disconnect the USB cable and power the drone from the flight battery so the BEC powers the ESP32.
2. Use a multimeter or oscilloscope to confirm the AUX output swings between roughly 1.0 ms, 1.5 ms, and 2.0 ms when you move the switch.
3. The firmware interprets these ranges as:
   * **>1700 µs** → Bombing mode
   * **1300–1700 µs** → Free Move mode
   * **<1300 µs** → Only Horizontal mode

---

## 6. Save Your Configuration

* Use `diff all` in the Betaflight CLI and save the output as documentation for future reference.
* Consider backing up the entire configuration (`dump all`) in case you need to restore it later.

---

## 7. Troubleshooting

* **Switch does nothing:** Ensure the transmitter mixes the switch to an AUX channel and that the receiver is bound.
* **Wrong mode triggered:** Adjust the midpoints and endpoints on your transmitter to hit 1000, 1500, and 2000 µs cleanly.
* **No PWM output:** Confirm the pad is a servo-capable output in Betaflight and that the F722 is powered.

