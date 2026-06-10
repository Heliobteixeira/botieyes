"""On-hardware flood / stability soak test for the BotiEyes network service (SC-006).

Streams emotion + gaze updates at ~10x the normal rate for a configurable
duration to verify newest-wins coalescing, crash-free operation, and stable
rendering/memory. Requires a running device on the LAN.

Usage (from the controller/ directory so botieyes_client is importable):
    python ../tests/net/flood_test.py --host 192.168.1.42 --duration 300
"""

from __future__ import annotations

import argparse
import math
import sys
import time

from botieyes_client import BotiEyesClient, BotiEyesError


def main() -> int:
    p = argparse.ArgumentParser(description="BotiEyes network flood/stability test")
    p.add_argument("--host", required=True, help="Device IP shown on the OLED")
    p.add_argument("--port", type=int, default=4210)
    p.add_argument("--duration", type=float, default=300.0, help="seconds (default 300)")
    p.add_argument("--rate", type=float, default=200.0, help="updates/sec (default ~10x)")
    args = p.parse_args()

    interval = 1.0 / args.rate
    sent = 0
    start = time.time()

    try:
        with BotiEyesClient(args.host, args.port) as eyes:
            print(f"Flooding {args.host}:{args.port} at {args.rate:.0f}/s "
                  f"for {args.duration:.0f}s...")
            while time.time() - start < args.duration:
                t = time.time() - start
                # Sweep emotion + gaze continuously so newest-wins is exercised.
                valence = 0.5 * math.sin(t * 1.3)
                arousal = 0.5 + 0.5 * math.sin(t * 0.7)
                eyes.set_emotion(valence, arousal, duration_ms=120)
                eyes.set_gaze(int(80 * math.sin(t)), int(40 * math.cos(t)),
                              duration_ms=120)
                sent += 2
                time.sleep(interval)

            # Final reconciliation: device should still respond.
            status = eyes.status()
            elapsed = time.time() - start
            print(f"\nSent {sent} updates in {elapsed:.1f}s "
                  f"({sent / elapsed:.0f}/s).")
            print(f"Device still responsive. Final status: {status}")
            print("PASS if eyes animated smoothly and no reset occurred.")
        return 0
    except BotiEyesError as exc:
        print(f"FAIL: device error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"FAIL: network error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
