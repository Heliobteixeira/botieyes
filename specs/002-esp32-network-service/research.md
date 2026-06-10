# Phase 0 Research: ESP32 Network Control Service

**Feature**: 002-esp32-network-service | **Date**: 2026-06-01

This document resolves the technical unknowns behind the plan. All spec clarifications were already
resolved in [spec.md](spec.md) (Clarifications, Session 2026-05-31); this focuses on technology choices.

## Decision 1: Transport — UDP application protocol

**Decision**: Use **UDP** with a thin custom application-layer protocol as the primary control channel.
Continuous emotion/gaze updates are sent as best-effort datagrams with a monotonically increasing
sequence number (newest wins); discrete actions and session control use small request/ack exchanges;
a ~1 s heartbeat maintains liveness.

**Rationale**:
- **Latency**: UDP has no connection setup, no head-of-line blocking, and no retransmission stalls.
  A lost emotion frame is harmless because the next frame supersedes it (matches FR-008 newest-wins and
  the streaming clarification), directly serving SC-001 (<100 ms p95) and SC-002 (≥20 updates/s).
- **Resilience**: There is no TCP connection to "break". After a Wi-Fi drop the device simply resumes
  accepting datagrams; recovery is automatic with no reconnection handshake (FR-010, SC-003).
- **Simplicity**: One non-blocking socket, fixed-size frames, no broker — aligns with Pragmatic Simplicity.

**Alternatives considered**:
- **TCP / WebSocket**: Reliable, ordered delivery, but head-of-line blocking under packet loss adds
  latency exactly when the network is degraded, and reconnection adds recovery delay. Reliability is
  unnecessary for superseded streaming updates. Rejected for the streaming path.
- **MQTT**: Requires a broker on the LAN — extra infrastructure and a single point of failure; pub/sub
  and QoS add latency and complexity. Rejected (over-engineered for one device + one controller).
- **ESP-NOW**: Very low latency, but it is a Wi-Fi-direct/MAC-paired protocol that the controller PC
  cannot speak without special hardware/drivers; "another machine on the same network" implies standard
  IP. Rejected for v1.
- **HTTP/REST**: Per-request connection overhead and polling are poor fits for a high-rate stream. Rejected.

## Decision 2: Frame format — fixed-size little-endian binary

**Decision**: Fixed-layout binary frames (≤32 bytes) with a 1-byte version, 1-byte opcode, 4-byte
sequence number, a small typed payload, and length/range validation on decode. Defined in
[contracts/network-protocol.md](contracts/network-protocol.md).

**Rationale**: O(1) parsing, no dynamic allocation, trivial to validate sizes (defends against malformed/
oversized messages, FR-014/SC-007), and compact for low latency. A `protocol_version` byte enables
additive evolution (Extensible Architecture).

**Alternatives considered**:
- **JSON/text**: Human-readable but needs a parser, heap, and is larger/slower; risk on a constrained
  MCU hot path. Rejected for the wire format (the reference client may still log human-readable forms).
- **Protobuf/CBOR**: More capable schemas but add a dependency and code size for no benefit at this scope.
  Rejected.

## Decision 3: Concurrency model — single non-blocking poll in the render loop

**Decision**: Poll the UDP socket non-blocking once per `loop()` iteration, parse at most the available
datagrams into a single most-recent target per channel (emotion, gaze) plus a small action flag set, then
call `BotiEyes::update()`. No background tasks, no blocking reads.

**Rationale**: Guarantees the render loop is never stalled by network I/O (FR-009, SC-002). Buffering only
the newest target per channel gives bounded memory under flooding (SC-006) and inherently implements
newest-wins (FR-008). Avoids RTOS task/locking complexity (Pragmatic Simplicity).

**Alternatives considered**:
- **Dedicated FreeRTOS networking task + queue**: More throughput headroom but adds synchronization and
  the risk of an unbounded queue replaying stale states. Rejected for v1; the single-loop model meets the
  performance budget.
