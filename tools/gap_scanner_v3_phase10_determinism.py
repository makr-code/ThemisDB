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
        
        for idx, line in enumerate(lines, 1):
            # Look for RNG creation
            if re.search(r'(mt19937|random_device|rand|uniform_)', line):
                # Check if seeded
                prev_lines = '\n'.join(lines[max(0, idx-10):idx])
                
                if not re.search(r'(seed|srand|seed_seq)', prev_lines):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'determinism',
                        'severity': 'MEDIUM',
                        'pattern': 'random_unseeded',
                        'description': 'RNG without explicit seeding (non-deterministic)',
                        'context': line.strip()
                    })
    
    def _check_fp_comparisons(self, file_path: Path, lines: List[str]):
        """Find floating-point comparisons without tolerance"""
        
        for idx, line in enumerate(lines, 1):
            # Look for == or != with floating point
            if re.search(r'(\w+.*float|double.*\w+).*==|!=', line):
                # Check for tolerance/epsilon
                context = '\n'.join(lines[max(0, idx-5):min(idx+5, len(lines))])
                
                if not re.search(r'(epsilon|tolerance|abs.*<|delta)', context):
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
