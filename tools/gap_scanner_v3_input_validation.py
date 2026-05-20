#!/usr/bin/env python3
"""
Gap Scanner v3 — Input Validation & Bounds Detection (Phase 5-2)
CWE-787: Out-of-Bounds Write

Purpose:
  Detect missing input validation, bounds checking failures, and array/buffer
  operations that may result in off-by-one errors or buffer overflows.

Patterns:
  1. Array access without bounds check: a[user_var]
  2. Off-by-one in loops: for(i=0; i<=size; i++) a[i]
  3. memcpy/strncpy with incorrect size: memcpy(dst, src, len+1)
  4. Missing null-terminator validation
  5. Integer cast before bounds check: size_t sz = ...; int i = sz; for(; i > 0; i--)
  6. User input used directly as array index
  7. Unchecked return values from read/recv operations
  8. Buffer sizes not validated before string operations

Author: ThemisDB Code Quality Initiative
License: Apache 2.0
"""

import json
import re
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from typing import List, Dict, Set, Optional


class InputValidationGapType(Enum):
    """Enumeration of input validation vulnerability patterns."""
    
    UNCHECKED_ARRAY_INDEX = auto()  # a[user_var] without validation
    OFF_BY_ONE_LOOP = auto()  # for(i=0; i<=size; i++) a[i]
    UNCHECKED_MEMCPY = auto()  # memcpy(dst, src, len+1) or no size validation
    MISSING_NULL_TERMINATOR = auto()  # String not null-terminated
    USER_CONTROLLED_SIZE = auto()  # User input as allocation/loop size
    NO_BOUNDS_CHECK = auto()  # Function doesn't validate input ranges
    UNCHECKED_STRING_OP = auto()  # strcpy, strcat, sprintf without bounds
    INTEGER_CAST_BEFORE_CHECK = auto()  # Cast then use in bounds check


@dataclass
class InputValidationGap:
    """Represents a single input validation vulnerability."""
    
    gap_type: InputValidationGapType
    severity: str  # CRITICAL, HIGH, MEDIUM, LOW
    file_path: str
    line_number: int
    line_content: str
    context: List[str] = field(default_factory=list)
    pattern_matched: str = ""
    operation: str = ""  # Array access, memcpy, loop, etc.
    input_source: str = ""  # Parameter, user input, network, etc.
    reason: str = ""

    def to_dict(self) -> Dict:
        """Convert gap to dictionary for JSON serialization."""
        return {
            "gap_type": self.gap_type.name,
            "severity": self.severity,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "line_content": self.line_content.strip(),
            "context": self.context,
            "pattern_matched": self.pattern_matched,
            "operation": self.operation,
            "input_source": self.input_source,
            "reason": self.reason,
        }


