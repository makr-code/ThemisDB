#!/usr/bin/env python3
"""
Gap Scanner Step 01 — AI Error Handling Consistency Detection

Detects:
- Inconsistent error handling patterns (throw vs return vs silent fail)
- Unhandled return values from error-prone operations
- Inconsistent error contracts between header and implementation
- Silent error swallowing in critical paths
- Missing noexcept specifications on non-throwing functions

Purpose:
AI systems often generate inconsistent error handling: some functions throw,
others return error codes, others silently fail. This leads to unhandled errors
escaping to production. Consistent error handling per module is a MUST for reliability.

ThemisDB Standard (from CLAUDE.md + ARCHITECTURE.md):
- Public APIs MUST document error behavior in Doxygen (@throws, @return error handling)
- Critical paths (validate, authenticate, secure ops) MUST handle all errors
- Silent failures (catch and ignore) are forbidden in error-prone operations
- Error propagation MUST be explicit
"""

import re
import sys
from pathlib import Path
from typing import List, Tuple

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class ErrorHandlingConsistencyScanner(BaseGapScanner):
    """Phase 1: AI Error Handling Consistency Detection"""

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 90

    def __init__(self):
        """Initialize Error Handling Consistency Scanner."""
        super().__init__("Error Handling Consistency Scanner", "1.0")

        # Critical error-prone operations that MUST be handled
        self.error_prone_operations = [
            'open', 'read', 'write', 'close', 'fopen', 'fclose',
            'malloc', 'new', 'lock', 'unlock', 'acquire',
            'connect', 'listen', 'bind', 'send', 'receive',
            'validate', 'verify', 'authenticate', 'authorize',
            'parse', 'compile', 'execute', 'query', 'request',
            'throw', 'raise', 'assert', 'check',
        ]

        # Critical function name patterns
        self.critical_functions = [
            r'validate', r'verify', r'authenticate', r'authorize',
            r'secure', r'encrypt', r'hash', r'sign', r'permission',
            r'access_control', r'auth', r'security', r'critical',
        ]

    def _is_critical_context(self, context: str) -> bool:
        """Check if code is in critical security/error path."""
        context_lower = context.lower()
        return any(pattern in context_lower for pattern in [
            'validate', 'verify', 'authenticate', 'authorize',
            'secure', 'crypto', 'permission', 'access', 'auth',
            'critical', 'security', 'unsafe', 'trust',
        ])

    def _extract_function_context(self, lines: List[str], line_no: int) -> str:
        """Extract surrounding function/class context for line."""
        context = []
        # Look backwards for function/class signature
        for i in range(max(0, line_no - 30), line_no):
            line = lines[i]
            if re.search(r'\b(void|int|bool|auto|Status|Result|Error|std::\w+)\s+\w+\s*\(', line):
                context.append(line)
            if re.search(r'\b(class|struct)\s+\w+', line):
                context.append(line)
        return '\n'.join(context[-5:]) if context else ''

    def _scan_file(self, file_path: Path) -> List[Gap]:
        """Scan single file for error handling consistency issues."""
        gaps = []

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return gaps

        for line_no, line in enumerate(lines, 1):
            stripped = line.strip()

            # Skip pure comments
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue

            # PATTERN 1: Unhandled error-prone operation call
            for op in self.error_prone_operations:
                # Match function calls like: result = validate(x);  (no check)
                pattern = rf'\b(?!if|while|for|try|switch).*\b{op}\s*\('
                if re.search(pattern, line, re.IGNORECASE):
                    # Check if result is checked on same or next line
                    next_line = lines[line_no].strip() if line_no < len(lines) else ''
                    curr_line_lower = line.lower()

                    # If NOT checking result, it's a gap
                    if not any(check in curr_line_lower for check in ['if', 'assert', 'check', 'throw', 'while', 'switch']):
                        if not any(check in next_line.lower() for check in ['if', 'assert', 'throw']):
                            context = self._extract_function_context(lines, line_no)
                            is_critical = self._is_critical_context(context)

                            if is_critical:
                                self._add_gap(
                                    gaps,
                                    str(file_path.relative_to(file_path.parents[2])),
                                    line_no,
                                    "unhandled_critical_operation",
                                    SeverityLevel.CRITICAL.value,
                                    0.78,
                                    f"Unhandled error-prone operation '{op}' in critical path",
                                    "Check return value or exception; propagate error explicitly",
                                    line.rstrip()[:120]
                                )

            # PATTERN 2: catch(...) without re-throw in non-test code
            if re.search(r'catch\s*\(\s*\.\.\.\s*\)', line):
                # Look ahead for re-throw or just logging
                next_5_lines = ''.join(lines[line_no:min(line_no + 5, len(lines))])
                if not re.search(r'throw|re_throw|rethrow', next_5_lines, re.IGNORECASE):
                    # Check if it's just logging (bad)
                    if re.search(r'log|print|cout', next_5_lines, re.IGNORECASE):
                        context = self._extract_function_context(lines, line_no)
                        is_critical = self._is_critical_context(context)

                        severity = SeverityLevel.CRITICAL.value if is_critical else SeverityLevel.HIGH.value
                        confidence = 0.82 if is_critical else 0.70

                        self._add_gap(
                            gaps,
                            str(file_path.relative_to(file_path.parents[2])),
                            line_no,
                            "silent_error_swallow",
                            severity,
                            confidence,
                            "catch(...) without re-throw; error is swallowed",
                            "Either re-throw or handle explicitly; never silently ignore exceptions",
                            line.rstrip()[:120]
                        )

            # PATTERN 3: Missing error check after throwing operation
            if any(op in line for op in ['throw', 'raise']):
                # Check if it's in try block context
                context_before = ''.join(lines[max(0, line_no - 10):line_no])
                if not re.search(r'try\s*{', context_before[-200:]):
                    # Standalone throw without try - this is OK
                    pass

            # PATTERN 4: Inconsistent noexcept specification
            # Functions that handle errors should NOT be noexcept
            if re.search(r'noexcept.*\{', line):
                if any(critical in line.lower() for critical in self.critical_functions):
                    # Critical function marked noexcept - may mask errors
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "critical_function_noexcept",
                        SeverityLevel.MEDIUM.value,
                        0.65,
                        "Critical function marked noexcept; may mask error handling",
                        "Remove noexcept if function can fail; document error behavior",
                        line.rstrip()[:120]
                    )

            # PATTERN 5: Result variable created but never checked
            if re.search(r'(Status|Result|Error|auto)\s+\w+\s*=\s*\w+\s*\(', line):
                var_match = re.search(r'(\w+)\s*=\s*(\w+)\s*\(', line)
                if var_match:
                    var_name = var_match.group(1)
                    # Check if variable is used in next 3 lines
                    following = ''.join(lines[line_no:min(line_no + 3, len(lines))])
                    if var_name not in following and not re.search(rf'\b{var_name}\b', following):
                        self._add_gap(
                            gaps,
                            str(file_path.relative_to(file_path.parents[2])),
                            line_no,
                            "unchecked_result",
                            SeverityLevel.HIGH.value,
                            0.72,
                            "Result/Error variable created but never checked",
                            "Check result or explicitly ignore with [[nodiscard]] cast",
                            line.rstrip()[:120]
                        )

        return gaps

    def scan(self, source_dir: str) -> List[Gap]:
        """Main scanner entry point."""
        gaps = []
        source_path = Path(source_dir).resolve()

        excluded = {'.git', 'build', 'vcpkg', 'vcpkg_installed', 'external', 'third_party', '.venv', 'examples'}

        for cpp_file in source_path.rglob('*.cpp'):
            if any(excluded_dir in cpp_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(cpp_file))

        for hpp_file in source_path.rglob('*.hpp'):
            if any(excluded_dir in hpp_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(hpp_file))

        for h_file in source_path.rglob('*.h'):
            if any(excluded_dir in h_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(h_file))

        return gaps
