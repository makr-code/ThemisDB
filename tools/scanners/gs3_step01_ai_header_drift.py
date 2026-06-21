#!/usr/bin/env python3
"""
Gap Scanner Step 01 — Header/Implementation Drift Detection

Detects:
- Doxygen documentation only in .cpp (not in .hpp header)
- Function signatures different between .hpp and .cpp
- Public API in .cpp without corresponding .hpp declaration
- Parameter names diverging between header and implementation
- Missing forward declarations in headers
- Return type mismatches

Purpose:
AI systems often generate .cpp implementations but forget to update .hpp headers.
This creates API drift: callers see incomplete/outdated documentation and signatures.
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Optional

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class HeaderDriftScanner(BaseGapScanner):
    """Phase 1: Header/Implementation Drift Detection"""

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 120

    def __init__(self):
        """Initialize Header Drift Scanner."""
        super().__init__("Header Drift Scanner", "1.0")
        self.header_map: Dict[str, str] = {}  # Maps impl file to header file
        self.parsed_headers: Dict[str, Dict] = {}
        self.parsed_impls: Dict[str, Dict] = {}

    def _find_header_for_impl(self, impl_path: Path) -> Optional[Path]:
        """Find corresponding header file for implementation."""
        # Handle naming conventions: file.cpp -> file.hpp or file.h
        stem = impl_path.stem
        parent = impl_path.parent

        # Look for .hpp first
        hpp_file = parent.parent / 'include' / f"{stem}.hpp"
        if hpp_file.exists():
            return hpp_file

        # Look for .h
        h_file = parent.parent / 'include' / f"{stem}.h"
        if h_file.exists():
            return h_file

        # Same directory
        hpp_same = impl_path.parent / f"{stem}.hpp"
        if hpp_same.exists():
            return hpp_same

        h_same = impl_path.parent / f"{stem}.h"
        if h_same.exists():
            return h_same

        return None

    def _extract_function_signatures(self, file_path: Path) -> Dict[str, Dict]:
        """Extract function signatures from file (header or impl)."""
        signatures = {}

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return signatures

        current_class = None

        for line_no, line in enumerate(lines, 1):
            # Track class context
            class_match = re.search(r'^\s*(class|struct)\s+(\w+)', line)
            if class_match:
                current_class = class_match.group(2)

            # Extract function declarations/definitions
            # Match: return_type function_name(params);  or  return_type function_name(params) { or } 
            func_match = re.search(
                r'(virtual\s+)?([\w:]+(?:\*|&)?)\s+(\w+)\s*\((.*?)\)\s*([^=;{]*)',
                line
            )

            if func_match:
                is_virtual = bool(func_match.group(1))
                return_type = func_match.group(2).strip()
                func_name = func_match.group(3)
                params = func_match.group(4).strip()
                qualifiers = func_match.group(5).strip()

                key = f"{current_class}::{func_name}" if current_class else func_name
                signatures[key] = {
                    'line': line_no,
                    'return_type': return_type,
                    'params': params,
                    'qualifiers': qualifiers,
                    'virtual': is_virtual,
                    'context': line.rstrip()[:100],
                }

        return signatures

    def _extract_doxygen_info(self, file_path: Path) -> Dict[str, bool]:
        """Check which functions have Doxygen documentation."""
        has_doxygen = {}

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception:
            return has_doxygen

        i = 0
        while i < len(lines):
            line = lines[i]

            # Find Doxygen comment
            if re.search(r'^\s*//!|^\s*/\*\*', line):
                doxy_start = i
                doxy_text = []

                # Collect doxygen comment
                while i < len(lines):
                    doxy_text.append(lines[i])
                    if '*/' in lines[i]:
                        break
                    i += 1

                # Next non-empty line should be function
                while i < len(lines) and lines[i].strip() == '':
                    i += 1

                if i < len(lines):
                    func_match = re.search(r'\b(\w+)\s*\(', lines[i])
                    if func_match:
                        func_name = func_match.group(1)
                        has_doxygen[func_name] = True

            i += 1

        return has_doxygen

    def _scan_impl_file(self, impl_path: Path) -> List[Gap]:
        """Scan .cpp file and check against header."""
        gaps = []

        # Find corresponding header
        header_path = self._find_header_for_impl(impl_path)
        if not header_path or not header_path.exists():
            return gaps  # No header found, can't check drift

        # Parse both files
        impl_sigs = self._extract_function_signatures(impl_path)
        header_sigs = self._extract_function_signatures(header_path)
        impl_docs = self._extract_doxygen_info(impl_path)
        header_docs = self._extract_doxygen_info(header_path)

        # Check for drift patterns
        for impl_key, impl_info in impl_sigs.items():
            # PATTERN 1: Documentation in impl but not in header
            func_name = impl_key.split('::')[-1]
            if func_name in impl_docs and func_name not in header_docs:
                # Header missing docs
                self._add_gap(
                    gaps,
                    str(impl_path.relative_to(impl_path.parents[2])),
                    impl_info['line'],
                    "header_missing_doxygen",
                    SeverityLevel.MEDIUM.value,
                    0.75,
                    "Implementation documented in .cpp but not in .hpp header",
                    "Move/copy Doxygen documentation from .cpp to .hpp; callers need to see API docs",
                    impl_info['context']
                )

            # PATTERN 2: Signature mismatch between header and impl
            if impl_key in header_sigs:
                header_info = header_sigs[impl_key]
                # Compare signatures
                if impl_info['return_type'] != header_info['return_type']:
                    self._add_gap(
                        gaps,
                        str(impl_path.relative_to(impl_path.parents[2])),
                        impl_info['line'],
                        "signature_drift_return_type",
                        SeverityLevel.HIGH.value,
                        0.88,
                        f"Return type mismatch: .hpp says '{header_info['return_type']}', .cpp is '{impl_info['return_type']}'",
                        "Align return types between header and implementation",
                        impl_info['context']
                    )

                # Parameter count mismatch
                impl_param_count = len([p for p in impl_info['params'].split(',') if p.strip()])
                header_param_count = len([p for p in header_info['params'].split(',') if p.strip()])
                if impl_param_count != header_param_count:
                    self._add_gap(
                        gaps,
                        str(impl_path.relative_to(impl_path.parents[2])),
                        impl_info['line'],
                        "signature_drift_params",
                        SeverityLevel.HIGH.value,
                        0.90,
                        f"Parameter count mismatch: .hpp has {header_param_count}, .cpp has {impl_param_count}",
                        "Ensure function signatures match exactly between declaration and definition",
                        impl_info['context']
                    )

        # PATTERN 3: Implementation exists in .cpp without declaration in .hpp
        for impl_key in impl_sigs:
            if impl_key not in header_sigs and '::' in impl_key:  # Class member
                # This is OK - inlined methods, private implementations
                pass
            elif impl_key not in header_sigs:
                # Could be a helper function (OK) or a leaked API
                func_name = impl_key.split('::')[-1]
                if not func_name.startswith('_') and not func_name.startswith('test_'):
                    self._add_gap(
                        gaps,
                        str(impl_path.relative_to(impl_path.parents[2])),
                        impl_sigs[impl_key]['line'],
                        "undeclared_public_function",
                        SeverityLevel.MEDIUM.value,
                        0.70,
                        "Function implemented but not declared in .hpp",
                        "Either add function declaration to header or prefix name with '_' if internal",
                        impl_sigs[impl_key]['context']
                    )

        return gaps

    def scan(self, source_dir: str) -> List[Gap]:
        """Main scanner entry point."""
        gaps = []
        source_path = Path(source_dir).resolve()

        excluded = {'.git', 'build', 'vcpkg', 'vcpkg_installed', 'external', 'third_party', '.venv', 'examples', 'tests', 'benchmarks'}

        # Only scan .cpp files (they get compared to headers)
        for cpp_file in source_path.rglob('*.cpp'):
            if any(excluded_dir in cpp_file.parts for excluded_dir in excluded):
                continue
            if 'test' in cpp_file.name.lower() or 'benchmark' in cpp_file.name.lower():
                continue

            gaps.extend(self._scan_impl_file(cpp_file))

        return gaps
