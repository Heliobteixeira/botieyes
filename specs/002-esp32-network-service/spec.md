# Feature Specification: ESP32 Network Control Service

**Feature Branch**: `002-esp32-network-service`  
**Created**: 2026-05-31  
**Status**: Draft  
**Input**: User description: "Build a network interface/service (or other better solution) for the current library that allows to control it from another machine on same network. It should have very low latency to allow fast emotions update and provide a reliable and resilient connection even in case of errors."

## Clarifications

### Session 2026-05-31

- Q: How should out-of-range command parameters be handled? → A: Reject entirely; leave current eye state unchanged and return an error (no clamping).
- Q: What timeout governs controller-unreachable fallback-to-idle and single-controller lock release? → A: 5 seconds of no contact (missed heartbeats/activity) for both.
- Q: How should the delivery guarantee balance latency vs. reliability? → A: Streaming/best-effort for continuous emotion & gaze updates (newest wins, dropped intermediates are acceptable); connection health and command confirmations are provided via lightweight acknowledgements/heartbeats.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Drive emotions in real time from a controller machine (Priority: P1)

An operator running a controller application on a laptop (or another device) on the same local network sends a stream of emotion commands to the ESP32-powered eyes. As the operator changes the desired emotion, the eyes on the OLED react almost immediately, allowing expressive, responsive behavior driven by external logic (e.g. an AI agent, a game, or a puppeteering UI).

**Why this priority**: This is the core value of the feature. Without low-latency remote emotion control, the service delivers nothing. A single working command path (set emotion → eyes react) is a usable MVP.

**Independent Test**: Connect a controller on the same network, send a sequence of emotion commands, and visually confirm the eyes update on the device within the latency budget. Delivers value as a standalone capability.

**Acceptance Scenarios**:

1. **Given** the device is powered on and joined to the local network, **When** the controller sends a "set emotion" command with valid valence/arousal values, **Then** the eyes begin transitioning to that emotion and the controller receives confirmation that the command was accepted.
2. **Given** an active control session, **When** the controller sends emotion updates in rapid succession (e.g. several per second), **Then** the eyes reflect the most recent command without visibly stalling or freezing, and older superseded commands do not cause the eyes to "replay" stale states.
3. **Given** a valid connection, **When** the controller sends a command with out-of-range values, **Then** the device rejects the command with an error indication and continues operating with its previous state unchanged.

---

### User Story 2 - Resilient connection that survives interruptions (Priority: P1)

The controller and the device maintain a session over an unreliable local network. When Wi-Fi briefly drops, a packet is lost, or the controller restarts, the connection recovers automatically without requiring a device power cycle, and the eyes never get stuck in a broken or frozen state.

**Why this priority**: The user explicitly requires reliability and resilience "even in case of errors." A low-latency channel that breaks permanently on the first hiccup is not usable in a real deployment, so this is co-critical with US1.

**Independent Test**: Establish a session, forcibly interrupt the network (disable Wi-Fi on the controller, drop packets, or kill the controller process), then restore it. Confirm the device returns to a controllable state automatically and the eyes remain in a safe, coherent state throughout.

**Acceptance Scenarios**:

1. **Given** an active session, **When** the network connection is lost for a short period, **Then** the device detects the loss, keeps the eyes in a safe defined state, and accepts a new/resumed session once connectivity returns — without manual intervention.
2. **Given** the controller process crashes or disconnects ungracefully, **When** a new controller connects, **Then** it can take over control of the device without a reboot.
3. **Given** the device loses Wi-Fi entirely, **When** connectivity is unavailable beyond a defined timeout, **Then** the device falls back to autonomous/idle behavior so it never appears frozen or dead.

---

### User Story 3 - Discover and connect to the device easily (Priority: P2)

An operator who does not know the device's network address can discover it on the local network and connect without manually configuring IP addresses each time.

**Why this priority**: Improves usability significantly but is not required for a functional MVP — a known/static address can be used initially. Hence P2.

**Independent Test**: From a fresh controller with no prior knowledge of the device address, locate the device on the network and establish a session.

**Acceptance Scenarios**:

1. **Given** the device is online, **When** an operator looks for it on the local network, **Then** the device is discoverable by a stable, human-recognizable name.
2. **Given** the device's network address changes (e.g. new DHCP lease), **When** the operator reconnects, **Then** discovery still resolves the device without manual reconfiguration.

---

### User Story 4 - Full expressive control surface over the network (Priority: P2)

Beyond raw emotion values, the controller can trigger the library's full expressive vocabulary remotely: named emotion presets, coupled eye gaze position, blink/wink animations, and enabling/disabling idle behavior.

