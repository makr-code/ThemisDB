#!/usr/bin/env python3
"""
Phase 6 - Design/Error Rules (IMPROVED)

Improvements:
1. Ignore tests/bench/examples for architectural blocking findings.
2. Swallowed catch(...) only in production-critical paths.
3. Allow intentional fallback/parse-guard patterns.
"""

import re
from pathlib import Path
from typing import Dict, List


class ThemisDesignErrorRulesScanImproved:
    """Improved design and error-handling rule scanner."""

    CATCH_ALL_BLOCK_RE = re.compile(r'catch\s*\(\s*\.\.\.\s*\)\s*\{(.*?)\}', re.DOTALL)
    INTENTIONAL_COMMENT_TOKENS = ('skip malformed', 'best effort', 'fallback', 'ignore', 'non-fatal', 'optional')

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    @staticmethod
    def _is_non_prod_path(path_norm: str) -> bool:
        markers = ['tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.', 'examples/', 'demo_', '/tools/']
        return any(m in path_norm for m in markers)

    def _read_text(self, file_path: Path) -> str:
        try:
            return file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception:
            return ''

    def _append(self, file_path: Path, line: int, severity: str, pattern: str, description: str, context: str):
        self.gaps.append({
            'file': str(file_path.relative_to(self.repo_root)).replace('\\', '/'),
            'line': line,
            'category': 'design_error_rules',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context.strip()[:180],
        })

    def _has_intentional_comment_markers(self, block: str) -> bool:
        lower = block.lower()
        return any(tok in lower for tok in self.INTENTIONAL_COMMENT_TOKENS)

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in ['.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx']:
                continue

            path_norm = str(file_path).replace('\\', '/').lower()
            if self._is_non_prod_path(path_norm):
                continue

            text = self._read_text(file_path)
            if not text:
                continue

            self._check_chimera_retry_duplication(file_path, path_norm, text)
            self._check_swallowed_catch_all(file_path, path_norm, text)

        return self.gaps

    def _check_chimera_retry_duplication(self, file_path: Path, path_norm: str, text: str) -> None:
        if '/chimera/' not in path_norm:
            return

        allowlist_tokens = ['chimera_client', 'database_adapter', 'adapter_factory', 'adapter_export']
        if any(tok in path_norm for tok in allowlist_tokens):
            return

        for idx, line in enumerate(text.splitlines(), 1):
            lower = line.lower()
            if any(tok in lower for tok in ['retry', 'backoff', 'jitter', 'sleep_for', 'attempt', 'max_retries']):
                self._append(
                    file_path,
                    idx,
                    'MEDIUM',
                    'chimera_retry_duplication',
                    'Chimera retry logic should be centralized (avoid per-adapter retries)',
                    line,
                )

    def _check_swallowed_catch_all(self, file_path: Path, path_norm: str, text: str) -> None:
        critical_tokens = ['/server/', '/api/', '/network/', '/security/', '/auth/', '/query/', '/chimera/']
        if not any(tok in path_norm for tok in critical_tokens):
            return

        for m in self.CATCH_ALL_BLOCK_RE.finditer(text):
            block = m.group(1) or ''
            block_lower = block.lower()
            start_line = text[:m.start()].count('\n') + 1

            if self._has_intentional_comment_markers(block):
                continue

            meaningful_lines = [
                ln.strip() for ln in block.splitlines()
                if ln.strip() and not ln.strip().startswith('//') and not ln.strip().startswith('/*')
            ]
            has_handling = any(tok in block_lower for tok in [
                'throw', 'return', 'log', 'spdlog', 'error', 'status', 'abort', 'terminate', 'fallback', 'recover'
            ])

            is_short = len(meaningful_lines) <= 4
            has_call_stmt = any('(' in ln and ')' in ln for ln in meaningful_lines)
            has_assignment = any('=' in ln for ln in meaningful_lines)

            if (not has_handling) and is_short and (not has_call_stmt) and (not has_assignment):
                self._append(
                    file_path,
                    start_line,
                    'HIGH',
                    'catch_all_swallow',
                    'catch(...) swallows errors without explicit handling',
                    'catch(...) { ... }',
                )
