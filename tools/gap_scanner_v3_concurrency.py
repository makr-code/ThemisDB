#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Concurrency Gaps Detection

Detects:
- Data races (unprotected shared data access)
- Lock ordering violations / deadlock risks
- Missing mutex/lock guards
- Callback/async race conditions
- Thread-unsafe singleton access
- Memory ordering issues
- Double-lock protection (mutex twice)
- TOCTOU races (Time-of-Check-Time-of-Use without lock) — CWE-362
- Double-Checked Locking anti-pattern — CWE-362
- Lost Wakeup in Condition Variables — CWE-362

Phase 1-4 Scanner Enhancements: +3 race condition patterns with CWE-362 mapping
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class ConcurrencyGapType(Enum):
    """Concurrency gap classifications"""
    DATA_RACE = "data_race"                       # Unprotected shared access
    DEADLOCK_RISK = "deadlock_risk"               # Lock ordering issue
    MISSING_LOCK = "missing_lock"                 # No mutex guard
    RACE_CONDITION = "race_condition"             # Callback/async race
    UNSAFE_SINGLETON = "unsafe_singleton"         # Unprotected singleton access
    MEMORY_ORDER = "memory_order"                 # std::memory_order issue
    DOUBLE_LOCK = "double_lock"                   # Locked twice (deadlock)
    CONDITION_RACE = "condition_race"             # Condition var without lock
    TOCTOU_RACE = "toctou_race"                   # Time-of-check-time-of-use (CWE-362)
    DOUBLE_CHECKED_LOCKING = "double_checked_locking"  # DCL anti-pattern (CWE-362)
    LOST_WAKEUP = "lost_wakeup"                   # Condition variable lost wakeup (CWE-362)


@dataclass
class ConcurrencyGap:
    """Represents a concurrency gap"""
    file_path: str
    line_num: int
    gap_type: ConcurrencyGapType
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


