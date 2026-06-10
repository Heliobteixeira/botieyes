"""Unit tests for the BotiEyes reference UDP client framing.

Runs entirely on loopback with a tiny mock device that decodes frames and
replies with ACK/STATUS, so no ESP32 hardware is required.

Run:
    cd controller
    python -m pytest test_client.py        # or: python test_client.py
"""

from __future__ import annotations

import socket
import struct
import threading

import botieyes_client as bc
from botieyes_client import BotiEyesClient, BotiEyesError


class MockDevice:
    """Minimal UDP device: decodes inbound frames, ACKs discrete commands,
    answers QUERY_STATUS, and records the last streaming command received."""

    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", 0))
        self.port = self.sock.getsockname()[1]
        self.received = []          # list of (opcode, seq, payload)
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self.nak_next = None        # set to a reason code to reject next discrete

    def start(self):
        self._thread.start()

    def stop(self):
        self._stop.set()
        # Unblock recvfrom.
        try:
            socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(
                b"\x00", ("127.0.0.1", self.port)
            )
        except OSError:
            pass
        self._thread.join(timeout=2.0)
        self.sock.close()

    def _serve(self):
        self.sock.settimeout(0.5)
        while not self._stop.is_set():
            try:
                data, addr = self.sock.recvfrom(64)
            except socket.timeout:
                continue
            except OSError:
                break
            if len(data) < bc.HEADER.size:
                continue
            version, opcode, seq = bc.HEADER.unpack_from(data, 0)
            payload = data[bc.HEADER.size:]
            self.received.append((opcode, seq, payload))

            if opcode in (bc.OP_SET_EMOTION, bc.OP_SET_GAZE, bc.OP_HEARTBEAT):
                continue  # streaming/heartbeat: no reply

            if opcode == bc.OP_QUERY_STATUS:
                body = struct.pack("<hHhhBBI", 350, 550, -45, 15, 0, 1, 123456)
                reply = bc.HEADER.pack(bc.PROTOCOL_VERSION, bc.OP_STATUS, 0) + body
                self.sock.sendto(reply, addr)
                continue

            # Discrete commands → ACK / NAK.
            accepted = self.nak_next is None
            reason = 0 if accepted else self.nak_next
            self.nak_next = None
            body = struct.pack("<IBB", seq, 1 if accepted else 0, reason)
            reply = bc.HEADER.pack(bc.PROTOCOL_VERSION, bc.OP_ACK, 0) + body
            self.sock.sendto(reply, addr)


def _client(dev: MockDevice) -> BotiEyesClient:
    return BotiEyesClient("127.0.0.1", dev.port, timeout=0.5)


def test_acquire_release_roundtrip():
    dev = MockDevice(); dev.start()
    try:
        c = _client(dev)
        ack = c.acquire()
        assert ack.accepted
        assert dev.received[-1][0] == bc.OP_ACQUIRE
        c.release()
        assert dev.received[-1][0] == bc.OP_RELEASE
        c.close()
    finally:
        dev.stop()


def test_set_emotion_framing():
    dev = MockDevice(); dev.start()
    try:
        c = _client(dev)
        c.set_emotion(0.35, 0.55, 400)
        # Give the mock a moment to record.
        c._sock.settimeout(0.2)
        import time as _t; _t.sleep(0.1)
        opcode, _, payload = dev.received[-1]
        assert opcode == bc.OP_SET_EMOTION
        v, a, dur = struct.unpack("<hHH", payload)
        assert v == 350 and a == 550 and dur == 400
        c.close()
    finally:
        dev.stop()


def test_preset_ack_and_nak():
    dev = MockDevice(); dev.start()
    try:
        c = _client(dev)
        ack = c.preset("happy", 1.0)
        assert ack.accepted and ack.reason == 0
        opcode, _, payload = dev.received[-1]
        assert opcode == bc.OP_SET_PRESET
        pid, intensity = struct.unpack("<BB", payload)
        assert pid == 0 and intensity == 100

        dev.nak_next = 4  # OUT_OF_RANGE
        try:
            c.preset("happy", 1.0)
            assert False, "expected BotiEyesError on NAK"
        except BotiEyesError:
            pass
        c.close()
    finally:
        dev.stop()


def test_gaze_blink_wink_idle_framing():
    dev = MockDevice(); dev.start()
    try:
        c = _client(dev)
        c.set_gaze(-45, 15, 300)
        import time as _t; _t.sleep(0.1)
        op, _, pl = dev.received[-1]
        assert op == bc.OP_SET_GAZE
        h, v, dur = struct.unpack("<hhH", pl)
        assert h == -45 and v == 15 and dur == 300

        c.blink(150)
        op, _, pl = dev.received[-1]
        assert op == bc.OP_BLINK and struct.unpack("<H", pl)[0] == 150

        c.wink("right", 150)
        op, _, pl = dev.received[-1]
        assert op == bc.OP_WINK
        eye, wdur = struct.unpack("<BH", pl)
        assert eye == 1 and wdur == 150

        c.set_idle(True)
        op, _, pl = dev.received[-1]
        assert op == bc.OP_SET_IDLE and pl == b"\x01"
        c.close()
    finally:
        dev.stop()


def test_status_decode():
    dev = MockDevice(); dev.start()
    try:
        c = _client(dev)
        s = c.status()
        assert abs(s.valence - 0.35) < 1e-6
        assert abs(s.arousal - 0.55) < 1e-6
        assert s.gaze_h == -45 and s.gaze_v == 15
        assert s.connection_healthy is True
        assert s.uptime_ms == 123456
        c.close()
    finally:
        dev.stop()


def _run_all():
    tests = [
        test_acquire_release_roundtrip,
        test_set_emotion_framing,
        test_preset_ack_and_nak,
        test_gaze_blink_wink_idle_framing,
        test_status_decode,
    ]
    failures = 0
    for t in tests:
        try:
            t()
            print(f"PASS {t.__name__}")
        except Exception as exc:  # noqa: BLE001
            failures += 1
            print(f"FAIL {t.__name__}: {exc}")
    print(f"\n{len(tests)} tests, {failures} failures")
    return failures


if __name__ == "__main__":
    raise SystemExit(1 if _run_all() else 0)
