# HeadTracker Self-Test Checklist

Print this page and check off each item as you complete your bench tests. Working through the list ensures the system is safe to fly.

---

## 1. Preparation

- [ ] Charge the headset power bank and drone LiPo.
- [ ] Inspect all cables for nicks or loose solder joints.
- [ ] Confirm HC-12 antennas are firmly attached.

## 2. Headset Node

- [ ] Connect headset ESP32 via USB and open `pio device monitor` at 115200 baud.
- [ ] Verify `[Headset] Ready.` appears within 5 seconds of reset.
- [ ] Move the headset; confirm yaw/pitch/roll values update in the console.
- [ ] Press the recenter button; yaw resets to 0° and LED turns solid.
- [ ] Run `python test/loopback_headset_pc.py <PORT>` and observe smooth plots.

## 3. Drone Node (USB Power)

- [ ] Connect drone ESP32 via USB and open the serial monitor.
- [ ] Confirm `[Drone] Ready.` message.
- [ ] Observe packet logs arriving when the headset is powered.
- [ ] Toggle transmitter switch; note mode changes in the console.
- [ ] Move the drone board; verify orientation prints respond immediately.

## 4. Servo Test (External 5 V Supply)

- [ ] Power servos from BEC (disconnect props for safety).
- [ ] With headset stationary, servos should hold pan=0°, tilt=0°.
- [ ] Rotate headset yaw; pan servo follows smoothly with no jitter.
- [ ] Pitch headset down/up; tilt servo follows within +30° to −90° range.
- [ ] Switch to Bombing mode; camera moves to pan=0°, tilt=−90° within 0.5 s.
- [ ] Disconnect headset power; servos return to neutral within 1 s (failsafe).

## 5. Flight-Ready Checks

- [ ] Mount the drone IMU securely and rerun calibration if removed.
- [ ] Confirm BEC output remains at 5.0 V under servo load.
- [ ] Check transmitter AUX channel endpoints reach 1000/1500/2000 µs.
- [ ] Verify all grounds (BEC, ESP32, FC, servos) are tied together.
- [ ] Secure all wires and antennas with zip ties or tape.

## 6. Documentation

- [ ] Update `hardware/bom_master.md` by running `make bom`.
- [ ] Note any deviations or improvements in `docs/troubleshooting.md`.
- [ ] Store calibration values in a safe place.

Sign and date when complete to keep maintenance logs current.

