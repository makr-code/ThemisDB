#!/usr/bin/env python3
"""
Gap Scanner V3 — Thread Safety Scanner (Phase 1.3)

Detects threading gaps:
- Race conditions (shared state without synchronization)
- Deadlock patterns (circular lock ordering)
- Lock misuse (missing lock, double lock)
- Volatile missing (shared primitives)
- Thread join without timeout
"""

import sys
from pathlib import Path
from typing import List
import re

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class ThreadSafetyScanner(BaseGapScanner):
    """Scan for threading and synchronization gaps."""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30
    
    def __init__(self):
        """Initialize Thread Safety Scanner."""
        super().__init__("Thread Safety Scanner", "3.1")
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for thread safety gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()  # Ensure absolute path
            self.files_scanned += 1
            
            lines = self._read_file_lines(file_path)
            if not lines:
                continue
            
            # Run all detectors
            gaps.extend(self._check_shared_state_without_sync(file_path, lines))
            gaps.extend(self._check_missing_volatile(file_path, lines))
            gaps.extend(self._check_lock_misuse(file_path, lines))
            gaps.extend(self._check_thread_join_no_timeout(file_path, lines))
            gaps.extend(self._check_circular_lock_ordering(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _check_shared_state_without_sync(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect shared state access without synchronization."""
        gaps = []
        
        # Pattern: member variable with thread-related methods (run, thread, async)
        patterns = [
            (r'\b(?:m_|_)\w+\b\s*=', 'assignment to member'),
            (r'static\s+\w+\s+\w+\s*;', 'static variable'),
        ]
        
        for line_no, line in enumerate(lines, 1):
            # Check for unprotected member access
            if 'm_' in line or '_' in line[:20]:  # Member variable indicator
                if any(re.search(p[0], line) for p in patterns):
                    # Check if protected by lock/guard in ±5 lines
                    context = self._context_window_search(
                        lines, line_no, 
                        [r'lock_guard|unique_lock|scoped_lock|mutex|mtx_'],
                        window=5
                    )
                    
                    if not context:
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type="shared_state_no_sync",
                            severity="HIGH",
                            confidence=0.68,
                            description="Shared state accessed without synchronization",
                            remediation="Add mutex lock/guard to protect access",
                            context=line.strip(),
                            scanner=self.name,
                            step="01_thread_safety"
                        ))
        
        return gaps
    
    def _check_missing_volatile(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect shared primitives without volatile."""
        gaps = []
        
        # Pattern: bool/int/double assigned in one thread, read in another
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\b(?:bool|int|uint|double|float)\s+\w+\s*=\s*(?:true|false|\d)', line):
                if 'volatile' not in line and 'atomic' not in line and 'mutex' not in line:
                    # Check if used in threading context
                    context = self._context_window_search(
                        lines, line_no,
                        [r'std::thread|thread|async'],
                        window=10
                    )
                    
                    if context:
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type="primitive_no_volatile",
                            severity="MEDIUM",
                            confidence=0.72,
                            description="Primitive shared across threads without volatile",
                            remediation="Use std::atomic<T> or volatile keyword",
                            context=line.strip(),
                            scanner=self.name,
                            step="01_thread_safety"
                        ))
        
        return gaps
    
    def _check_lock_misuse(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect lock misuse patterns."""
        gaps = []
        
        # Pattern 1: Explicit lock/unlock without guard
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\.lock\(\)|mtx\.lock\(\)', line):
                # Check for unlock in ±10 lines
                has_unlock = any(
                    re.search(r'\.unlock\(\)|mtx\.unlock\(\)', lines[i])
                    for i in range(max(0, line_no - 11), min(len(lines), line_no + 10))
                )
                
                # Check for guard in ±3 lines
                has_guard = self._context_window_search(
                    lines, line_no,
                    [r'lock_guard|unique_lock|scoped_lock'],
                    window=3
                )
                
                if has_unlock and not has_guard:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="explicit_lock_unlock",
                        severity="HIGH",
                        confidence=0.75,
                        description="Explicit lock/unlock without RAII guard",
                        remediation="Use std::lock_guard or std::unique_lock",
                        context=line.strip(),
                        scanner=self.name,
                        step="01_thread_safety"
                    ))
            
            # Pattern 2: Double lock (no unlock between locks)
            if re.search(r'\.lock\()', line):
                for i in range(line_no, min(len(lines), line_no + 8)):
                    if i != line_no - 1 and re.search(r'\.lock\()', lines[i]):
                        # Check if there's an unlock between
                        has_unlock = any(
                            re.search(r'\.unlock\()', lines[j])
                            for j in range(line_no - 1, i)
                        )
                        if not has_unlock:
                            gaps.append(Gap(
                                file=str(file_path.relative_to(self.source_path)),
                                line=line_no,
                                type="double_lock",
                                severity="CRITICAL",
                                confidence=0.70,
                                description="Double lock without unlock (potential deadlock)",
                                remediation="Add unlock before second lock or use single guard",
                                context=lines[i].strip(),
                                scanner=self.name,
                                step="01_thread_safety"
                            ))
                            break
        
        return gaps
    
    def _check_thread_join_no_timeout(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect thread join/wait without timeout."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\.join\(\)|\.wait\(\)', line):
                # Check for timeout in ±3 lines
                has_timeout = self._context_window_search(
                    lines, line_no,
                    [r'timeout|wait_for|deadline|expires_after|milliseconds|seconds'],
                    window=3
                )
                
                if not has_timeout:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="thread_join_no_timeout",
                        severity="CRITICAL",
                        confidence=0.80,
                        description="Thread join/wait without timeout (blocking indefinitely)",
                        remediation="Add timeout: thread.join(timeout_duration) or wait_for(...)",
                        context=line.strip(),
                        scanner=self.name,
                        step="01_thread_safety"
                    ))
        
        return gaps
    
    def _check_circular_lock_ordering(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect potential circular lock ordering (deadlock pattern)."""
        gaps = []
        
        # Pattern: mtx1.lock() ... mtx2.lock() in one path
        #          mtx2.lock() ... mtx1.lock() in another
        # This is a heuristic: same file, reverse lock order within functions
        
        lock_patterns = {}
        current_func = None
        
        for line_no, line in enumerate(lines, 1):
            # Track function scope (simple heuristic)
            if re.search(r'^\s*(?:void|int|bool|auto)\s+\w+\s*\(', line):
                current_func = line.split('(')[0].strip()
            
            # Find locks
            locks = re.findall(r'(\w+)\s*(?:\.|\->)\s*lock\(\)', line)
            if locks and current_func:
                if current_func not in lock_patterns:
                    lock_patterns[current_func] = []
                lock_patterns[current_func].extend(locks)
        
        # Check for circular patterns in single file
        # (Real check would need cross-function analysis)
        for func, locks_in_func in lock_patterns.items():
            if len(locks_in_func) >= 2:
                # Check if same locks appear in reverse order elsewhere
                for other_func, other_locks in lock_patterns.items():
                    if other_func != func and len(other_locks) >= 2:
                        if locks_in_func[:2] == other_locks[:2][::-1]:
                            # Potential deadlock pattern
                            gaps.append(Gap(
                                file=str(file_path.relative_to(self.source_path)),
                                line=1,  # File-level finding
                                type="circular_lock_ordering",
                                severity="MEDIUM",
                                confidence=0.55,
                                description="Potential circular lock ordering (deadlock risk)",
                                remediation="Use consistent lock ordering across functions or std::lock",
                                context=f"Function '{func}' vs '{other_func}'",
                                scanner=self.name,
                                step="01_thread_safety"
                            ))
        
        return gaps


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_dir> [--json]")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    json_output = '--json' in sys.argv
    
    scanner = ThreadSafetyScanner()
    print(f"[{scanner.name}] Starting scan...\n")
    
    gaps = scanner.scan(source_dir)
    
    # Organize by severity
    by_severity = {}
    by_type = {}
    for gap in gaps:
        by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1
        by_type[gap.type] = by_type.get(gap.type, 0) + 1
    
    print(f"\nFound {len(gaps)} thread safety gaps in {scanner.files_scanned:.0f}s")
    print(f"Scanned {scanner.files_scanned} files")
    
    if by_severity:
        print(f"\n  {', '.join(f'{sev}: {count}' for sev, count in sorted(by_severity.items()))}")
    
    if by_type:
        print(f"\nBy Type:")
        for typ, count in sorted(by_type.items(), key=lambda x: -x[1]):
            print(f"  {typ}: {count}")
    
    if json_output:
        import json
        print("\n" + json.dumps([gap.to_dict() for gap in gaps], indent=2))
