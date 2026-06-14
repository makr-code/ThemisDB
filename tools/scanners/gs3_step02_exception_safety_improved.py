#!/usr/bin/env python3
"""
Phase 6 - Exception Safety (IMPROVED)

Improvements:
1. Exclude test/benchmark/example files.
2. Reduce false positives for noexcept and moved-from patterns with tighter context checks.
3. Keep high-value checks: throwing destructors, raw new without RAII, missing noexcept on move ops.
"""

from dataclasses import dataclass
from enum import Enum
import re
from pathlib import Path
from typing import Any, Dict, List


class ExceptionSafetyGapType(Enum):
    MISSING_NOEXCEPT = 'missing_noexcept_on_move'
    EXCEPTION_IN_DTOR = 'exception_in_destructor'
    LEAKED_IN_EXCEPTION = 'resource_leaked_in_exception'


@dataclass
class ExceptionSafetyGap:
    gap_type: ExceptionSafetyGapType
    severity: str
    file_path: str
    line_number: int
    class_name: str = ''
    issue: str = ''
    reason: str = ''

    def to_dict(self):
        return {
            'gap_type': self.gap_type.value,
            'severity': self.severity,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'class_name': self.class_name,
            'issue': self.issue,
            'reason': self.reason,
        }


class ExceptionSafetyGapScannerImproved:
    """Improved exception safety scanner with production filters."""

    def __init__(self):
        self.gaps: List[ExceptionSafetyGap] = []
        self.gap_types: Dict[str, int] = {}

    @staticmethod
    def _is_non_prod_path(file_path: str) -> bool:
        p = file_path.replace('\\', '/').lower()
        markers = ['tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.', 'examples/', 'demo_']
        return any(m in p for m in markers)

    @staticmethod
    def _is_comment_or_pp(line: str) -> bool:
        s = line.strip()
        return not s or s.startswith('//') or s.startswith('/*') or s.startswith('*') or s.startswith('#')

    @staticmethod
    def _get_context(lines: List[str], idx0: int, window: int) -> str:
        start = max(0, idx0 - window)
        end = min(len(lines), idx0 + window + 1)
        return ''.join(lines[start:end])

    def scan_file(self, file_path: str) -> List[ExceptionSafetyGap]:
        if self._is_non_prod_path(file_path):
            return []

        try:
            lines = Path(file_path).read_text(encoding='utf-8', errors='ignore').splitlines(keepends=True)
        except Exception:
            return []

        file_gaps: List[ExceptionSafetyGap] = []

        for i, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue

            # Move ctor without noexcept
            if re.search(r'\b\w+\s*\(\s*\w+\s*&&\s*\w*\s*\)', line) and 'noexcept' not in line:
                ctx = self._get_context(lines, i - 1, 2)
                if '{' in ctx or ':' in line:
                    file_gaps.append(ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.MISSING_NOEXCEPT,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Move constructor without noexcept',
                        reason='Move ctors should be noexcept for strong container guarantees',
                    ))

            # Move assignment without noexcept
            if re.search(r'\boperator=\s*\(\s*\w+\s*&&\s*\w*\s*\)', line) and 'noexcept' not in line:
                file_gaps.append(ExceptionSafetyGap(
                    gap_type=ExceptionSafetyGapType.MISSING_NOEXCEPT,
                    severity='HIGH',
                    file_path=file_path,
                    line_number=i,
                    issue='Move assignment without noexcept',
                    reason='Move assignment should be noexcept when possible',
                ))

            # Destructor with throw/new in body context
            if re.search(r'~\s*\w+\s*\(', line):
                ctx = self._get_context(lines, i - 1, 8)
                if 'throw' in ctx:
                    file_gaps.append(ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.EXCEPTION_IN_DTOR,
                        severity='CRITICAL',
                        file_path=file_path,
                        line_number=i,
                        issue='Potential exception in destructor',
                        reason='Throwing in destructor may trigger std::terminate()',
                    ))

            # raw new without RAII nearby
            if re.search(r'\bnew\b', line) and not re.search(r'unique_ptr|shared_ptr|make_unique|make_shared', line):
                ctx = self._get_context(lines, i - 1, 3)
                if 'catch' not in ctx and 'unique_ptr' not in ctx and 'shared_ptr' not in ctx:
                    file_gaps.append(ExceptionSafetyGap(
                        gap_type=ExceptionSafetyGapType.LEAKED_IN_EXCEPTION,
                        severity='HIGH',
                        file_path=file_path,
                        line_number=i,
                        issue='Raw new without RAII protection',
                        reason='Exception path may leak resource',
                    ))

        self.gaps.extend(file_gaps)
        return file_gaps

    def run_full_scan(self, src_path: str) -> List[ExceptionSafetyGap]:
        root = Path(src_path)
        if not root.exists():
            return []

        all_gaps: List[ExceptionSafetyGap] = []
        for ext in ('*.cpp', '*.cc', '*.cxx', '*.hpp', '*.hh', '*.hxx', '*.h'):
            for p in root.rglob(ext):
                all_gaps.extend(self.scan_file(str(p)))

        for gap in all_gaps:
            gt = gap.gap_type.value
            self.gap_types[gt] = self.gap_types.get(gt, 0) + 1

        return all_gaps

    def get_stats(self) -> Dict[str, Any]:
        return {'total_gaps': len(self.gaps), 'gap_types': self.gap_types}