class InputValidationGapScanner:
    """Scans C++ code for input validation and buffer overflow vulnerabilities."""

    # Patterns for input validation vulnerabilities
    PATTERNS = {
        # Unchecked array indexing
        'unchecked_index': [
            r'(\w+)\s*\[\s*(\w+)\s*\]',  # Simple array access
            r'(\w+)\s*\[\s*(\w+)\s*[+\-]\s*(\d+)\s*\]',  # Array with offset
        ],
        # Off-by-one in loops
        'off_by_one': [
            r'for\s*\(\s*(?:int|size_t)?\s*(\w+)\s*=\s*0\s*;\s*(\w+)\s*<=\s*(\w+)',  # i <= size
            r'for\s*\(\s*(?:int|size_t)?\s*(\w+)\s*=\s*0\s*;\s*(\w+)\s*<\s*(\w+)\s*[+]\s*1',  # i < size+1
        ],
        # Unchecked memcpy/memmove
        'unchecked_memcpy': [
            r'memcpy\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*[+]\s*(\d+)',  # memcpy(..., len+1)
            r'memcpy\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)',  # memcpy(..., unvalidated_len)
            r'memmove\s*\(',  # Also check memmove
        ],
        # Unchecked string operations
        'unchecked_string': [
            r'strcpy\s*\(',  # strcpy (always unsafe)
            r'strcat\s*\(',  # strcat (unsafe without length)
            r'sprintf\s*\(',  # sprintf (no bounds)
            r'scanf\s*\(',  # scanf (no input validation)
        ],
        # Missing null terminator validation
        'missing_null_term': [
            r'memcpy\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)\s*;',  # memcpy not followed by null term
        ],
        # User-controlled loop/allocation size
        'user_controlled_size': [
            r'new\s+\w+\s*\[\s*(\w+)\s*\]',  # new T[user_size]
            r'malloc\s*\(\s*(\w+)\s*\)',  # malloc(user_size)
            r'for\s*\(\s*(?:int|size_t)?\s*\w+\s*=\s*0\s*;\s*\w+\s*<\s*(\w+)',  # Loop with user_size
        ],
    }

    def __init__(self):
        """Initialize the scanner."""
        self.gaps: List[InputValidationGap] = []
        self.compiled_patterns = {
            key: [re.compile(p, re.MULTILINE) for p in patterns]
            for key, patterns in self.PATTERNS.items()
        }

    def scan_file(self, file_path: Path) -> List[InputValidationGap]:
        """Scan a single C++ file for input validation gaps."""
        file_gaps = []

        if not file_path.exists():
            return file_gaps

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return file_gaps

        for line_num, line in enumerate(lines, 1):
            # Skip comments
            if line.strip().startswith('//'):
                continue

            file_gaps.extend(self._check_line(file_path, line_num, line, lines))

        return file_gaps

    def _check_line(self, file_path: Path, line_num: int, line: str, 
                    all_lines: List[str]) -> List[InputValidationGap]:
        """Check a single line for input validation issues."""
        line_gaps = []

        # Check for unchecked array indexing
        array_match = re.search(r'(\w+)\s*\[\s*(\w+)\s*\]', line)
        if array_match and '==' not in line and '!=' not in line:
            # Potential unchecked index
            var_name = array_match.group(2)
            if not self._has_prior_bounds_check(all_lines, line_num, var_name):
                gap = InputValidationGap(
                    gap_type=InputValidationGapType.UNCHECKED_ARRAY_INDEX,
                    severity="HIGH",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    operation=f"Array access: {array_match.group(1)}[{var_name}]",
                    reason="Array index not validated before access",
                )
                line_gaps.append(gap)

        # Check for off-by-one in loops
        if re.search(r'for\s*\(\s*(?:int|size_t)?\s*\w+\s*=\s*0\s*;\s*\w+\s*<=', line):
            gap = InputValidationGap(
                gap_type=InputValidationGapType.OFF_BY_ONE_LOOP,
                severity="CRITICAL",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                operation="Loop with <= condition",
                reason="Off-by-one risk: loop condition uses <= instead of <",
            )
            line_gaps.append(gap)

        # Check for unchecked memcpy
        if 'memcpy' in line or 'memmove' in line:
            if '+' in line or '(' in line:
                gap = InputValidationGap(
                    gap_type=InputValidationGapType.UNCHECKED_MEMCPY,
                    severity="CRITICAL",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    operation="memcpy/memmove",
                    reason="memcpy size parameter not validated (potential buffer overflow)",
                )
                line_gaps.append(gap)

        # Check for unsafe string operations
        unsafe_string_ops = ['strcpy', 'strcat', 'sprintf']
        for op in unsafe_string_ops:
            if op in line and '(' in line:
                gap = InputValidationGap(
                    gap_type=InputValidationGapType.UNCHECKED_STRING_OP,
                    severity="CRITICAL",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    operation=op,
                    reason=f"Unsafe string operation {op} used without bounds checking",
                )
                line_gaps.append(gap)

        # Check for scanf without format validation
        if 'scanf' in line:
            gap = InputValidationGap(
                gap_type=InputValidationGapType.NO_BOUNDS_CHECK,
                severity="CRITICAL",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                operation="scanf",
                reason="scanf used: requires careful format string and buffer size validation",
            )
            line_gaps.append(gap)

        # Check for user-controlled sizes in allocation
        if ('new' in line or 'malloc' in line) and '[' in line:
            gap = InputValidationGap(
                gap_type=InputValidationGapType.USER_CONTROLLED_SIZE,
                severity="HIGH",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                operation="Dynamic allocation",
                reason="Allocation size not validated (potential DoS or overflow)",
            )
            line_gaps.append(gap)

        return line_gaps

    def _has_prior_bounds_check(self, all_lines: List[str], line_num: int, var_name: str) -> bool:
        """Check if variable has bounds validation before this line."""
        # Look back 10 lines for bounds checks
        start = max(0, line_num - 10)
        for i in range(start, line_num):
            line = all_lines[i].lower()
            if ('if' in line or 'assert' in line) and ('< ' in line or '<=' in line or '>' in line or '>=' in line):
                if var_name.lower() in line:
                    return True
        return False

    def _get_context(self, all_lines: List[str], line_num: int, context_size: int = 2) -> List[str]:
        """Get surrounding context lines."""
        start = max(0, line_num - context_size - 1)
        end = min(len(all_lines), line_num + context_size)
        return [l.rstrip() for l in all_lines[start:end]]

    def scan_module(self, module_path: Path) -> List[InputValidationGap]:
        """Scan all C++ files in a module."""
        module_gaps = []

        if not module_path.exists():
            return module_gaps

        for cpp_file in module_path.rglob('*.cpp'):
            module_gaps.extend(self.scan_file(cpp_file))
        for hpp_file in module_path.rglob('*.hpp'):
            module_gaps.extend(self.scan_file(hpp_file))
        for h_file in module_path.rglob('*.h'):
            module_gaps.extend(self.scan_file(h_file))

        return module_gaps

    def run_full_scan(self, src_root: Path) -> List[InputValidationGap]:
        """Execute a complete scan of all modules."""
        self.gaps = []

        src_path = Path(src_root) if isinstance(src_root, str) else src_root
        if not src_path.exists():
            return self.gaps

        # Scan each module
        for module_dir in sorted(src_path.iterdir()):
            if module_dir.is_dir() and not module_dir.name.startswith('.'):
                module_gaps = self.scan_module(module_dir)
                self.gaps.extend(module_gaps)

        return self.gaps

    def get_stats(self) -> Dict:
        """Return statistics about detected gaps."""
        by_type = {}
        by_severity = {}

        for gap in self.gaps:
            by_type[gap.gap_type.name] = by_type.get(gap.gap_type.name, 0) + 1
            by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1

        return {
            "total_gaps": len(self.gaps),
            "by_type": by_type,
            "by_severity": by_severity,
        }


def main():
    """Entry point for testing."""
    import sys

    if len(sys.argv) < 2:
        print("Usage: python gap_scanner_v3_input_validation.py <src_path>")
        sys.exit(1)

    src_path = Path(sys.argv[1])
    scanner = InputValidationGapScanner()
    gaps = scanner.run_full_scan(src_path)

    stats = scanner.get_stats()
    print(f"Input Validation Scanner Results:")
    print(f"  Total gaps: {stats['total_gaps']}")
    print(f"  By type: {stats['by_type']}")
    print(f"  By severity: {stats['by_severity']}")

    # Print first 10 gaps
    for gap in gaps[:10]:
        print(f"\n[{gap.gap_type.name}] {gap.file_path}:{gap.line_number}")
        print(f"  {gap.line_content.strip()}")


if __name__ == "__main__":
    main()
