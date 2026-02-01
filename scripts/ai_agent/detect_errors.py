#!/usr/bin/env python3
"""
Error Detection Agent - Finds potential errors, faults, and failures
Free-tier compatible with comprehensive error checking
Runs pytest + py_compile + grep for TODOs
"""

import sys
import json
import re
import datetime
import time
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# Import common utilities
try:
    from common import (
        run_command, write_json_output, safe_log, ensure_output_dir,
        check_python_syntax, find_todos_in_file, get_file_list, format_duration
    )
except ImportError:
    # Fallback implementations
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
        print(f"[{level}] {msg}")
    
    def ensure_output_dir():
        path = Path(__file__).parent / "output"
        path.mkdir(parents=True, exist_ok=True)
        return path
    
    def check_python_syntax(file_path):
        import py_compile
        try:
            py_compile.compile(str(file_path), doraise=True)
            return True, None
        except Exception as e:
            return False, str(e)
    
    def find_todos_in_file(file_path):
        todos = []
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            for i, line in enumerate(content.split('\n'), 1):
                if re.search(r'\b(TODO|FIXME|HACK|XXX)\b', line, re.IGNORECASE):
                    todos.append((i, line.strip()))
        except:
            pass
        return todos
    
    def get_file_list(extensions, exclude_dirs=None):
        repo_root = Path(__file__).parent.parent.parent
        files = []
        for ext in extensions:
            for f in repo_root.rglob(f"*{ext}"):
                if not any(ex in f.parts for ex in (exclude_dirs or ['.git', '__pycache__'])):
                    files.append(f)
        return files
    
    def format_duration(seconds):
        return f"{seconds:.1f}s"

REPO_ROOT = Path(__file__).parent.parent.parent


