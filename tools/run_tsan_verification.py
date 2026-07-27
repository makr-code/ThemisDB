#!/usr/bin/env python3
"""
Sprint 9: ThreadSanitizer Verification Runner

Automated script to:
1. Build with ThreadSanitizer enabled
2. Run concurrency test suite
3. Parse and report TSAN findings
4. Generate verification report

Usage:
    python3 tools/run_tsan_verification.py [--preset <preset>] [--verbose]

Environment:
    TSAN_OPTIONS="halt_on_error=1:exitcode=42"  # Strict mode
"""

import subprocess
import sys
import os
import re
import json
from pathlib import Path
from typing import List, Dict, Tuple
from dataclasses import dataclass
from datetime import datetime


@dataclass
class TSanFinding:
    """Represents a ThreadSanitizer finding"""
    type: str  # "data_race", "deadlock", "lock_ordering", etc.
    location: str  # File:line
    thread_id: int
    severity: str  # "critical", "high", "medium", "low"
    description: str
    suggested_fix: str = ""


class ThreadSanitizerRunner:
    """Runs tests with ThreadSanitizer and collects findings"""
    
    def __init__(self, repo_root: Path = None):
        self.repo_root = repo_root or Path.cwd()
        self.build_dir = self.repo_root / "build-tsan"
        self.findings: List[TSanFinding] = []
        self.test_results: Dict = {
            "total_tests": 0,
            "passed": 0,
            "failed": 0,
            "tsan_findings": 0
        }
    
    def setup_tsan_environment(self):
        """Configure environment for ThreadSanitizer"""
        os.environ["TSAN_OPTIONS"] = (
            "halt_on_error=1:exitcode=42:verbosity=2:"
            "history_size=7:report_signal_unsafe=0"
        )
        os.environ["LSAN_OPTIONS"] = "verbosity=1:log_threads=1"
        print("[TSAN] Environment configured:")
        print(f"  TSAN_OPTIONS={os.environ.get('TSAN_OPTIONS')}")
    
    def configure_cmake(self):
        """Configure CMake with ThreadSanitizer preset"""
        print("\n[CMAKE] Configuring with ThreadSanitizer...")
        
        # Create build directory
        self.build_dir.mkdir(parents=True, exist_ok=True)
        
        cmd = [
            "cmake",
            "--preset", "linux-tsan-debug",
            "-DENABLE_THREAD_SANITIZER=ON",
            "-DCMAKE_BUILD_TYPE=Debug",
            f"-B{self.build_dir}",
            f"-S{self.repo_root}"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"[ERROR] CMake configuration failed:")
            print(result.stderr)
            return False
        
        print("[CMAKE] Configuration successful")
        return True
    
    def build(self, parallel: int = 16):
        """Build with ThreadSanitizer"""
        print(f"\n[BUILD] Building with ThreadSanitizer ({parallel} parallel jobs)...")
        
        cmd = [
            "cmake",
            "--build", str(self.build_dir),
            f"--parallel", str(parallel),
            "--"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"[ERROR] Build failed:")
            print(result.stderr)
            return False
        
        print("[BUILD] Build successful")
        return True
    
    def run_concurrency_tests(self) -> Tuple[bool, str]:
        """Run concurrency test suite with TSAN"""
        print("\n[TEST] Running concurrency test suite...")
        
        cmd = [
            "ctest",
            f"--test-dir", str(self.build_dir),
            "-L", "concurrency",
            "--output-on-failure",
            "--verbose",
            "-j", "4"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        self.test_results["total_tests"] = len(re.findall(r"Running test \d+", result.stdout))
        self.test_results["passed"] = result.stdout.count("Passed")
        self.test_results["failed"] = result.stdout.count("Failed")
        
        # Parse TSAN findings from output
        self._parse_tsan_output(result.stderr + result.stdout)
        
        success = result.returncode == 0 or result.returncode == 42  # 42 = TSAN halt
        return success, result.stdout + result.stderr
    
    def _parse_tsan_output(self, output: str):
        """Parse ThreadSanitizer output for findings"""
        print("\n[PARSE] Analyzing ThreadSanitizer output...")
        
        # Data race pattern
        race_pattern = re.compile(
            r"WARNING: ThreadSanitizer: data race\n.*?Previous (read|write).*?at.*?(?=WARNING:|$)",
            re.DOTALL
        )
        
        deadlock_pattern = re.compile(
            r"WARNING: ThreadSanitizer: lock-order-inversion\n.*?(?=WARNING:|$)",
            re.DOTALL
        )
        
        # Find data races
        for match in race_pattern.finditer(output):
            self.findings.append(TSanFinding(
                type="data_race",
                location=self._extract_location(match.group()),
                thread_id=0,  # Extract from output if needed
                severity="high",
                description=match.group()[:200]
            ))
        
        # Find deadlock/lock-order issues
        for match in deadlock_pattern.finditer(output):
            self.findings.append(TSanFinding(
                type="lock_ordering",
                location=self._extract_location(match.group()),
                thread_id=0,
                severity="critical",
                description=match.group()[:200]
            ))
        
        self.test_results["tsan_findings"] = len(self.findings)
        print(f"[PARSE] Found {len(self.findings)} ThreadSanitizer findings")
    
    def _extract_location(self, text: str) -> str:
        """Extract file:line from TSAN output"""
        match = re.search(r"([\w./]+):(\d+)", text)
        if match:
            return f"{match.group(1)}:{match.group(2)}"
        return "<unknown>"
    
    def generate_report(self) -> str:
        """Generate verification report"""
        timestamp = datetime.now().isoformat()
        
        report = f"""
# Sprint 9: ThreadSanitizer Verification Report

**Date:** {timestamp}  
**Build Directory:** {self.build_dir}

## Summary

| Metric | Value |
|--------|-------|
| Total Tests | {self.test_results['total_tests']} |
| Passed | {self.test_results['passed']} |
| Failed | {self.test_results['failed']} |
| TSAN Findings | {self.test_results['tsan_findings']} |

## ThreadSanitizer Findings

"""
        
        if not self.findings:
            report += "✅ **NO FINDINGS** - Concurrency tests clean\n"
        else:
            report += "⚠️ **FINDINGS DETECTED:**\n\n"
            
            by_type = {}
            for finding in self.findings:
                if finding.type not in by_type:
                    by_type[finding.type] = []
                by_type[finding.type].append(finding)
            
            for finding_type, findings_list in by_type.items():
                report += f"### {finding_type.upper()} ({len(findings_list)} findings)\n\n"
                
                for i, finding in enumerate(findings_list, 1):
                    report += f"**Finding {i}:** {finding.location}\n"
                    report += f"- Type: {finding.type}\n"
                    report += f"- Severity: {finding.severity}\n"
                    report += f"- Description: {finding.description}\n"
                    if finding.suggested_fix:
                        report += f"- Fix: {finding.suggested_fix}\n"
                    report += "\n"
        
        return report
    
    def run_all(self, verbose: bool = False) -> int:
        """Execute full TSAN verification pipeline"""
        print("\n" + "="*70)
        print("Sprint 9: ThreadSanitizer Verification Pipeline")
        print("="*70)
        
        self.setup_tsan_environment()
        
        if not self.configure_cmake():
            print("[FATAL] CMake configuration failed")
            return 1
        
        if not self.build():
            print("[FATAL] Build failed")
            return 1
        
        success, output = self.run_concurrency_tests()
        
        if verbose:
            print("\n[OUTPUT] CTest output:")
            print(output)
        
        report = self.generate_report()
        print("\n" + report)
        
        # Save report
        report_path = self.repo_root / "ai_working" / f"SPRINT_9_TSAN_REPORT_{datetime.now().strftime('%Y%m%d_%H%M%S')}.md"
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(report)
        print(f"\n[REPORT] Saved to {report_path}")
        
        # Return exit code based on findings
        if self.test_results["tsan_findings"] == 0:
            print("\n✅ ThreadSanitizer verification PASSED (no findings)")
            return 0
        else:
            print(f"\n❌ ThreadSanitizer verification FAILED ({self.test_results['tsan_findings']} findings)")
            return 1


def main():
    """Entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Sprint 9: ThreadSanitizer Verification Runner"
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path.cwd(),
        help="Repository root directory"
    )
    parser.add_argument(
        "--preset",
        type=str,
        default="linux-tsan-debug",
        help="CMake preset name"
    )
    parser.add_argument(
        "--parallel",
        type=int,
        default=16,
        help="Parallel build jobs"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Verbose output"
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip build, run tests only"
    )
    
    args = parser.parse_args()
    
    runner = ThreadSanitizerRunner(args.repo_root)
    
    if args.skip_build:
        print("[SKIP] Skipping build phase")
    else:
        runner.setup_tsan_environment()
        if not runner.configure_cmake():
            return 1
        if not runner.build(args.parallel):
            return 1
    
    success, output = runner.run_concurrency_tests()
    
    if args.verbose:
        print("\n[OUTPUT]")
        print(output)
    
    report = runner.generate_report()
    print("\n" + report)
    
    # Save report
    report_path = (
        runner.repo_root / "ai_working" / 
        f"SPRINT_9_TSAN_REPORT_{datetime.now().strftime('%Y%m%d_%H%M%S')}.md"
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(report)
    
    return 0 if runner.test_results["tsan_findings"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
