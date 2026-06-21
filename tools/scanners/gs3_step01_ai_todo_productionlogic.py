#!/usr/bin/env python3
"""
Gap Scanner Step 01 — AI TODO-as-Productionlogic Detection

Detects:
- TODO markers in control flow (if conditions, return statements, etc.)
- Incomplete validation marked with TODO but still executed
- Deferred implementation that's already in production path
- TODO in critical security/error handling paths

Purpose:
AI systems often generate code like:
  if (TODO_VALIDATE(x)) { ... }  // Incomplete validation in production
  return result; // TODO: add error handling
This scanner catches TODOs that are actually business logic, not just comments.
"""

import re
import sys
from pathlib import Path
from typing import List

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class TodoProductionlogicScanner(BaseGapScanner):
    """Phase 1: AI TODO-as-Productionlogic Detection"""

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60

    def __init__(self):
        """Initialize TODO-as-Productionlogic Scanner."""
        super().__init__("AI TODO-as-Productionlogic Scanner", "1.0")

        # Patterns for TODO that's actually code (not just comments)
        self.todo_in_code_patterns = [
            (r'\bTODO\s*\(', 'TODO as function/macro'),
            (r'if\s*\(\s*TODO', 'TODO in if condition'),
            (r'while\s*\(\s*TODO', 'TODO in while loop'),
            (r'return\s+TODO', 'Return value is TODO'),
            (r'TODO\s*[\+\-\*/=]', 'TODO in arithmetic/assignment'),
            (r'TODO_CHECK', 'TODO in validation macro'),
            (r'TODO_UNIMPL', 'Unimplemented feature marker'),
            (r'CHECK_TODO', 'TODO in check'),
        ]

        # Critical paths where TODO is especially dangerous
        self.critical_contexts = [
            'validate', 'verify', 'authenticate', 'authorize',
            'sanitize', 'escape', 'encrypt', 'hash',
            'permission', 'access', 'deny', 'allow',
            'security', 'auth', 'crypto', 'sign',
            'delete', 'drop', 'remove', 'destroy',
        ]

        # TODO comment patterns that are in control flow
        self.critical_todo_comments = [
            r'//.*TODO.*validate',
            r'//.*TODO.*check',
            r'//.*TODO.*verify',
            r'//.*TODO.*secure',
            r'//.*TODO.*auth',
            r'//.*TODO.*permission',
            r'//.*TODO.*implement',
        ]

    def _get_context_window(self, lines: List[str], line_no: int, window: int = 5) -> str:
        """Get surrounding code context."""
        start = max(0, line_no - window - 1)
        end = min(len(lines), line_no + window)
        return ''.join(lines[start:end])

    def _is_in_critical_context(self, context: str) -> bool:
        """Check if TODO is in security/critical function context."""
        context_lower = context.lower()
        return any(token in context_lower for token in self.critical_contexts)

    def _scan_file(self, file_path: Path) -> List[Gap]:
        """Scan single file for TODO-as-productionlogic patterns."""
        gaps = []

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return gaps

        for line_no, line in enumerate(lines, 1):
            stripped = line.strip()

            # Skip pure comment lines
            if stripped.startswith('//') and 'TODO' in stripped:
                # TODO in comments - check if it's in critical path
                for critical_pattern in self.critical_todo_comments:
                    if re.search(critical_pattern, line, re.IGNORECASE):
                        context = self._get_context_window(lines, line_no)
                        is_critical = self._is_in_critical_context(context)

                        if is_critical:
                            self._add_gap(
                                gaps,
                                str(file_path.relative_to(file_path.parents[2])),
                                line_no,
                                "todo_in_critical_path",
                                SeverityLevel.CRITICAL.value,
                                0.85,
                                "TODO in critical security/validation path",
                                "Complete the TODO implementation immediately; do not ship with unfinished validation",
                                line.rstrip()[:120]
                            )

            # Check for TODO as actual code (not just comment)
            if 'TODO' in line and not stripped.startswith('//'):
                for pattern, description in self.todo_in_code_patterns:
                    if re.search(pattern, line, re.IGNORECASE):
                        context = self._get_context_window(lines, line_no)
                        is_critical = self._is_in_critical_context(context)

                        severity = SeverityLevel.CRITICAL.value if is_critical else SeverityLevel.HIGH.value
                        confidence = 0.95 if is_critical else 0.88

                        self._add_gap(
                            gaps,
                            str(file_path.relative_to(file_path.parents[2])),
                            line_no,
                            "todo_as_productionlogic",
                            severity,
                            confidence,
                            f"TODO appears as actual code: {description}",
                            "Replace TODO with complete implementation; never leave TODO in control flow",
                            line.rstrip()[:120]
                        )

            # Pattern: return value followed by TODO comment on same line
            if 'return' in line and 'TODO' in line:
                if '//' in line:
                    # Extract the comment part
                    comment_part = line.split('//', 1)[1]
                    if any(keyword in comment_part.lower() for keyword in ['implement', 'complete', 'fix', 'handle']):
                        self._add_gap(
                            gaps,
                            str(file_path.relative_to(file_path.parents[2])),
                            line_no,
                            "incomplete_return_with_todo",
                            SeverityLevel.HIGH.value,
                            0.82,
                            "Return statement with incomplete TODO comment",
                            "Complete implementation or proper error handling before shipping",
                            line.rstrip()[:120]
                        )

        return gaps

    def scan(self, source_dir: str) -> List[Gap]:
        """Main scanner entry point."""
        gaps = []
        source_path = Path(source_dir).resolve()

        # Exclude common non-source directories
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