class ErrorDetector:
    """Detects errors and potential failures in code"""
    
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.info = []
        
    def detect_in_file(self, file_path: Path) -> Dict:
        """Detect errors in a single file"""
        result = {
            "file": str(file_path),
            "errors": [],
            "warnings": [],
            "info": [],
            "severity": "none"
        }
        
        if not file_path.exists():
            return result
            
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = content.split('\n')
            
            # Check for various error patterns
            for line_num, line in enumerate(lines, 1):
                line_errors = self._check_line(line, line_num, file_path.suffix)
                result["errors"].extend(line_errors["errors"])
                result["warnings"].extend(line_errors["warnings"])
                result["info"].extend(line_errors["info"])
                
            # Determine severity
            if len(result["errors"]) > 5:
                result["severity"] = "critical"
            elif len(result["errors"]) > 2:
                result["severity"] = "high"
            elif result["warnings"]:
                result["severity"] = "medium"
            elif result["info"]:
                result["severity"] = "low"
                
        except Exception as e:
            result["errors"].append(f"Read error: {str(e)}")
            
        return result
    
    def _check_line(self, line: str, line_num: int, suffix: str) -> Dict:
        """Check a single line for errors"""
        result = {"errors": [], "warnings": [], "info": []}
        
        # Null pointer issues
        if re.search(r'==\s*NULL\b', line) or re.search(r'=\s*NULL\b', line):
            if suffix in ['.cpp', '.c', '.h', '.ino']:
                result["warnings"].append(f"Line {line_num}: Use nullptr instead of NULL")
        
        # Division by zero potential
        if re.search(r'/\s*\w+\s*\)', line) and not re.search(r'if\s*\([^)]*==\s*0', line):
            if any(x in line for x in ['/', '%']):
                result["warnings"].append(f"Line {line_num}: Potential division by zero - check divisor")
        
        # Array bounds issues
        if re.search(r'\[\s*\w+\s*\]', line) and 'sizeof' not in line:
            if 'for' in line.lower() or 'while' in line.lower():
                result["warnings"].append(f"Line {line_num}: Potential array bounds access")
        
        # Uninitialized variables
        if re.search(r'int\s+\w+\s*;', line) and 'const' not in line:
            result["info"].append(f"Line {line_num}: Variable may be uninitialized")
        
        # Memory leaks - malloc without free
        if 'malloc(' in line or 'calloc(' in line:
            result["warnings"].append(f"Line {line_num}: Dynamic allocation detected - ensure proper deallocation")
        
        # Resource leaks - file/socket not closed
        if re.search(r'fopen\s*\(', line) and 'fclose' not in content_containing(line):
            result["warnings"].append(f"Line {line_num}: File opened - ensure it's closed")
        
        # Infinite loops
        if re.search(r'while\s*\(\s*true\s*\)', line) or re.search(r'while\s*\(\s*1\s*\)', line):
            result["info"].append(f"Line {line_num}: Infinite loop detected - ensure exit condition")
        
        # Blocking calls
        if 'delay(' in line:
            match = re.search(r'delay\s*\(\s*(\d+)\s*\)', line)
            if match:
                delay_ms = int(match.group(1))
                if delay_ms > 1000:
                    result["warnings"].append(f"Line {line_num}: Long delay ({delay_ms}ms) may block responsiveness")
        
        # Serial issues
        if 'Serial.' in line:
            if 'Serial.begin' not in line and 'Serial.' in line:
                result["warnings"].append(f"Line {line_num}: Serial used without initialization")
            if 'Serial.read' in line and 'Serial.available' not in line:
                result["warnings"].append(f"Line {line_num}: Serial read without available() check")
        
        # LED strip issues
        if 'FastLED' in line or 'APA102' in line:
            if 'show()' not in content_containing(line):
                result["info"].append(f"Line {line_num}: LED operations without show() call")
        
        return result
    
    def detect_project_errors(self) -> Dict:
        """Detect errors across the entire project"""
        self.errors = []
        self.warnings = []
        self.info = []
        
        results = {
            "summary": {
                "files_checked": 0,
                "total_errors": 0,
                "total_warnings": 0,
                "total_info": 0,
                "severity_distribution": {"critical": 0, "high": 0, "medium": 0, "low": 0, "none": 0}
            },
            "files": [],
            "critical_errors": [],
            "auto_fixes": []
        }
        
        # Define files to check
        extensions = ['.ino', '.cpp', '.c', '.h', '.py']
        for ext in extensions:
            for file_path in REPO_ROOT.rglob(f"*{ext}"):
                if any(part.startswith('.') for part in file_path.parts):
                    continue
                    
                result = self.detect_in_file(file_path)
                if result["errors"] or result["warnings"]:
                    results["files"].append(result)
                    
                results["summary"]["files_checked"] += 1
                results["summary"]["severity_distribution"][result.get("severity", "none")] += 1
                
        # Count totals
        for file_result in results["files"]:
            results["summary"]["total_errors"] += len(file_result["errors"])
            results["summary"]["total_warnings"] += len(file_result["warnings"])
            results["summary"]["total_info"] += len(file_result["info"])
        
        # Collect critical errors
        for file_result in results["files"]:
            if file_result["severity"] in ["critical", "high"]:
                results["critical_errors"].extend(file_result["errors"])
        
        # Generate auto-fix suggestions
        results["auto_fixes"] = self._generate_auto_fixes(results)
        
        return results
    
    def _generate_auto_fixes(self, results: Dict) -> List[Dict]:
        """Generate automatic fix suggestions"""
        fixes = []
        
        # Fix NULL -> nullptr
        fixes.append({
            "type": "replace",
            "pattern": r'\bNULL\b',
            "replacement": "nullptr",
            "description": "Replace NULL with nullptr for C++ compatibility",
            "files_affected": sum(1 for f in results["files"] if f["file"].endswith(('.cpp', '.h', '.ino')))
        })
        
        # Fix strcpy -> strncpy
        fixes.append({
            "type": "replace",
            "pattern": r'\bstrcpy\s*\(',
            "replacement": "strncpy(",
            "description": "Replace strcpy with strncpy to prevent buffer overflows",
            "files_affected": sum(1 for f in results["files"] if 'strcpy' in str(f))
        })
        
        # Fix size() == 0 -> empty()
        fixes.append({
            "type": "replace",
            "pattern": r'\.size\s*\(\s*\)\s*==\s*0',
            "replacement": ".empty()",
            "description": "Use empty() instead of size() == 0 for better readability",
            "files_affected": sum(1 for f in results["files"] if 'size() == 0' in str(f))
        })
        
        return fixes


def content_containing(line: str, keyword: str = None) -> str:
    """Helper to get surrounding content context"""
    # Simplified - in real implementation would return more context
    return line


