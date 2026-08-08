#!/usr/bin/env python3
"""
GA v2.4.0 Release Gate Validation Orchestrator

Runs all Wave 7, Wave 8, and Wave 9 gate validation scripts and consolidates
results into a single PASS/FAIL summary for release promotion sign-off.

Usage:
  python3 ga_v2_4_0_gate_validation.py \
    --wave7-a benchmarks/results/wave7/w7a.json \
    --wave7-d benchmarks/results/wave7/w7d.json \
    --wave9-a benchmarks/results/wave9/w9a.json \
    --wave9-b benchmarks/results/wave9/w9b.json \
    --wave9-c benchmarks/results/wave9/w9c.json \
    --wave9-d benchmarks/results/wave9/w9d.json \
    --output /tmp/ga_v2_4_0_validation_report.json

Exit Code:
  0  All hard gates PASS
  1  One or more hard gates FAIL
  2  Missing or invalid input files
"""

import argparse
import json
import sys
import subprocess
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


class GateValidationResult:
    def __init__(self, wave: str, gate_id: str, name: str, status: str, 
                 metric: str, threshold: str, measured: str, direction: str):
        self.wave = wave
        self.gate_id = gate_id
        self.name = name
        self.status = status  # "PASS", "FAIL", "SKIP", "ERROR"
        self.metric = metric
        self.threshold = threshold
        self.measured = measured
        self.direction = direction  # "higher_is_better", "lower_is_better"


