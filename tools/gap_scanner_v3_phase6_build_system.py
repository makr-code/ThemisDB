#!/usr/bin/env python3
"""
Phase 6-4: Build System Hardening Scanner

Detects CMake correctness issues, unsafe linker flag patterns, and dependency
problems that reduce build reproducibility, debuggability, or security posture:

  B-1  Missing explicit target dependencies (implicit CMake resolution)
  B-2  Inconsistent compiler flags across targets (add_compile_options vs target)
  B-3  Missing debug symbols in release presets
  B-4  Undefined symbol visibility (no visibility preset set)
  B-5  Unused / over-linked libraries
  B-6  Missing sanitizer flags in test targets
  B-7  LTO enabled on only part of the dependency graph
  B-8  Hardcoded absolute paths in CMakeLists.txt

Expected gaps: 200–400 across CMakeLists.txt files.
Complexity: MEDIUM (CMake text parsing; no full CMake model evaluation).
"""

import re
from pathlib import Path
from typing import List, Dict


class BuildSystemScanner:
    """Scan CMakeLists.txt and .cmake files for build system hardening issues."""

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    # ------------------------------------------------------------------
    # Public entry point
    # ------------------------------------------------------------------

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan *file_list* (CMake files only) and return gap dictionaries."""
        self.gaps = []

        cmake_files = [
            f for f in file_list
            if f.name == 'CMakeLists.txt' or f.suffix in {'.cmake'}
        ]

        for file_path in cmake_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as fh:
                    content = fh.read()
                    lines = content.split('\n')
            except OSError:
                continue

            self._check_missing_explicit_deps(file_path, lines, content)
            self._check_global_compile_options(file_path, lines)
            self._check_missing_debug_symbols(file_path, lines, content)
            self._check_visibility_preset(file_path, lines, content)
            self._check_unused_link_libraries(file_path, lines, content)
            self._check_missing_sanitizer_flags(file_path, lines, content)
            self._check_lto_mismatch(file_path, lines, content)
            self._check_absolute_paths(file_path, lines)

        return self.gaps

    # ------------------------------------------------------------------
    # Helper
    # ------------------------------------------------------------------

    def _gap(self, file_path: Path, line: int, pattern: str,
             severity: str, description: str, context: str) -> None:
        try:
            rel = str(file_path.relative_to(self.repo_root))
        except ValueError:
            rel = str(file_path)
        self.gaps.append({
            'file': rel,
            'line': line,
            'category': 'build_system',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context[:200].strip(),
        })

    # ------------------------------------------------------------------
    # B-1  Missing explicit dependencies
    # ------------------------------------------------------------------

    def _check_missing_explicit_deps(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-1: target_link_libraries referencing names that are not declared targets."""
        # Collect declared targets in this file.
        # CMake target names may contain letters, digits, underscores, hyphens, and dots.
        declared: set = set()
        for m in re.finditer(
            r'(?:add_executable|add_library)\s*\(\s*([\w.\-]+)', content
        ):
            declared.add(m.group(1))

        for idx, line in enumerate(lines, 1):
            m = re.search(
                r'target_link_libraries\s*\(\s*([\w.\-]+)\s+(PRIVATE|PUBLIC|INTERFACE)\s+'
                r'([\w.: \-]+)\)',
                line,
            )
            if not m:
                continue
            libs_raw = m.group(3).split()
            for lib in libs_raw:
                lib = lib.strip()
                # Skip keyword tokens, CMake variables, and known system libs
                if lib in {'PRIVATE', 'PUBLIC', 'INTERFACE'}:
                    continue
                if lib.startswith('$') or lib.startswith('-') or '::' in lib:
                    continue
                if lib in declared:
                    continue
                # Flag potential implicit dep (may be defined in parent scope)
                self._gap(
                    file_path, idx, 'missing_explicit_dep', 'MEDIUM',
                    f'target_link_libraries references "{lib}" which is not declared '
                    f'in this file; verify it is an explicit CMake target or '
                    f'imported target, not an implicit link path',
                    line,
                )

    # ------------------------------------------------------------------
    # B-2  Global add_compile_options
    # ------------------------------------------------------------------

    def _check_global_compile_options(
        self, file_path: Path, lines: List[str]
    ) -> None:
        """B-2: add_compile_options() applies globally and may conflict with targets."""
        # Only flag when target_compile_options() is also present: mixing global and
        # per-target flags in the same file creates hard-to-debug conflicts.
        content = '\n'.join(lines)
        if not re.search(r'\btarget_compile_options\s*\(', content):
            return
        for idx, line in enumerate(lines, 1):
            if re.search(r'\badd_compile_options\s*\(', line):
                self._gap(
                    file_path, idx, 'global_compile_options', 'MEDIUM',
                    'add_compile_options() applies to ALL subsequent targets; '
                    'prefer target_compile_options(target PRIVATE ...) for per-target flags '
                    'to avoid flag conflicts across targets',
                    line,
                )

    # ------------------------------------------------------------------
    # B-3  Missing debug symbols in release presets
    # ------------------------------------------------------------------

    def _check_missing_debug_symbols(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-3: Release builds should carry debug info for production diagnostics."""
        # Only meaningful for root CMakeLists.txt (CMakePresets.json is not passed
        # to scan_files() and therefore never reaches this check).
        if file_path.name != 'CMakeLists.txt':
            return

        # Look for Release build type without -g / /Zi
        for idx, line in enumerate(lines, 1):
            if re.search(r'CMAKE_BUILD_TYPE.*Release', line, re.IGNORECASE):
                # Check if debug symbols explicitly configured nearby
                surrounding = '\n'.join(lines[max(0, idx - 5):min(len(lines), idx + 10)])
                if not re.search(
                    r'(-g[0-9]?|/Zi|RelWithDebInfo|CMAKE_CXX_FLAGS.*-g)', surrounding
                ):
                    self._gap(
                        file_path, idx, 'missing_debug_symbols', 'MEDIUM',
                        'Release build type without explicit debug-symbol configuration; '
                        'add -g / /Zi or use RelWithDebInfo to enable post-mortem debugging',
                        line,
                    )

    # ------------------------------------------------------------------
    # B-4  Undefined symbol visibility
    # ------------------------------------------------------------------

    def _check_visibility_preset(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-4: Without a visibility preset, all symbols default to visible (ABI instability)."""
        if file_path.name != 'CMakeLists.txt':
            return
        # Only flag top-level CMakeLists that contain project()
        if not re.search(r'\bproject\s*\(', content):
            return
        if not re.search(r'CMAKE_CXX_VISIBILITY_PRESET', content):
            self._gap(
                file_path, 1, 'missing_visibility_preset', 'HIGH',
                'No CMAKE_CXX_VISIBILITY_PRESET set in top-level CMakeLists.txt; '
                'set to "hidden" and mark exported symbols with visibility annotations '
                'to stabilise ABI and reduce link-time symbol pollution',
                'project() declaration without visibility preset',
            )

    # ------------------------------------------------------------------
    # B-5  Over-linked libraries (heuristic: many libs, PRIVATE scope)
    # ------------------------------------------------------------------

    def _check_unused_link_libraries(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-5: Excessive link lists may contain unnecessary libraries."""
        for idx, line in enumerate(lines, 1):
            m = re.search(
                r'target_link_libraries\s*\(\s*\w+\s+PRIVATE\s+([\w: \t\n]+)\)',
                line,
            )
            if not m:
                continue
            libs = [
                l.strip() for l in m.group(1).split()
                if l.strip() and not l.strip().startswith('$')
                and '::' not in l and not l.startswith('-')
            ]
            if len(libs) >= 8:
                self._gap(
                    file_path, idx, 'excessive_link_libraries', 'LOW',
                    f'target_link_libraries lists {len(libs)} libraries for a single target; '
                    f'audit for unused deps to reduce binary size and link time',
                    line,
                )

    # ------------------------------------------------------------------
    # B-6  Missing sanitizer flags in test targets
    # ------------------------------------------------------------------

    def _check_missing_sanitizer_flags(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-6: Test targets should apply Address/UB sanitizers to catch memory bugs."""
        has_test_target = bool(
            re.search(r'add_(?:executable|test)\s*\(\s*\w*[Tt]est', content)
        )
        if not has_test_target:
            return

        if not re.search(r'-fsanitize=', content):
            self._gap(
                file_path, 1, 'missing_sanitizer_flags', 'HIGH',
                'Test targets present but no -fsanitize= flags configured; '
                'add -fsanitize=address,undefined to test target compiler options '
                'to detect memory errors and UB at test time',
                'Test target without sanitizer flags',
            )

    # ------------------------------------------------------------------
    # B-7  LTO on library but not on executable
    # ------------------------------------------------------------------

    def _check_lto_mismatch(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """B-7: Partial LTO enablement causes link-time errors or misoptimisation."""
        has_lto = re.search(r'INTERPROCEDURAL_OPTIMIZATION\s+ON', content)
        if not has_lto:
            return

        lto_targets = set(re.findall(
            r'set_target_properties\s*\(\s*(\w+)\s+PROPERTIES\s+'
            r'INTERPROCEDURAL_OPTIMIZATION\s+ON', content
        ))
        all_targets = set(re.findall(
            r'add_(?:executable|library)\s*\(\s*(\w+)', content
        ))

        # If LTO is not set on cmake global but only some targets, flag the difference
        if lto_targets and all_targets and not lto_targets.issuperset(all_targets):
            missing = all_targets - lto_targets
            for idx, line in enumerate(lines, 1):
                if re.search(r'INTERPROCEDURAL_OPTIMIZATION\s+ON', line):
                    self._gap(
                        file_path, idx, 'lto_mismatch', 'MEDIUM',
                        f'LTO enabled only on subset of targets {lto_targets}; '
                        f'targets without LTO: {missing}. Enable consistently or '
                        f'disable globally to avoid linker errors',
                        line,
                    )
                    break

    # ------------------------------------------------------------------
    # B-8  Hardcoded absolute paths
    # ------------------------------------------------------------------

    def _check_absolute_paths(self, file_path: Path, lines: List[str]) -> None:
        """B-8: Hardcoded absolute paths break out-of-tree and CI builds."""
        for idx, line in enumerate(lines, 1):
            # Skip comment lines
            if line.strip().startswith('#'):
                continue
            # Detect Unix absolute paths in string literals
            if re.search(r'["\s](/(?:home|usr|opt|mnt|var|etc|root|build)/\S+)', line):
                self._gap(
                    file_path, idx, 'hardcoded_absolute_path', 'HIGH',
                    'Hardcoded absolute path in CMake script breaks portability; '
                    'use ${CMAKE_SOURCE_DIR}, ${CMAKE_BINARY_DIR} or vcpkg variables',
                    line,
                )
            # Detect Windows absolute paths C:\...
            elif re.search(r'["\s]([A-Z]:\\[^"\s]+)', line):
                self._gap(
                    file_path, idx, 'hardcoded_absolute_path', 'HIGH',
                    'Hardcoded Windows absolute path in CMake script breaks portability; '
                    'use ${CMAKE_SOURCE_DIR} or environment variables instead',
                    line,
                )
