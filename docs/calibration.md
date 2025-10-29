# IMU Calibration Guide

Calibrating both MPU-6050 sensors ensures the camera remains steady and aligned with the real world. Complete these steps right after flashing the firmware.

---

## 1. Warm-Up

1. Power both ESP32 boards for at least 5 minutes so the gyroscope temperature stabilizes.
2. Place each board on a stable, vibration-free surface.

---

## 2. Gyroscope Bias Capture

1. Open the PlatformIO serial monitor (`pio device monitor`) for the headset board.
2. Press the **space bar** once to trigger the bias capture (the firmware prompts you).
3. Keep the board perfectly still for 5 seconds while the offsets are measured.
4. Wait for the "Gyro bias saved" message.
5. Repeat the process for the drone board.

The offsets are stored in NVS (non-volatile storage) and automatically reloaded at boot.

---

## 3. Accelerometer Six-Face Calibration

1. Follow the prompts printed in the serial monitor:
   * Place the board flat with **Z up** (component side up) and press Enter.
   * Rotate to **Z down**, **X up**, **X down**, **Y up**, **Y down**, pressing Enter each time.
2. The firmware calculates scale factors to ensure 1 g readings on each axis.
3. The final RMS error should be under 0.03 g. If higher, repeat the routine.

---

## 4. Alignment Check

1. Hold the headset board in front of you with X forward, Y right, Z up.
2. Check the serial output: yaw ≈ 0°, pitch ≈ 0°, roll ≈ 0°.
3. For the drone board, mount it in the frame and verify the same orientation.

If the axes differ from the expected orientation, adjust the mounting or update the axis mapping in `imu_mpu6050.cpp`.

---

## 5. Saving & Backups

* The calibration data is stored in the ESP32 NVS partition with the keys `imu_gyro_bias` and `imu_accel_cal`.
* To back it up, note the values printed at the end of the calibration routine and paste them into a text file for future reference.

---

## 6. Troubleshooting

* **Offsets drift after boot:** Ensure the boards were motionless during calibration.
* **Large tilt error after calibration:** Re-run the six-face routine and verify the board sits flush on each face.
* **Headset and drone disagree by a few degrees:** Use the recenter button (headset) while both boards are level.