def run_pytest_check() -> Dict:
    """Run pytest for error detection"""
    safe_log("Running pytest for error detection...", "INFO")
    examples_dir = REPO_ROOT / "examples"
    
    if not examples_dir.exists():
        return {"skipped": True, "reason": "examples/ directory not found"}
    
    start = time.time()
    exit_code, stdout, stderr = run_command(
        [sys.executable, "-m", "pytest", "test_*.py", "-v", "--tb=short"],
        cwd=examples_dir,
        timeout=180
    )
    duration = time.time() - start
    
    result = {
        "exit_code": exit_code,
        "duration": format_duration(duration),
        "passed": exit_code == 0,
        "output": stdout[-1000:] if stdout else "",
        "errors": stderr[-1000:] if stderr else ""
    }
    
    # Parse failures
    failures = re.findall(r'FAILED (.+?) -', stdout) if stdout else []
    result["failed_tests"] = failures
    
    safe_log(f"Pytest check: {'PASSED' if result['passed'] else 'FAILED'} ({result['duration']})", 
             "INFO" if result['passed'] else "ERROR")
    
    return result


def run_py_compile_check() -> Dict:
    """Run py_compile on all Python files"""
    safe_log("Running py_compile syntax check...", "INFO")
    start = time.time()
    
    python_files = get_file_list(['.py'])
    errors = []
    checked = 0
    
    for py_file in python_files:
        checked += 1
        is_valid, error_msg = check_python_syntax(py_file)
        if not is_valid:
            errors.append({
                "file": str(py_file.relative_to(REPO_ROOT)),
                "error": error_msg
            })
    
    duration = time.time() - start
    
    result = {
        "files_checked": checked,
        "errors": errors,
        "passed": len(errors) == 0,
        "duration": format_duration(duration)
    }
    
    safe_log(f"Py_compile check: {checked} files, {len(errors)} errors ({result['duration']})", 
             "INFO" if result['passed'] else "ERROR")
    
    return result


def grep_todos() -> Dict:
    """Find all TODO/FIXME/HACK comments"""
    safe_log("Searching for TODO/FIXME/HACK comments...", "INFO")
    start = time.time()
    
    # Search in Python, C/C++, and Arduino files
    file_extensions = ['.py', '.cpp', '.c', '.h', '.ino']
    all_files = get_file_list(file_extensions)
    
    todos_by_file = {}
    total_todos = 0
    
    for file_path in all_files:
        todos = find_todos_in_file(file_path)
        if todos:
            rel_path = str(file_path.relative_to(REPO_ROOT))
            todos_by_file[rel_path] = [
                {"line": line_num, "content": content}
                for line_num, content in todos
            ]
            total_todos += len(todos)
    
    duration = time.time() - start
    
    result = {
        "total_todos": total_todos,
        "files_with_todos": len(todos_by_file),
        "todos": todos_by_file,
        "duration": format_duration(duration)
    }
    
    safe_log(f"Found {total_todos} TODOs/FIXMEs in {len(todos_by_file)} files ({result['duration']})", "INFO")
    
    return result


def detect_errors_task(task_data: Dict) -> Dict:
    """Main error detection function"""
    safe_log("=== Starting Error Detection ===", "INFO")
    start_time = time.time()
    
    # Run static error detection
    detector = ErrorDetector()
    static_results = detector.detect_project_errors()
    
    # Run real checks
    checks = {
        "pytest": run_pytest_check(),
        "py_compile": run_py_compile_check(),
        "todos": grep_todos()
    }
    
    # Combine results
    results = {
        **static_results,
        "checks": checks,
        "task": task_data.get("name", "error_detection"),
        "timestamp": str(datetime.datetime.now()),
        "total_duration": format_duration(time.time() - start_time)
    }
    
    # Overall status
    all_passed = all(
        check.get("passed", True) or check.get("skipped", False)
        for check in [checks["pytest"], checks["py_compile"]]
    )
    results["overall_status"] = "PASSED" if all_passed else "FAILED"
    
    # Write output
    output_file = write_json_output(results, "detect_errors.json")
    safe_log(f"Error detection complete: {results['overall_status']}", "INFO")
    safe_log(f"Results written to {output_file}", "INFO")
    
    return results


if __name__ == "__main__":
    try:
        task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    except json.JSONDecodeError:
        task_data = {}
    result = detect_errors_task(task_data)
    print(json.dumps(result, indent=2))
