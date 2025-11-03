# Theory of Operation

This document explains how the head-tracking camera stays locked to the real world even while the drone pitches, rolls, and yaws. You do not need advanced math to follow along—every concept is described with plain language and simple diagrams.

---

## 1. Coordinate Frames

We describe orientations using three coordinate frames:

| Frame | Axes Definition | Notes |
|-------|-----------------|-------|
| **World** | X forward, Y right, Z up | Fixed to the ground (inertial) |
| **Drone Body** | X nose, Y right wing, Z down | Moves with the drone |
| **Headset** | X forward, Y right ear, Z up | Moves with the pilot's head |

We represent orientations with **quaternions** because they avoid gimbal lock and provide smooth interpolation.

---

## 2. IMU Fusion (Madgwick Filter)

Each ESP32 reads raw accelerometer and gyroscope data from its MPU-6050 sensor at 500 Hz. The Madgwick filter combines:

* **Gyroscope integration** for short-term orientation changes.
* **Accelerometer gravity vector** for long-term drift correction.

The result is a unit quaternion `q = (w, x, y, z)` describing the board's orientation relative to the world frame. The code in `firmware/common/madgwick.cpp` implements the update step with clear comments.

---

## 3. Aligning the Frames

* The headset node publishes its orientation `q_head_world` in the world frame.
* The drone node measures its own orientation `q_drone_world` in the world frame.

We need the camera orientation **relative to the drone body** so we can command servos mounted on the drone. To do that we compute the relative quaternion:

```text
q_relative = conjugate(q_drone_world) × q_head_world
```

The conjugate operation essentially "subtracts" the drone orientation from the head orientation. The result describes where the pilot is looking **from the drone's perspective**.

---

## 4. Extracting Pan and Tilt

From `q_relative` we derive Euler angles using the aerospace ZYX convention:

* **Yaw** → rotates around the drone's Z (down) axis, controls pan.
* **Pitch** → rotates around the drone's Y (right) axis, controls tilt.
* **Roll** is ignored for servo commands but used for stability checks.

The conversion function in `servo_map.h` clamps tilt to a safe range (+30° up to −90° down) and ensures pan stays within the servo's mechanical limits.

---

## 5. Mode Logic

| Mode | Behavior |
|------|----------|
| **Only Horizontal** | Pan follows head yaw. Tilt equals `pitch_head − pitch_drone`, ignoring roll so the horizon stays level even while banking. |
| **Free Move** | Pan and tilt follow the full `q_relative` orientation. The camera feels like it is hanging in space, locked to the pilot's view of the ground. |
| **Bombing** | Ignores head input; smoothly drives tilt to −90° (straight down) and pan to 0° (aligned with drone nose). |

The mode selection comes from the PWM signal on GPIO27. The firmware averages several readings to avoid jitter when the switch is between positions.

---

## 6. Filtering and Slew Limits

* **Low-Pass Filter:** A first-order filter (~5 Hz cutoff) smooths the commanded angles to remove jitter from the radio link.
* **Slew Rate Limit:** Pan changes are limited to 120° per second, tilt to 90° per second. This protects the servos and prevents sudden jumps on packet loss recovery.

---

## 7. Failsafe Strategy

If the drone node does not receive a valid packet for more than 150 ms (~5 packets), it begins blending the camera back toward neutral (pan 0°, tilt 0°) over 0.75 seconds. As soon as a new packet arrives, the normal control loop resumes.

This ensures the camera never points unpredictably if the pilot walks behind a building or the headset loses power.

---

## 8. Latency Budget

| Stage | Approx. Time |
|-------|--------------|
| Headset IMU sample + filter | 2 ms |
| Packet encoding + UART transmit | 4 ms |
| Radio propagation | ~1 µs |
| Drone UART receive + decode | 4 ms |
| Drone IMU update | 2 ms |
| Servo command update | 10–15 ms (servo internal) |

Total: **<30 ms** typical, meeting the target of <50 ms end-to-end.

---

## 9. Extending the System

* Add magnetometers for absolute yaw lock by integrating an MPU-9250. Replace the Madgwick update step with the magnetometer-enabled version.
* Integrate camera stabilization by feeding PID-corrected rates into the servo commands.
* Send telemetry back to the headset (e.g., packet loss, mode) by extending the packet structure and blinking patterns.

Understanding the theory behind the code makes it easier to adapt the project to your own aircraft or headgear designs.

