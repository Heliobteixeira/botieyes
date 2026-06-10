# Host-native tests for the BotiEyes network layer

These tests exercise the **platform-agnostic** parts of the network control
service (`CommandCodec` and `SessionManager`) off-target — no ESP32, no Wi-Fi,
no hardware required. They compile against `BotiEyes/src/net/` with a tiny
header-only harness (`test_harness.h`).

## Run

```bash
cd tests/net
make            # builds and runs all tests
make clean
```

Or run a single suite:

```bash
make test_command_codec && ./test_command_codec
make test_session_manager && ./test_session_manager
```

## What is covered

- `test_command_codec`: frame encode/decode round-trips, truncated/oversized
  frames, bad version, unknown opcode, and out-of-range rejection (no clamping).
- `test_session_manager`: lock acquire / reject-when-locked / takeover,
  heartbeat refresh, 5 s no-contact timeout, and newest-wins coalescing.

The ESP32 transport adapter (`BotiEyesServer`) is intentionally **not** built
here; it depends on the Arduino/Wi-Fi stack and is validated on hardware.

## Flood / stability test (on hardware, SC-006)

`flood_test.py` is an on-hardware soak test: it streams emotion/gaze updates at
~10× the normal rate for a configurable duration to confirm newest-wins
coalescing holds up, the device does not crash, and rendering/memory stay
stable.

```bash
cd controller
python ../tests/net/flood_test.py --host <device-ip> --duration 300
```

Pass criteria (manual observation on the OLED + final status query):
- Eyes keep animating smoothly; latest target always wins (no stalling/lag
  buildup).
- No device reset across the run; `status()` still responds at the end.
- Heap stays stable (watch the ESP32 serial log if instrumented).