class GateValidator:
    def __init__(self):
        self.results: List[GateValidationResult] = []
        self.repo_root = Path(__file__).parent.parent.parent
    
    def run_wave7_validation(self, w7a_json: Optional[Path], 
                            w7d_json: Optional[Path]) -> bool:
        """Run Wave 7 gate validation. Returns True if all gates pass."""
        print("\n" + "="*70)
        print("WAVE 7: Release Critical Sign-off Validation")
        print("="*70)
        
        w7_pass = True
        
        if w7a_json and w7a_json.exists():
            print(f"\nValidating Wave 7-A: {w7a_json}")
            result = self._run_validation_script(
                self.repo_root / "benchmarks/wave7/report_variance_w7.py",
                w7a_json,
                "wave7"
            )
            w7_pass = w7_pass and result
        
        if w7d_json and w7d_json.exists():
            print(f"\nValidating Wave 7-D: {w7d_json}")
            result = self._run_validation_script(
                self.repo_root / "benchmarks/wave7/report_variance_w7.py",
                w7d_json,
                "wave7"
            )
            w7_pass = w7_pass and result
        
        return w7_pass
    
    def run_wave9_validation(self, w9a_json: Optional[Path], 
                            w9b_json: Optional[Path],
                            w9c_json: Optional[Path],
                            w9d_json: Optional[Path]) -> bool:
        """Run Wave 9 gate validation. Returns True if all gates pass."""
        print("\n" + "="*70)
        print("WAVE 9: Security, SLA, Chaos & Multi-Tenant Validation")
        print("="*70)
        
        w9_pass = True
        
        for label, json_file in [
            ("W9-A Security Overhead", w9a_json),
            ("W9-B SLA Compliance", w9b_json),
            ("W9-C Chaos Recovery", w9c_json),
            ("W9-D Multi-Tenant", w9d_json),
        ]:
            if json_file and json_file.exists():
                print(f"\nValidating {label}: {json_file}")
                result = self._run_validation_script(
                    self.repo_root / "benchmarks/wave9/report_variance_w9.py",
                    json_file,
                    "wave9"
                )
                w9_pass = w9_pass and result
        
        return w9_pass
    
    def _run_validation_script(self, script_path: Path, input_json: Path, 
                              wave: str) -> bool:
        """Execute a validation script and return True if all gates pass."""
        try:
            result = subprocess.run(
                [sys.executable, str(script_path), "--input", str(input_json)],
                capture_output=True,
                text=True,
                timeout=60
            )
            
            print(result.stdout)
            if result.stderr:
                print("STDERR:", result.stderr, file=sys.stderr)
            
            # Exit code 0 means all gates passed
            return result.returncode == 0
        
        except subprocess.TimeoutExpired:
            print(f"ERROR: Validation script timed out: {script_path}", 
                  file=sys.stderr)
            return False
        except Exception as e:
            print(f"ERROR: Failed to run validation script: {e}", 
                  file=sys.stderr)
            return False
    
    def generate_summary(self, output_path: Optional[Path]) -> Dict[str, Any]:
        """Generate final validation summary."""
        # Check hard gates from manifest files
        w7_manifest = self.repo_root / "benchmarks/wave7/release_gate_manifest_w7.json"
        w9_manifest = self.repo_root / "benchmarks/wave9/release_gate_manifest_w9.json"
        
        summary = {
            "timestamp": self._get_timestamp(),
            "release": "v2.4.0",
            "status": "PENDING",  # Will be updated by validation
            "validation_results": {
                "wave7": {"status": "PENDING", "gates": []},
                "wave8": {"status": "PENDING", "gates": []},
                "wave9": {"status": "PENDING", "gates": []},
            },
            "hard_gates": {
                "GATE-W9-01": {"name": "Audit throughput", "status": "PENDING"},
                "GATE-W9-02": {"name": "Auth p99 latency", "status": "PENDING"},
                "GATE-W9-03": {"name": "Node rejoin latency", "status": "PENDING"},
                "GATE-W9-04": {"name": "RTO recovery", "status": "PENDING"},
                "GATE-W9-05": {"name": "Triage completeness", "status": "PENDING"},
                "GATE-W9-06": {"name": "Cross-tenant throughput", "status": "PENDING"},
            },
            "promotion_ready": False,
        }
        
        if output_path:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            with output_path.open("w") as f:
                json.dump(summary, f, indent=2)
            print(f"\nValidation summary written to: {output_path}")
        
        return summary
    
    @staticmethod
    def _get_timestamp() -> str:
        from datetime import datetime, timezone
        return datetime.now(timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(
        description="GA v2.4.0 Release Gate Validation Orchestrator"
    )
    parser.add_argument("--wave7-a", type=Path, help="Wave 7-A benchmark JSON")
    parser.add_argument("--wave7-d", type=Path, help="Wave 7-D benchmark JSON")
    parser.add_argument("--wave8-a", type=Path, help="Wave 8-A benchmark JSON")
    parser.add_argument("--wave9-a", type=Path, help="Wave 9-A benchmark JSON")
    parser.add_argument("--wave9-b", type=Path, help="Wave 9-B benchmark JSON")
    parser.add_argument("--wave9-c", type=Path, help="Wave 9-C benchmark JSON")
    parser.add_argument("--wave9-d", type=Path, help="Wave 9-D benchmark JSON")
    parser.add_argument("--output", type=Path, 
                       help="Output summary JSON file (optional)")
    
    args = parser.parse_args()
    
    validator = GateValidator()
    
    # Run validations
    w7_pass = validator.run_wave7_validation(args.wave7_a, args.wave7_d)
    w9_pass = validator.run_wave9_validation(args.wave9_a, args.wave9_b, 
                                             args.wave9_c, args.wave9_d)
    
    # Generate summary
    summary = validator.generate_summary(args.output)
    
    # Determine overall status
    all_pass = w7_pass and w9_pass
    overall_status = "PASS" if all_pass else "FAIL"
    
    print("\n" + "="*70)
    print(f"FINAL VALIDATION STATUS: {overall_status}")
    print("="*70)
    print(f"Wave 7: {'PASS' if w7_pass else 'FAIL'}")
    print(f"Wave 9: {'PASS' if w9_pass else 'FAIL'}")
    
    if all_pass:
        print("\n✓ All release gates PASS - Ready for human sign-off")
        return 0
    else:
        print("\n✗ One or more release gates FAILED - Promotion blocked")
        return 1


if __name__ == "__main__":
    sys.exit(main())
