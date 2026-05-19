#!/usr/bin/env python3
"""Phase 3 Scale: All 65 modules with Ollama + GitHub Integration"""

import json
import subprocess
import time
from pathlib import Path
from typing import Dict, List

class Phase3Scaler:
    """Scale Phase 3 to all 65 modules + GitHub"""
    
    def __init__(self):
        self.repo = "makr-code/ThemisDB"
        self.ollama_endpoint = "http://localhost:11434/api/generate"
        self.model = "codellama:latest"
        self.max_tasks_per_module = 5
        
    def load_all_modules(self) -> List[str]:
        """Load all 65 modules from Phase 2 results"""
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        return sorted(batch.keys())
    
    def get_issue_number_for_module(self, module: str) -> int:
        """Get GitHub issue number for module"""
        modules = self.load_all_modules()
        return 5245 + modules.index(module)
    
    def get_branch_name(self, module: str) -> str:
        """Get git branch name for module"""
        return f"feature/phase3-{module}-ollama"
    
    def report_plan(self):
        """Show scaling plan before execution"""
        modules = self.load_all_modules()
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        
        total_gaps = sum(m['total_gaps'] for m in batch.values())
        total_effort = sum(m['effort_estimate']['total_hours'] for m in batch.values())
        
        print("=" * 80)
        print("PHASE 3 SCALE PLAN: ALL 65 MODULES")
        print("=" * 80)
        print()
        print(f"Total Modules:     {len(modules)}")
        print(f"Total Gaps:        {total_gaps:,}")
        print(f"Total Effort:      {total_effort:,.0f} hours")
        print(f"Estimated Time:    5-8 hours continuous")
        print(f"Model:             {self.model}")
        print(f"Tasks per Module:  {self.max_tasks_per_module}")
        print()
        print("Module List (with Issue Numbers):")
        print("-" * 80)
        
        for idx, module in enumerate(modules, 1):
            issue = self.get_issue_number_for_module(module)
            gaps = batch[module]['total_gaps']
            effort = batch[module]['effort_estimate']['total_hours']
            branch = self.get_branch_name(module)
            print(f"  {idx:2d}. #{issue:4d} | {module:20s} | {gaps:6,} gaps | {effort:6.0f}h | {branch}")
        
        print()
        print("GitHub Integration:")
        print("-" * 80)
        print("  - Create feature branch for each module")
        print("  - Commit generated code")
        print("  - Create Draft PR via: gh pr create --draft")
        print("  - Link Issue in PR body")
        print("  - Update Issue status: gh issue close/comment")
        print()
        print("Next Command:")
        print("  python phase3_scale_all.py --execute")
        print()

if __name__ == '__main__':
    import sys
    
    scaler = Phase3Scaler()
    
    if '--execute' in sys.argv:
        print("[!] Phase 3 Full Scale execution not yet implemented")
        print("[*] Current status: 3 modules tested (INDEX, ANALYTICS, STORAGE)")
        print("[*] Ready to scale: All 65 modules")
        print()
        print("To proceed with full scale:")
        print("  1. Run: python phase3_codegen.py --batch --all-modules")
        print("  2. Create GitHub branches + PRs")
        print("  3. Close related issues")
    else:
        scaler.report_plan()
