#!/usr/bin/env python3
"""
GS3 CLI Integration Test Suite

Tests all major CLI subcommands:
- list-scanners
- scan
- report
- config
"""

import json
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple


class Colors:
    """ANSI color codes"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'


class GS3IntegrationTest:
    """Test runner for GS3 CLI"""
    
    def __init__(self):
        self.test_results: List[Tuple[str, bool, str]] = []
        self.repo_root = Path(__file__).parent.parent
        self.tools_dir = self.repo_root / "tools"
        self.ai_working = self.repo_root / "ai_working"
        self.ai_working.mkdir(exist_ok=True)
        
    def run_command(self, cmd: List[str], timeout: int = 300) -> Tuple[int, str, str]:
        """Run a shell command and return exit code, stdout, stderr"""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=str(self.repo_root)
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", f"Command timed out after {timeout}s"
        except Exception as e:
            return -1, "", str(e)
    
    def test_help(self) -> bool:
        """Test: python tools/gs3.py --help"""
        print(f"\n{Colors.BLUE}[TEST] Main help{Colors.RESET}")
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "--help"
        ])
        
        success = code == 0 and "scan" in stdout and "report" in stdout
        msg = "✓ Help output contains all subcommands" if success else f"✗ Help failed: {stderr}"
        self.test_results.append(("Main help", success, msg))
        print(f"  {msg}")
        return success
    
    def test_list_scanners(self) -> bool:
        """Test: python tools/gs3.py list-scanners"""
        print(f"\n{Colors.BLUE}[TEST] List scanners{Colors.RESET}")
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "list-scanners"
        ])
        
        success = (code == 0 and 
                  "Step 1" in stdout and 
                  "Step 2" in stdout and 
                  "Step 3" in stdout and 
                  "Step 4" in stdout and
                  "46 scanners found" in stdout)
        
        msg = "✓ All 46 scanners listed correctly" if success else f"✗ Scanner list failed: {stderr}"
        self.test_results.append(("List scanners", success, msg))
        print(f"  {msg}")
        return success
    
    def test_list_scanners_step1(self) -> bool:
        """Test: python tools/gs3.py list-scanners --step 1"""
        print(f"\n{Colors.BLUE}[TEST] List scanners (step 1 only){Colors.RESET}")
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "list-scanners", "--step", "1"
        ])
        
        success = code == 0 and "Step 1" in stdout and "18 scanners found" in stdout
        msg = "✓ Step 1 scanners listed correctly" if success else f"✗ Step 1 list failed: {stderr}"
        self.test_results.append(("List scanners --step 1", success, msg))
        print(f"  {msg}")
        return success
    
    def test_scan_quick(self) -> bool:
        """Test: quick scan on include directory"""
        print(f"\n{Colors.BLUE}[TEST] Quick scan (include/){Colors.RESET}")
        
        output_json = self.ai_working / "test_scan_quick.json"
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "scan",
            "include",
            "--scan-mode", "fast",
            "--output", str(output_json)
        ], timeout=600)
        
        success = (code == 0 and 
                  output_json.exists() and 
                  "Total gaps:" in stdout)
        
        if success:
            # Try to parse JSON to verify it's valid
            try:
                with open(output_json) as f:
                    data = json.load(f)
                    gap_count = len(data.get("gaps", []))
                    msg = f"✓ Scan complete ({gap_count} gaps found)"
            except:
                msg = "✓ Scan complete (JSON created)"
        else:
            msg = f"✗ Scan failed"
        
        self.test_results.append(("Quick scan", success, msg))
        print(f"  {msg}")
        return success
    
    def test_report_markdown(self) -> bool:
        """Test: generate markdown report"""
        print(f"\n{Colors.BLUE}[TEST] Report generation (Markdown){Colors.RESET}")
        
        scan_json = self.ai_working / "test_scan_quick.json"
        report_md = self.ai_working / "test_report_quick.md"
        
        if not scan_json.exists():
            msg = "⊘ Skipped: No scan results available"
            self.test_results.append(("Report (Markdown)", None, msg))
            print(f"  {msg}")
            return None
        
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "report",
            str(scan_json),
            "--format", "md",
            "--output", str(report_md)
        ])
        
        success = code == 0 and report_md.exists()
        msg = "✓ Markdown report generated" if success else f"✗ Report failed: {stderr}"
        self.test_results.append(("Report (Markdown)", success, msg))
        print(f"  {msg}")
        return success
    
    def test_report_json(self) -> bool:
        """Test: generate JSON report"""
        print(f"\n{Colors.BLUE}[TEST] Report generation (JSON){Colors.RESET}")
        
        scan_json = self.ai_working / "test_scan_quick.json"
        report_json = self.ai_working / "test_report_quick.json"
        
        if not scan_json.exists():
            msg = "⊘ Skipped: No scan results available"
            self.test_results.append(("Report (JSON)", None, msg))
            print(f"  {msg}")
            return None
        
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "report",
            str(scan_json),
            "--format", "json",
            "--output", str(report_json)
        ])
        
        success = code == 0 and report_json.exists()
        msg = "✓ JSON report generated" if success else f"✗ Report failed: {stderr}"
        self.test_results.append(("Report (JSON)", success, msg))
        print(f"  {msg}")
        return success
    
    def test_config_show(self) -> bool:
        """Test: python tools/gs3.py config --show"""
        print(f"\n{Colors.BLUE}[TEST] Config show{Colors.RESET}")
        code, stdout, stderr = self.run_command([
            sys.executable, "tools/gs3.py", "config", "--show"
        ])
        
        success = code == 0
        msg = "✓ Config show executed successfully" if success else f"✗ Config show failed: {stderr}"
        self.test_results.append(("Config --show", success, msg))
        print(f"  {msg}")
        return success
    
    def run_all(self):
        """Run all tests"""
        print("\n" + "=" * 100)
        print("GS3 CLI INTEGRATION TEST SUITE")
        print("=" * 100)
        
        # Run tests in order
        self.test_help()
        self.test_list_scanners()
        self.test_list_scanners_step1()
        self.test_scan_quick()
        self.test_report_markdown()
        self.test_report_json()
        self.test_config_show()
        
        # Print summary
        self._print_summary()
    
    def _print_summary(self):
        """Print test results summary"""
        print("\n" + "=" * 100)
        print("TEST RESULTS SUMMARY")
        print("=" * 100 + "\n")
        
        passed = sum(1 for _, result, _ in self.test_results if result is True)
        failed = sum(1 for _, result, _ in self.test_results if result is False)
        skipped = sum(1 for _, result, _ in self.test_results if result is None)
        
        for name, result, msg in self.test_results:
            if result is True:
                status = f"{Colors.GREEN}PASS{Colors.RESET}"
            elif result is False:
                status = f"{Colors.RED}FAIL{Colors.RESET}"
            else:
                status = f"{Colors.YELLOW}SKIP{Colors.RESET}"
            
            print(f"  [{status}] {name:35s} - {msg}")
        
        print(f"\n{Colors.BLUE}Summary:{Colors.RESET}")
        print(f"  Passed:  {Colors.GREEN}{passed}{Colors.RESET}")
        print(f"  Failed:  {Colors.RED}{failed}{Colors.RESET}")
        print(f"  Skipped: {Colors.YELLOW}{skipped}{Colors.RESET}")
        print(f"  Total:   {passed + failed + skipped}")
        
        if failed == 0:
            print(f"\n{Colors.GREEN}✓ ALL TESTS PASSED{Colors.RESET}\n")
            return 0
        else:
            print(f"\n{Colors.RED}✗ {failed} TEST(S) FAILED{Colors.RESET}\n")
            return 1


def main():
    """Main entry point"""
    tester = GS3IntegrationTest()
    exit_code = tester.run_all()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
