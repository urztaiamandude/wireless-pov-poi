#!/usr/bin/env python3
"""
Comprehensive Code Analysis Agent for Wireless POV POI
Detects errors, faults, potential failures, and suggests improvements
Free-tier compatible with local analysis
Runs real checks: Python compile, pytest, optional PlatformIO build
"""

import sys
import json
import ast
import re
import datetime
import os
import time
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# Import common utilities
try:
    from common import (
        run_command, write_json_output, safe_log, 
        ensure_output_dir, check_python_syntax, format_duration
    )
except ImportError:
    # Fallback if common not available
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
        except py_compile.PyCompileError as e:
            return False, str(e)
    
    def format_duration(seconds):
        return f"{seconds:.1f}s"

# Project root
REPO_ROOT = Path(__file__).parent.parent.parent

# Known error patterns
ERROR_PATTERNS = [
    # C/C++ patterns
    (r'serial\.begin\s*\([^)]*\)\s*;', 'WARNING: serial.begin() typically needs baud rate parameter'),
    (r'delay\s*\(\s*0\s*\)', 'INFO: delay(0) has no effect'),
    (r'while\s*\(\s*1\s*\)', 'INFO: Infinite while loop detected'),
    (r'for\s*\(\s*;\s*true\s*;', 'INFO: Infinite for loop detected'),
    
    # Memory issues
    (r'malloc\s*\(', 'WARNING: Consider using new/delete or smart pointers'),
    (r'strcpy\s*\(', 'WARNING: strcpy is unsafe, use strncpy or std::string'),
    (r'sprintf\s*\(', 'WARNING: sprintf is unsafe, use snprintf'),
    
    # Python patterns
    (r'except\s*:', 'WARNING: Bare except clause, catch specific exceptions'),
    (r'print\s+\w+', 'INFO: Consider using logging module instead of print'),
    
    # General patterns
    (r'TODO', 'INFO: TODO comment found'),
    (r'FIXME', 'WARNING: FIXME comment found - needs attention'),
    (r'HACK', 'WARNING: HACK comment found - workaround detected'),
    (r'XXX', 'WARNING: XXX comment found - questionable code'),
]

# Potential failure patterns
FAILURE_PATTERNS = [
    (r'delay\s*\(\s*\d+\s*\)', 'WARNING: Long delay detected - may affect responsiveness'),
    (r'serial\.available\s*\(\s*\)\s*&&\s*!serial\.read', 'INFO: Checking serial without reading'),
    (r'digitalWrite\s*\([^,)]+\s*,\s*HIGH\s*\)', 'INFO: Consider using digitalWrite HIGH for consistency'),
    (r'analogWrite\s*\([^,)]+,\s*0\s*\)', 'INFO: analogWrite with 0 may not turn off LED'),
    (r'if\s*\([^)]+\s*==\s*NULL\s*\)', 'WARNING: Use nullptr instead of NULL in C++'),
    (r'\.size\s*\(\s*\)\s*==\s*0', 'INFO: Use empty() instead of size() == 0'),
]

# Code quality issues
QUALITY_PATTERNS = [
    (r'\b[a-z][a-z0-9]*_[a-z0-9]*\b', 'STYLE: Consider using camelCase or PascalCase'),
    (r'^\s{0,3}\d+\s+', 'STYLE: Line numbers detected in code'),
    (r'\t', 'STYLE: Tab character detected - use spaces for consistency'),
    (r'\r\n', 'INFO: Windows line endings detected'),
]


