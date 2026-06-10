# Hardware Validation Checklist: ESP32 Network Control Service

Feature: 002-esp32-network-service
Board: TTGO LoRa32 (ESP32 + SSD1306 128x64)
Date: 2026-06-09

This checklist is the shortest practical on-device pass to close the remaining hardware-only tasks:
- T019: US1 latency + newest-wins
- T026: US2 resilience + recovery
- T030: US3 connect-by-address + single-controller lock
- T036: US4 full expressive surface
- T041: US5 status accuracy
- T047: full quickstart validation

## Setup

1. Edit [BotiEyes/examples/NetworkControl/NetworkControl.ino](BotiEyes/examples/NetworkControl/NetworkControl.ino) and set:

```cpp
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASS = "your-password";
```

2. Flash the sketch to the TTGO LoRa32.
3. Open the serial monitor at `115200`.
4. Wait for the OLED to show the device IP and UDP port `4210`.
5. On the controller machine, from repo root:

```bash
cd controller
python3 cli.py --host <device-ip>
```

Expected:
- CLI connects on the first try.
- OLED is animating before any remote commands arrive.
- Serial monitor shows Wi-Fi connected and server started.

## One-Pass Validation

### A. US3 connect-by-address and lock behavior

1. Use the IP shown on the OLED to connect the first CLI.
2. In a second terminal, start another CLI against the same IP:

```bash
cd controller
python3 cli.py --host <device-ip>
```

Expected:
- First client connects successfully.
- Second client fails with `IN_USE` / could not acquire control.

Tasks covered:
- T030
- T047 (partial)

### B. US1 low-latency emotion control

In the first CLI:

```text
emotion 0.35 0.55
emotion -0.30 0.80
emotion 0.00 0.10
```

Expected:
- Visible reaction begins in under 100 ms.
- Expression changes smoothly.
- No stall or long lag between command and visible response.

Tasks covered:
- T019

### C. US1 newest-wins streaming

Paste several rapid commands in sequence:

```text
emotion 0.35 0.55
emotion -0.30 0.80
emotion 0.15 0.85
emotion 0.25 0.40
emotion 0.00 0.45
```

Expected:
- The final visible target dominates.
- Earlier targets do not backlog and play out one by one.
- Device remains responsive.

Tasks covered:
- T019

### D. US1 invalid input rejection

In the CLI:

```text
emotion 0.9 0.5
```

Expected:
- Eyes stay unchanged.
- No device crash or freeze.
- CLI may not print an error because streaming invalid frames are dropped silently by design.

Tasks covered:
- T019
- T047 (partial)

### E. US4 full expressive control surface

Run:

```text
preset happy 1.0
preset anxious 1.0
gaze -45 0
gaze 45 15
blink
wink left
wink right
idle off
idle on
```

Expected:
- Each preset is visibly distinct.
- Gaze moves to the requested direction.
- Blink closes both eyes.
- Wink closes only the requested eye.
- `idle off` stops idle behavior; `idle on` restores it.
- ACK-backed commands report `OK` in the CLI.

Tasks covered:
- T036
- T047 (partial)

### F. US5 status accuracy

Run:

```text
status
```

Expected:
- Returned valence/arousal approximately match the visible expression.
- Returned gaze matches the visible eye position.
- `connection_healthy=True` while the controlling CLI is active.
- `idle_enabled` matches the last `idle on/off` command.
- `uptime_ms` is increasing.

Tasks covered:
- T041
- T047 (partial)

### G. US2 controller crash / ungraceful disconnect

1. With one CLI controlling the device, kill it abruptly.
2. Start a new CLI within 10 seconds:

```bash
cd controller
python3 cli.py --host <device-ip>
```

Expected:
- The old session expires within about 5 seconds.
- The new controller can acquire the lock afterward.
- Eyes never freeze while waiting.
- If no controller is active, idle behavior resumes automatically.

Tasks covered:
- T026
- T047 (partial)

### H. US2 network interruption recovery

1. With a CLI connected, disable the controller machine Wi-Fi for 10 to 30 seconds.
2. Re-enable Wi-Fi.
3. Reconnect if needed using the same CLI command.

Expected:
- The device keeps animating during the outage.
- After timeout, the device falls back to idle behavior.
- Control resumes automatically or can be re-acquired without rebooting the board.
- No freeze, crash, or OLED corruption.

Tasks covered:
- T026
- T047 (partial)

### I. Quickstart end-to-end pass

Confirm the full documented flow works without deviation:
- Flash sketch
- Read IP from OLED
- Connect with `python3 cli.py --host <device-ip>`
- Run `emotion`, `preset`, `gaze`, `blink`, `wink`, `idle`, `status`
- Exit with `quit`

Tasks covered:
- T047

## Pass / Fail Record

Mark each item after running it on the board.

- [ ] A. Connect-by-address and `IN_USE` lock rejection
- [ ] B. Low-latency emotion reaction
- [ ] C. Newest-wins rapid streaming
- [ ] D. Invalid streaming input leaves state unchanged
- [ ] E. Preset / gaze / blink / wink / idle surface
- [ ] F. Status matches visible state
- [ ] G. Controller crash recovery
- [ ] H. Network interruption recovery
- [ ] I. Full quickstart flow

## Task Closure Map

Use this mapping when updating [specs/002-esp32-network-service/tasks.md](specs/002-esp32-network-service/tasks.md):

- If B and C pass: mark `T019` complete.
- If G and H pass: mark `T026` complete.
- If A passes: mark `T030` complete.
- If E passes: mark `T036` complete.
- If F passes: mark `T041` complete.
- If I passes: mark `T047` complete.

## Notes to Capture If Anything Fails

Record these before changing code:
- Exact command sent
- What the OLED showed
- Serial log lines around the failure
- Whether Wi-Fi was up/down
- Whether the issue recovered automatically
- Whether a reboot was required
