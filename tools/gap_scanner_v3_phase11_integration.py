#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Phase 11 Security Hardening Integration

Integrates Phase 11 security scanners into the main gap audit pipeline:
- Data Leak Detection (P11-1)
- Encryption Leak Detection (P11-2) — placeholder
- E2E Security Encryption (P11-3) — placeholder
- Key Failure Detection (P11-4)
- Attack Vector Detection (P11-5) — placeholder
- Military Hardening (P11-6) — placeholder
"""

import sys
import json
from pathlib import Path
from typing import Dict, List
from dataclasses import asdict


def load_phase11_scanners(repo_root: str = '.'):
    """Load all Phase 11 security scanners."""
    scanners = {}
    
    try:
        # Try importing Data Leak Scanner (P11-1)
        sys.path.insert(0, str(Path(repo_root) / 'tools'))
        
        from gap_scanner_v3_phase11_data_leak import DataLeakScanner
        scanners['data_leak'] = DataLeakScanner(repo_root)
        print("[Phase 11] Loaded P11-1: Data Leak Detection Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-1 Data Leak Scanner: {e}")
    
    try:
        # P11-2: Encryption Leak Detection
        from gap_scanner_v3_phase11_encryption_leak import EncryptionLeakScanner
        scanners['encryption_leak'] = EncryptionLeakScanner(repo_root)
        print("[Phase 11] Loaded P11-2: Encryption Leak Detection Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-2 Encryption Leak Scanner: {e}")
    
    try:
        # P11-3: E2E Security Encryption
        from gap_scanner_v3_phase11_e2e_encryption import E2EEncryptionScanner
        scanners['e2e_encryption'] = E2EEncryptionScanner(repo_root)
        print("[Phase 11] Loaded P11-3: E2E Security Encryption Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-3 E2E Encryption Scanner: {e}")
    
    try:
        # P11-4: Key Failure Detection
        from gap_scanner_v3_phase11_key_failure import KeyFailureScanner
        scanners['key_failure'] = KeyFailureScanner(repo_root)
        print("[Phase 11] Loaded P11-4: Key Failure Detection Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-4 Key Failure Scanner: {e}")
    
    try:
        # P11-5: Attack Vector Detection
        from gap_scanner_v3_phase11_attack_vectors import AttackVectorScanner
        scanners['attack_vectors'] = AttackVectorScanner(repo_root)
        print("[Phase 11] Loaded P11-5: Attack Vector Detection Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-5 Attack Vector Scanner: {e}")
    
    try:
        # P11-6: Military Hardening
        from gap_scanner_v3_phase11_military_hardening import MilitaryHardeningScanner
        scanners['military_hardening'] = MilitaryHardeningScanner(repo_root)
        print("[Phase 11] Loaded P11-6: Military Hardening Scanner")
    except Exception as e:
        print(f"[Phase 11] Failed to load P11-6 Military Hardening Scanner: {e}")
    
    return scanners


def run_phase11_scanners(repo_root: str = '.') -> Dict[str, Dict]:
    """Run all Phase 11 security scanners and aggregate results."""
    scanners = load_phase11_scanners(repo_root)
    
    if not scanners:
        print("[Phase 11] No scanners loaded")
        return {}
    
    results = {
        'phase_11': {
            'timestamp': __import__('datetime').datetime.now().isoformat(),
            'scanners': {},
            'total_gaps': 0,
            'gaps_by_type': {},
        }
    }
    
    # Run each scanner
    for scanner_name, scanner in scanners.items():
        print(f"[Phase 11] Running {scanner_name} scanner...")
        
        try:
            gaps = scanner.scan_repository()
            
            # Convert gaps to dict format
            gaps_list = []
            for file_path, file_gaps in gaps.items():
                for gap in file_gaps:
                    gap_dict = gap.to_dict()
                    gaps_list.append(gap_dict)
                    
                    # Track gap types
                    gap_type = gap.gap_type.value
                    if gap_type not in results['phase_11']['gaps_by_type']:
                        results['phase_11']['gaps_by_type'][gap_type] = 0
                    results['phase_11']['gaps_by_type'][gap_type] += 1
            
            results['phase_11']['scanners'][scanner_name] = {
                'total': len(gaps_list),
                'gaps': gaps_list,
            }
            results['phase_11']['total_gaps'] += len(gaps_list)
            
            print(f"  > {scanner_name}: {len(gaps_list)} gaps found")
        
        except Exception as e:
            print(f"  > Error running {scanner_name}: {e}")
            import traceback
            traceback.print_exc()
    
    return results


def merge_phase11_into_summary(phase11_results: Dict, existing_summary: Dict) -> Dict:
    """Merge Phase 11 results into existing gap summary."""
    if 'phase_11' in phase11_results:
        # Add Phase 11 data
        phase11_data = phase11_results['phase_11']
        existing_summary['phase_11_gaps'] = phase11_data['total_gaps']
        existing_summary['phase_11_by_type'] = phase11_data['gaps_by_type']
        
        # Update total gaps
        if 'total_gaps' in existing_summary:
            existing_summary['total_gaps'] += phase11_data['total_gaps']
        
        # Update severity breakdown
        for scanner_name, scanner_data in phase11_data['scanners'].items():
            for gap in scanner_data['gaps']:
                severity = gap.get('severity', 'MEDIUM')
                key = f'{severity.lower()}_gaps'
                if key not in existing_summary:
                    existing_summary[key] = 0
                existing_summary[key] += 1
    
    return existing_summary


if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='Phase 11 Security Scanner Integration')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    # Run Phase 11 scanners
    results = run_phase11_scanners(args.repo)
    
    # Output results
    output_json = json.dumps(results, indent=2)
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output_json)
        print(f"\n[Phase 11] Results written to {args.output}")
    else:
        print(output_json)
    
    # Print summary
    if 'phase_11' in results:
        p11 = results['phase_11']
        print(f"\n=== Phase 11 Security Hardening Summary ===")
        print(f"Total gaps found: {p11['total_gaps']}")
        print(f"Gaps by type:")
        for gap_type, count in p11['gaps_by_type'].items():
            print(f"  - {gap_type}: {count}")
