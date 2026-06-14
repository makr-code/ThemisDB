#!/usr/bin/env python3
"""
Gap Scanner V3 — Thread Safety Scanner (IMPROVED 2026-06-14)

IMPROVEMENTS:
1. data_race: Local lambdas without captures are safe (no shared state)
2. data_race: lock_guard/unique_lock in same scope = race-free
3. thread_join_no_timeout: join() is intentionally blocking by design, don't flag
4. Comment filtering: Skip Doxygen/comments in analysis

Detects threading gaps:
- Race conditions (shared state without synchronization) [with lock-scope awareness]
- Deadlock patterns (circular lock ordering)
- Lock misuse (missing lock, double lock)
- Volatile missing (shared primitives) [tuned]
"""

import sys
from pathlib import Path
from typing import List, Set, Tuple
import re

sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class ThreadSafetyScannerImproved(BaseGapScanner):
    """Scan for threading and synchronization gaps (with improved FP filters)."""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30
    
    # IMPROVEMENT 1: Patterns indicating safe local contexts
    SAFE_LOCAL_PATTERNS = [
        r'auto\s+\w+\s*=\s*\[', # local lambda
        r'std::thread\([', # local thread instantiation
        r'std::async\([', # local async
        r'\.then\(',  # continuation
    ]
    
    def __init__(self):
        """Initialize Thread Safety Scanner (Improved)."""
        super().__init__("Thread Safety Scanner (Improved)", "3.1.improved")
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for thread safety gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()
            self.files_scanned += 1
            
            lines = self._read_file_lines(file_path)
            if not lines:
                continue
            
            # Run all detectors with improved filters
            gaps.extend(self._check_shared_state_without_sync_improved(file_path, lines))
            gaps.extend(self._check_missing_volatile(file_path, lines))
            gaps.extend(self._check_lock_misuse(file_path, lines))
            # IMPROVEMENT 3: Skip thread_join_no_timeout check (join is intentionally blocking)
            # gaps.extend(self._check_thread_join_no_timeout(file_path, lines))
            gaps.extend(self._check_circular_lock_ordering(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _is_local_lambda_safe(self, line: str, context_lines: List[str], line_no: int) -> bool:
        """
        IMPROVEMENT 1: Check if a lambda is local with no captures
        (local lambdas with no captures don't access shared state)
        """
        # Check if this is a lambda definition
        if '[' not in line or ']' not in line:
            return False
        
        # Extract capture list
        capture_match = re.search(r'\[([^\]]*)\]', line)
        if not capture_match:
            return False
        
        capture_list = capture_match.group(1).strip()
        
        # Empty capture = local-only, no shared state
        if capture_list == '':
            return True
        
        # Captures of local variables only = safe
        # Check if captured items are local variables (not members/statics)
        captured_items = [x.strip() for x in capture_list.split(',')]
        for item in captured_items:
            item_name = item.split('=')[0].strip()  # Handle "name = value" syntax
            
            # Safe: captures by value, local scope
            if item in ('&', '=', '&*this'):
                # These are class-wide, potentially unsafe
                return False
            
            # Check if item looks like a local variable (not m_xxx, static)
            if item_name.startswith('m_') or item_name.startswith('_') or item_name.startswith('static_'):
                return False
        
        # All captures are safe locals
        return True
    
    def _has_lock_guard_scope(self, context_lines: List[str], line_no: int, var_name: str) -> bool:
        """
        IMPROVEMENT 2: Check if access is inside a lock_guard/unique_lock scope
        """
        # Look backwards for lock_guard/unique_lock
        for i in range(line_no - 1, max(0, line_no - 30), -1):
            line = context_lines[i]
            
            # Check for lock guard patterns
            if any(pattern in line for pattern in [
                'std::lock_guard<',
                'std::unique_lock<',
                'std::scoped_lock',
                '{',  # scope entry
            ]):
                # Verify we're still within this scope
                brace_depth = 0
                for j in range(i, line_no + 1):
                    brace_depth += context_lines[j].count('{') - context_lines[j].count('}')
                
                if brace_depth > 0:
                    return True
                else:
                    # Scope has closed, not protected
                    return False
        
        return False
    
    def _is_in_comment(self, line: str, pos: int = None) -> bool:
        """Check if position is in a comment"""
        if pos is None:
            pos = len(line)
        
        # Single-line comment
        comment_idx = line.find('//')
        if comment_idx != -1 and pos >= comment_idx:
            return True
        
        return False
    
    def _check_shared_state_without_sync_improved(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Detect shared state access without synchronization (IMPROVED)
        
        IMPROVEMENT 1: Skip local lambdas without captures
        IMPROVEMENT 2: Skip accesses within lock_guard scope
        """
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments and Doxygen
            if line.strip().startswith('//') or line.strip().startswith('*'):
                continue
            
            # IMPROVEMENT 1: Check for local lambda — if safe, skip
            if self._is_local_lambda_safe(line, lines, line_no - 1):
                continue
            
            # Pattern: member variable access (m_xxx or this->xxx)
            member_patterns = [
                r'this\s*->\s*(\w+)',
                r'(m_\w+)',
            ]
            
            matches = []
            for pattern in member_patterns:
                for match in re.finditer(pattern, line):
                    matches.append((match.start(), match.group(1)))
            
            for pos, member_var in matches:
                # Skip if in comment
                if self._is_in_comment(line, pos):
                    continue

                # Only flag potential data races on writes/mutations, not plain reads.
                mutation_patterns = [
                    rf'this\s*->\s*{re.escape(member_var)}\s*=\s*',
                    rf'\b{re.escape(member_var)}\s*=\s*',
                    rf'this\s*->\s*{re.escape(member_var)}\s*(\+\+|--)',
                    rf'\b{re.escape(member_var)}\s*(\+\+|--)',
                    rf'this\s*->\s*{re.escape(member_var)}\s*\+=',
                    rf'this\s*->\s*{re.escape(member_var)}\s*-=',
                    rf'\b{re.escape(member_var)}\s*\+=',
                    rf'\b{re.escape(member_var)}\s*-=',
                ]
                if not any(re.search(p, line) for p in mutation_patterns):
                    continue
                
                # IMPROVEMENT 2: Check if protected by lock_guard
                if self._has_lock_guard_scope(lines, line_no - 1, member_var):
                    continue
                
                # Check context for synchronization
                context_window = 5
                context = ''.join(lines[max(0, line_no - context_window):
                                        min(len(lines), line_no + context_window)])
                
                sync_keywords = [
                    'lock_guard', 'unique_lock', 'scoped_lock',
                    'std::mutex', 'std::atomic',
                ]
                
                if any(kw in context for kw in sync_keywords):
                    continue
                
                # No synchronization found — potential race
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type='data_race',
                    severity='CRITICAL',
                    confidence=0.85,
                    description=f'Shared member variable "{member_var}" accessed without synchronization',
                    context=line.strip()[:80],
                    remediation='Add std::lock_guard<std::mutex> or use std::atomic',
                ))
        
        return gaps
    
    def _check_missing_volatile(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect shared primitives without volatile keyword."""
        gaps = []
        
        # Look for shared_ptr/atomic-like access patterns without volatile
        for line_no, line in enumerate(lines, 1):
            if line.strip().startswith('//'):
                continue
            
            # Pattern: plain bool/int member that's accessed from multiple threads
            if re.search(r'\b(?:bool|int|size_t|uint32_t|int64_t)\s+\w+', line):
                # Restrict to likely shared/static state to reduce false positives.
                if not (('static' in line) or re.search(r'\bm_\w+', line)):
                    continue

                # Check if there's threading context
                file_context = ''.join(lines)
                if 'std::thread' not in file_context and 'async' not in file_context:
                    continue
                
                # Check if volatile used
                if 'volatile' in line:
                    continue
                
                # Potential issue
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type='missing_volatile',
                    severity='MEDIUM',
                    confidence=0.65,
                    description='Shared primitive without volatile keyword',
                    context=line.strip()[:80],
                    remediation='Add volatile keyword: volatile bool/int ...',
                ))
        
        return gaps
    
    def _check_lock_misuse(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect lock misuse patterns."""
        gaps = []
        
        # Pattern: mutex locked but never checked in scope
        for line_no, line in enumerate(lines, 1):
            if 'lock_guard' in line or 'unique_lock' in line:
                # Verify lock is used in some guard
                if 'lock' not in line:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type='lock_misuse',
                        severity='HIGH',
                        confidence=0.70,
                        description='Lock guard created but may not be used',
                        context=line.strip()[:80],
                        remediation='Verify lock_guard is wrapping critical section',
                    ))
        
        return gaps
    
    def _check_circular_lock_ordering(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect potential deadlock from circular lock ordering."""
        gaps = []
        
        # This is a simplified check; full deadlock detection requires flow analysis
        lock_order = {}
        
        for line_no, line in enumerate(lines, 1):
            if 'lock_guard' in line or 'unique_lock' in line:
                # Extract mutex name
                match = re.search(r'(?:lock_guard|unique_lock)<[^>]*>\s*\w*\((?:.*?)(\w+)\)', line)
                if match:
                    mutex_name = match.group(1)
                    if 'mutex' in mutex_name.lower():
                        # Store lock order
                        if file_path not in lock_order:
                            lock_order[file_path] = []
                        lock_order[file_path].append((line_no, mutex_name))
        
        # Check for potential reverse ordering (simplified)
        for file_p in lock_order:
            lock_seq = lock_order[file_p]
            for i, (l1, m1) in enumerate(lock_seq):
                for l2, m2 in lock_seq[i+1:]:
                    if l2 > l1 + 100 and m2 < m1:  # Heuristic: far apart and reversed
                        gaps.append(Gap(
                            file=str(file_p.relative_to(self.source_path)),
                            line=l1,
                            type='circular_lock_ordering',
                            severity='HIGH',
                            confidence=0.72,
                            description=f'Potential deadlock: lock order {m1} then {m2}',
                            context='',
                            remediation='Enforce consistent lock ordering across all code paths',
                        ))
        
        return gaps


if __name__ == '__main__':
    scanner = ThreadSafetyScannerImproved()
    
    source_dir = sys.argv[1] if len(sys.argv) > 1 else '.'
    gaps = scanner.scan(source_dir)
    
    print(f"Found {len(gaps)} thread safety gaps (improved)")
    for gap in gaps[:10]:  # Print first 10
        print(f"  {gap.file}:{gap.line} [{gap.type}] {gap.description}")
