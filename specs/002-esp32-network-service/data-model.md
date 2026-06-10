# Phase 1 Data Model: ESP32 Network Control Service

**Feature**: 002-esp32-network-service | **Date**: 2026-06-01

Entities below are the in-RAM structures of the device-side service. All state is volatile (no persistence).
Sizes are indicative for an ESP32 (ample RAM); the design still avoids dynamic allocation in the hot path.

## Entity: ControlSession

Represents the single active controller currently permitted to command the device.

| Field | Type | Description | Validation / Notes |
|-------|------|-------------|--------------------|
| `active` | bool | Whether a controller currently holds the lock | Derived from address + liveness |
| `clientIp` | uint32 (IPv4) | Source IP of the controlling client | Set on successful ACQUIRE |
| `clientPort` | uint16 | Source UDP port of the controlling client | Set on successful ACQUIRE |
| `lastContactMs` | uint32 | `millis()` of last frame/heartbeat from the holder | Updated on any valid frame from holder |
| `lastAppliedSeq` | uint32 | Highest sequence number applied (newest-wins guard) | Frames with `seq <= lastAppliedSeq` ignored |

**Lifecycle / state transitions**:

```text
IDLE (no controller)
  └─(ACQUIRE from X)──────────────▶ LOCKED(by X)
LOCKED(by X)
  ├─(frame/heartbeat from X)──────▶ LOCKED(by X)  [refresh lastContactMs]
  ├─(ACQUIRE/cmd from Y≠X)────────▶ LOCKED(by X)  [reply NAK "in use"]
  ├─(RELEASE from X)──────────────▶ IDLE
  └─(now - lastContactMs > 5000)──▶ IDLE          [timeout: release lock]
IDLE
  └─(no controller)───────────────▶ device runs autonomous idle behavior
```

**Rules**:
- Only frames from `clientIp:clientPort` mutate device state while `active` is true (FR-017).
- Timeout (5 s no contact) releases the lock and triggers idle fallback (FR-011, FR-016).
- A new controller may ACQUIRE only when `active` is false (after RELEASE or timeout) → enables takeover (FR-012).

## Entity: Command

A single decoded instruction from controller to device. Decoded from a wire frame by `CommandCodec`.

| Field | Type | Description |
|-------|------|-------------|
| `version` | uint8 | Protocol version; mismatch → NAK |
| `opcode` | uint8 | Command type (see table below) |
| `seq` | uint32 | Monotonic sequence number (newest-wins for streaming) |
| `payload` | union | Opcode-specific fields (below) |

**Opcodes & payloads**:

| Opcode | Name | Payload fields | Maps to library call |
|--------|------|----------------|----------------------|
| 0x01 | `ACQUIRE` | — | Lock acquire (session) |
| 0x02 | `RELEASE` | — | Lock release (session) |
| 0x03 | `HEARTBEAT` | — | Liveness refresh |
| 0x10 | `SET_EMOTION` | `valence:int16(×1000)`, `arousal:uint16(×1000)`, `duration_ms:uint16` | `setEmotion(v, a, dur)` |
| 0x11 | `SET_PRESET` | `presetId:uint8`, `intensity:uint8(×100)` | `happy()/sad()/...` |
| 0x12 | `SET_GAZE` | `h:int16(deg)`, `v:int16(deg)`, `duration_ms:uint16` | `setEyePosition(h, v, dur)` |
| 0x13 | `BLINK` | `duration_ms:uint16` | `blink(dur)` |
| 0x14 | `WINK` | `eye:uint8(0=L,1=R)`, `duration_ms:uint16` | `wink(eye, dur)` |
| 0x15 | `SET_IDLE` | `enable:uint8(0/1)` | `enableIdleBehavior(enable)` |
| 0x20 | `QUERY_STATUS` | — | Returns `StatusReport` |

**Validation rules** (decode-time; failure → NAK with reason, state unchanged — FR-007, FR-014, SC-007):
- Frame length must exactly match the opcode's expected size; truncated/oversized → reject.
- `version` must match supported protocol version.
- `valence` ∈ [-0.5, +0.5], `arousal` ∈ [0.0, 1.0] after scaling; out-of-range → **reject (no clamping)**.
- `h` ∈ [-90, +90], `v` ∈ [-45, +45]; out-of-range → reject.
- `presetId` must be a known preset; `eye` ∈ {0,1}; unknown → reject.

**Scaling conventions**: floats are transmitted as fixed-point integers (`×1000` for valence/arousal,
`×100` for intensity) to keep frames compact and avoid float-endianness concerns; codec converts on decode.

## Entity: PendingTargets (newest-wins buffer)

Holds the most recent streaming target per channel between render frames; implements FR-008 / SC-006.

| Field | Type | Description |
|-------|------|-------------|
| `hasEmotion` | bool | A new emotion target is pending |
| `emotion` | {valence, arousal, duration} | Latest emotion target only (older discarded) |
| `hasGaze` | bool | A new gaze target is pending |
| `gaze` | {h, v, duration} | Latest gaze target only |
| `pendingActions` | bitset | One-shot actions queued this frame (blink/wink/idle toggle) |

**Rules**: Buffer is bounded and constant-size; receiving a newer streaming command overwrites the pending
target rather than appending. Applied once per render frame, then cleared.

## Entity: StatusReport (device → controller)

Returned in response to `QUERY_STATUS` and used for state reconciliation (FR-013, US5).

| Field | Type | Description |
|-------|------|-------------|
| `valence` | int16 (×1000) | Current interpolated valence |
| `arousal` | uint16 (×1000) | Current interpolated arousal |
| `gazeH` | int16 | Current horizontal gaze (deg) |
| `gazeV` | int16 | Current vertical gaze (deg) |
| `idleEnabled` | uint8 | Idle behavior on/off |
| `connectionHealthy` | uint8 | Lock held + within heartbeat window |
| `uptimeMs` | uint32 | Device uptime |

## Entity: Acknowledgement (device → controller)

Reply to discrete commands; carries accept/reject outcome (FR-019).

| Field | Type | Description |
|-------|------|-------------|
| `ackSeq` | uint32 | `seq` of the command being acknowledged |
| `accepted` | uint8 | 1 = applied, 0 = rejected |
| `reason` | uint8 | Reason code: OK, IN_USE, BAD_VERSION, BAD_LENGTH, OUT_OF_RANGE, UNKNOWN_OPCODE |

## Relationships

```text
ControlSession 1 ──owns──▶ 1 PendingTargets (per active controller)
Command ──decoded-by──▶ CommandCodec ──validated-against──▶ ControlSession (ownership) + range rules
Command(streaming) ──coalesced-into──▶ PendingTargets ──applied-to──▶ BotiEyes public API
Command(discrete)  ──produces──▶ Acknowledgement
QUERY_STATUS ──produces──▶ StatusReport
```

## Mapping to existing library API

The service is a translator; it adds **no** new rendering or emotion logic:

| Network concept | Existing BotiEyes call (unchanged) |
|-----------------|-----------------------------------|
| `SET_EMOTION` | `setEmotion(valence, arousal, duration_ms)` |
| `SET_PRESET` | `happy/sad/angry/calm/...(intensity)` |
| `SET_GAZE` | `setEyePosition(h, v, duration_ms)` |
| `BLINK` / `WINK` | `blink(duration_ms)` / `wink(eye, duration_ms)` |
| `SET_IDLE` | `enableIdleBehavior(enable)` |
| idle fallback / no controller | `enableIdleBehavior(true)` + autonomous `update()` |
| `QUERY_STATUS` | `getCurrentEmotion()`, `getEyePosition()` |
