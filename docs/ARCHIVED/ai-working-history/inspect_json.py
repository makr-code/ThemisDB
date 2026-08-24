import json

with open('ai_working/gs3_quick_scan.json', 'r') as f:
    data = json.load(f)

print('JSON Structure:')
if isinstance(data, dict):
    print('Root type: dict')
    print('Keys:', list(data.keys())[:20])
elif isinstance(data, list):
    print('Root type: list')
    print(f'Number of items: {len(data)}')
    if data and isinstance(data[0], dict):
        print('First item keys:', list(data[0].keys()))
        print('\nFirst gap sample:')
        print(json.dumps(data[0], indent=2))
