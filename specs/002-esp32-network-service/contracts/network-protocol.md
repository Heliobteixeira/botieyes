# Contract: BotiEyes Network Control Protocol (v1)

**Feature**: 002-esp32-network-service | **Date**: 2026-06-01 | **Protocol version**: `1`

This is the wire contract between a controller (any machine on the LAN) and the ESP32 device.
It is the authoritative interface specification; the device service and the reference client MUST conform.

## Transport

- **Protocol**: UDP/IPv4 over the local network.
- **Device listen port**: `4210` (UDP). The device renders its IP on the OLED at startup (FR-015).
- **Controller port**: ephemeral; the device replies to the source `IP:port` of received frames.
- **Delivery semantics**: best-effort. Streaming commands (`SET_EMOTION`, `SET_GAZE`) are unacknowledged and
  newest-wins; discrete commands receive an `ACK`. No retransmission or ordering is guaranteed by transport.
- **Byte order**: little-endian for all multi-byte fields.

## Frame format

All frames share a 6-byte header followed by an opcode-specific payload. Maximum frame size is **32 bytes**.

```text
Offset  Size  Field      Description
0       1     version    Protocol version (= 1)
1       1     opcode     Command/response code (see tables)
2       4     seq        Monotonic sequence number (uint32), set by sender
6       ...   payload    Opcode-specific (0–26 bytes)
```

- Frames whose total length does not exactly match the opcode's defined size are **rejected** (`BAD_LENGTH`).
- Frames with an unsupported `version` are **rejected** (`BAD_VERSION`).
- Unknown `opcode` values are **rejected** (`UNKNOWN_OPCODE`).
- Rejection never mutates device state (FR-007, FR-014, SC-007).

## Fixed-point encoding

To keep frames compact and endianness-safe, floats are sent as scaled integers:

| Logical value | Wire type | Scale | Range on wire |
|---------------|-----------|-------|---------------|
| valence (−0.5..+0.5) | int16 | ×1000 | −500..+500 |
| arousal (0.0..1.0) | uint16 | ×1000 | 0..1000 |
| intensity (0.0..1.0) | uint8 | ×100 | 0..100 |
| gaze h (−90..+90 deg) | int16 | ×1 | −90..+90 |
| gaze v (−45..+45 deg) | int16 | ×1 | −45..+45 |
| duration_ms | uint16 | ×1 | 0..65535 |

## Commands (controller → device)

### Session control

| Opcode | Name | Payload | Effect |
|--------|------|---------|--------|
| `0x01` | `ACQUIRE` | none | Request the single-controller lock. ACK `OK` if free; NAK `IN_USE` if held by another (FR-017). |
| `0x02` | `RELEASE` | none | Release the lock if held by sender. ACK `OK`. |
| `0x03` | `HEARTBEAT` | none | Refresh liveness. Sent ~every 1 s. No ACK (counts as contact). |

### Expression control

| Opcode | Name | Payload (after 6-byte header) | Maps to |
|--------|------|------------------------------|---------|
| `0x10` | `SET_EMOTION` | `int16 valence`, `uint16 arousal`, `uint16 duration_ms` | `setEmotion()` — streaming, no ACK |
| `0x11` | `SET_PRESET` | `uint8 presetId`, `uint8 intensity` | preset helpers — ACK |
| `0x12` | `SET_GAZE` | `int16 h`, `int16 v`, `uint16 duration_ms` | `setEyePosition()` — streaming, no ACK |
| `0x13` | `BLINK` | `uint16 duration_ms` | `blink()` — ACK |
| `0x14` | `WINK` | `uint8 eye (0=L,1=R)`, `uint16 duration_ms` | `wink()` — ACK |
| `0x15` | `SET_IDLE` | `uint8 enable (0/1)` | `enableIdleBehavior()` — ACK |
| `0x20` | `QUERY_STATUS` | none | Returns `STATUS` (0x81) |

**Preset IDs** (match library helpers): `0 happy, 1 sad, 2 angry, 3 calm, 4 excited, 5 tired,
6 surprised, 7 anxious, 8 content, 9 curious, 10 thinking, 11 confused, 12 neutral`.

All expression commands require the sender to currently hold the lock; otherwise NAK `IN_USE`.

