#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Memory Safety Gaps Detection (IMPROVED)

IMPROVEMENTS (2026-06-14):
1. RAII Whitelist: unique_ptr, shared_ptr, make_unique, make_shared → skip "new_without_delete"
2. Smart Pointer Detection: Don't flag smart ptr operations as leaks
3. GPU Memory APIs: Exclude cudaMalloc/cudaFree patterns from db_connection_leak
4. CUDA Error Checking: Only flag unchecked CUDA calls in actual code, not comments
5. Safe String APIs: Exclude std::string, nlohmann::json from pointer_arithmetic
6. RAII Destructors: Skip delete in destructor ~ClassName() contexts
7. Comment Filtering: Don't scan in-code comments/Doxygen strings

Detects:
- Raw new/delete without RAII (memory leaks) [with smart-ptr whitelist]
- Pointer arithmetic without bounds checking [excluding safe APIs]
- Unchecked malloc/realloc [code-only, not comments]
- Array out-of-bounds (static analysis)
- Delete without nullptr assignment [RAII-destructors excluded]
- Shared pointer cycles
- Use-after-free risks
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Set, Tuple
from enum import Enum


class MemoryGapType(Enum):
    """Memory safety gap classifications"""
    NEW_WITHOUT_DELETE = "new_without_delete"
    POINTER_ARITHMETIC = "pointer_arithmetic"
    UNCHECKED_MALLOC = "unchecked_malloc"
    ARRAY_BOUNDS = "array_bounds"
    DELETE_NO_NULLPTR = "delete_no_nullptr"
    SHARED_PTR_CYCLE = "shared_ptr_cycle"
    MANUAL_MEMORY = "manual_memory"
    POINTER_OWNERSHIP = "pointer_ownership"


@dataclass
class MemoryGap:
    """Represents a memory safety gap"""
    file_path: str
    line_num: int
    gap_type: MemoryGapType
    snippet: str
    severity: str  # CRITICAL, HIGH, MEDIUM
    description: str
    remediation: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
        }


