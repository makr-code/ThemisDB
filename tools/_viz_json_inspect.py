import json
import os
import sys

path = 'ai_working/include_graph_tools_scanners_libclang.json'
if len(sys.argv) > 1:
    path = sys.argv[1]

print('Checking', path)
if not os.path.exists(path):
    print('MISSING')
    sys.exit(2)

with open(path, 'r', encoding='utf-8') as f:
    d = json.load(f)

nodes = d.get('nodes', [])
edges = d.get('edges', [])
chunks = d.get('chunks', {})
gaps = d.get('gaps', {})

print('nodes:', len(nodes))
print('edges:', len(edges))
if isinstance(chunks, dict):
    print('chunks mapping keys:', len(chunks.keys()))
else:
    print('chunks list len:', len(chunks))
if isinstance(gaps, dict):
    print('gaps mapping keys:', len(gaps.keys()))
else:
    print('gaps list len:', len(gaps))

# print sample
print('\nSample nodes (first 5):')
for n in nodes[:5]:
    print('-', n.get('id') or n.get('path') or n.get('label') or n)

print('\nDone')
