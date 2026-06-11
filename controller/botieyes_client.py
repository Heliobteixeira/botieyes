"""BotiEyes UDP control client (reference controller).

Speaks the BotiEyes Network Control Protocol v1 (see
specs/002-esp32-network-service/contracts/network-protocol.md).

Uses only the Python standard library. Best-effort streaming for emotion/gaze,
ACK'd discrete commands, automatic ~1 s heartbeat, and re-acquire on timeout.
"""

from __future__ import annotations

import socket
import struct
import threading
import time
from dataclasses import dataclass
from typing import Optional

# --- Protocol constants (mirror BotiEyes/src/net/NetProtocol.h) ---
PROTOCOL_VERSION = 1
DEFAULT_UDP_PORT = 4210
HEADER = struct.Struct("<BBI")  # version, opcode, seq (little-endian)

HEARTBEAT_INTERVAL_S = 1.0
SESSION_TIMEOUT_S = 5.0

# Opcodes
OP_ACQUIRE = 0x01
OP_RELEASE = 0x02
OP_HEARTBEAT = 0x03
OP_SET_EMOTION = 0x10
OP_SET_PRESET = 0x11
OP_SET_GAZE = 0x12
OP_BLINK = 0x13
OP_WINK = 0x14
OP_SET_IDLE = 0x15
OP_QUERY_STATUS = 0x20
OP_ACK = 0x80
OP_STATUS = 0x81

# Reason codes
REASON_NAMES = {
    0: "OK",
    1: "IN_USE",
    2: "BAD_VERSION",
    3: "BAD_LENGTH",
    4: "OUT_OF_RANGE",
    5: "UNKNOWN_OPCODE",
}

VALENCE_SCALE = 1000
AROUSAL_SCALE = 1000
INTENSITY_SCALE = 100

PRESETS = [
    "happy", "sad", "angry", "calm", "excited", "tired", "surprised",
    "anxious", "content", "curious", "thinking", "confused", "neutral",
]


class BotiEyesError(Exception):
    """Raised when the device rejects a discrete command."""


@dataclass
class StatusReport:
    valence: float
    arousal: float
    gaze_h: int
    gaze_v: int
    idle_enabled: bool
    connection_healthy: bool
    uptime_ms: int


@dataclass
class Ack:
    ack_seq: int
    accepted: bool
    reason: int

    @property
    def reason_name(self) -> str:
        return REASON_NAMES.get(self.reason, f"UNKNOWN({self.reason})")


