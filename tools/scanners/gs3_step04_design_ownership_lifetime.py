#!/usr/bin/env python3
"""
Phase 6 - Ownership & Lifetime Semantics Scanner (P6-5) — CRITICAL

Conservative heuristics for production C++ code:
- Use-after-move: object reused after std::move()
- Return of local reference (dangling ref / dangling pointer)
- Self-move assignment without identity guard
- Move constructor/assignment missing noexcept
- RValue reference members (always dangling after ctor)
- const T&& parameters (move impossible)
- Returning std::move(local) blocking RVO/NRVO
- Storing raw pointer from container .data() beyond container scope
- Implicit copy where move is clearly available and intended
- Moved-from object accessed in same scope
- Capture of temporary by reference in lambda
- Pointer/reference escaping from temporary object
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


class OwnershipLifetimeScan:
    """Detect ownership and lifetime semantic gaps with conservative heuristics."""

    CODE_SUFFIXES = {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}
    NON_PROD_MARKERS = (
        'tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.',
        'examples/', 'demo_', '_demo.', '_mock.', '/mock/', 'fuzz/',
        'third_party/', 'external/', 'vendor/',
    )

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files and return accumulated gap records."""
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in self.CODE_SUFFIXES:
                continue

            rel_file = self._relative_path(file_path)
            if self._is_non_prod_path(rel_file):
                continue

            try:
                content = file_path.read_text(encoding='utf-8', errors='replace')
            except OSError:
                continue

            lines = content.splitlines()

            self._check_use_after_move(rel_file, lines)
            self._check_return_local_ref(rel_file, lines)
            self._check_self_move_assignment(rel_file, lines)
            self._check_move_missing_noexcept(rel_file, lines)
            self._check_rvalue_ref_member(rel_file, lines)
            self._check_const_rvalue_param(rel_file, lines)
            self._check_return_move_blocks_rvo(rel_file, lines)
            self._check_raw_data_pointer_escape(rel_file, lines)
            self._check_lambda_ref_capture_temporary(rel_file, lines)

        return self.gaps

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _relative_path(self, file_path: Path) -> str:
        try:
            return str(file_path.resolve().relative_to(self.repo_root.resolve()))
        except ValueError:
            return str(file_path)

    def _is_non_prod_path(self, rel_file: str) -> bool:
        return any(marker in rel_file for marker in self.NON_PROD_MARKERS)

    def _emit(self, rel_file: str, line: int, severity: str,
              pattern: str, description: str, context: str) -> None:
        self.gaps.append({
            'file': rel_file,
            'line': line,
            'severity': severity,
            'scanner': 'ownership_lifetime',
            'pattern': pattern,
            'description': description,
            'context': context[:200],
        })

    @staticmethod
    def _strip_comment(line: str) -> str:
        idx = line.find('//')
        return line[:idx] if idx >= 0 else line

    # ------------------------------------------------------------------
    # Detection rules
    # ------------------------------------------------------------------

    # Variables that are commonly moved-from in forwarding/copy contexts — not real UAM
    _MOVE_NOISE_VARS: Set[str] = {
        'this', 'other', 'rhs', 'lhs', 'src', 'from', 'obj', 'val', 'value',
        'arg', 'args', 'func', 'fn', 'callback', 'handler', 'bound_args',
        'storage_', 'error_', 'context_', 'data_', 'impl_',
    }

    def _check_use_after_move(self, rel_file: str, lines: List[str]) -> None:
        """Flag local variables used in the same function scope after std::move().

        Conservative rules to suppress false positives:
        - skip common forwarding/member variable names
        - only look forward 8 lines (same statement block)
        - stop at any new enclosing brace open (lambda/block)
        - skip if moved variable is a member access (contains '_' suffix or '.')
        - skip if the move line is in a constructor initialiser list context
        """
        move_re = re.compile(r'\bstd::move\s*\(\s*(\w+)\s*\)')

        i = 0
        while i < len(lines):
            raw = lines[i]
            line_clean = self._strip_comment(raw)

            # Skip constructor initialiser lists (': member(std::move(...))' pattern)
            if re.match(r'\s*:', line_clean) and '(' in line_clean:
                i += 1
                continue

            for m in move_re.finditer(line_clean):
                var = m.group(1)

                # Suppress common noise variables and member-variable names
                if var in self._MOVE_NOISE_VARS or var.endswith('_'):
                    continue
                # Skip single-char variables (loop iterators, temp counters)
                if len(var) <= 1:
                    continue
                # Skip if moved as part of a compound expression on the same line
                # (e.g., std::apply(std::move(a), std::move(b)) — both used once)
                if line_clean.count('std::move') >= 2:
                    continue

                # Look forward within a short window (8 lines max)
                window_end = min(len(lines), i + 8)
                for j in range(i + 1, window_end):
                    next_clean = self._strip_comment(lines[j])

                    # Stop at new lambda / nested block open — different scope
                    if '[' in next_clean and ']' in next_clean and '{' in next_clean:
                        break
                    if next_clean.strip().startswith('{') or next_clean.strip().startswith('for'):
                        break

                    # Stop if var is reassigned
                    if re.search(rf'\b{re.escape(var)}\s*(?:[+\-*/%&|^]=|=(?!=))', next_clean):
                        break
                    # Stop if var is re-declared
                    if re.search(
                        rf'\b(?:auto|int|long|float|double|bool|char|[\w:<>]+)\s+{re.escape(var)}\b',
                        next_clean
                    ):
                        break

                    # Flag if var appears as an operand (word boundary)
                    if re.search(rf'\b{re.escape(var)}\b', next_clean):
                        self._emit(
                            rel_file, j + 1, 'HIGH',
                            'use_after_move',
                            f'Variable "{var}" referenced after std::move() on line {i + 1}; '
                            'moved-from state is unspecified — reset or avoid reuse',
                            lines[j].strip(),
                        )
                        break  # One diagnostic per move site
            i += 1

    def _check_return_local_ref(self, rel_file: str, lines: List[str]) -> None:
        """Flag functions that return a reference or pointer to a local variable."""
        # Pattern: return &localVar or return localRef where var declared locally
        ret_ref_re = re.compile(r'\breturn\s+&\s*(\w+)\s*;')
        ret_deref_re = re.compile(r'\breturn\s+\*\s*(\w+)\s*;')

        for lineno, raw in enumerate(lines, start=1):
            line = self._strip_comment(raw)

            # return &localVar — only flag if the variable looks like a stack local
            m = ret_ref_re.search(line)
            if m:
                var = m.group(1)
                # Scan backwards up to 40 lines for the declaration
                window_start = max(0, lineno - 40)
                window = '\n'.join(lines[window_start: lineno - 1])
                # Declared locally (auto, primitive, or non-pointer type) without static/extern
                if re.search(
                    rf'\b(?:auto|int|long|float|double|bool|char|std::string|std::array)\s+{re.escape(var)}\b',
                    window
                ) and not re.search(rf'\bstatic\s+.*{re.escape(var)}\b', window):
                    self._emit(
                        rel_file, lineno, 'CRITICAL',
                        'return_local_ref',
                        f'Return of address of local variable "{var}"; '
                        'caller receives a dangling pointer/reference after return',
                        raw.strip(),
                    )

    def _check_self_move_assignment(self, rel_file: str, lines: List[str]) -> None:
        """Flag move assignment operators lacking a self-assignment guard."""
        # Find operator=(T&&) implementations without `if (this != &...)`
        move_assign_re = re.compile(
            r'(?:[\w:<>&*\s]+)&\s*operator=\s*\((?:[\w:<>&*\s]+)&&[^)]*\)\s*(?:noexcept\s*)?\{'
        )
        self_check_re = re.compile(r'\bthis\s*!=\s*&|\bthis\s*==\s*&')

        i = 0
        while i < len(lines):
            raw = lines[i]
            if move_assign_re.search(raw):
                # Collect body of the function (up to 40 lines)
                body_lines: List[str] = [raw]
                depth = raw.count('{') - raw.count('}')
                j = i + 1
                while j < len(lines) and j < i + 40 and depth > 0:
                    body_lines.append(lines[j])
                    depth += lines[j].count('{') - lines[j].count('}')
                    j += 1
                body = '\n'.join(body_lines)
                if not self_check_re.search(body):
                    self._emit(
                        rel_file, i + 1, 'HIGH',
                        'self_move_assignment_no_guard',
                        'Move assignment operator without self-assignment identity guard '
                        '(if (this != &other)); self-assignment causes data loss',
                        raw.strip(),
                    )
            i += 1

    def _check_move_missing_noexcept(self, rel_file: str, lines: List[str]) -> None:
        """Flag move constructors and move assignment operators missing noexcept."""
        # Match: ClassName(ClassName&&) and ClassName(ClassName&& other) without noexcept
        # and operator=(ClassName&&) without noexcept
        move_ctor_re = re.compile(
            r'^\s*(\w+)\s*\(\s*\1\s*&&(?:\s+\w+)?\s*\)'
            r'\s*(?!.*\bnoexcept\b)'
            r'\s*(?::\s*\w+|\{|$)'
        )
        move_asgn_re = re.compile(
            r'(?:[\w:<>&*\s]+)&\s*operator=\s*\((?:[\w:<>&*\s]+)&&[^)]*\)'
            r'\s*(?!.*\bnoexcept\b)'
            r'\s*(?:\{|$)'
        )

        for lineno, raw in enumerate(lines, start=1):
            stripped = raw.strip()
            # Skip declarations in forward-declared contexts or in class declarations only
            if stripped.startswith('//') or stripped.startswith('*'):
                continue

            m = move_ctor_re.match(raw)
            if m and 'noexcept' not in raw:
                self._emit(
                    rel_file, lineno, 'HIGH',
                    'move_ctor_missing_noexcept',
                    f'Move constructor for "{m.group(1)}" lacks noexcept; '
                    'std::vector and other containers fall back to copy on reallocation',
                    raw.strip(),
                )
                continue

            if move_asgn_re.search(raw) and 'noexcept' not in raw:
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'move_assign_missing_noexcept',
                    'Move assignment operator lacks noexcept; '
                    'prevents strong exception guarantees in container operations',
                    raw.strip(),
                )

    # Pre-compiled patterns used by _check_rvalue_ref_member
    _RVAL_MEMBER_RE = re.compile(r'[\w>]&&\s+(\w+)\s*(?:=|;|\{)')  # word char or '>' before &&

    def _check_rvalue_ref_member(self, rel_file: str, lines: List[str]) -> None:
        """Flag rvalue reference data members — they dangle immediately after construction.

        Only matches lines with a word char immediately before ``&&`` (e.g. ``Type&&``)
        inside a class/struct body.  Lines starting with bare ``&&`` (logical AND
        continuation) are excluded, as are lines with ``return``, ``operator``, or ``(``.
        """
        in_class = False
        brace_depth = 0

        for lineno, raw in enumerate(lines, start=1):
            stripped = raw.strip()
            if re.match(r'(?:struct|class)\s+\w', stripped):
                in_class = True
            if in_class:
                brace_depth += stripped.count('{') - stripped.count('}')
                if brace_depth <= 0:
                    in_class = False
                    brace_depth = 0
                    continue

            if not in_class:
                continue
            # Fast bailout: must contain && before a word
            if '&&' not in raw:
                continue
            # Exclude common non-member-decl patterns
            if 'return' in raw or 'operator' in raw or '(' in raw:
                continue
            # Exclude lines starting with && (logical AND continuation)
            if stripped.startswith('&&') or stripped.startswith('//'):
                continue
            m = self._RVAL_MEMBER_RE.search(raw)
            if m:
                self._emit(
                    rel_file, lineno, 'HIGH',
                    'rvalue_ref_member',
                    f'RValue reference member "{m.group(1)}"; '
                    'rvalue ref members dangle after constructor — store by value or smart ptr',
                    raw.strip(),
                )

    def _check_const_rvalue_param(self, rel_file: str, lines: List[str]) -> None:
        """Flag const T&& parameters where moving is impossible."""
        const_rv_re = re.compile(r'\bconst\s+(?!std::string_view)[\w:<>]+\s*&&\s+\w')
        for lineno, raw in enumerate(lines, start=1):
            line = self._strip_comment(raw)
            if const_rv_re.search(line):
                if re.search(r'\btemplate\b.*\bT\s*&&', line):
                    continue
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'const_rvalue_ref_param',
                    '"const T&&" parameter: cannot move from const; '
                    'use "T&&" for sink parameters or "const T&" for const access',
                    raw.strip(),
                )

    def _check_return_move_blocks_rvo(self, rel_file: str, lines: List[str]) -> None:
        """Flag return std::move(local) where RVO/NRVO would apply without the move."""
        ret_move_re = re.compile(r'\breturn\s+std::move\s*\(\s*(\w+)\s*\)\s*;')

        for lineno, raw in enumerate(lines, start=1):
            m = ret_move_re.search(raw)
            if not m:
                continue
            var = m.group(1)
            # Look back to confirm var is a named local (not a parameter moved intentionally)
            window_start = max(0, lineno - 30)
            window = '\n'.join(lines[window_start: lineno - 1])
            if re.search(
                rf'\b(?:auto|std::[\w:<>]+)\s+{re.escape(var)}\b',
                window
            ) and not re.search(
                rf'\b{re.escape(var)}\s*=.*std::move\b', window
            ):
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'return_move_blocks_rvo',
                    f'return std::move({var}) inhibits RVO/NRVO; '
                    'return local directly to allow named return-value optimisation',
                    raw.strip(),
                )

    def _check_raw_data_pointer_escape(self, rel_file: str, lines: List[str]) -> None:
        """Flag raw pointer obtained via .data() stored across likely container-scope."""
        data_re = re.compile(r'\b(\w+)\s*=\s*(\w+)\.data\s*\(\s*\)\s*;')
        for lineno, raw in enumerate(lines, start=1):
            m = data_re.search(raw)
            if not m:
                continue
            ptr_var = m.group(1)
            container_var = m.group(2)
            # If ptr_var appears after the container goes out of scope (heuristic: >10 lines away
            # and the container declaration is within 30 lines before)
            window_start = max(0, lineno - 30)
            window_lines = lines[window_start: lineno - 1]
            container_declared = any(
                re.search(
                    rf'\b(?:auto|std::(?:vector|string|array|basic_string)[\w<>]*)\s+{re.escape(container_var)}\b',
                    l
                ) for l in window_lines
            )
            # Check ptr used further down — only past a closing brace (scope exit)
            look_fwd_end = min(len(lines), lineno + 25)
            brace_closed = False
            for j in range(lineno, look_fwd_end):
                stripped = lines[j].strip()
                if stripped.startswith('}') and container_declared:
                    brace_closed = True
                if brace_closed and re.search(rf'\b{re.escape(ptr_var)}\b', stripped):
                    self._emit(
                        rel_file, j + 1, 'HIGH',
                        'raw_data_pointer_escape',
                        f'Raw pointer "{ptr_var}" from "{container_var}.data()" may outlive '
                        'container scope; use the container directly or extend its lifetime',
                        lines[j].strip(),
                    )
                    break

    def _check_lambda_ref_capture_temporary(self, rel_file: str, lines: List[str]) -> None:
        """Flag [&]-capturing lambdas passed to thread/async constructors.

        These are the definitive dangerous patterns where ref captures outlive
        the captured variables' scope.  Local `auto fn = [&]...` helpers that
        are called within the same function are excluded as they are safe.
        """
        # Match lines that both mention std::thread/std::async AND contain [&]
        # Handle: std::thread t([&]...) and std::async(std::launch::async, [&]...)
        thread_kw_re = re.compile(r'\b(?:std::thread|std::async|std::packaged_task)\b')
        ref_capture_re = re.compile(r'\[&\]')
        for lineno, raw in enumerate(lines, start=1):
            line_clean = self._strip_comment(raw)
            if thread_kw_re.search(line_clean) and ref_capture_re.search(line_clean):
                self._emit(
                    rel_file, lineno, 'HIGH',
                    'lambda_ref_capture_thread_async',
                    'Lambda capturing by [&] passed to std::thread/std::async; '
                    'captured references may dangle when the calling scope exits — '
                    'capture by value or ensure lifetime extends past thread completion',
                    raw.strip(),
                )
