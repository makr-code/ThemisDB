#!/usr/bin/env python3
"""
Phase 5 - Legacy/Duplication Scanner (IMPROVED)

Improvements:
1. Legacy marker alerts only for production code paths (exclude tests/bench/examples/tools).
2. If '@deprecated' documentation exists near marker, downgrade/skip finding.
3. Duplicate signature scan ignores known backend/test variants more aggressively.
"""

import re
from pathlib import Path
from typing import Dict, List


class LegacyDuplicationScanImproved:
    """Improved scan for legacy path markers and duplicate implementation signatures."""

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

        self._legacy_re = re.compile(
            r'\b(legacy|compat(?:ibility)?|deprecated|old[_\s-]?path|backward[_\s-]?compat(?:ibility)?)\b',
            re.IGNORECASE,
        )
        self._deprecated_tag_re = re.compile(r'@deprecated|\\deprecated', re.IGNORECASE)

        self._qualified_method_re = re.compile(
            r'^\s*(?:template\s*<[^>]*>\s*)?'
            r'(?:(?:inline|static|constexpr|virtual)\s+)*'
            r'(?:(?:[\w:<>,~*&]+)\s+)+'
            r'([A-Za-z_][\w:]*)::(~?[A-Za-z_]\w*)\s*\(([^;]*)\)\s*(?:const)?\s*(?:\{|$)'
        )

        self._variant_suffix_re = re.compile(
            r'(_openssl|_stub|_mock|_cpu|_gpu|_cuda|_rocm|_vulkan|_metal|_directx|_sim|_test|_bench)$',
            re.IGNORECASE,
        )

    @staticmethod
    def _normalize_params(param_text: str) -> str:
        return re.sub(r'\s+', ' ', param_text.strip())

    @staticmethod
    def _looks_like_definition(lines: List[str], index: int) -> bool:
        line = lines[index]
        if ';' in line and '{' not in line:
            return False
        if '{' in line:
            return True
        for j in range(index + 1, min(index + 8, len(lines))):
            nxt = lines[j]
            if ';' in nxt and '{' not in nxt:
                return False
            if '{' in nxt:
                return True
        return False

    @staticmethod
    def _is_non_prod_path(rel_file: str) -> bool:
        p = rel_file.replace('\\', '/').lower()
        markers = [
            'tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.',
            'examples/', 'demo_', '_demo.', 'tools/', 'scripts/', 'fuzz/',
        ]
        return any(m in p for m in markers)

    def _has_deprecated_doc_nearby(self, lines: List[str], idx0: int) -> bool:
        start = max(0, idx0 - 5)
        end = min(len(lines), idx0 + 2)
        context = '\n'.join(lines[start:end])
        return self._deprecated_tag_re.search(context) is not None

    def _is_likely_mirror_or_variant_pair(self, files: List[str]) -> bool:
        if len(files) != 2:
            return False

        p0 = Path(files[0].replace('\\', '/'))
        p1 = Path(files[1].replace('\\', '/'))
        b0, b1 = p0.stem, p1.stem

        if p0.name == p1.name:
            return True
        if b0 == b1:
            return True
        if b0.startswith(b1 + '_') or b1.startswith(b0 + '_'):
            return True
        if self._variant_suffix_re.search(b0) or self._variant_suffix_re.search(b1):
            return True

        pair = {p0.parts[1] if len(p0.parts) > 1 else '', p1.parts[1] if len(p1.parts) > 1 else ''}
        if pair in ({'base', 'themis'}, {'src', 'include'}):
            return True

        return False

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []
        signature_locations: Dict[str, List[Dict]] = {}

        for file_path in file_list:
            if file_path.suffix not in ['.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx']:
                continue

            try:
                lines = file_path.read_text(encoding='utf-8', errors='ignore').splitlines()
            except Exception:
                continue

            rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/')
            is_non_prod = self._is_non_prod_path(rel_file)

            for idx, line in enumerate(lines, 1):
                stripped = line.strip()
                is_comment_or_pp = (
                    stripped.startswith('//') or stripped.startswith('/*') or
                    stripped.startswith('*') or stripped.startswith('#')
                )

                if (not is_non_prod) and is_comment_or_pp and self._legacy_re.search(stripped):
                    if self._has_deprecated_doc_nearby(lines, idx - 1):
                        continue
                    self.gaps.append({
                        'file': rel_file,
                        'line': idx,
                        'category': 'legacy_duplication',
                        'severity': 'MEDIUM',
                        'pattern': 'legacy_or_compat_path',
                        'description': 'Legacy/compatibility marker without explicit @deprecated contract.',
                        'context': stripped,
                    })

                if is_non_prod or file_path.suffix not in ['.cpp', '.cc', '.cxx']:
                    continue

                m = self._qualified_method_re.search(line)
                if not m or not self._looks_like_definition(lines, idx - 1):
                    continue

                owner = m.group(1)
                method = m.group(2)
                params = self._normalize_params(m.group(3))

                owner_leaf = owner.split('::')[-1]
                if method == owner_leaf or method == f'~{owner_leaf}' or method.startswith('operator'):
                    continue

                signature_key = f'{owner}::{method}({params})'
                sig_list = signature_locations.setdefault(signature_key, [])
                if any(existing['file'] == rel_file for existing in sig_list):
                    continue
                sig_list.append({'file': rel_file, 'line': idx, 'context': stripped})

        for signature, locations in signature_locations.items():
            files = {loc['file'] for loc in locations}
            if len(files) <= 1:
                continue

            files_sorted = sorted(files)
            if self._is_likely_mirror_or_variant_pair(files_sorted):
                continue

            dup_text = ', '.join(files_sorted)
            for loc in locations:
                self.gaps.append({
                    'file': loc['file'],
                    'line': loc['line'],
                    'category': 'legacy_duplication',
                    'severity': 'MEDIUM',
                    'pattern': 'duplicate_qualified_signature',
                    'description': f'Potential duplicate implementation signature across files: {signature}',
                    'context': loc['context'],
                    'duplicate_in': dup_text,
                })

        return self.gaps