class CodeAnalyzer:
    """Comprehensive code analysis for the POV POI project"""
    
    def __init__(self):
        self.issues = []
        self.warnings = []
        self.info = []
        self.score = 100
        
    def analyze_file(self, file_path: Path) -> Dict:
        """Analyze a single file for issues"""
        result = {
            "file": str(file_path),
            "issues": [],
            "warnings": [],
            "info": [],
            "score": 100
        }
        
        if not file_path.exists():
            return result
            
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = content.split('\n')
            
            # Analyze each line
            for line_num, line in enumerate(lines, 1):
                line_issues = self._analyze_line(line, line_num)
                result["issues"].extend(line_issues["issues"])
                result["warnings"].extend(line_issues["warnings"])
                result["info"].extend(line_issues["info"])
            
            # Language-specific analysis
            if file_path.suffix in ['.py']:
                self._analyze_python(content, result)
            elif file_path.suffix in ['.ino', '.cpp', '.c', '.h']:
                self._analyze_arduino(content, result, file_path)
                
            # Calculate score
            result["score"] = max(0, 100 - len(result["issues"]) * 5 - len(result["warnings"]) * 2)
            
        except Exception as e:
            result["info"].append(f"Analysis error: {str(e)}")
            
        return result
    
    def _analyze_line(self, line: str, line_num: int) -> Dict:
        """Analyze a single line for patterns"""
        result = {"issues": [], "warnings": [], "info": []}
        
        for pattern, message in ERROR_PATTERNS:
            if re.search(pattern, line, re.IGNORECASE):
                result["issues"].append(f"Line {line_num}: {message}")
                self.score -= 5
                
        for pattern, message in FAILURE_PATTERNS:
            if re.search(pattern, line, re.IGNORECASE):
                result["warnings"].append(f"Line {line_num}: {message}")
                self.score -= 2
                
        for pattern, message in QUALITY_PATTERNS:
            if re.search(pattern, line, re.IGNORECASE):
                result["info"].append(f"Line {line_num}: {message}")
                
        return result
    
    def _analyze_python(self, content: str, result: Dict) -> None:
        """Python-specific analysis"""
        try:
            tree = ast.parse(content)
            
            for node in ast.walk(tree):
                # Check for bare except
                if isinstance(node, ast.ExceptHandler):
                    if node.type is None:
                        result["issues"].append("Bare except clause detected")
                        
                # Check for very long functions
                if isinstance(node, ast.FunctionDef):
                    if node.end_lineno - node.lineno > 50:
                        result["warnings"].append(f"Function '{node.name}' is very long ({node.end_lineno - node.lineno} lines)")
                        
        except SyntaxError as e:
            result["issues"].append(f"Syntax error: {str(e)}")
    
    def _analyze_arduino(self, content: str, result: Dict, file_path: Path) -> None:
        """Arduino/C++ specific analysis"""
        # Check for missing loop()
        if 'void loop()' not in content:
            result["warnings"].append("Missing loop() function - required for Arduino")
            
        # Check for missing setup()
        if 'void setup()' not in content:
            result["warnings"].append("Missing setup() function - required for Arduino")
            
        # Check for Serial without begin
        if 'Serial.' in content and 'Serial.begin' not in content:
            result["issues"].append("Serial used without Serial.begin()")
            
        # Check for delay in ISR-like contexts
        if 'void' in content and 'interrupt' in content.lower():
            if 'delay' in content:
                result["issues"].append("delay() used in potential interrupt context - avoid in ISRs")
    
    def analyze_project(self) -> Dict:
        """Analyze the entire project"""
        self.issues = []
        self.warnings = []
        self.info = []
        
        analysis_results = {
            "summary": {
                "files_analyzed": 0,
                "total_issues": 0,
                "total_warnings": 0,
                "total_info": 0,
                "overall_score": 100
            },
            "files": [],
            "critical_issues": [],
            "recommendations": []
        }
        
        # Files to analyze
        file_patterns = [
            "**/*.ino",
            "**/*.cpp",
            "**/*.c",
            "**/*.h",
            "**/*.py",
        ]
        
        # Analyze each file
        for pattern in file_patterns:
            for file_path in REPO_ROOT.glob(pattern):
                # Skip hidden and test files
                if any(part.startswith('.') for part in file_path.parts):
                    continue
                    
                result = self.analyze_file(file_path)
                if result["issues"] or result["warnings"] or result["info"]:
                    analysis_results["files"].append(result)
                    
                analysis_results["summary"]["files_analyzed"] += 1
                analysis_results["summary"]["total_issues"] += len(result["issues"])
                analysis_results["summary"]["total_warnings"] += len(result["warnings"])
                analysis_results["summary"]["total_info"] += len(result["info"])
        
        # Calculate overall score
        total_items = analysis_results["summary"]["files_analyzed"]
        if total_items > 0:
            analysis_results["summary"]["overall_score"] = max(0, 100 - 
                (analysis_results["summary"]["total_issues"] * 5 + 
                 analysis_results["summary"]["total_warnings"] * 2))
        
        # Identify critical issues
        analysis_results["critical_issues"] = [
            issue for file_result in analysis_results["files"]
            for issue in file_result["issues"]
            if "WARNING" in issue or "ERROR" in issue
        ][:20]  # Top 20 critical issues
        
        # Generate recommendations
        analysis_results["recommendations"] = self._generate_recommendations(analysis_results)
        
        return analysis_results
    
    def _generate_recommendations(self, results: Dict) -> List[str]:
        """Generate improvement recommendations"""
        recommendations = []
        summary = results["summary"]
        
        if summary["total_issues"] > 10:
            recommendations.append("HIGH PRIORITY: Fix all code issues before further development")
            
        if summary["total_warnings"] > 20:
            recommendations.append("MEDIUM PRIORITY: Address warnings to improve code quality")
            
        if summary["overall_score"] < 80:
            recommendations.append("Code quality score below 80 - consider refactoring")
            
        if not any("TODO" in str(f) for f in results["files"]):
            recommendations.append("Consider adding TODO comments for planned features")
            
        recommendations.append("Run full test suite after fixing critical issues")
        recommendations.append("Consider adding more unit tests for firmware functions")
        
        return recommendations


