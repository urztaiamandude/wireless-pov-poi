#!/usr/bin/env python3
"""
Post-build/flash hardware test runner for Wireless POV Poi.

Usage
-----
  # Run all tests (auto-detect serial ports)
  python -m scripts.test_hardware.run_tests

  # Teensy serial tests only
  python -m scripts.test_hardware.run_tests --suite teensy --teensy-port COM3

  # ESP32 API tests only (must be connected to POV-POI-WiFi)
  python -m scripts.test_hardware.run_tests --suite api

  # Integration tests (needs both Teensy serial AND ESP32 WiFi)
  python -m scripts.test_hardware.run_tests --suite integration

  # Full pipeline: build, flash, then test
  python -m scripts.test_hardware.run_tests --build --flash

  # Save JSON report
  python -m scripts.test_hardware.run_tests --report results.json
"""

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

import serial
import serial.tools.list_ports

# Ensure project root is importable
PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from scripts.test_hardware.result import TestReport, TestResult, Verdict
from scripts.test_hardware import test_teensy_serial
from scripts.test_hardware import test_esp32_api
from scripts.test_hardware import test_integration


# ---------------------------------------------------------------------------
# Port detection
# ---------------------------------------------------------------------------

TEENSY_VID_PID = [
    (0x16C0, 0x0483),  # Teensy USB Serial
    (0x16C0, 0x0486),  # Teensy RawHID
]

ESP32_VID_PID = [
    (0x10C4, 0xEA60),  # CP210x (common ESP32 USB-UART)
    (0x1A86, 0x7523),  # CH340
    (0x1A86, 0x55D4),  # CH9102
    (0x303A, 0x1001),  # ESP32-S3 native USB
]

BAUD = 115200


def find_port(vid_pid_list: list[tuple[int, int]], label: str) -> Optional[str]:
    """Auto-detect a serial port matching known VID:PID pairs."""
    for port_info in serial.tools.list_ports.comports():
        if port_info.vid is not None and port_info.pid is not None:
            for vid, pid in vid_pid_list:
                if port_info.vid == vid and port_info.pid == pid:
                    return port_info.device
    return None


