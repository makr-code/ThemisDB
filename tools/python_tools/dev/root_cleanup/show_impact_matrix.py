#!/usr/bin/env python3
"""Show impact classification matrix for AI-vibe findings"""

import json
from collections import Counter

# Load the new scan with impact info
data = json.load(open('ai_working/scan_graph_impact_fixed.json'))
gaps = data.get('gaps', [])

# Extract AI-vibe findings (only TODO and simulation/stub types from phase 1 scanners)
ai_vibe_types = ['todo_', 'simulation_', 'stub_', 'hardcoded_llm_prompt', 'llm_prompt_injection', 'missing_input_delimiter', 'unchecked_result', 'missing_doxygen']
ai_vibe = [g for g in gaps if any(x in g.get('type', '') for x in ai_vibe_types)]

print("AI-VIBE FINDINGS BY SEVERITY × IMPACT")
print()

# Create 2D matrix: Severity × Impact
severity_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']
impact_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']

matrix = {}
for sev in severity_levels:
    matrix[sev] = {}
    for imp in impact_levels:
        count = len([g for g in ai_vibe if g.get('severity') == sev and g.get('impact_level') == imp])
        matrix[sev][imp] = count

# Print matrix
print('               IMPACT LEVEL')
print('              ' + '   '.join(f'{x:>8s}' for x in impact_levels))
for sev in severity_levels:
    row = ' '.join(f'{matrix[sev][imp]:>8d}' for imp in impact_levels)
    print(f'{sev:>10s}  {row}')

print()
print(f'Total AI-Vibe: {len(ai_vibe)}')
print()

# Show critical path (worst combo)
critical_path = [g for g in ai_vibe if g.get('severity') == 'CRITICAL' and g.get('impact_level') == 'CRITICAL']
print(f'🔴 CRITICAL SEVERITY + CRITICAL IMPACT: {len(critical_path)} findings')
for g in critical_path[:3]:
    fname = g['file'].split('/')[-1]
    gtype = g['type']
    subsys = g.get('subsystem', '?')
    print(f'   - {gtype}: {fname}:{g["line"]} [{subsys}]')

print()
print('Subsystem Breakdown:')
subsys_count = Counter(g.get('subsystem', '?') for g in ai_vibe if g.get('subsystem'))
if subsys_count:
    for subsys, count in sorted(subsys_count.items(), key=lambda x: -x[1]):
        print(f'   - {subsys:15s}: {count:3d}')
else:
    print('   (No subsystems found)')

