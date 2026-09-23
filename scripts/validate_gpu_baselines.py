#!/usr/bin/env python3
"""
Phase 3 Baseline Validation Script

Validates baseline evidence structure and completeness before sign-off.
Checks latency (p50/p95/p99), throughput, CUDA reduction, test execution results.
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from datetime import datetime

class BaselineValidator:
    """Validates GPU baseline evidence against Wave A requirements."""
    
    # Wave A acceptance criteria for baselines
    REQUIRED_SECTIONS = [
        "metadata",
        "cuda_reduction_metrics",
        "latency_baselines",
        "throughput_baselines",
        "cpu_fallback_validation",
        "kernel_sla_validation",
        "resource_exhaustion_validation",
        "multi_gpu_scaling",
        "breakeven_analysis",
        "test_execution_summary",
        "acceptance_checklist",
        "sign_off"
    ]
    
    LATENCY_PERCENTILES = ["p50", "p95", "p99"]
    FALLBACK_SCENARIOS = ["error_injection", "oom", "timeout", "device_lost"]
    TIMEOUT_SCENARIOS = ["5s_timeout", "busy_wait_timeout", "cancellation"]
    EXHAUSTION_SCENARIOS = ["oom_handling", "memory_pool_exhaustion", "stream_limit"]
    
    def __init__(self, baseline_file: Path):
        self.baseline_file = baseline_file
        self.baseline_data = None
        self.errors = []
        self.warnings = []
        
    def load_baseline(self) -> bool:
        """Load and parse baseline JSON."""
        try:
            with open(self.baseline_file, "r") as f:
                self.baseline_data = json.load(f)
            return True
        except FileNotFoundError:
            self.errors.append(f"Baseline file not found: {self.baseline_file}")
            return False
        except json.JSONDecodeError as e:
            self.errors.append(f"Invalid JSON: {e}")
            return False
    
    def validate_structure(self) -> bool:
        """Validate baseline JSON structure."""
        if not self.baseline_data:
            self.errors.append("Baseline data not loaded")
            return False
        
        # Check required sections
        missing = [s for s in self.REQUIRED_SECTIONS if s not in self.baseline_data]
        if missing:
            self.errors.append(f"Missing required sections: {missing}")
            return False
        
        return True
    
    def validate_metadata(self) -> bool:
        """Validate metadata section."""
        meta = self.baseline_data.get("metadata", {})
        required_meta = ["timestamp", "gpu_model", "cuda_version", "test_platform"]
        
        missing = [k for k in required_meta if not meta.get(k)]
        if missing:
            self.warnings.append(f"Metadata missing fields: {missing}")
            return False
        
        return True
    
    def validate_cuda_reduction(self) -> bool:
        """Validate CUDA reduction metrics."""
        reduction = self.baseline_data.get("cuda_reduction_metrics", {})
        
        if "reduction_percentage" not in reduction:
            self.errors.append("Missing CUDA reduction percentage")
            return False
        
        reduction_pct = reduction.get("reduction_percentage", 0)
        if reduction_pct < 40.0:
            self.errors.append(f"CUDA reduction {reduction_pct}% < 40% target")
            return False
        
        if reduction_pct >= 40.0:
            self.warnings.append(f"✓ CUDA reduction {reduction_pct}% >= 40% target")
        
        return True
    
    def validate_latency_baselines(self) -> bool:
        """Validate latency baseline measurements."""
        latency = self.baseline_data.get("latency_baselines", {})
        
        for metric_name in ["gpu_path", "cpu_fallback_path"]:
            if metric_name not in latency:
                self.errors.append(f"Missing latency metric: {metric_name}")
                return False
            
            metric = latency[metric_name]
            missing_pcts = [p for p in self.LATENCY_PERCENTILES if p not in metric]
            if missing_pcts:
                self.errors.append(f"Missing latency percentiles in {metric_name}: {missing_pcts}")
                return False
        
        return True
    
    def validate_throughput_baselines(self) -> bool:
        """Validate throughput measurements."""
        throughput = self.baseline_data.get("throughput_baselines", {})
        
        required_keys = ["gpu_path_ops_per_sec", "cpu_fallback_ops_per_sec", "speedup_ratio"]
        missing = [k for k in required_keys if k not in throughput]
        if missing:
            self.errors.append(f"Missing throughput metrics: {missing}")
            return False
        
        speedup = throughput.get("speedup_ratio", 0)
        if speedup < 1.0:
            self.warnings.append(f"GPU speedup {speedup}x < 1.0x (CPU faster)")
        
        return True
    
    def validate_test_results(self) -> bool:
        """Validate Wave A test execution results."""
        tests = self.baseline_data.get("test_execution_summary", {})
        
        test_suites = {
            "gpu_fallback_tests": 12,
            "gpu_timeout_tests": 12,
            "gpu_exhaustion_tests": 12
        }
        
        for suite_name, expected_count in test_suites.items():
            suite = tests.get(suite_name, {})
            passed = suite.get("passed", 0)
            total = suite.get("total", 0)
            
            if total == 0:
                self.warnings.append(f"No test results for {suite_name}")
                continue
            
            if passed < total:
                self.errors.append(f"{suite_name}: {passed}/{total} passed (expected {expected_count})")
                return False
        
        return True
    
    def validate_acceptance_checklist(self) -> bool:
        """Validate acceptance checklist completion."""
        checklist = self.baseline_data.get("acceptance_checklist", {})
        
        required_checks = [
            "cuda_reduction_target_met",
            "latency_baselines_captured",
            "throughput_baselines_captured",
            "fallback_paths_validated",
            "timeout_enforcement_verified",
            "resource_limits_verified",
            "multi_gpu_parity_verified",
            "test_suite_passed"
        ]
        
        unchecked = [c for c in required_checks if not checklist.get(c, False)]
        if unchecked:
            self.errors.append(f"Acceptance checklist not complete: {unchecked}")
            return False
        
        return True
    
    def validate_sign_off(self) -> bool:
        """Validate sign-off section."""
        sign_off = self.baseline_data.get("sign_off", {})
        
        required_signoffs = ["platform_release_approval", "qa_verification", "security_clearance"]
        missing = [s for s in required_signoffs if not sign_off.get(s)]
        
        if missing:
            self.warnings.append(f"Pending sign-offs: {missing}")
            return False
        
        return True
    
    def run_all_validations(self) -> bool:
        """Run all validation checks."""
        checks = [
            ("Structure", self.validate_structure),
            ("Metadata", self.validate_metadata),
            ("CUDA Reduction", self.validate_cuda_reduction),
            ("Latency Baselines", self.validate_latency_baselines),
            ("Throughput Baselines", self.validate_throughput_baselines),
            ("Test Results", self.validate_test_results),
            ("Acceptance Checklist", self.validate_acceptance_checklist),
            ("Sign-Off", self.validate_sign_off),
        ]
        
        results = {}
        for check_name, check_func in checks:
            results[check_name] = check_func()
        
        return all(results.values())
    
    def print_report(self):
        """Print validation report."""
        print("\n" + "="*70)
        print("GPU BASELINE VALIDATION REPORT")
        print("="*70)
        
        if self.errors:
            print("\n❌ ERRORS (must fix):")
            for error in self.errors:
                print(f"  - {error}")
        else:
            print("\n✅ No critical errors")
        
        if self.warnings:
            print("\n⚠️  WARNINGS (check before sign-off):")
            for warning in self.warnings:
                print(f"  - {warning}")
        else:
            print("\n✅ No warnings")
        
        status = "✅ PASS" if not self.errors else "❌ FAIL"
        print(f"\n{status}: Baseline evidence validation")
        print("="*70 + "\n")
        
        return len(self.errors) == 0


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Validate GPU baseline evidence")
    parser.add_argument("--baseline", default="/home/runner/work/ThemisDB/ThemisDB/benchmarks/wave8/GPU_BASELINES_2026_Q4.json",
                       help="Path to baseline JSON file")
    parser.add_argument("--strict", action="store_true", help="Fail on warnings (not just errors)")
    
    args = parser.parse_args()
    baseline_path = Path(args.baseline)
    
    validator = BaselineValidator(baseline_path)
    
    if not validator.load_baseline():
        print("Failed to load baseline file")
        return 1
    
    validator.run_all_validations()
    success = validator.print_report()
    
    if args.strict and validator.warnings:
        print("Strict mode: warnings treated as failures")
        success = False
    
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
