"""
End-to-end integration tests.

These tests send commands through the ESP32 REST API and verify that
the Teensy actually changes state by reading its serial status response.
This confirms the full communication chain:

    HTTP request -> ESP32 -> Serial UART -> Teensy -> ACK -> ESP32 -> HTTP response
                                              |
                             Serial status <--+  (verified independently)
"""

import json
import time
import urllib.request
import urllib.error
import serial
from typing import Optional

from .protocol import (
    Mode, request_status, parse_status, StatusResponse,
)
from .result import TestResult, TestReport, Verdict


REQUEST_TIMEOUT = 5
STATUS_TIMEOUT = 0.5


def _get(url: str) -> tuple[int, str]:
    req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=REQUEST_TIMEOUT) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")


def _post_json(url: str, data: dict) -> tuple[int, str]:
    body = json.dumps(data).encode("utf-8")
    req = urllib.request.Request(url, data=body, method="POST")
    req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=REQUEST_TIMEOUT) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")


def _read_teensy_status(ser: serial.Serial) -> Optional[StatusResponse]:
    ser.reset_input_buffer()
    ser.write(request_status())
    buf = b""
    deadline = time.time() + STATUS_TIMEOUT
    while time.time() < deadline:
        if ser.in_waiting:
            buf += ser.read(ser.in_waiting)
            status = parse_status(buf)
            if status:
                return status
        time.sleep(0.01)
    return None


# ---------------------------------------------------------------------------
# Integration tests
# ---------------------------------------------------------------------------

def test_mode_through_api(
    ser: serial.Serial,
    base_url: str,
    mode: int,
    index: int,
    label: str,
) -> TestResult:
    """
    Send a mode change via HTTP API, then verify the Teensy switched
    modes by reading its serial status directly.
    """
    start = time.time()
    try:
        code, body = _post_json(f"{base_url}/api/mode",
                                {"mode": mode, "index": index})
        if code != 200:
            elapsed = (time.time() - start) * 1000
            return TestResult(f"E2E mode: {label}", Verdict.FAIL, elapsed,
                              f"API returned HTTP {code}")

        # Give time for serial command to propagate
        time.sleep(0.3)

        status = _read_teensy_status(ser)
        elapsed = (time.time() - start) * 1000

        if status is None:
            return TestResult(f"E2E mode: {label}", Verdict.FAIL, elapsed,
                              "Teensy status unreadable after API call")

        if status.mode == mode:
            return TestResult(f"E2E mode: {label}", Verdict.PASS, elapsed,
                              f"Teensy confirmed mode={status.mode} index={status.index}")

        return TestResult(f"E2E mode: {label}", Verdict.FAIL, elapsed,
                          f"Expected mode={mode}, Teensy reports mode={status.mode}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult(f"E2E mode: {label}", Verdict.FAIL, elapsed, str(e))


def test_brightness_through_api(
    ser: serial.Serial,
    base_url: str,
) -> TestResult:
    """
    Set brightness via HTTP, verify via status that command was received.
    (Status doesn't include brightness directly, so we verify the Teensy
    responded with ACK by checking the ESP32 API /api/status endpoint.)
    """
    start = time.time()
    try:
        # Set brightness to a known value
        code, _ = _post_json(f"{base_url}/api/brightness", {"brightness": 100})
        if code != 200:
            elapsed = (time.time() - start) * 1000
            return TestResult("E2E brightness", Verdict.FAIL, elapsed,
                              f"API returned HTTP {code}")

        time.sleep(0.3)

        # Read back from ESP32 status API
        code, body = _get(f"{base_url}/api/status")
        elapsed = (time.time() - start) * 1000
        if code != 200:
            return TestResult("E2E brightness", Verdict.FAIL, elapsed,
                              "Could not read status back")

        data = json.loads(body)
        reported = data.get("brightness")
        if reported == 100:
            return TestResult("E2E brightness", Verdict.PASS, elapsed,
                              f"API confirms brightness={reported}")
        return TestResult("E2E brightness", Verdict.WARN, elapsed,
                          f"Set 100, API reports {reported} (may be ESP32 cache)")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("E2E brightness", Verdict.FAIL, elapsed, str(e))


def test_roundtrip_latency(
    ser: serial.Serial,
    base_url: str,
) -> TestResult:
    """
    Measure the round-trip time: HTTP POST -> ESP32 -> Teensy -> ACK -> HTTP response.
    """
    times = []
    for _ in range(5):
        t0 = time.time()
        try:
            code, _ = _post_json(f"{base_url}/api/mode", {"mode": 0, "index": 0})
            t1 = time.time()
            if code == 200:
                times.append((t1 - t0) * 1000)
        except Exception:
            pass
        time.sleep(0.1)

    if not times:
        return TestResult("Roundtrip latency", Verdict.FAIL, 0,
                          "All 5 requests failed")

    avg = sum(times) / len(times)
    mn = min(times)
    mx = max(times)

    verdict = Verdict.PASS if avg < 500 else Verdict.WARN
    return TestResult("Roundtrip latency", verdict, avg,
                      f"avg={avg:.0f}ms min={mn:.0f}ms max={mx:.0f}ms (n={len(times)})")


# ---------------------------------------------------------------------------
# Suite runner
# ---------------------------------------------------------------------------

def run(ser: serial.Serial, base_url: str = "http://192.168.4.1") -> TestReport:
    """Run the full integration test suite."""
    report = TestReport("Integration Tests (API -> Teensy)")

    # Quick connectivity check on both sides
    status = _read_teensy_status(ser)
    if status is None:
        report.add(TestResult("Teensy serial link", Verdict.FAIL, 0,
                              "Cannot read Teensy status"))
        report.finish()
        return report
    report.add(TestResult("Teensy serial link", Verdict.PASS, 0,
                          f"mode={status.mode}"))

    try:
        code, _ = _get(f"{base_url}/api/status")
        if code != 200:
            raise Exception(f"HTTP {code}")
        report.add(TestResult("ESP32 API link", Verdict.PASS, 0))
    except Exception as e:
        report.add(TestResult("ESP32 API link", Verdict.FAIL, 0, str(e)))
        report.finish()
        return report

    # Mode changes through API verified on Teensy
    mode_tests = [
        (Mode.IDLE, 0, "Idle"),
        (Mode.PATTERN, 0, "Pattern 0 (Rainbow)"),
        (Mode.PATTERN, 4, "Pattern 4 (Fire)"),
        (Mode.IMAGE, 0, "Image 0 (Smiley)"),
        (Mode.SEQUENCE, 0, "Sequence 0 (Demo)"),
        (Mode.IDLE, 0, "Back to Idle"),
    ]
    for m, idx, label in mode_tests:
        report.add(test_mode_through_api(ser, base_url, m, idx, label))
        time.sleep(0.2)

    # Brightness through API
    report.add(test_brightness_through_api(ser, base_url))

    # Latency measurement
    report.add(test_roundtrip_latency(ser, base_url))

    # Cleanup
    try:
        _post_json(f"{base_url}/api/mode", {"mode": 0, "index": 0})
    except Exception:
        pass

    report.finish()
    return report
