#!/usr/bin/env python3
import json
from pathlib import Path

ai_working = Path('ai_working')

for scan_file in ['scan_src.json', 'scan_graph_impact_fixed.json']:
    filepath = ai_working / scan_file
    if filepath.exists():
        with open(filepath) as f:
            data = json.load(f)
        gaps = data.get('gaps', []) if isinstance(data, dict) else data
        
        print(f'File: {scan_file}')
        if gaps:
            sample = gaps[0]
            print(f'  Sample gap keys: {list(sample.keys())}')
            print(f'  Has impact_level: {"impact_level" in sample}')
            print(f'  Has subsystem: {"subsystem" in sample}')
            print(f'  Total gaps: {len(gaps)}')
            
            # Check if any gaps have impact_level
            with_impact = sum(1 for g in gaps if 'impact_level' in g and g['impact_level'])
            print(f'  Gaps with impact_level: {with_impact}')
        print()
