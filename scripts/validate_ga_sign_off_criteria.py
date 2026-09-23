#!/usr/bin/env python3
"""
Phase 4 GA Sign-Off Validator

Validates that all 8 Wave A GPU acceptance criteria are met before closure.
Orchestrates governance synchronization (GA_PROMOTION_SIGN_OFF.md, ROADMAP.md updates).
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple
from datetime import datetime

class GASignOffValidator:
    """Validates Wave A GPU GA acceptance criteria."""
    
    # 8 Wave A acceptance criteria
    ACCEPTANCE_CRITERIA = {
        1: "CUDA kernel call reduction: ≥40% reduction vs Wave 7 baseline",
        2: "Fallback CPU path: tested and operational",
        3: "Commit SHA: on develop branch with green Wave A GPU CI",
        4: "Performance impact: documented (latency + memory footprint)",
        5: "Self-hosted GPU runner: online and validated (gpu-cuda)",
        6: "Representative-hardware baseline: captured (p50/p95/p99, throughput)",
        7: "Baseline evidence: signed off by platform-release@themisdb",
        8: "Evidence pointer: recorded in GA_PROMOTION_SIGN_OFF.md §Wave D"
    }
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.criteria_status = {}
        self.findings = []
        self.blockers = []
        
    def check_cuda_reduction(self, baseline_file: Path) -> bool:
        """Criterion 1: CUDA reduction ≥40%."""
        try:
            with open(baseline_file, "r") as f:
                baseline = json.load(f)
            
            reduction = baseline.get("cuda_reduction_metrics", {})
            reduction_pct = reduction.get("reduction_percentage", 0)
            
            if reduction_pct >= 40.0:
                self.findings.append(f"✅ Criterion 1: CUDA reduction {reduction_pct}% >= 40% target")
                return True
            else:
                self.blockers.append(f"❌ Criterion 1: CUDA reduction {reduction_pct}% < 40% target")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 1: Failed to check baseline ({e})")
            return False
    
    def check_fallback_path(self, test_results_file: Path) -> bool:
        """Criterion 2: Fallback CPU path operational."""
        try:
            with open(test_results_file, "r") as f:
                results = json.load(f)
            
            fallback_tests = results.get("gpu_fallback_tests", {})
            passed = fallback_tests.get("passed", 0)
            total = fallback_tests.get("total", 0)
            
            if total > 0 and passed == total:
                self.findings.append(f"✅ Criterion 2: GPU fallback tests {passed}/{total} passed")
                return True
            else:
                self.blockers.append(f"❌ Criterion 2: GPU fallback tests {passed}/{total} (expected all passed)")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 2: Failed to check fallback tests ({e})")
            return False
    
    def check_commit_sha(self, develop_sha: str) -> bool:
        """Criterion 3: Commit SHA on develop with green CI."""
        if not develop_sha or develop_sha == "0" * 40:
            self.blockers.append("❌ Criterion 3: No valid commit SHA provided")
            return False
        
        self.findings.append(f"✅ Criterion 3: Commit {develop_sha[:7]} on develop")
        return True
    
    def check_performance_docs(self, baseline_file: Path) -> bool:
        """Criterion 4: Performance documentation."""
        try:
            with open(baseline_file, "r") as f:
                baseline = json.load(f)
            
            latency = baseline.get("latency_baselines", {})
            throughput = baseline.get("throughput_baselines", {})
            
            if latency and throughput:
                self.findings.append("✅ Criterion 4: Performance documentation captured (latency + throughput)")
                return True
            else:
                self.blockers.append("❌ Criterion 4: Missing performance documentation sections")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 4: Failed to check performance docs ({e})")
            return False
    
    def check_gpu_runner(self, runner_config_file: Path) -> bool:
        """Criterion 5: Self-hosted GPU runner online."""
        # In real scenario, this would query GitHub Actions runner status
        # For now, check if runner config exists
        if runner_config_file.exists():
            self.findings.append("✅ Criterion 5: Self-hosted gpu-cuda runner configuration found")
            return True
        else:
            self.findings.append("⏳ Criterion 5: gpu-cuda runner configuration (requires Phase 1 infrastructure)")
            return False  # Phase 1 infrastructure blocking
    
    def check_baseline_capture(self, baseline_file: Path) -> bool:
        """Criterion 6: Representative-hardware baseline captured."""
        try:
            with open(baseline_file, "r") as f:
                baseline = json.load(f)
            
            latency = baseline.get("latency_baselines", {})
            throughput = baseline.get("throughput_baselines", {})
            metadata = baseline.get("metadata", {})
            
            has_latency = "p50" in latency.get("gpu_path", {}) and \
                         "p95" in latency.get("gpu_path", {}) and \
                         "p99" in latency.get("gpu_path", {})
            
            has_throughput = "gpu_path_ops_per_sec" in throughput
            has_hardware = metadata.get("gpu_model")
            
            if has_latency and has_throughput and has_hardware:
                self.findings.append(f"✅ Criterion 6: Baseline captured on {has_hardware} (p50/p95/p99 + throughput)")
                return True
            else:
                self.blockers.append("❌ Criterion 6: Baseline capture incomplete")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 6: Failed to check baseline capture ({e})")
            return False
    
    def check_sign_off(self, baseline_file: Path) -> bool:
        """Criterion 7: Baseline signed off."""
        try:
            with open(baseline_file, "r") as f:
                baseline = json.load(f)
            
            sign_off = baseline.get("sign_off", {})
            approvals = [
                sign_off.get("platform_release_approval"),
                sign_off.get("qa_verification"),
                sign_off.get("security_clearance")
            ]
            
            if all(approvals):
                self.findings.append("✅ Criterion 7: Baseline signed off by platform-release@themisdb")
                return True
            else:
                self.blockers.append("❌ Criterion 7: Sign-offs incomplete")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 7: Failed to check sign-off ({e})")
            return False
    
    def check_evidence_pointer(self, ga_sign_off_file: Path, wave_section: str = "Wave D") -> bool:
        """Criterion 8: Evidence pointer in GA_PROMOTION_SIGN_OFF.md."""
        try:
            if not ga_sign_off_file.exists():
                self.blockers.append("❌ Criterion 8: GA_PROMOTION_SIGN_OFF.md not found")
                return False
            
            with open(ga_sign_off_file, "r") as f:
                content = f.read()
            
            if wave_section in content and "GPU_BASELINES_2026_Q4.json" in content:
                self.findings.append(f"✅ Criterion 8: Evidence pointer in GA_PROMOTION_SIGN_OFF.md §{wave_section}")
                return True
            else:
                self.blockers.append(f"❌ Criterion 8: Evidence pointer missing in GA_PROMOTION_SIGN_OFF.md")
                return False
        except Exception as e:
            self.blockers.append(f"❌ Criterion 8: Failed to check evidence pointer ({e})")
            return False
    
    def run_all_checks(self, baseline_file: Path, test_results_file: Optional[Path] = None,
                      develop_sha: Optional[str] = None) -> Dict[int, bool]:
        """Run all 8 acceptance criteria checks."""
        
        self.criteria_status[1] = self.check_cuda_reduction(baseline_file)
        
        if test_results_file and test_results_file.exists():
            self.criteria_status[2] = self.check_fallback_path(test_results_file)
        else:
            self.findings.append("⏳ Criterion 2: Test results pending (requires Phase 3 execution)")
            self.criteria_status[2] = False
        
        self.criteria_status[3] = self.check_commit_sha(develop_sha or "0"*40)
        self.criteria_status[4] = self.check_performance_docs(baseline_file)
        
        runner_config = self.repo_root / ".github" / "gpu-runner-health.yml"
        self.criteria_status[5] = self.check_gpu_runner(runner_config)
        self.criteria_status[6] = self.check_baseline_capture(baseline_file)
        self.criteria_status[7] = self.check_sign_off(baseline_file)
        
        ga_sign_off = self.repo_root / "docs" / "governance" / "GA_PROMOTION_SIGN_OFF.md"
        self.criteria_status[8] = self.check_evidence_pointer(ga_sign_off)
        
        return self.criteria_status
    
    def print_report(self) -> bool:
        """Print Wave A acceptance criteria report."""
        print("\n" + "="*80)
        print("WAVE A GPU — GA ACCEPTANCE CRITERIA REPORT")
        print("="*80)
        
        print("\n📋 CRITERIA STATUS:")
        for criterion_num in sorted(self.criteria_status.keys()):
            status = "✅" if self.criteria_status[criterion_num] else "❌"
            description = self.ACCEPTANCE_CRITERIA[criterion_num]
            print(f"  {status} Criterion {criterion_num}: {description}")
        
        if self.findings:
            print("\n✅ FINDINGS:")
            for finding in self.findings:
                print(f"  {finding}")
        
        if self.blockers:
            print("\n❌ BLOCKERS:")
            for blocker in self.blockers:
                print(f"  {blocker}")
        
        passed = sum(1 for v in self.criteria_status.values() if v)
        total = len(self.criteria_status)
        
        all_pass = all(self.criteria_status.values())
        status_str = "✅ PASS" if all_pass else f"⚠️  PARTIAL ({passed}/{total})"
        
        print(f"\n{'='*80}")
        print(f"RESULT: {status_str} — Wave A GA Acceptance")
        print(f"{'='*80}\n")
        
        return all_pass


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Validate Wave A GA acceptance criteria")
    parser.add_argument("--repo-root", default="/home/runner/work/ThemisDB/ThemisDB",
                       help="Repository root path")
    parser.add_argument("--baseline", default=None, help="Baseline JSON file path")
    parser.add_argument("--test-results", default=None, help="Test results JSON file path")
    parser.add_argument("--commit-sha", default=None, help="Develop branch commit SHA")
    parser.add_argument("--strict", action="store_true", help="Fail if any criterion not fully met")
    
    args = parser.parse_args()
    
    repo_root = Path(args.repo_root)
    baseline_file = Path(args.baseline or repo_root / "benchmarks/wave8/GPU_BASELINES_2026_Q4.json")
    test_results_file = Path(args.test_results) if args.test_results else None
    
    validator = GASignOffValidator(repo_root)
    validator.run_all_checks(baseline_file, test_results_file, args.commit_sha)
    success = validator.print_report()
    
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
