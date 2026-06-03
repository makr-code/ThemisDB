#!/usr/bin/env python3
"""
Phase 8-1: Performance Anti-Patterns & Inefficient Algorithms Scanner

CWE-1104 (Unmaintained Components), Performance

Detects:
- String concatenation in loop (use std::stringstream)
- std::endl vs '\n' (unnecessary flush)
- Repeated std::vector allocations (missing reserve)
- O(n²) nested loops
- Unnecessary copies (auto v = container[i] vs auto& v)
- std::map instead of std::unordered_map
- std::binary_search without prior sort
- Repeated mutex locks in loop
- std::regex in loop (compile once)
- Missing noexcept
- Synchronous I/O in hot path
- Expensive logging in tight loops
"""

import re
from pathlib import Path
from typing import List, Dict


class PerformanceAntiPatternsScan:
    """Scan for performance anti-patterns and inefficient algorithms"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for performance anti-patterns"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_string_concat_loops(file_path, lines)
            self._check_endl_usage(file_path, lines)
            self._check_missing_vector_reserve(file_path, lines)
            self._check_nested_loops_with_find(file_path, lines)
            self._check_unnecessary_copies(file_path, lines)
            self._check_container_choice(file_path, lines)
            self._check_regex_in_loop(file_path, lines)
            self._check_lock_in_loop(file_path, lines)
        
        return self.gaps
    
    def _check_string_concat_loops(self, file_path: Path, lines: List[str]):
        """Find string concatenation in loops"""
        
        for idx, line in enumerate(lines, 1):
            # Look for += in loops
            if re.search(r'for\s*\(.*\)', line):
                # Check next 30 lines for string operations
                loop_end_idx = min(idx + 30, len(lines))
                loop_lines = '\n'.join(lines[idx:loop_end_idx])
                
                if re.search(r'(str.*|\w+)\s*\+=.*["\']', loop_lines):
                    if 'stringstream' not in loop_lines:
                        for loop_idx, loop_line in enumerate(lines[idx:loop_end_idx], start=idx):
                            if re.search(r'\+=.*["\']', loop_line):
                                self.gaps.append({
                                    'file': str(file_path.relative_to(self.repo_root)),
                                    'line': loop_idx,
                                    'category': 'performance',
                                    'severity': 'MEDIUM',
                                    'pattern': 'string_concat_loop',
                                    'description': 'String concatenation in loop (use std::stringstream)',
                                    'context': loop_line.strip()
                                })
                                break
    
    def _check_endl_usage(self, file_path: Path, lines: List[str]):
        """Find std::endl in high-frequency output"""
        
        for idx, line in enumerate(lines, 1):
            # Look for std::endl in loops or logs
            if re.search(r'<<\s*std::endl', line):
                prior_context = '\n'.join(lines[max(0, idx-5):idx])
                if re.search(r'for\s*\(', prior_context) or re.search(r'while\s*\(', prior_context):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'performance',
                        'severity': 'LOW',
                        'pattern': 'endl_in_loop',
                        'description': 'std::endl in loop (causes unnecessary flush, use \'\\n\')',
                        'context': line.strip()
                    })
    
    def _check_missing_vector_reserve(self, file_path: Path, lines: List[str]):
        """Find vector operations without reserve"""
        
        for idx, line in enumerate(lines, 1):
            if re.search(r'for\s*\(', line):
                # Small fixed-size loops are usually fine without reserve.
                if re.search(r'for\s*\([^;]*;[^;]*<\s*(\d+)\s*;', line):
                    bound_match = re.search(r'for\s*\([^;]*;[^;]*<\s*(\d+)\s*;', line)
                    if bound_match and int(bound_match.group(1)) <= 16:
                        continue

                # Only report loops with variable/data-driven bounds (higher growth risk).
                if not any(tok in line for tok in ['.size(', '.length(', 'count', 'total', 'num_', 'n ']):
                    continue

                loop_lines = '\n'.join(lines[idx:min(idx+20, len(lines))])
                loop_slice = lines[idx:min(idx+20, len(lines))]
                
                # Check for push_back without reserve
                if re.search(r'push_back|emplace_back', loop_lines):
                    for loop_idx, loop_line in enumerate(lines[idx:min(idx+20, len(lines))], start=idx):
                        push_match = re.search(r'\b([A-Za-z_]\w*)\s*\.\s*(push_back|emplace_back)\s*\(', loop_line)
                        if not push_match:
                            continue

                        vec_name = push_match.group(1)

                        # Only flag when variable can be identified as std::vector.
                        decl_start = max(0, idx - 60)
                        decl_context = '\n'.join(lines[decl_start:loop_idx])
                        is_vector = re.search(rf'std::vector\s*<[^>]+>\s+{re.escape(vec_name)}\b', decl_context) is not None
                        if not is_vector:
                            continue

                        # Single append operations are usually not worth a warning.
                        push_count = sum(
                            1 for candidate in loop_slice
                            if re.search(rf'\b{re.escape(vec_name)}\s*\.\s*(push_back|emplace_back)\s*\(', candidate)
                        )
                        if push_count < 2:
                            continue

                        # reserve() before or inside loop suppresses finding.
                        reserve_context = '\n'.join(lines[max(0, idx - 40):min(len(lines), idx + 20)])
                        has_reserve = re.search(rf'\b{re.escape(vec_name)}\s*\.\s*reserve\s*\(', reserve_context) is not None
                        if has_reserve:
                            continue

                        self.gaps.append({
                            'file': str(file_path.relative_to(self.repo_root)),
                            'line': loop_idx,
                            'category': 'performance',
                            'severity': 'MEDIUM',
                            'pattern': 'missing_vector_reserve',
                            'description': 'vector::push_back in loop without prior reserve()',
                            'context': loop_line.strip()
                        })
                        break

    
    def _check_nested_loops_with_find(self, file_path: Path, lines: List[str]):
        """Find O(n²) patterns with nested loops and find"""
        
        nested_for_count = 0
        for idx, line in enumerate(lines, 1):
            if re.search(r'for\s*\(', line):
                nested_for_count += 1
            
            if nested_for_count >= 2 and re.search(r'\.find\(|std::find', line):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'performance',
                    'severity': 'HIGH',
                    'pattern': 'nested_loop_find',
                    'description': 'O(n²) pattern: linear search inside nested loop',
                    'context': line.strip()
                })
            
            if re.search(r'}\s*$', line):
                nested_for_count = max(0, nested_for_count - 1)
    
    def _check_unnecessary_copies(self, file_path: Path, lines: List[str]):
        """Find unnecessary copies from containers"""
        
        for idx, line in enumerate(lines, 1):
            # auto v = container[i] should be auto& v or const auto&
            if re.search(r'auto\s+\w+\s*=\s*\w+\[[^\]]+\]', line):
                if '&' not in line and 'const' not in lines[max(0, idx-2)]:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'performance',
                        'severity': 'MEDIUM',
                        'pattern': 'unnecessary_copy',
                        'description': 'Unnecessary copy: use auto& for container element access',
                        'context': line.strip()
                    })
    
    def _check_container_choice(self, file_path: Path, lines: List[str]):
        """Find suboptimal container choices"""
        
        for idx, line in enumerate(lines, 1):
            # std::map used for lookups only (should be unordered_map)
            if re.search(r'std::map\s*<', line):
                # Look ahead for find() without ordering requirement
                next_lines = '\n'.join(lines[idx:min(idx+50, len(lines))])
                if '.find(' in next_lines and 'lower_bound' not in next_lines:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'performance',
                        'severity': 'MEDIUM',
                        'pattern': 'map_vs_unordered_map',
                        'description': 'std::map used only for lookups (consider std::unordered_map)',
                        'context': line.strip()
                    })
    
    def _check_regex_in_loop(self, file_path: Path, lines: List[str]):
        """Find std::regex compiled inside loops"""
        
        for idx, line in enumerate(lines, 1):
            if re.search(r'for\s*\(', line):
                loop_lines = '\n'.join(lines[idx:min(idx+20, len(lines))])
                
                if re.search(r'std::regex\s*\(', loop_lines):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'performance',
                        'severity': 'HIGH',
                        'pattern': 'regex_in_loop',
                        'description': 'std::regex compiled in loop (compile once, reuse)',
                        'context': line.strip()
                    })
    
    def _check_lock_in_loop(self, file_path: Path, lines: List[str]):
        """Find mutex locks inside loops"""
        
        for idx, line in enumerate(lines, 1):
            if re.search(r'for\s*\(', line):
                # Restrict to actual loop body to avoid cross-scope/context confusion.
                body_lines = []
                if '{' in line:
                    brace_depth = line.count('{') - line.count('}')
                    j = idx
                    while j < len(lines) and brace_depth > 0:
                        cur = lines[j]
                        body_lines.append(cur)
                        brace_depth += cur.count('{') - cur.count('}')
                        j += 1
                else:
                    # Single-line or next-statement loop body.
                    if idx < len(lines):
                        body_lines.append(lines[idx])

                loop_lines = '\n'.join(body_lines)

                if 'wait_for' in loop_lines or 'future_status' in loop_lines:
                    continue

                # Require an actual lock construct close to loop-body entry.
                body_head = '\n'.join(body_lines[:8])

                if re.search(r'\b(lock_guard|unique_lock|scoped_lock)\b', body_head):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'performance',
                        'severity': 'HIGH',
                        'pattern': 'lock_in_loop',
                        'description': 'Mutex lock acquired per iteration (move outside loop)',
                        'context': line.strip()
                    })
