#!/usr/bin/env python3
"""
Phase 4 — Observability Scanner (IMPROVED 2026-06-14)

IMPROVEMENTS:
1. Test/Benchmark Exclusion: benchmarks/** and tests/** entirely skipped
2. Production-only: Only src/** checked for trace points
3. Mock patterns: MOCK_*, _mock, stub patterns excluded

Detects (Production only):
- Missing trace/observability points in critical paths
- Non-deterministic output
- Timestamps without UTC timezone
"""

import re
from pathlib import Path
from typing import List, Dict


class ObservabilityScannerImproved:
    """Scan for missing observability/tracing (IMPROVED)"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
        
        # Critical paths that should have observability
        self.critical_functions = [
            'execute_query', 'handle_request', 'process_batch',
            'replicate', 'failover', 'recover', 'commit',
            'gc_run', 'compact', 'merge'
        ]
    
    def _is_test_code(self, file_path: Path) -> bool:
        """
        IMPROVEMENT 1: Skip test and benchmark files
        """
        rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/').lower()
        
        # IMPROVEMENT 1: Exclude benchmarks and tests
        exclude_patterns = [
            'tests/', 'test_', '_test.cpp',
            'benchmarks/', 'bench_', '_bench.cpp',
            'examples/', 'demo_', '_demo.cpp',
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
        """Scan files for observability gaps"""
        
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
            
            self._check_missing_trace_points(file_path, lines)
            self._check_determinism(file_path, lines)
        
        return self.gaps
    
    def _check_missing_trace_points(self, file_path: Path, lines: List[str]):
        """Find critical paths without tracing"""
        
        for idx, line in enumerate(lines, 1):
            # Look for critical functions
            for func in self.critical_functions:
                if not re.search(rf'\b{func}\s*\(', line, re.IGNORECASE):
                    continue
                
                # Look forward for trace/observability
                context = ''.join(lines[idx:min(len(lines), idx + 20)])
                
                trace_keywords = [
                    'trace', 'span', 'scope', 'meter',
                    'histogram', 'counter', 'gauge',
                    'TRACE_', 'SPAN_', 'metrics', 'observ',
                ]
                
                if any(x in context for x in trace_keywords):
                    continue
                
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'type': 'missing_trace_point',
                    'severity': 'MEDIUM',
                    'description': f'Critical function "{func}" missing trace point',
                    'context': line.strip()[:80],
                    'remediation': 'Add: auto span = tracer->start_span("name");'
                })
    
    def _check_determinism(self, file_path: Path, lines: List[str]):
        """Check for non-deterministic operations"""
        
        for idx, line in enumerate(lines, 1):
            # Look for randomness/timestamps without UTC
            if any(x in line for x in ['rand()', 'random(', 'time(', 'chrono::now(', 'clock()']):
                context = ''.join(lines[max(0, idx - 3):min(len(lines), idx + 3)])
                
                # If used for comparison or assertion, OK
                if any(x in context for x in ['assert', 'check', 'verify']):
                    continue
                
                # If seeded properly, OK
                if 'seed' in context or 'SetSeed' in context:
                    continue
                
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'type': 'non_deterministic',
                    'severity': 'MEDIUM',
                    'description': 'Non-deterministic operation (randomness/time)',
                    'context': line.strip()[:80],
                    'remediation': 'Seed randomness or use UTC timestamps'
                })


if __name__ == '__main__':
    import sys
    
    repo_root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
    scanner = ObservabilityScannerImproved(repo_root=str(repo_root))
    
    cpp_files = list(repo_root.rglob('*.cpp')) + list(repo_root.rglob('*.hpp'))
    gaps = scanner.scan_files(cpp_files)
    
    print(f"Found {len(gaps)} observability gaps (improved, tests excluded)")
    for gap in gaps[:10]:
        print(f"  {gap['file']}:{gap['line']} [{gap['type']}]")
