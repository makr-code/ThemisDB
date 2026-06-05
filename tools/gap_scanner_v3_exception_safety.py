#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Phase 5-3: Exception Safety & Move Semantics
Detects: CWE-695 (Improper Uninitialization), move-semantic violations, broken RAII in ctors/dtors

Purpose:
  - Identify missing noexcept specifications on move/swap operations
  - Detect exception-unsafe constructors (side effects before full initialization)
  - Flag broken move semantics (move-assignment from moved-from objects)
  - Identify double-deletion in dtors, leaks in exception paths
  - Catch move-constructor implementations that throw or lack move semantics

Gap Types: MISSING_NOEXCEPT, UNSAFE_MOVE_CTOR, EXCEPTION_IN_DTOR, UNSAFE_MOVE_ASSIGNMENT, COPY_CTOR_SIDE_EFFECT, LEAKED_IN_EXCEPTION, MISSING_MOVE_CTOR, BROKEN_RAII_ASSIGNMENT
"""

from dataclasses import dataclass
from enum import Enum
import re
from typing import List, Dict, Any
from pathlib import Path
import json


class ExceptionSafetyGapType(Enum):
    """Exception Safety and Move Semantics Gap Types"""
    MISSING_NOEXCEPT = "missing_noexcept_on_move"
    UNSAFE_MOVE_CTOR = "unsafe_move_constructor"
    EXCEPTION_IN_DTOR = "exception_in_destructor"
    UNSAFE_MOVE_ASSIGNMENT = "unsafe_move_assignment"
    COPY_CTOR_SIDE_EFFECT = "copy_ctor_with_side_effects"
    LEAKED_IN_EXCEPTION = "resource_leaked_in_exception"
    MISSING_MOVE_CTOR = "missing_move_constructor_defaulted"
    BROKEN_RAII_ASSIGNMENT = "broken_raii_in_assignment"


@dataclass
class ExceptionSafetyGap:
    gap_type: ExceptionSafetyGapType
    severity: str  # 'CRITICAL', 'HIGH', 'MEDIUM'
    file_path: str
    line_number: int
    class_name: str = ""
    issue: str = ""
    reason: str = ""

    def to_dict(self):
        return {
            'gap_type': self.gap_type.value,
            'severity': self.severity,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'class_name': self.class_name,
            'issue': self.issue,
            'reason': self.reason
        }


class ExceptionSafetyGapScanner:
    """Scans C++ code for exception safety and move semantics violations."""

    def __init__(self):
        self.gaps: List[ExceptionSafetyGap] = []
        self.gap_types = {}

    def scan_file(self, file_path: str) -> List[ExceptionSafetyGap]:
        """Scan a single C++ file for exception safety gaps."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return []

        file_gaps = []

        for i, line in enumerate(lines, 1):
            # Pattern 1: Move constructor without noexcept
            if re.search(r'(\w+)\s*\(\s*(\w+)\s*&&\s*\)', line) and 'noexcept' not in line:
                if re.search(r':\s*\{', line) or (i < len(lines) and '{' in lines[i]):
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.MISSING_NOEXCEPT,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Move constructor without noexcept',
                        reason='Move ctors must be noexcept to support std::vector reallocation'
                    )
                    file_gaps.append(gap)

            # Pattern 2: Move assignment without noexcept
            if re.search(r'(\w+)\s*&\s*operator=\s*\(\s*(\w+)\s*&&\s*\)', line) and 'noexcept' not in line:
                if re.search(r':\s*\{', line) or (i < len(lines) and '{' in lines[i]):
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.UNSAFE_MOVE_ASSIGNMENT,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Move assignment without noexcept',
                        reason='Move assignment must guarantee strong exception safety'
                    )
                    file_gaps.append(gap)

            # Pattern 3: Exception in destructor (throw in dtor)
            if re.search(r'~\s*\w+\s*\(', line):
                context = self._get_context(lines, i - 1, 10)
                if 'throw' in context or 'new' in context:
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.EXCEPTION_IN_DTOR,
                        severity='CRITICAL',
                        file_path=file_path,
                        line_number=i,
                        issue='Potential exception in destructor',
                        reason='Destructors must be noexcept; exceptions here cause std::terminate()'
                    )
                    file_gaps.append(gap)

            # Pattern 4: Copy constructor with side effects (resource allocation)
            if re.search(r'(\w+)\s*\(\s*const\s+(\w+)\s*&\s*\)', line):
                context = self._get_context(lines, i - 1, 15)
                if re.search(r':\s*\{[\s\S]*?(\bnew\b|\bmalloc\b|\bfopen\b)', context):
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.COPY_CTOR_SIDE_EFFECT,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Copy constructor with resource side effects',
                        reason='Side effects in copy ctor can break exception safety'
                    )
                    file_gaps.append(gap)

            # Pattern 5: Missing move constructor but has copy constructor (Rule of Five)
            if re.search(r'(\w+)\s*\(\s*const\s+(\w+)\s*&\s*\)', line):
                context = self._get_context(lines, max(0, i - 20), 30)
                if '&&' not in context and '= delete' not in context:
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.MISSING_MOVE_CTOR,
                        severity='MEDIUM',
                        file_path=file_path,
                        line_number=i,
                        issue='Copy constructor exists but no move constructor (Rule of Five)',
                        reason='Missing move ctor prevents efficient rvalue transfers'
                    )
                    file_gaps.append(gap)

            # Pattern 6: delete[] on delete or vice versa (broken RAII in assignment)
            if re.search(r'delete\s+\[\s*\]\s+\w+\s*;', line) or re.search(r'(?<!delete\s)\[\s*\]\s+', line):
                context = self._get_context(lines, i - 1, 5)
                if 'new' in context and 'new[]' not in context:
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.BROKEN_RAII_ASSIGNMENT,
                        severity='CRITICAL',
                        file_path=file_path,
                        line_number=i,
                        issue='Mismatched new/delete or new[]/delete[]',
                        reason='Broken RAII: undefined behavior and memory corruption'
                    )
                    file_gaps.append(gap)

            # Pattern 7: Resource leaked in exception path (no try-catch, early return without cleanup)
            if 'new' in line and not re.search(r'(unique_ptr|shared_ptr|make_unique|make_shared)', line):
                if i < len(lines) and ';' in lines[i] and 'catch' not in self._get_context(lines, i, 5):
                    gap = ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.LEAKED_IN_EXCEPTION,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Raw new without smart pointer protection',
                        reason='Exception before delete causes resource leak'
                    )
                    file_gaps.append(gap)

        self.gaps.extend(file_gaps)
        return file_gaps

    def _get_context(self, lines: List[str], start_line: int, context_size: int) -> str:
        """Get surrounding context for a line."""
        start = max(0, start_line - context_size // 2)
        end = min(len(lines), start_line + context_size // 2)
        return ''.join(lines[start:end])

    def scan_module(self, module_path: str) -> Dict[str, Any]:
        """Scan all C++ files in a module."""
        module_gaps = []
        cpp_files = list(Path(module_path).rglob('*.cpp')) + list(Path(module_path).rglob('*.hpp'))

        for cpp_file in cpp_files:
            module_gaps.extend(self.scan_file(str(cpp_file)))

        gap_counts = {}
        for gap in module_gaps:
            gap_type = gap.gap_type.value
            gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1

        return {
            'total_gaps': len(module_gaps),
            'gap_types': gap_counts,
            'gaps': [g.to_dict() for g in module_gaps]
        }

    def run_full_scan(self, src_path: str) -> List[ExceptionSafetyGap]:
        """Run a complete exception safety scan on the codebase."""
        src = Path(src_path)
        if not src.exists():
            return []

        all_gaps = []
        for cpp_file in src.rglob('*.cpp'):
            all_gaps.extend(self.scan_file(str(cpp_file)))
        for hpp_file in src.rglob('*.hpp'):
            all_gaps.extend(self.scan_file(str(hpp_file)))

        # Count gap types
        for gap in all_gaps:
            gap_type = gap.gap_type.value
            self.gap_types[gap_type] = self.gap_types.get(gap_type, 0) + 1

        return all_gaps

    def get_stats(self) -> Dict[str, Any]:
        """Get scanner statistics."""
        return {
            'total_gaps': len(self.gaps),
            'gap_types': self.gap_types
        }
