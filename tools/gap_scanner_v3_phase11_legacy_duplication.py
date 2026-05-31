#!/usr/bin/env python3
"""
Phase 11: Legacy Paths & Duplicate Implementation Scanner

Detects:
- Legacy/compatibility/fallback code paths that should be reviewed
- Potential duplicate C++ method implementations across multiple files

Goal:
Reduce blind spots around maintainability debt and accidental duplicate logic.
"""

import re
from pathlib import Path
from typing import List, Dict


class LegacyDuplicationScan:
    """Scan for legacy code paths and duplicate implementation signatures."""

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

        self._legacy_re = re.compile(
            r'\b(legacy|compat(?:ibility)?|deprecated|old[_\s-]?path|backward[_\s-]?compat(?:ibility)?)\b',
            re.IGNORECASE,
        )

        # Qualified C++ method definitions: Namespace::Class::method(...)
        self._qualified_method_re = re.compile(
            r'^\s*(?:template\s*<[^>]*>\s*)?'
            r'(?:(?:inline|static|constexpr|virtual)\s+)*'
            r'(?:(?:[\w:<>,~*&]+)\s+)+'
            r'([A-Za-z_][\w:]*)::(~?[A-Za-z_]\w*)\s*\(([^;]*)\)\s*(?:const)?\s*(?:\{|$)'
        )
        self._variant_suffix_re = re.compile(
            r'(_openssl|_stub|_mock|_cpu|_gpu|_cuda|_rocm|_vulkan|_metal|_directx|_sim|_test)$',
            re.IGNORECASE,
        )

    @staticmethod
    def _normalize_params(param_text: str) -> str:
        """Normalize parameter list to reduce formatting-only differences."""
        collapsed = re.sub(r'\s+', ' ', param_text.strip())
        return collapsed

    @staticmethod
    def _looks_like_definition(lines: List[str], index: int) -> bool:
        """Best-effort check that current line belongs to a function definition."""
        line = lines[index]
        if ';' in line and '{' not in line:
            return False
        if '{' in line:
            return True

        # Multi-line signature: look ahead for opening brace before statement terminator.
        for j in range(index + 1, min(index + 8, len(lines))):
            nxt = lines[j]
            if ';' in nxt and '{' not in nxt:
                return False
            if '{' in nxt:
                return True
        return False

    def _is_likely_mirror_or_variant_pair(self, files: List[str]) -> bool:
        """Return True for known benign duplicate patterns across mirrored/variant files."""
        if len(files) != 2:
            return False

        p0 = Path(files[0].replace('\\', '/'))
        p1 = Path(files[1].replace('\\', '/'))
        b0 = p0.stem
        b1 = p1.stem

        # Exact mirrored filename in different module folders is often intentional.
        if p0.name == p1.name:
            return True

        # Common backend variant pairs (foo.cpp + foo_openssl.cpp etc.).
        if b0 == b1:
            return True
        if b0.startswith(b1 + '_') or b1.startswith(b0 + '_'):
            return True
        if self._variant_suffix_re.search(b0) or self._variant_suffix_re.search(b1):
            return True

        # Known mirrored module pair in this repository.
        pair = {p0.parts[1] if len(p0.parts) > 1 else '', p1.parts[1] if len(p1.parts) > 1 else ''}
        if pair == {'base', 'themis'}:
            return True

        return False

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files and return all detected gaps."""
        self.gaps = []
        signature_locations: Dict[str, List[Dict]] = {}

        for file_path in file_list:
            if file_path.suffix not in ['.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx']:
                continue

            try:
                lines = file_path.read_text(encoding='utf-8', errors='ignore').splitlines()
            except Exception:
                continue

            rel_file = str(file_path.relative_to(self.repo_root))

            for idx, line in enumerate(lines, 1):
                stripped = line.strip()

                # Legacy/compat/deprecation markers in comments or preprocessor lines.
                is_comment_or_pp = (
                    stripped.startswith('//') or stripped.startswith('/*') or
                    stripped.startswith('*') or stripped.startswith('#')
                )
                if is_comment_or_pp and self._legacy_re.search(stripped):
                    self.gaps.append({
                        'file': rel_file,
                        'line': idx,
                        'category': 'legacy_duplication',
                        'severity': 'HIGH',
                        'pattern': 'legacy_or_compat_path',
                        'description': 'Legacy/compatibility/deprecation marker detected (review removal/containment plan).',
                        'context': stripped,
                    })

                # Capture potential duplicate implementation signatures.
                if file_path.suffix not in ['.cpp', '.cc', '.cxx']:
                    continue

                m = self._qualified_method_re.search(line)
                if m:
                    if not self._looks_like_definition(lines, idx - 1):
                        continue

                    qualified_owner = m.group(1)
                    method_name = m.group(2)
                    param_text = self._normalize_params(m.group(3))

                    # Ignore constructors/destructors/operators; they are noisy for this check.
                    owner_leaf = qualified_owner.split('::')[-1]
                    if method_name == owner_leaf or method_name == f'~{owner_leaf}' or method_name.startswith('operator'):
                        continue

                    signature_key = f'{qualified_owner}::{method_name}({param_text})'
                    sig_list = signature_locations.setdefault(signature_key, [])
                    if any(existing['file'] == rel_file for existing in sig_list):
                        continue
                    sig_list.append({
                        'file': rel_file,
                        'line': idx,
                        'context': stripped,
                    })

        # Report signatures implemented in multiple files.
        for signature, locations in signature_locations.items():
            files = {loc['file'] for loc in locations}
            if len(files) <= 1:
                continue

            files_sorted = sorted(files)
            if self._is_likely_mirror_or_variant_pair(files_sorted):
                continue

            file_list_text = ', '.join(files_sorted)
            for loc in locations:
                self.gaps.append({
                    'file': loc['file'],
                    'line': loc['line'],
                    'category': 'legacy_duplication',
                    'severity': 'MEDIUM',
                    'pattern': 'duplicate_qualified_signature',
                    'description': f'Potential duplicate implementation signature across files: {signature}',
                    'context': loc['context'],
                    'duplicate_in': file_list_text,
                })

        return self.gaps
