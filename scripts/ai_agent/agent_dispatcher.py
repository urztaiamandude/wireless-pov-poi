#!/usr/bin/env python3
"""
AI Agent Dispatcher - Autonomous task routing and execution for Wireless POV POI
Part of the free-tier AI agent team system
"""

import os
import sys
import json
import subprocess
import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Configuration
REPO_ROOT = Path(__file__).parent.parent.parent
TASKS_DIR = REPO_ROOT / ".github" / "tasks"
OUTPUT_DIR = REPO_ROOT / "scripts" / "ai_agent" / "output"

# Agent definitions
AGENTS = {
    "firmware": {
        "description": "Handles Teensy 4.1 and ESP32 firmware development",
        "extensions": [".ino", ".cpp", ".c", ".h"],
        "directories": ["firmware/", "esp32_firmware/", "teensy_firmware/"],
        "scripts": ["analyze_firmware.py", "fix_firmware.py", "generate_patterns.py"]
    },
    "documentation": {
        "description": "Maintains and improves all documentation",
        "extensions": [".md", ".txt", ".rst"],
        "directories": ["docs/", "wiki/", "README.md"],
        "scripts": ["update_docs.py", "check_docs.py"]
    },
    "testing": {
        "description": "Automated testing and quality assurance",
        "extensions": [".py", ".sh"],
        "directories": ["examples/", "scripts/", "test_*.py"],
        "scripts": ["run_tests.py", "test_converter.py", "test_firmware.py"]
    },
    "analysis": {
        "description": "Code analysis, bug detection, and error finding",
        "extensions": [".py", ".ino", ".cpp", ".c", ".h", ".md"],
        "directories": [".github/", "scripts/"],
        "scripts": ["analyze_code.py", "detect_errors.py", "check_security.py"]
    }
}


def log_message(message: str, level: str = "INFO") -> None:
    """Log a message with timestamp and level"""
    timestamp = datetime.datetime.now().isoformat()
    print(f"[{timestamp}] [{level}] {message}")
    
    # Also write to log file
    log_file = OUTPUT_DIR / "dispatcher.log"
    log_file.parent.mkdir(parents=True, exist_ok=True)
    with open(log_file, "a") as f:
        f.write(f"[{timestamp}] [{level}] {message}\n")


def analyze_task_type(task_content: str) -> str:
    """Determine which agent should handle a task based on content"""
    task_lower = task_content.lower()
    
    # Check keywords for each agent type
    firmware_keywords = ["firmware", "teensy", "esp32", "led", "pov", "pattern", "arduino", "platformio"]
    docs_keywords = ["documentation", "readme", "guide", "docs", "wiki", "comment"]
    test_keywords = ["test", "bug", "error", "fix", "fail", "crash", "assertion"]
    analysis_keywords = ["analyze", "review", "code", "structure", "performance", "security"]
    
    # Count keyword matches
    firmware_count = sum(1 for kw in firmware_keywords if kw in task_lower)
    docs_count = sum(1 for kw in docs_keywords if kw in task_lower)
    test_count = sum(1 for kw in test_keywords if kw in task_lower)
    analysis_count = sum(1 for kw in analysis_keywords if kw in task_lower)
    
    # Return agent with highest match count
    counts = [
        ("firmware", firmware_count),
        ("documentation", docs_count),
        ("testing", test_count),
        ("analysis", analysis_count)
    ]
    
    best_match = max(counts, key=lambda x: x[1])
    return best_match[0] if best_match[1] > 0 else "analysis"


def read_task_file(task_path: Path) -> Optional[Dict]:
    """Read and parse a task file"""
    try:
        content = task_path.read_text()
        return {
            "path": str(task_path),
            "content": content,
            "type": analyze_task_type(content),
            "name": task_path.stem
        }
    except Exception as e:
        log_message(f"Error reading task {task_path}: {e}", "ERROR")
        return None


def get_pending_tasks() -> List[Dict]:
    """Get all pending tasks from the tasks directory"""
    tasks = []
    if TASKS_DIR.exists():
        for task_file in TASKS_DIR.glob("*.md"):
            task_data = read_task_file(task_file)
            if task_data:
                tasks.append(task_data)
    return tasks


def run_agent_script(agent_type: str, script_name: str, task_data: Dict) -> Tuple[bool, str]:
    """Run a specific agent script"""
    agent_config = AGENTS.get(agent_type, {})
    scripts_dir = OUTPUT_DIR / "scripts"
    
    # Look for the script in various locations
    possible_paths = [
        scripts_dir / script_name,
        REPO_ROOT / "scripts" / "ai_agent" / script_name,
        REPO_ROOT / "scripts" / script_name,
    ]
    
    script_path = None
    for path in possible_paths:
        if path.exists():
            script_path = path
            break
    
    if not script_path:
        log_message(f"Script {script_name} not found for agent {agent_type}", "WARNING")
        # Create the script if it doesn't exist
        script_path = scripts_dir / script_name
        create_agent_script(agent_type, script_name, script_path)
    
    try:
        result = subprocess.run(
            [sys.executable, str(script_path), json.dumps(task_data)],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )
        
        if result.returncode == 0:
            return True, result.stdout
        else:
            return False, result.stderr
    except subprocess.TimeoutExpired:
        return False, "Script timed out after 5 minutes"
    except Exception as e:
        return False, str(e)


