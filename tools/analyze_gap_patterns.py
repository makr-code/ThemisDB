#!/usr/bin/env python3
"""Analyze gap patterns to improve filters"""

import json
from pathlib import Path
from collections import defaultdict

# Load aggregate  
agg_file = Path('ai_working/gap_scan_v3_aggregate.json')
if not agg_file.exists():
    print("[FAIL] Aggregate not found")
    exit(1)

with open(agg_file) as f:
    aggregate = json.load(f)

# Analyze gaps by context size and category
context_analysis = defaultdict(list)
category_analysis = defaultdict(lambda: {'count': 0, 'total_context_len': 0, 'avg_len': 0})

for module, data in aggregate.items():
    for category, gaps in data.get('by_category', {}).items():
        cat_data = category_analysis[category]
        cat_data['count'] += len(gaps)
        
        for gap in gaps:
            context = gap.get('context', '').strip()
            context_len = len(context)
            
            cat_data['total_context_len'] += context_len
            
            # Find short/empty context gaps (likely FP)
            if context_len < 50:
                context_analysis[category].append({
                    'module': module,
                    'context': context[:40],
                    'len': context_len
                })

# Calculate averages
for cat in category_analysis:
    count = category_analysis[cat]['count']
    if count > 0:
        category_analysis[cat]['avg_len'] = int(category_analysis[cat]['total_context_len'] / count)

# Print analysis
print("[Categories with SHORT context (likely FP candidates)]")
print("-" * 80)
for cat in sorted(category_analysis.keys(), key=lambda x: category_analysis[x]['avg_len']):
    data = category_analysis[cat]
    short_count = len(context_analysis[cat])
    print(f"{cat:30} | Avg ctx: {data['avg_len']:4}ch | Short ctx: {short_count:4} / {data['count']:4} ({100*short_count/data['count']:.1f}%)")

print("\n[Top short-context examples (potential FP)]")
print("-" * 80)
all_short = []
for cat, gaps in context_analysis.items():
    for gap in gaps[:2]:
        all_short.append((cat, gap))

for cat, gap in sorted(all_short, key=lambda x: x[1]['len'])[:15]:
    print(f"CAT: {cat:25} LEN: {gap['len']:3} | {repr(gap['context'][:50])}")
