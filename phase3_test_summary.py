#!/usr/bin/env python3
import json
from pathlib import Path

# Quick summary of all 3 modules
with open('ai_working/phase2_batch_results.json') as f:
    batch = json.load(f)

modules = ['index', 'analytics', 'storage']
print('\n[*] Phase 3 Test Suite Summary:\n')
for mod in modules:
    plan = batch[mod]
    gaps = plan['total_gaps']
    hours = plan['effort_estimate']['total_hours']
    tasks = len(plan['task_breakdown'])
    print(f'{mod.upper():15} | Gaps: {gaps:6,} | Effort: {hours:6.0f}h | Tasks: {tasks:2}')
print()
