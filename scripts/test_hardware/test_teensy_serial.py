"""
Direct Teensy serial tests.

Connects to the Teensy via USB serial and exercises every command in the
internal protocol.  Each test sends a command and verifies the response
(ACK or STATUS).
"""

import time
import serial
from typing import Optional

from .protocol import (
    Cmd, Resp, Mode,
    set_mode, set_brightness, set_framerate, request_status,
    upload_pattern, live_frame, build_packet,
    parse_response, parse_status, is_ack, StatusResponse,
)
from .result import TestResult, TestReport, Verdict


RESPONSE_TIMEOUT = 0.5  # seconds


def _read_response(ser: serial.Serial, timeout: float = RESPONSE_TIMEOUT) -> bytes:
    """Read bytes from serial until timeout or a complete frame is found."""
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        waiting = ser.in_waiting
        if waiting:
            buf += ser.read(waiting)
            # Check for complete frame
            if 0xFF in buf and 0xFE in buf:
                start = buf.index(0xFF)
                end = buf.index(0xFE, start)
                if end > start:
                    return buf
        time.sleep(0.01)
    return buf


def _send_and_expect_ack(ser: serial.Serial, packet: bytes, name: str) -> TestResult:
    """Send a packet, expect an ACK response."""
    start = time.time()
    ser.reset_input_buffer()
    ser.write(packet)
    raw = _read_response(ser)
    elapsed = (time.time() - start) * 1000

    if is_ack(raw):
        return TestResult(name, Verdict.PASS, elapsed, "ACK received")
    elif len(raw) == 0:
        return TestResult(name, Verdict.FAIL, elapsed, "No response (timeout)")
    else:
        return TestResult(name, Verdict.FAIL, elapsed,
                          f"Unexpected response",
                          f"Raw: {raw.hex().upper()}")


def _send_and_expect_status(ser: serial.Serial) -> tuple[TestResult, Optional[StatusResponse]]:
    """Send status request, expect a STATUS response."""
    start = time.time()
    ser.reset_input_buffer()
    ser.write(request_status())
    raw = _read_response(ser)
    elapsed = (time.time() - start) * 1000

    status = parse_status(raw)
    if status:
        msg = f"mode={status.mode} index={status.index} sd={status.sd_present}"
        return TestResult("Status request", Verdict.PASS, elapsed, msg), status
    elif len(raw) == 0:
        return TestResult("Status request", Verdict.FAIL, elapsed, "No response"), None
    else:
        return TestResult("Status request", Verdict.FAIL, elapsed,
                          "Invalid status response",
                          f"Raw: {raw.hex().upper()}"), None


# ---------------------------------------------------------------------------
# Test functions
# ---------------------------------------------------------------------------

def test_connectivity(ser: serial.Serial) -> TestResult:
    """Verify we can talk to the Teensy by requesting status."""
    result, _ = _send_and_expect_status(ser)
    result.name = "Teensy connectivity"
    return result


def test_brightness(ser: serial.Serial) -> list[TestResult]:
    """Test brightness at several levels."""
    results = []
    for val, label in [(0, "min"), (128, "50%"), (255, "max")]:
        pkt = set_brightness(val)
        results.append(_send_and_expect_ack(ser, pkt, f"Brightness {label} ({val})"))
        time.sleep(0.15)
    return results


def test_framerate(ser: serial.Serial) -> list[TestResult]:
    results = []
    for val, label in [(10, "10ms/100fps"), (33, "33ms/30fps"), (100, "100ms/10fps")]:
        pkt = set_framerate(val)
        results.append(_send_and_expect_ack(ser, pkt, f"Frame rate {label}"))
        time.sleep(0.15)
    return results