## Responses (device → controller)

| Opcode | Name | Payload | Meaning |
|--------|------|---------|---------|
| `0x80` | `ACK` | `uint32 ackSeq`, `uint8 accepted`, `uint8 reason` | Result of a discrete command (FR-019) |
| `0x81` | `STATUS` | `int16 valence`, `uint16 arousal`, `int16 gazeH`, `int16 gazeV`, `uint8 idleEnabled`, `uint8 connectionHealthy`, `uint32 uptimeMs` | Reply to `QUERY_STATUS` (FR-013) |

### Reason codes (in `ACK`)

| Code | Name | Meaning |
|------|------|---------|
| `0` | `OK` | Command accepted and applied |
| `1` | `IN_USE` | Another controller holds the lock (or sender is not the holder) |
| `2` | `BAD_VERSION` | Unsupported protocol version |
| `3` | `BAD_LENGTH` | Frame size did not match opcode |
| `4` | `OUT_OF_RANGE` | A parameter was outside its valid range (rejected, not clamped) |
| `5` | `UNKNOWN_OPCODE` | Opcode not recognized |

## Sequencing & newest-wins

- Senders set `seq` strictly increasing per session.
- For streaming opcodes (`SET_EMOTION`, `SET_GAZE`), the device keeps only the highest-`seq` target per
  channel; frames with `seq <= lastAppliedSeq` for that channel are ignored (FR-008). This prevents stale
  emotions from being applied after newer ones, even on reordering.
- Discrete commands are applied in arrival order; each yields one `ACK`.

## Session & liveness rules

- **Lock**: single active controller, keyed by source `IP:port` (FR-017).
- **Heartbeat**: controller sends `HEARTBEAT` ~every 1 s; any received frame also counts as contact.
- **Timeout**: after **5 s** with no contact from the holder, the device releases the lock and reverts to
  autonomous idle behavior (FR-011, FR-016). A new controller may then `ACQUIRE` (takeover, FR-012).
- **Recovery**: because UDP is connectionless, the controller resumes simply by sending frames again after a
  network blip; if the lock expired it re-`ACQUIRE`s (FR-010, SC-003).

## Error & safety guarantees

- Malformed, truncated, oversized, or out-of-range frames are rejected without affecting eye state or
  device stability (FR-014, SC-007).
- **Discrete** commands report rejection via a `NAK` (`ACK` with `accepted=0` and a reason code) so the
  controller is explicitly informed (FR-007, FR-019).
- **Streaming** commands (`SET_EMOTION`, `SET_GAZE`) that fail validation are dropped without an error reply;
  the controller observes the unchanged state via `QUERY_STATUS` (FR-007, FR-013, FR-019).
- Network handling is non-blocking; it never stalls rendering (FR-009).
- v1 assumes a trusted LAN with no authentication (FR-018); the service MUST NOT be exposed to untrusted
  networks.

## Example exchanges

```text
# Acquire control
C → D  ver=1 op=0x01 seq=1                      (ACQUIRE)
D → C  op=0x80 ackSeq=1 accepted=1 reason=0     (ACK OK)

# Stream emotions (no acks)
C → D  ver=1 op=0x10 seq=2 valence=+350 arousal=550 dur=400   (happy-ish)
C → D  ver=1 op=0x10 seq=3 valence=-300 arousal=800 dur=400   (angry-ish, supersedes)

# Heartbeat
C → D  ver=1 op=0x03 seq=4                       (HEARTBEAT)

# Blink (discrete, acked)
C → D  ver=1 op=0x13 seq=5 dur=150
D → C  op=0x80 ackSeq=5 accepted=1 reason=0

# Out-of-range emotion is rejected
C → D  ver=1 op=0x10 seq=6 valence=+900 ...      (valence > +500)
# (streaming: dropped silently; a following QUERY_STATUS shows unchanged state)

# Second controller blocked
C2 → D ver=1 op=0x01 seq=1                        (ACQUIRE)
D → C2 op=0x80 ackSeq=1 accepted=0 reason=1       (NAK IN_USE)

# Release
C → D  ver=1 op=0x02 seq=7                        (RELEASE)
D → C  op=0x80 ackSeq=7 accepted=1 reason=0
```
