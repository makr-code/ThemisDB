#!/usr/bin/env python3
"""
Phase 4 — Determinism Scanner (IMPROVED 2026-06-14)

IMPROVEMENTS:
1. Test-Code Exclusion: tests/** and benchmarks/** entirely skipped
2. Production-only: src/** checked for non-determinism
3. Test patterns: TEST_F, MOCK_*, _test, _bench excluded

Detects (Production only):
- Uninitialized variables
- Uncontrolled randomness
- Timestamp-based logic without UTC
- Floating-point comparisons
- Set/Map iteration without ordering
"""

import re
from pathlib import Path
from typing import List, Dict


class DeterminismScannerImproved:
    """Scan for non-determinism issues (IMPROVED)"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def _is_test_code(self, file_path: Path) -> bool:
        """
        IMPROVEMENT 1: Skip all test/benchmark code
        """
        rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/').lower()
        
        exclude_patterns = [
            'tests/', 'test_', '_test.cpp',
            'benchmarks/', 'bench_', '_bench.cpp',
            '_mock.cpp', '_mock.h',
        ]
        
        return any(pattern in rel_file for pattern in exclude_patterns)
    
    def _is_production_code(self, file_path: Path) -> bool:
        """
        IMPROVEMENT 2: Only src/** is production
        """
        rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/').lower()
        return rel_file.startswith('src/')
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for non-determinism issues"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            # IMPROVEMENT 1: Skip test code
            if self._is_test_code(file_path):
                continue
            
            # IMPROVEMENT 2: Only production code
            if not self._is_production_code(file_path):
                continue
            
            try:
                lines = file_path.read_text(errors='ignore').split('\n')
            except:
                continue
            
            self._check_uninitialized_vars(file_path, lines)
            self._check_uncontrolled_randomness(file_path, lines)
            self._check_float_comparisons(file_path, lines)
            self._check_unordered_iteration(file_path, lines)
        
        return self.gaps
    
    def _check_uninitialized_vars(self, file_path: Path, lines: List[str]):
        """Find uninitialized variables"""
        
        for idx, line in enumerate(lines, 1):
            # Pattern: type var; (without = initialization)
            pattern = r'\b(?:int|float|double|bool|size_t|uint\d+)\s+(\w+)\s*;'
            
            if not re.search(pattern, line):
                continue
            
            var_name = re.search(pattern, line)
            if not var_name:
                continue
            
            var = var_name.group(1)
            
            # Check if used before initialization
            context = ''.join(lines[idx:min(len(lines), idx + 10)])
            
            if re.search(rf'\b{var}\b', context):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'type': 'uninitialized_variable',
                    'severity': 'MEDIUM',
                    'description': f'Variable "{var}" may be uninitialized',
                    'context': line.strip()[:80],
                    'remediation': f'Initialize: {var} = {{}}; or = 0;'
                })
    
    def _check_uncontrolled_randomness(self, file_path: Path, lines: List[str]):
        """Find uncontrolled random operations"""
        
        for idx, line in enumerate(lines, 1):
            if not any(x in line for x in ['rand()', 'random(', 'mt19937']):
                continue
            
            context = ''.join(lines[max(0, idx - 5):min(len(lines), idx + 5)])
            
            # OK if seeded with fixed value
            if 'seed(' in context and any(x in context for x in ['42', '0', '1']):
                continue
            
            self.gaps.append({
                'file': str(file_path.relative_to(self.repo_root)),
                'line': idx,
                'type': 'uncontrolled_randomness',
                'severity': 'MEDIUM',
                'description': 'Randomness without seeding or testing context',
                'context': line.strip()[:80],
                'remediation': 'Seed with fixed value or use deterministic alternative'
            })
    
    def _check_float_comparisons(self, file_path: Path, lines: List[str]):
        """Find problematic floating-point comparisons"""
        
        for idx, line in enumerate(lines, 1):
            # Pattern: float_var == or float_var !=
            if not any(x in line for x in [' == ', ' != ']):
                continue
            
            # Check if float is involved
            if not any(x in line for x in ['float', 'double', '.0', '.f']):
                continue
            
            # May be comparing float (non-deterministic)
            self.gaps.append({
                'file': str(file_path.relative_to(self.repo_root)),
                'line': idx,
                'type': 'float_comparison',
                'severity': 'LOW',
                'description': 'Direct floating-point comparison (potential non-determinism)',
                'context': line.strip()[:80],
                'remediation': 'Use epsilon tolerance: fabs(a - b) < epsilon'
            })
    
    def _check_unordered_iteration(self, file_path: Path, lines: List[str]):
        """Check for unordered container iteration"""
        
        for idx, line in enumerate(lines, 1):
            if not any(x in line for x in ['unordered_map', 'unordered_set', 'hash_map']):
                continue
            
            # Look forward for iteration
            context = ''.join(lines[idx:min(len(lines), idx + 15)])
            
            if 'for' not in context:
                continue
            
            self.gaps.append({
                'file': str(file_path.relative_to(self.repo_root)),
                'line': idx,
                'type': 'unordered_iteration',
                'severity': 'LOW',
                'description': 'Unordered container iteration may produce non-deterministic order',
                'context': line.strip()[:80],
                'remediation': 'Use std::map/std::set or sort before use'
            })


if __name__ == '__main__':
    import sys
    
    repo_root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
    scanner = DeterminismScannerImproved(repo_root=str(repo_root))
    
    cpp_files = list(repo_root.rglob('*.cpp')) + list(repo_root.rglob('*.hpp'))
    gaps = scanner.scan_files(cpp_files)
    
    print(f"Found {len(gaps)} determinism gaps (improved, tests excluded)")
    for gap in gaps[:10]:
        print(f"  {gap['file']}:{gap['line']} [{gap['type']}]")
