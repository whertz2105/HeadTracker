#!/usr/bin/env python3
"""Visualize drone and camera orientation on a PC.

This script connects to the drone ESP32 serial port. It plots two traces:
1. Drone body orientation from the onboard IMU.
2. Commanded pan/tilt angles derived from the control loop.

Use it during bench tests to confirm the compensation math behaves as expected.
"""

import argparse
import sys
from collections import deque

try:
    import serial  # type: ignore
except ImportError:  # pragma: no cover
    sys.exit("pyserial is required. Install with `pip install pyserial`.")

try:
    import matplotlib.pyplot as plt  # type: ignore
except ImportError:  # pragma: no cover
    sys.exit("matplotlib is required. Install with `pip install matplotlib`.")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Drone orientation visualizer")
    parser.add_argument("port", help="Serial port (e.g., COM6 or /dev/ttyUSB1)")
    parser.add_argument("--baud", type=int, default=115200)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    ser = serial.Serial(args.port, args.baud, timeout=1)

    drone_pitch = deque(maxlen=300)
    cam_tilt = deque(maxlen=300)

    plt.ion()
    fig, ax = plt.subplots(2, 1, sharex=True)
    drone_line, = ax[0].plot([], [], label="Drone Pitch")
    cam_line, = ax[1].plot([], [], label="Camera Tilt", color="orange")
    ax[0].set_ylabel("Drone Pitch (deg)")
    ax[1].set_ylabel("Camera Tilt (deg)")
    ax[1].set_xlabel("Sample")
    ax[0].legend()
    ax[1].legend()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue
            if "[Drone] Packet" in line:
                parts = line.split()
                try:
                    head_pitch = float(parts[7])
                except (IndexError, ValueError):
                    continue
            if "[Drone] Orientation" in line:
                parts = line.split()
                try:
                    drone_pitch = float(parts[6])
                except (IndexError, ValueError):
                    continue
                drone_pitch.append(float(parts[6]))
                drone_line.set_data(range(len(drone_pitch)), list(drone_pitch))
                ax[0].set_xlim(0, len(drone_pitch))
                ax[0].set_ylim(-90, 90)
                fig.canvas.draw()
                fig.canvas.flush_events()
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        ser.close()
        plt.ioff()
        plt.close(fig)


if __name__ == "__main__":
    main()
