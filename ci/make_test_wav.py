#!/usr/bin/env python3
"""Write a short test WAV (mono, 16-bit PCM): a chirp sweeping 200 Hz -> 4 kHz.

Standard library only, so CI needs nothing beyond python3.
Usage: make_test_wav.py <out.wav> [seconds] [sample_rate]
"""

import math
import struct
import sys
import wave


def main() -> None:
    path = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 3.0
    rate = int(sys.argv[3]) if len(sys.argv) > 3 else 11025
    start_hz, end_hz = 200.0, 4000.0

    frames = bytearray()
    for n in range(int(seconds * rate)):
        t = n / rate
        # Linear chirp: the phase is the integral of the instantaneous frequency.
        phase = 2.0 * math.pi * (start_hz * t + (end_hz - start_hz) * t * t / (2.0 * seconds))
        frames += struct.pack("<h", int(0.8 * 32767 * math.sin(phase)))

    with wave.open(path, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(bytes(frames))


if __name__ == "__main__":
    main()
