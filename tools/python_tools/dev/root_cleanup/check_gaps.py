#!/usr/bin/env python3
import json

data = json.load(open('ai_working/scan_graph_impact_fixed.json'))
gaps = data.get('gaps', [])
todos = [g for g in gaps if 'todo' in g.get('type', '')]

print(f'Total gaps: {len(gaps)}')
print(f'TODO gaps: {len(todos)}')

types_count = {}
for g in gaps:
    t = g.get('type', '?')
    types_count[t] = types_count.get(t, 0) + 1

print('\nTop 10 types:')
for t, c in sorted(types_count.items(), key=lambda x: -x[1])[:10]:
    print(f'  {t}: {c}')

if todos:
    print(f'\n✓ First TODO finding:')
    print(json.dumps(todos[0], indent=2)[:200])
else:
    print(f'\n✗ No TODO findings found')
