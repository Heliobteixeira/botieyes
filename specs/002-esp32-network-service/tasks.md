# Tasks: ESP32 Network Control Service

**Input**: Design documents from `/specs/002-esp32-network-service/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/network-protocol.md

**Tests**: Included. The plan and research explicitly require host-native unit tests for the codec and
session state machine, plus Python client tests.

**Organization**: Tasks are grouped by user story (US1–US5) to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story the task belongs to (US1–US5)
- Exact file paths are included in each task

## Path Conventions

- Device service: `BotiEyes/src/net/`
- Device example: `BotiEyes/examples/NetworkControl/`
- Reference controller: `controller/`
- Host-native tests: `tests/net/`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Create the directory layout and shared protocol constants used by every story.

- [X] T001 Create network layer directory structure: `BotiEyes/src/net/`, `BotiEyes/examples/NetworkControl/`, `controller/`, and `tests/net/` per [plan.md](plan.md) project structure
- [X] T002 [P] Create shared protocol constants header `BotiEyes/src/net/NetProtocol.h` (protocol version `1`, UDP port `4210`, opcodes 0x01–0x20, response codes 0x80/0x81, reason codes 0–5, max frame size 32, fixed-point scales) per [contracts/network-protocol.md](contracts/network-protocol.md)
- [X] T003 [P] Create Python controller package skeleton: `controller/botieyes_client.py`, `controller/cli.py`, `controller/requirements.txt` (stdlib-only), and `controller/__init__.py`
- [X] T004 [P] Create host-native test harness scaffold in `tests/net/` (PlatformIO native env or host compiler) with a README documenting how to run codec/session tests off-target

**Checkpoint**: Empty but compilable structure with shared constants in place.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The platform-agnostic codec and session/lock state machine that ALL user stories depend on.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T005 [P] Define command/response data structures in `BotiEyes/src/net/CommandCodec.h` (`Command`, `Acknowledgement`, `StatusReport`, opcode/reason enums) per [data-model.md](data-model.md)
- [X] T006 Implement `CommandCodec::decode()` in `BotiEyes/src/net/CommandCodec.cpp` — parse 6-byte header, validate version/length/opcode, decode fixed-point payloads, enforce ranges (reject, no clamping) per [contracts/network-protocol.md](contracts/network-protocol.md) (depends on T005)
- [X] T007 Implement `CommandCodec::encode()` for `ACK` (0x80) and `STATUS` (0x81) frames in `BotiEyes/src/net/CommandCodec.cpp` (depends on T005)
- [X] T008 [P] Define `SessionManager` interface in `BotiEyes/src/net/SessionManager.h` (`ControlSession`, `PendingTargets`, lock/heartbeat/timeout API) per [data-model.md](data-model.md)
- [X] T009 Implement `SessionManager` core in `BotiEyes/src/net/SessionManager.cpp` — single-controller lock by IP:port, acquire/release, 5 s no-contact timeout, heartbeat refresh, takeover (depends on T008)
- [X] T010 Implement newest-wins coalescing in `SessionManager` (`PendingTargets`: keep highest-`seq` emotion/gaze target, drop superseded, queue one-shot actions) in `BotiEyes/src/net/SessionManager.cpp` (depends on T009)

**Checkpoint**: Pure, host-testable codec + session logic compile off-target. User stories can now begin.

---

## Phase 3: User Story 1 - Drive emotions in real time (Priority: P1) 🎯 MVP

**Goal**: A controller on the LAN sets emotions and the eyes react in <100 ms via best-effort streaming.

**Independent Test**: Send valid/rapid/out-of-range emotion commands from the controller; eyes react to the
newest valid target within the latency budget; out-of-range rejected with state unchanged.

### Tests for User Story 1

- [X] T011 [P] [US1] Codec round-trip + validation tests in `tests/net/test_command_codec.*` — `SET_EMOTION` encode/decode, truncated/oversized frames, bad version, valence/arousal out-of-range rejection (FR-007, FR-014, SC-007)
- [X] T012 [P] [US1] Newest-wins selection test in `tests/net/test_session_manager.*` — higher-`seq` emotion supersedes, stale `seq` ignored (FR-008)

### Implementation for User Story 1

- [X] T013 [US1] Create `BotiEyesServer` transport adapter in `BotiEyes/src/net/BotiEyesServer.h` (ESP32 `#ifdef`-gated; holds `BotiEyes&`, `CommandCodec`, `SessionManager`; `begin(port)`, non-blocking `poll()`, `applyPending()`)
- [X] T014 [US1] Implement non-blocking UDP receive + decode loop in `BotiEyes/src/net/BotiEyesServer.cpp` using `WiFiUDP` (read available datagrams, decode, route streaming targets into `PendingTargets`) (depends on T013, T006, T010)
- [X] T015 [US1] Map `SET_EMOTION` pending target to `BotiEyes::setEmotion(valence, arousal, duration_ms)` in `applyPending()` once per frame in `BotiEyes/src/net/BotiEyesServer.cpp` (depends on T014)
- [X] T016 [US1] Create example sketch `BotiEyes/examples/NetworkControl/NetworkControl.ino` — Wi-Fi join, init BotiEyes + SSD1306 (SDA=4/SCL=15/RST=16), start server on port 4210, render loop calling `server.poll()` + `eyes.update()` at ≥20 FPS
- [X] T017 [P] [US1] Implement `controller/botieyes_client.py` `set_emotion()` + low-level UDP send framing (fixed-point encode) per protocol
- [X] T018 [P] [US1] Add `emotion` command to `controller/cli.py` interactive loop
- [ ] T019 [US1] On-hardware visual validation on TTGO LoRa32 — confirm emotion updates render in <100 ms and rapid streaming shows newest target without stalling (SC-001, SC-002); per repo commit policy before any commit **(requires physical board — flash NetworkControl.ino)**

