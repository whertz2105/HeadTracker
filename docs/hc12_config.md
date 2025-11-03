# HC-12 Configuration Guide

The HC-12 serial radio is the long-range link between the headset and drone nodes. This guide shows you how to program both modules with identical settings.

---

## 1. Required Gear

* HC-12 module
* USB-to-serial adapter (3.3 V level) or an Arduino set to 3.3 V
* Jumper wires
* Terminal program (PlatformIO serial monitor, PuTTY, CoolTerm, etc.)

> **Safety:** The HC-12 is a 3.3 V device. Do **not** connect it to 5 V serial pins.

---

## 2. Wiring for AT Mode

| USB Adapter Pin | HC-12 Pin |
|-----------------|-----------|
| 3.3 V | VCC |
| GND | GND |
| TX | RX |
| RX | TX |
| DTR or manual jumper | SET |

To enter AT mode, pull the **SET** pin low **before** applying power.

---

## 3. AT Command Session

1. Open a terminal at **9600 baud**, 8 data bits, no parity, 1 stop bit.
2. Type each command followed by Enter. The module replies with `OK+...` messages.

```text
AT               → checks connection
AT+B9600         → set UART baud rate to 9600
AT+P8            → set transmit power to level 8 (max)
AT+C032          → select channel 32 (adjust if you need a different frequency)
AT+FU3           → set FU3 low-latency transparent mode
AT+RX            → optional; displays current settings
```

3. Remove power, release the SET pin, then power again for normal operation.

Repeat the process for both modules so they share the same channel and settings.

---

## 4. Integrating with the ESP32

* Connect the HC-12 TX/RX pins to ESP32 GPIO16/17 respectively.
* Tie HC-12 GND to ESP32 GND and to the rest of the system ground.
* Leave the SET pin floating during normal use; pull it low for future configuration.

---

## 5. Range Testing Tips

* Keep antennas vertical and at least one wavelength (~69 cm) above ground for best range.
* Avoid pointing the drone's carbon arms directly at the receiving module.
* Use the `test/loopback_headset_pc.py` tool to monitor packet counts while walking away.

