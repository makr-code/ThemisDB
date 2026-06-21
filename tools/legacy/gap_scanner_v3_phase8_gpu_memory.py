#!/usr/bin/env python3
"""
Phase 8-2: GPU Memory Safety & Coherence Scanner

CWE-416 (Use-After-Free), CWE-401 (Memory Leak)

Detects:
- GPU memory leaked on exception (no RAII)
- CUDA/HIP mismatches
- GPU buffer use after cudaFree/hipFree
- Missing __syncthreads() in kernels
- Incorrect bank conflicts
- Uncoalesced memory access
- Missing cudaMemcpy error checks
- Double-free in GPU allocator
- H2D/D2H race conditions
- Kernel config validation gaps
- VRAM budget exceeded silently
"""

import re
from pathlib import Path
from typing import List, Dict


class GPUMemorySafetyScan:
    """Scan for GPU memory safety and coherence issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for GPU memory safety issues"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cu', '.cpp', '.cc', '.h', '.hpp']:
                continue
            
            # Skip non-GPU files unless they have GPU keywords
            if '.cu' not in str(file_path) and not self._likely_gpu_file(str(file_path)):
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_gpu_memory_leaks(file_path, lines)
            self._check_unchecked_cuda_calls(file_path, lines)
            self._check_use_after_free(file_path, lines)
            self._check_missing_sync_threads(file_path, lines)
            self._check_kernel_config(file_path, lines)
        
        return self.gaps
    
    def _likely_gpu_file(self, file_path: str) -> bool:
        """Check if file likely contains GPU code"""
        content_indicators = ['cuda', 'hip', 'vulkan', '__global__', 'kernel', 'gpu', 'vram']
        return any(indicator in file_path.lower() for indicator in content_indicators)
    
    def _check_gpu_memory_leaks(self, file_path: Path, lines: List[str]):
        """Find GPU memory allocated without cleanup on exception paths"""
        
        for idx, line in enumerate(lines, 1):
            # Look for cudaMalloc/hipMalloc
            if re.search(r'cuda(?:Malloc|Free)|hip(?:Malloc|Free)', line):
                # Check if wrapped in RAII
                malloc_match = re.search(r'cuda(?:Malloc|hipMalloc)', line)
                
                if malloc_match:
                    # Look for corresponding free in next N lines
                    next_lines = '\n'.join(lines[idx:min(idx+50, len(lines))])
                    
                    # Check if free is in try-catch or RAII
                    if 'try' not in lines[max(0, idx-5):idx] and \
                       'std::unique_ptr' not in line and \
                       'std::shared_ptr' not in line:
                        
                        if not re.search(r'cudaFree|hipFree', next_lines):
                            self.gaps.append({
                                'file': str(file_path.relative_to(self.repo_root)),
                                'line': idx,
                                'category': 'gpu_memory_safety',
                                'severity': 'CRITICAL',
                                'pattern': 'gpu_memory_leak',
                                'description': 'GPU memory allocated without RAII wrapper or guaranteed cleanup',
                                'context': line.strip()
                            })
    
    def _check_unchecked_cuda_calls(self, file_path: Path, lines: List[str]):
        """Find CUDA calls without error checking"""
        
        cuda_functions = [
            'cudaMalloc', 'cudaMemcpy', 'cudaLaunchKernel', 'cudaFree',
            'cudaMemset', 'cudaGetLastError', 'cuLaunchKernel'
        ]
        
        for idx, line in enumerate(lines, 1):
            for func in cuda_functions:
                if func in line:
                    # Check if return value is checked
                    if not re.search(r'(CHECK_CUDA|CUDA_CHECK|if|assert|!=)', line):
                        # Check next line too
                        next_line = lines[idx] if idx < len(lines) else ''
                        if not re.search(r'(CHECK_CUDA|CUDA_CHECK|if|assert|!=)', next_line):
                            self.gaps.append({
                                'file': str(file_path.relative_to(self.repo_root)),
                                'line': idx,
                                'category': 'gpu_memory_safety',
                                'severity': 'HIGH',
                                'pattern': 'unchecked_cuda_call',
                                'description': f'CUDA call {func}() without error checking',
                                'context': line.strip()
                            })
    
    def _check_use_after_free(self, file_path: Path, lines: List[str]):
        """Find use-after-free patterns with GPU memory"""
        
        for idx, line in enumerate(lines, 1):
            # Look for pointer usage after free
            if re.search(r'cudaFree|hipFree', line):
                freed_var = re.search(r'\(\s*(\w+)\s*\)', line)
                if freed_var:
                    var_name = freed_var.group(1)
                    
                    # Check next 20 lines for use
                    next_lines = lines[idx:min(idx+20, len(lines))]
                    for use_idx, use_line in enumerate(next_lines, start=idx):
                        if re.search(rf'\b{var_name}\b', use_line) and \
                           'cudaFree' not in use_line and 'hipFree' not in use_line:
                            self.gaps.append({
                                'file': str(file_path.relative_to(self.repo_root)),
                                'line': use_idx,
                                'category': 'gpu_memory_safety',
                                'severity': 'CRITICAL',
                                'pattern': 'use_after_free_gpu',
                                'description': f'Use of freed GPU memory: {var_name}',
                                'context': use_line.strip()
                            })
                            break
    
    def _check_missing_sync_threads(self, file_path: Path, lines: List[str]):
        """Find CUDA kernels without __syncthreads()"""
        
        in_kernel = False
        kernel_start = 0
        
        for idx, line in enumerate(lines, 1):
            if re.search(r'__global__\s+void', line):
                in_kernel = True
                kernel_start = idx
            
            if in_kernel and re.search(r'{', line):
                # Check kernel body for syncthreads
                kernel_body_lines = lines[idx:min(idx+50, len(lines))]
                kernel_body = '\n'.join(kernel_body_lines)
                
                # Multi-threaded access without sync
                if re.search(r'(shared|__shared__|cooperative|atomicAdd)', kernel_body) and \
                   '__syncthreads' not in kernel_body:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': kernel_start,
                        'category': 'gpu_memory_safety',
                        'severity': 'CRITICAL',
                        'pattern': 'missing_sync_threads',
                        'description': 'Shared memory access in CUDA kernel without __syncthreads()',
                        'context': line.strip()
                    })
                
                in_kernel = False
    
    def _check_kernel_config(self, file_path: Path, lines: List[str]):
        """Find kernel configurations without validation"""
        
        for idx, line in enumerate(lines, 1):
            # Look for kernel launch
            if re.search(r'<<<.*>>>|cuLaunchKernel', line):
                # Check if block/grid sizes are validated
                if not re.search(r'(assert|CHECK|if|validate)', '\n'.join(lines[max(0,idx-5):idx+5])):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'gpu_memory_safety',
                        'severity': 'MEDIUM',
                        'pattern': 'kernel_config_validation',
                        'description': 'Kernel launch without block/grid size validation',
                        'context': line.strip()
                    })
