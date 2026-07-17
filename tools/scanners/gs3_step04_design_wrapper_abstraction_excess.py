#!/usr/bin/env python3
"""
ThemisDB Gap Scanner V3 — Wrapper Abstraction Excess Scanner

Detects "Boring Code" anti-pattern: cascading wrapper classes that add
complexity without functional value.
"""

from typing import List, Dict, Set, Tuple, Optional
from dataclasses import dataclass
import re
from pathlib import Path

# Import base classes
import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap


@dataclass
class ClassInfo:
    """Information about a class."""
    name: str
    line: int
    methods: List[str]
    has_ctor: bool
    has_state: bool
    wraps_other_class: bool
    wrapper_depth: int


class WrapperAbstractionExcessScanner(BaseGapScanner):
    """
    Detects cascading wrapper classes and "Boring Code" anti-patterns.
    
    Boring code = wrapper classes that only delegate without adding value.
    Detects:
    - Thin wrappers (few methods, just passthrough)
    - Passthrough methods (delegate without transformation)
    - Abstraction cascades (A->B->C->D chains)
    """
    
    def __init__(self):
        super().__init__(
            name="Wrapper Abstraction Excess",
            version="1.0"
        )
        self.file_classes: Dict[str, List[ClassInfo]] = {}
        self.wrapper_chains: List[Tuple[str, int, List[str]]] = []
    
    def scan_files(self, file_list: List[Path]) -> List[Gap]:
        """Scan a list of pre-collected C++ files. Required by the phase7-10 uniform loop."""
        self.gaps = []
        for file_path in file_list:
            self.scan_file(str(file_path))
        return self.gaps

    def scan(self, source_dir: str = ".") -> list:
        """Implementation of required scan() method from BaseGapScanner."""
        source_path = Path(source_dir)
        
        if source_path.is_file():
            self.scan_file(str(source_path))
        elif source_path.is_dir():
            for cpp_file in source_path.rglob("*.cpp"):
                self.scan_file(str(cpp_file))
            for hpp_file in source_path.rglob("*.hpp"):
                self.scan_file(str(hpp_file))
            for h_file in source_path.rglob("*.h"):
                self.scan_file(str(h_file))
        
        return self.gaps
    
    def scan_file(self, file_path: str) -> None:
        """Scan a single C++ file for wrapper excess."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return
        
        # Parse class definitions
        classes = self._extract_classes(file_path, content, lines)
        if classes:
            self.file_classes[file_path] = classes
        
        # Detect wrapper patterns
        self._detect_wrapper_patterns(file_path, classes, lines)
        
        # Detect passthrough methods
        self._detect_passthrough_methods(file_path, classes, content, lines)
        
        # Detect composition chains
        self._detect_composition_chains(file_path, classes, lines)
    
    def _extract_classes(
        self, file_path: str, content: str, lines: List[str]
    ) -> List[ClassInfo]:
        """Extract class definitions from content."""
        classes = []
        
        # Find class definitions - simpler pattern
        class_pattern = r'(?:^|\n)\s*(?:class|struct)\s+(\w+)'
        
        for match in re.finditer(class_pattern, content, re.MULTILINE):
            class_name = match.group(1)
            start_pos = match.start()
            end_of_declaration = match.end()
            
            # Find line number
            line_no = content[:start_pos].count('\n')
            
            # Find the opening brace
            body_start = content.find('{', end_of_declaration)
            if body_start < 0:
                continue
            
            # Find matching closing brace
            brace_count = 0
            body_end = body_start
            for i in range(body_start, len(content)):
                if content[i] == '{':
                    brace_count += 1
                elif content[i] == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        body_end = i
                        break
            
            if body_end == body_start:  # Didn't find closing brace
                continue
            
            class_body = content[body_start:body_end+1]
            
            # Check for state (member variables)
            has_state = self._has_member_variables(class_body)
            
            # Check if it wraps other classes
            wraps_other = self._check_if_wrapper(class_body)
            wrapper_depth = self._calculate_wrapper_depth(class_body)
            
            # Extract methods
            methods = self._extract_class_methods(class_body)
            
            # Check for constructor
            has_ctor = class_name in methods or f"{class_name}(" in class_body
            
            classes.append(ClassInfo(
                name=class_name,
                line=line_no,
                methods=methods,
                has_ctor=has_ctor,
                has_state=has_state,
                wraps_other_class=wraps_other,
                wrapper_depth=wrapper_depth,
            ))
        
        return classes
    
    def _has_member_variables(self, class_body: str) -> bool:
        """Check if class has member variables (state)."""
        # Remove comments
        class_body = re.sub(r'//.*?$', '', class_body, flags=re.MULTILINE)
        class_body = re.sub(r'/\*.*?\*/', '', class_body, flags=re.DOTALL)
        
        # Look for member declarations patterns (any identifier followed by ;)
        # Pattern: <type> <name> ; or <type>* <name> ;
        # Must not be in a method (check for parentheses)
        patterns = [
            r'\b(?:int|float|double|bool|char|long|short|signed|unsigned)\s+\w+\s*[;=]',
            r'\w+\s*\*\s*\w+\s*[;=]',  # Any pointer member
            r'std::\w+<[^>]+>\s+\w+\s*[;=]',  # STL member
        ]
        
        for pattern in patterns:
            if re.search(pattern, class_body):
                return True
        
        return False
    
    def _check_if_wrapper(self, class_body: str) -> bool:
        """Check if class wraps another class (composition/delegation pattern)."""
        # Remove comments
        class_body = re.sub(r'//.*?$', '', class_body, flags=re.MULTILINE)
        class_body = re.sub(r'/\*.*?\*/', '', class_body, flags=re.DOTALL)
        
        # Check for wrapped object patterns
        patterns = [
            r'std::unique_ptr\s*<',
            r'std::shared_ptr\s*<',
            r'std::weak_ptr\s*<',
            r'\w+\s*\*\s+\w+\s*[;=]',  # pointer member (e.g., Item* item_;)
        ]
        
        for pattern in patterns:
            if re.search(pattern, class_body):
                return True
        
        return False
    
    def _calculate_wrapper_depth(self, class_body: str) -> int:
        """Calculate how many levels deep of wrapping exists."""
        # Remove comments
        class_body = re.sub(r'//.*?$', '', class_body, flags=re.MULTILINE)
        class_body = re.sub(r'/\*.*?\*/', '', class_body, flags=re.DOTALL)
        
        # Count unique non-primitive member types
        depth = 0
        
        # Count std::unique_ptr/shared_ptr members
        unique_ptrs = len(re.findall(r'std::(?:unique_ptr|shared_ptr|weak_ptr)<[\w:]+>', class_body))
        depth += unique_ptrs
        
        # Count raw pointer members (excluding method pointers)
        raw_ptrs = len(re.findall(r'\w+\*\s+\w+\s*[;=]', class_body))
        depth += raw_ptrs
        
        # Count capitalized type members (likely class instances)
        class_members = len(re.findall(r'(?:class|struct)?\s+[A-Z]\w+\s+\w+\s*[;=]', class_body))
        depth += class_members
        
        return min(depth, 10)  # Cap at 10 for practicality
    
    def _extract_class_methods(self, class_body: str) -> List[str]:
        """Extract method names from class body."""
        methods = []
        
        # Remove comments
        class_body_clean = re.sub(r'//.*?$', '', class_body, flags=re.MULTILINE)
        class_body_clean = re.sub(r'/\*.*?\*/', '', class_body_clean, flags=re.DOTALL)
        
        # Look for method declarations/definitions
        # Pattern: name(
        # This matches both declarations and definitions
        method_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\('
        
        for match in re.finditer(method_pattern, class_body_clean):
            method_name = match.group(1)
            # Filter out likely false positives (keywords, control structures)
            if method_name not in ['if', 'while', 'for', 'switch', 'return', 'sizeof', 'catch', 'else', 'case']:
                methods.append(method_name)
        
        return list(set(methods))  # Remove duplicates
    
    def _detect_wrapper_patterns(
        self, file_path: str, classes: List[ClassInfo], lines: List[str]
    ) -> None:
        """Detect thin wrapper classes."""
        for cls in classes:
            # Thin wrapper: wraps others AND has few methods
            # (We expect it to have state, e.g., the wrapped object pointer)
            if cls.wraps_other_class and len(cls.methods) <= 3:
                severity = 'CRITICAL' if cls.wrapper_depth > 5 else \
                          'HIGH' if cls.wrapper_depth > 3 else 'MEDIUM'
                
                confidence = 0.8 if cls.wrapper_depth > 3 else 0.6
                
                self._add_gap(
                    self.gaps,
                    file_path,
                    cls.line,
                    'thin_wrapper',
                    severity,
                    confidence,
                    f"Class '{cls.name}' appears to be a thin wrapper "
                    f"(depth: {cls.wrapper_depth}, methods: {len(cls.methods)}). "
                    f"Consider merging with wrapped class or adding real functionality.",
                    "Add meaningful functionality to wrapper, or remove "
                    "the abstraction layer and use wrapped class directly.",
                    lines[cls.line] if cls.line < len(lines) else "",
                )
    
    def _detect_passthrough_methods(
        self, file_path: str, classes: List[ClassInfo], content: str, lines: List[str]
    ) -> None:
        """Detect classes with many passthrough methods."""
        # Look for passthrough method patterns
        passthrough_pattern = r'(?:m_\w+|this->m_\w+)\s*(?:->|\.)\s*(\w+)\s*\('
        
        content_str = '\n'.join(lines)
        passthrough_count = len(re.findall(passthrough_pattern, content_str))
        
        if passthrough_count >= 5:  # Multiple passthrough methods
            # Find approximate line with first passthrough
            for match in re.finditer(passthrough_pattern, content_str):
                line_no = content_str[:match.start()].count('\n') + 1
                
                if line_no > 0 and line_no <= len(lines):
                    self._add_gap(
                        self.gaps,
                        file_path,
                        line_no,
                        'passthrough_methods',
                        'MEDIUM',
                        0.7,
                        "Passthrough method detected - delegates to wrapped object "
                        "without transformation. Part of boring code anti-pattern.",
                        "Consider removing wrapper class and using wrapped object "
                        "directly, or add meaningful transformation logic.",
                        lines[line_no - 1] if line_no <= len(lines) else "",
                    )
                    break
    
    def _detect_composition_chains(
        self, file_path: str, classes: List[ClassInfo], lines: List[str]
    ) -> None:
        """Detect A-wraps-B-wraps-C abstraction cascades."""
        # Build a wrapper dependency graph
        wraps_graph: Dict[str, List[str]] = {}
        
        for cls in classes:
            if cls.wraps_other_class:
                wraps_graph[cls.name] = []  # Will be filled with what it wraps
        
        # Find chains with depth > 3
        for start_class in wraps_graph:
            chain_length = 0
            current = start_class
            visited = set()
            
            while current and chain_length < 10 and current not in visited:
                visited.add(current)
                chain_length += 1
                
                # Try to find next wrapped class
                found = False
                for cls in classes:
                    if cls.name == current and cls.wraps_other_class:
                        # This class wraps something, but we don't know exactly what
                        # So we just count it as a layer
                        found = True
                        break
                
                if not found:
                    break
                
                current = None  # Move to next hypothetical level
            
            if chain_length > 3:  # >3 levels is excessive
                cls_line = next((c.line for c in classes if c.name == start_class), 1)
                severity = 'CRITICAL' if chain_length > 5 else 'HIGH'
                
                self._add_gap(
                    self.gaps,
                    file_path,
                    cls_line,
                    'abstraction_cascade',
                    severity,
                    0.75,
                    f"Abstraction cascade detected: over {chain_length} levels of indirection "
                    f"add complexity without functional value ('Boring Code' anti-pattern).",
                    "Flatten abstraction hierarchy. Remove intermediate wrappers "
                    "that don't add meaningful transformation or business logic.",
                    lines[cls_line - 1] if cls_line <= len(lines) else "",
                )
