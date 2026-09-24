#!/usr/bin/env python3
"""
CUDA Call Reduction Measurement Script

Measures the reduction in unchecked CUDA calls after Phase 2 refactoring.
Compares pre-Phase 2 vs post-Phase 2 codebase.

Usage:
  python3 measure_cuda_reduction.py --phase=2 --output=/path/to/output.json
"""

import re
import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple

# CUDA call patterns to search for
UNCHECKED_CUDA_PATTERNS = {
    "cudaMalloc": r"\bcudaMalloc\s*\(",
    "cudaFree": r"\bcudaFree\s*\(",
    "cudaMemcpy": r"\bcudaMemcpy\s*\(",
    "cudaMemcpyAsync": r"\bcudaMemcpyAsync\s*\(",
    "cudaMemset": r"\bcudaMemset\s*\(",
    "cudaDeviceSynchronize": r"\bcudaDeviceSynchronize\s*\(",
    "cudaStreamCreate": r"\bcudaStreamCreate\s*\(",
    "cudaStreamDestroy": r"\bcudaStreamDestroy\s*\(",
    "cudaEventCreate": r"\bcudaEventCreate\s*\(",
    "cudaEventDestroy": r"\bcudaEventDestroy\s*\(",
    "cudaLaunchKernel": r"\bcudaLaunchKernel\s*\(",
    "thrust_sequence": r"\bthrust::sequence\s*\(",
    "thrust_copy": r"\bthrust::copy\s*\(",
    "thrust_copy_if": r"\bthrust::copy_if\s*\(",
    "thrust_sort": r"\bthrust::sort\s*\(",
    "thrust_stable_sort": r"\bthrust::stable_sort\s*\(",
    "thrust_reduce": r"\bthrust::reduce\s*\(",
}

# Wrapper patterns that protect CUDA calls
WRAPPED_CUDA_PATTERNS = {
    "CHECKED_CUDA": r"\bCHECKED_CUDA\s*\(",
    "CHECKED_HIP": r"\bCHECKED_HIP\s*\(",
    "CudaStreamGuard": r"\bCudaStreamGuard\b",
    "CudaEventGuard": r"\bCudaEventGuard\b",
    "CudaDeviceMemoryGuard": r"\bCudaDeviceMemoryGuard\b",
    "DeviceMemoryGuard": r"\bDeviceMemoryGuard\b",
    "KernelSLAGuard": r"\bKernelSLAGuard\b",
}


def count_pattern_occurrences(file_path: Path, pattern: str) -> int:
    """Count occurrences of a pattern in a file."""
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
        return len(re.findall(pattern, content))
    except (IOError, UnicodeDecodeError):
        return 0


def count_cuda_calls(directory: Path, pattern_name: str, pattern_regex: str) -> Dict[str, int]:
    """Count CUDA calls in all C++ files in a directory."""
    results = {}
    cpp_files = list(directory.glob("**/*.cpp")) + list(directory.glob("**/*.h"))
    
    for cpp_file in cpp_files:
        count = count_pattern_occurrences(cpp_file, pattern_regex)
        if count > 0:
            results[str(cpp_file.relative_to(directory))] = count
    
    return results


def measure_cuda_calls(gpu_module_path: Path, phase: str = "2") -> Dict[str, any]:
    """Measure CUDA call reduction in GPU module."""
    
    results = {
        "phase": phase,
        "measurement_type": "CUDA call reduction analysis",
        "gpu_module_path": str(gpu_module_path),
        "timestamp": __import__("datetime").datetime.utcnow().isoformat() + "Z",
    }
    
    # Count all unchecked CUDA calls
    all_unchecked = {}
    for pattern_name, pattern_regex in UNCHECKED_CUDA_PATTERNS.items():
        file_counts = count_cuda_calls(gpu_module_path, pattern_name, pattern_regex)
        if file_counts:
            all_unchecked[pattern_name] = {
                "total": sum(file_counts.values()),
                "by_file": file_counts
            }
    
    # Count all wrapped CUDA calls
    all_wrapped = {}
    for wrapper_name, wrapper_regex in WRAPPED_CUDA_PATTERNS.items():
        file_counts = count_cuda_calls(gpu_module_path, wrapper_name, wrapper_regex)
        if file_counts:
            all_wrapped[wrapper_name] = {
                "total": sum(file_counts.values()),
                "by_file": file_counts
            }
    
    # Calculate totals
    total_unchecked = sum(item["total"] for item in all_unchecked.values())
    total_wrapped = sum(item["total"] for item in all_wrapped.values())
    
    # Known baseline (from Phase 2 audit 2026-08-24)
    wave7_baseline = 340
    
    # Calculate reduction percentage
    # Reduction = (baseline - current) / baseline * 100
    reduction_percent = ((wave7_baseline - total_unchecked) / wave7_baseline * 100) if wave7_baseline > 0 else 0.0
    
    results["unchecked_cuda_calls"] = {
        "by_type": all_unchecked,
        "total": total_unchecked
    }
    results["wrapped_cuda_calls"] = {
        "by_wrapper": all_wrapped,
        "total": total_wrapped
    }
    results["reduction_analysis"] = {
        "wave7_baseline_calls": wave7_baseline,
        "current_unchecked_calls": total_unchecked,
        "reduction_count": wave7_baseline - total_unchecked,
        "reduction_percentage": round(reduction_percent, 1),
        "target_percentage": 40.0,
        "target_met": reduction_percent >= 40.0
    }
    
    return results


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Measure CUDA call reduction")
    parser.add_argument("--phase", default="2", help="Phase number (default: 2)")
    parser.add_argument("--output", default="/tmp/cuda_reduction_measurement.json",
                       help="Output JSON file path")
    parser.add_argument("--gpu-module", 
                       default="/home/runner/work/ThemisDB/ThemisDB/src/gpu",
                       help="GPU module path")
    
    args = parser.parse_args()
    
    gpu_path = Path(args.gpu_module)
    if not gpu_path.exists():
        print(f"Error: GPU module path not found: {gpu_path}")
        sys.exit(1)
    
    # Run measurement
    results = measure_cuda_calls(gpu_path, args.phase)
    
    # Write results
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    
    # Print summary
    print(f"\n{'='*60}")
    print(f"CUDA Call Reduction Measurement - Phase {args.phase}")
    print(f"{'='*60}")
    print(f"GPU Module: {gpu_path}")
    print(f"Wave 7 Baseline: {results['reduction_analysis']['wave7_baseline_calls']} unchecked calls")
    print(f"Current Count: {results['reduction_analysis']['current_unchecked_calls']} unchecked calls")
    print(f"Reduction: {results['reduction_analysis']['reduction_count']} calls ({results['reduction_analysis']['reduction_percentage']}%)")
    print(f"Target: {results['reduction_analysis']['target_percentage']}%")
    print(f"Status: {'✅ PASS' if results['reduction_analysis']['target_met'] else '❌ FAIL'}")
    print(f"Output: {output_path}")
    print(f"{'='*60}\n")
    
    sys.exit(0 if results['reduction_analysis']['target_met'] else 1)


if __name__ == "__main__":
    main()
