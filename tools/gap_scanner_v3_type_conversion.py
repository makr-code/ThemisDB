#!/usr/bin/env python3
"""
Gap Scanner v3 — Type Conversion & Narrowing Detection (Phase 5-1)
CWE-190: Integer Overflow or Wraparound

Purpose:
  Detect unsafe type conversions, narrowing casts, and arithmetic operations
  that may result in integer overflow, truncation, or sign mismatches.

Patterns:
  1. Implicit narrowing: size_t → int, uint64_t → int, etc.
  2. Arithmetic before truncation: (a + b) assigned to smaller type
  3. Size calculations with potential overflow: width * height * depth
  4. Function returns assigned to smaller types (e.g., malloc, read, size())
  5. Conditional assignments with type mismatch
  6. Mixed signedness in arithmetic (signed + unsigned)
  7. Unsafe array indexing with user-controlled int
  8. Shift operations exceeding bit width

Author: ThemisDB Code Quality Initiative
License: Apache 2.0
"""

import json
import re
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from typing import List, Dict, Set, Optional


class TypeConversionGapType(Enum):
    """Enumeration of type conversion vulnerability patterns."""
    
    IMPLICIT_NARROWING = auto()  # size_t → int, uint64_t → int
    ARITHMETIC_OVERFLOW = auto()  # (a + b) → smaller type
    MULTIPLICATION_OVERFLOW = auto()  # width * height * depth without checks
    SIGNED_UNSIGNED_MISMATCH = auto()  # int x = -1; size_t y = x;
    FUNCTION_RETURN_TRUNCATION = auto()  # int len = malloc(...) >> truncated
    CAST_TO_SMALLER_TYPE = auto()  # (int)large_value, static_cast<int>(...)
    ARRAY_INDEX_OVERFLOW = auto()  # a[user_int] where user_int could overflow
    SHIFT_OVERFLOW = auto()  # x << (count >= bitwidth)


@dataclass
class TypeConversionGap:
    """Represents a single type conversion vulnerability."""
    
    gap_type: TypeConversionGapType
    severity: str  # CRITICAL, HIGH, MEDIUM, LOW
    file_path: str
    line_number: int
    line_content: str
    context: List[str] = field(default_factory=list)
    pattern_matched: str = ""
    conversion_from: str = ""  # Source type
    conversion_to: str = ""    # Target type
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
            "conversion_from": self.conversion_from,
            "conversion_to": self.conversion_to,
            "reason": self.reason,
        }


