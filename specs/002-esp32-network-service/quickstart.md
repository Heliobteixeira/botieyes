# Quickstart: ESP32 Network Control Service

**Feature**: 002-esp32-network-service | **Date**: 2026-06-01

This guide shows how to flash the network-enabled firmware to a TTGO LoRa32 and drive the eyes from
another machine on the same LAN. See [contracts/network-protocol.md](contracts/network-protocol.md) for the
full protocol and [plan.md](plan.md) for the design.

## Prerequisites

- TTGO LoRa32 (ESP32 + integrated SSD1306 128x64). OLED pins: SDA=GPIO4, SCL=GPIO15, RST=GPIO16.
- ESP-IDF v6.0.1 active shell (`source ~/.espressif/tools/activate_idf_v6.0.1.sh`).
- The existing `BotiEyes` library installed/available.
- A controller machine (macOS/Linux/Windows) on the **same Wi-Fi network**, with Python 3.8+.

## 1. Configure Wi-Fi and flash the device

1. Activate ESP-IDF and enter the project root:

  ```bash
  source ~/.espressif/tools/activate_idf_v6.0.1.sh
  cd esp-idf
  ```

2. Set the ESP target and configure Wi-Fi credentials:

  ```bash
  idf.py set-target esp32
  idf.py menuconfig
  ```

  In `menuconfig`, set SSID/password under `BotiEyes Network Control`.

3. Build, flash, and monitor:

  ```bash
  idf.py build flash monitor
  ```

4. On boot, the OLED shows the device **IP address** (e.g. `192.168.1.42`) once Wi-Fi connects, then the
   eyes start in autonomous idle behavior. Note the IP — you will point the controller at it.

> Per the repo commit policy, after changing anything under `BotiEyes/src/**` or `examples/**` that affects
> rendering/timing/I2C/display/API, build + upload to the board and visually verify before committing.

### Arduino fallback (temporary)

If you need the legacy Arduino path during migration, use
`BotiEyes/examples/NetworkControl/NetworkControl.ino`. ESP-IDF remains the
primary supported ESP32 path.

## 2. Drive the eyes from the controller

Use the reference Python client:

```bash
cd controller
python cli.py --host 192.168.1.42          # start an interactive control session
```

The CLI acquires the lock, sends ~1 s heartbeats automatically, and accepts commands:

```text
> emotion 0.35 0.55        # set valence/arousal (happy-ish)
> preset happy 1.0         # named preset with intensity
> gaze -45 0               # look left
> blink                    # blink both eyes
> wink left                # wink left eye
> idle off                 # disable idle behavior
> status                   # print current device state
> emotion -0.30 0.80       # angry-ish (supersedes prior streaming target)
> quit                     # release lock and exit
```

Or use the client library directly:

```python
from botieyes_client import BotiEyesClient

with BotiEyesClient("192.168.1.42") as eyes:   # acquires lock, starts heartbeat
    eyes.set_emotion(valence=0.35, arousal=0.55, duration_ms=400)
    eyes.preset("happy", intensity=1.0)
    eyes.set_gaze(h=-45, v=0)
    eyes.blink()
    print(eyes.status())                        # StatusReport
# context exit releases the lock
```

## 3. Validate the acceptance scenarios

| Scenario | How to verify | Expected (spec) |
|----------|---------------|-----------------|
| US1 low-latency emotion | Send `emotion` commands; watch the OLED | Eyes begin reacting in <100 ms (SC-001) |
| US1 rapid updates | Stream 20+ emotions/sec | Newest target shown, no stalling (SC-002, FR-008) |
| US1 out-of-range | `emotion 0.9 0.5` (valence > 0.5) | Rejected; eyes unchanged (FR-007, SC-007) |
| US2 network drop | Disable controller Wi-Fi 10–30 s, re-enable | Eyes stay animated; control resumes automatically (SC-003/SC-004) |
| US2 controller crash | Kill the CLI process; reconnect a new one | New controller takes over after ≤5 s (FR-012) |
| US2 lost network | Power device with Wi-Fi off | Eyes run idle autonomously; retries join (FR-016) |
| US3 connect by address | Use the IP shown on the OLED | Session established first try (SC-005) |
| US4 full control surface | Exercise preset/gaze/blink/wink/idle | Each behavior visible on device (FR-003–FR-006) |
| US5 status | `status` | Returned values match the OLED (FR-013) |
| Second controller | Run a second CLI against the same device | Rejected with `IN_USE` (FR-017) |

## 4. Run the host-native tests

Logic tests run off-target (no board required):

```bash
# from repo root — exact runner per tasks.md (PlatformIO native env or host compiler)
#   - test_command_codec:   frame encode/decode, truncated/oversized/malformed, range checks
#   - test_session_manager: acquire / reject-when-locked / takeover / heartbeat / 5 s timeout / newest-wins
```

Python client tests:

```bash
cd controller
python -m pytest
```

## Troubleshooting

- **No IP on OLED**: Wi-Fi failed to join. Check SSID/password; the device keeps retrying and runs idle
  meanwhile (FR-016).
- **`IN_USE` rejection**: Another controller holds the lock. Wait ≤5 s after it stops, or release it from the
  other client.
- **Eyes not reacting**: Confirm controller and device are on the same subnet and UDP port `4210` is not
  blocked; run `status` to confirm the session is healthy.
- **Eyes freeze**: Should never happen by design (SC-004); if observed, capture serial logs and the last
  commands sent — this indicates a render-loop stall to investigate.
