#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Platform Portability Detection

Detects:
- Missing platform guards (#ifdef _WIN32, etc.)
- Hardcoded path separators
- Assuming POSIX-only APIs
- Windows/Linux API gaps
- Endianness assumptions
- Integer size assumptions
- Unportable preprocessor directives
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class PlatformGapType(Enum):
    """Platform portability gap classifications"""
    MISSING_PLATFORM_GUARD = "missing_platform_guard"  # No #ifdef
    POSIX_ONLY_API = "posix_only_api"             # POSIX-specific API
    WINDOWS_ONLY_API = "windows_only_api"         # Windows-specific API
    HARDCODED_PATH = "hardcoded_path"             # Path separator hardcoded
    ENDIANNESS_ASSUMPTION = "endianness_assumption"  # Assumes little-endian
    SIZE_ASSUMPTION = "size_assumption"           # Assumes pointer/int size
    UNPORTABLE_PRAGMA = "unportable_pragma"       # Non-portable pragma


@dataclass
class PlatformGap:
    """Represents a platform portability gap"""
    file_path: str
    line_num: int
    gap_type: PlatformGapType
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


class PlatformGapScanner:
    """Detect platform portability issues in C++ code"""
    
    POSIX_APIS = [
        'pthread_', 'fork(', 'unlink(', 'getpid(', 'pipe(', 'select('
    ]
    
    WINDOWS_APIS = [
        'CreateThread', 'WaitForSingleObject', 'GetCurrentProcess', 'RegOpenKeyEx'
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[PlatformGap]] = {}
        self.file_platform_context = {}  # Track ifdef context per file
    
    def _should_skip_hardcoded_path(self, line: str, context: str) -> bool:
        """WHITELIST: Distinguish compile-time vs runtime paths.
        
        Skip compile-time constants, configuration sources, environment variables.
        Returns True if this line should be whitelisted (NOT flagged as a gap).
        """
        l_lower = line.lower()
        
        # WHITELIST 1: Compile-time constants
        if 'constexpr' in context or '#define' in context:
            return True
        
        # WHITELIST 2: Configuration sources
        if 'config::' in l_lower or 'config_' in l_lower or 'getConfig(' in line:
            return True
        
        # WHITELIST 3: Environment variables
        if 'getenv' in line or 'std::getenv' in line or 'std::env' in line:
            return True
        
        # WHITELIST 4: Test paths and fixtures
        if '_test' in str(line).lower() or 'fixture' in l_lower or 'test_data' in l_lower:
            return True
        
        # WHITELIST 5: URL/URI paths (not filesystem paths)
        if 'http://' in line or 'https://' in line or 'file://' in line:
            return True
        
        # WHITELIST 6: Comments or debug logging
        if 'LOG' in line or 'printf' in line or 'cout' in line or 'log_' in l_lower:
            return True
        
        return False
    
    def scan_file(self, file_path: Path) -> List[PlatformGap]:
        """Scan single file for platform portability issues"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Track ifdef context
        ifdef_stack = []
        
        for line_num, line in enumerate(lines, 1):
            # Skip comments
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue
            if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name:
                continue
            
            # Track #ifdef context
            if '#ifdef' in line or '#if defined' in line:
                ifdef_stack.append(line.strip())
            if '#endif' in line and ifdef_stack:
                ifdef_stack.pop()
            
            current_ifdef = ' '.join(ifdef_stack) if ifdef_stack else ''
            
            # Check for POSIX-only APIs without platform guard
            for api_name in ['pthread_', 'fork(', 'unlink(', 'getpid(', 'pipe(', 'select(']:
                if api_name in line:
                    if '_WIN32' not in current_ifdef and 'ifndef _WIN32' not in current_ifdef:
                        gap = PlatformGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=PlatformGapType.POSIX_ONLY_API,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'POSIX-only API {api_name} without platform guard',
                            remediation=f'Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative'
                        )
                        gaps.append(gap)
            
            # Check for Windows-only APIs without platform guard
            for api_name in ['CreateThread', 'WaitForSingleObject', 'GetCurrentProcess', 'RegOpenKeyEx']:
                if api_name in line:
                    if '_WIN32' not in current_ifdef:
                        gap = PlatformGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=PlatformGapType.WINDOWS_ONLY_API,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'Windows-only API {api_name} without platform guard',
                            remediation=f'Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction'
                        )
                        gaps.append(gap)
            
            # Check for hardcoded path separators
            if ('"' in line or "'" in line) and ('\\' in line and '/' in line):
                # Skip if this is a whitelisted pattern
                current_ifdef = ' '.join(ifdef_stack) if ifdef_stack else ''
                if self._should_skip_hardcoded_path(line, current_ifdef):
                    continue
                
                # Contains both / and \ in string
                string_matches = re.findall(r'["\']([^"\']*[/\\][^"\']*)["\']', line)
                for match in string_matches:
                    gap = PlatformGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=PlatformGapType.HARDCODED_PATH,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description=f'Hardcoded path separator — not portable',
                        remediation='Use std::filesystem::path or boost::filesystem for cross-platform paths'
                    )
                    gaps.append(gap)
            
            # Check for endianness assumptions
            if 'htonl' in line or 'ntohl' in line:
                # These assume big-endian network byte order
                next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
                
                if 'memcpy' in next_context or 'reinterpret_cast' in next_context:
                    gap = PlatformGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=PlatformGapType.ENDIANNESS_ASSUMPTION,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description='Endianness conversion may have undefined behavior',
                        remediation='Document endianness assumptions or use portable serialization'
                    )
                    gaps.append(gap)
            
            # Check for size assumptions (pointer/int)
            if 'sizeof(' in line and ('8' in line or '4' in line or '16' in line):
                # Hardcoded size assumption
                gap = PlatformGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=PlatformGapType.SIZE_ASSUMPTION,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='Hardcoded size assumption — pointer/int size may differ on platforms',
                    remediation='Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants'
                )
                gaps.append(gap)
            
            # Check for non-portable pragmas
            if '#pragma pack' in line or '#pragma once' in line:
                gap = PlatformGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=PlatformGapType.UNPORTABLE_PRAGMA,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Non-standard pragma may not be supported on all platforms',
                    remediation='Use standard C++ mechanisms: static_assert, or move to portable config'
                )
                gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[PlatformGap]]:
        """Scan module for platform portability issues"""
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
        """Scan all modules for platform portability issues"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for PLATFORM PORTABILITY GAPS...")
        
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
