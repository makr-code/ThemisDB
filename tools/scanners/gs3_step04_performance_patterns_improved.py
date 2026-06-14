#!/usr/bin/env python3
"""
Phase 6 - Performance Patterns (IMPROVED)

Improvements:
1. Exclude test/benchmark/example/mock/tool code paths.
2. Only flag hot-path-like code (src/include) to reduce noise.
3. Keep conservative checks for nested-loop-find, expensive loop IO/logging, and reserve misses.
"""

import re
from pathlib import Path
from typing import Dict, List


class PerformanceAntiPatternsScanImproved:
    """Improved performance anti-pattern scanner with reduced false positives."""

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    @staticmethod
    def _is_non_prod_path(rel_file: str) -> bool:
        p = rel_file.lower()
        markers = [
            'tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.',
            'examples/', 'demo_', '_demo.', '_mock.', 'tools/', 'scripts/', 'fuzz/',
        ]
        return any(m in p for m in markers)

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in ['.cpp', '.cc', '.h', '.hpp', '.cxx', '.hh', '.hxx']:
                continue

            try:
                rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/')
            except Exception:
                rel_file = str(file_path).replace('\\', '/')

            if self._is_non_prod_path(rel_file):
                continue
            if not (rel_file.startswith('src/') or rel_file.startswith('include/')):
                continue

            try:
                lines = file_path.read_text(encoding='utf-8', errors='ignore').split('\n')
            except Exception:
                continue

            self._check_nested_loops_with_find(rel_file, lines)
            self._check_missing_vector_reserve(rel_file, lines)
            self._check_expensive_ops_in_loop(rel_file, lines)

        return self.gaps

    def _emit(self, rel_file: str, line: int, severity: str, pattern: str, description: str, context: str):
        self.gaps.append({
            'file': rel_file,
            'line': line,
            'category': 'performance',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context.strip()[:180],
        })

    def _check_nested_loops_with_find(self, rel_file: str, lines: List[str]):
        nested_for = 0
        for idx, line in enumerate(lines, 1):
            stripped = line.strip()
            if stripped.startswith('//'):
                continue
            if re.search(r'\bfor\s*\(', line):
                nested_for += 1
            if nested_for >= 2 and re.search(r'\.(find|count)\s*\(|std::find\s*\(', line):
                self._emit(
                    rel_file,
                    idx,
                    'HIGH',
                    'nested_loop_find',
                    'Possible O(n^2) lookup in nested loops',
                    line,
                )
            if '}' in line:
                nested_for = max(0, nested_for - line.count('}'))

    def _check_missing_vector_reserve(self, rel_file: str, lines: List[str]):
        for idx, line in enumerate(lines, 1):
            if not re.search(r'\bfor\s*\(', line):
                continue
            if not any(tok in line for tok in ['.size(', '.length(', 'count', 'total', 'num_', '< n', '<N']):
                continue

            end = min(len(lines), idx + 20)
            window = lines[idx:end]
            for w_idx, w_line in enumerate(window, start=idx + 1):
                m = re.search(r'\b([A-Za-z_]\w*)\s*\.\s*(push_back|emplace_back)\s*\(', w_line)
                if not m:
                    continue
                vec = m.group(1)

                decl_ctx = '\n'.join(lines[max(0, idx - 60):w_idx])
                if not re.search(rf'std::vector\s*<[^>]+>\s+{re.escape(vec)}\b', decl_ctx):
                    continue

                reserve_ctx = '\n'.join(lines[max(0, idx - 40):min(len(lines), idx + 20)])
                if re.search(rf'\b{re.escape(vec)}\s*\.\s*reserve\s*\(', reserve_ctx):
                    continue

                self._emit(
                    rel_file,
                    w_idx,
                    'MEDIUM',
                    'missing_vector_reserve',
                    'vector push_back/emplace_back in loop without reserve()',
                    w_line,
                )
                break

    def _check_expensive_ops_in_loop(self, rel_file: str, lines: List[str]):
        for idx, line in enumerate(lines, 1):
            if not re.search(r'\b(for|while)\s*\(', line):
                continue
            loop_ctx = '\n'.join(lines[idx:min(len(lines), idx + 25)])

            if re.search(r'std::regex\b', loop_ctx):
                self._emit(rel_file, idx, 'MEDIUM', 'regex_in_loop', 'std::regex used in loop body', line)
            if re.search(r'<<\s*std::endl', loop_ctx):
                self._emit(rel_file, idx, 'LOW', 'endl_in_loop', 'std::endl in loop causes frequent flush', line)
            if re.search(r'\b(log|LOG_|printf|std::cout)\b', loop_ctx) and re.search(r'\b(for|while)\s*\(', loop_ctx):
                self._emit(rel_file, idx, 'LOW', 'expensive_logging_loop', 'Potential expensive logging in loop', line)
