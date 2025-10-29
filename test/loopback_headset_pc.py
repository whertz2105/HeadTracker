#!/usr/bin/env python3
"""Display headset orientation in real time on a PC.

This script listens to the ESP32 serial port and plots yaw/pitch/roll values.
It is intentionally simple so it runs on any computer without extra drivers.
"""

import argparse
import sys
import time
from collections import deque

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover - executed on user machines
    sys.exit("pyserial is required. Install with `pip install pyserial`." )

try:
    import matplotlib.pyplot as plt  # type: ignore
except ImportError as exc:  # pragma: no cover
    sys.exit("matplotlib is required. Install with `pip install matplotlib`." )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Headset orientation viewer")
    parser.add_argument("port", help="Serial port of the headset ESP32 (e.g., COM5 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    ser = serial.Serial(args.port, args.baud, timeout=1)

    yaw_data = deque(maxlen=200)
    pitch_data = deque(maxlen=200)
    roll_data = deque(maxlen=200)
    timestamps = deque(maxlen=200)

    plt.ion()
    fig, ax = plt.subplots()
    yaw_line, = ax.plot([], [], label="Yaw")
    pitch_line, = ax.plot([], [], label="Pitch")
    roll_line, = ax.plot([], [], label="Roll")
    ax.set_ylim(-180, 180)
    ax.set_xlabel("Sample")
    ax.set_ylabel("Degrees")
    ax.legend()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue
            if "[Headset] Seq" not in line:
                continue
            parts = line.split()
            try:
                yaw = float(parts[4])
                pitch = float(parts[6])
                roll = float(parts[8])
            except (IndexError, ValueError):
                continue
            timestamps.append(time.time())
            yaw_data.append(yaw)
            pitch_data.append(pitch)
            roll_data.append(roll)

            yaw_line.set_data(range(len(yaw_data)), list(yaw_data))
            pitch_line.set_data(range(len(pitch_data)), list(pitch_data))
            roll_line.set_data(range(len(roll_data)), list(roll_data))
            ax.set_xlim(0, len(yaw_data))
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