class ConcurrencyGapScanner:
    """Detect concurrency issues in C++ code"""
    
    PATTERNS = {
        'lock_guard': re.compile(r'std::lock_guard|std::unique_lock'),
        'mutex': re.compile(r'std::mutex|std::shared_mutex'),
        'atomic': re.compile(r'std::atomic'),
        'thread': re.compile(r'std::thread|\.detach\(\)|\.join\(\)'),
        'condition': re.compile(r'std::condition_variable|\.wait\(|\.notify'),
        'mutable': re.compile(r'\bmutable\s+'),
    }
    
    SHARED_DATA_KEYWORDS = [
        'static', 'global_', '_g_', 'g_', 'shared', 'cache', 'registry',
        'map_', '_map', 'set_', '_set', 'queue_', '_queue', 'pool_'
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[ConcurrencyGap]] = {}

    def _has_lock_context(self, lines: List[str], line_idx: int) -> bool:
        """Best-effort lock scope detection around a line index."""
        start = max(0, line_idx - 60)
        end = min(len(lines), line_idx + 6)
        context = ''.join(lines[start:end])
        return bool(re.search(r'std::(lock_guard|unique_lock|scoped_lock|shared_lock)\b', context))

    def _collect_lock_targets_in_block(self, lines: List[str], start_idx: int) -> List[str]:
        """Collect lock targets in the current lexical block only."""
        # Determine current scope depth at start_idx.
        scope_depth = 0
        for i in range(0, max(0, start_idx)):
            scope_depth += lines[i].count('{') - lines[i].count('}')

        targets: List[str] = []
        nested_lock_hit = False
        current_depth = scope_depth
        end_idx = min(len(lines), start_idx + 120)

        for i in range(start_idx, end_idx):
            l = lines[i]
            m = re.search(r'std::(?:lock_guard|unique_lock|scoped_lock|shared_lock)\s*<[^>]*>\s*\w*\s*\(\s*([A-Za-z_]\w*)', l)
            if m:
                targets.append(m.group(1))
                if len(set(targets)) > 1 and current_depth > scope_depth:
                    nested_lock_hit = True

            current_depth += l.count('{') - l.count('}')
            # Stop when we leave the original scope.
            if i > start_idx and current_depth < scope_depth:
                break

        if not nested_lock_hit:
            return []
        return targets
    
    def _detect_toctou_race(self, lines: List[str], line_idx: int) -> bool:
        """Detect TOCTOU race: if (condition_check()) { operation() } without lock.
        
        Pattern matches:
        - if (file_exists(path)) { process_file(path); }
        - if (cache.find(key) != end) { use_value(cache[key]); }
        - if (ptr != nullptr) { ptr->method(); } // without lock before check
        """
        if line_idx >= len(lines):
            return False
        
        line = lines[line_idx]
        
        # Check for typical TOCTOU patterns:
        # 1. File existence check
        if_file_pattern = r'if\s*\(\s*(?:fs::|std::)?(?:exists|is_file|file_exists|fopen|stat|access)\s*\('
        # 2. Container find check
        if_find_pattern = r'if\s*\(\s*(?:[\w:]+\.)?find\s*\(\s*[\w\.]+\s*\)\s*(?:!=|==)\s*(?:end|\.end)\(\)\s*\)'
        # 3. Pointer null check without preceding lock
        if_ptr_pattern = r'if\s*\(\s*[\w\.]+\s*(?:!=|==)\s*(?:nullptr|NULL|0)\s*\)'
        
        is_toctou_check = bool(re.search(if_file_pattern, line) or 
                               re.search(if_find_pattern, line) or 
                               re.search(if_ptr_pattern, line))
        
        if not is_toctou_check:
            return False
        
        # Now check if there's a lock guard immediately before
        prev_context = ''.join(lines[max(0, line_idx-3):line_idx])
        
        # If we see a lock_guard or unique_lock right before, it's protected
        if re.search(r'std::(?:lock_guard|unique_lock|scoped_lock)\s*<', prev_context):
            return False
        
        # Check within the if block (next few lines) for the operation
        next_context = ''.join(lines[line_idx:min(len(lines), line_idx+5)])
        
        # If there's a lock right after the if, it's better practice
        if re.search(r'if\s*\([^)]*\)\s*\{\s*std::(?:lock_guard|unique_lock)', next_context):
            # This is still TOCTOU but less severe since lock is held for operation
            return True
        
        # Full TOCTOU: check without lock, then use without lock
        use_patterns = [
            r'\.read\(',
            r'\.write\(',
            r'\[\s*[\w\.]+\s*\]',  # array/map access
            r'(?:[\w:]+\.)?(?:process|handle|use|access|update|modify)\s*\(',
            r'->\s*(?:\w+|operator)',  # pointer dereference
        ]
        
        has_operation = any(re.search(pat, next_context) for pat in use_patterns)
        return has_operation
    
    def _detect_double_checked_locking(self, lines: List[str], line_idx: int) -> bool:
        """Detect double-checked locking (DCL) anti-pattern.
        
        Pattern: if (!initialized) { lock_guard l(m); if (!initialized) { init(); } }
        
        This is problematic because:
        - First check has no synchronization
        - Between first and second check, another thread could initialize
        - Memory ordering is not guaranteed without acquire/release semantics
        """
        if line_idx >= len(lines):
            return False
        
        line = lines[line_idx].strip()
        
        # First check: look for if (!var) or if (var == false/null)
        first_check_pattern = r'if\s*\(\s*![\w\.]+\s*\)' if '!' in line else r'if\s*\(\s*[\w\.]+\s*==\s*(?:false|nullptr|NULL|0)\s*\)'
        
        if not re.search(first_check_pattern, line) and not re.search(r'if\s*\(\s*!', line):
            return False
        
        # Look ahead for lock_guard and nested if with same condition
        next_lines = ''.join(lines[line_idx:min(len(lines), line_idx+10)])
        
        # Must have a lock guard and then another condition check
        has_lock = bool(re.search(r'(?:std::)?(?:lock_guard|unique_lock|scoped_lock)\s*<', next_lines))
        
        # Extract variable name from first check to look for in second check
        var_match = re.search(r'![\w\.]+', line)
        if not var_match:
            var_match = re.search(r'[\w\.]+\s*==\s*(?:false|nullptr|NULL|0)', line)
        
        if var_match and has_lock:
            # Look for nested if with similar pattern
            nested_if_pattern = r'if\s*\(\s*![\w\.]+\s*\)|if\s*\(\s*[\w\.]+\s*==\s*(?:false|nullptr|NULL)'
            has_nested_check = bool(re.search(nested_if_pattern, next_lines))
            
            return has_nested_check
        
        return False
    
    def _detect_lost_wakeup(self, lines: List[str], line_idx: int) -> bool:
        """Detect lost wakeup pattern in condition variables.
        
        Pattern: cv.wait() without holding unique_lock, or condition variable accessed
        without synchronized notification/wait pattern.
        
        Issues:
        - wait() without lock held
        - spurious wakeup without proper condition check
        - notify without lock held
        """
        if line_idx >= len(lines):
            return False
        
        line = lines[line_idx].strip()
        
        # Check for condition variable operations
        cv_ops = r'(?:\.wait\(|\.wait_for\(|\.wait_until\(|\.notify_one\(|\.notify_all\()'
        
        if not re.search(cv_ops, line):
            return False
        
        # For wait operations, check if lock is held
        if '.wait' in line:
            # Should be called with unique_lock as parameter
            if '.wait(' in line:
                # Pattern: cv.wait(lock) or cv.wait(lock, predicate)
                # If no parameter or wrong parameter type, it's lost wakeup
                wait_match = re.search(r'\.wait\(\s*([^)]*)\)', line)
                if wait_match:
                    param = wait_match.group(1).strip()
                    
                    # Check if parameter looks like a valid lock
                    if not param or param == '':
                        return True  # Lost wakeup: no lock parameter
                    
                    # If parameter contains a lambda or function, it's likely correct
                    # (predicate-based wait with lock)
                    if '[' in param or '->' in param or '()' in param:
                        return False
                    
                    # If parameter doesn't look like a lock variable, might be lost wakeup
                    if not re.search(r'(?:lock|guard|l(?:ock)?|mutex|m)\b', param, re.IGNORECASE):
                        return True
                else:
                    return True
            
            # Also check context for lock before wait
            prev_context = ''.join(lines[max(0, line_idx-5):line_idx])
            
            # If we see a lock_guard but it's for a different variable, lost wakeup
            lock_match = re.search(r'std::(?:lock_guard|unique_lock)\s*<[^>]*>\s*(?:lock|l|guard)\s*\(\s*(\w+)', prev_context)
            if lock_match:
                locked_var = lock_match.group(1)
                # Check if the wait is using a condition variable without the right lock
                cv_match = re.search(r'(\w+)\.wait', line)
                if cv_match:
                    cv_name = cv_match.group(1)
                    # If condition var exists but lock variable isn't in wait(), lost wakeup
                    if locked_var != cv_name and not re.search(rf'\({locked_var}\)', line):
                        return True
        
        # For notify operations, check if notification happens without lock held
        elif '.notify' in line:
            # notify_one/notify_all should ideally be called while holding the lock
            # Check previous context for lock
            prev_context = ''.join(lines[max(0, line_idx-3):line_idx])
            
            if 'lock_guard' not in prev_context and 'unique_lock' not in prev_context:
                # Notify without holding lock — potential lost wakeup for waiting threads
                return True
        
        return False
    
    def scan_file(self, file_path: Path) -> List[ConcurrencyGap]:
        """Scan single file for concurrency gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            # Skip comments and test code
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue
            if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name:
                continue
            
            # Check for shared data without lock
            for keyword in self.SHARED_DATA_KEYWORDS:
                if keyword in line and '=' in line:
                    # Found potential shared data access
                    # Check broader context to catch lock scopes declared earlier.
                    has_lock = self._has_lock_context(lines, line_num - 1)
                    
                    if not has_lock and '->' in line:
                        gap = ConcurrencyGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ConcurrencyGapType.DATA_RACE,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'Shared data access without lock protection',
                            remediation='Protect shared data with std::lock_guard or std::unique_lock'
                        )
                        gaps.append(gap)
            
            # Check for missing lock guards around mutex
            if re.search(r'mutex\s*\(', line) or re.search(r'\.lock\(\)', line):
                # Found mutex usage
                next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                
                # Check if unlock is present and balanced
                lock_count = next_context.count('.lock()')
                unlock_count = next_context.count('.unlock()')
                
                if lock_count > 0 and unlock_count == 0:
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.MISSING_LOCK,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Raw .lock() without corresponding .unlock() — use RAII instead',
                        remediation='Replace with std::lock_guard<std::mutex> guard(mutex);'
                    )
                    gaps.append(gap)
            
            # Check for potential deadlock (nested locking)
            if 'lock_guard' in line or 'unique_lock' in line:
                # Check only within current lexical block to avoid cross-function FPs.
                lock_targets = self._collect_lock_targets_in_block(lines, line_num - 1)
                unique_targets = {t for t in lock_targets if t}

                if len(unique_targets) > 1:
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.DEADLOCK_RISK,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Multiple locks acquired in nested scope — potential deadlock',
                        remediation='Use std::scoped_lock(m1, m2) or enforce consistent lock ordering'
                    )
                    gaps.append(gap)
            
            # Check for condition variable without lock
            if 'condition_variable' in line:
                next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                
                if '.wait(' in next_context or '.notify' in next_context:
                    if 'lock_guard' not in next_context and 'unique_lock' not in next_context:
                        gap = ConcurrencyGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ConcurrencyGapType.CONDITION_RACE,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description='Condition variable used without holding lock',
                            remediation='Condition var operations require locked std::unique_lock'
                        )
                        gaps.append(gap)
            
            # Check for unsafe singleton access
            if 'static' in line and 'getInstance' in line:
                next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                prev_context = ''.join(lines[max(0, line_num-3):line_num])
                
                if 'lock_guard' not in next_context and 'lock_guard' not in prev_context:
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.UNSAFE_SINGLETON,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Singleton access without thread-safety mechanism',
                        remediation='Protect with std::lock_guard or use Meyer singleton pattern'
                    )
                    gaps.append(gap)
            
            # Check for memory ordering issues
            if 'std::memory_order' in line:
                if 'memory_order_relaxed' in line and 'atomic' in ''.join(lines[max(0, line_num-3):line_num]):
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.MEMORY_ORDER,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='memory_order_relaxed used — potential visibility issue',
                        remediation='Use memory_order_acquire/release unless truly lock-free'
                    )
                    gaps.append(gap)
            
            # Check for double-lock pattern (same mutex twice)
            if re.search(r'(\w+).*\.lock\(\)', line):
                var_match = re.search(r'(\w+).*\.lock\(\)', line)
                if var_match:
                    mutex_name = var_match.group(1)
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                    
                    if f'{mutex_name}.lock()' in next_context:
                        gap = ConcurrencyGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ConcurrencyGapType.DOUBLE_LOCK,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'Potential double-lock on same mutex: {mutex_name}',
                            remediation='Ensure proper lock nesting or use recursive_mutex if needed'
                        )
                        gaps.append(gap)
            
            # C-1: Pattern 1 - TOCTOU (Time-of-Check-Time-of-Use) Race Detection
            if 'if' in line and '(' in line:
                if self._detect_toctou_race(lines, line_num - 1):
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.TOCTOU_RACE,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='TOCTOU race condition: resource checked then used without lock (CWE-362)',
                        remediation='Acquire lock BEFORE the if-check, hold through operation. Use atomic or lock_guard.'
                    )
                    gaps.append(gap)
            
            # C-1: Pattern 2 - Double-Checked Locking Anti-Pattern Detection
            if 'if' in line and '!' in line and '(' in line:
                if self._detect_double_checked_locking(lines, line_num - 1):
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.DOUBLE_CHECKED_LOCKING,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Double-checked locking anti-pattern detected (CWE-362). First check lacks synchronization.',
                        remediation='Use atomic<bool> with acquire/release semantics, Meyer singleton, or call_once pattern instead.'
                    )
                    gaps.append(gap)
            
            # C-1: Pattern 3 - Lost Wakeup in Condition Variables Detection
            if 'wait' in line or 'notify' in line:
                if self._detect_lost_wakeup(lines, line_num - 1):
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.LOST_WAKEUP,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Lost wakeup pattern: condition variable operation without proper lock or predicate (CWE-362)',
                        remediation='Always use cv.wait(lock, predicate) or ensure lock held during notify. Add spurious wakeup guard.'
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[ConcurrencyGap]]:
        """Scan module for concurrency gaps"""
        gaps_by_file = {}
        
        src_dir = self.repo_root / 'src' / module
        include_dir = self.repo_root / 'include' / module
        
        for directory in [src_dir, include_dir]:
            if not directory.exists():
                continue
            
            cpp_files = list(directory.rglob('*.cpp'))
            hpp_files = list(directory.rglob('*.hpp'))
            for file_path in cpp_files + hpp_files:
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path.relative_to(self.repo_root))] = gaps
        
        return gaps_by_file
    
    def run_full_scan(self, output_dir: str = 'ai_working') -> Dict[str, any]:
        """Scan all modules for concurrency gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for CONCURRENCY GAPS...")
        
        src_root = self.repo_root / 'src'
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            
            if total_gaps > 0:
                print(f"   {module:30} {total_gaps:4} gaps")
                
                gap_counts = {}
                for gaps in gaps_by_file.values():
                    for gap in gaps:
                        gap_type = gap.gap_type.value
                        gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1
                
                severity_counts = {
                    'critical': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'CRITICAL'),
                    'high': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'HIGH'),
                    'medium': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'MEDIUM'),
                }
                
                aggregate[module] = {
                    'total': total_gaps,
                    'gaps_by_file': {f: [g.to_dict() for g in gaps] for f, gaps in gaps_by_file.items()},
                    'gap_types': gap_counts,
                    'severity_critical': severity_counts['critical'],
                    'severity_high': severity_counts['high'],
                    'severity_medium': severity_counts['medium'],
                }
        
        return aggregate
