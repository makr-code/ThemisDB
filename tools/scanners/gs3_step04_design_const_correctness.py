#!/usr/bin/env python3
"""
Phase 6 - Const Correctness & API Design Scanner

Conservative heuristics for production C++ code:
- const_cast inside const methods
- non-const reference/pointer returned from const methods
- mutable members written from const methods
- logical-const violations via member-pointer writes in const methods
- method-chaining APIs returning non-const *this from const methods
- heavy parameters passed by value without move/ownership transfer
- suspicious const volatile combinations
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Dict, List


class ConstCorrectnessApiScan:
    """Detect const-correctness and API-design gaps with low-noise heuristics."""

    CODE_SUFFIXES = {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}
    NON_PROD_MARKERS = (
        'tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.',
        'examples/', 'demo_', '_demo.', '_mock.', '/mock/', 'fuzz/', 'third_party/', 'external/',
    )
    HEAVY_PARAM_PATTERNS = (
        r'std::string',
        r'std::wstring',
        r'std::u16string',
        r'std::u32string',
        r'std::filesystem::path',
        r'std::vector\s*<[^>]+>',
        r'std::map\s*<[^>]+>',
        r'std::unordered_map\s*<[^>]+>',
        r'std::set\s*<[^>]+>',
        r'std::unordered_set\s*<[^>]+>',
        r'std::function\s*<[^>]+>',
    )
    MUTATING_MEMBER_CALLS = (
        'clear', 'push_back', 'emplace_back', 'insert', 'erase', 'reset', 'assign',
        'swap', 'store', 'fetch_add', 'fetch_sub', 'set', 'update', 'refresh',
    )

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in self.CODE_SUFFIXES:
                continue

            rel_file = self._relative_path(file_path)
            if self._is_non_prod_path(rel_file):
                continue
            if not (rel_file.startswith('src/') or rel_file.startswith('include/')):
                continue

            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')
            except Exception:
                continue

            lines = content.split('\n')
            mutable_members = self._collect_mutable_members(lines)
            methods = self._collect_method_blocks(content)

            self._check_mutable_declarations(rel_file, lines)
            self._check_const_volatile_mix(rel_file, lines)

            for method in methods:
                self._check_const_method_signature(rel_file, method)
                self._check_const_method_body(rel_file, method, mutable_members)
                self._check_heavy_by_value_params(rel_file, method)

        return self.gaps

    def _relative_path(self, file_path: Path) -> str:
        try:
            return str(file_path.relative_to(self.repo_root)).replace('\\', '/')
        except Exception:
            return str(file_path).replace('\\', '/')

    def _is_non_prod_path(self, rel_file: str) -> bool:
        lower = rel_file.lower()
        return any(marker in lower for marker in self.NON_PROD_MARKERS)

    def _emit(self, rel_file: str, line: int, severity: str, pattern: str, description: str, context: str):
        self.gaps.append({
            'file': rel_file,
            'line': line,
            'category': 'const_correctness',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context.strip()[:200],
        })

    def _collect_mutable_members(self, lines: List[str]) -> Dict[str, Dict[str, object]]:
        members: Dict[str, Dict[str, object]] = {}
        for idx, line in enumerate(lines, 1):
            if 'mutable' not in line or '(' in line:
                continue
            match = re.search(r'\bmutable\s+([^;=]+?)\s+([A-Za-z_]\w*)\s*(?:[;=])', line)
            if not match:
                continue
            members[match.group(2)] = {
                'line': idx,
                'type': match.group(1).strip(),
                'context': line.strip(),
            }
        return members

    def _collect_method_blocks(self, content: str) -> List[Dict[str, object]]:
        methods: List[Dict[str, object]] = []
        lines = content.split('\n')
        i = 0

        while i < len(lines):
            line = lines[i]
            stripped = line.strip()
            if not stripped or stripped.startswith('//'):
                i += 1
                continue

            if '(' not in line:
                i += 1
                continue

            signature_lines = [line]
            j = i
            while j + 1 < len(lines) and '{' not in ''.join(signature_lines) and ';' not in ''.join(signature_lines):
                j += 1
                signature_lines.append(lines[j])
                if len(signature_lines) >= 8:
                    break

            signature = ' '.join(x.strip() for x in signature_lines)
            if '{' not in signature or ';' in signature.split('{', 1)[0]:
                i += 1
                continue
            if re.search(r'^\s*(if|for|while|switch|catch)\b', signature):
                i += 1
                continue

            brace_depth = signature.count('{') - signature.count('}')
            body_lines = signature_lines[:]
            k = j
            while brace_depth > 0 and k + 1 < len(lines):
                k += 1
                body_lines.append(lines[k])
                brace_depth += lines[k].count('{') - lines[k].count('}')

            start_line = i + 1
            body_text = '\n'.join(body_lines)
            methods.append({
                'signature': signature.split('{', 1)[0].strip(),
                'body': body_text,
                'line': start_line,
                'is_const': bool(re.search(r'\)\s*(?:const\b|const\s+(?:noexcept|override|final)\b)', signature)),
            })
            i = k + 1

        return methods

    def _check_mutable_declarations(self, rel_file: str, lines: List[str]):
        for idx, line in enumerate(lines, 1):
            if 'mutable' not in line:
                continue
            if not re.search(r'\bmutable\s+std::(?:vector|map|unordered_map|set|unordered_set|string|optional)\b', line):
                continue

            window = '\n'.join(lines[max(0, idx - 3):min(len(lines), idx + 4)])
            if re.search(r'\b(std::mutex|std::shared_mutex|std::atomic|lock_guard|unique_lock)\b', window):
                continue

            self._emit(
                rel_file,
                idx,
                'MEDIUM',
                'mutable_cache_member',
                'Mutable cache-like member without nearby synchronization or ownership note',
                line,
            )

    def _check_const_volatile_mix(self, rel_file: str, lines: List[str]):
        for idx, line in enumerate(lines, 1):
            if re.search(r'\bconst\s+volatile\b|\bvolatile\s+const\b', line):
                self._emit(
                    rel_file,
                    idx,
                    'LOW',
                    'const_volatile_mix',
                    'Suspicious const/volatile combination; document access and lifetime semantics explicitly',
                    line,
                )

    def _check_const_method_signature(self, rel_file: str, method: Dict[str, object]):
        if not method['is_const']:
            return

        signature = str(method['signature'])
        line = int(method['line'])

        ref_match = re.search(r'^(.*?)\b([A-Za-z_~]\w*|operator\s*[^\s(]+)\s*\(', signature)
        if not ref_match:
            return

        return_type = ref_match.group(1).strip()

        if '&' in return_type and 'const' not in return_type.split('&', 1)[0]:
            severity = 'HIGH'
            pattern = 'nonconst_ref_return_from_const'
            description = 'Const method returns non-const reference, allowing mutation through a const API'
            if re.search(r'std::(?:vector|map|unordered_map|set|unordered_set|string|span)', return_type):
                pattern = 'mutable_collection_return_from_const'
                description = 'Const method returns mutable collection/view reference'
            self._emit(rel_file, line, severity, pattern, description, signature)

        ptr_match = re.search(r'(?<!const)\b[\w:<>]+\s*\*\s*$', return_type)
        if ptr_match and 'const' not in return_type.split('*', 1)[0]:
            self._emit(
                rel_file,
                line,
                'MEDIUM',
                'nonconst_ptr_return_from_const',
                'Const method returns non-const pointer, exposing mutable state',
                signature,
            )

    def _check_const_method_body(self, rel_file: str, method: Dict[str, object], mutable_members: Dict[str, Dict[str, object]]):
        if not method['is_const']:
            return

        body = str(method['body'])
        signature = str(method['signature'])
        line = int(method['line'])

        if 'const_cast<' in body or 'const_cast(' in body:
            self._emit(
                rel_file,
                line,
                'CRITICAL',
                'const_cast_in_const_method',
                'const_cast used inside const method breaks const-correctness contract',
                signature,
            )

        mutable_write_detected = False
        for member_name in mutable_members:
            assign_pattern = rf'\b{re.escape(member_name)}\s*='
            mutate_pattern = rf'\b{re.escape(member_name)}\s*(?:\.|->)\s*(?:{"|".join(self.MUTATING_MEMBER_CALLS)})\s*\('
            if re.search(assign_pattern, body) or re.search(mutate_pattern, body):
                mutable_write_detected = True
                self._emit(
                    rel_file,
                    line,
                    'HIGH',
                    'mutable_member_written_in_const_method',
                    f'Mutable member "{member_name}" is written from a const method',
                    signature,
                )

        if re.search(r'\*\s*[A-Za-z_]\w*_\s*=', body):
            self._emit(
                rel_file,
                line,
                'HIGH',
                'member_pointer_write_in_const_method',
                'Const method writes through a member pointer, violating logical const semantics',
                signature,
            )

        mutator_regex = r'(?:' + '|'.join(self.MUTATING_MEMBER_CALLS) + r')'
        if not mutable_write_detected and re.search(rf'\b[A-Za-z_]\w*_\s*(?:\.|->)\s*{mutator_regex}\s*\(', body):
            self._emit(
                rel_file,
                line,
                'HIGH',
                'member_mutation_in_const_method',
                'Const method invokes mutating operation on a member object',
                signature,
            )

        if 'return *this;' in body:
            ret_match = re.search(r'^(.*?)\b([A-Za-z_~]\w*|operator\s*[^\s(]+)\s*\(', signature)
            if ret_match:
                return_type = ret_match.group(1).strip()
                if '&' in return_type and 'const' not in return_type.split('&', 1)[0]:
                    self._emit(
                        rel_file,
                        line,
                        'HIGH',
                        'const_method_returns_nonconst_self',
                        'Const method returns non-const *this, enabling mutating method chains',
                        signature,
                    )

    def _check_heavy_by_value_params(self, rel_file: str, method: Dict[str, object]):
        signature = str(method['signature'])
        body = str(method['body'])
        line = int(method['line'])

        if '&&' in signature:
            return

        for type_pattern in self.HEAVY_PARAM_PATTERNS:
            matches = re.finditer(
                rf'(?:^|[,(])\s*(?:const\s+)?({type_pattern})\s+([A-Za-z_]\w*)\s*(?=[,)])',
                signature,
            )
            for match in matches:
                param_name = match.group(2)
                if re.search(rf'\bstd::move\s*\(\s*{re.escape(param_name)}\s*\)', body):
                    continue
                if re.search(rf'\b{re.escape(param_name)}\s*=', body):
                    continue
                self._emit(
                    rel_file,
                    line,
                    'MEDIUM',
                    'heavy_param_by_value',
                    f'Heavy parameter "{param_name}" passed by value without move/ownership transfer',
                    signature,
                )
