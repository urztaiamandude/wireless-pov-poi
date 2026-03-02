"""
ESP32 WiFi REST API tests.

Connects to the ESP32 access point and exercises every HTTP endpoint.
Tests are non-destructive where possible - they read state, set known
values, then restore the original state.
"""

import json
import time
import urllib.parse
import urllib.request
import urllib.error
from typing import Optional

from .result import TestResult, TestReport, Verdict


DEFAULT_BASE_URL = "http://192.168.4.1"
REQUEST_TIMEOUT = 5  # seconds


def _get(url: str, timeout: float = REQUEST_TIMEOUT) -> tuple[int, str]:
    """HTTP GET, returns (status_code, body)."""
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https"):
        raise ValueError(f"Unsafe URL scheme: {parsed.scheme!r}; only http/https are allowed")
    req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")


def _post_json(url: str, data: dict, timeout: float = REQUEST_TIMEOUT) -> tuple[int, str]:
    """HTTP POST with JSON body, returns (status_code, body)."""
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https"):
        raise ValueError(f"Unsafe URL scheme: {parsed.scheme!r}; only http/https are allowed")
    body = json.dumps(data).encode("utf-8")
    req = urllib.request.Request(url, data=body, method="POST")
    req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")


# ---------------------------------------------------------------------------
# Individual tests
# ---------------------------------------------------------------------------

