import json
from pathlib import Path

# Check individual module files
print("=== Checking Individual Module Files ===")
llm_file = Path('ai_working/gap_scan_v3_llm.json')
with open(llm_file, 'r', encoding='utf-8') as f:
    data = json.load(f)

print(f'LLM File - Data Type: {type(data).__name__}')
if isinstance(data, dict):
    print(f'  Total keys: {len(data)}')
    keys = list(data.keys())
    print(f'  First 5 keys: {keys[:5]}')
    print(f'  Last 5 keys: {keys[-5:]}')
    # Count actual gaps
    total_gaps = 0
    for key in data:
        if isinstance(data[key], list):
            total_gaps += len(data[key])
    print(f'  Total gaps in dict: {total_gaps}')
elif isinstance(data, list):
    print(f'  List length: {len(data)}')

# Check aggregate
print("\n=== Checking Aggregate File ===")
agg_file = Path('ai_working/gap_scan_v3_aggregate.json')
with open(agg_file, 'r', encoding='utf-8') as f:
    agg = json.load(f)

print(f'Aggregate File - Data Type: {type(agg).__name__}')
if isinstance(agg, dict):
    print(f'  Total keys: {len(agg)}')
    keys = list(agg.keys())
    print(f'  Keys: {keys}')
    
    # Count gaps in aggregate
    total_agg_gaps = 0
    for module in agg:
        if isinstance(agg[module], list):
            total_agg_gaps += len(agg[module])
    print(f'  Total gaps in aggregate: {total_agg_gaps}')
    
    # Check if 'llm' key has data
    if 'llm' in agg:
        print(f'  LLM gaps in aggregate: {len(agg["llm"]) if isinstance(agg["llm"], list) else "not a list"}')

print("\n=== Summary from gap_scan_v3_summary.json ===")
with open('ai_working/gap_scan_v3_summary.json', 'r') as f:
    summary = json.load(f)
print(f'Reported total gaps: {summary["total_gaps"]}')
