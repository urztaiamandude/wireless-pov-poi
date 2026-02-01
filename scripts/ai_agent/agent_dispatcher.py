#!/usr/bin/env python3
"""
AI Agent Dispatcher - Autonomous task routing and execution for Wireless POV POI
Reads tasks from .github/tasks/*.md, parses Type field, runs corresponding scripts
NO git mutations - read-only task processing
"""

import os
import sys
import json
import subprocess
import datetime
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Import common utilities
try:
    from common import (
        load_all_tasks, run_command, write_json_output, 
        safe_log, ensure_output_dir
    )
except ImportError:
    # Fallback implementations
    def load_all_tasks():
        return []
    
    def run_command(cmd, cwd=None, timeout=300, capture_output=True):
        import subprocess
        result = subprocess.run(cmd, cwd=cwd, capture_output=capture_output, text=True, timeout=timeout)
        return result.returncode, result.stdout, result.stderr
    
    def write_json_output(data, filename):
        path = Path(__file__).parent / "output" / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
        return path
    
    def safe_log(msg, level="INFO", log_file=None):
        timestamp = datetime.datetime.now().isoformat()
        print(f"[{timestamp}] [{level}] {msg}")
    
    def ensure_output_dir():
        path = Path(__file__).parent / "output"
        path.mkdir(parents=True, exist_ok=True)
        return path

# Configuration
REPO_ROOT = Path(__file__).parent.parent.parent
TASKS_DIR = REPO_ROOT / ".github" / "tasks"
OUTPUT_DIR = REPO_ROOT / "scripts" / "ai_agent" / "output"
SCRIPTS_DIR = REPO_ROOT / "scripts" / "ai_agent"

# Map task types to scripts
TASK_TYPE_SCRIPTS = {
    "analysis": "analyze_code.py",
    "test": "detect_errors.py",
    "pattern": "generate_patterns.py",
    "firmware": "generate_patterns.py",  # Firmware tasks often involve patterns
}


def dispatch_task(task_data: Dict) -> Dict:
    """Dispatch a task to the appropriate script based on type"""
    task_type = task_data.get("type", "analysis").lower()
    task_name = task_data.get("name", "unknown")
    
    safe_log(f"Dispatching task '{task_name}' (type: {task_type})")
    
    # Get script for this task type
    script_name = TASK_TYPE_SCRIPTS.get(task_type, "analyze_code.py")
    script_path = SCRIPTS_DIR / script_name
    
    if not script_path.exists():
        safe_log(f"Script not found: {script_path}", "ERROR")
        return {
            "task": task_name,
            "type": task_type,
            "script": script_name,
            "success": False,
            "error": f"Script {script_name} not found"
        }
    
    # Run the script
    safe_log(f"Running {script_name} for task {task_name}")
    start = time.time()
    
    exit_code, stdout, stderr = run_command(
        [sys.executable, str(script_path), json.dumps(task_data)],
        cwd=REPO_ROOT,
        timeout=600  # 10 minute timeout
    )
    
    duration = time.time() - start
    
    result = {
        "task": task_name,
        "type": task_type,
        "script": script_name,
        "exit_code": exit_code,
        "success": exit_code == 0,
        "duration": f"{duration:.1f}s",
        "output": stdout[-1000:] if stdout else "",  # Last 1000 chars
        "errors": stderr[-500:] if stderr else ""
    }
    
    if result["success"]:
        safe_log(f"Task '{task_name}' completed successfully ({result['duration']})", "INFO")
    else:
        safe_log(f"Task '{task_name}' failed with exit code {exit_code}", "ERROR")
    
    return result


def main():
    """Main entry point for the dispatcher"""
    safe_log("=== AI Agent Dispatcher Starting ===")
    start_time = time.time()
    
    ensure_output_dir()
    
    # Load all tasks from .github/tasks/
    safe_log(f"Loading tasks from {TASKS_DIR}")
    tasks = load_all_tasks(TASKS_DIR)
    
    if not tasks:
        safe_log("No tasks found", "WARNING")
        summary = {
            "total_tasks": 0,
            "successful": 0,
            "failed": 0,
            "skipped": 0,
            "tasks": [],
            "timestamp": str(datetime.datetime.now()),
            "duration": "0.0s"
        }
        write_json_output(summary, "dispatcher_summary.json")
        return 0
    
    safe_log(f"Found {len(tasks)} tasks to process")
    
    # Process each task
    results = []
    for task in tasks:
        safe_log(f"--- Processing task: {task['name']} ---")
        result = dispatch_task(task)
        results.append(result)
        
        # Small delay between tasks
        time.sleep(0.5)
    
    # Generate summary report
    total_duration = time.time() - start_time
    summary = {
        "total_tasks": len(tasks),
        "successful": sum(1 for r in results if r["success"]),
        "failed": sum(1 for r in results if not r["success"]),
        "tasks": results,
        "timestamp": str(datetime.datetime.now()),
        "duration": f"{total_duration:.1f}s"
    }
    
    # Write summary
    summary_file = write_json_output(summary, "dispatcher_summary.json")
    
    safe_log("=== Dispatcher Complete ===")
    safe_log(f"Total: {summary['total_tasks']}, Success: {summary['successful']}, Failed: {summary['failed']}")
    safe_log(f"Duration: {summary['duration']}")
    safe_log(f"Summary written to {summary_file}")
    
    # NOTE: We do NOT mutate task files - they remain in .github/tasks/ for future runs
    # This is a read-only dispatcher
    
    return 0 if summary["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
