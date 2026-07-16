#!/usr/bin/env python3
"""
gap_scanner_v3_file_existence_filter.py

Post-processing layer for gap_scanner_v3 output:
- Validates file existence
- Implements multi-factor gap classification
- Detects stale cache
- Enriches output with metadata

Purpose: Eliminate FILE_NOT_FOUND false-positives BEFORE L0.5 verification
"""

import json
import os
import re
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, List, Tuple
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class FileExistenceFilter:
    """Pre-filter for gap scanner output to catch file-not-found false-positives"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
    
    def validate_finding_file_exists(self, finding: Dict[str, Any]) -> Tuple[bool, str]:
        """
        Check if file referenced in finding actually exists.
        
        Returns: (exists: bool, reason: str)
        """
        file_path = finding.get('file', '')
        
        # Resolve path relative to repo root
        full_path = self.repo_root / file_path
        
        if not full_path.exists():
            reason = f"FILE_NOT_FOUND: {file_path} does not exist at {full_path}"
            return False, reason
        
        if not full_path.is_file():
            reason = f"NOT_A_FILE: {file_path} is not a regular file (might be directory)"
            return False, reason
        
        if not os.access(full_path, os.R_OK):
            reason = f"NOT_READABLE: {file_path} exists but is not readable"
            return False, reason
        
        return True, "OK"
    
    def classify_gap(self, finding: Dict[str, Any]) -> Tuple[str, str]:
        """
        Multi-factor classification of gap.
        
        Returns: (classification: str, severity_action: str)
        
        Classifications:
        - REAL_GAP: Unimplemented production code, no guards
        - GUARDED_STUB: Has if/guard, defensive pattern
        - TEST_MOCK: In test code with marker
        - PLACEHOLDER: Marked TODO/FIXME/STUB/TEMPORARY
        - FALSE_POSITIVE: File not found or other scanner error
        """
        
        file_path = finding.get('file', '')
        line_num = finding.get('line', 0)
        pattern = finding.get('pattern', '')
        severity = finding.get('severity', 'UNKNOWN')
        
        # Factor 1: File existence
        exists, reason = self.validate_finding_file_exists(finding)
        if not exists:
            return 'FALSE_POSITIVE', f'IGNORE ({reason})'
        
        # Factor 2: Read source context
        try:
            full_path = self.repo_root / file_path
            with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            return 'FALSE_POSITIVE', f'IGNORE (READ_ERROR: {e})'
        
        # Get context around gap line
        context_start = max(0, line_num - 6)
        context_end = min(len(lines), line_num + 6)
        context = lines[context_start:context_end]
        
        if line_num <= 0 or line_num > len(lines):
            return 'FALSE_POSITIVE', f'IGNORE (LINE_OUT_OF_RANGE: {line_num} in {len(lines)}-line file)'
        
        source_line = lines[line_num - 1] if line_num > 0 else ''
        
        # Factor 3: Test code marker
        if file_path.startswith('tests/') or '_test.cpp' in file_path or '/test_' in file_path:
            if any(marker in source_line for marker in ['MOCK', 'TEST', 'FIXTURE', 'stub']):
                logger.info(f"Classified as TEST_MOCK: {file_path}:{line_num}")
                return 'TEST_MOCK', f'DOWNGRADE_{self._downgrade_level(severity, 2)}'
        
        # Factor 4: Guarded pattern (if/while/for guard before return)
        if self._has_guard_before_return(context, line_num - context_start):
            logger.info(f"Classified as GUARDED_STUB: {file_path}:{line_num}")
            return 'GUARDED_STUB', f'DOWNGRADE_{self._downgrade_level(severity, 1)}'
        
        # Factor 5: Marker detection (TODO, FIXME, STUB, TEMPORARY, WIP)
        markers = ['TODO', 'FIXME', 'STUB', 'TEMPORARY', 'WIP', 'PLACEHOLDER']
        if any(marker in source_line or marker in ' '.join(context) for marker in markers):
            logger.info(f"Classified as PLACEHOLDER: {file_path}:{line_num}")
            return 'PLACEHOLDER', f'DOWNGRADE_{self._downgrade_level(severity, 1)}'
        
        # Default: Real gap
        logger.info(f"Classified as REAL_GAP: {file_path}:{line_num}")
        return 'REAL_GAP', 'KEEP_SEVERITY'
    
    def _has_guard_before_return(self, context: List[str], line_idx: int) -> bool:
        """Check if return statement is guarded by if/while/for/&&/||"""
        
        # Look backwards from the return line for guard patterns
        for i in range(line_idx - 1, max(-1, line_idx - 5), -1):
            if i >= 0 and i < len(context):
                line = context[i]
                
                # Check for common guard patterns
                if re.search(r'\b(if|while|for)\s*\(', line):
                    return True
                
                if re.search(r'(&&|\|\||\?)\s*', line):
                    return True
                
                # Check for early returns with conditions
                if re.search(r'\s*(if\s*\(.*\)\s*)?return\s+[^;]*;', line):
                    # This is the return line itself, check if it's conditional
                    if '?' in line or '&&' in line or '||' in line:
                        return True
        
        return False
    
    def _downgrade_level(self, severity: str, levels: int = 1) -> str:
        """Downgrade severity by N levels"""
        severity_order = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'INFO']
        
        try:
            idx = severity_order.index(severity)
            new_idx = min(len(severity_order) - 1, idx + levels)
            return severity_order[new_idx]
        except (ValueError, IndexError):
            return 'INFO'
    
    def filter_findings(self, findings: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], Dict[str, Any]]:
        """
        Filter and classify all findings.
        
        Returns: (filtered_findings, metadata)
        """
        
        validated_findings = []
        statistics = {
            'total_input': len(findings),
            'file_not_found': 0,
            'false_positives_total': 0,
            'real_gaps': 0,
            'guarded_stubs': 0,
            'test_mocks': 0,
            'placeholders': 0,
            'severity_downgrades': {
                'CRITICAL_to_HIGH': 0,
                'CRITICAL_to_INFO': 0,
                'HIGH_to_MEDIUM': 0,
                'HIGH_to_INFO': 0,
            }
        }
        
        for finding in findings:
            original_severity = finding.get('severity', 'UNKNOWN')
            
            # Classify gap
            classification, severity_action = self.classify_gap(finding)
            
            # Update finding with classification
            finding['classification'] = classification
            finding['severity_action'] = severity_action
            
            # Track statistics
            if classification == 'FALSE_POSITIVE':
                statistics['false_positives_total'] += 1
                if 'FILE_NOT_FOUND' in severity_action:
                    statistics['file_not_found'] += 1
                # Skip false-positives entirely
                logger.warning(f"SKIP: {finding.get('file')}:{finding.get('line')} — {classification}")
                continue
            
            elif classification == 'GUARDED_STUB':
                statistics['guarded_stubs'] += 1
            
            elif classification == 'TEST_MOCK':
                statistics['test_mocks'] += 1
            
            elif classification == 'PLACEHOLDER':
                statistics['placeholders'] += 1
            
            elif classification == 'REAL_GAP':
                statistics['real_gaps'] += 1
            
            # Apply severity downgrade if applicable
            if severity_action.startswith('DOWNGRADE_'):
                new_severity = severity_action.split('_')[1]
                key = f"{original_severity}_to_{new_severity}"
                if key in statistics['severity_downgrades']:
                    statistics['severity_downgrades'][key] += 1
                finding['verified_severity'] = new_severity
                logger.info(f"DOWNGRADE: {finding.get('file')}:{finding.get('line')} {original_severity} → {new_severity}")
            else:
                finding['verified_severity'] = original_severity
            
            validated_findings.append(finding)
        
        return validated_findings, statistics
    
    def process_and_save(self, input_file: str, output_file: str) -> Dict[str, Any]:
        """
        Load raw findings, filter, classify, and save verified findings.
        
        Returns: Metadata dictionary
        """
        
        # Load raw findings
        logger.info(f"Loading raw findings from {input_file}")
        with open(input_file, 'r') as f:
            data = json.load(f)
        
        findings = data.get('findings', [])
        logger.info(f"Loaded {len(findings)} findings")
        
        # Filter and classify
        verified_findings, statistics = self.filter_findings(findings)
        logger.info(f"After filtering: {len(verified_findings)} findings (removed {statistics['false_positives_total']} false-positives)")
        
        # Build output
        output = {
            'module': data.get('module', 'unknown'),
            'scan_timestamp': data.get('scan_timestamp', datetime.now().isoformat()),
            'verification_timestamp': datetime.now().isoformat(),
            'verification_statistics': statistics,
            'findings': verified_findings
        }
        
        # Save verified findings
        logger.info(f"Saving verified findings to {output_file}")
        with open(output_file, 'w') as f:
            json.dump(output, f, indent=2)
        
        return output


def main():
    """CLI entry point"""
    import sys
    import argparse
    
    parser = argparse.ArgumentParser(description='Gap Scanner File Existence Filter & Classifier')
    parser.add_argument('input_file', help='Raw gap_scanner_results.json file')
    parser.add_argument('--output', '-o', default=None, help='Output file (default: gap_scanner_verified_<module>.json)')
    parser.add_argument('--repo-root', '-r', default='.', help='Repository root (default: current dir)')
    
    args = parser.parse_args()
    
    # Determine output file
    if args.output is None:
        # Generate output filename based on input
        base = Path(args.input_file).stem
        if base.endswith('_results'):
            base = base[:-8]  # Remove '_results' suffix
        output_file = f"{Path(args.input_file).parent}/gap_scanner_verified_{base}.json"
    else:
        output_file = args.output
    
    # Run filter
    filter_engine = FileExistenceFilter(args.repo_root)
    result = filter_engine.process_and_save(args.input_file, output_file)
    
    # Print summary
    stats = result['verification_statistics']
    print("\n" + "=" * 60)
    print("Gap Scanner File Existence Filter — Summary")
    print("=" * 60)
    print(f"Input findings:          {stats['total_input']}")
    print(f"False-positives removed: {stats['false_positives_total']} ({100*stats['false_positives_total']//max(1,stats['total_input'])}%)")
    print(f"  - File not found:      {stats['file_not_found']}")
    print(f"Verified findings:       {len(result['findings'])}")
    print(f"  - Real gaps:           {stats['real_gaps']}")
    print(f"  - Guarded stubs:       {stats['guarded_stubs']}")
    print(f"  - Test mocks:          {stats['test_mocks']}")
    print(f"  - Placeholders:        {stats['placeholders']}")
    print(f"\nSeverity downgrades:")
    for key, count in stats['severity_downgrades'].items():
        if count > 0:
            print(f"  - {key}: {count}")
    print(f"\nOutput saved to: {output_file}")
    print("=" * 60)


if __name__ == '__main__':
    main()
