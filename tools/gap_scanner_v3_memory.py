#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Memory Safety Gaps Detection

Detects memory safety violations:
- M-1: Use-After-Free (CWE-416)
  * Iterator invalidation after container modification
  * Pointer to temporary object
  * Use after std::move with moved-from object reuse
- M-2: Double-Free (CWE-415)
  * Double-free in exception paths
  * Double-free in loop clearing
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class MemorySafetyGapType(Enum):
    """Memory safety gap classifications"""
    ITERATOR_INVALIDATION = "iterator_invalidation"          # Iterator use after container modification
    POINTER_TO_TEMPORARY = "pointer_to_temporary"            # Pointer to temporary object
    USE_AFTER_MOVE = "use_after_move"                        # Using moved-from object
    DOUBLE_FREE_EXCEPTION = "double_free_exception"          # Delete in try and catch
    DOUBLE_FREE_LOOP = "double_free_loop"                    # Delete in loop then clear/destructor


@dataclass
class MemorySafetyGap:
    """Represents a memory safety gap"""
    file_path: str
    line_num: int
    gap_type: MemorySafetyGapType
    snippet: str
    severity: str  # CRITICAL, HIGH, MEDIUM
    description: str
    remediation: str
    cwe: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
            'cwe': self.cwe,
        }


