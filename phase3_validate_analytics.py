#!/usr/bin/env python3
import json
from pathlib import Path

plan_file = Path('ai_working/phase2_batch_results.json')
with open(plan_file) as f:
    batch = json.load(f)

plan = batch['analytics']
print('[*] ANALYTICS Module Plan:')
print(f"    Total gaps: {plan['total_gaps']:,}")
print(f"    Effort: {plan['effort_estimate']['total_hours']:.0f} hours")
print(f"    Tasks: {len(plan['task_breakdown'])}")
print(f"    CRITICAL: {plan['gap_distribution']['CRITICAL']}")
print(f"    HIGH: {plan['gap_distribution']['HIGH']}")
print(f"\n[OK] Ready for Phase 3 execution")
