#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 ÔÇö Concurrency Gaps Detection

Detects:
- Data races (unprotected shared data access)
- Lock ordering violations / deadlock risks
- Missing mutex/lock guards
- Callback/async race conditions
- Thread-unsafe singleton access
- Memory ordering issues
- Double-lock protection (mutex twice)
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
                    # Check if next few lines have lock protection
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
                    prev_context = ''.join(lines[max(0, line_num-2):line_num])
                    combined = prev_context + next_context
                    
                    has_lock = any(lock_pattern in combined for lock_pattern in 
                                 ['lock_guard', 'unique_lock', 'scoped_lock'])
                    
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
                        description='Raw .lock() without corresponding .unlock() ÔÇö use RAII instead',
                        remediation='Replace with std::lock_guard<std::mutex> guard(mutex);'
                    )
                    gaps.append(gap)
            
            # Check for potential deadlock (nested locking)
            if 'lock_guard' in line or 'unique_lock' in line:
                # Check if multiple locks in nested scope
                next_lines = lines[line_num:min(len(lines), line_num+10)]
                lock_count = sum(1 for l in next_lines if 'lock_guard' in l or 'unique_lock' in l)
                
                if lock_count > 1:
                    gap = ConcurrencyGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ConcurrencyGapType.DEADLOCK_RISK,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Multiple locks acquired in nested scope ÔÇö potential deadlock',
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
                        description='memory_order_relaxed used ÔÇö potential visibility issue',
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