class MemorySafetyGapScanner:
    """Detect memory safety gaps in C++ code"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[MemorySafetyGap]] = {}

    def _is_comment_or_test(self, line: str, file_path: Path) -> bool:
        """Check if line is a comment or part of test code"""
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            return True
        if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name.lower():
            return True
        return False

    def _get_context_lines(self, lines: List[str], start_idx: int, before: int = 5, after: int = 5) -> str:
        """Extract context lines around a given index"""
        start = max(0, start_idx - before)
        end = min(len(lines), start_idx + 1 + after)
        return ''.join(lines[start:end])

    def _find_variable_declaration(self, lines: List[str], end_idx: int, var_name: str) -> int:
        """Find the declaration line of a variable"""
        for i in range(end_idx, max(-1, end_idx - 50), -1):
            if re.search(rf'(?:auto|.*?[*&]?)\s+{re.escape(var_name)}\s*=', lines[i]):
                return i
        return -1

    def _is_in_try_block(self, lines: List[str], line_idx: int) -> bool:
        """Check if line is within a try block"""
        scope_depth = 0
        for i in range(line_idx, max(-1, line_idx - 100), -1):
            scope_depth -= lines[i].count('}')
            scope_depth += lines[i].count('{')
            
            if 'try' in lines[i] and '{' in lines[i]:
                return True
            if scope_depth < 0:
                break
        return False

    def _find_matching_catch(self, lines: List[str], try_idx: int) -> List[int]:
        """Find catch blocks matching a try block"""
        catch_lines = []
        scope_depth = 0
        in_try_block = False
        
        for i in range(try_idx, min(len(lines), try_idx + 200)):
            line = lines[i]
            
            if 'try' in line and '{' in line:
                in_try_block = True
            
            if in_try_block:
                scope_depth += line.count('{') - line.count('}')
                
                if scope_depth == 0 and i > try_idx:
                    in_try_block = False
                    if 'catch' in lines[i] or (i + 1 < len(lines) and 'catch' in lines[i + 1]):
                        catch_lines.append(i)
                        if i + 1 < len(lines) and 'catch' in lines[i + 1]:
                            catch_lines.append(i + 1)
        
        return catch_lines

    def _scan_iterator_invalidation(self, lines: List[str], file_path: Path) -> List[MemorySafetyGap]:
        """Detect iterator invalidation after container modification"""
        gaps = []
        
        for line_idx, line in enumerate(lines, 1):
            if self._is_comment_or_test(line, file_path):
                continue
            
            # Pattern: auto it = v.begin();
            if re.search(r'(?:auto|.*?)\s+\w+\s*=\s*\w+\.(?:begin|rbegin|cbegin|crbegin)\(\)', line):
                it_match = re.search(r'\b(\w+)\s*=\s*(\w+)\.(?:begin|rbegin|cbegin|crbegin)\(\)', line)
                if not it_match:
                    continue
                
                it_name = it_match.group(1)
                container_name = it_match.group(2)
                
                # Look for container modification within next 15 lines
                context_lines = lines[line_idx:min(len(lines), line_idx + 15)]
                context = '\n'.join(context_lines)
                
                # Check for container-modifying operations
                if re.search(rf'{re.escape(container_name)}\.(push_back|pop_back|insert|erase|clear|resize|reserve)\s*\(', context):
                    # Check for iterator use after modification
                    if re.search(rf'\*{re.escape(it_name)}|{re.escape(it_name)}\->|\*\*{re.escape(it_name)}', context):
                        gap = MemorySafetyGap(
                            file_path=str(Path(file_path).relative_to(self.repo_root)),
                            line_num=line_idx,
                            gap_type=MemorySafetyGapType.ITERATOR_INVALIDATION,
                            snippet=line.strip()[:120],
                            severity='CRITICAL',
                            description=f'Iterator {it_name} used after container {container_name} modification',
                            remediation='Reassign iterator after container modification or use indices/ranges',
                            cwe='CWE-416'
                        )
                        gaps.append(gap)
        
        return gaps

    def _scan_pointer_to_temporary(self, lines: List[str], file_path: Path) -> List[MemorySafetyGap]:
        """Detect pointer to temporary object"""
        gaps = []
        
        for line_idx, line in enumerate(lines, 1):
            if self._is_comment_or_test(line, file_path):
                continue
            
            # Pattern: Type* p = &SomeFunc().member;
            if re.search(r'\b\w+\s*\*\s+\w+\s*=\s*&\s*\w+\([^)]*\)\.', line):
                ptr_match = re.search(r'(\w+)\s*\*\s+(\w+)\s*=\s*&\s*(\w+)\([^)]*\)\.(\w+)', line)
                if ptr_match:
                    ptr_type = ptr_match.group(1)
                    ptr_name = ptr_match.group(2)
                    func_name = ptr_match.group(3)
                    member_name = ptr_match.group(4)
                    
                    gap = MemorySafetyGap(
                        file_path=str(Path(file_path).relative_to(self.repo_root)),
                        line_num=line_idx,
                        gap_type=MemorySafetyGapType.POINTER_TO_TEMPORARY,
                        snippet=line.strip()[:120],
                        severity='CRITICAL',
                        description=f'Pointer {ptr_name} references member of temporary object from {func_name}()',
                        remediation=f'Store result as {ptr_type} not pointer, or use reference with extended lifetime',
                        cwe='CWE-416'
                    )
                    gaps.append(gap)
            
            # Pattern: Type* p = &func();
            if re.search(r'\b\w+\s*\*\s+\w+\s*=\s*&\s*\w+\s*\([^)]*\)\s*;', line):
                temp_ptr_match = re.search(r'(\w+)\s*\*\s+(\w+)\s*=\s*&\s*(\w+)\s*\(', line)
                if temp_ptr_match and '->' not in line and '.' not in line.split('=')[1]:
                    ptr_type = temp_ptr_match.group(1)
                    ptr_name = temp_ptr_match.group(2)
                    func_name = temp_ptr_match.group(3)
                    
                    gap = MemorySafetyGap(
                        file_path=str(Path(file_path).relative_to(self.repo_root)),
                        line_num=line_idx,
                        gap_type=MemorySafetyGapType.POINTER_TO_TEMPORARY,
                        snippet=line.strip()[:120],
                        severity='CRITICAL',
                        description=f'Pointer {ptr_name} to temporary object returned by {func_name}()',
                        remediation=f'Store as value {ptr_type} or use reference, ensure lifetime exceeds usage',
                        cwe='CWE-416'
                    )
                    gaps.append(gap)
        
        return gaps

    def _scan_use_after_move(self, lines: List[str], file_path: Path) -> List[MemorySafetyGap]:
        """Detect use of moved-from objects"""
        gaps = []
        
        for line_idx, line in enumerate(lines, 1):
            if self._is_comment_or_test(line, file_path):
                continue
            
            # Pattern: T u = std::move(t); ... use(t);
            if 'std::move' in line:
                move_match = re.search(r'(\w+)\s*=\s*std::move\s*\(\s*(\w+)\s*\)', line)
                if not move_match:
                    continue
                
                moved_to = move_match.group(1)
                moved_from = move_match.group(2)
                
                # Check following lines for use of moved-from variable
                for check_idx in range(line_idx, min(len(lines), line_idx + 20)):
                    check_line = lines[check_idx]
                    
                    if self._is_comment_or_test(check_line, file_path):
                        continue
                    
                    # Skip if variable is reassigned
                    if re.search(rf'{re.escape(moved_from)}\s*=\s*', check_line) and check_idx > line_idx:
                        break
                    
                    # Check for usage patterns (excluding move itself and current line)
                    if check_idx > line_idx:
                        use_patterns = [
                            rf'{re.escape(moved_from)}\s*\.',           # t.method()
                            rf'{re.escape(moved_from)}\s*\->',          # t->member
                            rf'\*\s*{re.escape(moved_from)}',           # *t
                            rf'&\s*{re.escape(moved_from)}\b',          # &t
                            rf'\[\s*{re.escape(moved_from)}\s*\]',      # array[t]
                        ]
                        
                        for pattern in use_patterns:
                            if re.search(pattern, check_line):
                                gap = MemorySafetyGap(
                                    file_path=str(Path(file_path).relative_to(self.repo_root)),
                                    line_num=check_idx + 1,
                                    gap_type=MemorySafetyGapType.USE_AFTER_MOVE,
                                    snippet=check_line.strip()[:120],
                                    severity='CRITICAL',
                                    description=f'Use of moved-from object {moved_from} after std::move',
                                    remediation=f'Avoid reusing {moved_from} after std::move; use {moved_to} or reassign',
                                    cwe='CWE-416'
                                )
                                gaps.append(gap)
                                break
        
        return gaps

    def _scan_double_free_exception(self, lines: List[str], file_path: Path) -> List[MemorySafetyGap]:
        """Detect double-free in exception paths"""
        gaps = []
        
        for line_idx, line in enumerate(lines, 1):
            if self._is_comment_or_test(line, file_path):
                continue
            
            # Find delete in try block
            if 'try' in line and '{' in line:
                try_idx = line_idx - 1
                context = self._get_context_lines(lines, try_idx, before=0, after=100)
                
                # Find delete statements in try block
                delete_matches = list(re.finditer(r'delete\s+(\w+)', context))
                if not delete_matches:
                    continue
                
                for delete_match in delete_matches:
                    deleted_var = delete_match.group(1)
                    
                    # Find corresponding catch blocks
                    catch_lines = self._find_matching_catch(lines, try_idx)
                    
                    for catch_idx in catch_lines:
                        if catch_idx >= len(lines):
                            continue
                        
                        # Check if same variable is deleted in catch
                        catch_context = self._get_context_lines(lines, catch_idx, before=0, after=20)
                        if re.search(rf'delete\s+{re.escape(deleted_var)}\b', catch_context):
                            gap = MemorySafetyGap(
                                file_path=str(Path(file_path).relative_to(self.repo_root)),
                                line_num=catch_idx + 1,
                                gap_type=MemorySafetyGapType.DOUBLE_FREE_EXCEPTION,
                                snippet=lines[catch_idx].strip()[:120] if catch_idx < len(lines) else 'catch block',
                                severity='CRITICAL',
                                description=f'Double-free of {deleted_var}: deleted in try block and catch block',
                                remediation='Use smart pointers (unique_ptr/shared_ptr) or delete only once in cleanup',
                                cwe='CWE-415'
                            )
                            gaps.append(gap)
        
        return gaps

    def _scan_double_free_loop(self, lines: List[str], file_path: Path) -> List[MemorySafetyGap]:
        """Detect double-free in loop clearing patterns"""
        gaps = []
        
        for line_idx, line in enumerate(lines, 1):
            if self._is_comment_or_test(line, file_path):
                continue
            
            # Pattern: for (auto p : collection) delete p;
            if re.search(r'for\s*\(\s*(?:auto|.*?[*&]?)\s+\w+\s*:\s*(\w+)\s*\)', line):
                loop_match = re.search(r'for\s*\(\s*(?:auto|.*?)\s+(\w+)\s*:\s*(\w+)\s*\)', line)
                if not loop_match:
                    continue
                
                loop_var = loop_match.group(1)
                container = loop_match.group(2)
                
                # Check loop body for delete
                context = self._get_context_lines(lines, line_idx - 1, before=0, after=10)
                if re.search(rf'delete\s+{re.escape(loop_var)}', context):
                    # Check for subsequent clear/destructor
                    further_context = self._get_context_lines(lines, line_idx - 1, before=0, after=30)
                    
                    if re.search(rf'{re.escape(container)}\s*\.clear\(\)', further_context):
                        gap = MemorySafetyGap(
                            file_path=str(Path(file_path).relative_to(self.repo_root)),
                            line_num=line_idx,
                            gap_type=MemorySafetyGapType.DOUBLE_FREE_LOOP,
                            snippet=line.strip()[:120],
                            severity='CRITICAL',
                            description=f'Double-free: {container} elements deleted in loop, then cleared',
                            remediation=f'Either delete in loop OR call clear(), not both; prefer smart pointers',
                            cwe='CWE-415'
                        )
                        gaps.append(gap)
            
            # Pattern: container of raw pointers, manual delete in destructor after loop clear
            if 'delete' in line and re.search(r'for\s*\(', self._get_context_lines(lines, line_idx - 1, before=30, after=0)):
                prev_context = self._get_context_lines(lines, line_idx - 1, before=30, after=0)
                container_match = re.search(r'for\s*\(\s*auto\s+\w+\s*:\s*(\w+)\s*\)', prev_context)
                
                if container_match:
                    container_name = container_match.group(1)
                    delete_match = re.search(rf'delete\s+(\w+)', line)
                    
                    if delete_match:
                        deleted_var = delete_match.group(1)
                        
                        if re.search(rf'{re.escape(container_name)}\s*\.(?:clear|resize)\s*\(', prev_context):
                            gap = MemorySafetyGap(
                                file_path=str(Path(file_path).relative_to(self.repo_root)),
                                line_num=line_idx,
                                gap_type=MemorySafetyGapType.DOUBLE_FREE_LOOP,
                                snippet=line.strip()[:120],
                                severity='CRITICAL',
                                description=f'Potential double-free: {container_name} cleared in loop, delete in cleanup',
                                remediation='Use smart pointers in container or clear container without separate delete',
                                cwe='CWE-415'
                            )
                            gaps.append(gap)
        
        return gaps

    def scan_file(self, file_path: Path) -> List[MemorySafetyGap]:
        """Scan single file for memory safety gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Skip non-C++ files
        if file_path.suffix not in ['.cpp', '.hpp', '.h', '.cc', '.cxx']:
            return gaps
        
        # Run all memory safety checks
        gaps.extend(self._scan_iterator_invalidation(lines, file_path))
        gaps.extend(self._scan_pointer_to_temporary(lines, file_path))
        gaps.extend(self._scan_use_after_move(lines, file_path))
        gaps.extend(self._scan_double_free_exception(lines, file_path))
        gaps.extend(self._scan_double_free_loop(lines, file_path))
        
        return gaps

    def scan_module(self, module: str) -> Dict[str, List[MemorySafetyGap]]:
        """Scan module for memory safety gaps"""
        gaps_by_file = {}
        
        src_dir = self.repo_root / 'src' / module
        include_dir = self.repo_root / 'include' / module
        
        for directory in [src_dir, include_dir]:
            if not directory.exists():
                continue
            
            cpp_files = list(directory.rglob('*.cpp'))
            hpp_files = list(directory.rglob('*.hpp'))
            h_files = list(directory.rglob('*.h'))
            cc_files = list(directory.rglob('*.cc'))
            
            for file_path in cpp_files + hpp_files + h_files + cc_files:
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path.relative_to(self.repo_root))] = gaps
        
        return gaps_by_file

    def run_full_scan(self, output_dir: str = 'ai_working') -> Dict[str, any]:
        """Scan all modules for memory safety gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for MEMORY SAFETY GAPS...")
        
        src_root = self.repo_root / 'src'
        if not src_root.exists():
            print("   [!] src/ directory not found")
            return {}
        
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        total_gaps_all = 0
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            total_gaps_all += total_gaps
            
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
        
        print(f"\n[✓] TOTAL MEMORY SAFETY GAPS: {total_gaps_all}")
        
        # Write results
        output_file = output_path / 'memory_safety_gaps_report.json'
        with open(output_file, 'w') as f:
            json.dump(aggregate, f, indent=2)
        print(f"[✓] Report written to {output_file}")
        
        return aggregate


def main():
    """Main entry point for standalone execution"""
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    scanner = MemorySafetyGapScanner(repo_root=repo_root)
    results = scanner.run_full_scan(output_dir=output_dir)
    
    return 0 if results else 1


if __name__ == '__main__':
    exit(main())
