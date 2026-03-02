#!/usr/bin/env python3
"""
detect_errors.py - Build-based error detection for the wireless-pov-poi repository.

Runs:
  1. PlatformIO firmware builds (Teensy 4.1 + ESP32-S3) unless SKIP_PIO=1.
  2. TypeScript type-check (tsc --noEmit) if the webui exists and npm/node available.
  3. Python syntax check (python -m compileall) on examples/ and scripts/.

Produces:
  scripts/ai_agent/output/findings.json   -- machine-readable findings
  scripts/ai_agent/output/report.md       -- human-readable Markdown report

Usage:
    python3 scripts/ai_agent/detect_errors.py
    python3 scripts/ai_agent/detect_errors.py '{"name":"my_run"}'

Environment variables:
    SKIP_PIO=1   Skip PlatformIO builds (default: 0, builds run when pio available).
"""

from __future__ import annotations

import json
import os
import re
import shutil
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
WEBUI_DIR = REPO_ROOT / "esp32_firmware" / "webui"


# ---------------------------------------------------------------------------
# Subprocess helper
# ---------------------------------------------------------------------------

def _run(
    cmd: List[str],
    cwd: Optional[Path] = None,
    timeout: int = 600,
) -> Tuple[int, str, str]:
    """Run a command, return (returncode, stdout, stderr)."""
    result = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    return result.returncode, result.stdout, result.stderr


# ---------------------------------------------------------------------------
# PlatformIO builds
# ---------------------------------------------------------------------------

def _pio_exe() -> Optional[str]:
    return shutil.which("pio")


def _parse_pio_errors(env_name: str, output: str) -> List[Dict]:
    """Extract error/warning findings from PlatformIO build output."""
    findings: List[Dict] = []
    seen: set = set()
    for line in output.splitlines():
        # GCC-style:  path/file.cpp:10:5: error: message
        m = re.match(r"^(.+?):(\d+):\d+:\s*(error|warning):\s*(.+)$", line)
        if m:
            key = (m.group(1), m.group(2), m.group(4))
            if key in seen:
                continue
            seen.add(key)
            findings.append({
                "source": f"pio_build:{env_name}",
                "severity": "error" if m.group(3) == "error" else "warn",
                "file": m.group(1),
                "line_start": int(m.group(2)),
                "description": m.group(4).strip(),
                "suggested_fix": None,
            })
        # PlatformIO error banner:  [ERROR] or *** [.pio/...] Error N
        elif re.search(r"\[ERROR\]|\*\*\* .+ Error \d+", line):
            if line.strip() not in seen:
                seen.add(line.strip())
                findings.append({
                    "source": f"pio_build:{env_name}",
                    "severity": "error",
                    "description": line.strip(),
                    "suggested_fix": None,
                })
    return findings


def _run_pio_builds() -> Tuple[List[Dict], bool]:
    """Run PlatformIO builds. Returns (findings, all_passed)."""
    findings: List[Dict] = []
    pio = _pio_exe()
    if not pio:
        findings.append({
            "source": "pio_build",
            "severity": "info",
            "description": (
                "PlatformIO (pio) not found in PATH — firmware builds skipped. "
                "Install with: pip install platformio"
            ),
            "suggested_fix": "Run: pip install platformio",
        })
        return findings, True  # Not a hard failure if pio is absent

    builds = [
        ("teensy41", REPO_ROOT),
        ("esp32s3", REPO_ROOT / "esp32_firmware"),
    ]

    all_passed = True
    for env_name, cwd in builds:
        if not cwd.exists():
            print(f"  [pio] Skipping {env_name}: directory not found ({cwd})")
            continue
        print(f"  [pio] Building env:{env_name} in {cwd} ...")
        try:
            rc, stdout, stderr = _run([pio, "run", "-e", env_name], cwd=cwd)
        except subprocess.TimeoutExpired:
            findings.append({
                "source": f"pio_build:{env_name}",
                "severity": "error",
                "description": f"PlatformIO build timed out for env:{env_name}.",
                "suggested_fix": "Check for infinite loops or network issues in build.",
            })
            all_passed = False
            continue

        combined = stdout + stderr
        status_icon = "✅" if rc == 0 else "❌"
        print(f"    {status_icon} exit code: {rc}")

        if rc != 0:
            all_passed = False
            parsed = _parse_pio_errors(env_name, combined)
            if not parsed:
                # Fallback: include last 20 lines of output as a single finding
                tail = "\n".join(combined.splitlines()[-20:])
                parsed = [{
                    "source": f"pio_build:{env_name}",
                    "severity": "error",
                    "description": (
                        f"PlatformIO build failed for env:{env_name} "
                        f"(exit code {rc}):\n{tail}"
                    ),
                    "suggested_fix": "Review the full build log for compilation errors.",
                }]
            findings.extend(parsed)
        else:
            findings.append({
                "source": f"pio_build:{env_name}",
                "severity": "info",
                "description": f"PlatformIO build PASSED for env:{env_name}.",
            })

    return findings, all_passed


# ---------------------------------------------------------------------------
# TypeScript check
# ---------------------------------------------------------------------------

