#!/usr/bin/env python3
"""
Gap Scanner Step 01 — AI Simulation/Stub/Mockup Leak Detection

Detects:
- Test/demo code paths left in production source files
- Simulation/stub markers that should be removed before production
- Build flags that enable non-production code paths
- Return values that mock/placeholder actual behavior
- Incomplete implementations marked with stub indicators

Purpose:
AI systems often leave "works for now" placeholder code or simulation paths that
are designed for development/testing but accidentally shipped to production.
This scanner catches those patterns before they cause runtime surprises.
"""

import re
import sys
from pathlib import Path
from typing import List

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class SimulationStubLeakScanner(BaseGapScanner):
    """Phase 1: AI Simulation/Stub/Mockup Leak Detection"""

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60

    def __init__(self):
        """Initialize Simulation/Stub Leak Scanner."""
        super().__init__("AI Simulation/Stub Leak Scanner", "1.0")

        # Patterns for simulation/stub/mockup markers
        self.stub_patterns = [
            (r'\bstub::', 'Explicit stub namespace'),
            (r'\bTODO_STUB\b', 'TODO stub macro'),
            (r'\bMOCK_.*\(', 'Mock macro usage'),
            (r'\bFAKE_', 'Fake implementation prefix'),
            (r'\bSIMULATION_', 'Simulation flag'),
            (r'\btest_only\(', 'Test-only function call'),
            (r'\bDEVEL_ONLY\b', 'Development-only marker'),
        ]

        self.return_patterns = [
            (r'return\s+true\s*;\s*//\s*(mock|stub|fake|temp)', 'Mock return true'),
            (r'return\s+false\s*;\s*//\s*(mock|stub|fake|temp)', 'Mock return false'),
            (r'return\s+nullptr\s*;\s*//\s*(mock|stub|fake|temp)', 'Mock return nullptr'),
            (r'return\s+\{\}\s*;\s*//\s*(mock|stub|fake|temp)', 'Mock return empty'),
        ]

        self.ifdef_patterns = [
            r'#ifdef\s+SIMULATION',
            r'#ifdef\s+DEBUG_MODE',
            r'#ifdef\s+TEST_BUILD',
            r'#ifdef\s+MOCK_ENABLED',
            r'#ifdef\s+STUB_',
        ]

        self.comment_patterns = [
            r'//\s*(TEMPORARY|TEMP|HACK|STUB|MOCK|SIMULATION|FAKE)',
            r'//\s*TODO:\s*(remove|delete|fix|implement|complete)',
            r'/\*\s*(TEMPORARY|STUB|MOCK|SIMULATION)',
        ]

    def _scan_file(self, file_path: Path) -> List[Gap]:
        """Scan single file for simulation/stub leaks."""
        gaps = []

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return gaps

        for line_no, line in enumerate(lines, 1):
            # Skip comment-only lines
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue

            # Check for stub/mock patterns
            for pattern, description in self.stub_patterns:
                if re.search(pattern, line, re.IGNORECASE):
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "simulation_stub_marker",
                        SeverityLevel.HIGH.value,
                        0.85,
                        f"Simulation/stub marker found: {description}",
                        "Remove stub/mock code or move to test/example directory; replace with real implementation",
                        line.rstrip()[:120]
                    )

            # Check for mock return patterns
            for pattern, description in self.return_patterns:
                if re.search(pattern, line, re.IGNORECASE):
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "mock_return_value",
                        SeverityLevel.HIGH.value,
                        0.90,
                        f"Mock/placeholder return value: {description}",
                        "Replace with actual implementation or proper error handling",
                        line.rstrip()[:120]
                    )

            # Check for simulation build flags
            for ifdef_pattern in self.ifdef_patterns:
                if re.search(ifdef_pattern, line):
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "simulation_build_flag",
                        SeverityLevel.MEDIUM.value,
                        0.80,
                        "Simulation/mock build flag found in production code",
                        "Ensure this flag is not enabled in production builds; verify CMakeLists.txt excludes this",
                        line.rstrip()[:120]
                    )

            # Check for temporary/stub comments (lower priority)
            for comment_pattern in self.comment_patterns:
                if re.search(comment_pattern, line, re.IGNORECASE):
                    match = re.search(comment_pattern, line, re.IGNORECASE)
                    if match:
                        # Only flag if it looks like actual stub/temporary code, not generic TODOs
                        if any(token in match.group().lower() for token in ['stub', 'mock', 'simulation', 'fake']):
                            self._add_gap(
                                gaps,
                                str(file_path.relative_to(file_path.parents[2])),
                                line_no,
                                "stub_temporary_comment",
                                SeverityLevel.MEDIUM.value,
                                0.70,
                                "Temporary/stub comment in code",
                                "Replace stub implementation with real code or remove if no longer needed",
                                line.rstrip()[:120]
                            )

        return gaps

    def scan(self, source_dir: str) -> List[Gap]:
        """Main scanner entry point."""
        gaps = []
        source_path = Path(source_dir).resolve()

        # Exclude common non-source directories
        excluded = {'.git', 'build', 'vcpkg', 'vcpkg_installed', 'external', 'third_party', '.venv'}

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
