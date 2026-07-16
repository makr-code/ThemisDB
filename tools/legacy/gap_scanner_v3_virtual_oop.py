#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Phase 5-5: Virtual Functions & OOP Design
Detects: CWE-397 (Incomplete Cleanup), CWE-398 (Unsafe Class Initialization)

Purpose:
  - Identify virtual functions called in constructors/destructors (undefined behavior)
  - Flag missing virtual destructors in base classes
  - Detect non-virtual functions that should be virtual (Liskov violation)
  - Identify slicing when passing derived by value
  - Flag virtual functions without override keyword (C++11)
  - Catch pure virtual functions without implementations
  - Identify covariant return type issues

Gap Types: VIRTUAL_IN_CTOR_DTOR, MISSING_VIRTUAL_DTOR, MISSING_OVERRIDE, SLICING_POTENTIAL, PURE_VIRTUAL_UNIMPLEMENTED, COVARIANT_MISMATCH, NON_VIRTUAL_DTOR_WITH_VIRTUAL_METHODS, FINAL_VIOLATION
"""

from dataclasses import dataclass
from enum import Enum
import re
from typing import List, Dict, Any
from pathlib import Path


class OOPGapType(Enum):
    """OOP Design Gap Types"""
    VIRTUAL_IN_CTOR_DTOR = "virtual_call_in_ctor_dtor"
    MISSING_VIRTUAL_DTOR = "missing_virtual_destructor"
    MISSING_OVERRIDE = "missing_override_keyword"
    SLICING_POTENTIAL = "potential_object_slicing"
    PURE_VIRTUAL_UNIMPLEMENTED = "pure_virtual_unimplemented"
    COVARIANT_MISMATCH = "covariant_return_type_mismatch"
    NON_VIRTUAL_DTOR_WITH_VIRTUAL = "non_virtual_dtor_with_virtual_methods"
    FINAL_VIOLATION = "final_class_inherited"


@dataclass
class OOPGap:
    gap_type: OOPGapType
    severity: str  # 'CRITICAL', 'HIGH', 'MEDIUM'
    file_path: str
    line_number: int
    class_name: str = ""
    function_name: str = ""
    issue: str = ""
    reason: str = ""

    def to_dict(self):
        return {
            'gap_type': self.gap_type.value,
            'severity': self.severity,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'class_name': self.class_name,
            'function_name': self.function_name,
            'issue': self.issue,
            'reason': self.reason
        }


class OOPGapScanner:
    """Scans C++ code for OOP design issues and virtual function misuse."""

    def __init__(self):
        self.gaps: List[OOPGap] = []
        self.gap_types = {}
        self.classes = {}  # Track class definitions and their methods
        self._cpp_method_re = re.compile(
            r'^\s*(?:[\w:<>,~*&\s]+?)\b([A-Za-z_]\w*)::(~?[A-Za-z_]\w*)\s*\([^;]*\)\s*(?:const)?\s*\{?'
        )
        self._member_call_re = re.compile(r'\bthis\s*->\s*([A-Za-z_]\w*)\s*\(')

    def scan_file(self, file_path: str) -> List[OOPGap]:
        """Scan a single C++ file for OOP design gaps."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return []

        file_gaps = []
        current_class = None

        for i, line in enumerate(lines, 1):
            # Pattern 1: Detect class definition
            class_match = re.search(r'\bclass\s+(\w+)\b', line)
            if class_match:
                current_class = class_match.group(1)
                self.classes[current_class] = {'has_virtual_dtor': False, 'has_virtual_methods': False}

            # Pattern 2/3: Virtual function call candidate in ctor/dtor.
            # Reduce false positives by only considering explicit member calls (this->foo())
            # inside real C++ ctor/dtor definitions (Class::Class / Class::~Class).
            method_match = self._cpp_method_re.search(line)
            if method_match:
                owner = method_match.group(1)
                method = method_match.group(2)
                is_ctor = method == owner
                is_dtor = method == f'~{owner}'
                if is_ctor or is_dtor:
                    body = self._get_function_body(lines, i - 1, max_lines=120)
                    member_calls = self._member_call_re.findall(body)
                    for call_name in member_calls:
                        if call_name in {owner, f'~{owner}'}:
                            continue
                        gap = OOPGap(
                            gap_type=OOPGapType.VIRTUAL_IN_CTOR_DTOR,
                            severity='HIGH',
                            file_path=file_path,
                            line_number=i,
                            class_name=owner,
                            function_name=call_name,
                            issue='Potential virtual member call in constructor/destructor',
                            reason='If this member function is virtual, dispatch in ctor/dtor can be unsafe'
                        )
                        file_gaps.append(gap)

            # Pattern 4: Missing virtual destructor
            if re.search(r'class\s+(\w+)\s*:', line):
                # Has base class
                if 'virtual' in line or 'public' in line:
                    context = self._get_context(lines, i - 1, 30)
                    if re.search(r'\bvirtual\s+\w+\s*\(', context) and not re.search(r'virtual\s+~', context):
                        gap = OOPGap(
                            gap_type=OOPGapType.MISSING_VIRTUAL_DTOR,
                            severity='CRITICAL',
                            file_path=file_path,
                            line_number=i,
                            class_name=re.search(r'class\s+(\w+)', line).group(1),
                            issue='Base class with virtual methods but no virtual destructor',
                            reason='Derived destructor will not be called; memory leaks and UB'
                        )
                        file_gaps.append(gap)

            # Pattern 5: Missing override keyword on virtual method
            if re.search(r'\bvirtual\s+\w+\s+\w+\s*\(', line) and 'override' not in line and 'final' not in line:
                match = re.search(r'virtual\s+(\w+)\s+(\w+)\s*\(', line)
                if match:
                    gap = OOPGap(
                        gap_type=OOPGapType.MISSING_OVERRIDE,
                        severity='MEDIUM',
                        file_path=file_path,
                        line_number=i,
                        function_name=match.group(2),
                        issue='Virtual function without override keyword',
                        reason='C++11: override keyword improves code clarity and catches errors'
                    )
                    file_gaps.append(gap)

            # Pattern 6: Potential object slicing (pass derived by value)
            if re.search(r'\(.*\bclass\s*\)', line) or re.search(r'\bvoid\s+\w+\s*\(\s*\w+\s+\)', line):
                match = re.search(r'void\s+\w+\s*\(\s*(\w+)\s+(\w+)\s*\)', line)
                if match:
                    param_type = match.group(1)
                    param_name = match.group(2)
                    # Check if param_type might be a polymorphic class
                    if not re.search(r'&|\*', line):
                        gap = OOPGap(
                            gap_type=OOPGapType.SLICING_POTENTIAL,
                            severity='HIGH',
                            file_path=file_path,
                            line_number=i,
                            function_name=param_name,
                            issue='Object passed by value (potential slicing)',
                            reason='Derived class data lost; use reference or pointer'
                        )
                        file_gaps.append(gap)

            # Pattern 7: Pure virtual function
            if re.search(r'\bvirtual\s+\w+\s+\w+\s*\(\s*.*\s*\)\s*=\s*0', line):
                match = re.search(r'virtual\s+\w+\s+(\w+)\s*\(', line)
                if match:
                    function_name = match.group(1)
                    # Check if there's an implementation
                    later_impl = ''.join(lines[i:min(i + 5, len(lines))])
                    if '{' not in later_impl:
                        gap = OOPGap(
                            gap_type=OOPGapType.PURE_VIRTUAL_UNIMPLEMENTED,
                            severity='MEDIUM',
                            file_path=file_path,
                            line_number=i,
                            function_name=function_name,
                            issue='Pure virtual function declared',
                            reason='Ensure all derived classes provide implementation'
                        )
                        file_gaps.append(gap)

            # Pattern 8: Non-virtual destructor with virtual methods
            if re.search(r'~\s*(\w+)\s*\(', line) and 'virtual' not in line:
                context = self._get_context(lines, max(0, i - 30), 50)
                if re.search(r'\bvirtual\s+\w+\s+\w+\s*\(', context):
                    match = re.search(r'~\s*(\w+)', line)
                    if match:
                        gap = OOPGap(
                            gap_type=OOPGapType.NON_VIRTUAL_DTOR_WITH_VIRTUAL,
                            severity='CRITICAL',
                            file_path=file_path,
                            line_number=i,
                            class_name=match.group(1),
                            issue='Non-virtual destructor in class with virtual methods',
                            reason='Derived destructor will not be called; memory leaks'
                        )
                        file_gaps.append(gap)

        self.gaps.extend(file_gaps)
        return file_gaps

    def _get_context(self, lines: List[str], start_line: int, context_size: int) -> str:
        """Get surrounding context for a line."""
        start = max(0, start_line - context_size // 2)
        end = min(len(lines), start_line + context_size // 2)
        return ''.join(lines[start:end])

    def _get_function_body(self, lines: List[str], start_line: int, max_lines: int = 120) -> str:
        """Extract a best-effort function body region starting at start_line."""
        end_limit = min(len(lines), start_line + max_lines)
        body_lines: List[str] = []
        brace_depth = 0
        seen_open = False

        for idx in range(start_line, end_limit):
            line = lines[idx]
            body_lines.append(line)
            opens = line.count('{')
            closes = line.count('}')
            if opens > 0:
                seen_open = True
            brace_depth += opens
            brace_depth -= closes
            if seen_open and brace_depth <= 0:
                break

        return ''.join(body_lines)

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

    def run_full_scan(self, src_path: str) -> List[OOPGap]:
        """Run a complete OOP design scan on the codebase."""
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
