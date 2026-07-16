#!/usr/bin/env python3
"""Add impact classification to existing scan files"""

import json
from pathlib import Path
import sys

# Import impact classifier
sys.path.insert(0, str(Path.cwd()))
from tools.scanners.gs3_impact_classifier import ImpactClassifier

def enrich_scan_with_impact(scan_file):
    """Add impact_level and subsystem to all gaps in a scan file"""
    
    filepath = Path('ai_working') / scan_file
    
    print(f'Processing: {scan_file}')
    
    # Load scan data
    with open(filepath) as f:
        data = json.load(f)
    
    gaps = data.get('gaps', []) if isinstance(data, dict) else data
    original_count = len(gaps)
    
    # Classify each gap
    enriched_count = 0
    for gap in gaps:
        if 'impact_level' not in gap or not gap['impact_level']:
            filepath_str = gap.get('file', '')
            impact_level, subsystem = ImpactClassifier.classify(filepath_str)
            gap['impact_level'] = impact_level
            gap['subsystem'] = subsystem
            enriched_count += 1
    
    # Save updated data
    if isinstance(data, dict) and 'gaps' in data:
        data['gaps'] = gaps
    else:
        data = gaps
    
    with open(filepath, 'w') as f:
        json.dump(data, f)
    
    print(f'  ✅ Enriched {enriched_count}/{original_count} gaps')
    print(f'  Saved to: {filepath.name}')
    print()

# Enrich all 4 main scan files
for scan_file in ['scan_src.json', 'scan_include.json', 'scan_tests.json', 'scan_benchmarks.json']:
    filepath = Path('ai_working') / scan_file
    if filepath.exists():
        enrich_scan_with_impact(scan_file)
    else:
        print(f'⚠️  File not found: {scan_file}\n')

print('✅ All scans enriched with impact classification')