**Why this priority**: Extends the MVP to expose the complete library capability set, maximizing expressiveness. Valuable but layered on top of the core emotion path.

**Independent Test**: From the controller, exercise each command category (preset emotion, gaze position, blink, wink, idle toggle) and visually confirm the corresponding behavior on the device.

**Acceptance Scenarios**:

1. **Given** a connected session, **When** the controller requests a named emotion preset (e.g. "happy") with an intensity, **Then** the eyes express that preset.
2. **Given** a connected session, **When** the controller sets a gaze position within valid ranges, **Then** both eyes move to that position.
3. **Given** a connected session, **When** the controller triggers a blink or a wink, **Then** the corresponding animation plays.
4. **Given** a connected session, **When** the controller enables or disables idle behavior, **Then** the device honors that setting.

---

### User Story 5 - Observe device status remotely (Priority: P3)

The controller can query the device's current state (current emotion, gaze, connection health, uptime) to keep its own UI in sync and to confirm commands took effect.

**Why this priority**: Useful for building robust controller UIs and diagnostics, but the device is controllable without it. Hence P3.

**Independent Test**: With an active session, request the device state and confirm the returned values match what is shown on the OLED.

**Acceptance Scenarios**:

1. **Given** a connected session, **When** the controller requests current status, **Then** the device reports its current emotion, gaze position, and connection health.

---

### Edge Cases

