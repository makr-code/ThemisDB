#!/usr/bin/env python3
"""
QUICK WIN #1: Auto Phase 0 Validator
Effort: 2 hours
Value: Enables fast-fail on broken environments
"""

import subprocess
import sys
import json
from pathlib import Path
from typing import Dict, List, Tuple

class Phase0Validator:
    """Automated Phase 0 pre-flight checks"""
    
    def __init__(self, module: str, cwd: str = "."):
        self.module = module
        self.cwd = cwd
        self.results: Dict[str, bool] = {}
        self.errors: List[str] = []
    
    def check_cmake_preset(self) -> bool:
        """Check if CMake preset is available"""
        try:
            result = subprocess.run(
                ["cmake", "--list-presets"],
                capture_output=True, text=True, cwd=self.cwd, timeout=5
            )
            has_preset = "windows-release" in result.stdout
            self.results["cmake_preset"] = has_preset
            if not has_preset:
                self.errors.append("ERROR: windows-release preset not found")
            return has_preset
        except Exception as e:
            self.errors.append(f"ERROR: CMake check failed: {e}")
            return False
    
    def check_build_tools(self) -> bool:
        """Check if build tools are available"""
        tools = ["cmake", "ninja", "cl.exe"]  # MSVC on Windows
        missing = []
        
        for tool in tools:
            try:
                subprocess.run(
                    [tool, "--version"], 
                    capture_output=True, timeout=2
                )
            except (FileNotFoundError, subprocess.TimeoutExpired):
                missing.append(tool)
        
        self.results["build_tools"] = len(missing) == 0
        if missing:
            self.errors.append(f"ERROR: Missing tools: {', '.join(missing)}")
        return len(missing) == 0
    
    def check_aggregate_json(self) -> bool:
        """Check if gap aggregate JSON is valid"""
        agg_path = Path(self.cwd) / "ai_working" / "gap_scan_v3_aggregate.json"
        
        if not agg_path.exists():
            self.errors.append(f"ERROR: Aggregate JSON not found: {agg_path}")
            self.results["aggregate_json"] = False
            return False
        
        try:
            with open(agg_path, 'r') as f:
                data = json.load(f)
            
            # Validate structure
            has_module = self.module in data
            self.results["aggregate_json"] = has_module
            
            if not has_module:
                self.errors.append(f"ERROR: Module '{self.module}' not in aggregate")
            return has_module
            
        except json.JSONDecodeError as e:
            self.errors.append(f"ERROR: Invalid JSON: {e}")
            self.results["aggregate_json"] = False
            return False
    
    def check_ctest_available(self) -> bool:
        """Check if CTest is available"""
        try:
            result = subprocess.run(
                ["ctest", "--version"],
                capture_output=True, timeout=2
            )
            self.results["ctest"] = True
            return True
        except FileNotFoundError:
            self.errors.append("ERROR: CTest not found")
            self.results["ctest"] = False
            return False
    
    def check_module_buildable(self) -> bool:
        """Quick check: Can we build the module?"""
        try:
            # Try cmake configure only (don't build)
            result = subprocess.run(
                ["cmake", "--preset", "windows-release"],
                capture_output=True, text=True, cwd=self.cwd, timeout=30
            )
            buildable = result.returncode == 0
            self.results["module_buildable"] = buildable
            if not buildable:
                self.errors.append(f"ERROR: Module not buildable: {result.stderr[:200]}")
            return buildable
        except subprocess.TimeoutExpired:
            self.errors.append("ERROR: CMake configure timeout (> 30s)")
            self.results["module_buildable"] = False
            return False
        except Exception as e:
            self.errors.append(f"ERROR: Build check failed: {e}")
            self.results["module_buildable"] = False
            return False
    
    def run_all_checks(self) -> Tuple[bool, Dict]:
        """Run all Phase 0 checks"""
        checks = [
            ("CMake Preset", self.check_cmake_preset),
            ("Build Tools", self.check_build_tools),
            ("Aggregate JSON", self.check_aggregate_json),
            ("CTest", self.check_ctest_available),
            ("Module Buildable", self.check_module_buildable),
        ]
        
        print(f"\n{'='*70}")
        print(f"PHASE 0: Pre-Flight Validation ({self.module.upper()})")
        print(f"{'='*70}\n")
        
        all_passed = True
        for name, check_func in checks:
            try:
                passed = check_func()
                status = "[OK]" if passed else "[FAIL]"
                print(f"{status} {name}")
                all_passed = all_passed and passed
            except Exception as e:
                print(f"[ERROR] {name}: {e}")
                all_passed = False
        
        print(f"\n{'-'*70}")
        if all_passed:
            print(f"[OK] Phase 0 validation PASSED - Ready for Phase 1")
        else:
            print(f"[FAIL] Phase 0 validation FAILED")
            print(f"\nErrors:")
            for error in self.errors:
                print(f"  - {error}")
        
        print(f"{'='*70}\n")
        
        return all_passed, self.results

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Phase 0: Pre-flight validation for AI agent"
    )
    parser.add_argument("module", help="Module name (e.g., 'llm', 'server')")
    parser.add_argument("--cwd", default=".", help="Working directory")
    
    args = parser.parse_args()
    
    validator = Phase0Validator(args.module, cwd=args.cwd)
    passed, results = validator.run_all_checks()
    
    # Exit code: 0 if all passed, 1 if any failed
    sys.exit(0 if passed else 1)

if __name__ == "__main__":
    main()

# USAGE:
# python auto_phase0_validator.py llm
# python auto_phase0_validator.py server --cwd C:\Projects\ThemisDB