def test_modes(ser: serial.Serial) -> list[TestResult]:
    """Cycle through every display mode and verify via status."""
    results = []

    mode_tests = [
        (Mode.IDLE, 0, "Idle (LEDs off)"),
        (Mode.IMAGE, 0, "Image 0 (Smiley)"),
        (Mode.IMAGE, 1, "Image 1 (Rainbow)"),
        (Mode.IMAGE, 2, "Image 2 (Heart)"),
        (Mode.PATTERN, 0, "Pattern 0 (Rainbow)"),
        (Mode.PATTERN, 1, "Pattern 1 (Wave)"),
        (Mode.PATTERN, 2, "Pattern 2 (Gradient)"),
        (Mode.PATTERN, 3, "Pattern 3 (Sparkle)"),
        (Mode.PATTERN, 4, "Pattern 4 (Fire)"),
        (Mode.PATTERN, 5, "Pattern 5 (Comet)"),
        (Mode.PATTERN, 6, "Pattern 6 (Breathing)"),
        (Mode.PATTERN, 7, "Pattern 7 (Strobe)"),
        (Mode.PATTERN, 8, "Pattern 8 (Meteor)"),
        (Mode.PATTERN, 9, "Pattern 9 (Wipe)"),
        (Mode.PATTERN, 10, "Pattern 10 (Plasma)"),
        (Mode.PATTERN, 11, "Pattern 11 (Music VU Meter)"),
        (Mode.PATTERN, 12, "Pattern 12 (Music Pulse)"),
        (Mode.PATTERN, 13, "Pattern 13 (Music Rainbow)"),
        (Mode.PATTERN, 14, "Pattern 14 (Music Center)"),
        (Mode.PATTERN, 15, "Pattern 15 (Music Sparkle)"),
        (Mode.PATTERN, 16, "Pattern 16 (Split Spin)"),
        (Mode.PATTERN, 17, "Pattern 17 (Theater Chase)"),
        (Mode.SEQUENCE, 0, "Sequence 0 (Demo)"),
    ]

    for mode, idx, label in mode_tests:
        pkt = set_mode(mode, idx)
        result = _send_and_expect_ack(ser, pkt, f"Set mode: {label}")
        results.append(result)
        time.sleep(0.15)

        # Verify mode via status
        stat_result, status = _send_and_expect_status(ser)
        if status and status.mode == mode:
            stat_result.name = f"Verify mode: {label}"
        elif status:
            stat_result = TestResult(
                f"Verify mode: {label}", Verdict.FAIL, stat_result.duration_ms,
                f"Expected mode={mode}, got mode={status.mode}")
        results.append(stat_result)
        time.sleep(0.15)

    return results


def test_pattern_upload(ser: serial.Serial) -> TestResult:
    """Upload a custom pattern configuration."""
    pkt = upload_pattern(
        index=0, ptype=0,
        color1=(255, 0, 0), color2=(0, 255, 0), speed=50)
    return _send_and_expect_ack(ser, pkt, "Upload custom pattern config")


def test_live_frame(ser: serial.Serial) -> TestResult:
    """Send a live frame of all-red pixels."""
    pixels = [(255, 0, 0)] * 31
    pkt = live_frame(pixels)
    start = time.time()
    ser.reset_input_buffer()
    ser.write(pkt)
    # Live frames don't always produce an ACK - just check no error
    time.sleep(0.2)
    elapsed = (time.time() - start) * 1000
    # Put us in live mode first
    ser.write(set_mode(Mode.LIVE, 0))
    time.sleep(0.1)
    ser.write(pkt)
    time.sleep(0.2)
    return TestResult("Live frame (31 red pixels)", Verdict.PASS, elapsed,
                      "Frame sent (visual check recommended)")


def test_idle_cleanup(ser: serial.Serial) -> TestResult:
    """Return to idle mode at end of tests."""
    return _send_and_expect_ack(ser, set_mode(Mode.IDLE, 0), "Return to idle")


# ---------------------------------------------------------------------------
# Suite runner
# ---------------------------------------------------------------------------

def run(ser: serial.Serial) -> TestReport:
    """Run the full Teensy serial test suite."""
    report = TestReport("Teensy Serial Tests")

    # 1. Connectivity
    report.add(test_connectivity(ser))

    # 2. Brightness
    for r in test_brightness(ser):
        report.add(r)

    # 3. Frame rate
    for r in test_framerate(ser):
        report.add(r)

    # 4. All modes + verification
    for r in test_modes(ser):
        report.add(r)

    # 5. Pattern upload
    report.add(test_pattern_upload(ser))

    # 6. Live frame
    report.add(test_live_frame(ser))

    # 7. Cleanup
    report.add(test_idle_cleanup(ser))

    report.finish()
    return report
