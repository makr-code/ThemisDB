#!/usr/bin/env python3
"""
GPU CUDA Call Audit Tool (Baseline)
Audit CUDA calls in src/gpu/ and include/gpu/
Track: unchecked calls, RAII wrapper coverage, audit metadata
"""

import json
import os
import re
from pathlib import Path
from typing import Dict, List

class CudaAuditReport:
    def __init__(self):
        self.unchecked_calls = []
        self.wrapped_calls = []
        self.audit_comments = []
        self.files_scanned = 0
        self.total_lines = 0
        
    def to_json(self):
        return {
            "audit_date": "2026-09-02",
            "baseline": True,
            "statistics": {
                "total_unchecked_calls": len(self.unchecked_calls),
                "total_wrapped_calls": len(self.wrapped_calls),
                "total_audit_comments": len(self.audit_comments),
                "files_scanned": self.files_scanned,
                "total_lines": self.total_lines,
            },
            "target_phase_c": "340 → 170 (50% reduction)",
            "target_phase_d": "340 → 51 (85% reduction)",
            "unchecked_calls_sample": self.unchecked_calls[:20],
            "summary": f"Found {len(self.unchecked_calls)} unchecked calls. "
                      f"Target Phase C: ≤170 | Target Phase D: ≤51"
        }

def scan_cuda_calls(repo_root: str) -> CudaAuditReport:
    """Scan GPU module for CUDA calls"""
    report = CudaAuditReport()
    
    gpu_dirs = [
        Path(repo_root) / "src" / "gpu",
        Path(repo_root) / "include" / "gpu"
    ]
    
    cuda_patterns = [
        r'cuda\w+\s*\(',
        r'hip\w+\s*\(',
    ]
    
    wrapper_patterns = [
        r'CudaStreamGuard',
        r'CudaEventGuard',
        r'CudaDeviceMemoryGuard',
        r'CudaSafeRaii',
    ]
    
    for gpu_dir in gpu_dirs:
        if not gpu_dir.exists():
            continue
            
        for pattern in ["*.cpp", "*.h"]:
            for file_path in gpu_dir.rglob(pattern):
                if file_path.is_file():
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                            lines = f.readlines()
                            report.files_scanned += 1
                            report.total_lines += len(lines)
                            
                            for line_no, line in enumerate(lines, 1):
                                for cuda_pattern in cuda_patterns:
                                    if re.search(cuda_pattern, line):
                                        is_wrapped = any(
                                            re.search(wp, line) for wp in wrapper_patterns
                                        )
                                        has_audit = "UNCHECKED:" in line or "AUDIT:" in line
                                        
                                        call_info = {
                                            "file": str(file_path.relative_to(repo_root)),
                                            "line": line_no,
                                            "code": line.strip()[:80],
                                            "wrapped": is_wrapped,
                                            "audit_comment": has_audit
                                        }
                                        
                                        if is_wrapped:
                                            report.wrapped_calls.append(call_info)
                                        elif has_audit:
                                            report.audit_comments.append(call_info)
                                        else:
                                            report.unchecked_calls.append(call_info)
                    except Exception as e:
                        print(f"Warning: {file_path}: {e}")
    
    return report

def main():
    repo_root = "/home/runner/work/ThemisDB/ThemisDB"
    report = scan_cuda_calls(repo_root)
    
    output = report.to_json()
    print(json.dumps(output, indent=2))
    
    output_path = Path(repo_root) / "ai_working" / "GPU_CUDA_AUDIT_BASELINE_2026_09_02.json"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Report: {output_path}")
    print(f"📊 Summary: {len(report.unchecked_calls)} unchecked calls")

if __name__ == "__main__":
    main()
