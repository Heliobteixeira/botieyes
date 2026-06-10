# Implementation Plan: ESP32 Network Control Service

**Branch**: `002-esp32-network-service` | **Date**: 2026-06-01 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-esp32-network-service/spec.md`

## Summary

Add a network control service to the ESP32 build of BotiEyes so a controller on the same LAN can drive the eyes in real time with sub-100 ms latency and a resilient, self-healing connection. Technical approach: a thin **UDP-based application protocol** providing best-effort, newest-wins streaming for continuous emotion/gaze updates plus lightweight acknowledgements and ~1 s heartbeats for connection health, command confirmation, and a single-controller lock (5 s no-contact timeout). The service is an **additive, ESP32-gated layer** on top of the existing `BotiEyes` library: it parses datagrams and calls the existing public API (`setEmotion`, `setEyePosition`, presets, `blink`, `wink`, `enableIdleBehavior`) without changing core rendering or the local-only usage path. Network handling is fully non-blocking so the render loop keeps running at 20+ FPS, and the device falls back to autonomous idle behavior whenever no controller is present.

## Technical Context

**Language/Version**: C++ (Arduino/PlatformIO, ESP32 core) for the device service; Python 3.8+ for a reference controller client used in validation
**Primary Dependencies**: ESP32 Wi-Fi stack (`WiFi`, `WiFiUDP` / `AsyncUDP`) on device; existing BotiEyes library + Adafruit GFX/SSD1306; Python `socket` (stdlib) for the controller client (no third-party runtime deps)
**Storage**: N/A (session and device state held in RAM only; no persistence)
**Testing**: Host-native unit tests (compiled off-target) for the platform-agnostic command codec and session/lock state machine; Python client unit tests; on-hardware visual validation on TTGO LoRa32 per repo commit policy
**Target Platform**: TTGO LoRa32 (ESP32 + integrated SSD1306 128x64); controller runs on macOS/Linux/Windows on the same LAN
**Project Type**: Embedded library extension (ESP32 networking layer) + small Python reference client
**Performance Goals**: <100 ms end-to-end command-to-react latency (p95) on a healthy LAN; sustain ≥20 emotion updates/sec; maintain ≥20 FPS rendering while servicing network traffic
**Constraints**: Non-blocking network I/O only (must not stall the render loop); newest-wins coalescing with bounded memory; 5 s no-contact timeout for idle fallback and lock release; ~1 s heartbeat interval; trusted-LAN only (no auth in v1); additive — must not regress local-only library behavior
**Scale/Scope**: Single device; single active controller (lock) with takeover; ~8 command types; one example sketch + one reference Python client

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Initial Check (Pre-Research)

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Pragmatic Simplicity | ✅ **PASS** | Single thin UDP protocol; no broker, no TCP reconnection machinery, no auth in v1; reuses existing library API instead of new rendering |
| II. Maintainable Code | ✅ **PASS** | Layer split into codec (pure, testable), session/lock state machine (pure), and a thin ESP32 transport adapter; clear `.h`/`.cpp` separation following existing conventions |
| III. Performance-First | ✅ **PASS** | Explicit latency/throughput/FPS budgets; non-blocking I/O; best-effort streaming avoids head-of-line latency |
| IV. Hardware Abstraction | ✅ **PASS** | Codec and session logic are platform-agnostic and host-testable; only the transport adapter is ESP32-specific (`#ifdef`-gated) |
| V. Emotion-Driven | ✅ **PASS** | Protocol carries valence/arousal + presets, preserving the continuous emotional model end to end |
| VI. Cross-Platform Emulation | ✅ **PASS** | Same protocol can drive the existing Python emulator as a software endpoint; reference client is cross-platform |
| VII. Extensible Architecture | ✅ **PASS** | Versioned protocol header and typed command set allow new commands/transports later without core changes |
| VIII. Continuous Learning | ✅ **PASS** | 3 clarifications captured in spec; research records transport/latency decisions and alternatives |

