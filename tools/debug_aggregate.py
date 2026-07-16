import json
from pathlib import Path

agg_path = Path("ai_working/gap_scan_v3_aggregate.json")
print(f"File exists: {agg_path.exists()}")

with open(agg_path, 'r') as f:
    data = json.load(f)

all_gaps = []
print(f"Data type: {type(data).__name__}")
print(f"Top-level keys: {list(data.keys())}")

if isinstance(data, dict):
    for module, module_data in list(data.items())[:1]:
        print(f"\nModule '{module}':")
        print(f"  Type: {type(module_data).__name__}")
        print(f"  Is dict: {isinstance(module_data, dict)}")
        if isinstance(module_data, dict):
            print(f"  Keys: {list(module_data.keys())}")
            if 'gaps' in module_data:
                gaps = module_data['gaps']
                print(f"  'gaps' value type: {type(gaps).__name__}")
                print(f"  First gap type: {type(gaps[0]) if gaps else 'empty'}")