def _run_typescript_check() -> List[Dict]:
    """Run tsc --noEmit in the webui directory if available."""
    findings: List[Dict] = []
    if not WEBUI_DIR.exists():
        return findings

    node = shutil.which("node")
    npm = shutil.which("npm")
    if not node or not npm:
        findings.append({
            "source": "typescript_check",
            "severity": "info",
            "description": "node/npm not found — TypeScript check skipped.",
        })
        return findings

    # Install dependencies if node_modules is missing
    if not (WEBUI_DIR / "node_modules").exists():
        print("  [tsc] Installing webui dependencies (npm ci)...")
        rc, stdout, stderr = _run([npm, "ci"], cwd=WEBUI_DIR, timeout=300)
        if rc != 0:
            findings.append({
                "source": "typescript_check",
                "severity": "warn",
                "description": f"npm ci failed (exit {rc}): {stderr.strip()[:200]}",
                "suggested_fix": "Run 'npm ci' in esp32_firmware/webui/ and resolve errors.",
            })
            return findings

    # Run tsc --noEmit
    print("  [tsc] Running tsc --noEmit...")
    npx = shutil.which("npx") or "npx"
    rc, stdout, stderr = _run([npx, "tsc", "--noEmit"], cwd=WEBUI_DIR, timeout=120)
    combined = stdout + stderr
    status_icon = "✅" if rc == 0 else "❌"
    print(f"    {status_icon} tsc exit code: {rc}")

    if rc != 0:
        for line in combined.splitlines():
            # TypeScript error:  file.ts(10,5): error TS1234: message
            m = re.match(r"^(.+?)\((\d+),\d+\):\s*(error|warning)\s+(TS\d+):\s*(.+)$", line)
            if m:
                findings.append({
                    "source": "typescript_check",
                    "severity": "error" if m.group(3) == "error" else "warn",
                    "file": _rel(WEBUI_DIR / m.group(1)),
                    "line_start": int(m.group(2)),
                    "description": f"[{m.group(4)}] {m.group(5).strip()}",
                    "suggested_fix": None,
                })
    else:
        findings.append({
            "source": "typescript_check",
            "severity": "info",
            "description": "TypeScript type-check PASSED (tsc --noEmit).",
        })

    return findings


def _rel(p: Path) -> str:
    try:
        return str(p.relative_to(REPO_ROOT))
    except ValueError:
        return str(p)


# ---------------------------------------------------------------------------
# Python syntax check
# ---------------------------------------------------------------------------

def _run_python_syntax_check() -> List[Dict]:
    """Check Python files for syntax errors using compileall."""
    findings: List[Dict] = []
    py_dirs = [REPO_ROOT / "examples", REPO_ROOT / "scripts"]

    for d in py_dirs:
        if not d.exists():
            continue
        rc, stdout, stderr = _run(
            [sys.executable, "-m", "compileall", "-q", str(d)]
        )
        if rc != 0:
            for line in (stdout + stderr).splitlines():
                if not line.strip():
                    continue
                m = re.search(r'File "(.+?)", line (\d+)', line)
                if m:
                    findings.append({
                        "source": "python_syntax",
                        "severity": "error",
                        "file": _rel(Path(m.group(1))),
                        "line_start": int(m.group(2)),
                        "description": f"Python syntax error: {line.strip()}",
                        "suggested_fix": "Fix the syntax error at the indicated location.",
                    })
                elif "SyntaxError" in line or "Error" in line:
                    findings.append({
                        "source": "python_syntax",
                        "severity": "error",
                        "description": line.strip(),
                        "suggested_fix": "Fix the Python syntax error.",
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
        fh.write("# AI Agent — Build & Error Detection Report\n\n")
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
                    fh.write(f"**File:** `{loc}`  \n")
                fh.write(f"**Severity:** {f.get('severity', 'info')}  \n")
                fh.write(f"**Description:** {f.get('description', '')}  \n")
                if f.get("suggested_fix"):
                    fh.write(
                        f"\n**Suggested Fix:**\n```\n{f['suggested_fix']}\n```\n"
                    )
                fh.write("\n")
        else:
            fh.write("## Findings\n\nNo issues found. ✅\n")
    print(f"  Saved: {md_path}")
    print(f"\n  Summary: {n_errors} errors, {n_warns} warnings, {n_info} info")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    run_name = "detect_errors"
    if len(sys.argv) > 1:
        try:
            args = json.loads(sys.argv[1])
            run_name = args.get("name", run_name)
        except (json.JSONDecodeError, AttributeError, TypeError):
            pass

    skip_pio = os.environ.get("SKIP_PIO", "0").strip() == "1"

    print(f"[detect_errors] Build & error detection (run: {run_name})")
    print(f"  SKIP_PIO={skip_pio}")

    findings: List[Dict] = []
    pio_passed = True

    if not skip_pio:
        print("  Running PlatformIO builds...")
        pio_findings, pio_passed = _run_pio_builds()
        findings.extend(pio_findings)
    else:
        print("  PlatformIO builds SKIPPED (SKIP_PIO=1).")
        findings.append({
            "source": "pio_build",
            "severity": "info",
            "description": "PlatformIO builds were skipped (SKIP_PIO=1).",
        })

    print("  Running TypeScript check...")
    findings.extend(_run_typescript_check())

    print("  Running Python syntax check...")
    findings.extend(_run_python_syntax_check())

    _save_outputs(findings, run_name)

    # Return non-zero exit if PIO builds actually failed
    if not pio_passed:
        print("\n[detect_errors] ❌ One or more builds FAILED.")
        return 1
    py_errors = sum(
        1 for f in findings
        if f.get("severity") == "error" and f.get("source") != "pio_build"
    )
    return 1 if py_errors > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
