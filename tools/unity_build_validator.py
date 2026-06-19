#!/usr/bin/env python3
"""
Unity Build Validator for ThemisDB

PURPOSE:
  Detects namespace closure and brace balance issues in C++ source files,
  especially in Unity build contexts where multiple .cpp files are concatenated.

PROBLEM DOCUMENTED:
  - Type: "Unity Build Namespace Closure Imbalance"
  - Root Cause: When .cpp files are concatenated in Unity mode, each file
    must either:
    a) NOT open/close namespaces (rely on previous file's open), OR
    b) Consistently match the namespace structure of the previous file
  - Symptom: MSVC C2143/C2065/C2923 errors at arbitrary lines in later files
  - Detection: Cumulative brace tracking across file sequences

MODULES AFFECTED (as of 2026-06-19):
  - themis_graph (20 files in Unity): explain_plan.cpp (file 12) +
    ontology_manager.cpp (file 13) namespace mismatch

USAGE:
  python tools/unity_build_validator.py [--cmake-preset windows-release-hyperscaler]
    [--module themis_graph] [--verbose]

INTEGRATION POINTS:
  1. CMake configuration phase (cmake/CMakeLists.txt) - optional pre-build check
  2. CI/CD pipeline before full build attempt
  3. Gap scanner orchestrator (tools/gs3_orchestrator.py)
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Optional


@dataclass
class BraceAnalysis:
    """Result of analyzing braces in a single file or sequence."""
    file_path: str
    total_opens: int
    total_closes: int
    balance: int  # opens - closes
    has_unclosed_at_eof: bool
    has_closes_without_open: bool
    first_unmatched_line: Optional[int] = None
    namespace_opens: list[tuple[int, str]] = None  # (line, namespace_name)
    namespace_closes: list[int] = None
    errors: list[str] = None

    def __post_init__(self):
        if self.namespace_opens is None:
            self.namespace_opens = []
        if self.namespace_closes is None:
            self.namespace_closes = []
        if self.errors is None:
            self.errors = []

    def to_dict(self):
        d = asdict(self)
        d['namespace_opens'] = self.namespace_opens
        d['namespace_closes'] = self.namespace_closes
        d['errors'] = self.errors
        return d


class UnityBraceValidator:
    """Analyzes brace balance and namespace closures in C++ source files."""

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.cmake_file = self.repo_root / "cmake" / "ModularBuild.cmake"
        self.verbose = False

    def analyze_file(self, file_path: Path) -> BraceAnalysis:
        """Analyze a single C++ file for brace balance and namespace issues."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            return BraceAnalysis(
                file_path=str(file_path),
                total_opens=0,
                total_closes=0,
                balance=0,
                has_unclosed_at_eof=False,
                has_closes_without_open=True,
                errors=[f"Read error: {e}"]
            )

        analysis = BraceAnalysis(
            file_path=str(file_path),
            total_opens=0,
            total_closes=0,
            balance=0,
            has_unclosed_at_eof=False,
            has_closes_without_open=False,
        )

        open_stack = []
        namespace_re = re.compile(r'namespace\s+(\w+)\s*\{')
        namespace_close_re = re.compile(r'\}\s*//\s*namespace\s+(\w+)')

        for line_no, line in enumerate(lines, 1):
            # Track namespace opens
            for match in namespace_re.finditer(line):
                ns_name = match.group(1)
                analysis.namespace_opens.append((line_no, ns_name))

            # Track namespace closes
            for match in namespace_close_re.finditer(line):
                analysis.namespace_closes.append(line_no)

            # Track braces
            for col, char in enumerate(line):
                if char == '{':
                    analysis.total_opens += 1
                    open_stack.append((line_no, col))
                elif char == '}':
                    analysis.total_closes += 1
                    if open_stack:
                        open_stack.pop()
                    else:
                        if not analysis.first_unmatched_line:
                            analysis.first_unmatched_line = line_no
                        analysis.has_closes_without_open = True

        analysis.balance = analysis.total_opens - analysis.total_closes

        if open_stack:
            analysis.has_unclosed_at_eof = True
            if not analysis.first_unmatched_line:
                first_unclosed = open_stack[0]
                analysis.first_unmatched_line = first_unclosed[0]

        return analysis

    def analyze_unity_sequence(self, module_name: str, file_list: list[str]) -> dict:
        """
        Simulate Unity build: analyze cumulative brace balance across file sequence.
        Returns detailed report of where imbalance occurs.
        """
        results = {
            'module': module_name,
            'files_analyzed': len(file_list),
            'files': [],
            'cumulative_analysis': [],
            'problems_detected': [],
        }

        cumulative_balance = 0
        cumulative_opens = 0
        cumulative_closes = 0

        for idx, file_path_str in enumerate(file_list, 1):
            file_path = Path(file_path_str)
            if not file_path.is_absolute():
                file_path = self.repo_root / file_path

            analysis = self.analyze_file(file_path)
            cumulative_opens += analysis.total_opens
            cumulative_closes += analysis.total_closes
            prev_balance = cumulative_balance
            cumulative_balance += analysis.balance

            file_result = {
                'index': idx,
                'file': str(file_path.relative_to(self.repo_root) if file_path.is_absolute() else file_path),
                'opens': analysis.total_opens,
                'closes': analysis.total_closes,
                'balance': analysis.balance,
                'cumulative_balance': cumulative_balance,
                'namespace_opens': analysis.namespace_opens,
                'namespace_closes': analysis.namespace_closes,
            }

            results['files'].append(file_result)

            # Cumulative milestone
            results['cumulative_analysis'].append({
                'after_file': idx,
                'cumulative_opens': cumulative_opens,
                'cumulative_closes': cumulative_closes,
                'cumulative_balance': cumulative_balance,
            })

            # Detect problems
            if analysis.has_unclosed_at_eof:
                results['problems_detected'].append({
                    'severity': 'WARNING',
                    'file_index': idx,
                    'file': file_result['file'],
                    'issue': 'UNCLOSED_BRACES_AT_EOF',
                    'detail': f"File ends with {analysis.total_opens - analysis.total_closes} unclosed braces",
                    'line': analysis.first_unmatched_line,
                })

            if analysis.has_closes_without_open:
                results['problems_detected'].append({
                    'severity': 'ERROR',
                    'file_index': idx,
                    'file': file_result['file'],
                    'issue': 'CLOSES_WITHOUT_OPEN',
                    'detail': f"File has closing braces without opening (line {analysis.first_unmatched_line})",
                    'line': analysis.first_unmatched_line,
                })

            if cumulative_balance != prev_balance + analysis.balance:
                results['problems_detected'].append({
                    'severity': 'ERROR',
                    'file_index': idx,
                    'file': file_result['file'],
                    'issue': 'CUMULATIVE_CALCULATION_ERROR',
                    'detail': 'Internal calculation mismatch',
                })

            if idx > 1 and cumulative_balance != 0 and analysis.total_opens > 0:
                # In Unity build, if cumulative is imbalanced but file opens braces,
                # namespace structure might be wrong
                if (cumulative_balance < 0 and analysis.namespace_opens):
                    results['problems_detected'].append({
                        'severity': 'WARNING',
                        'file_index': idx,
                        'file': file_result['file'],
                        'issue': 'NAMESPACE_REOPENING_AFTER_IMBALANCE',
                        'detail': f"File opens namespaces at line {analysis.namespace_opens[0][0]} "
                                  f"but cumulative balance already negative ({prev_balance})",
                        'line': analysis.namespace_opens[0][0],
                    })

        return results

    def load_module_sources_from_cmake(self, module_name: str) -> list[str]:
        """Extract source file list for a module from CMakeLists.txt."""
        if not self.cmake_file.exists():
            return []

        try:
            with open(self.cmake_file, 'r') as f:
                content = f.read()
        except Exception:
            return []

        # Search for set(MODULE_NAME_SOURCES ...)
        pattern = rf'set\({module_name.upper()}_SOURCES(.*?)\)'
        match = re.search(pattern, content, re.DOTALL | re.IGNORECASE)

        if not match:
            return []

        sources_block = match.group(1)
        # Extract relative paths (typically ../src/...)
        files = re.findall(r'\.\./(src/[^\s]+\.cpp)', sources_block)
        return files

    def validate_module(self, module_name: str) -> dict:
        """Full validation pipeline for a module."""
        files = self.load_module_sources_from_cmake(module_name)

        if not files:
            return {
                'module': module_name,
                'status': 'SKIPPED',
                'reason': f'Module {module_name} not found in {self.cmake_file} or has no sources',
            }

        result = self.analyze_unity_sequence(module_name, files)

        # Summarize findings
        errors = [p for p in result['problems_detected'] if p['severity'] == 'ERROR']
        warnings = [p for p in result['problems_detected'] if p['severity'] == 'WARNING']

        result['status'] = 'CRITICAL' if errors else ('WARNING' if warnings else 'OK')
        result['error_count'] = len(errors)
        result['warning_count'] = len(warnings)

        return result

    def scan_all_graph_modules(self) -> dict:
        """Scan all themis_* modules for Unity build issues."""
        modules_to_check = [
            'themis_graph',
            'themis_analytics',
            'themis_ml',
            'themis_metrics',
        ]

        results = {}
        for module in modules_to_check:
            if self.verbose:
                print(f"  Scanning {module}...")
            results[module] = self.validate_module(module)

        return results


