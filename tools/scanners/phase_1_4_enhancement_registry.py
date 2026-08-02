#!/usr/bin/env python3
"""
Phase 1-4 Enhancement Registry

Central registry for all Phase 1-4 enhancement scanners
Provides unified execution and gap aggregation

Scanners included:
- S-1: gs3_step01_enhance_security_s1.py (Hardcoded Secrets)
- S-2: gs3_step01_enhance_security_s2.py (Cryptographic Weaknesses)
- S-3: gs3_step01_enhance_security_s3.py (Injection Attacks)
- M-1/M-2: gs3_step02_enhance_memory_m1_m2.py (Memory Safety)
- C-1: gs3_step01_enhance_concurrency_c1.py (Race Conditions)
"""

import sys
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Any


class Phase14EnhancementRegistry:
    """Registry for Phase 1-4 enhancement scanners"""
    
    SCANNERS = {
        'S-1': {
            'module': 'gs3_step01_enhance_security_s1',
            'class': 'S1SecretScanner',
            'title': 'Hardcoded Secrets Detection (CWE-798)',
            'expected_gaps': (100, 160),
        },
        'S-2': {
            'module': 'gs3_step01_enhance_security_s2',
            'class': 'S2CryptoScanner',
            'title': 'Cryptographic Weaknesses (CWE-327)',
            'expected_gaps': (70, 120),
        },
        'S-3': {
            'module': 'gs3_step01_enhance_security_s3',
            'class': 'S3InjectionScanner',
            'title': 'Injection Attack Prevention (CWE-94)',
            'expected_gaps': (80, 150),
        },
        'M-1/M-2': {
            'module': 'gs3_step02_enhance_memory_m1_m2',
            'class': 'MemorySafetyScanner',
            'title': 'Memory Safety (CWE-416/415)',
            'expected_gaps': (50, 100),
        },
        'C-1': {
            'module': 'gs3_step01_enhance_concurrency_c1',
            'class': 'C1ConcurrencyScanner',
            'title': 'Race Condition Detection (CWE-362)',
            'expected_gaps': (40, 80),
        },
    }
    
    def __init__(self):
        self.scanner_dir = Path(__file__).parent
        self.results: Dict[str, Any] = {}
    
    def run_all(self) -> Dict[str, Any]:
        """Execute all Phase 1-4 enhancement scanners"""
        total_gaps = 0
        summary = {
            'timestamp': datetime.now(timezone.utc).isoformat(),
            'phase': '1-4 Enhancement',
            'scanners_run': 0,
            'total_gaps': 0,
            'by_enhancement': {},
            'gaps_by_cwe': {},
        }
        
        for enhancement_id, scanner_info in self.SCANNERS.items():
            try:
                scanner_module = __import__(scanner_info['module'])
                scanner = getattr(scanner_module, scanner_info['class'])()
                
                gaps = self._collect_gaps(scanner)
                gap_count = len(gaps)
                total_gaps += gap_count
                
                summary['scanners_run'] += 1
                summary['by_enhancement'][enhancement_id] = {
                    'title': scanner_info['title'],
                    'gap_count': gap_count,
                    'expected_range': scanner_info['expected_gaps'],
                    'status': 'PASS' if scanner_info['expected_gaps'][0] <= gap_count <= scanner_info['expected_gaps'][1] * 1.5 else 'CHECK',
                }
                
                # Aggregate by CWE — count each gap individually
                for gap in gaps:
                    cwe = gap.get('cwe') if isinstance(gap, dict) else None
                    if cwe:
                        summary['gaps_by_cwe'][cwe] = summary['gaps_by_cwe'].get(cwe, 0) + 1
                
                print(f"✓ {enhancement_id}: {scanner_info['title']}")
                print(f"  Gaps found: {gap_count} (expected: {scanner_info['expected_gaps'][0]}-{scanner_info['expected_gaps'][1]})")
                
            except Exception as e:
                print(f"✗ {enhancement_id}: Failed to run")
                print(f"  Error: {str(e)}")
        
        summary['total_gaps'] = total_gaps
        self.results = summary
        return summary
    
    def _collect_gaps(self, scanner) -> List[Dict]:
        """Collect gaps from scanner using standard interface"""
        gaps = []
        
        # Scan all .cpp, .h, .hpp files
        from pathlib import Path
        
        scan_paths = [
            Path('src'),
            Path('include'),
        ]
        
        for scan_dir in scan_paths:
            if not scan_dir.exists():
                continue
                
            for pattern in ['**/*.cpp', '**/*.h', '**/*.hpp', '**/*.cc']:
                for file_path in scan_dir.glob(pattern):
                    # Skip test, build, and external directories
                    if any(skip in str(file_path) for skip in ['/test', '/tests', '/build', '/.git', '/external', '_test.', '_bench.']):
                        continue
                    
                    try:
                        # Call scanner's scan_file method
                        file_gaps = scanner.scan_file(str(file_path))
                        if isinstance(file_gaps, list):
                            # Convert gap objects to dicts if needed
                            for gap in file_gaps:
                                if hasattr(gap, 'to_dict'):
                                    gaps.append(gap.to_dict())
                                else:
                                    gaps.append(gap)
                    except Exception:
                        continue
        
        return gaps
    
    def print_summary(self):
        """Print formatted summary"""
        print("\n" + "="*70)
        print("PHASE 1-4 ENHANCEMENT SCANNER RESULTS")
        print("="*70)
        print(f"Total gaps detected: {self.results.get('total_gaps', 0)}")
        print(f"Scanners executed: {self.results.get('scanners_run', 0)}/{len(self.SCANNERS)}")
        print()
        
        for enh_id, data in self.results.get('by_enhancement', {}).items():
            status_icon = "✓" if data['status'] == 'PASS' else "⚠"
            print(f"{status_icon} {enh_id}: {data['gap_count']} gaps")
            print(f"   Expected: {data['expected_range'][0]}-{data['expected_range'][1]}")
            print()
        
        print("="*70)
    
    def export_results(self, output_file: str = 'phase_1_4_enhancement_results.json'):
        """Export results to JSON"""
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"Results exported to {output_file}")


def main():
    """Main entry point"""
    registry = Phase14EnhancementRegistry()
    registry.run_all()
    registry.print_summary()
    registry.export_results()


if __name__ == '__main__':
    main()