**Gate Result**: ✅ **PASS** — no violations; no entries required in Complexity Tracking.

### Post-Design Check (After Phase 1)

| Principle | Status | Design Validation |
|-----------|--------|-------------------|
| I. Pragmatic Simplicity | ✅ **PASS** | Final design = one UDP socket, fixed-size binary frames, a small state machine; no dynamic allocation in the hot path |
| II. Maintainable Code | ✅ **PASS** | Three cohesive units (`CommandCodec`, `SessionManager`, `BotiEyesServer`); each independently testable; transport isolated behind an interface |
| III. Performance-First | ✅ **PASS** | Fixed ≤32-byte frames, O(1) parse, single most-recent target buffered per channel (newest wins), heartbeat budget negligible |
| IV. Hardware Abstraction | ✅ **PASS** | `CommandCodec` and `SessionManager` compile and unit-test on host without ESP32; only `BotiEyesServer` touches `WiFiUDP` |
| V. Emotion-Driven | ✅ **PASS** | Emotion/gaze fields map 1:1 to existing `setEmotion`/`setEyePosition`; presets map to existing helpers |
| VI. Cross-Platform Emulation | ✅ **PASS** | Reference client validates protocol independently of hardware; protocol documented for emulator reuse |
| VII. Extensible Architecture | ✅ **PASS** | `protocol_version` + reserved type space allow additive commands; transport adapter swappable |
| VIII. Continuous Learning | ✅ **PASS** | research.md captures rationale (UDP vs TCP/WebSocket/MQTT/ESP-NOW) and alternatives considered |

**Post-Design Gate Result**: ✅ **PASS** — design ready for task generation.

## Project Structure

### Documentation (this feature)

```text
specs/002-esp32-network-service/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   └── network-protocol.md   # Wire protocol + command contract
├── checklists/
│   └── requirements.md  # Spec quality checklist
└── tasks.md             # Phase 2 output (/speckit.tasks - NOT created here)
```

### Source Code (repository root)

```text
BotiEyes/
├── src/
│   ├── BotiEyes.h / .cpp           # Existing library (unchanged public behavior)
│   └── net/                        # NEW — ESP32 network control layer
│       ├── CommandCodec.h / .cpp   # Pure: encode/decode wire frames (host-testable)
│       ├── SessionManager.h / .cpp # Pure: lock, heartbeat, timeout, newest-wins state machine
│       ├── BotiEyesServer.h / .cpp # ESP32 transport adapter (WiFiUDP) — #ifdef ESP32
│       └── NetProtocol.h           # Shared constants: ports, opcodes, version, limits
└── examples/
    └── NetworkControl/
        └── NetworkControl.ino      # NEW — Wi-Fi join + server + render loop on TTGO LoRa32

controller/                          # NEW — reference Python controller client
├── botieyes_client.py              # UDP client library (stdlib socket)
├── cli.py                          # Minimal CLI to drive emotions/gaze/animations
└── requirements.txt                # (empty/stdlib-only; present for parity)

tests/
└── net/                            # NEW — host-native unit tests
    ├── test_command_codec.*        # Frame encode/decode round-trips, malformed input
    └── test_session_manager.*      # Lock acquire/takeover, heartbeat/timeout, newest-wins
```

**Structure Decision**: The network service is split into three cohesive units under `BotiEyes/src/net/`. `CommandCodec` and `SessionManager` are **platform-agnostic and host-testable** (no Arduino/Wi-Fi headers), satisfying Hardware Abstraction and enabling fast off-target unit tests. Only `BotiEyesServer` depends on the ESP32 Wi-Fi stack and is compiled behind an `#ifdef ESP32` guard, so the existing library continues to build and behave identically for non-networked and non-ESP32 users. A small `controller/` Python client provides a cross-platform way to exercise and validate the protocol without bespoke tooling.

## Complexity Tracking

> No constitution violations. No entries required.