def run_python_compileall() -> Dict:
    """Run Python compileall check on the repository"""
    safe_log("Running Python compileall check...", "INFO")
    start = time.time()
    
    exit_code, stdout, stderr = run_command(
        [sys.executable, "-m", "compileall", ".", "-q"],
        cwd=REPO_ROOT,
        timeout=120
    )
    
    duration = time.time() - start
    
    result = {
        "exit_code": exit_code,
        "duration": format_duration(duration),
        "passed": exit_code == 0,
        "errors": []
    }
    
    if stderr:
        result["errors"] = stderr.strip().split('\n')
    
    safe_log(f"Compileall check: {'PASSED' if result['passed'] else 'FAILED'} ({result['duration']})", 
             "INFO" if result['passed'] else "ERROR")
    
    return result


def run_pytest_examples() -> Dict:
    """Run pytest on examples directory"""
    safe_log("Running pytest on examples/...", "INFO")
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
    
    # Parse pytest output
    result = {
        "exit_code": exit_code,
        "duration": format_duration(duration),
        "passed": exit_code == 0,
        "output": stdout,
        "errors": stderr
    }
    
    # Try to extract test counts
    passed_match = re.search(r'(\d+) passed', stdout)
    failed_match = re.search(r'(\d+) failed', stdout)
    
    if passed_match:
        result["tests_passed"] = int(passed_match.group(1))
    if failed_match:
        result["tests_failed"] = int(failed_match.group(1))
    
    safe_log(f"Pytest: {'PASSED' if result['passed'] else 'FAILED'} ({result['duration']})", 
             "INFO" if result['passed'] else "ERROR")
    
    return result


def run_platformio_build() -> Dict:
    """Run optional PlatformIO build (skipped if SKIP_PIO=1)"""
    if os.environ.get("SKIP_PIO") == "1":
        safe_log("Skipping PlatformIO build (SKIP_PIO=1)", "INFO")
        return {"skipped": True, "reason": "SKIP_PIO=1 environment variable set"}
    
    safe_log("Running PlatformIO build...", "INFO")
    
    # Check if platformio is available
    exit_code, _, _ = run_command(["pio", "--version"], timeout=10)
    if exit_code != 0:
        safe_log("PlatformIO not found, skipping build", "WARNING")
        return {"skipped": True, "reason": "PlatformIO not installed"}
    
    # Check if platformio.ini exists
    pio_ini = REPO_ROOT / "platformio.ini"
    if not pio_ini.exists():
        return {"skipped": True, "reason": "platformio.ini not found"}
    
    start = time.time()
    exit_code, stdout, stderr = run_command(
        ["pio", "run", "-e", "teensy41"],
        cwd=REPO_ROOT,
        timeout=600  # 10 minutes for build
    )
    duration = time.time() - start
    
    result = {
        "exit_code": exit_code,
        "duration": format_duration(duration),
        "passed": exit_code == 0,
        "output": stdout[-500:] if stdout else "",  # Last 500 chars
        "errors": stderr[-500:] if stderr else ""
    }
    
    safe_log(f"PlatformIO build: {'PASSED' if result['passed'] else 'FAILED'} ({result['duration']})", 
             "INFO" if result['passed'] else "ERROR")
    
    return result


def analyze_code_task(task_data: Dict) -> Dict:
    """Main analysis function for the agent"""
    safe_log("=== Starting Code Analysis ===", "INFO")
    start_time = time.time()
    
    # Run static code analysis
    analyzer = CodeAnalyzer()
    static_results = analyzer.analyze_project()
    
    # Run real checks
    checks = {
        "compileall": run_python_compileall(),
        "pytest": run_pytest_examples(),
        "platformio_build": run_platformio_build()
    }
    
    # Combine results
    results = {
        **static_results,
        "checks": checks,
        "task": task_data.get("name", "code_analysis"),
        "timestamp": str(datetime.datetime.now()),
        "total_duration": format_duration(time.time() - start_time)
    }
    
    # Determine overall status
    all_passed = all(
        check.get("passed", True) or check.get("skipped", False)
        for check in checks.values()
    )
    results["overall_status"] = "PASSED" if all_passed else "FAILED"
    
    # Write output
    output_file = write_json_output(results, "analyze_code.json")
    safe_log(f"Analysis complete: {results['overall_status']}", "INFO")
    safe_log(f"Results written to {output_file}", "INFO")
    
    return results


if __name__ == "__main__":
    try:
        task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    except json.JSONDecodeError:
        task_data = {}
    result = analyze_code_task(task_data)
    print(json.dumps(result, indent=2))
