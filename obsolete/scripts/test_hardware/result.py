"""
Test result tracking for hardware tests.
"""

import json
import time
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Optional


class Verdict(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    SKIP = "SKIP"
    WARN = "WARN"


@dataclass
class TestResult:
    name: str
    verdict: Verdict
    duration_ms: float = 0.0
    message: str = ""
    details: str = ""


@dataclass
class TestReport:
    suite_name: str
    results: list[TestResult] = field(default_factory=list)
    start_time: float = field(default_factory=time.time)
    end_time: float = 0.0

    def add(self, result: TestResult):
        self.results.append(result)

    @property
    def passed(self) -> int:
        return sum(1 for r in self.results if r.verdict == Verdict.PASS)

    @property
    def failed(self) -> int:
        return sum(1 for r in self.results if r.verdict == Verdict.FAIL)

    @property
    def skipped(self) -> int:
        return sum(1 for r in self.results if r.verdict == Verdict.SKIP)

    @property
    def warned(self) -> int:
        return sum(1 for r in self.results if r.verdict == Verdict.WARN)

    @property
    def total(self) -> int:
        return len(self.results)

    @property
    def all_passed(self) -> bool:
        return self.failed == 0

    def finish(self):
        self.end_time = time.time()

    def summary_line(self) -> str:
        elapsed = self.end_time - self.start_time if self.end_time else time.time() - self.start_time
        return (
            f"{self.suite_name}: {self.passed}/{self.total} passed, "
            f"{self.failed} failed, {self.skipped} skipped, "
            f"{self.warned} warnings  ({elapsed:.1f}s)"
        )

    def print_report(self):
        """Print a human-readable test report to stdout."""
        width = 72
        print()
        print("=" * width)
        print(f"  {self.suite_name}")
        print("=" * width)
        for r in self.results:
            icon = {
                Verdict.PASS: "+",
                Verdict.FAIL: "X",
                Verdict.SKIP: "-",
                Verdict.WARN: "!",
            }[r.verdict]
            line = f"  [{icon}] {r.name}"
            if r.duration_ms:
                line += f"  ({r.duration_ms:.0f}ms)"
            print(line)
            if r.message:
                print(f"      {r.message}")
            if r.details and r.verdict in (Verdict.FAIL, Verdict.WARN):
                for detail_line in r.details.strip().split("\n"):
                    print(f"      | {detail_line}")
        print("-" * width)
        print(f"  {self.summary_line()}")
        print("=" * width)

    def to_dict(self) -> dict:
        return {
            "suite": self.suite_name,
            "start": self.start_time,
            "end": self.end_time,
            "summary": {
                "total": self.total,
                "passed": self.passed,
                "failed": self.failed,
                "skipped": self.skipped,
                "warnings": self.warned,
            },
            "results": [
                {
                    "name": r.name,
                    "verdict": r.verdict.value,
                    "duration_ms": r.duration_ms,
                    "message": r.message,
                    "details": r.details,
                }
                for r in self.results
            ],
        }

    def save_json(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w") as f:
            json.dump(self.to_dict(), f, indent=2)
