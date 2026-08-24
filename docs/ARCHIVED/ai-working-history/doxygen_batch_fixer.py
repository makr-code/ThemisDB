#!/usr/bin/env python3
"""
Doxygen Batch Fixer for Query Module (Phase 2)
Identifies and reports documentation issues in C++ headers
"""

import re
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple

@dataclass
class DocIssue:
    """Represents a documentation issue in a header file"""
    file: str
    line: int
    severity: str  # 'error', 'warning'
    issue_type: str  # 'missing_brief', 'missing_param', 'param_mismatch', etc.
    details: str
    
    def __str__(self):
        return f"{self.file}:{self.line} [{self.severity}] {self.issue_type}: {self.details}"

class DoxygenAuditScanner:
    """Scans C++ headers for Doxygen documentation issues"""
    
    def __init__(self):
        self.issues: List[DocIssue] = []
        self.files_scanned = 0
        
    def scan_file(self, fpath: Path) -> List[DocIssue]:
        """Scan a single C++ header file for Doxygen issues"""
        file_issues = []
        self.files_scanned += 1
        
        with open(fpath, 'r', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')
        
        # Find all Doxygen comment blocks and their associated functions
        doc_blocks = self._extract_doc_blocks(lines)
        
        for block_start, block_end, block_content, func_line, func_sig in doc_blocks:
            issues = self._check_doc_block(block_content, func_sig, func_line)
            for issue in issues:
                issue.file = str(fpath)
                file_issues.append(issue)
                self.issues.append(issue)
        
        return file_issues
    
    def _extract_doc_blocks(self, lines: List[str]) -> List[Tuple[int, int, str, int, str]]:
        """Extract Doxygen comment blocks and their associated function signatures"""
        blocks = []
        i = 0
        
        while i < len(lines):
            # Look for /** start
            if '/**' in lines[i]:
                block_start = i
                block_lines = [lines[i]]
                i += 1
                
                # Collect until */
                while i < len(lines) and '*/' not in lines[i]:
                    block_lines.append(lines[i])
                    i += 1
                
                if i < len(lines):
                    block_lines.append(lines[i])
                    block_end = i
                    block_content = '\n'.join(block_lines)
                    
                    # Look ahead for function signature (skip empty lines)
                    func_line = block_end + 1
                    while func_line < len(lines) and not lines[func_line].strip():
                        func_line += 1
                    
                    if func_line < len(lines):
                        func_sig = lines[func_line].strip()
                        # Look for complete signature (may span multiple lines)
                        while func_line < len(lines) and ';' not in func_sig and '{' not in func_sig:
                            func_line += 1
                            if func_line < len(lines):
                                func_sig += ' ' + lines[func_line].strip()
                        
                        blocks.append((block_start, block_end, block_content, block_end + 1, func_sig))
            
            i += 1
        
        return blocks
    
    def _check_doc_block(self, doc_block: str, func_sig: str, line_no: int) -> List[DocIssue]:
        """Check a documentation block for issues"""
        issues = []
        
        # Skip if it's just a file header or struct/enum doc
        if any(x in func_sig for x in ['struct', 'enum', 'class', 'namespace', 'using']):
            return issues
        
        # Extract function name and parameters
        func_match = re.search(r'(\w+)\s*\(([^)]*)\)', func_sig)
        if not func_match:
            return issues
        
        func_name = func_match.group(1)
        params_str = func_match.group(2)
        
        # Parse parameter list
        param_names = self._parse_parameters(params_str)
        
        # Check for @brief
        if '@brief' not in doc_block and 'struct' not in func_sig:
            issues.append(DocIssue(
                file='',  # set by caller
                line=line_no,
                severity='warning',
                issue_type='missing_brief',
                details=f"Function '{func_name}' missing @brief"
            ))
        
        # Check @param consistency
        doc_params = self._extract_doc_params(doc_block)
        
        # For now, report if parameter count mismatch
        if param_names and doc_params and len(param_names) != len(doc_params):
            issues.append(DocIssue(
                file='',
                line=line_no,
                severity='warning',
                issue_type='param_mismatch',
                details=f"@param count ({len(doc_params)}) ≠ signature params ({len(param_names)}): {param_names}"
            ))
        
        # Check @return for non-void functions
        if 'void' not in func_sig.split('(')[0] and '@return' not in doc_block:
            if 'Result<' in func_sig or 'Optional' in func_sig or any(x in func_sig for x in ['int', 'bool', 'string', 'std::']):
                issues.append(DocIssue(
                    file='',
                    line=line_no,
                    severity='warning',
                    issue_type='missing_return',
                    details=f"Non-void function '{func_name}' missing @return"
                ))
        
        return issues
    
    def _parse_parameters(self, params_str: str) -> List[str]:
        """Extract parameter names from function signature"""
        if not params_str.strip():
            return []
        
        # Split by comma but respect nested brackets
        params = []
        current = ''
        depth = 0
        
        for char in params_str:
            if char in '<({[':
                depth += 1
            elif char in '>)}]':
                depth -= 1
            elif char == ',' and depth == 0:
                if current.strip():
                    # Extract last identifier as parameter name
                    match = re.search(r'(\w+)\s*(?:=|$)', current.strip())
                    if match:
                        params.append(match.group(1))
                current = ''
                continue
            
            current += char
        
        if current.strip():
            match = re.search(r'(\w+)\s*(?:=|$)', current.strip())
            if match:
                params.append(match.group(1))
        
        return params
    
    def _extract_doc_params(self, doc_block: str) -> List[str]:
        """Extract @param names from Doxygen comment"""
        matches = re.findall(r'@param\s+(\w+)', doc_block)
        return matches
    
    def report_summary(self):
        """Print summary of issues found"""
        print(f"\n{'='*70}")
        print(f"DOXYGEN AUDIT SUMMARY - Query Module Batch 1")
        print(f"{'='*70}\n")
        
        print(f"Files scanned: {self.files_scanned}")
        print(f"Total issues: {len(self.issues)}\n")
        
        # Group by severity and type
        by_type = {}
        for issue in self.issues:
            key = issue.issue_type
            by_type.setdefault(key, []).append(issue)
        
        print("Issues by type:")
        for issue_type, issues_list in sorted(by_type.items(), key=lambda x: -len(x[1])):
            print(f"\n  {issue_type}: {len(issues_list)}")
            for issue in issues_list[:3]:  # Show first 3 examples
                print(f"    - {issue}")
            if len(issues_list) > 3:
                print(f"    ... and {len(issues_list) - 3} more")
        
        print(f"\n{'='*70}\n")

def main():
    """Main entry point"""
    query_dir = Path("include/query")
    
    # Priority files for Batch 1
    priority_files = [
        'query_engine.h',
        'query_executor.h',
        'query_compiler.h',
        'query_optimizer.h',
        'aql_parser.h',
        'aql_runner.h',
        'aql_translator.h',
        'result_stream.h',
        'semantic_cache.h',
        'query_profiler.h',
        'query_rewrite_rule.h',
        'adaptive_join.h',
        'adaptive_optimizer.h',
        'parallel_executor.h',
        'window_evaluator.h',
        'incremental_agg.h',
        'tensor_aware_query_optimizer.h',
        'query_resource_limits.h',
        'query_cache.h',
        'query_cache_manager.h',
    ]
    
    scanner = DoxygenAuditScanner()
    
    print("Scanning Query Module Headers for Doxygen Issues...")
    print("="*70 + "\n")
    
    for fname in priority_files:
        fpath = query_dir / fname
        if fpath.exists():
            file_issues = scanner.scan_file(fpath)
            if file_issues:
                print(f"{fname}: {len(file_issues)} issues")
        else:
            print(f"{fname}: NOT FOUND")
    
    scanner.report_summary()
    
    return len(scanner.issues)

if __name__ == '__main__':
    issue_count = main()
    sys.exit(min(issue_count, 1))  # Exit code 0 if no issues, 1 if issues found
