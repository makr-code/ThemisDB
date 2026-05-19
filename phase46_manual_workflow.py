#!/usr/bin/env python3
"""Phase 4-6: Manual Module-by-Module GitHub Integration"""

import json
from pathlib import Path
from typing import Dict

class ModuleWorkflow:
    """Manual workflow for one module: Phase 3 -> GitHub PR -> Issue Close"""
    
    def __init__(self, module: str):
        self.module = module
        self.repo = "makr-code/ThemisDB"
        self.base_branch = "develop"
        
    def get_issue_number(self) -> int:
        """Get GitHub issue number for this module"""
        modules = self._get_all_modules()
        return 5245 + modules.index(self.module)
    
    def _get_all_modules(self) -> list:
        """Get ordered module list"""
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        return sorted(batch.keys())
    
    def load_phase3_results(self) -> Dict:
        """Load Phase 3 code generation results"""
        results_file = Path(f'ai_working/phase3_{self.module}_results.json')
        if not results_file.exists():
            return None
        with open(results_file) as f:
            return json.load(f)
    
    def load_phase2_plan(self) -> Dict:
        """Load Phase 2 planning data"""
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        return batch[self.module]
    
    def get_branch_name(self) -> str:
        """Get feature branch name"""
        return f"feature/phase3-{self.module}-codegen"
    
    def show_workflow(self):
        """Display manual workflow steps"""
        issue_num = self.get_issue_number()
        branch = self.get_branch_name()
        results = self.load_phase3_results()
        plan = self.load_phase2_plan()
        
        print("=" * 80)
        print(f"MODULE WORKFLOW: {self.module.upper()}")
        print("=" * 80)
        print()
        
        print(f"GitHub Issue:       #{issue_num}")
        print(f"Feature Branch:     {branch}")
        print(f"Base Branch:        {self.base_branch}")
        print()
        
        print("PHASE 3 RESULTS:")
        print("-" * 80)
        if results:
            print(f"  Model:            {results['model']}")
            print(f"  Tasks Generated:  {results['tasks_generated']}")
            print(f"  Syntax Valid:     {results['syntax_ok']}/{results['tasks_generated']}")
            print(f"  Execution Time:   {results['execution_time']:.1f}s")
            print(f"  Results File:     ai_working/phase3_{self.module}_results.json")
        else:
            print(f"  [MISSING] No Phase 3 results for {self.module}")
        
        print()
        print("PHASE 2 PLAN:")
        print("-" * 80)
        print(f"  Total Gaps:       {plan['total_gaps']:,}")
        print(f"  Estimated Effort: {plan['effort_estimate']['total_hours']:.0f} hours")
        print(f"  Tasks:            {len(plan['task_breakdown'])}")
        print()
        
        print("MANUAL WORKFLOW STEPS:")
        print("-" * 80)
        print()
        print("Step 1: Create Feature Branch")
        print(f"  $ git checkout -b {branch}")
        print()
        
        print("Step 2: Review Phase 3 Results")
        print(f"  $ cat ai_working/phase3_{self.module}_results.json | jq '.results[0]'")
        print(f"  -> Check code quality, syntax, documentation")
        print()
        
        print("Step 3: Create Code Patch (if manual edits needed)")
        print(f"  $ git add <modified_files>")
        print(f"  $ git commit -m 'Phase 3: Ollama code generation for {self.module}'")
        print()
        
        print("Step 4: Create GitHub PR")
        print(f"  $ gh pr create --draft \\")
        print(f"      --title 'Phase 3: Code Generation - {self.module.upper()}' \\")
        print(f"      --base {self.base_branch} \\")
        print(f"      --body-file pr_body_{self.module}.md")
        print()
        
        print("Step 5: Link Issue in PR")
        print(f"  $ gh pr comment <PR_NUM> --body 'Closes #{issue_num}'")
        print()
        
        print("Step 6: Update Issue Status")
        print(f"  $ gh issue comment #{issue_num} --body 'Phase 3 code generation completed. See PR.'")
        print()
        
        print("QUICK REFERENCE:")
        print("-" * 80)
        print(f"  Phase 3 Results:    ai_working/phase3_{self.module}_results.json")
        print(f"  Issue #:            {issue_num}")
        print(f"  Branch:             {branch}")
        print()

if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python phase46_manual_workflow.py <module>")
        print()
        print("Example: python phase46_manual_workflow.py index")
        sys.exit(1)
    
    module = sys.argv[1]
    workflow = ModuleWorkflow(module=module)
    workflow.show_workflow()