def test_wifi_reachable(base: str) -> TestResult:
    """Check that the ESP32 web server responds at all."""
    start = time.time()
    try:
        code, body = _get(f"{base}/", timeout=5)
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("WiFi reachable", Verdict.PASS, elapsed,
                              f"HTTP 200, {len(body)} bytes")
        return TestResult("WiFi reachable", Verdict.FAIL, elapsed,
                          f"HTTP {code}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("WiFi reachable", Verdict.FAIL, elapsed, str(e))


def test_get_status(base: str) -> tuple[TestResult, Optional[dict]]:
    """GET /api/status and validate response schema."""
    start = time.time()
    try:
        code, body = _get(f"{base}/api/status")
        elapsed = (time.time() - start) * 1000
        if code != 200:
            return TestResult("GET /api/status", Verdict.FAIL, elapsed,
                              f"HTTP {code}"), None

        data = json.loads(body)
        required = {"mode", "brightness", "framerate"}
        missing = required - set(data.keys())
        if missing:
            return TestResult("GET /api/status", Verdict.FAIL, elapsed,
                              f"Missing keys: {missing}",
                              f"Got: {json.dumps(data)}"), None

        msg = f"mode={data['mode']} bright={data['brightness']} fps={data['framerate']}"
        return TestResult("GET /api/status", Verdict.PASS, elapsed, msg), data
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/status", Verdict.FAIL, elapsed, str(e)), None


def test_set_brightness(base: str, value: int = 128) -> TestResult:
    start = time.time()
    try:
        code, body = _post_json(f"{base}/api/brightness", {"brightness": value})
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult(f"POST /api/brightness ({value})", Verdict.PASS, elapsed)
        return TestResult(f"POST /api/brightness ({value})", Verdict.FAIL, elapsed,
                          f"HTTP {code}: {body}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult(f"POST /api/brightness ({value})", Verdict.FAIL, elapsed, str(e))


def test_set_framerate(base: str, value: int = 30) -> TestResult:
    start = time.time()
    try:
        code, body = _post_json(f"{base}/api/framerate", {"framerate": value})
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult(f"POST /api/framerate ({value})", Verdict.PASS, elapsed)
        return TestResult(f"POST /api/framerate ({value})", Verdict.FAIL, elapsed,
                          f"HTTP {code}: {body}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult(f"POST /api/framerate ({value})", Verdict.FAIL, elapsed, str(e))


def test_set_mode(base: str, mode: int, index: int, label: str) -> TestResult:
    start = time.time()
    try:
        code, body = _post_json(f"{base}/api/mode", {"mode": mode, "index": index})
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult(f"POST /api/mode ({label})", Verdict.PASS, elapsed)
        return TestResult(f"POST /api/mode ({label})", Verdict.FAIL, elapsed,
                          f"HTTP {code}: {body}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult(f"POST /api/mode ({label})", Verdict.FAIL, elapsed, str(e))


def test_upload_pattern(base: str) -> TestResult:
    start = time.time()
    payload = {
        "index": 0,
        "type": 0,
        "color1": {"r": 255, "g": 0, "b": 0},
        "color2": {"r": 0, "g": 0, "b": 255},
        "speed": 50,
    }
    try:
        code, body = _post_json(f"{base}/api/pattern", payload)
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("POST /api/pattern", Verdict.PASS, elapsed)
        return TestResult("POST /api/pattern", Verdict.FAIL, elapsed,
                          f"HTTP {code}: {body}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("POST /api/pattern", Verdict.FAIL, elapsed, str(e))


def test_live_mode(base: str) -> TestResult:
    """POST /api/live with 31 green pixels."""
    start = time.time()
    pixels = [{"r": 0, "g": 255, "b": 0}] * 31
    try:
        code, body = _post_json(f"{base}/api/live", {"pixels": pixels})
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("POST /api/live (31 green)", Verdict.PASS, elapsed)
        return TestResult("POST /api/live (31 green)", Verdict.FAIL, elapsed,
                          f"HTTP {code}: {body}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("POST /api/live (31 green)", Verdict.FAIL, elapsed, str(e))


def test_sync_data(base: str) -> TestResult:
    """GET /api/sync/data and verify it returns image/pattern listings."""
    start = time.time()
    try:
        code, body = _get(f"{base}/api/sync/data")
        elapsed = (time.time() - start) * 1000
        if code != 200:
            return TestResult("GET /api/sync/data", Verdict.FAIL, elapsed,
                              f"HTTP {code}")
        data = json.loads(body)
        images = data.get("images", [])
        patterns = data.get("patterns", [])
        if not images:
            return TestResult("GET /api/sync/data", Verdict.WARN, elapsed,
                              "images list is empty")
        if not patterns:
            return TestResult("GET /api/sync/data", Verdict.WARN, elapsed,
                              "patterns list is empty")
        return TestResult("GET /api/sync/data", Verdict.PASS, elapsed,
                          f"{len(images)} images, {len(patterns)} patterns")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/sync/data", Verdict.FAIL, elapsed, str(e))


def test_sync_status(base: str) -> TestResult:
    start = time.time()
    try:
        code, body = _get(f"{base}/api/sync/status")
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("GET /api/sync/status", Verdict.PASS, elapsed)
        return TestResult("GET /api/sync/status", Verdict.FAIL, elapsed,
                          f"HTTP {code}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/sync/status", Verdict.FAIL, elapsed, str(e))


def test_device_config(base: str) -> TestResult:
    start = time.time()
    try:
        code, body = _get(f"{base}/api/device/config")
        elapsed = (time.time() - start) * 1000
        if code != 200:
            return TestResult("GET /api/device/config", Verdict.FAIL, elapsed,
                              f"HTTP {code}")
        data = json.loads(body)
        if "deviceId" not in data and "deviceName" not in data:
            return TestResult("GET /api/device/config", Verdict.WARN, elapsed,
                              f"Missing expected fields: {list(data.keys())}")
        return TestResult("GET /api/device/config", Verdict.PASS, elapsed,
                          f"deviceName={data.get('deviceName', '?')}")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/device/config", Verdict.FAIL, elapsed, str(e))


def test_sd_list(base: str) -> TestResult:
    """GET /api/sd/list - may return empty if no SD card."""
    start = time.time()
    try:
        code, body = _get(f"{base}/api/sd/list")
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("GET /api/sd/list", Verdict.PASS, elapsed,
                              f"Response: {body[:100]}")
        return TestResult("GET /api/sd/list", Verdict.WARN, elapsed,
                          f"HTTP {code} (SD card may not be present)")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/sd/list", Verdict.FAIL, elapsed, str(e))


def test_sd_info(base: str) -> TestResult:
    """GET /api/sd/info - may fail if no SD card."""
    start = time.time()
    try:
        code, body = _get(f"{base}/api/sd/info")
        elapsed = (time.time() - start) * 1000
        if code == 200:
            return TestResult("GET /api/sd/info", Verdict.PASS, elapsed,
                              f"Response: {body[:100]}")
        return TestResult("GET /api/sd/info", Verdict.WARN, elapsed,
                          f"HTTP {code} (SD card may not be present)")
    except Exception as e:
        elapsed = (time.time() - start) * 1000
        return TestResult("GET /api/sd/info", Verdict.FAIL, elapsed, str(e))


# ---------------------------------------------------------------------------
# Suite runner
# ---------------------------------------------------------------------------

def run(base_url: str = DEFAULT_BASE_URL) -> TestReport:
    """Run the full ESP32 REST API test suite."""
    report = TestReport("ESP32 REST API Tests")

    # 1. Reachability
    result = test_wifi_reachable(base_url)
    report.add(result)
    if result.verdict == Verdict.FAIL:
        report.add(TestResult("Remaining API tests", Verdict.SKIP, 0,
                              "Skipped: ESP32 not reachable"))
        report.finish()
        return report

    # 2. Status
    statusResult, status = test_get_status(base_url)
    report.add(statusResult)

    # Save original state for restore
    origBrightness = status.get("brightness", 128) if status else 128
    origFramerate = status.get("framerate", 30) if status else 30

    # 3. Brightness control
    report.add(test_set_brightness(base_url, 64))
    report.add(test_set_brightness(base_url, 255))
    report.add(test_set_brightness(base_url, origBrightness))

    # 4. Frame rate control
    report.add(test_set_framerate(base_url, 60))
    report.add(test_set_framerate(base_url, origFramerate))

    # 5. Mode switching
    modes = [
        (0, 0, "Idle"),
        (1, 0, "Image 0"),
        (2, 0, "Pattern 0 (Rainbow)"),
        (2, 4, "Pattern 4 (Fire)"),
        (2, 10, "Pattern 10 (Plasma)"),
        (3, 0, "Sequence 0"),
    ]
    for m, idx, label in modes:
        report.add(test_set_mode(base_url, m, idx, label))
        time.sleep(0.2)

    # 6. Pattern upload
    report.add(test_upload_pattern(base_url))

    # 7. Live mode
    report.add(test_set_mode(base_url, 4, 0, "Live mode"))
    report.add(test_live_mode(base_url))

    # 8. Sync endpoints
    report.add(test_sync_data(base_url))
    report.add(test_sync_status(base_url))

    # 9. Device config
    report.add(test_device_config(base_url))

    # 10. SD card endpoints
    report.add(test_sd_list(base_url))
    report.add(test_sd_info(base_url))

    # 11. Cleanup - return to idle
    report.add(test_set_mode(base_url, 0, 0, "Idle (cleanup)"))

    report.finish()
    return report
