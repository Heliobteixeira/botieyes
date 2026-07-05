"""Interactive CLI for driving a BotiEyes ESP32 device over the network.

Usage:
    python3 cli.py --host 192.168.1.139 [--port 4210]

Then type commands at the prompt (see `help`).
"""

from __future__ import annotations

import argparse
import sys

from botieyes_client import BotiEyesClient, BotiEyesError

HELP = """\
Commands:
  emotion <valence> <arousal> [duration_ms]   set emotion (v: -0.5..0.5, a: 0..1)
  preset <name> [intensity]                   named preset (e.g. happy 1.0)
  gaze <h> <v> [duration_ms]                  gaze angles (h: -90..90, v: -45..45)
  blink [duration_ms]                         blink both eyes
  wink <left|right> [duration_ms]             wink one eye
  idle <on|off>                               toggle idle behavior
  status                                      print device status
  help                                        show this help
  quit                                        release lock and exit
"""


def _dispatch(eyes: BotiEyesClient, parts: list[str]) -> bool:
    cmd = parts[0].lower()
    try:
        if cmd == "emotion":
            v, a = float(parts[1]), float(parts[2])
            dur = int(parts[3]) if len(parts) > 3 else 400
            eyes.set_emotion(v, a, dur)
        elif cmd == "preset":
            name = parts[1]
            intensity = float(parts[2]) if len(parts) > 2 else 1.0
            print(eyes.preset(name, intensity).reason_name)
        elif cmd == "gaze":
            h, v = int(parts[1]), int(parts[2])
            dur = int(parts[3]) if len(parts) > 3 else 300
            eyes.set_gaze(h, v, dur)
        elif cmd == "blink":
            dur = int(parts[1]) if len(parts) > 1 else 150
            print(eyes.blink(dur).reason_name)
        elif cmd == "wink":
            eye = parts[1] if len(parts) > 1 else "left"
            dur = int(parts[2]) if len(parts) > 2 else 150
            print(eyes.wink(eye, dur).reason_name)
        elif cmd == "idle":
            enable = parts[1].lower() in ("on", "true", "1", "enable")
            print(eyes.set_idle(enable).reason_name)
        elif cmd == "status":
            print(eyes.status())
        elif cmd in ("help", "?"):
            print(HELP)
        elif cmd in ("quit", "exit", "q"):
            return False
        else:
            print(f"Unknown command: {cmd!r} (type 'help')")
    except (IndexError, ValueError):
        print(f"Bad arguments for {cmd!r} (type 'help')")
    except BotiEyesError as exc:
        print(f"Error: {exc}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="BotiEyes network controller CLI")
    parser.add_argument("--host", required=True, help="Device IP shown on the OLED")
    parser.add_argument("--port", type=int, default=4210, help="Device UDP port")
    args = parser.parse_args()

    try:
        with BotiEyesClient(args.host, args.port) as eyes:
            print(f"Connected to {args.host}:{args.port}. Type 'help' for commands.")
            while True:
                try:
                    line = input("> ").strip()
                except EOFError:
                    break
                if not line:
                    continue
                if not _dispatch(eyes, line.split()):
                    break
    except BotiEyesError as exc:
        print(f"Could not acquire control: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"Network error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
