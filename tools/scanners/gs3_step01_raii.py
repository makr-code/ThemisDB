#!/usr/bin/env python3
"""
Gap Scanner V3 — RAII Resource Management Scanner (Phase 1.4)

Detects RAII gaps:
- Manual cleanup instead of destructor
- Missing RAII wrapper for resources
- Explicit delete without wrapper
- Resource leaks on exception paths
"""

import sys
from pathlib import Path
from typing import List
import re

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class RAIIScanner(BaseGapScanner):
    """Scan for RAII violations and resource management gaps."""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30
    
    def __init__(self):
        """Initialize RAII Scanner."""
        super().__init__("RAII Scanner", "3.1")
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for RAII gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()  # Ensure absolute path
            self.files_scanned += 1
            
            lines = self._read_file_lines(file_path)
            if not lines:
                continue
            
            # Run all detectors
            gaps.extend(self._check_manual_cleanup(file_path, lines))
            gaps.extend(self._check_unwrapped_resources(file_path, lines))
            gaps.extend(self._check_explicit_delete(file_path, lines))
            gaps.extend(self._check_resource_leak_on_exception(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _check_manual_cleanup(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect manual cleanup in destructor instead of member cleanup."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Pattern: ~ClassName() { ... manually free/delete/close ... }
            if re.search(r'~\w+\s*\(', line):
                # Search destructor body
                brace_count = 0
                destructor_start = line_no - 1
                destructor_end = line_no
                
                for i in range(destructor_start, min(len(lines), destructor_start + 20)):
                    brace_count += lines[i].count('{') - lines[i].count('}')
                    destructor_end = i
                    if brace_count == 0 and i > destructor_start:
                        break
                
                destructor_body = '\n'.join(lines[destructor_start:destructor_end + 1])
                
                # Look for manual cleanup patterns
                cleanup_patterns = [
                    r'delete\s+',
                    r'free\s*\(',
                    r'close\s*\(\)',
                    r'release\s*\(\)',
                    r'\.clear\(\)',
                ]
                
                for pattern in cleanup_patterns:
                    if re.search(pattern, destructor_body):
                        # Check if member variable wraps the cleanup
                        has_wrapper = any(
                            re.search(r'unique_ptr|shared_ptr|auto_ptr|scoped_ptr|lock_guard|scoped_lock',
                                     line)
                            for line in destructor_body.split('\n')
                        )
                        
                        if not has_wrapper:
                            gaps.append(Gap(
                                file=str(file_path.relative_to(self.source_path)),
                                line=line_no,
                                type="manual_cleanup_in_destructor",
                                severity="HIGH",
                                confidence=0.75,
                                description="Manual resource cleanup in destructor (should use RAII wrapper)",
                                remediation="Use std::unique_ptr or similar RAII wrapper instead",
                                context=destructor_body.split('\n')[0].strip(),
                                scanner=self.name,
                                step="01_raii"
                            ))
                            break
        
        return gaps
    
    def _check_unwrapped_resources(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect allocated resources without RAII wrapper."""
        gaps = []
        
        # Pattern: Type* ptr = new Type(...) without unique_ptr/shared_ptr
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\b\w+\*\s+\w+\s*=\s*new\s+', line):
                if 'unique_ptr' not in line and 'shared_ptr' not in line:
                    # Check if this is wrapped in a manage() call or stored in container
                    context = self._context_window_search(
                        lines, line_no,
                        [r'unique_ptr|shared_ptr|make_unique|make_shared|std::move'],
                        window=3
                    )
                    
                    if not context:
                        var_name = re.search(r'\b\w+\*\s+(\w+)\s*=', line)
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type="unwrapped_resource",
                            severity="CRITICAL",
                            confidence=0.85,
                            description="Raw pointer allocated without RAII wrapper",
                            remediation="Use std::unique_ptr<T> or std::shared_ptr<T> instead",
                            context=line.strip(),
                            scanner=self.name,
                            step="01_raii"
                        ))
        
        return gaps
    
    def _check_explicit_delete(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect explicit delete statements."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\bdelete\s+', line):
                # Check if within a wrapper class (allowed in some cases)
                # Look for context: this is in a wrapper's destructor
                if not self._is_in_raii_wrapper_class(lines, line_no):
                    # Check if nullptr assignment follows
                    has_nullptr = self._context_window_search(
                        lines, line_no,
                        [r'=\s*nullptr'],
                        window=2
                    )
                    
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="explicit_delete",
                        severity="HIGH",
                        confidence=0.72,
                        description="Explicit delete statement (prefer smart pointers)",
                        remediation="Use std::unique_ptr/std::shared_ptr to avoid manual delete",
                        context=line.strip(),
                        scanner=self.name,
                        step="01_raii"
                    ))
        
        return gaps
    
    def _check_resource_leak_on_exception(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect potential resource leaks on exception paths."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Pattern: allocate + operation that can throw
            if 'new ' in line and '(' in line:
                var_match = re.search(r'\b(\w+)\s*=\s*new\s+', line)
                if var_match:
                    var_name = var_match.group(1)
                    
                    # Check next lines for throw/exception before cleanup
                    cleanup_patterns = [f'delete\\s+{var_name}', f'{var_name}\\s*=\\s*nullptr']
                    has_cleanup = False
                    
                    for i in range(line_no, min(len(lines), line_no + 15)):
                        if any(re.search(p, lines[i]) for p in cleanup_patterns):
                            has_cleanup = True
                            break
                        
                        # Check for throw/exception before cleanup
                        if re.search(r'throw\s+|\.throw\(', lines[i]):
                            if not has_cleanup:
                                gaps.append(Gap(
                                    file=str(file_path.relative_to(self.source_path)),
                                    line=line_no,
                                    type="resource_leak_on_throw",
                                    severity="HIGH",
                                    confidence=0.68,
                                    description="Resource allocated but may leak on exception",
                                    remediation="Use try/catch to cleanup or wrap in smart_ptr from allocation",
                                    context=line.strip(),
                                    scanner=self.name,
                                    step="01_raii"
                                ))
                                break
        
        return gaps
    
    def _is_in_raii_wrapper_class(self, lines: List[str], line_no: int) -> bool:
        """Check if delete statement is in a RAII wrapper class definition."""
        # Look backwards for class declaration with 'Wrapper' or similar in name
        for i in range(line_no - 1, max(0, line_no - 50), -1):
            if 'class ' in lines[i] or 'struct ' in lines[i]:
                class_line = lines[i]
                # Check if class name suggests it's a wrapper
                return any(
                    keyword in class_line.lower()
                    for keyword in ['wrapper', 'handle', 'guard', 'scoped', 'auto', 'raii']
                )
        return False


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_dir> [--json]")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    json_output = '--json' in sys.argv
    
    scanner = RAIIScanner()
    print(f"[{scanner.name}] Starting scan...\n")
    
    gaps = scanner.scan(source_dir)
    
    # Organize by severity
    by_severity = {}
    by_type = {}
    for gap in gaps:
        by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1
        by_type[gap.type] = by_type.get(gap.type, 0) + 1
    
    print(f"\nFound {len(gaps)} RAII gaps in {scanner.files_scanned} files")
    
    if by_severity:
        print(f"\n  {', '.join(f'{sev}: {count}' for sev, count in sorted(by_severity.items()))}")
    
    if by_type:
        print(f"\nBy Type:")
        for typ, count in sorted(by_type.items(), key=lambda x: -x[1]):
            print(f"  {typ}: {count}")
    
    if json_output:
        import json
        print("\n" + json.dumps([gap.to_dict() for gap in gaps], indent=2))
