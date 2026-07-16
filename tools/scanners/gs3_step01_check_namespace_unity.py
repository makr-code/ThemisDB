#!/usr/bin/env python3
"""
Gap Scanner Step 01 — Namespace, Forward Declarations & Unity Build Check

Detects:
- Namespace declaration/closure mismatches
- Forward declaration inconsistencies (declared but not defined, or vice versa)
- Improper namespace nesting
- Missing/improper include guards
- Multiple inclusion conflicts (unity build incompatibility)
- Incomplete type declarations (fwd decl without full definition)
"""

import re
import sys
from pathlib import Path
from typing import List, Tuple, Dict, Set, Optional
from collections import defaultdict

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class NamespaceUnityCheckScanner(BaseGapScanner):
    """Phase 1: Namespace, Forward Declarations & Unity Build Consistency"""
    
    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 120
    
    def __init__(self):
        """Initialize Namespace/Unity Scanner."""
        super().__init__("Namespace & Unity Build Scanner", "1.0")
        
        # Regex patterns
        self.namespace_open_pattern = re.compile(r'^\s*namespace\s+(\w+)\s*\{')
        self.namespace_close_pattern = re.compile(r'^\s*\}\s*(?://\s*namespace\s+(\w+))?')
        self.fwd_decl_class_pattern = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*;')
        self.fwd_decl_enum_pattern = re.compile(r'^\s*enum\s+(?:class\s+)?(\w+)\s*(?::\s*\w+)?\s*;')
        self.class_def_pattern = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?:<.*?>)?\s*(?:\{|:)')
        self.enum_def_pattern = re.compile(r'^\s*enum\s+(?:class\s+)?(\w+)\s*(?::\s*\w+)?\s*\{')
        self.include_guard_pattern = re.compile(r'^\s*#ifndef\s+([A-Z0-9_]+)(?:\s*$|\n)')
        self.pragma_once_pattern = re.compile(r'^\s*#pragma\s+once')
        self.include_pattern = re.compile(r'^\s*#include\s+[<"]([^>"]+)[>"]')
        self.comment_pattern = re.compile(r'//.*$|/\*.*?\*/', re.DOTALL)
        
    def _extract_namespace_path(self, lines: List[str]) -> Tuple[List[Tuple[int, str]], List[Tuple[int, str]]]:
        """
        Extract namespace opens and closes with line numbers.
        Returns (opens_list, closes_list) where each contains (line_no, namespace_name)
        """
        opens = []
        closes = []
        indent_level = 0
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            line_clean = self.comment_pattern.sub('', line)
            
            # Count braces to track indent
            opens_in_line = line_clean.count('{')
            closes_in_line = line_clean.count('}')
            prev_indent = indent_level
            indent_level += opens_in_line - closes_in_line
            
            # Check for namespace open (only at file scope, indent 0)
            if prev_indent == 0 and opens_in_line > 0:
                match_open = self.namespace_open_pattern.search(line_clean)
                if match_open:
                    opens.append((line_no, match_open.group(1)))
            
            # Check for namespace close (only at file scope, moving from indent 1 to 0)
            if prev_indent > 0 and indent_level == 0 and closes_in_line > 0:
                match_close = self.namespace_close_pattern.search(line_clean)
                if match_close and (line_clean.strip() == '}' or line_clean.strip().startswith('} //')):
                    closes.append((line_no, match_close.group(1) or ""))
        
        return opens, closes
    
    def _validate_namespace_nesting(self, opens: List[Tuple[int, str]], closes: List[Tuple[int, str]]) -> List[Gap]:
        """
        Validate proper namespace nesting.
        Returns gaps for mismatched namespace declarations/closures.
        """
        gaps = []
        
        # Check count match
        if len(opens) != len(closes):
            gap = Gap(
                file="<file>",
                line=0,
                type="namespace_mismatch_count",
                severity=SeverityLevel.CRITICAL.value,
                confidence=0.95,
                description=f"Namespace mismatch: {len(opens)} opens vs {len(closes)} closes",
                remediation="Ensure each 'namespace NAME {' has a matching '}' closure",
            )
            gaps.append(gap)
        
        # Check for improper nesting (LIFO)
        if opens and closes:
            last_open_line = opens[-1][0]
            first_close_line = closes[0][0]
            
            # If first close comes before last open, we likely have improper nesting
            if first_close_line < last_open_line:
                gap = Gap(
                    file="<file>",
                    line=first_close_line,
                    type="namespace_improper_nesting",
                    severity=SeverityLevel.HIGH.value,
                    confidence=0.85,
                    description="Namespace closure appears to come before opening (LIFO violation)",
                    remediation="Check namespace declaration/closure order; ensure LIFO (Last-In-First-Out) nesting",
                )
                gaps.append(gap)
        
        # Check for missing namespace name in closure comment (only for actual namespace closes)
        for close_line, close_ns_name in closes:
            if not close_ns_name:  # No namespace name captured in closure
                gap = Gap(
                    file="<file>",
                    line=close_line,
                    type="namespace_missing_closure_comment",
                    severity=SeverityLevel.LOW.value,
                    confidence=0.70,
                    description="Namespace closure lacks identifying comment (e.g., '} // namespace foo')",
                    remediation="Add '// namespace <name>' comment to closing brace for clarity",
                )
                gaps.append(gap)
        
        return gaps
    
    def _collect_forward_declarations(self, lines: List[str]) -> Dict[str, List[Tuple[int, str]]]:
        """
        Collect all forward declarations (classes, structs, enums).
        Returns dict: {'classes': [(line, name), ...], 'enums': [...]}
        """
        fwd_decls = defaultdict(list)
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            line_clean = self.comment_pattern.sub('', line)
            
            # Check class/struct forward declarations
            match_class = self.fwd_decl_class_pattern.search(line_clean)
            if match_class:
                fwd_decls['classes'].append((line_no, match_class.group(1)))
            
            # Check enum forward declarations
            match_enum = self.fwd_decl_enum_pattern.search(line_clean)
            if match_enum:
                fwd_decls['enums'].append((line_no, match_enum.group(1)))
        
        return fwd_decls
    
    def _collect_definitions(self, lines: List[str]) -> Dict[str, List[Tuple[int, str]]]:
        """
        Collect all class/struct/enum definitions.
        Returns dict: {'classes': [(line, name), ...], 'enums': [...]}
        """
        defs = defaultdict(list)
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            line_clean = self.comment_pattern.sub('', line)
            
            # Check class/struct definitions
            match_class = self.class_def_pattern.search(line_clean)
            if match_class and ';' not in line_clean:  # Not a fwd decl
                defs['classes'].append((line_no, match_class.group(1)))
            
            # Check enum definitions
            match_enum = self.enum_def_pattern.search(line_clean)
            if match_enum:
                defs['enums'].append((line_no, match_enum.group(1)))
        
        return defs
    
    def _validate_forward_declarations(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Validate forward declarations against definitions.
        Returns gaps for incomplete declarations or orphaned definitions.
        """
        gaps = []
        
        fwd_decls = self._collect_forward_declarations(lines)
        defs = self._collect_definitions(lines)
        
        # Check for forward-declared but never defined classes/structs
        for fwd_line, fwd_name in fwd_decls.get('classes', []):
            defined = any(name == fwd_name for _, name in defs.get('classes', []))
            
            # Only flag as gap if file is header (likely external definition)
            if not defined and str(file_path).endswith('.cpp'):
                gap = Gap(
                    file="<file>",
                    line=fwd_line,
                    type="fwd_decl_never_defined",
                    severity=SeverityLevel.MEDIUM.value,
                    confidence=0.75,
                    description=f"Class/struct '{fwd_name}' is forward-declared but never defined in this file",
                    remediation="Either define the class/struct, or ensure it's defined in a header that's included",
                )
                gaps.append(gap)
        
        # Check for forward-declared but never defined enums
        for fwd_line, fwd_name in fwd_decls.get('enums', []):
            defined = any(name == fwd_name for _, name in defs.get('enums', []))
            
            if not defined and str(file_path).endswith('.cpp'):
                gap = Gap(
                    file="<file>",
                    line=fwd_line,
                    type="fwd_decl_enum_never_defined",
                    severity=SeverityLevel.MEDIUM.value,
                    confidence=0.80,
                    description=f"Enum '{fwd_name}' is forward-declared but never defined in this file",
                    remediation="Either define the enum, or ensure it's defined in a header that's included",
                )
                gaps.append(gap)
        
        return gaps
    
    def _check_include_guards(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Check for proper include guards in header files.
        Returns gaps for missing or improperly formatted include guards.
        """
        gaps = []
        
        # Only check header files
        if not (str(file_path).endswith('.h') or str(file_path).endswith('.hpp')):
            return gaps
        
        if len(lines) < 5:
            return gaps  # Too small, probably doesn't need guard
        
        # Check first 10 lines for include guard or pragma once
        has_include_guard = False
        has_pragma_once = False
        
        for i, line in enumerate(lines[:10]):
            if self.include_guard_pattern.search(line):
                has_include_guard = True
                break
            if self.pragma_once_pattern.search(line):
                has_pragma_once = True
                break
        
        if not has_include_guard and not has_pragma_once:
            gap = Gap(
                file="<file>",
                line=1,
                type="missing_include_guard",
                severity=SeverityLevel.HIGH.value,
                confidence=0.90,
                description="Header file lacks include guard or #pragma once",
                remediation="Add '#pragma once' at the top or use '#ifndef GUARD_NAME' / '#define GUARD_NAME'",
            )
            gaps.append(gap)
        
        return gaps
    
    def _check_unity_build_compatibility(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Check for common unity build incompatibilities.
        Returns gaps for patterns that break unity builds.
        """
        gaps = []
        in_anonymous_namespace = False
        
        for line_no, line in enumerate(lines, 1):
            # Track if we're in anonymous namespace
            if 'namespace {' in line:
                in_anonymous_namespace = True
            if line.strip().startswith('} // anonymous namespace'):
                in_anonymous_namespace = False
            
            # Pattern: using namespace in header (bad for unity builds)
            if ('using namespace' in line and 
                str(file_path).endswith(('.h', '.hpp')) and 
                not line.strip().startswith('//')):
                
                gap = Gap(
                    file="<file>",
                    line=line_no,
                    type="unity_build_using_namespace",
                    severity=SeverityLevel.MEDIUM.value,
                    confidence=0.85,
                    description="Header file contains 'using namespace' which breaks unity builds",
                    remediation="Remove 'using namespace' from headers; use fully qualified names instead",
                )
                gaps.append(gap)
            
            # Pattern: static functions/variables at file scope in .cpp (potential ODR issues in unity)
            # Only if NOT in anonymous namespace (anonymous namespace is the correct pattern)
            if (not in_anonymous_namespace and
                'static ' in line and 
                str(file_path).endswith('.cpp') and 
                re.search(r'\bstatic\s+(void|bool|int|auto|std::|[\w:]+\*?\s+\w+)', line) and
                not line.strip().startswith('//')):
                
                # Additional filter: must look like function/variable declaration, not inside a statement
                if '(' in line or '=' in line or ';' in line:
                    gap = Gap(
                        file="<file>",
                        line=line_no,
                        type="unity_build_static_file_scope",
                        severity=SeverityLevel.LOW.value,
                        confidence=0.65,
                        description="File-scope static declaration may cause ODR issues in unity builds",
                        remediation="Wrap in anonymous namespace { ... } or move to header with inline",
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan(self, file_path: Path) -> List[Gap]:
        """
        Main scanner entry point.
        """
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            return gaps
        
        if not lines:
            return gaps
        
        # 1. Namespace validation
        opens, closes = self._extract_namespace_path(lines)
        namespace_gaps = self._validate_namespace_nesting(opens, closes)
        gaps.extend(namespace_gaps)
        
        # 2. Forward declaration validation (for .cpp files primarily)
        if str(file_path).endswith('.cpp'):
            fwd_gaps = self._validate_forward_declarations(file_path, lines)
            gaps.extend(fwd_gaps)
        
        # 3. Include guard check (for .h/.hpp files)
        guard_gaps = self._check_include_guards(file_path, lines)
        gaps.extend(guard_gaps)
        
        # 4. Unity build compatibility check
        unity_gaps = self._check_unity_build_compatibility(file_path, lines)
        gaps.extend(unity_gaps)
        
        # Set file path for all gaps
        for gap in gaps:
            gap.file = str(file_path.relative_to(file_path.parents[2]))
        
        return gaps