**Checkpoint**: MVP — remote emotion control works end to end and is independently demoable.

---

## Phase 4: User Story 2 - Resilient connection that survives interruptions (Priority: P1)

**Goal**: Session self-heals after Wi-Fi drops/packet loss/controller crash; eyes never freeze; idle fallback.

**Independent Test**: Interrupt the network/kill the controller, then restore; device returns to controllable
state automatically within 10 s and eyes stay animated throughout.

### Tests for User Story 2

- [X] T020 [P] [US2] Session timeout + takeover tests in `tests/net/test_session_manager.*` — 5 s no-contact releases lock, lock reclaimable by new controller, ungraceful disconnect handled (FR-011, FR-012, FR-017)
- [X] T021 [P] [US2] Heartbeat liveness test in `tests/net/test_session_manager.*` — any frame refreshes contact; absence triggers timeout (FR-011)

### Implementation for User Story 2

- [X] T022 [US2] Implement heartbeat handling + liveness tracking in `BotiEyes/src/net/BotiEyesServer.cpp` (`HEARTBEAT` opcode refreshes `lastContactMs`) (depends on T014)
- [X] T023 [US2] Implement idle fallback in `BotiEyes/src/net/BotiEyesServer.cpp` — on lock timeout / no controller, call `enableIdleBehavior(true)` and keep autonomous `update()` running (FR-011, FR-016) (depends on T009, T015)
- [X] T024 [US2] Ensure non-blocking guarantee: `poll()` never blocks the render loop even with no/lost network; verify Wi-Fi reconnect retry runs in background in `NetworkControl.ino` (FR-009, FR-010)
- [X] T025 [P] [US2] Implement automatic ~1 s heartbeat thread/timer and re-acquire-on-timeout in `controller/botieyes_client.py`
- [ ] T026 [US2] On-hardware resilience validation on TTGO LoRa32 — drop controller Wi-Fi 10–30 s, kill/restart controller; confirm auto-recovery ≤10 s and eyes never freeze (SC-003, SC-004); per repo commit policy **(requires physical board)**

**Checkpoint**: US1 + US2 both work — low-latency control that is also resilient.

---

## Phase 5: User Story 3 - Discover and connect easily (Priority: P2)

