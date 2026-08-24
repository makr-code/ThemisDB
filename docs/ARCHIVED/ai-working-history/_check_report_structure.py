import json
from pathlib import Path

report_file = Path('ai_working/VALIDATION_ANALYSIS_REPORT.json')
with open(report_file) as f:
    data = json.load(f)

print('Type:', type(data).__name__)
if isinstance(data, dict):
    print('Keys:', list(data.keys())[:10])
elif isinstance(data, list):
    print('List length:', len(data))
    if data:
        print('First item keys:', list(data[0].keys()))
    
    # Show FP samples by category
    fps_by_cat = {}
    for item in data:
        if item.get('classification') == 'FP':
            cat = item.get('category', 'unknown')
            if cat not in fps_by_cat:
                fps_by_cat[cat] = []
            fps_by_cat[cat].append(item)
    
    print('\nFP categories and counts:')
    for cat, items in sorted(fps_by_cat.items(), key=lambda x: -len(x[1])):
        print(f'  {cat}: {len(items)} FP items')
        if items:
            first = items[0]
            print(f'    Sample: {first.get("file")}:{first.get("line")} - {first.get("reasoning")[:60]}...')
