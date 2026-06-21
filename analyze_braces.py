import json
from collections import Counter

with open('ai_working/gap_scan_braces_test.json', 'r') as f:
    data = json.load(f)

print(f'Total findings: {len(data)}')
print(f'Unique files: {len(set(g.get("file") for g in data))}')
types = Counter(g.get('type') for g in data)
print(f'Finding types: {dict(types)}')

# Check findings per file
files = Counter(g.get('file') for g in data)
print(f'\nFindings per file (top 20):')
for file, count in files.most_common(20):
    print(f'  {file}: {count}')

# Sample a few findings
print(f'\nSample findings (first 3):')
for i, finding in enumerate(data[:3], 1):
    print(f'\n--- Finding {i} ---')
    print(f'File: {finding.get("file")}')
    print(f'Type: {finding.get("type")}')
    print(f'Line: {finding.get("line")}')
    print(f'Description: {finding.get("description", "")[:100]}...')