def main():
    parser = argparse.ArgumentParser(
        description='Unity Build Validator - Detect namespace and brace balance issues'
    )
    parser.add_argument(
        '--repo-root',
        default='.',
        help='Repository root (default: current directory)'
    )
    parser.add_argument(
        '--module',
        default='themis_graph',
        help='Module to validate (default: themis_graph)'
    )
    parser.add_argument(
        '--all-modules',
        action='store_true',
        help='Validate all graph modules instead of single module'
    )
    parser.add_argument(
        '--output',
        help='Save results to JSON file'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    validator = UnityBraceValidator(args.repo_root)
    validator.verbose = args.verbose

    if args.verbose:
        print(f"Repository root: {validator.repo_root}")
        print(f"CMake file: {validator.cmake_file}")
        print()

    if args.all_modules:
        if args.verbose:
            print("Scanning all modules for Unity build issues...")
        results = validator.scan_all_graph_modules()
    else:
        if args.verbose:
            print(f"Scanning module: {args.module}")
        results = {args.module: validator.validate_module(args.module)}

    # Output results
    for module, result in results.items():
        print(f"\n{'='*70}")
        print(f"Module: {module}")
        print(f"Status: {result.get('status', 'UNKNOWN')}")

        if 'files' in result:
            print(f"\nFiles analyzed: {len(result['files'])}")
            print("Cumulative balance by file:")
            for item in result['cumulative_analysis'][-3:]:  # Show last 3
                print(
                    f"  After file {item['after_file']}: "
                    f"opens={item['cumulative_opens']}, "
                    f"closes={item['cumulative_closes']}, "
                    f"balance={item['cumulative_balance']}"
                )

        if result.get('problems_detected'):
            print(f"\nProblems detected: {len(result['problems_detected'])}")
            for prob in result['problems_detected'][:5]:  # Show first 5
                print(
                    f"  [{prob['severity']}] {prob['issue']} "
                    f"(file #{prob.get('file_index', '?')}, line {prob.get('line', '?')})"
                )
                print(f"    --> {prob['detail']}")
            if len(result['problems_detected']) > 5:
                print(f"  ... and {len(result['problems_detected']) - 5} more")

    # Save JSON output if requested
    if args.output:
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w') as f:
            json.dump(results, f, indent=2, default=str)
        print(f"\nResults saved to: {output_path}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