class TypeConversionGapScanner:
    """Scans C++ code for type conversion and integer overflow vulnerabilities."""

    # Patterns for type conversion vulnerabilities
    PATTERNS = {
        # Implicit narrowing conversions
        'implicit_narrowing': [
            r'(\w+\s+\w+\s*=\s*)(size_t|uint64_t|uint32_t|unsigned\s+long)\s+(\w+)',  # int x = size_t val
            r'(\w+\s+\w+\s*=\s*)(\w*::|)std::size_t\s+(\w+)',  # int len = std::size_t
        ],
        # Arithmetic overflow: (a + b) or (a * b) assigned to smaller type
        'arithmetic_overflow': [
            r'(\w+)\s*=\s*\(.*?[+\-*/%]\s*.*?\)',  # Simple arithmetic
            r'(\w+)\s*=\s*(\w+)\s*[+\-*]\s*(\w+)',  # Binary ops
            r'(\w+)\s*=\s*(\w+)\s*\*\s*(\w+)\s*\*\s*(\w+)',  # Triple multiplication
        ],
        # Multiplication overflow (common pattern: width * height * depth)
        'multiplication_overflow': [
            r'(\w+)\s*=\s*(\w+)\s*\*\s*(\w+)\s*\*\s*(\w+)',  # a * b * c
            r'malloc\s*\(\s*(\w+)\s*\*\s*(\w+)\s*\*\s*(\w+)\s*\)',  # malloc(w*h*d)
            r'new\s+\w+\s*\[\s*(\w+)\s*\*\s*(\w+)\s*\*\s*(\w+)\s*\]',  # new T[w*h*d]
        ],
        # Signed/unsigned mismatch
        'signed_unsigned_mismatch': [
            r'(\w+\s+\w+)\s*=\s*-1\s*;',  # Assigning -1 to size_t/unsigned
            r'(size_t|unsigned)\s+(\w+)\s*=\s*(\w+)\s*[+\-*]\s*(-?\d+)',  # Mixing signed/unsigned
        ],
        # Function return truncation: malloc, read, size(), length() → smaller type
        'function_return_truncation': [
            r'(int|short)\s+(\w+)\s*=\s*malloc\s*\(',  # int len = malloc(...)
            r'(int|short)\s+(\w+)\s*=\s*read\s*\(',  # int bytes = read(...)
            r'(int|short)\s+(\w+)\s*=\s*(\w+)\.size\s*\(',  # int len = vec.size()
            r'(int|short)\s+(\w+)\s*=\s*(\w+)\.length\s*\(',  # int len = str.length()
        ],
        # Explicit casts to smaller types
        'cast_to_smaller': [
            r'\(int\)\s*(\w+|.*?)',  # (int)value
            r'static_cast<int>\s*\(\s*(\w+)',  # static_cast<int>(value)
            r'reinterpret_cast<int\s*\*>\s*\(',  # reinterpret_cast<int*>(
        ],
        # Array indexing with potential overflow
        'array_index_overflow': [
            r'(\w+)\s*\[\s*(\w+)\s*\]',  # a[user_var] — simplistic
            r'(\w+)\s*\[\s*(\w+)\s*[+\-*]\s*(\d+)\s*\]',  # a[i+offset]
        ],
        # Shift operations exceeding bit width
        'shift_overflow': [
            r'(\w+)\s*<<\s*(\w+)',  # x << y (y could exceed bitwidth)
            r'(\w+)\s*>>\s*(\w+)',  # x >> y
        ],
    }

    def __init__(self):
        """Initialize the scanner."""
        self.gaps: List[TypeConversionGap] = []
        self.compiled_patterns = {
            key: [re.compile(p, re.MULTILINE) for p in patterns]
            for key, patterns in self.PATTERNS.items()
        }

    def scan_file(self, file_path: Path) -> List[TypeConversionGap]:
        """Scan a single C++ file for type conversion gaps."""
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
                    all_lines: List[str]) -> List[TypeConversionGap]:
        """Check a single line for type conversion issues."""
        line_gaps = []

        # Check implicit narrowing
        for pattern in self.compiled_patterns['implicit_narrowing']:
            if pattern.search(line):
                gap = TypeConversionGap(
                    gap_type=TypeConversionGapType.IMPLICIT_NARROWING,
                    severity="HIGH",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    reason="Implicit narrowing conversion detected (e.g., size_t → int)",
                )
                line_gaps.append(gap)

        # Check arithmetic overflow
        for pattern in self.compiled_patterns['arithmetic_overflow']:
            if pattern.search(line) and '=' in line and not '==' in line:
                gap = TypeConversionGap(
                    gap_type=TypeConversionGapType.ARITHMETIC_OVERFLOW,
                    severity="HIGH",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    reason="Arithmetic operation result assigned to variable (potential overflow)",
                )
                line_gaps.append(gap)

        # Check multiplication overflow
        if re.search(r'(\w+)\s*\*\s*(\w+)\s*\*\s*(\w+)', line):
            gap = TypeConversionGap(
                gap_type=TypeConversionGapType.MULTIPLICATION_OVERFLOW,
                severity="CRITICAL",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                reason="Multi-factor multiplication detected (CWE-190, likely overflow risk)",
            )
            line_gaps.append(gap)

        # Check function return truncation
        for pattern in self.compiled_patterns['function_return_truncation']:
            if pattern.search(line):
                gap = TypeConversionGap(
                    gap_type=TypeConversionGapType.FUNCTION_RETURN_TRUNCATION,
                    severity="CRITICAL",
                    file_path=str(file_path),
                    line_number=line_num,
                    line_content=line,
                    context=self._get_context(all_lines, line_num),
                    pattern_matched=line.strip(),
                    reason="Function return (malloc, read, size()) assigned to smaller int type",
                )
                line_gaps.append(gap)

        # Check explicit casts
        if 'static_cast<int>' in line or 'reinterpret_cast<int' in line or re.search(r'\(int\)', line):
            gap = TypeConversionGap(
                gap_type=TypeConversionGapType.CAST_TO_SMALLER_TYPE,
                severity="MEDIUM",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                reason="Explicit cast to int detected (verify no overflow on source)",
            )
            line_gaps.append(gap)

        # Check shift operations
        if re.search(r'<<|>>', line) and not line.strip().startswith('//'):
            gap = TypeConversionGap(
                gap_type=TypeConversionGapType.SHIFT_OVERFLOW,
                severity="MEDIUM",
                file_path=str(file_path),
                line_number=line_num,
                line_content=line,
                context=self._get_context(all_lines, line_num),
                pattern_matched=line.strip(),
                reason="Shift operation detected (verify shift count < bitwidth)",
            )
            line_gaps.append(gap)

        return line_gaps

    def _get_context(self, all_lines: List[str], line_num: int, context_size: int = 2) -> List[str]:
        """Get surrounding context lines."""
        start = max(0, line_num - context_size - 1)
        end = min(len(all_lines), line_num + context_size)
        return [l.rstrip() for l in all_lines[start:end]]

    def scan_module(self, module_path: Path) -> List[TypeConversionGap]:
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

    def run_full_scan(self, src_root: Path) -> List[TypeConversionGap]:
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
        print("Usage: python gap_scanner_v3_type_conversion.py <src_path>")
        sys.exit(1)

    src_path = Path(sys.argv[1])
    scanner = TypeConversionGapScanner()
    gaps = scanner.run_full_scan(src_path)

    stats = scanner.get_stats()
    print(f"Type Conversion Scanner Results:")
    print(f"  Total gaps: {stats['total_gaps']}")
    print(f"  By type: {stats['by_type']}")
    print(f"  By severity: {stats['by_severity']}")

    # Print first 10 gaps
    for gap in gaps[:10]:
        print(f"\n[{gap.gap_type.name}] {gap.file_path}:{gap.line_number}")
        print(f"  {gap.line_content.strip()}")


if __name__ == "__main__":
    main()