**Goal**: Operator connects using the device's address shown on the OLED; first-attempt session success.

**Independent Test**: From a fresh controller, read the IP off the OLED and establish a session first try.

### Implementation for User Story 3

- [X] T027 [US3] Render the device IP address on the SSD1306 OLED after Wi-Fi join in `BotiEyes/examples/NetworkControl/NetworkControl.ino` (FR-015)
- [X] T028 [US3] Implement `ACQUIRE`/`RELEASE` handling + "in use" NAK in `BotiEyes/src/net/BotiEyesServer.cpp` (single active session) (depends on T009, T014)
- [X] T029 [P] [US3] Implement session acquire/release in `controller/botieyes_client.py` (context-manager `__enter__`/`__exit__`) and `--host` argument in `controller/cli.py`
- [ ] T030 [US3] On-hardware validation — connect using the OLED-shown IP on first attempt; confirm second controller is rejected with `IN_USE` (SC-005, FR-017); per repo commit policy **(requires physical board)**

**Checkpoint**: US1–US3 functional — discoverable-by-address connection with single-controller arbitration.

---

## Phase 6: User Story 4 - Full expressive control surface (Priority: P2)

**Goal**: Remote access to presets, gaze, blink, wink, and idle toggle.

**Independent Test**: Exercise each command category from the controller and confirm the behavior on device.

### Tests for User Story 4

- [X] T031 [P] [US4] Codec tests for `SET_PRESET`, `SET_GAZE`, `BLINK`, `WINK`, `SET_IDLE` in `tests/net/test_command_codec.*` — encode/decode + range/enum validation (presetId, eye, gaze bounds) (FR-003–FR-006)

### Implementation for User Story 4

- [X] T032 [US4] Map `SET_GAZE` pending target to `BotiEyes::setEyePosition(h, v, duration_ms)` in `applyPending()` in `BotiEyes/src/net/BotiEyesServer.cpp` (depends on T015)
- [X] T033 [US4] Map `SET_PRESET` to preset helpers (happy/sad/angry/calm/excited/tired/surprised/anxious/content/curious/thinking/confused/neutral with intensity) in `BotiEyes/src/net/BotiEyesServer.cpp` (depends on T015)
- [X] T034 [US4] Handle one-shot actions `BLINK`/`WINK`/`SET_IDLE` from `PendingTargets` action set, mapping to `blink()`/`wink(eye)`/`enableIdleBehavior()` with `ACK` replies in `BotiEyes/src/net/BotiEyesServer.cpp` (depends on T010, T015)
- [X] T035 [P] [US4] Add `preset`, `gaze`, `blink`, `wink`, `idle` methods to `controller/botieyes_client.py` and corresponding CLI commands in `controller/cli.py`
- [ ] T036 [US4] On-hardware validation — exercise preset/gaze/blink/wink/idle and confirm each behavior renders (FR-003–FR-006); per repo commit policy **(requires physical board)**

**Checkpoint**: US1–US4 functional — complete expressive vocabulary over the network.

---

## Phase 7: User Story 5 - Observe device status remotely (Priority: P3)

**Goal**: Controller queries current emotion/gaze/health/uptime to reconcile its UI.

**Independent Test**: With an active session, request status and confirm values match the OLED.

### Tests for User Story 5

- [X] T037 [P] [US5] `STATUS`/`ACK` encode tests in `tests/net/test_command_codec.*` — `QUERY_STATUS` response fields and `ACK` accept/reject reason codes (FR-013, FR-019)

### Implementation for User Story 5

- [X] T038 [US5] Handle `QUERY_STATUS` in `BotiEyes/src/net/BotiEyesServer.cpp` — build `StatusReport` from `getCurrentEmotion()`, `getEyePosition()`, idle flag, connection health, `millis()` uptime; encode 0x81 reply (depends on T007, T014)
- [X] T039 [US5] Emit `ACK`/`NAK` for all discrete commands with correct reason codes in `BotiEyes/src/net/BotiEyesServer.cpp` (FR-019) (depends on T007, T028, T034)
- [X] T040 [P] [US5] Add `status()` method to `controller/botieyes_client.py` and `status` CLI command (decode `STATUS`/`ACK`)
- [ ] T041 [US5] On-hardware validation — query status and confirm returned values match the OLED (FR-013); per repo commit policy **(requires physical board)**

