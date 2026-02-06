#!/usr/bin/env python3
"""
Common utilities for AI Agent system
Shared helpers for task loading, command execution, JSON output, and logging
"""

import os
import sys
import json
import subprocess
import datetime
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any

# Project paths
REPO_ROOT = Path(__file__).parent.parent.parent
TASKS_DIR = REPO_ROOT / ".github" / "tasks"
OUTPUT_DIR = REPO_ROOT / "scripts" / "ai_agent" / "output"


def ensure_output_dir() -> Path:
    """Ensure the output directory exists"""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    return OUTPUT_DIR


def safe_log(message: str, level: str = "INFO", log_file: Optional[Path] = None) -> None:
    """
    Safely log a message with timestamp and level
    
    Args:
        message: The message to log
        level: Log level (INFO, WARNING, ERROR, DEBUG)
        log_file: Optional specific log file path
    """
    timestamp = datetime.datetime.now().isoformat()
    log_line = f"[{timestamp}] [{level}] {message}"
    print(log_line)
    
    # Write to log file
    if log_file is None:
        log_file = ensure_output_dir() / "dispatcher.log"
    
    try:
        with open(log_file, "a") as f:
            f.write(log_line + "\n")
    except Exception as e:
        print(f"Warning: Could not write to log file: {e}")


def run_command(
    cmd: List[str],
    cwd: Optional[Path] = None,
    timeout: int = 300,
    capture_output: bool = True
) -> Tuple[int, str, str]:
    """
    Run a command and return exit code, stdout, stderr
    
    Args:
        cmd: Command and arguments as list
        cwd: Working directory (defaults to REPO_ROOT)
        timeout: Command timeout in seconds
        capture_output: Whether to capture stdout/stderr
        
    Returns:
        Tuple of (exit_code, stdout, stderr)
    """
    if cwd is None:
        cwd = REPO_ROOT
        
    safe_log(f"Running command: {' '.join(cmd)}", "DEBUG")
    
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            capture_output=capture_output,
            text=True,
            timeout=timeout
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", f"Command timed out after {timeout} seconds"
    except FileNotFoundError:
        return -1, "", f"Command not found: {cmd[0]}"
    except Exception as e:
        return -1, "", str(e)


def write_json_output(data: Dict[str, Any], filename: str) -> Path:
    """
    Write JSON data to output file
    
    Args:
        data: Data to write as JSON
        filename: Output filename (relative to output dir)
        
    Returns:
        Path to written file
    """
    output_path = ensure_output_dir() / filename
    
    try:
        with open(output_path, "w") as f:
            json.dump(data, f, indent=2)
        safe_log(f"Wrote output to {output_path}", "INFO")
        return output_path
    except Exception as e:
        safe_log(f"Error writing JSON output: {e}", "ERROR")
        raise


def load_task_from_markdown(task_path: Path) -> Optional[Dict[str, Any]]:
    """
    Load and parse a task from a markdown file
    
    Args:
        task_path: Path to task markdown file
        
    Returns:
        Dict with task data or None if error
    """
    try:
        content = task_path.read_text()
        
        # Parse task metadata
        task = {
            "name": task_path.stem,
            "path": str(task_path),
            "content": content,
            "type": "analysis",  # Default
            "priority": "medium",
            "status": "unknown"
        }
        
        # Extract metadata from markdown
        # Look for **Field**: Value patterns
        metadata_patterns = {
            "type": r'\*\*Type\*\*:\s*(\w+)',
            "priority": r'\*\*Priority\*\*:\s*(\w+)',
            "status": r'\*\*Status\*\*:\s*(\w+)',
        }
        
        for field, pattern in metadata_patterns.items():
            match = re.search(pattern, content, re.IGNORECASE)
            if match:
                task[field] = match.group(1).lower()
        
        # Extract title
        title_match = re.search(r'^#\s+(?:Task:\s+)?(.+)$', content, re.MULTILINE)
        if title_match:
            task["title"] = title_match.group(1).strip()
        
        return task
        
    except Exception as e:
        safe_log(f"Error loading task {task_path}: {e}", "ERROR")
        return None


def load_all_tasks(tasks_dir: Optional[Path] = None) -> List[Dict[str, Any]]:
    """
    Load all task files from the tasks directory
    
    Args:
        tasks_dir: Directory containing task files (defaults to .github/tasks/)
        
    Returns:
        List of task dictionaries
    """
    if tasks_dir is None:
        tasks_dir = TASKS_DIR
    
    tasks = []
    
    if not tasks_dir.exists():
        safe_log(f"Tasks directory not found: {tasks_dir}", "WARNING")
        return tasks
    
    for task_file in tasks_dir.glob("*.md"):
        task = load_task_from_markdown(task_file)
        if task:
            tasks.append(task)
            safe_log(f"Loaded task: {task['name']} (type: {task['type']})", "DEBUG")
    
    safe_log(f"Loaded {len(tasks)} tasks from {tasks_dir}", "INFO")
    return tasks


def check_python_syntax(file_path: Path) -> Tuple[bool, Optional[str]]:
    """
    Check Python file syntax using py_compile
    
    Args:
        file_path: Path to Python file
        
    Returns:
        Tuple of (is_valid, error_message)
    """
    import py_compile
    
    try:
        py_compile.compile(str(file_path), doraise=True)
        return True, None
    except py_compile.PyCompileError as e:
        return False, str(e)


def find_todos_in_file(file_path: Path) -> List[Tuple[int, str]]:
    """
    Find TODO/FIXME/HACK comments in a file
    
    Args:
        file_path: Path to file to search
        
    Returns:
        List of (line_number, line_content) tuples
    """
    todos = []
    
    try:
        content = file_path.read_text(encoding='utf-8', errors='ignore')
        lines = content.split('\n')
        
        for line_num, line in enumerate(lines, 1):
            if re.search(r'\b(TODO|FIXME|HACK|XXX)\b', line, re.IGNORECASE):
                todos.append((line_num, line.strip()))
    except Exception:
        pass
    
    return todos


def format_duration(seconds: float) -> str:
    """Format duration in human-readable form"""
    if seconds < 60:
        return f"{seconds:.1f}s"
    elif seconds < 3600:
        minutes = seconds / 60
        return f"{minutes:.1f}m"
    else:
        hours = seconds / 3600
        return f"{hours:.1f}h"


def get_file_list(extensions: List[str], exclude_dirs: Optional[List[str]] = None) -> List[Path]:
    """
    Get list of files with specified extensions
    
    Args:
        extensions: List of file extensions (e.g., ['.py', '.cpp'])
        exclude_dirs: List of directory patterns to exclude
        
    Returns:
        List of Path objects
    """
    if exclude_dirs is None:
        exclude_dirs = ['.git', '__pycache__', 'node_modules', '.venv', 'venv']
    
    files = []
    
    for ext in extensions:
        for file_path in REPO_ROOT.rglob(f"*{ext}"):
            # Check if any parent is in exclude list
            if any(excluded in file_path.parts for excluded in exclude_dirs):
                continue
            files.append(file_path)
    
    return files
