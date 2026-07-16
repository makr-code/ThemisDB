#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Phase 5-4: Uninitialized Variables & Undefined Behavior
Detects: CWE-457 (Use of Uninitialized Variable), CWE-416 (Use-After-Free fragments)

Purpose:
  - Identify variables declared but not initialized before use
  - Detect pointer/reference use without null/validity checks
  - Flag conditional initialization paths (init in if, use outside)
  - Identify uninitialized struct/array fields
  - Catch use-after-free patterns and dangling pointers
  - Flag uninitialized class members in constructors

Gap Types: UNINITIALIZED_VAR, UNINITIALIZED_POINTER, UNINITIALIZED_MEMBER, CONDITIONAL_INIT_USE, USE_AFTER_FREE, DANGLING_REFERENCE, UNINITIALIZED_ARRAY, POINTER_WITHOUT_CHECK
"""

from dataclasses import dataclass
from enum import Enum
import re
from typing import List, Dict, Any
from pathlib import Path


class UninitializedGapType(Enum):
    """Uninitialized Variable Gap Types"""
    UNINITIALIZED_VAR = "uninitialized_variable"
    UNINITIALIZED_POINTER = "uninitialized_pointer"
    UNINITIALIZED_MEMBER = "uninitialized_member_field"
    CONDITIONAL_INIT_USE = "conditional_initialization_use"
    USE_AFTER_FREE = "use_after_free"
    DANGLING_REFERENCE = "dangling_reference"
    UNINITIALIZED_ARRAY = "uninitialized_array"
    POINTER_WITHOUT_CHECK = "pointer_without_null_check"


@dataclass
class UninitializedGap:
    gap_type: UninitializedGapType
    severity: str  # 'CRITICAL', 'HIGH', 'MEDIUM'
    file_path: str
    line_number: int
    var_name: str = ""
    context: str = ""
    reason: str = ""

    def to_dict(self):
        return {
            'gap_type': self.gap_type.value,
            'severity': self.severity,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'var_name': self.var_name,
            'context': self.context,
            'reason': self.reason
        }


class UninitializedGapScanner:
    """Scans C++ code for uninitialized variables and undefined behavior."""

    def __init__(self):
        self.gaps: List[UninitializedGap] = []
        self.gap_types = {}

    @staticmethod
    def _is_comment_or_preprocessor(line: str) -> bool:
        stripped = line.strip()
        return (
            not stripped or
            stripped.startswith('//') or
            stripped.startswith('/*') or
            stripped.startswith('*') or
            stripped.startswith('#')
        )

    @staticmethod
    def _has_bare_declaration(lines: List[str], line_num: int, var_name: str, window: int = 12) -> bool:
        start = max(0, line_num - window)
        decl_re = re.compile(rf'\b(?:const\s+)?[\w:<>\s*&]+\b{re.escape(var_name)}\s*;')
        for line in lines[start:line_num]:
            if decl_re.search(line) and '=' not in line and 'nullptr' not in line:
                return True
        return False

    @staticmethod
    def _has_pointer_declaration(lines: List[str], line_num: int, var_name: str, window: int = 12) -> bool:
        start = max(0, line_num - window)
        ptr_decl_re = re.compile(rf'\b[\w:<>\s*&]+\*\s*{re.escape(var_name)}\s*;')
        for line in lines[start:line_num]:
            if ptr_decl_re.search(line) and '=' not in line and 'nullptr' not in line:
                return True
        return False

    @staticmethod
    def _find_if_block(lines: List[str], if_line_index: int, lookahead: int = 24):
        depth = 0
        block_start = None
        end_limit = min(len(lines), if_line_index + min(lookahead, 4))

        for idx in range(if_line_index, end_limit):
            line = lines[idx]
            if '{' not in line and idx > if_line_index:
                stripped = line.strip()
                if stripped and not stripped.startswith('//') and not stripped.startswith('/*') and not stripped.startswith('*'):
                    break

            for ch in line:
                if ch == '{':
                    depth += 1
                    if block_start is None:
                        block_start = idx
                elif ch == '}' and depth > 0:
                    depth -= 1
                    if depth == 0 and block_start is not None:
                        return block_start, idx

        return None

    def scan_file(self, file_path: str) -> List[UninitializedGap]:
        """Scan a single C++ file for uninitialized variable gaps."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return []

        file_gaps = []

        for i, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            # Pattern 1: Pointer declared without initialization
            if re.search(r'\b\w+\s*\*\s*\w+\s*;', line) and '=' not in line and 'nullptr' not in line:
                match = re.search(r'(\w+\s*\*\s*(\w+))\s*;', line)
                if match:
                    var_name = match.group(2)
                    # Check if used later without check
                    if i < len(lines):
                        later_context = ''.join(lines[i:min(i + 5, len(lines))])
                        if re.search(rf'{var_name}\s*[-]>', later_context) or re.search(rf'{var_name}\s*\[', later_context):
                            gap = UninitializedGap(
                                gap_type=UninitializedGapType.UNINITIALIZED_POINTER,
                                severity='CRITICAL',
                                file_path=file_path,
                                line_number=i,
                                var_name=var_name,
                                context='Pointer declared but not initialized',
                                reason='Undefined behavior: potential null-pointer dereference'
                            )
                            file_gaps.append(gap)

            # Pattern 2: Variable declared in if-block, used outside
            if re.search(r'\bif\s*\(', line):
                block = self._find_if_block(lines, i - 1)
                if block:
                    block_start, block_end = block
                    block_text = ''.join(lines[block_start:block_end + 1])
                    matches = re.finditer(r'\b(?:const\s+)?[\w:<>\s*&]+\b(\w+)\s*=\s*[^;]+;', block_text)
                    for match in matches:
                        var_name = match.group(1)
                        if not self._has_bare_declaration(lines, block_start, var_name):
                            continue

                        pre_block = ''.join(lines[max(0, block_start - 5):block_start])
                        if re.search(rf'\b{re.escape(var_name)}\b\s*=', pre_block):
                            continue

                        later = ''.join(lines[block_end + 1:min(block_end + 8, len(lines))])
                        if re.search(rf'\b{re.escape(var_name)}\b', later):
                            gap = UninitializedGap(
                                gap_type=UninitializedGapType.CONDITIONAL_INIT_USE,
                                severity='HIGH',
                                file_path=file_path,
                                line_number=i,
                                var_name=var_name,
                                context='Variable initialized conditionally',
                                reason='Use outside conditional block may use uninitialized value'
                            )
                            file_gaps.append(gap)

            # Pattern 3: Struct with uninitialized fields
            if re.search(r'\bstruct\s+\w+\s*\{', line):
                context = self._get_context(lines, i - 1, 30)
                if re.search(r'\b(?:int|double|float|bool|char)\s+\w+\s*;', context) and '= {' not in context:
                    gap = UninitializedGap(
                        gap_type=UninitializedGapType.UNINITIALIZED_MEMBER,
                        severity='MEDIUM',
                        file_path=file_path,
                        line_number=i,
                        context='Struct with uninitialized fields',
                        reason='Default constructor does not initialize POD members'
                    )
                    file_gaps.append(gap)

            # Pattern 4: Array declared without initialization
            if re.search(r'\b\w+\s+\w+\[\d+\]\s*;', line) and '=' not in line and '{}' not in line:
                match = re.search(r'(\w+)\s+(\w+)\[\d+\]', line)
                if match:
                    var_name = match.group(2)
                    later = ''.join(lines[i:min(i + 5, len(lines))])
                    if re.search(rf'{var_name}\[', later):
                        gap = UninitializedGap(
                            gap_type=UninitializedGapType.UNINITIALIZED_ARRAY,
                            severity='HIGH',
                            file_path=file_path,
                            line_number=i,
                            var_name=var_name,
                            context='Array declared without initialization',
                            reason='Array contains garbage values; use with zeros or explicit init'
                        )
                        file_gaps.append(gap)

            # Pattern 5: delete followed by potential reuse (use-after-free)
            if 'delete' in line:
                later = ''.join(lines[i:min(i + 3, len(lines))])
                match = re.search(r'delete\s+(\w+)\s*;', line)
                if match:
                    var_name = match.group(1)
                    if re.search(rf'\b{var_name}\b', later) and '->' in later:
                        gap = UninitializedGap(
                            gap_type=UninitializedGapType.USE_AFTER_FREE,
                            severity='CRITICAL',
                            file_path=file_path,
                            line_number=i,
                            var_name=var_name,
                            context='Potential use-after-free',
                            reason='Undefined behavior: memory already freed'
                        )
                        file_gaps.append(gap)

            # Pattern 6: Pointer/reference without null check before use
            if re.search(r'->|\.|\[\s*\w+\s*\]', line):
                match = re.search(r'(\w+)\s*->', line)
                if match:
                    var_name = match.group(1)
                    if not self._has_pointer_declaration(lines, i - 1, var_name):
                        continue
                    if 'if' not in line and 'CHECK' not in line and 'assert' not in line:
                        context = self._get_context(lines, i - 3, 5)
                        if f'{var_name} =' not in context and 'nullptr' not in context and '?' not in context:
                            gap = UninitializedGap(
                                gap_type=UninitializedGapType.POINTER_WITHOUT_CHECK,
                                severity='HIGH',
                                file_path=file_path,
                                line_number=i,
                                var_name=var_name,
                                context='Pointer dereference without null check',
                                reason='Potential null-pointer dereference'
                            )
                            file_gaps.append(gap)

            # Pattern 7: Dangling reference (local address stored)
            if re.search(r'&\s*\w+\s*;', line) and 'this' not in line:
                if re.search(r'(\w+)\s*&\s*(\w+)\s*=\s*\w+\s*;', line):
                    match = re.search(r'(\w+)\s*&\s*(\w+)\s*=\s*&?\s*(\w+)', line)
                    if match:
                        ref_var = match.group(2)
                        source = match.group(3)
                        if re.search(rf'(return\s+{ref_var}|{ref_var}\.|\b{ref_var}\b)', ''.join(lines[i:min(i+5, len(lines))])):
                            gap = UninitializedGap(
                                gap_type=UninitializedGapType.DANGLING_REFERENCE,
                                severity='HIGH',
                                file_path=file_path,
                                line_number=i,
                                var_name=ref_var,
                                context='Reference to local or temporary',
                                reason='Dangling reference: object lifetime may end'
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

    def run_full_scan(self, src_path: str) -> List[UninitializedGap]:
        """Run a complete uninitialized variable scan on the codebase."""
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
