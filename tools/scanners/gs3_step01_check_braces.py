#!/usr/bin/env python3
"""
Gap Scanner Step 01 — Braces Balance Check

Detects:
- Unbalanced opening/closing braces in C++ files
- Missing closing braces (more opens than closes)
- Extra closing braces (more closes than opens)
- Namespace/class/function scope mismatch
"""

import re
import sys
from pathlib import Path
from typing import List, Tuple

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class BracesCheckScanner(BaseGapScanner):
    """Phase 1: Braces Balance Check"""
    
    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60
    
    def __init__(self):
        """Initialize Braces Check Scanner."""
        super().__init__("Braces Check Scanner", "1.0")
        
        # Pattern to detect string and comment content that should be ignored
        self.string_pattern = re.compile(r'"(?:\\.|[^"\\])*"')
        self.char_pattern = re.compile(r"'(?:\\.|[^'\\])*'")
        self.single_line_comment = re.compile(r'//.*$')
        self.multi_line_comment_start = re.compile(r'/\*')
        self.multi_line_comment_end = re.compile(r'\*/')
    
    def _strip_comments_and_strings(self, line: str) -> str:
        """Remove comments and string literals from line to get actual braces."""
        # Remove single-line comments
        line = self.single_line_comment.sub('', line)
        
        # Remove string literals
        line = self.string_pattern.sub('', line)
        
        # Remove character literals
        line = self.char_pattern.sub('', line)
        
        return line
    
    def _count_braces(self, file_path: Path, lines: List[str]) -> Tuple[int, int, List[Tuple[int, str]]]:
        """
        Count opening and closing braces in file, ignoring comments and strings.
        
        FIXED: Properly handle single-line multi-line comments (/* ... */ on same line)
        
        Returns:
            Tuple of (open_count, close_count, issues_list)
            issues_list contains (line_no, description) for detected problems
        """
        open_count = 0
        close_count = 0
        issues = []
        in_multiline_comment = False
        
        for line_no, line in enumerate(lines, 1):
            # FIX: Handle both single-line and multi-line comments
            # Check for multi-line comment start on this line
            if not in_multiline_comment:
                # Check if this line contains both /* and */ (single-line multi-line comment)
                start_pos = self.multi_line_comment_start.search(line)
                end_pos = self.multi_line_comment_end.search(line)
                
                if start_pos and end_pos and start_pos.end() <= end_pos.start():
                    # Single-line multi-line comment: /* ... */ on same line
                    # Remove the comment and continue processing this line
                    line = line[:start_pos.start()] + line[end_pos.end():]
                elif start_pos:
                    # Multi-line comment starts here
                    in_multiline_comment = True
                    continue
                # else: no comment, process normally
            
            # If we're in a multi-line comment
            if in_multiline_comment:
                # Check if this line ends the comment
                if self.multi_line_comment_end.search(line):
                    in_multiline_comment = False
                continue
            
            # Strip comments and strings (single-line comments only, multi-line already handled)
            clean_line = self._strip_comments_and_strings(line)
            
            # Count braces
            line_opens = clean_line.count('{')
            line_closes = clean_line.count('}')
            
            open_count += line_opens
            close_count += line_closes
            
            # Check for closing without opening (potential issue)
            if close_count > open_count:
                issues.append((line_no, f"Extra closing brace detected (stack imbalance)"))
        
        return open_count, close_count, issues
    
    def _analyze_scope_context(self, file_path: Path, lines: List[str]) -> List[Tuple[int, str]]:
        """
        Analyze scope context to identify likely scope mismatch lines.
        Returns list of (line_no, context_description) tuples.
        
        FIXED: Only track scopes for definitions with opening braces, not declarations.
        This eliminates false positives from forward declarations like 'class Foo;' or 'struct Bar;'
        """
        scope_stack = []
        issues = []
        in_multiline_comment = False
        
        for line_no, line in enumerate(lines, 1):
            # Track multi-line comments - FIX: handle single-line multi-line comments
            if not in_multiline_comment:
                start_pos = self.multi_line_comment_start.search(line)
                end_pos = self.multi_line_comment_end.search(line)
                
                if start_pos and end_pos and start_pos.end() <= end_pos.start():
                    # Single-line multi-line comment: /* ... */ on same line
                    # Remove the comment and continue processing this line
                    line = line[:start_pos.start()] + line[end_pos.end():]
                elif start_pos:
                    # Multi-line comment starts here
                    in_multiline_comment = True
                    continue
            
            # If we're in a multi-line comment
            if in_multiline_comment:
                # Check if this line ends the comment
                if self.multi_line_comment_end.search(line):
                    in_multiline_comment = False
                continue
            
            clean_line = self._strip_comments_and_strings(line)
            
            # Track scope entries - ONLY for definitions with opening braces
            # FIX: Require { after namespace/class/struct to avoid forward declarations
            namespace_match = re.search(r'\bnamespace\s+(\w+)\s*\{', clean_line)
            # FIX: Class definitions must have { (not declarations like 'class Foo;')
            class_match = re.search(r'\bclass\s+(\w+)\s*[:\{]', clean_line)
            # FIX: Struct definitions must have { (not declarations like 'struct Foo;')
            struct_match = re.search(r'\bstruct\s+(\w+)\s*[:\{]', clean_line)
            
            # FIX: Improved function detection - must end with {
            function_match = re.search(r'\b\w+\s+\w+\s*\([^)]*\)\s*(?:const)?\s*(?:noexcept)?\s*(?:override)?\s*(?:final)?\s*\{', clean_line)
            
            if namespace_match:
                scope_stack.append(('namespace', namespace_match.group(1), line_no))
            elif class_match and '{' in clean_line:
                # Only track class definitions (with {), not forward declarations
                scope_stack.append(('class', class_match.group(1), line_no))
            elif struct_match and '{' in clean_line:
                # Only track struct definitions (with {), not forward declarations
                scope_stack.append(('struct', struct_match.group(1), line_no))
            elif function_match:
                # Function definitions always have {
                scope_stack.append(('function', 'unknown', line_no))
            
            # Track scope exits
            if '}' in clean_line:
                close_count = clean_line.count('}')
                for _ in range(close_count):
                    if scope_stack:
                        scope_type, scope_name, scope_start = scope_stack.pop()
                        # Ensure matching - could add type checking here
                    else:
                        issues.append((line_no, "Closing brace without matching opening scope"))
        
        # Any remaining items in scope_stack are unclosed
        for scope_type, scope_name, scope_start in scope_stack:
            issues.append((scope_start, f"Unclosed {scope_type} '{scope_name}' (missing closing brace)"))
        
        return issues
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for brace balance issues"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            # Only check C++ header and source files
            if file_path.suffix not in {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.c'}:
                continue
            
            file_path = file_path.resolve()
            self.files_scanned += 1
            
            try:
                lines = self._read_file_lines(file_path)
            except Exception as e:
                self._log(f"Error reading {file_path}: {e}")
                continue
            
            # Count braces
            open_count, close_count, count_issues = self._count_braces(file_path, lines)
            
            # Check for imbalance
            if open_count != close_count:
                imbalance = open_count - close_count
                
                # Create a gap for the imbalance
                gap = Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=1,
                    type="braces_imbalance",
                    severity="CRITICAL" if abs(imbalance) > 1 else "HIGH",
                    confidence=1.0,
                    description=f"Brace imbalance detected: {open_count} opening braces, {close_count} closing braces (diff: {imbalance:+d})",
                    remediation=f"Check file for missing or extra braces. Use check_braces.py for detailed analysis.",
                    context=f"Total opens: {open_count}, Total closes: {close_count}"
                )
                gaps.append(gap)
            
            # Add issues found during counting (like extra closing braces mid-file)
            for line_no, issue_desc in count_issues:
                gap = Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="braces_imbalance_midfile",
                    severity="HIGH",
                    confidence=0.95,
                    description=f"Brace balance issue at line {line_no}: {issue_desc}",
                    remediation="Review opening and closing braces around this line.",
                    context=lines[line_no - 1].strip() if line_no <= len(lines) else ""
                )
                gaps.append(gap)
            
            # Analyze scope context
            scope_issues = self._analyze_scope_context(file_path, lines)
            for line_no, issue_desc in scope_issues:
                gap = Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="scope_mismatch",
                    severity="CRITICAL" if "Unclosed" in issue_desc else "MEDIUM",
                    confidence=0.9,
                    description=f"Scope issue at line {line_no}: {issue_desc}",
                    remediation="Ensure all namespaces, classes, and functions have matching braces.",
                    context=lines[line_no - 1].strip() if line_no <= len(lines) else ""
                )
                gaps.append(gap)
        
        return self.deduplicate(gaps)