**Checkpoint**: All five user stories independently functional.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Hardening, performance verification, docs, and full quickstart validation.

- [X] T042 [P] Add Python client unit tests in `controller/test_client.py` (framing round-trips, fixed-point encode) and document `python -m pytest`
- [X] T043 [P] Flood/stability test — sustained 5-minute 10x command flood; confirm newest-wins, no crash, stable memory/rendering (SC-006) documented in `tests/net/` (`flood_test.py` + README; on-hardware run requires physical board)
- [X] T044 [P] Update `BotiEyes/library.properties` and `BotiEyes/keywords.txt` for the new `net/` classes and example
- [X] T045 [P] Add a "Network Control" section to `BotiEyes/README.md` linking to [quickstart.md](quickstart.md) and [contracts/network-protocol.md](contracts/network-protocol.md)
- [X] T046 Verify additive guarantee — existing examples (BasicEmotion, etc.) still build unchanged and non-ESP32 builds exclude `BotiEyesServer` via `#ifdef` (FR-020)
- [ ] T047 Execute full [quickstart.md](quickstart.md) validation end to end on TTGO LoRa32; per repo commit policy **(requires physical board)**
- [X] T048 [P] Document the trusted-LAN-only / no-auth constraint (FR-018) in `BotiEyes/examples/NetworkControl/NetworkControl.ino` header comment and the README "Network Control" section — warn against exposing UDP port 4210 to untrusted/public networks

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately.
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories.
- **User Stories (Phase 3–7)**: All depend on Foundational. US1 is the MVP. US2 builds on the US1 server but
  is independently testable. US3/US4/US5 layer additional handlers onto the same server.
- **Polish (Phase 8)**: Depends on all targeted user stories.

### User Story Dependencies

- **US1 (P1)**: Foundational only — delivers the MVP streaming-emotion path + server skeleton.
- **US2 (P1)**: Reuses the US1 server (`poll`/`applyPending`) to add heartbeat/timeout/idle-fallback. Testable independently via fault injection.
- **US3 (P2)**: Adds OLED address + acquire/release on the existing server.
- **US4 (P2)**: Adds preset/gaze/animation mappings via the same `applyPending()` extension point.
- **US5 (P3)**: Adds status/ack responses.

### Within Each User Story

- Tests (codec/session) are written to FAIL before implementation.
- Codec/session (Foundational) before server wiring; server wiring before mapping to library calls.
- Pure-logic tasks before hardware validation.
- Hardware validation is the last task of each story (and gates commits per repo policy).

### Parallel Opportunities

- Setup: T002, T003, T004 in parallel.
- Foundational: T005/T008 in parallel; codec (T006/T007) and session (T009/T010) are largely parallel tracks.
- Within a story, `[P]` tasks touch different files (e.g., Python client vs. C++ server vs. tests).
- Test tasks for a story run in parallel before its implementation.

---

## Parallel Example: User Story 1

```bash
# Tests first (parallel, different files):
Task: "Codec tests in tests/net/test_command_codec.* (T011)"
Task: "Newest-wins test in tests/net/test_session_manager.* (T012)"

# Then parallel client/CLI work alongside C++ server wiring:
Task: "set_emotion() in controller/botieyes_client.py (T017)"
Task: "emotion command in controller/cli.py (T018)"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1: Setup.
2. Phase 2: Foundational (codec + session) — CRITICAL, blocks everything.
3. Phase 3: User Story 1 — remote streaming emotion control.
4. **STOP and VALIDATE**: flash to TTGO LoRa32 and confirm <100 ms reactions (SC-001/SC-002).

### Incremental Delivery

- Add US2 (resilience) next since it is co-P1 and required for real deployments.
- Then US3 (connect-by-address + lock), US4 (full control surface), US5 (status) in priority order.
- Each story ends with on-hardware validation and is independently demoable.
