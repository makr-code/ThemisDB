#!/usr/bin/env python3
"""
Gap Scanner Step 01.1 — Memory Safety

Detects:
- Raw new/delete without RAII wrapper (memory leaks)
- Pointer arithmetic without bounds checking
- Unchecked malloc/calloc/realloc
- Array out-of-bounds (static analysis)
- Delete without nullptr assignment (use-after-free risk)
- Shared pointer reference cycles
- Unsafe pointer ownership patterns
"""

import re
import sys
from pathlib import Path
from typing import List

# Import base scanner
sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class MemorySafetyScanner(BaseGapScanner):
    """Phase 1.1: Memory Safety (±5 line context)"""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30
    
    def __init__(self):
        """Initialize Memory Safety Scanner."""
        super().__init__("Memory Safety Scanner", "3.1")
    
    def __init__(self):
        super().__init__("MemorySafetyScanner", "3.0")
        
        # Pattern definitions
        self.new_pattern = re.compile(r'\bnew\s+\w+\s*\(')
        self.delete_pattern = re.compile(r'\bdelete\s+(\w+)')
        self.malloc_pattern = re.compile(r'\b(malloc|calloc|realloc)\s*\(')
        self.unique_ptr_pattern = re.compile(r'std::unique_ptr\s*<|make_unique\s*<')
        self.shared_ptr_pattern = re.compile(r'std::shared_ptr\s*<|make_shared\s*<')
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for memory safety gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()  # Ensure absolute path
            self.files_scanned += 1
            lines = self._read_file_lines(file_path)
            
            # Skip test files
            if 'test' in file_path.name.lower():
                continue
            
            gaps.extend(self._check_new_without_raii(file_path, lines))
            gaps.extend(self._check_pointer_arithmetic(file_path, lines))
            gaps.extend(self._check_unchecked_malloc(file_path, lines))
            gaps.extend(self._check_delete_without_nullptr(file_path, lines))
            gaps.extend(self._check_shared_ptr_cycles(file_path, lines))
            gaps.extend(self._check_array_bounds(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _check_new_without_raii(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect raw new() without smart pointer wrapper"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if self.new_pattern.search(line):
                # Check if smart ptr in context (±2 lines)
                if not self._context_window_search(lines, line_no, 
                    ['unique_ptr', 'shared_ptr', 'make_unique', 'make_shared'], 
                    window=2):
                    
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="new_without_raii",
                        severity="CRITICAL",
                        confidence=0.80,
                        description="Raw new() without RAII wrapper — potential memory leak",
                        remediation="Use std::unique_ptr<T> or std::make_unique<T>()",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_pointer_arithmetic(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect pointer/array access without bounds checking"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            if line.strip().startswith('//'):
                continue
            
            # Check for pointer/array dereference
            if ('ptr' in line or 'buffer' in line or 'data' in line or 'array' in line) and \
               ('->' in line or ('[' in line and ']' in line)):
                
                # Check if bounds guard exists in previous context
                has_guard = self._context_window_search(lines, line_no,
                    [r'if\s*\([^)]*size\s*\(\)', 
                     r'if\s*\([^)]*\.length\s*\(',
                     r'if\s*\([^)]*\bindex\b',
                     r'assert\s*\([^)]*size',
                     r'CHECK\s*\([^)]*size'],
                    window=12)
                
                if not has_guard:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="pointer_arithmetic_unbounded",
                        severity="HIGH",
                        confidence=0.70,
                        description="Pointer/array access without bounds validation",
                        remediation="Add bounds check before dereferencing (e.g., if (index < size))",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_unchecked_malloc(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect malloc/calloc/realloc without null checks"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            match = self.malloc_pattern.search(line)
            if match:
                malloc_type = match.group(1)
                
                # Check if result is checked in next few lines
                has_check = self._context_window_search(lines, line_no,
                    [r'if\s*\([^)]*!=\s*nullptr',
                     r'if\s*\([^)]*==\s*NULL',
                     r'if\s*\(!\s*\w+\)',
                     r'CHECK.*nullptr'],
                    window=3)
                
                if not has_check:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="unchecked_malloc",
                        severity="HIGH",
                        confidence=0.75,
                        description=f"Unchecked {malloc_type}() — missing null pointer check",
                        remediation=f"Add: if (ptr != nullptr) before dereferencing",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_delete_without_nullptr(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect delete without nullifying pointer (use-after-free risk)"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            match = self.delete_pattern.search(line)
            if match:
                var_name = match.group(1)
                
                # Check if variable is nullified after delete
                has_nullptr = self._context_window_search(lines, line_no,
                    [rf'{var_name}\s*=\s*nullptr'],
                    window=2)
                
                if not has_nullptr:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="delete_without_nullptr",
                        severity="HIGH",
                        confidence=0.70,
                        description="Delete without nullifying pointer — use-after-free risk",
                        remediation=f"After delete: {var_name} = nullptr;",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_shared_ptr_cycles(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect bidirectional shared_ptr (reference cycle risk)"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if self.shared_ptr_pattern.search(line):
                # Check for bidirectional pointers
                if any(member in line for member in ['next', 'prev', 'parent', 'child']):
                    # Count shared_ptr in context
                    context = '\n'.join(self._get_context(lines, line_no, window=10))
                    shared_ptr_count = context.count('shared_ptr')
                    
                    if shared_ptr_count >= 2:
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type="shared_ptr_cycle",
                            severity="MEDIUM",
                            confidence=0.65,
                            description="Bidirectional shared_ptr — potential reference cycle leak",
                            remediation="Use weak_ptr for back-references to break cycle",
                            context=context
                        ))
        
        return gaps
    
    def _check_array_bounds(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect static array bounds violations"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Look for array declaration: Type arr[SIZE];
            array_match = re.search(r'(\w+)\s*\[\s*(\d+)\s*\]', line)
            if array_match:
                array_name = array_match.group(1)
                array_size = int(array_match.group(2))
                
                # Look for loop accessing this array in next lines
                next_lines = '\n'.join(lines[line_no:min(len(lines), line_no+20)])
                loop_match = re.search(rf'for\s*\([^)]*\s*<\s*(\d+)', next_lines)
                
                if loop_match:
                    loop_size = int(loop_match.group(1))
                    if loop_size > array_size:
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type="array_bounds_violation",
                            severity="CRITICAL",
                            confidence=0.85,
                            description=f"Array bounds violation: loop {loop_size} > array size {array_size}",
                            remediation="Fix loop condition or increase array size",
                            context='\n'.join(self._get_context(lines, line_no, window=5))
                        ))
        
        return gaps


if __name__ == "__main__":
    import time
    
    source_dir = sys.argv[1] if len(sys.argv) > 1 else "./src"
    
    print("[Memory Safety Scanner] Starting scan...")
    start = time.time()
    
    scanner = MemorySafetyScanner()
    gaps = scanner.scan(source_dir)
    
    elapsed = time.time() - start
    
    print(f"\nFound {len(gaps)} memory safety gaps in {elapsed:.2f}s")
    print(f"Scanned {scanner.files_scanned} files\n")
    
    # Group by severity
    by_severity = {}
    for gap in gaps:
        sev = gap.severity
        by_severity[sev] = by_severity.get(sev, 0) + 1
    
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM']:
        if sev in by_severity:
            print(f"  {sev}: {by_severity[sev]}")
    
    # Group by type
    by_type = {}
    for gap in gaps:
        typ = gap.type
        by_type[typ] = by_type.get(typ, 0) + 1
    
    print("\nBy Type:")
    for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:5]:
        print(f"  {typ}: {count}")
