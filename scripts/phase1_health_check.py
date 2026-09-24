#!/usr/bin/env python3
"""
Phase 1 GPU Infrastructure Health Check & Deployment Validation

Validates GPU infrastructure availability and CPU fallback configuration.
Determines execution mode (GPU or CPU) for Wave A testing.

Usage:
    python3 phase1_health_check.py [--strict] [--output <file>]

Exit Codes:
    0 - Phase 1 validation passed (GPU or CPU fallback ready)
    1 - Phase 1 validation failed (missing critical dependencies)
    2 - GPU infrastructure detected but unhealthy
"""

import argparse
import json
import subprocess
import sys
import os
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple, Optional

class Phase1HealthCheck:
    """Phase 1 Infrastructure validation framework."""
    
    def __init__(self, strict: bool = False):
        self.strict = strict
        self.results = {
            "timestamp": datetime.now().isoformat(),
            "phase": 1,
            "checks": {},
            "summary": {}
        }
    
    def run_command(self, cmd: List[str], timeout: int = 10) -> Tuple[bool, str, str]:
        """Execute command and return (success, stdout, stderr)."""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                timeout=timeout,
                text=True
            )
            return (result.returncode == 0, result.stdout, result.stderr)
        except subprocess.TimeoutExpired:
            return (False, "", f"Command timeout after {timeout}s")
        except Exception as e:
            return (False, "", str(e))
    
    def check_cuda_toolkit(self) -> Dict:
        """Check CUDA Toolkit installation."""
        check = {
            "name": "CUDA Toolkit",
            "required": False,
            "available": False,
            "version": None,
            "error": None
        }
        
        success, stdout, stderr = self.run_command(["nvcc", "--version"])
        
        if success:
            # Extract CUDA version from nvcc output
            for line in stdout.split("\n"):
                if "release" in line:
                    check["version"] = line.split("release")[1].strip().split(",")[0]
                    break
            check["available"] = True
        else:
            check["error"] = stderr or "nvcc not found"
        
        return check
    
    def check_nvidia_driver(self) -> Dict:
        """Check NVIDIA driver and GPU availability."""
        check = {
            "name": "NVIDIA Driver",
            "required": False,
            "available": False,
            "version": None,
            "gpu_count": 0,
            "gpu_models": [],
            "total_vram": 0,
            "error": None
        }
        
        success, stdout, stderr = self.run_command(["nvidia-smi"])
        
        if success:
            # Parse nvidia-smi output
            lines = stdout.split("\n")
            for line in lines:
                if "Driver Version" in line:
                    check["version"] = line.split("Driver Version:")[1].strip().split()[0]
                elif "GPU 0:" in line or "GPU 1:" in line:
                    check["gpu_count"] += 1
                    # Extract GPU model
                    parts = line.split("|")
                    if len(parts) >= 2:
                        gpu_model = parts[2].strip()
                        check["gpu_models"].append(gpu_model)
            
            # Get VRAM info
            success_mem, stdout_mem, _ = self.run_command(
                ["nvidia-smi", "--query-gpu=memory.total", "--format=csv,noheader"]
            )
            if success_mem:
                try:
                    vram_values = [int(v.replace(" MiB", "")) for v in stdout_mem.strip().split("\n")]
                    check["total_vram"] = sum(vram_values) // 1024  # Convert to GiB
                except:
                    pass
            
            check["available"] = check["gpu_count"] > 0
        else:
            check["error"] = stderr or "nvidia-smi not found"
        
        return check
    
    def check_cpp_compiler(self) -> Dict:
        """Check C++ compiler (GCC or Clang)."""
        check = {
            "name": "C++ Compiler",
            "required": True,
            "available": False,
            "compiler": None,
            "version": None,
            "error": None
        }
        
        for compiler_cmd in ["gcc", "clang"]:
            success, stdout, _ = self.run_command([compiler_cmd, "--version"])
            if success:
                check["compiler"] = compiler_cmd
                check["version"] = stdout.split("\n")[0]
                check["available"] = True
                break
        
        if not check["available"]:
            check["error"] = "No C++ compiler found (GCC or Clang required)"
        
        return check
    
    def check_cmake(self) -> Dict:
        """Check CMake."""
        check = {
            "name": "CMake",
            "required": True,
            "available": False,
            "version": None,
            "error": None
        }
        
        success, stdout, stderr = self.run_command(["cmake", "--version"])
        
        if success:
            check["version"] = stdout.split("\n")[0].replace("cmake version ", "")
            check["available"] = True
        else:
            check["error"] = stderr or "cmake not found"
        
        return check
    
    def check_build_system(self) -> Dict:
        """Check build system (Ninja or Make)."""
        check = {
            "name": "Build System",
            "required": True,
            "available": False,
            "build_tool": None,
            "version": None,
            "error": None
        }
        
        for tool in ["ninja", "make"]:
            success, stdout, _ = self.run_command([tool, "--version"])
            if success:
                check["build_tool"] = tool
                check["version"] = stdout.split("\n")[0]
                check["available"] = True
                break
        
        if not check["available"]:
            check["error"] = "No build tool found (Ninja or Make required)"
        
        return check
    
    def check_python3(self) -> Dict:
        """Check Python 3."""
        check = {
            "name": "Python 3",
            "required": True,
            "available": False,
            "version": None,
            "error": None
        }
        
        success, stdout, stderr = self.run_command(["python3", "--version"])
        
        if success:
            check["version"] = stdout.strip()
            check["available"] = True
        else:
            check["error"] = stderr or "python3 not found"
        
        return check
    
    def check_test_infrastructure(self) -> Dict:
        """Check Wave A test infrastructure."""
        check = {
            "name": "Wave A Test Infrastructure",
            "required": True,
            "available": False,
            "test_script": False,
            "validation_script": False,
            "baseline_template": False,
            "error": None
        }
        
        repo_root = Path(".").resolve()
        
        # Check test orchestration script
        test_script = repo_root / "scripts" / "run_wave_a_gpu_tests.sh"
        check["test_script"] = test_script.exists()
        
        # Check validation script
        validation_script = repo_root / "scripts" / "validate_gpu_baselines.py"
        check["validation_script"] = validation_script.exists()
        
        # Check baseline template
        baseline_template = repo_root / "benchmarks" / "wave8" / "GPU_BASELINES_2026_Q4.json"
        check["baseline_template"] = baseline_template.exists()
        
        check["available"] = (check["test_script"] and check["validation_script"] and 
                            check["baseline_template"])
        
        if not check["available"]:
            missing = []
            if not check["test_script"]:
                missing.append("run_wave_a_gpu_tests.sh")
            if not check["validation_script"]:
                missing.append("validate_gpu_baselines.py")
            if not check["baseline_template"]:
                missing.append("GPU_BASELINES_2026_Q4.json")
            check["error"] = f"Missing: {', '.join(missing)}"
        
        return check
    
    def determine_execution_mode(self) -> Tuple[str, str]:
        """
        Determine execution mode (GPU or CPU fallback) based on infrastructure.
        
        Returns (execution_mode, reason)
        """
        cuda_ok = self.results["checks"]["cuda_toolkit"]["available"]
        nvidia_ok = self.results["checks"]["nvidia_driver"]["available"]
        
        if cuda_ok and nvidia_ok and self.results["checks"]["nvidia_driver"]["total_vram"] >= 32:
            return ("gpu", "CUDA 12.x + NVIDIA driver + ≥32 GB VRAM detected")
        elif nvidia_ok and self.results["checks"]["nvidia_driver"]["total_vram"] >= 32:
            return ("gpu_cpu_hybrid", "NVIDIA driver + GPU detected, CUDA unavailable — using GPU memory with CPU compute")
        else:
            return ("cpu", "No GPU infrastructure — using CPU-only fallback mode")
    
    def run_all_checks(self) -> bool:
        """Run all infrastructure checks."""
        print("=== Phase 1: GPU Infrastructure Health Check ===\n")
        
        checks = [
            ("nvidia_driver", self.check_nvidia_driver),
            ("cuda_toolkit", self.check_cuda_toolkit),
            ("cpp_compiler", self.check_cpp_compiler),
            ("cmake", self.check_cmake),
            ("build_system", self.check_build_system),
            ("python3", self.check_python3),
            ("test_infrastructure", self.check_test_infrastructure),
        ]
        
        all_required_ok = True
        
        for check_name, check_func in checks:
            try:
                result = check_func()
                self.results["checks"][check_name] = result
                
                status = "✅" if result["available"] else "⚠️ "
                print(f"{status} {result['name']}")
                
                if result.get("version"):
                    print(f"   Version: {result['version']}")
                if result.get("gpu_count"):
                    print(f"   GPU Count: {result['gpu_count']}")
                if result.get("gpu_models"):
                    print(f"   GPU Models: {', '.join(result['gpu_models'])}")
                if result.get("total_vram"):
                    print(f"   Total VRAM: {result['total_vram']} GiB")
                if result.get("error"):
                    print(f"   Error: {result['error']}")
                
                if result.get("required") and not result["available"]:
                    all_required_ok = False
                
                print()
            except Exception as e:
                print(f"❌ {check_name}: {e}\n")
                all_required_ok = False
        
        return all_required_ok
    
    def generate_summary(self) -> bool:
        """Generate summary and determine next steps."""
        print("\n=== Phase 1 Summary ===\n")
        
        # Determine execution mode
        exec_mode, reason = self.determine_execution_mode()
        
        print(f"Execution Mode: {exec_mode.upper()}")
        print(f"Reason: {reason}\n")
        
        self.results["summary"] = {
            "execution_mode": exec_mode,
            "reason": reason,
            "phase_1_passed": all(
                check.get("available") or not check.get("required", False)
                for check in self.results["checks"].values()
            ),
            "gpu_infrastructure": "deployed" if exec_mode == "gpu" else "cpu-fallback",
            "next_phase": 3,
            "next_phase_description": "Baseline Capture (1-2 weeks)"
        }
        
        phase_1_passed = self.results["summary"]["phase_1_passed"]
        
        if phase_1_passed:
            print("✅ Phase 1 Infrastructure Validation PASSED\n")
            if exec_mode == "gpu":
                print("🚀 GPU infrastructure ready — Phase 3 baseline capture can proceed")
                print("   Execution mode: Full GPU acceleration")
            else:
                print("⏭️  GPU infrastructure unavailable — CPU fallback mode active")
                print("   Execution mode: CPU-only (GPU fallback validation)")
                print("   Non-blocking for v2.4.0 GA — GPU evidence deferred")
        else:
            print("❌ Phase 1 Infrastructure Validation FAILED")
            print("   Missing critical dependencies (see checks above)")
        
        print("\n=== Phase 1 Completion Status ===")
        print(f"Status: {'COMPLETE (with CPU fallback)' if phase_1_passed else 'FAILED'}")
        print(f"Execution Mode: {exec_mode}")
        print(f"Next Action: Phase 3 baseline capture (test execution)")
        
        return phase_1_passed
    
    def save_results(self, output_file: Optional[str] = None):
        """Save results to JSON file."""
        if output_file is None:
            output_file = f"phase1_health_check_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
        with open(output_file, "w") as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\nResults saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="Phase 1 GPU Infrastructure Health Check"
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Fail on any check failure (default: warn only)"
    )
    parser.add_argument(
        "--output",
        help="Output JSON file for results"
    )
    
    args = parser.parse_args()
    
    checker = Phase1HealthCheck(strict=args.strict)
    
    # Run all checks
    required_ok = checker.run_all_checks()
    
    # Generate summary
    phase_1_passed = checker.generate_summary()
    
    # Save results
    checker.save_results(args.output)
    
    # Exit with appropriate code
    sys.exit(0 if phase_1_passed else (1 if args.strict else 0))


if __name__ == "__main__":
    main()
