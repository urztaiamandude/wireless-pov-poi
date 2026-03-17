#!/usr/bin/env python3
"""
agent_dispatcher.py - Main AI agent orchestrator for wireless-pov-poi.

Runs all analysis and build-check sub-agents in sequence and produces a
consolidated findings report.

Sub-agents executed:
  1. analyze_code.py  — static heuristic analysis of firmware and Python sources
  2. detect_errors.py — PlatformIO firmware builds + TypeScript + Python syntax

Produces:
  scripts/ai_agent/output/findings.json   -- merged machine-readable findings
  scripts/ai_agent/output/report.md       -- merged human-readable Markdown report

Usage:
    python3 scripts/ai_agent/agent_dispatcher.py
    python3 scripts/ai_agent/agent_dispatcher.py '{"name":"my_run"}'

Environment variables:
    SKIP_PIO=1   Passed through to detect_errors.py to skip firmware builds.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Path constants
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
OUTPUT_DIR = SCRIPT_DIR / "output"

_AGENT_SCRIPTS = [
    SCRIPT_DIR / "analyze_code.py",
    SCRIPT_DIR / "detect_errors.py",
]

_SUB_OUTPUT_NAMES = [
    "analyze_findings.json",
    "detect_findings.json",
]


# ---------------------------------------------------------------------------
# Run a sub-agent script and collect its output JSON
# ---------------------------------------------------------------------------

def _run_agent(
    script: Path, run_name: str, tmp_output: Path
) -> Optional[Dict]:
    """
    Run a sub-agent script with a custom run_name that writes its findings.json
    to a separate filename so the dispatcher can merge them.
    """
    if not script.exists():
        print(f"  [dispatcher] WARNING: script not found: {script}")
        return None

    env = os.environ.copy()
    # Redirect the sub-agent's output to a temporary file name via an env var.
    # Since the scripts write to OUTPUT_DIR/findings.json, we rename afterwards.
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    arg = json.dumps({"name": run_name})
    print(f"\n  [dispatcher] Running: {script.name} ...")
    result = subprocess.run(
        [sys.executable, str(script), arg],
        env=env,
        text=True,
    )
    rc = result.returncode
    icon = "✅" if rc == 0 else "❌"
    print(f"  [dispatcher] {icon} {script.name} exited with code {rc}")

    # Read the findings.json the sub-agent just wrote
    findings_path = OUTPUT_DIR / "findings.json"
    if findings_path.exists():
        try:
            data = json.loads(findings_path.read_text(encoding="utf-8"))
            # Move to the per-agent temp name
            findings_path.rename(tmp_output)
            return data
        except (json.JSONDecodeError, OSError):
            pass
    return None


# ---------------------------------------------------------------------------
# Merge findings from multiple sub-agents
# ---------------------------------------------------------------------------

def _merge(run_name: str, results: List[Optional[Dict]]) -> Dict:
    """Merge multiple findings dicts from sub-agents into one."""
    all_findings: List[Dict] = []
    for r in results:
        if r and isinstance(r.get("findings"), list):
            all_findings.extend(r["findings"])

    # Re-assign stable IDs in the merged list
    for idx, finding in enumerate(all_findings):
        finding["id"] = f"{finding.get('source', 'unknown')}-{idx:04d}"

    ts = datetime.now(timezone.utc).isoformat()
    n_errors = sum(1 for f in all_findings if f.get("severity") == "error")
    n_warns = sum(1 for f in all_findings if f.get("severity") == "warn")
    n_info = sum(1 for f in all_findings if f.get("severity") == "info")

    return {
        "timestamp": ts,
        "run_name": run_name,
        "summary": {"errors": n_errors, "warnings": n_warns, "info": n_info},
        "findings": all_findings,
    }


# ---------------------------------------------------------------------------
# Save consolidated output
# ---------------------------------------------------------------------------

def _save_outputs(data: Dict) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    run_name = data.get("run_name", "dispatcher")
    ts = data.get("timestamp", "")
    summary = data.get("summary", {})
    findings = data.get("findings", [])
    n_errors = summary.get("errors", 0)
    n_warns = summary.get("warnings", 0)
    n_info = summary.get("info", 0)

    json_path = OUTPUT_DIR / "findings.json"
    with open(json_path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2)
    print(f"\n  Saved: {json_path}")

    md_path = OUTPUT_DIR / "report.md"
    with open(md_path, "w", encoding="utf-8") as fh:
        fh.write("# AI Agent — Consolidated Analysis Report\n\n")
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
    run_name = "dispatcher"
    if len(sys.argv) > 1:
        try:
            args = json.loads(sys.argv[1])
            run_name = args.get("name", run_name)
        except (json.JSONDecodeError, AttributeError, TypeError):
            pass

    print(f"[agent_dispatcher] Starting (run: {run_name})")
    print(f"  Repo root: {REPO_ROOT}")
    print(f"  Output dir: {OUTPUT_DIR}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    results: List[Optional[Dict]] = []
    tmp_names = [
        OUTPUT_DIR / "analyze_findings.json",
        OUTPUT_DIR / "detect_findings.json",
    ]

    for script, tmp_name in zip(_AGENT_SCRIPTS, tmp_names):
        data = _run_agent(script, run_name, tmp_name)
        results.append(data)

    merged = _merge(run_name, results)
    _save_outputs(merged)

    errors = merged["summary"]["errors"]
    if errors > 0:
        print(f"\n[agent_dispatcher] ❌ {errors} error(s) found.")
        return 1
    print("\n[agent_dispatcher] ✅ All checks completed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
