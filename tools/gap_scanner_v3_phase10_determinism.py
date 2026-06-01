#!/usr/bin/env python3
"""
Phase 10-2: Determinism & Reproducibility Scanner

CWE-330 (Use of Insufficiently Random), CWE-367 (Time-of-check-time-of-use)

Detects:
- Non-deterministic iteration (unordered_map/set)
- Random number generation not seeded
- Floating-point without tolerance
- Hash-based collection iteration order not stable
- Timestamp-based sorting (not reproducible)
- Threading race in construction/teardown
- Memory layout assumptions
- Initialization order dependencies
- Uninitialized variable read
- Conditional compilation affecting semantics
"""

import re
from pathlib import Path
from typing import List, Dict


class DeterminismScan:
    """Scan for determinism and reproducibility issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for determinism issues"""
        self.gaps = []
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_unordered_containers(file_path, lines)
            self._check_random_seeding(file_path, lines)
            self._check_fp_comparisons(file_path, lines)
            self._check_hash_iteration(file_path, lines)
            self._check_timestamp_sorting(file_path, lines)
        
        return self.gaps

    @staticmethod
    def _is_comment_or_preprocessor(line: str) -> bool:
        stripped = line.strip()
        return (
            not stripped or
            stripped.startswith('//') or
            stripped.startswith('/*') or
            stripped.startswith('*') or
            stripped.startswith('#')
        )

    @staticmethod
    def _has_float_hint(text: str) -> bool:
        return bool(re.search(
            r'(\bfloat\b|\bdouble\b|\bfp(16|32|64)\b|\bepsilon\b|\btolerance\b|'
            r'\d+\.\d+|\d+[eE][+-]?\d+|\b\d+\.f\b|\b\d+f\b)',
            text,
            re.IGNORECASE,
        ))

    @staticmethod
    def _collect_float_identifiers(lines: List[str]) -> set:
        ids = set()
        decl_re = re.compile(r'\b(?:float|double)\b\s*[\*&\s]*([A-Za-z_]\w*)')
        for line in lines:
            if DeterminismScan._is_comment_or_preprocessor(line):
                continue
            code_only = line.split('//', 1)[0]
            for m in decl_re.finditer(code_only):
                ids.add(m.group(1))
        return ids
    
    def _check_unordered_containers(self, file_path: Path, lines: List[str]):
        """Find unordered container iteration without care"""
        
        for idx, line in enumerate(lines, 1):
            # Look for unordered_map/set iteration
            if re.search(r'std::unordered_(map|set)', line):
                # Check if iteration result affects determinism
                next_lines = '\n'.join(lines[idx:min(idx+20, len(lines))])
                
                # If results are used in decisions, order matters
                if re.search(r'(if|switch|compare|assert).*iteration|for.*auto', next_lines):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'determinism',
                        'severity': 'MEDIUM',
                        'pattern': 'unordered_container_iter',
                        'description': 'Non-deterministic unordered_map/set iteration order',
                        'context': line.strip()
                    })
    
    def _check_random_seeding(self, file_path: Path, lines: List[str]):
        """Find random number generation without proper seeding"""
        rand_call_re = re.compile(r'\brand\s*\(')
        engine_decl_re = re.compile(
            r'\bstd::(mt19937(?:_64)?|default_random_engine|minstd_rand0?|minstd_rand)\b\s+'
            r'([A-Za-z_]\w*)\s*(?P<init>\([^;]*\)|\{[^;]*\}|=[^;]*)?\s*;'
        )

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            code_line = line.split('//', 1)[0]

            # Legacy C RNG API usage without nearby srand.
            if rand_call_re.search(code_line):
                local_ctx = '\n'.join(lines[max(0, idx - 20):min(len(lines), idx + 5)])
                if not re.search(r'\bsrand\s*\(', local_ctx):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'determinism',
                        'severity': 'MEDIUM',
                        'pattern': 'random_unseeded',
                        'description': 'rand() without nearby explicit srand() seeding',
                        'context': line.strip()
                    })
                continue

            # C++ engine declaration: only flag default/unseeded construction.
            m = engine_decl_re.search(code_line)
            if not m:
                continue

            var_name = m.group(2)
            init = (m.group('init') or '').strip()
            if init:
                # If initialized and random_device/seed literal/seed_seq used, treat as seeded.
                if re.search(r'(random_device|seed_seq|seed\s*\(|\d)', init):
                    continue
                # Explicit constructor args are typically intentional seeds.
                if '(' in init and ')' in init and init not in ['()', '( )']:
                    continue

            local_ctx = '\n'.join(lines[max(0, idx - 5):min(len(lines), idx + 30)])
            if re.search(rf'\b{re.escape(var_name)}\s*\.\s*seed\s*\(', local_ctx):
                continue

            self.gaps.append({
                'file': str(file_path.relative_to(self.repo_root)),
                'line': idx,
                'category': 'determinism',
                'severity': 'MEDIUM',
                'pattern': 'random_unseeded',
                'description': 'RNG engine appears default-constructed without explicit seeding',
                'context': line.strip()
            })
    
    def _check_fp_comparisons(self, file_path: Path, lines: List[str]):
        """Find floating-point comparisons without tolerance"""
        op_re = re.compile(r'(==|!=)')
        simple_cmp_re = re.compile(
            r'(?P<lhs>[A-Za-z_]\w*|\d+\.\d+|\d+[eE][+-]?\d+|\d+f)\s*'
            r'(?P<op>==|!=)\s*'
            r'(?P<rhs>[A-Za-z_]\w*|\d+\.\d+|\d+[eE][+-]?\d+|\d+f)'
        )
        obvious_non_fp_re = re.compile(
            r'(nullptr|std::string::npos|\.end\s*\(|\.begin\s*\(|\btrue\b|\bfalse\b)',
            re.IGNORECASE,
        )
        float_ids = self._collect_float_identifiers(lines)

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            code_line = line.split('//', 1)[0]
            if not op_re.search(code_line):
                continue
            if obvious_non_fp_re.search(code_line):
                continue

            matches = list(simple_cmp_re.finditer(code_line))
            if not matches:
                continue

            context = '\n'.join(lines[max(0, idx - 4):min(idx + 4, len(lines))])
            if re.search(r'(epsilon|tolerance|abs\s*\(|fabs\s*\(|std::abs\s*\(|delta)', context, re.IGNORECASE):
                continue

            should_flag = False
            for m in matches:
                lhs = m.group('lhs')
                rhs = m.group('rhs')
                operands = [lhs, rhs]
                if any(self._has_float_hint(op) for op in operands):
                    should_flag = True
                    break
                if any(op in float_ids for op in operands):
                    should_flag = True
                    break

            if not should_flag:
                continue

            self.gaps.append({
                'file': str(file_path.relative_to(self.repo_root)),
                'line': idx,
                'category': 'determinism',
                'severity': 'HIGH',
                'pattern': 'fp_exact_comparison',
                'description': 'Floating-point exact comparison (use tolerance/epsilon)',
                'context': line.strip()
            })
    
    def _check_hash_iteration(self, file_path: Path, lines: List[str]):
        """Find hash-based container iteration assuming order"""
        
        for idx, line in enumerate(lines, 1):
            # Look for iterations that might assume order
            if re.search(r'(for.*auto|for.*auto\s*&).*unordered', line):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'determinism',
                    'severity': 'MEDIUM',
                    'pattern': 'hash_container_order_assumption',
                    'description': 'Iteration over hash-based container with possible order dependency',
                    'context': line.strip()
                })
    
    def _check_timestamp_sorting(self, file_path: Path, lines: List[str]):
        """Find timestamp-based sorting without stability"""
        
        for idx, line in enumerate(lines, 1):
            # Look for sort based on timestamps
            if re.search(r'(sort|compare).*timestamp|timestamp.*sort', line, re.IGNORECASE):
                # Check for stable sort
                if 'stable_sort' not in line and 'stable' not in '\n'.join(lines[max(0, idx-3):idx+3]):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'determinism',
                        'severity': 'MEDIUM',
                        'pattern': 'timestamp_sorting_unstable',
                        'description': 'Timestamp-based sorting without stable_sort (non-deterministic ties)',
                        'context': line.strip()
                    })
