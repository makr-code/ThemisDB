#!/usr/bin/env python3
import json
from pathlib import Path

# Test: Load phase2 plan for index
plan_file = Path('ai_working/phase2_batch_results.json')
with open(plan_file) as f:
    batch = json.load(f)

if 'index' not in batch:
    print('[ERROR] Module index not found')
    exit(1)

plan = batch['index']
print('[*] INDEX Module Plan Loaded:')
print(f"    Total gaps: {plan.get('total_gaps', 0):,}")
print(f"    Estimated hours: {plan.get('effort_estimate', {}).get('total_hours', 0):.0f}h")
print(f"    Tasks: {len(plan.get('task_breakdown', []))}")
print(f"\n[OK] Ready for Phase 3 code generation")

# Show first 3 tasks
tasks = plan.get('task_breakdown', [])[:3]
for i, task in enumerate(tasks, 1):
    print(f"\n  Task {i}: {task.get('id')} ({task.get('priority')})")
    print(f"    Effort: {task.get('effort_hours'):.0f}h")
    print(f"    Gap Range: {task.get('gap_range')}")
