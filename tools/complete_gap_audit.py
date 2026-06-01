#!/usr/bin/env python3
"""
ThemisDB Complete Gap Audit Workflow (v3)

One-command execution of the full pipeline:
1. Scan for gaps (v3)
2. Generate summary reports (v3)
3. Update file headers (canonical writer)
4. Generate module documentation
5. Distribute docs to module directories
6. Compare scanner outputs (optional)

Usage:
    python tools/complete_gap_audit.py              # Full pipeline
    python tools/complete_gap_audit.py --no-dist    # Skip distribution
    python tools/complete_gap_audit.py --scan-only  # Scan only
"""

import subprocess
import sys
from pathlib import Path

class GapAuditWorkflow:
    """Execute complete gap audit workflow"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.steps_completed = []
        self.steps_failed = []
    
    def run_step(self, name: str, command: str, skip: bool = False) -> bool:
        """Execute a workflow step"""
        if skip:
            print(f"[SKIP] {name} (skipped)")
            return True
        
        print(f"\n[...] {name}...", end=' ', flush=True)
        
        result = subprocess.run(
            command,
            shell=True,
            cwd=str(self.repo_root),
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("[OK]")
            self.steps_completed.append(name)
            return True
        else:
            print("[FAIL]")
            print(f"   Error: {result.stderr[:200]}")
            self.steps_failed.append(name)
            return False
    
    def run_complete_workflow(self, skip_distribution: bool = False,
                            skip_clustering: bool = False) -> bool:
        """Run the complete workflow"""
        
        print("=" * 70)
        print("ThemisDB Complete Gap Audit Workflow")
        print("=" * 70)
        
        # Step 1: Scan
        self.run_step(
            "STEP 1: Scan for Implementation Gaps (v3)",
            "python tools/gap_scanner_v3.py"
        )
        
        # Step 2: Generate summary & file headers
        self.run_step(
            "STEP 2: Generate Reports & Update File Headers",
            "python tools/gap_audit_pipeline_v3.py"
        )
        
        # Step 3: Generate module documentation
        self.run_step(
            "STEP 3: Generate Module Gap Documentation",
            "python tools/module_doc_generator.py . ai_working ai_working/module_gaps"
        )
        
        # Step 4: Distribute to module directories
        self.run_step(
            "STEP 4: Distribute Documentation to Module Directories",
            "python tools/module_doc_distributor.py ai_working/module_gaps .",
            skip=skip_distribution
        )
        
        # Step 5: Analyze and compare (optional)
        self.run_step(
            "STEP 5: Compare Scanner Results",
            "python tools/compare_scanners.py ai_working ai_working",
            skip=skip_clustering
        )
        
        # Final report
        print("\n" + "=" * 70)
        print("[OK] Workflow Summary")
        print("=" * 70)
        
        print(f"\n[OK] Completed Steps ({len(self.steps_completed)}):")
        for step in self.steps_completed:
            print(f"   [OK] {step}")
        
        if self.steps_failed:
            print(f"\n[FAIL] Failed Steps ({len(self.steps_failed)}):")
            for step in self.steps_failed:
                print(f"   [FAIL] {step}")
        
        print(f"\n[INFO] Artifacts Generated:")
        print(f"   [OK] Gap scan results: ai_working/gap_scan_v3_*.json")
        print(f"   [OK] File headers: Updated with gap statistics")
        print(f"   [OK] Module docs: ai_working/module_gaps/")
        print(f"   [OK] Developer docs: src/<module>/MODULE_GAPS.md (if distributed)")
        print(f"   [OK] Summary: ai_working/gap_scan_v3_summary.json")
        
        print(f"\n[ACTION] Next Steps:")
        print(f"   1. Review: cat ai_working/gap_scan_v2_summary.json")
        print(f"   2. Check modules: ls -la src/*/MODULE_GAPS.md")
        print(f"   3. GitHub issues: python tools/gap_clusterer.py")
        print(f"   4. Implement fixes and re-run monthly")
        
        print("\n" + "=" * 70)
        
        return len(self.steps_failed) == 0

def main():
    """Main entry point"""
    
    import argparse
    
    parser = argparse.ArgumentParser(
        description='ThemisDB Complete Gap Audit Workflow'
    )
    parser.add_argument('--no-dist', action='store_true',
                       help='Skip distribution to module directories')
    parser.add_argument('--no-compare', action='store_true',
                       help='Skip scanner comparison')
    parser.add_argument('--scan-only', action='store_true',
                       help='Run only the scanning step')
    parser.add_argument('--repo', default='.', help='Repository root')
    
    args = parser.parse_args()
    
    workflow = GapAuditWorkflow(args.repo)
    
    if args.scan_only:
        # Just scan
        success = workflow.run_step(
            "Scan for Implementation Gaps",
            "python tools/gap_scanner_v3.py"
        )
    else:
        # Full workflow
        success = workflow.run_complete_workflow(
            skip_distribution=args.no_dist,
            skip_clustering=args.no_compare
        )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