class BotiEyesClient:
    """Client for one ESP32 BotiEyes device.

    Use as a context manager to acquire/release the single-controller lock and
    run the background heartbeat automatically::

        with BotiEyesClient("192.168.1.42") as eyes:
            eyes.set_emotion(0.35, 0.55)
    """

    def __init__(self, host: str, port: int = DEFAULT_UDP_PORT, timeout: float = 1.0):
        self._addr = (host, port)
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(timeout)
        self._seq = 0
        self._seq_lock = threading.Lock()
        self._hb_stop = threading.Event()
        self._hb_thread: Optional[threading.Thread] = None

    # --- framing helpers ---
    def _next_seq(self) -> int:
        with self._seq_lock:
            self._seq += 1
            return self._seq

    def _send(self, opcode: int, payload: bytes = b"") -> int:
        seq = self._next_seq()
        frame = HEADER.pack(PROTOCOL_VERSION, opcode, seq) + payload
        self._sock.sendto(frame, self._addr)
        return seq

    def _recv_frame(self) -> Optional[bytes]:
        try:
            data, _ = self._sock.recvfrom(64)
            return data
        except socket.timeout:
            return None

    def _await_ack(self, seq: int, retries: int = 3) -> Ack:
        """Send already done by caller; wait for an ACK matching seq."""
        for _ in range(retries):
            frame = self._recv_frame()
            if frame is None:
                continue
            if len(frame) >= HEADER.size and frame[1] == OP_ACK:
                ack_seq, accepted, reason = struct.unpack_from("<IBB", frame, HEADER.size)
                if ack_seq == seq:
                    return Ack(ack_seq, bool(accepted), reason)
        raise BotiEyesError(f"No ACK received for seq={seq}")

    def _discrete(self, opcode: int, payload: bytes = b"") -> Ack:
        seq = self._send(opcode, payload)
        ack = self._await_ack(seq)
        if not ack.accepted:
            raise BotiEyesError(f"Command rejected: {ack.reason_name}")
        return ack

    # --- session control ---
    def acquire(self) -> Ack:
        return self._discrete(OP_ACQUIRE)

    def release(self) -> Ack:
        return self._discrete(OP_RELEASE)

    def heartbeat(self) -> None:
        # Best-effort, no ACK expected.
        self._send(OP_HEARTBEAT)

    # --- streaming commands (best-effort, no ACK) ---
    def set_emotion(self, valence: float, arousal: float, duration_ms: int = 400) -> None:
        payload = struct.pack(
            "<hHH",
            int(round(valence * VALENCE_SCALE)),
            int(round(arousal * AROUSAL_SCALE)),
            duration_ms,
        )
        self._send(OP_SET_EMOTION, payload)

    def set_gaze(self, h: int, v: int, duration_ms: int = 300) -> None:
        payload = struct.pack("<hhH", h, v, duration_ms)
        self._send(OP_SET_GAZE, payload)

    # --- discrete expression commands (ACK'd) ---
    def preset(self, name: str, intensity: float = 1.0) -> Ack:
        try:
            preset_id = PRESETS.index(name)
        except ValueError as exc:
            raise BotiEyesError(f"Unknown preset: {name}") from exc
        payload = struct.pack("<BB", preset_id, int(round(intensity * INTENSITY_SCALE)))
        return self._discrete(OP_SET_PRESET, payload)

    def blink(self, duration_ms: int = 150) -> Ack:
        return self._discrete(OP_BLINK, struct.pack("<H", duration_ms))

    def wink(self, eye: str = "left", duration_ms: int = 150) -> Ack:
        eye_id = 0 if eye.lower().startswith("l") else 1
        return self._discrete(OP_WINK, struct.pack("<BH", eye_id, duration_ms))

    def set_idle(self, enable: bool) -> Ack:
        return self._discrete(OP_SET_IDLE, struct.pack("<B", 1 if enable else 0))

    # --- status ---
    def status(self, retries: int = 3) -> StatusReport:
        seq = self._send(OP_QUERY_STATUS)
        for _ in range(retries):
            frame = self._recv_frame()
            if frame is None:
                continue
            if len(frame) >= HEADER.size and frame[1] == OP_STATUS:
                v, a, gh, gv, idle, healthy, uptime = struct.unpack_from(
                    "<hHhhBBI", frame, HEADER.size
                )
                return StatusReport(
                    valence=v / VALENCE_SCALE,
                    arousal=a / AROUSAL_SCALE,
                    gaze_h=gh,
                    gaze_v=gv,
                    idle_enabled=bool(idle),
                    connection_healthy=bool(healthy),
                    uptime_ms=uptime,
                )
        raise BotiEyesError(f"No STATUS received for seq={seq}")

    # --- heartbeat thread + re-acquire on timeout ---
    def _heartbeat_loop(self) -> None:
        while not self._hb_stop.wait(HEARTBEAT_INTERVAL_S):
            try:
                self.heartbeat()
            except OSError:
                # Transient network error: keep trying; device will idle-fallback
                # after SESSION_TIMEOUT_S and we re-acquire below once reachable.
                try:
                    self.acquire()
                except (OSError, BotiEyesError):
                    pass

    def start_heartbeat(self) -> None:
        if self._hb_thread is not None:
            return
        self._hb_stop.clear()
        self._hb_thread = threading.Thread(target=self._heartbeat_loop, daemon=True)
        self._hb_thread.start()

    def stop_heartbeat(self) -> None:
        self._hb_stop.set()
        if self._hb_thread is not None:
            self._hb_thread.join(timeout=2.0)
            self._hb_thread = None

    def close(self) -> None:
        self.stop_heartbeat()
        self._sock.close()

    # --- context manager ---
    def __enter__(self) -> "BotiEyesClient":
        self.acquire()
        self.start_heartbeat()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        try:
            self.stop_heartbeat()
            self.release()
        except (OSError, BotiEyesError):
            pass
        finally:
            self._sock.close()