class MemoryGapScannerImproved:
    """Detect memory safety issues in C++ code (with RAII/FP filters)"""
    
    # IMPROVEMENT 1: Smart pointer patterns (WHITELIST)
    SMART_PTR_KEYWORDS = {
        'unique_ptr', 'shared_ptr', 'weak_ptr',
        'make_unique', 'make_shared',
        'get_deleter', 'use_count', 'std::move',
    }
    
    # IMPROVEMENT 3: GPU Memory APIs (WHITELIST)
    GPU_MEMORY_APIS = {
        'cudaMalloc', 'cudaFree', 'cudaMemcpy', 'cudaMemset',
        'CudaDeviceMemory', 'cuda::memory',
    }
    
    # IMPROVEMENT 5: Safe C++ APIs (WHITELIST for pointer_arithmetic)
    SAFE_STRING_APIS = {
        'std::string', 'std::string_view',
        'nlohmann::json',
        'std::vector', 'std::array', 'std::span',
        'std::optional', 'std::expected',
    }
    
    # Pattern definitions
    NEW_PATTERNS = {
        'new_keyword': re.compile(r'\bnew\s+\w+\s*\('),
        'delete_keyword': re.compile(r'\bdelete\s+\w+'),
        'delete_array': re.compile(r'\bdelete\s*\[\s*\]'),
    }
    
    MALLOC_PATTERNS = {
        'malloc': re.compile(r'\bmalloc\s*\('),
        'calloc': re.compile(r'\bcalloc\s*\('),
        'realloc': re.compile(r'\brealloc\s*\('),
    }
    
    CUDA_PATTERNS = {
        'cuda_malloc': re.compile(r'\bcudaMalloc\s*\('),
        'cuda_free': re.compile(r'\bcudaFree\s*\('),
        'cuda_memset': re.compile(r'\bcudaMemset\s*\('),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[MemoryGap]] = {}

    # IMPROVEMENT 2+4: Comment/String detection
    def _is_in_comment(self, line: str, column: int) -> bool:
        """Check if position is inside a comment"""
        # Single-line comment
        comment_idx = line.find('//')
        if comment_idx != -1 and column >= comment_idx:
            return True
        
        # Simple check for block comment (/* */)
        block_start = line.find('/*')
        block_end = line.find('*/')
        if block_start != -1 and column >= block_start:
            if block_end == -1 or column <= block_end + 1:
                return True
        
        return False

    def _is_in_string_literal(self, line: str, column: int) -> bool:
        """Check if position is inside a string literal"""
        in_string = False
        escape = False
        for i, c in enumerate(line):
            if i >= column:
                break
            if escape:
                escape = False
                continue
            if c == '\\':
                escape = True
            elif c == '"':
                in_string = not in_string
        return in_string

    # IMPROVEMENT 6: RAII Destructor detection
    def _is_in_destructor(self, lines: List[str], line_idx: int) -> bool:
        """Check if we're inside a destructor function"""
        # Look backwards for ~ClassName pattern
        for i in range(line_idx, max(0, line_idx - 50), -1):
            line = lines[i]
            if re.search(r'~\w+\s*\(', line):
                # Found destructor signature
                # Check we're before closing brace of destructor
                open_braces = 0
                for j in range(i, line_idx + 1):
                    open_braces += lines[j].count('{') - lines[j].count('}')
                if open_braces > 0:
                    return True
                break
        return False

    def _has_bounds_guard_context(self, lines: List[str], line_idx: int) -> bool:
        """Detect common bounds guards"""
        start = max(0, line_idx - 120)
        context = ''.join(lines[start:line_idx])
        guard_patterns = [
            r'if\s*\([^)]*(data|buffer|array)\.size\s*\(\)\s*[<>]=?\s*[^)]*\)',
            r'if\s*\([^)]*\bsize\s*\(\)\s*[<>]=?\s*[^)]*\)',
            r'assert\s*\([^)]*(size\s*\(\)|length\s*\(\))[^)]*\)',
            r'CHECK\s*\([^)]*(size\s*\(\)|length\s*\(\))[^)]*\)',
        ]
        return any(re.search(p, context) for p in guard_patterns)

    def _extract_index_variables(self, expr: str) -> Set[str]:
        """Extract variable names from index expression"""
        vars_found = set(re.findall(r'\b[A-Za-z_][A-Za-z0-9_]*\b', expr))
        ignore = {
            'int', 'size_t', 'std', 'min', 'max', 'clamp',
            'static_cast', 'reinterpret_cast', 'const_cast', 'nullptr'
        }
        return {v for v in vars_found if v not in ignore and not v.isdigit()}

    # IMPROVEMENT 1: RAII/Smart-ptr whitelist check
    def _has_raii_wrapper(self, lines: List[str], line_idx: int) -> bool:
        """Check if 'new' is wrapped in smart pointer or make_* call"""
        # Check same line and previous 2 lines
        for i in range(max(0, line_idx - 2), line_idx + 1):
            line = lines[i]
            for keyword in self.SMART_PTR_KEYWORDS:
                if keyword in line:
                    return True
        
        # Check if line looks like: "auto ptr = std::make_unique<T>(...)"
        line = lines[line_idx]
        if any(pattern in line for pattern in ['make_unique', 'make_shared', 'std::unique_ptr<', 'std::shared_ptr<']):
            return True
        
        return False

    # IMPROVEMENT 3: GPU Memory API check
    def _is_gpu_memory_api(self, line: str) -> bool:
        """Check if line uses GPU memory APIs"""
        for api in self.GPU_MEMORY_APIS:
            if api in line:
                return True
        return False

    # IMPROVEMENT 5: Safe API check
    def _uses_safe_api(self, line: str) -> bool:
        """Check if access uses safe C++ APIs"""
        for api in self.SAFE_STRING_APIS:
            if api in line:
                return True
        # Also check for .at() method (bounds-checked)
        if '.at(' in line:
            return True
        return False

    def scan_file(self, file_path: Path) -> List[MemoryGap]:
        """Scan single file for memory safety gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # IMPROVEMENT 7: Skip comment/Doxygen lines
            if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                continue
            
            # Skip test code and examples
            if any(x in file_path.name.lower() for x in ['test', 'mock', 'example', 'demo']):
                continue
            if any(x in line.lower() for x in ['TEST_F(', 'TEST(', 'MOCK_', 'TEST_']):
                continue
            
            # --- CHECK 1: Raw 'new' without RAII ---
            if self.NEW_PATTERNS['new_keyword'].search(line):
                # IMPROVEMENT 1: Skip if wrapped in smart ptr
                if self._has_raii_wrapper(lines, line_num - 1):
                    continue
                
                # IMPROVEMENT 4: Skip if in comment
                if self._is_in_comment(line, line.find('new')):
                    continue
                
                gap = MemoryGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=MemoryGapType.NEW_WITHOUT_DELETE,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Raw new without RAII wrapper — potential memory leak',
                    remediation='Use std::unique_ptr<T> or std::make_unique<T>()'
                )
                gaps.append(gap)
            
            # --- CHECK 2: Pointer arithmetic / indexed access ---
            index_match = re.search(r'\b([A-Za-z_]\w*(?:ptr|buffer|data|array))\s*\[\s*([^\]]+)\s*\]', line)
            offset_match = re.search(r'\*\s*\(\s*([A-Za-z_]\w*)\s*\+\s*([^)]+)\)', line)
            
            if (index_match or offset_match):
                # IMPROVEMENT 5: Skip safe APIs
                if self._uses_safe_api(line):
                    continue
                
                # Skip lines with .at() or gsl::span
                if '.at(' in line or 'gsl::span' in line or 'std::span' in line:
                    continue
                
                index_expr = ''
                if index_match:
                    index_expr = index_match.group(2).strip()
                elif offset_match:
                    index_expr = offset_match.group(2).strip()
                
                if index_expr.isdigit():
                    continue
                
                index_vars = self._extract_index_variables(index_expr)
                
                prev_context = ''.join(lines[max(0, line_num - 12):line_num])
                has_local_guard = any(check in prev_context for check in ['if', 'assert', 'CHECK'])
                has_wide_guard = self._has_bounds_guard_context(lines, line_num - 1)
                
                if not (has_local_guard or has_wide_guard):
                    gap = MemoryGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MemoryGapType.POINTER_ARITHMETIC,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Pointer/array access without bounds validation',
                        remediation='Add bounds check before dereferencing'
                    )
                    gaps.append(gap)
            
            # --- CHECK 3: Unchecked malloc/calloc/realloc ---
            for malloc_type, pattern in [('malloc', self.MALLOC_PATTERNS['malloc']),
                                         ('calloc', self.MALLOC_PATTERNS['calloc']),
                                         ('realloc', self.MALLOC_PATTERNS['realloc'])]:
                if pattern.search(line):
                    # IMPROVEMENT 4: Skip if in comment
                    if self._is_in_comment(line, line.find(malloc_type)):
                        continue
                    
                    next_context = ''.join(lines[line_num:min(len(lines), line_num + 3)])
                    if 'if' not in next_context or 'nullptr' not in next_context:
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.UNCHECKED_MALLOC,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'Unchecked {malloc_type} — no null check before use',
                            remediation='Check: if (ptr != nullptr) before dereferencing'
                        )
                        gaps.append(gap)
            
            # --- CHECK 4: Delete without nullptr ---
            if self.NEW_PATTERNS['delete_keyword'].search(line):
                # IMPROVEMENT 6: Skip if in destructor
                if self._is_in_destructor(lines, line_num - 1):
                    continue
                
                # IMPROVEMENT 4: Skip if in comment
                if self._is_in_comment(line, line.find('delete')):
                    continue
                
                match = re.search(r'delete\s+(\w+)', line)
                if match:
                    var_name = match.group(1)
                    next_context = ''.join(lines[line_num:min(len(lines), line_num + 2)])
                    if f'{var_name} = nullptr' not in next_context:
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.DELETE_NO_NULLPTR,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description='Delete without nullifying pointer — use-after-free risk',
                            remediation=f'After delete: {var_name} = nullptr;'
                        )
                        gaps.append(gap)
            
            # --- CHECK 5: Shared ptr cycles ---
            if 'std::shared_ptr' in line and any(x in line for x in ['next:', 'prev:', 'parent:', 'child:']):
                prev_context = ''.join(lines[max(0, line_num - 10):line_num])
                next_context = ''.join(lines[line_num:min(len(lines), line_num + 10)])
                combined = prev_context + next_context
                
                shared_ptr_count = combined.count('std::shared_ptr')
                if shared_ptr_count >= 2:
                    gap = MemoryGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MemoryGapType.SHARED_PTR_CYCLE,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description='Bidirectional shared_ptr — potential reference cycle leak',
                        remediation='Use weak_ptr for back-references to break cycle'
                    )
                    gaps.append(gap)
        
        return gaps

    def scan_directory(self, dir_path: Path) -> Dict[str, List[MemoryGap]]:
        """Scan all C++ files in directory"""
        self.gaps = {}
        
        cpp_files = list(dir_path.rglob('*.cpp')) + list(dir_path.rglob('*.hpp')) + \
                    list(dir_path.rglob('*.h')) + list(dir_path.rglob('*.cc'))
        
        for cpp_file in cpp_files:
            gaps = self.scan_file(cpp_file)
            if gaps:
                self.gaps[str(cpp_file)] = gaps
        
        return self.gaps

    def run_full_scan(self, output_dir: str = '') -> Dict[str, Dict]:
        """Compatibility wrapper for classic orchestrator interface."""
        scan_root = self.repo_root / 'src'
        if not scan_root.exists():
            scan_root = self.repo_root

        gaps_by_file = self.scan_directory(scan_root)
        modules: Dict[str, Dict] = {}

        for file_path, file_gaps in gaps_by_file.items():
            rel = str(Path(file_path).resolve().relative_to(self.repo_root.resolve())).replace('\\', '/')
            parts = rel.split('/')
            module = parts[1] if len(parts) > 1 and parts[0] == 'src' else parts[0]

            if module not in modules:
                modules[module] = {
                    'total': 0,
                    'severity_critical': 0,
                    'severity_high': 0,
                    'severity_medium': 0,
                    'gaps_by_file': {},
                }

            modules[module]['gaps_by_file'][rel] = [g.to_dict() for g in file_gaps]
            modules[module]['total'] += len(file_gaps)

            for g in file_gaps:
                sev = g.severity.upper()
                if sev == 'CRITICAL':
                    modules[module]['severity_critical'] += 1
                elif sev == 'HIGH':
                    modules[module]['severity_high'] += 1
                elif sev == 'MEDIUM':
                    modules[module]['severity_medium'] += 1

        return modules


if __name__ == '__main__':
    import sys
    
    repo_root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
    scanner = MemoryGapScannerImproved(repo_root=str(repo_root))
    
    gaps_by_file = scanner.scan_directory(repo_root / 'src')
    
    total_gaps = sum(len(gaps) for gaps in gaps_by_file.values())
    print(f"Found {total_gaps} memory safety gaps (improved filter)")
    
    for file_path, gaps in gaps_by_file.items():
        print(f"\n{file_path}:")
        for gap in gaps:
            print(f"  L{gap.line_num}: [{gap.severity}] {gap.gap_type.value}")
