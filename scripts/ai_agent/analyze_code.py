#!/usr/bin/env python3
"""
analyze_code.py - Static code analysis for the wireless-pov-poi repository.

Runs heuristic checks on C++ firmware files and a Python syntax check on all
Python sources. Does NOT run any build tools (see detect_errors.py for that).

Produces:
  scripts/ai_agent/output/findings.json   -- machine-readable findings
  scripts/ai_agent/output/report.md       -- human-readable Markdown report

Usage:
    python3 scripts/ai_agent/analyze_code.py
    python3 scripts/ai_agent/analyze_code.py '{"name":"my_run"}'
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Path constants
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
OUTPUT_DIR = SCRIPT_DIR / "output"

_EXCLUDE_DIRS: Tuple[str, ...] = (
    ".pio", ".git", "node_modules", "__pycache__",
    "build_output", "dist", ".venv", "build",
)


# ---------------------------------------------------------------------------
# Filesystem helpers
# ---------------------------------------------------------------------------

def _is_excluded(path: Path) -> bool:
    return any(part in _EXCLUDE_DIRS for part in path.parts)


def _find_files(root: Path, extensions: Tuple[str, ...]) -> List[Path]:
    results: List[Path] = []
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if _is_excluded(p):
            continue
        if p.suffix in extensions:
            results.append(p)
    return sorted(results)


def _rel(p: Path) -> str:
    try:
        return str(p.relative_to(REPO_ROOT))
    except ValueError:
        return str(p)


# ---------------------------------------------------------------------------
# C++ heuristic checks
# ---------------------------------------------------------------------------

def _check_cpp_file(path: Path) -> List[Dict]:
    findings: List[Dict] = []
    try:
        content = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return findings

    lines = content.splitlines()
    for lineno, line in enumerate(lines, 1):
        # ── Off-by-one: loop uses <= NUM_LEDS / <= DISPLAY_LEDS ────────────
        if re.search(r"for\s*\([^;]+;\s*\w+\s*<=\s*(NUM_LEDS|DISPLAY_LEDS)\s*;", line):
            findings.append({
                "source": "analyze_code:led_loop_bounds",
                "severity": "warn",
                "file": _rel(path),
                "line_start": lineno,
                "line_end": lineno,
                "description": (
                    f"Potential off-by-one error: loop condition uses '<= NUM_LEDS' or "
                    f"'<= DISPLAY_LEDS'. This would access leds[NUM_LEDS] which is out of "
                    f"bounds (valid indices are 0 – NUM_LEDS-1). "
                    f"Use '< NUM_LEDS' instead."
                ),
                "suggested_fix": "Change '<= NUM_LEDS' / '<= DISPLAY_LEDS' to '< NUM_LEDS' / '< DISPLAY_LEDS'.",
            })

        # ── Hardcoded out-of-bounds access: leds[32] ───────────────────────
        if re.search(r"\bleds\s*\[\s*32\s*\]", line):
            findings.append({
                "source": "analyze_code:led_hardcoded_oob",
                "severity": "warn",
                "file": _rel(path),
                "line_start": lineno,
                "line_end": lineno,
                "description": (
                    "Hardcoded leds[32] accesses index 32, which is out of bounds "
                    "for a strip with NUM_LEDS=32 (valid indices 0 – NUM_LEDS-1). "
                    "Use leds[NUM_LEDS - 1] instead of a magic number."
                ),
                "suggested_fix": "Replace leds[32] with leds[NUM_LEDS - 1].",
            })

        # ── delay() in loops — may cause LED stutter ───────────────────────
        # Heuristic: flag delay() calls that appear *inside* a function body
        # (not in a comment) without FastLED.show() nearby. Keep threshold low
        # to avoid false positives; mark as info rather than warn.
        stripped = line.strip()
        if (
            re.search(r"\bdelay\s*\(\s*\d{4,}\s*\)", line)
            and not stripped.startswith("//")
            and not stripped.startswith("*")
        ):
            findings.append({
                "source": "analyze_code:large_delay",
                "severity": "info",
                "file": _rel(path),
                "line_start": lineno,
                "line_end": lineno,
                "description": (
                    "Large delay() call (≥ 1000 ms) detected. In a real-time LED "
                    "POV loop this blocks all updates and may cause visible flicker. "
                    "Consider using non-blocking timing with millis()."
                ),
                "suggested_fix": (
                    "Replace delay(N) with a millis()-based timer:\n"
                    "  unsigned long t = millis();\n"
                    "  if (millis() - t >= N) { /* your code */ t = millis(); }"
                ),
            })

    return findings


def _run_cpp_checks() -> List[Dict]:
    findings: List[Dict] = []
    cpp_dirs = [REPO_ROOT / "teensy_firmware", REPO_ROOT / "esp32_firmware"]
    for d in cpp_dirs:
        if not d.exists():
            continue
        for f in _find_files(d, (".ino", ".cpp", ".h")):
            findings.extend(_check_cpp_file(f))
    return findings


# ---------------------------------------------------------------------------
# Python syntax check
# ---------------------------------------------------------------------------

def _run_python_syntax_check() -> List[Dict]:
    """Use compileall to check Python files for syntax errors."""
    findings: List[Dict] = []
    py_dirs = [REPO_ROOT / "examples", REPO_ROOT / "scripts"]

    for d in py_dirs:
        if not d.exists():
            continue
        result = subprocess.run(
            [sys.executable, "-m", "compileall", "-q", str(d)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            combined = (result.stdout + result.stderr).splitlines()
            for line in combined:
                if not line.strip():
                    continue
                # compileall typically prints:  Compiling <file> ...
                # and on error:  File "<file>", line N
                m = re.search(r'File "(.+?)", line (\d+)', line)
                if m:
                    findings.append({
                        "source": "analyze_code:python_syntax",
                        "severity": "error",
                        "file": _rel(Path(m.group(1))),
                        "line_start": int(m.group(2)),
                        "description": f"Python syntax error: {line.strip()}",
                        "suggested_fix": "Fix the syntax error in the indicated file/line.",
                    })
                elif "SyntaxError" in line or "Error" in line:
                    findings.append({
                        "source": "analyze_code:python_syntax",
                        "severity": "error",
                        "file": str(d),
                        "description": line.strip(),
                        "suggested_fix": "Fix the syntax error in the Python file.",
                    })
    return findings


# ---------------------------------------------------------------------------
# Optional ruff linting
# ---------------------------------------------------------------------------

def _run_ruff() -> List[Dict]:
    """Run ruff if installed, return findings (never fail if ruff absent)."""
    import shutil
    if not shutil.which("ruff"):
        return []
    findings: List[Dict] = []
    result = subprocess.run(
        ["ruff", "check", "--output-format=json",
         str(REPO_ROOT / "examples"), str(REPO_ROOT / "scripts")],
        capture_output=True,
        text=True,
    )
    try:
        items = json.loads(result.stdout)
    except json.JSONDecodeError:
        return []
    for item in items:
        findings.append({
            "source": "analyze_code:ruff",
            "severity": "warn" if item.get("fix") else "info",
            "file": _rel(Path(item.get("filename", ""))),
            "line_start": item.get("location", {}).get("row"),
            "description": f"[{item.get('code','')}] {item.get('message','')}",
            "suggested_fix": (
                item.get("fix", {}).get("message") if item.get("fix") else None
            ),
        })
    return findings


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def _save_outputs(findings: List[Dict], run_name: str) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    ts = datetime.now(timezone.utc).isoformat()
    n_errors = sum(1 for f in findings if f.get("severity") == "error")
    n_warns = sum(1 for f in findings if f.get("severity") == "warn")
    n_info = sum(1 for f in findings if f.get("severity") == "info")
    summary = {"errors": n_errors, "warnings": n_warns, "info": n_info}

    # Assign stable IDs
    for idx, finding in enumerate(findings):
        if "id" not in finding:
            finding["id"] = f"{finding.get('source', 'unknown')}-{idx:04d}"

    data = {
        "timestamp": ts,
        "run_name": run_name,
        "summary": summary,
        "findings": findings,
    }

    json_path = OUTPUT_DIR / "findings.json"
    with open(json_path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2)
    print(f"  Saved: {json_path}")

    md_path = OUTPUT_DIR / "report.md"
    with open(md_path, "w", encoding="utf-8") as fh:
        fh.write("# AI Agent — Static Code Analysis Report\n\n")
        fh.write(f"**Run:** `{run_name}`  \n**Timestamp:** {ts}  \n\n")
        fh.write("## Summary\n\n")
        fh.write("| Severity | Count |\n|---|---|\n")
        fh.write(f"| 🔴 Errors | {n_errors} |\n")
        fh.write(f"| ⚠️  Warnings | {n_warns} |\n")
        fh.write(f"| ℹ️  Info | {n_info} |\n\n")
        if findings:
            fh.write("## Findings\n\n")
            for f in findings:
                icon = {"error": "🔴", "warn": "⚠️", "info": "ℹ️"}.get(
                    f.get("severity", "info"), "ℹ️"
                )
                fh.write(f"### {icon} {f.get('id', 'n/a')}\n\n")
                if f.get("file"):
                    loc = f["file"]
                    if f.get("line_start"):
                        loc += f":{f['line_start']}"
                        if f.get("line_end") and f["line_end"] != f["line_start"]:
                            loc += f"-{f['line_end']}"
                    fh.write(f"**File:** `{loc}`  \n")
                fh.write(f"**Severity:** {f.get('severity', 'info')}  \n")
                fh.write(f"**Description:** {f.get('description', '')}  \n")
                if f.get("suggested_fix"):
                    fh.write(f"\n**Suggested Fix:**\n```\n{f['suggested_fix']}\n```\n")
                fh.write("\n")
        else:
            fh.write("## Findings\n\nNo issues found. ✅\n")
    print(f"  Saved: {md_path}")
    print(
        f"\n  Summary: {n_errors} errors, {n_warns} warnings, {n_info} info"
    )


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    run_name = "analyze_code"
    if len(sys.argv) > 1:
        try:
            args = json.loads(sys.argv[1])
            run_name = args.get("name", run_name)
        except (json.JSONDecodeError, AttributeError, TypeError):
            pass

    print(f"[analyze_code] Static analysis (run: {run_name})")

    findings: List[Dict] = []
    print("  Checking C++ firmware heuristics...")
    findings.extend(_run_cpp_checks())
    print("  Checking Python syntax...")
    findings.extend(_run_python_syntax_check())
    print("  Checking for ruff (optional)...")
    findings.extend(_run_ruff())

    _save_outputs(findings, run_name)

    # analyze_code does not fail the workflow even if warnings exist;
    # it is a reporting tool. Errors in Python syntax are the exception.
    errors = sum(1 for f in findings if f.get("severity") == "error")
    return 1 if errors > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