def list_serial_ports():
    """Print all visible serial ports for diagnostics."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("  (no serial ports found)")
        return
    for p in ports:
        vid = f"{p.vid:04X}" if p.vid else "????"
        pid = f"{p.pid:04X}" if p.pid else "????"
        print(f"  {p.device:20s}  {vid}:{pid}  {p.description}")


def open_serial(port: str) -> serial.Serial:
    ser = serial.Serial(port, BAUD, timeout=0.5)
    time.sleep(2)  # Wait for device reset after connection
    ser.reset_input_buffer()
    return ser


# ---------------------------------------------------------------------------
# Build / Flash helpers
# ---------------------------------------------------------------------------

def run_cmd(label: str, cmd: list[str], cwd: Optional[str] = None) -> TestResult:
    """Run a shell command, return a TestResult."""
    start = time.time()
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True,
            timeout=300, cwd=cwd or str(PROJECT_ROOT),
        )
        elapsed = (time.time() - start) * 1000
        if result.returncode == 0:
            return TestResult(label, Verdict.PASS, elapsed)
        return TestResult(label, Verdict.FAIL, elapsed,
                          f"Exit code {result.returncode}",
                          result.stderr[-500:] if result.stderr else "")
    except FileNotFoundError:
        elapsed = (time.time() - start) * 1000
        return TestResult(label, Verdict.FAIL, elapsed,
                          f"Command not found: {cmd[0]}")
    except subprocess.TimeoutExpired:
        elapsed = (time.time() - start) * 1000
        return TestResult(label, Verdict.FAIL, elapsed, "Timed out (300s)")


def build_firmware() -> TestReport:
    report = TestReport("Build")
    report.add(run_cmd("Build Teensy firmware", ["pio", "run", "-e", "teensy41"]))
    report.add(run_cmd("Build ESP32 firmware", ["pio", "run", "-e", "esp32"],
                       cwd=str(PROJECT_ROOT / "esp32_firmware")))
    report.finish()
    return report


def flash_firmware(teensy_port: Optional[str], esp32_port: Optional[str]) -> TestReport:
    report = TestReport("Flash")
    if teensy_port:
        report.add(run_cmd("Flash Teensy", [
            "pio", "run", "-e", "teensy41", "-t", "upload",
            "--upload-port", teensy_port,
        ]))
    else:
        report.add(TestResult("Flash Teensy", Verdict.SKIP, 0, "No port specified"))

    if esp32_port:
        report.add(run_cmd("Flash ESP32", [
            "pio", "run", "-e", "esp32", "-t", "upload",
            "--upload-port", esp32_port,
        ], cwd=str(PROJECT_ROOT / "esp32_firmware")))
    else:
        report.add(TestResult("Flash ESP32", Verdict.SKIP, 0, "No port specified"))

    report.finish()
    return report


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Post-build/flash hardware test runner for Wireless POV Poi",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--suite", choices=["all", "teensy", "api", "integration"],
        default="all", help="Which test suite to run (default: all)",
    )
    parser.add_argument("--teensy-port", help="Teensy serial port (auto-detected if omitted)")
    parser.add_argument("--esp32-port", help="ESP32 serial port (auto-detected if omitted)")
    parser.add_argument("--esp32-url", default="http://192.168.4.1",
                        help="ESP32 web server URL (default: http://192.168.4.1)")
    parser.add_argument("--build", action="store_true", help="Build firmware before testing")
    parser.add_argument("--flash", action="store_true", help="Flash firmware before testing")
    parser.add_argument("--report", help="Save JSON report to this path")
    parser.add_argument("--list-ports", action="store_true", help="List serial ports and exit")

    args = parser.parse_args()

    if args.list_ports:
        print("Available serial ports:")
        list_serial_ports()
        return

    all_reports: list[TestReport] = []

    # ── Port detection ──────────────────────────────────────────────
    teensy_port = args.teensy_port
    esp32_port = args.esp32_port

    if not teensy_port:
        teensy_port = find_port(TEENSY_VID_PID, "Teensy")
    if not esp32_port:
        esp32_port = find_port(ESP32_VID_PID, "ESP32")

    print("=" * 72)
    print("  Wireless POV Poi - Hardware Test Runner")
    print("=" * 72)
    print()
    print("Available serial ports:")
    list_serial_ports()
    print()
    print(f"  Teensy port: {teensy_port or '(not found)'}")
    print(f"  ESP32 port:  {esp32_port or '(not found)'}")
    print(f"  ESP32 URL:   {args.esp32_url}")
    print(f"  Suite:       {args.suite}")
    print()

    # ── Build ───────────────────────────────────────────────────────
    if args.build:
        print("Building firmware...")
        build_report = build_firmware()
        build_report.print_report()
        all_reports.append(build_report)
        if not build_report.all_passed:
            print("\nBuild failed. Aborting.")
            _save_and_exit(all_reports, args.report, 1)

    # ── Flash ───────────────────────────────────────────────────────
    if args.flash:
        print("Flashing firmware...")
        flash_report = flash_firmware(teensy_port, esp32_port)
        flash_report.print_report()
        all_reports.append(flash_report)
        if not flash_report.all_passed:
            non_skipped = [r for r in flash_report.results if r.verdict != Verdict.SKIP]
            if any(r.verdict == Verdict.FAIL for r in non_skipped):
                print("\nFlash failed. Aborting.")
                _save_and_exit(all_reports, args.report, 1)
        print("\nWaiting 5s for devices to boot...")
        time.sleep(5)

    # ── Teensy serial tests ─────────────────────────────────────────
    teensy_ser = None
    if args.suite in ("all", "teensy", "integration") and teensy_port:
        try:
            print(f"Opening Teensy serial on {teensy_port}...")
            teensy_ser = open_serial(teensy_port)
        except serial.SerialException as e:
            print(f"  Could not open {teensy_port}: {e}")
            all_reports.append(_skip_report("Teensy Serial Tests",
                                            f"Port unavailable: {e}"))

    if args.suite in ("all", "teensy") and teensy_ser:
        report = test_teensy_serial.run(teensy_ser)
        report.print_report()
        all_reports.append(report)
    elif args.suite in ("all", "teensy") and not teensy_port:
        report = _skip_report("Teensy Serial Tests", "No Teensy port detected")
        report.print_report()
        all_reports.append(report)

    # ── ESP32 API tests ─────────────────────────────────────────────
    if args.suite in ("all", "api"):
        report = test_esp32_api.run(args.esp32_url)
        report.print_report()
        all_reports.append(report)

    # ── Integration tests ───────────────────────────────────────────
    if args.suite in ("all", "integration"):
        if teensy_ser:
            report = test_integration.run(teensy_ser, args.esp32_url)
            report.print_report()
            all_reports.append(report)
        else:
            report = _skip_report("Integration Tests",
                                  "Requires both Teensy serial and ESP32 WiFi")
            report.print_report()
            all_reports.append(report)

    # ── Cleanup ─────────────────────────────────────────────────────
    if teensy_ser:
        teensy_ser.close()

    # ── Summary ─────────────────────────────────────────────────────
    print()
    print("=" * 72)
    print("  OVERALL SUMMARY")
    print("=" * 72)
    total_pass = sum(r.passed for r in all_reports)
    total_fail = sum(r.failed for r in all_reports)
    total_skip = sum(r.skipped for r in all_reports)
    total_warn = sum(r.warned for r in all_reports)
    total_all = sum(r.total for r in all_reports)
    for r in all_reports:
        print(f"  {r.summary_line()}")
    print("-" * 72)
    print(f"  TOTAL: {total_pass}/{total_all} passed, "
          f"{total_fail} failed, {total_skip} skipped, {total_warn} warnings")
    if total_fail == 0:
        print("\n  ALL TESTS PASSED")
    else:
        print(f"\n  {total_fail} TEST(S) FAILED")
    print("=" * 72)

    exit_code = 0 if total_fail == 0 else 1
    _save_and_exit(all_reports, args.report, exit_code)


def _skip_report(name: str, reason: str) -> TestReport:
    report = TestReport(name)
    report.add(TestResult(name, Verdict.SKIP, 0, reason))
    report.finish()
    return report


def _save_and_exit(reports: list[TestReport], report_path: Optional[str], exit_code: int):
    if report_path:
        combined = {
            "timestamp": time.time(),
            "suites": [r.to_dict() for r in reports],
        }
        path = Path(report_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w") as f:
            json.dump(combined, f, indent=2)
        print(f"\nJSON report saved to: {path}")
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
