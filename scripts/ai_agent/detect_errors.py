#!/usr/bin/env python3
"""
Error Detection Agent - Finds potential errors, faults, and failures
Free-tier compatible with comprehensive error checking
"""

import sys
import json
import re
import datetime
from pathlib import Path
from typing import Dict, List, Tuple, Optional

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


def detect_errors_task(task_data: Dict) -> Dict:
    """Main error detection function"""
    detector = ErrorDetector()
    results = detector.detect_project_errors()
    
    results["task"] = task_data.get("name", "error_detection")
    results["timestamp"] = str(datetime.datetime.now())
    
    return results


if __name__ == "__main__":
    try:
        task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    except json.JSONDecodeError:
        task_data = {}
    result = detect_errors_task(task_data)
    print(json.dumps(result, indent=2))