- **`AsyncUDP` callbacks**: Viable and non-blocking, but callback context constraints complicate state
  updates; a simple polled `WiFiUDP` is easier to reason about and test. Either works; plan targets polled
  `WiFiUDP` with `AsyncUDP` as a drop-in alternative behind the transport adapter.

## Decision 4: Session, lock, and liveness

**Decision**: A `SessionManager` state machine tracks the single active controller by source IP:port.
A controller acquires the lock with a HELLO/ACQUIRE; others are rejected with an "in use" ack (FR-017).
Liveness is tracked from any received frame plus explicit ~1 s heartbeats; after **5 s** of no contact the
lock is released and the device falls back to autonomous idle behavior (FR-011, FR-016). Takeover after a
crash works because the previous lock expires on timeout (FR-012).

**Rationale**: Directly encodes the two timeout-related clarifications; keeps a single source of truth for
ownership and health; pure logic, fully host-testable.

**Alternatives considered**:
- **Last-writer-wins (no lock)**: Simpler but the spec chose single-controller arbitration. Rejected per
  FR-017.
- **TCP keepalive for liveness**: Tied to the rejected TCP transport. Rejected.

## Decision 5: Acknowledgements & error reporting

**Decision**: Discrete actions (acquire/release, blink, wink, idle toggle, status query) get a small ACK/NAK
reply with a reason code; streaming emotion/gaze updates are unacknowledged except via periodic status.
Out-of-range parameters are **rejected** (NAK, no clamping) leaving state unchanged (clarification + FR-007).

**Rationale**: Gives the controller confirmation and health (FR-019) without paying ack latency on the
high-rate stream (streaming clarification). Status responses let the controller reconcile state (FR-013, US5).

**Alternatives considered**:
- **Ack every datagram**: Doubles traffic and adds latency for no benefit on superseded updates. Rejected.
- **No acks at all**: Loses command confirmation and health signaling required by FR-019/FR-011. Rejected.

## Decision 6: Network address visibility & provisioning

**Decision**: Wi-Fi credentials are compile-time/provisioned out of band (v1). On join, the device renders
its IP address on the OLED so the operator can point the controller at it (FR-015). No mDNS in v1.

**Rationale**: Matches the clarified "configured/known address, surfaced on the OLED" decision; smallest
viable footprint. mDNS/Bonjour is noted as a future enhancement.

**Alternatives considered**:
- **mDNS/Bonjour discovery**: Better UX but deferred per clarification; adds a library and multicast handling.
  Deferred to a future feature.

## Decision 7: Testing strategy

**Decision**: Host-native unit tests for `CommandCodec` (encode/decode round-trips, truncated/oversized/
malformed frames, range checks) and `SessionManager` (acquire, reject-when-locked, takeover, heartbeat,
5 s timeout, newest-wins selection). Python client unit tests for framing. On-hardware visual validation on
the TTGO LoRa32 per the repo commit policy for rendering/timing/I2C/API changes.

**Rationale**: Keeps the bulk of logic off-target for fast, deterministic tests (Hardware Abstraction);
hardware validation still required before committing embedded changes per repo guidelines.

**Alternatives considered**:
- **Hardware-only testing**: Slow, flaky, and gated on a board; insufficient for protocol edge cases. Rejected
  as the primary method.

## Summary of resolved unknowns

| Unknown | Resolution |
|---------|-----------|
| Transport for low latency + resilience | UDP best-effort streaming + acks/heartbeats |
| Wire format | Fixed-size little-endian binary, versioned, ≤32 B |
| Avoiding render stalls | Single non-blocking poll per loop; newest-wins buffering |
| Single-controller arbitration | `SessionManager` lock by IP:port, "in use" NAK |
| Liveness & fallback timing | ~1 s heartbeat, 5 s no-contact → release + idle |
| Out-of-range handling | Reject (NAK), no clamping, state unchanged |
| Address discovery (v1) | Static/known IP, shown on OLED; mDNS deferred |
| Testing | Host-native codec/session tests + HW visual validation |

All NEEDS CLARIFICATION items are resolved. Ready for Phase 1 design.