- **Conflicting controllers**: What happens when a second controller tries to command the device while one already holds control? The device must reject the second controller with a clear "in use" indication rather than oscillating between two command sources.
- **Command flooding**: When commands arrive faster than the device can render, the device must shed/coalesce intermediate commands and always honor the newest target instead of building an unbounded backlog.
- **Malformed or oversized messages**: The device must reject malformed, truncated, or oversized messages without crashing or corrupting state.
- **Out-of-range parameters**: Values outside library-supported ranges must be rejected (no clamping) with a clear error indication, leaving the current eye state unchanged.
- **Network unavailable at boot**: If the device cannot join the network at startup, it must still run autonomously (idle behavior) and keep retrying to join in the background.
- **Long idle session**: If a controller connects but sends no commands for an extended period, the device must keep the connection healthy (or cleanly time it out) without freezing the eyes.
- **Time/order of delivery**: If commands arrive out of order, the device must not apply a stale emotion after a newer one.
- **Resource exhaustion**: The device must not run out of memory or stall its rendering loop under sustained network load.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The device MUST expose a network-accessible control service reachable from other machines on the same local network.
- **FR-002**: The service MUST accept commands to set a target emotion using the library's valence/arousal model and apply them to the eyes.
- **FR-003**: The service MUST accept commands for named emotion presets with an optional intensity (matching the library's preset set).
- **FR-004**: The service MUST accept commands to set coupled eye gaze position within the library's supported horizontal and vertical ranges.
- **FR-005**: The service MUST accept commands to trigger blink and wink (left/right) animations.
- **FR-006**: The service MUST accept commands to enable or disable idle behavior.
- **FR-007**: The service MUST validate every incoming command and reject invalid or out-of-range commands (no clamping), leaving the current eye state unchanged on rejection. For discrete commands (session control, presets, blink, wink, idle toggle, status), rejection MUST be reported to the controller with a clear error/reason indication. For best-effort streaming updates (continuous emotion/gaze), a rejected frame is dropped without applying it; the controller observes the unchanged state via a subsequent status query (see FR-013, FR-019) rather than a per-frame error.
- **FR-008**: The service MUST always act on the most recent emotion/position target when commands arrive faster than they can be rendered, coalescing or discarding superseded intermediate commands rather than queuing them unboundedly.
- **FR-009**: The device MUST continue rendering eye animations smoothly while servicing network traffic (network handling MUST NOT stall the rendering loop).
- **FR-010**: The service MUST automatically recover from transient network interruptions and accept a new or resumed control session without requiring a device reboot.
- **FR-011**: The device MUST keep the eyes in a safe, coherent state during connection loss and MUST fall back to autonomous/idle behavior if the controller is unreachable for more than 5 seconds (no heartbeats/activity received).
- **FR-012**: The service MUST tolerate ungraceful controller disconnects and allow a subsequent controller to take over control.
- **FR-013**: The service MUST provide a way to query current device status (current emotion, gaze, connection health, and uptime).
- **FR-014**: The service MUST handle malformed, truncated, or oversized messages without crashing or corrupting device state.
- **FR-015**: For v1, the device MUST be reachable at a configured/known network address; zero-config name-based discovery (e.g. mDNS/Bonjour-style) is out of scope for v1 and deferred to a future enhancement. The device MUST make its current network address observable (e.g. shown on the OLED or otherwise surfaced) so the operator can connect.
- **FR-016**: The device MUST operate autonomously (idle behavior) when no controller is connected and when the network is unavailable.
- **FR-017**: The service MUST allow only a single active controlling session at a time (single-controller lock). While one controller holds control, command attempts from other controllers MUST be rejected with a clear "in use" indication. Control MUST be releasable explicitly and MUST be reclaimable by a new controller after the current controller disconnects or after 5 seconds of no contact (no heartbeats/activity).
- **FR-018**: For v1, the control channel operates on a trusted local network only and does NOT require authentication or pairing. The service MUST NOT be designed to be exposed to untrusted/public networks; authentication is a deferred future enhancement.
- **FR-019**: The device MUST let the controller confirm whether a command took effect. For discrete commands, this MUST be an explicit acceptance/rejection acknowledgement with a reason on rejection. For best-effort streaming updates (continuous emotion/gaze), confirmation is provided via the queryable device status (FR-013) rather than a per-command acknowledgement.
- **FR-020**: The network service MUST coexist with the existing library API without changing the library's existing local-control behavior for users who do not use networking.
- **FR-021**: Continuous emotion and gaze updates MAY be delivered with best-effort (streaming) semantics where superseded intermediate updates can be dropped (consistent with newest-wins coalescing in FR-008); the device MUST NOT rely on every such update arriving. Connection health and command confirmation MUST be provided via lightweight acknowledgement/heartbeat signals rather than guaranteed in-order delivery of every update.

### Key Entities *(include if feature involves data)*

- **Control Session**: A logical connection between a controller and the device. Attributes: identity of the controlling client, liveness/health, last-activity time, and current ownership of control.
- **Command**: A single instruction from controller to device. Types: set emotion (valence/arousal), preset emotion (name + intensity), set gaze position, blink, wink, toggle idle, query status. Attributes: type, parameters, and an ordering/recency indicator so the newest target wins.
- **Device State**: The device's current observable condition. Attributes: current emotion, current gaze position, idle on/off, connection health, uptime.
- **Acknowledgement / Error Response**: The device's reply to a command. Attributes: accepted vs. rejected, and a reason on rejection.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: From command sent on the controller to the eyes beginning to react on the device, end-to-end latency on a healthy local network is under 100 ms for 95% of commands.
- **SC-002**: The device sustains at least 20 emotion updates per second from a controller without dropped rendering frames or visible stalling.
- **SC-003**: After a network interruption of up to 30 seconds, the device returns to a controllable state automatically within 10 seconds of connectivity being restored, with no manual reboot.
- **SC-004**: During any connection loss or controller crash, the eyes never enter a frozen, blank, or corrupted state — they remain animated (idle/autonomous) 100% of the time.
- **SC-005**: Using the device's surfaced network address, a controller can establish a control session on first attempt in at least 95% of attempts on a typical home/office network.
- **SC-006**: Under a sustained 5-minute flood of commands at 10x the normal rate, the device applies the newest target, never crashes, and memory/rendering remain stable.
- **SC-007**: 100% of malformed or out-of-range commands are rejected without affecting the current eye state or device stability.

## Assumptions

- The controller machine and the device are on the same trusted local network (LAN/Wi-Fi); internet/cloud connectivity and remote-over-internet control are out of scope for v1.
- The device is reached at a configured/known network address for v1; zero-config discovery is deferred. The device surfaces its current address (e.g. on the OLED) so operators can connect.
- The device hardware is the TTGO LoRa32 (ESP32 + integrated SSD1306 128x64 OLED) used by the existing library; Wi-Fi connectivity is available on this hardware.
- The network service is an additive layer on top of the existing BotiEyes library and reuses its emotion/position/animation API rather than reimplementing rendering.
- "Low latency" is interpreted as sub-100 ms end-to-end on a healthy LAN, consistent with responsive real-time puppeteering.
- Continuous emotion/gaze updates favor freshness over guaranteed delivery: dropping a superseded update is acceptable because the next update supersedes it; health and confirmation come from lightweight acks/heartbeats.
- A single primary controller at a time is the common case; the service enforces a single active controller via a lock, with takeover after disconnect/timeout.
- Wi-Fi credentials for the device are provisioned out of band (e.g. configured at build/flash time or via an existing setup mechanism); credential provisioning UX is out of scope for this spec.
- Security posture for v1 assumes a trusted LAN with no authentication; strong authentication is a deferred future enhancement.
- The controller-side application is out of scope to build here beyond what is needed to validate the device service; this spec focuses on the device-side network service and its observable contract.