def create_agent_script(agent_type: str, script_name: str, script_path: Path) -> None:
    """Create a basic agent script if it doesn't exist"""
    script_path.parent.mkdir(parents=True, exist_ok=True)
    
    script_templates = {
        "analyze_firmware.py": '''#!/usr/bin/env python3
"""
Firmware Analysis Agent - Analyzes Teensy and ESP32 firmware for errors and improvements
"""

import sys
import json
from pathlib import Path

def analyze_firmware(task_data):
    """Analyze firmware code for potential issues"""
    print(f"Analyzing firmware for task: {task_data.get('name', 'unknown')}")
    # TODO: Implement firmware analysis
    return {"issues_found": 0, "suggestions": []}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = analyze_firmware(task_data)
    print(json.dumps(result))
''',
        "fix_firmware.py": '''#!/usr/bin/env python3
"""
Firmware Fix Agent - Automatically fixes common firmware issues
"""

import sys
import json

def fix_firmware(task_data):
    """Fix identified firmware issues"""
    print(f"Fixing firmware for task: {task_data.get('name', 'unknown')}")
    # TODO: Implement automatic fixes
    return {"fixes_applied": 0}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = fix_firmware(task_data)
    print(json.dumps(result))
''',
        "generate_patterns.py": '''#!/usr/bin/env python3
"""
Pattern Generation Agent - Creates new LED patterns for POV display
"""

import sys
import json
import random

PATTERNS = [
    "rainbow_cycle",
    "wave_sine",
    "gradient_fade",
    "sparkle_random",
    "pulse_beat",
    "fire_effect",
    "comet_tail",
    "plasma_wave"
]

def generate_patterns(task_data):
    """Generate new LED patterns"""
    num_patterns = task_data.get("num_patterns", 3)
    new_patterns = random.sample(PATTERNS, min(num_patterns, len(PATTERNS)))
    print(f"Generated {len(new_patterns)} new patterns")
    return {"patterns": new_patterns}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = generate_patterns(task_data)
    print(json.dumps(result))
''',
        "analyze_code.py": '''#!/usr/bin/env python3
"""
Code Analysis Agent - Comprehensive code analysis and error detection
"""

import sys
import json
import ast
from pathlib import Path

def analyze_code(task_data):
    """Perform comprehensive code analysis"""
    print(f"Analyzing code for task: {task_data.get('name', 'unknown')}")
    # TODO: Implement comprehensive analysis
    return {"files_analyzed": 0, "issues": [], "score": 100}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = analyze_code(task_data)
    print(json.dumps(result))
''',
        "detect_errors.py": '''#!/usr/bin/env python3
"""
Error Detection Agent - Finds potential errors and faults in code
"""

import sys
import json
import re

def detect_errors(task_data):
    """Detect potential errors and faults"""
    print(f"Detecting errors for task: {task_data.get('name', 'unknown')}")
    # TODO: Implement error detection
    return {"errors": [], "warnings": [], "info": []}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = detect_errors(task_data)
    print(json.dumps(result))
''',
        "run_tests.py": '''#!/usr/bin/env python3
"""
Testing Agent - Runs automated tests and reports results
"""

import sys
import json
import subprocess

def run_tests(task_data):
    """Run automated tests"""
    print(f"Running tests for task: {task_data.get('name', 'unknown')}")
    # TODO: Implement test execution
    return {"passed": 0, "failed": 0, "skipped": 0}

if __name__ == "__main__":
    task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    result = run_tests(task_data)
    print(json.dumps(result))
'''
    }
    
    template = script_templates.get(script_name, "# Placeholder script\n")
    script_path.write_text(template)
    script_path.chmod(0o755)
    log_message(f"Created script: {script_path}", "INFO")


def dispatch_task(task_data: Dict) -> Dict:
    """Dispatch a task to the appropriate agent"""
    agent_type = task_data.get("type", "analysis")
    agent_config = AGENTS.get(agent_type, {})
    
    log_message(f"Dispatching task '{task_data['name']}' to {agent_type} agent")
    
    results = {
        "task": task_data["name"],
        "agent": agent_type,
        "success": False,
        "output": "",
        "errors": []
    }
    
    # Run each script for the agent
    for script_name in agent_config.get("scripts", []):
        success, output = run_agent_script(agent_type, script_name, task_data)
        if success:
            results["success"] = True
            results["output"] += output + "\n"
        else:
            results["errors"].append(f"{script_name}: {output}")
    
    return results


def main():
    """Main entry point for the dispatcher"""
    log_message("Starting AI Agent Dispatcher", "INFO")
    
    # Get pending tasks
    tasks = get_pending_tasks()
    log_message(f"Found {len(tasks)} pending tasks")
    
    # Process each task
    results = []
    for task in tasks:
        log_message(f"Processing task: {task['name']}")
        result = dispatch_task(task)
        results.append(result)
        
        # Mark task as processed (rename to .done)
        task_path = Path(task["path"])
        if task_path.exists():
            done_path = task_path.with_suffix(".done")
            task_path.rename(done_path)
    
    # Generate summary report
    summary = {
        "total_tasks": len(tasks),
        "successful": sum(1 for r in results if r["success"]),
        "failed": sum(1 for r in results if not r["success"]),
        "results": results
    }
    
    # Save summary
    summary_file = OUTPUT_DIR / "dispatcher_summary.json"
    summary_file.write_text(json.dumps(summary, indent=2))
    
    log_message(f"Dispatcher complete. Success: {summary['successful']}/{summary['total_tasks']}", "INFO")
    
    return 0 if summary["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
