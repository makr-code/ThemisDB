#!/usr/bin/env python3
"""
Phase 1-4 Enhancement: C-1 Race Condition Detection (Enhanced)

CWE-362: Concurrent Execution using Shared Resource with Improper Synchronization

Enhanced Patterns:
1. Missing synchronization on shared data
2. Lock ordering issues (potential deadlocks)
3. Double-checked locking patterns
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Set
from enum import Enum


class C1ConcurrencyGapType(Enum):
    """C-1 Concurrency gap categories"""
    UNSYNCHRONIZED_SHARED_DATA = "unsynchronized_shared_data"
    LOCK_ORDERING_ISSUE = "lock_ordering_issue"
    DOUBLE_CHECKED_LOCKING = "double_checked_locking"
    RACE_CONDITION_GLOBAL = "race_condition_global"


@dataclass
class C1ConcurrencyGap:
    """Represents a C-1 concurrency/race condition gap"""
    file_path: str
    line_num: int
    gap_type: C1ConcurrencyGapType
    snippet: str
    severity: str  # CRITICAL, HIGH
    description: str
    remediation: str
    pattern_name: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
            'pattern': self.pattern_name,
            'enhancement': 'C-1',
            'cwe': 'CWE-362',
        }


class C1ConcurrencyScanner:
    """C-1: Race Condition Detection"""
    
    # Pattern 1: Unsynchronized Shared Data Access
    UNSYNC_SHARED_PATTERNS = {
        'global_modified_no_lock': (
            re.compile(r'^static\s+(?:int|bool|double|char|long|short|float)\s+\w+\s*[;=]', re.MULTILINE),
            'HIGH',
            'Static/global variable without synchronization — potential data race'
        ),
        'member_without_mutable_or_lock': (
            re.compile(r'mutable\s+\w+\s+\w+\s*;(?!.*(?:mutex|lock|atomic))', re.MULTILINE),
            'HIGH',
            'Mutable member without synchronization primitive'
        ),
        'atomic_missing': (
            re.compile(r'(?:counter|flag|state|ready|done)\s*(?:\+\+|--|\s*[=]\s*[!])', re.IGNORECASE),
            'HIGH',
            'Shared flag/counter without std::atomic — data race possible'
        ),
        'read_write_no_sync': (
            re.compile(r'(?:pGlobal|g_\w+|s_\w+|shared_\w+)\s*(?:\+\+|--|\s*=)(?!.*(?:lock|atomic))', re.IGNORECASE),
            'HIGH',
            'Global/shared data modified without synchronization'
        ),
    }
    
    # Pattern 2: Lock Ordering Issues
    LOCK_ORDERING_PATTERNS = {
        'lock_unlock_mismatch': (
            re.compile(r'(\w+)->lock\s*\(\);.*?(\w+)->unlock\s*\(\);', re.MULTILINE),
            'CRITICAL',
            'Lock/unlock on different objects — potential deadlock'
        ),
        'multiple_locks_no_order': (
            re.compile(r'(?:mutex1|mutex2|lock1|lock2)\s*\.lock\s*\(\).*?\n.*?(?:mutex1|mutex2|lock1|lock2)\s*\.lock\s*\(\)', re.MULTILINE),
            'HIGH',
            'Multiple locks acquired without consistent ordering'
        ),
        'lock_guard_multiple': (
            re.compile(r'std::lock_guard.*?mutex_?a.*?std::lock_guard.*?mutex_?b', re.MULTILINE),
            'HIGH',
            'Multiple lock_guards on different mutexes (check lock ordering)'
        ),
        'nested_lock_without_defer': (
            re.compile(r'\.lock\s*\(\).*?\.lock\s*\(\)(?!.*std::lock|defer_lock)', re.MULTILINE),
            'HIGH',
            'Nested lock() without deadlock prevention mechanism'
        ),
    }
    
    # Pattern 3: Double-Checked Locking (Broken Pattern in C++)
    DOUBLE_CHECKED_LOCKING_PATTERNS = {
        'dcl_pattern': (
            re.compile(r'if\s*\(\s*!\w+\s*\)\s*{.*?{.*?lock.*?if\s*\(\s*!\w+\s*\)', re.MULTILINE | re.DOTALL),
            'CRITICAL',
            'Double-checked locking pattern — prone to data race (use std::call_once or lazy statics)'
        ),
        'singleton_unsafe': (
            re.compile(r'static\s+\w+\*\s+getInstance\s*\(\)\s*{.*?if\s*\(\s*!.*instance.*?\)\s*{', re.MULTILINE | re.DOTALL),
            'CRITICAL',
            'Unsafe singleton pattern — potential data race (use std::call_once or C++11 static initialization)'
        ),
        'volatile_check_no_lock': (
            re.compile(r'volatile.*?if\s*\([^)]*\)\s*{(?!.*(?:lock|atomic))', re.MULTILINE),
            'HIGH',
            'Volatile flag check without synchronization — not thread-safe'
        ),
    }
    
    # Pattern 4: Race Conditions on Global State
    GLOBAL_RACE_PATTERNS = {
        'thread_local_missing': (
            re.compile(r'static\s+(?:std::vector|std::map|std::set|std::string)\s+\w+\s*[;=](?!.*thread_local)', re.MULTILINE),
            'HIGH',
            'Static container without thread_local or synchronization'
        ),
        'cache_invalidation': (
            re.compile(r'(?:cache|buffer|buffer_data|cached_value)\s*[=]\s*.*?;(?!.*(?:lock|atomic|thread_local))', re.MULTILINE),
            'HIGH',
            'Cache/buffer modification without synchronization — possible stale reads'
        ),
        'callbacks_unsynced': (
            re.compile(r'std::function.*callback.*=.*\n.*?callback\s*\(', re.MULTILINE),
            'HIGH',
            'Callback invocation without synchronization — race condition possible'
        ),
    }
    
    def __init__(self):
        self.gaps: List[C1ConcurrencyGap] = []
    
    def scan_file(self, file_path: str) -> List[C1ConcurrencyGap]:
        """Scan a single file for C-1 race conditions"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            return gaps
        
        # Scan for unsynchronized shared data
        gaps.extend(self._scan_patterns(
            content, file_path, self.UNSYNC_SHARED_PATTERNS,
            C1ConcurrencyGapType.UNSYNCHRONIZED_SHARED_DATA
        ))
        
        # Scan for lock ordering issues
        gaps.extend(self._scan_patterns(
            content, file_path, self.LOCK_ORDERING_PATTERNS,
            C1ConcurrencyGapType.LOCK_ORDERING_ISSUE
        ))
        
        # Scan for double-checked locking
        gaps.extend(self._scan_patterns(
            content, file_path, self.DOUBLE_CHECKED_LOCKING_PATTERNS,
            C1ConcurrencyGapType.DOUBLE_CHECKED_LOCKING
        ))
        
        # Scan for global race conditions
        gaps.extend(self._scan_patterns(
            content, file_path, self.GLOBAL_RACE_PATTERNS,
            C1ConcurrencyGapType.RACE_CONDITION_GLOBAL
        ))
        
        return gaps
    
    def _scan_patterns(self, content: str, file_path: str,
                       patterns: Dict, gap_type: C1ConcurrencyGapType) -> List[C1ConcurrencyGap]:
        """Scan for a set of patterns and return gaps"""
        gaps = []
        
        for pattern_name, (pattern_re, severity, description) in patterns.items():
            matches = list(pattern_re.finditer(content))
            
            for match in matches:
                # Calculate line number
                line_num = content[:match.start()].count('\n') + 1
                
                # Get snippet
                line_start = content.rfind('\n', 0, match.start()) + 1
                line_end = content.find('\n', match.end())
                if line_end == -1:
                    line_end = len(content)
                
                snippet = content[line_start:line_end].strip()[:100]
                
                gap = C1ConcurrencyGap(
                    file_path=file_path,
                    line_num=line_num,
                    gap_type=gap_type,
                    snippet=snippet,
                    severity=severity,
                    description=description,
                    remediation='Use std::mutex + std::lock_guard, std::atomic for flags, or higher-level synchronization primitives',
                    pattern_name=pattern_name
                )
                gaps.append(gap)
        
        return gaps


def main():
    """Main entry point for scanner"""
    import sys
    
    scanner = C1ConcurrencyScanner()
    all_gaps = []
    
    # Scan .cpp and .h files
    for ext in ['**/*.cpp', '**/*.h', '**/*.hpp']:
        for file_path in Path('.').glob(ext):
            if any(skip in str(file_path) for skip in ['test', 'build', '.git', 'external']):
                continue
            gaps = scanner.scan_file(str(file_path))
            all_gaps.extend(gaps)
    
    # Output results
    results = {
        'enhancement': 'C-1',
        'cwe': 'CWE-362',
        'title': 'Race Condition Detection',
        'total_gaps': len(all_gaps),
        'gaps': [gap.to_dict() for gap in all_gaps]
    }
    
    print(json.dumps(results, indent=2))
    return len(all_gaps)


if __name__ == '__main__':
    main()
