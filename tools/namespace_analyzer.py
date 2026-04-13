"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            namespace_analyzer.py                              ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     821                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Namespace Analyzer
===========================

This tool analyzes the ThemisDB codebase to extract and report on:
- Namespaces and their hierarchies
- Classes, structs, and enums within each namespace
- Functions and their signatures
- Variables and constants
- Temporal information (when each entity was introduced/modified)

Usage:
    python3 namespace_analyzer.py [options]

Options:
    --output-dir DIR    Output directory for reports (default: ./namespace_analysis)
    --format FORMAT     Output format: json, markdown, csv, all (default: all)
    --include-git       Include git metadata (timestamps, authors)
    --verbose           Enable verbose output
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass, field, asdict
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Set, Optional, Tuple
import csv


@dataclass
class GitMetadata:
    """Git metadata for a code entity"""
    first_commit: Optional[str] = None
    first_commit_date: Optional[str] = None
    first_commit_author: Optional[str] = None
    last_commit: Optional[str] = None
    last_commit_date: Optional[str] = None
    last_commit_author: Optional[str] = None
    total_commits: int = 0


@dataclass
class Variable:
    """Represents a variable or constant"""
    name: str
    type_hint: str
    file_path: str
    line_number: int
    is_const: bool = False
    is_constexpr: bool = False
    is_static: bool = False
    git_metadata: Optional[GitMetadata] = None


@dataclass
class Function:
    """Represents a function or method"""
    name: str
    signature: str
    return_type: str
    file_path: str
    line_number: int
    is_method: bool = False
    is_static: bool = False
    is_virtual: bool = False
    is_const: bool = False
    is_template: bool = False
    parameters: List[str] = field(default_factory=list)
    git_metadata: Optional[GitMetadata] = None


@dataclass
class Class:
    """Represents a class or struct"""
    name: str
    type: str  # 'class', 'struct', 'enum', 'enum class'
    file_path: str
    line_number: int
    base_classes: List[str] = field(default_factory=list)
    methods: List[Function] = field(default_factory=list)
    members: List[Variable] = field(default_factory=list)
    is_template: bool = False
    template_params: List[str] = field(default_factory=list)
    git_metadata: Optional[GitMetadata] = None


@dataclass
class Namespace:
    """Represents a namespace"""
    name: str
    full_name: str
    parent: Optional[str] = None
    classes: List[Class] = field(default_factory=list)
    functions: List[Function] = field(default_factory=list)
    variables: List[Variable] = field(default_factory=list)
    nested_namespaces: List[str] = field(default_factory=list)
    files: Set[str] = field(default_factory=set)


class NamespaceAnalyzer:
    """Main analyzer class for ThemisDB namespace analysis"""
    
    def __init__(self, repo_root: Path, include_git: bool = False, verbose: bool = False):
        self.repo_root = repo_root
        self.include_git = include_git
        self.verbose = verbose
        self.namespaces: Dict[str, Namespace] = {}
        self.file_cache: Dict[str, List[str]] = {}
        
    def log(self, message: str):
        """Print message if verbose mode is enabled"""
        if self.verbose:
            print(f"[INFO] {message}")
    
    def get_git_metadata(self, file_path: str, line_number: int) -> Optional[GitMetadata]:
        """Get git metadata for a specific line in a file"""
        if not self.include_git:
            return None
        
        try:
            # Get blame information for the specific line
            blame_cmd = [
                'git', 'blame', '-L', f'{line_number},{line_number}',
                '--porcelain', str(file_path)
            ]
            blame_result = subprocess.run(
                blame_cmd,
                cwd=self.repo_root,
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if blame_result.returncode != 0:
                return None
            
            # Parse blame output
            blame_lines = blame_result.stdout.split('\n')
            commit_hash = blame_lines[0].split()[0] if blame_lines else None
            
            if not commit_hash:
                return None
            
            # Get commit details
            log_cmd = [
                'git', 'log', '--follow', '--format=%H|%ai|%an',
                '--', str(file_path)
            ]
            log_result = subprocess.run(
                log_cmd,
                cwd=self.repo_root,
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if log_result.returncode != 0:
                return None
            
            commits = []
            for line in log_result.stdout.strip().split('\n'):
                if line:
                    parts = line.split('|')
                    if len(parts) == 3:
                        commits.append({
                            'hash': parts[0],
                            'date': parts[1],
                            'author': parts[2]
                        })
            
            if not commits:
                return None
            
            # First commit is the oldest (last in list), last commit is newest (first in list)
            last_commit = commits[0]
            first_commit = commits[-1]
            
            return GitMetadata(
                first_commit=first_commit['hash'][:8],
                first_commit_date=first_commit['date'],
                first_commit_author=first_commit['author'],
                last_commit=last_commit['hash'][:8],
                last_commit_date=last_commit['date'],
                last_commit_author=last_commit['author'],
                total_commits=len(commits)
            )
        except Exception as e:
            print(f"Error getting git metadata: {e}")
            return None
    
    def read_file(self, file_path: Path) -> List[str]:
        """Read file content with caching"""
        file_str = str(file_path)
        if file_str not in self.file_cache:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    self.file_cache[file_str] = f.readlines()
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
                self.file_cache[file_str] = []
        return self.file_cache[file_str]
    
    def extract_namespace_hierarchy(self, full_namespace: str) -> List[str]:
        """Extract namespace hierarchy from full namespace name"""
        parts = full_namespace.split('::')
        hierarchy = []
        for i in range(len(parts)):
            hierarchy.append('::'.join(parts[:i+1]))
        return hierarchy
    
    def parse_class_declaration(self, line: str, lines: List[str], line_idx: int) -> Optional[Tuple[str, str, List[str], bool, List[str]]]:
        """Parse class/struct/enum declaration
        
        This regex pattern matches C++ class/struct/enum declarations with:
        - Optional template parameters: template<typename T>
        - Class type: class, struct, enum, enum class
        - Optional attributes: __attribute__((...))
        - Class name
        - Optional inheritance: : public Base, protected Base2
        - Opening brace or semicolon
        """
        # Match class/struct/enum declarations
        class_match = re.match(
            r'^\s*(?:template\s*<([^>]+)>\s*)?(class|struct|enum(?:\s+class)?)\s+(?:__attribute__\(\([^)]+\)\)\s+)?(?:\w+\s+)?(\w+)(?:\s*:\s*(.+?))?(?:\s*\{|;)',
            line
        )
        
        if not class_match:
            return None
        
        template_params_str = class_match.group(1)
        class_type = class_match.group(2)
        class_name = class_match.group(3)
        inheritance = class_match.group(4)
        
        is_template = template_params_str is not None
        template_params = []
        if is_template:
            # Simple template parameter extraction
            template_params = [p.strip() for p in template_params_str.split(',')]
        
        base_classes = []
        if inheritance:
            # Parse base classes
            bases = re.findall(r'(?:public|protected|private)?\s*([a-zA-Z_][\w:]*)', inheritance)
            base_classes = [b.strip() for b in bases if b.strip()]
        
        return class_name, class_type, base_classes, is_template, template_params
    
    def parse_function_declaration(self, line: str) -> Optional[Tuple[str, str, str, bool, bool, bool, bool, List[str]]]:
        """Parse function declaration
        
        This regex pattern matches C++ function declarations with:
        - Optional template parameters
        - Optional modifiers: virtual, static, inline, constexpr, explicit
        - Return type (with pointers/references)
        - Function name
        - Parameters in parentheses
        - Optional const qualifier
        - Optional trailing specifiers: override, final, noexcept, = default/delete/0
        """
        # Skip preprocessor directives, comments, and non-function lines
        if line.strip().startswith('#') or '//' in line or line.strip().startswith('/*'):
            return None
        
        # Match function declarations
        func_match = re.match(
            r'^\s*(?:template\s*<[^>]+>\s*)?((?:virtual|static|inline|constexpr|explicit)\s+)*([a-zA-Z_][\w:<>]*(?:\s*\*|\s*&)?)\s+([a-zA-Z_]\w*)\s*\(([^)]*)\)\s*(const)?\s*(?:override|final|noexcept|=\s*(?:0|default|delete))?',
            line
        )
        
        if not func_match:
            return None
        
        modifiers = func_match.group(1) or ''
        return_type = func_match.group(2).strip()
        func_name = func_match.group(3)
        params_str = func_match.group(4)
        is_const = func_match.group(5) is not None
        
        is_virtual = 'virtual' in modifiers
        is_static = 'static' in modifiers
        is_template = 'template' in line[:line.find(func_name)]
        
        # Parse parameters
        parameters = []
        if params_str.strip():
            # Simple parameter parsing
            param_parts = params_str.split(',')
            for param in param_parts:
                param = param.strip()
                if param and param != 'void':
                    parameters.append(param)
        
        signature = f"{return_type} {func_name}({params_str})"
        if is_const:
            signature += " const"
        
        return func_name, signature, return_type, is_static, is_virtual, is_const, is_template, parameters
    
    def parse_variable_declaration(self, line: str) -> Optional[Tuple[str, str, bool, bool, bool]]:
        """Parse variable/constant declaration"""
        # Match variable declarations (simplified)
        var_match = re.match(
            r'^\s*((?:static|const|constexpr|extern|inline)\s+)*([a-zA-Z_][\w:<>]*(?:\s*\*|\s*&)?)\s+([a-zA-Z_]\w*)\s*(?:=|;)',
            line
        )
        
        if not var_match:
            return None
        
        modifiers = var_match.group(1) or ''
        type_hint = var_match.group(2).strip()
        var_name = var_match.group(3)
        
        is_const = 'const' in modifiers
        is_constexpr = 'constexpr' in modifiers
        is_static = 'static' in modifiers
        
        return var_name, type_hint, is_const, is_constexpr, is_static
    
    def analyze_file(self, file_path: Path):
        """Analyze a single C++ header file"""
        self.log(f"Analyzing {file_path}")
        
        lines = self.read_file(file_path)
        if not lines:
            return
        
        current_namespace = []
        in_class = False
        class_depth = 0
        brace_count = 0
        
        for line_idx, line in enumerate(lines, 1):
            # Track namespace declarations
            ns_match = re.match(r'^\s*namespace\s+([a-zA-Z_][\w]*)\s*\{', line)
            if ns_match:
                ns_name = ns_match.group(1)
                current_namespace.append(ns_name)
                full_ns = '::'.join(current_namespace)
                
                if full_ns not in self.namespaces:
                    parent = '::'.join(current_namespace[:-1]) if len(current_namespace) > 1 else None
                    self.namespaces[full_ns] = Namespace(
                        name=ns_name,
                        full_name=full_ns,
                        parent=parent
                    )
                    
                    if parent and parent in self.namespaces:
                        self.namespaces[parent].nested_namespaces.append(full_ns)
                
                self.namespaces[full_ns].files.add(str(file_path.relative_to(self.repo_root)))
                continue
            
            # Track namespace closing
            if re.match(r'^\s*}\s*//\s*namespace', line) and current_namespace:
                current_namespace.pop()
                continue
            
            # Skip if not in a namespace
            if not current_namespace:
                continue
            
            full_ns = '::'.join(current_namespace)
            
            # Parse class/struct/enum declarations
            class_info = self.parse_class_declaration(line, lines, line_idx)
            if class_info:
                class_name, class_type, base_classes, is_template, template_params = class_info
                
                git_meta = self.get_git_metadata(str(file_path), line_idx)
                
                cls = Class(
                    name=class_name,
                    type=class_type,
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_number=line_idx,
                    base_classes=base_classes,
                    is_template=is_template,
                    template_params=template_params,
                    git_metadata=git_meta
                )
                self.namespaces[full_ns].classes.append(cls)
                continue
            
            # Parse function declarations
            func_info = self.parse_function_declaration(line)
            if func_info:
                func_name, signature, return_type, is_static, is_virtual, is_const, is_template, parameters = func_info
                
                git_meta = self.get_git_metadata(str(file_path), line_idx)
                
                func = Function(
                    name=func_name,
                    signature=signature,
                    return_type=return_type,
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_number=line_idx,
                    is_static=is_static,
                    is_virtual=is_virtual,
                    is_const=is_const,
                    is_template=is_template,
                    parameters=parameters,
                    git_metadata=git_meta
                )
                self.namespaces[full_ns].functions.append(func)
                continue
            
            # Parse variable declarations
            var_info = self.parse_variable_declaration(line)
            if var_info:
                var_name, type_hint, is_const, is_constexpr, is_static = var_info
                
                git_meta = self.get_git_metadata(str(file_path), line_idx)
                
                var = Variable(
                    name=var_name,
                    type_hint=type_hint,
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_number=line_idx,
                    is_const=is_const,
                    is_constexpr=is_constexpr,
                    is_static=is_static,
                    git_metadata=git_meta
                )
                self.namespaces[full_ns].variables.append(var)
    
    def find_header_files(self) -> List[Path]:
        """Find all C++ header files in the repository"""
        header_files = []
        
        include_dir = self.repo_root / 'include'
        src_dir = self.repo_root / 'src'
        
        for directory in [include_dir, src_dir]:
            if directory.exists():
                for ext in ['*.h', '*.hpp', '*.hh']:
                    header_files.extend(directory.rglob(ext))
        
        return sorted(header_files)
    
    def analyze(self):
        """Run the full analysis"""
        print(f"Starting namespace analysis of ThemisDB at {self.repo_root}")
        print(f"Git metadata: {'enabled' if self.include_git else 'disabled'}")
        
        header_files = self.find_header_files()
        print(f"Found {len(header_files)} header files to analyze")
        
        for file_path in header_files:
            self.analyze_file(file_path)
        
        print(f"Analysis complete. Found {len(self.namespaces)} namespaces")
        
        # Print summary
        total_classes = sum(len(ns.classes) for ns in self.namespaces.values())
        total_functions = sum(len(ns.functions) for ns in self.namespaces.values())
        total_variables = sum(len(ns.variables) for ns in self.namespaces.values())
        
        print(f"  Total classes: {total_classes}")
        print(f"  Total functions: {total_functions}")
        print(f"  Total variables: {total_variables}")
    
    def generate_json_report(self, output_dir: Path):
        """Generate JSON report"""
        output_file = output_dir / 'namespace_analysis.json'
        print(f"Generating JSON report: {output_file}")
        
        # Convert to dict for JSON serialization
        data = {
            'metadata': {
                'timestamp': datetime.now().isoformat(),
                'repo_root': str(self.repo_root),
                'total_namespaces': len(self.namespaces),
                'git_metadata_included': self.include_git
            },
            'namespaces': {}
        }
        
        for ns_name, ns in sorted(self.namespaces.items()):
            ns_dict = {
                'name': ns.name,
                'full_name': ns.full_name,
                'parent': ns.parent,
                'nested_namespaces': ns.nested_namespaces,
                'files': sorted(list(ns.files)),
                'classes': [],
                'functions': [],
                'variables': []
            }
            
            for cls in ns.classes:
                cls_dict = asdict(cls)
                ns_dict['classes'].append(cls_dict)
            
            for func in ns.functions:
                ns_dict['functions'].append(asdict(func))
            
            for var in ns.variables:
                ns_dict['variables'].append(asdict(var))
            
            data['namespaces'][ns_name] = ns_dict
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, default=str)
        
        print(f"JSON report saved to {output_file}")
    
    def generate_markdown_report(self, output_dir: Path):
        """Generate Markdown report"""
        output_file = output_dir / 'namespace_analysis.md'
        print(f"Generating Markdown report: {output_file}")
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("# ThemisDB Namespace Analysis Report\n\n")
            f.write(f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            f.write(f"**Repository:** {self.repo_root}\n\n")
            f.write(f"**Git Metadata:** {'Included' if self.include_git else 'Not included'}\n\n")
            
            # Summary
            f.write("## Summary\n\n")
            f.write(f"- **Total Namespaces:** {len(self.namespaces)}\n")
            total_classes = sum(len(ns.classes) for ns in self.namespaces.values())
            total_functions = sum(len(ns.functions) for ns in self.namespaces.values())
            total_variables = sum(len(ns.variables) for ns in self.namespaces.values())
            f.write(f"- **Total Classes:** {total_classes}\n")
            f.write(f"- **Total Functions:** {total_functions}\n")
            f.write(f"- **Total Variables:** {total_variables}\n\n")
            
            # Top namespaces
            f.write("## Top Namespaces by Entity Count\n\n")
            ns_by_count = sorted(
                self.namespaces.items(),
                key=lambda x: len(x[1].classes) + len(x[1].functions) + len(x[1].variables),
                reverse=True
            )[:10]
            
            f.write("| Namespace | Classes | Functions | Variables | Total |\n")
            f.write("|-----------|---------|-----------|-----------|-------|\n")
            for ns_name, ns in ns_by_count:
                total = len(ns.classes) + len(ns.functions) + len(ns.variables)
                f.write(f"| `{ns_name}` | {len(ns.classes)} | {len(ns.functions)} | {len(ns.variables)} | {total} |\n")
            f.write("\n")
            
            # Detailed namespace information
            f.write("## Namespace Details\n\n")
            
            for ns_name, ns in sorted(self.namespaces.items()):
                f.write(f"### `{ns_name}`\n\n")
                
                if ns.parent:
                    f.write(f"**Parent Namespace:** `{ns.parent}`\n\n")
                
                if ns.nested_namespaces:
                    f.write(f"**Nested Namespaces:** {len(ns.nested_namespaces)}\n")
                    for nested in sorted(ns.nested_namespaces):
                        f.write(f"- `{nested}`\n")
                    f.write("\n")
                
                f.write(f"**Files:** {len(ns.files)}\n")
                for file in sorted(ns.files):
                    f.write(f"- `{file}`\n")
                f.write("\n")
                
                # Classes
                if ns.classes:
                    f.write(f"#### Classes ({len(ns.classes)})\n\n")
                    for cls in sorted(ns.classes, key=lambda x: x.name):
                        f.write(f"**`{cls.name}`** ({cls.type})\n")
                        f.write(f"- Location: `{cls.file_path}:{cls.line_number}`\n")
                        if cls.is_template:
                            f.write(f"- Template parameters: `{', '.join(cls.template_params)}`\n")
                        if cls.base_classes:
                            f.write(f"- Base classes: `{', '.join(cls.base_classes)}`\n")
                        if cls.git_metadata:
                            f.write(f"- First commit: `{cls.git_metadata.first_commit}` by {cls.git_metadata.first_commit_author} on {cls.git_metadata.first_commit_date}\n")
                            f.write(f"- Last modified: `{cls.git_metadata.last_commit}` by {cls.git_metadata.last_commit_author} on {cls.git_metadata.last_commit_date}\n")
                        f.write("\n")
                
                # Functions
                if ns.functions:
                    f.write(f"#### Functions ({len(ns.functions)})\n\n")
                    for func in sorted(ns.functions, key=lambda x: x.name)[:20]:  # Limit to first 20
                        modifiers = []
                        if func.is_static:
                            modifiers.append('static')
                        if func.is_virtual:
                            modifiers.append('virtual')
                        if func.is_const:
                            modifiers.append('const')
                        if func.is_template:
                            modifiers.append('template')
                        
                        modifier_str = f" [{', '.join(modifiers)}]" if modifiers else ""
                        f.write(f"- `{func.name}(...)`{modifier_str}\n")
                        f.write(f"  - Signature: `{func.signature}`\n")
                        f.write(f"  - Location: `{func.file_path}:{func.line_number}`\n")
                        if func.git_metadata:
                            f.write(f"  - First commit: `{func.git_metadata.first_commit}` on {func.git_metadata.first_commit_date}\n")
                    
                    if len(ns.functions) > 20:
                        f.write(f"\n*... and {len(ns.functions) - 20} more functions*\n")
                    f.write("\n")
                
                # Variables
                if ns.variables:
                    f.write(f"#### Variables ({len(ns.variables)})\n\n")
                    for var in sorted(ns.variables, key=lambda x: x.name)[:20]:  # Limit to first 20
                        modifiers = []
                        if var.is_const:
                            modifiers.append('const')
                        if var.is_constexpr:
                            modifiers.append('constexpr')
                        if var.is_static:
                            modifiers.append('static')
                        
                        modifier_str = f" [{', '.join(modifiers)}]" if modifiers else ""
                        f.write(f"- `{var.name}` : `{var.type_hint}`{modifier_str}\n")
                        f.write(f"  - Location: `{var.file_path}:{var.line_number}`\n")
                        if var.git_metadata:
                            f.write(f"  - First commit: `{var.git_metadata.first_commit}` on {var.git_metadata.first_commit_date}\n")
                    
                    if len(ns.variables) > 20:
                        f.write(f"\n*... and {len(ns.variables) - 20} more variables*\n")
                    f.write("\n")
                
                f.write("---\n\n")
        
        print(f"Markdown report saved to {output_file}")
    
    def generate_csv_report(self, output_dir: Path):
        """Generate CSV reports"""
        print(f"Generating CSV reports in {output_dir}")
        
        # Namespaces CSV
        ns_file = output_dir / 'namespaces.csv'
        with open(ns_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['Namespace', 'Parent', 'Classes', 'Functions', 'Variables', 'Files'])
            for ns_name, ns in sorted(self.namespaces.items()):
                writer.writerow([
                    ns.full_name,
                    ns.parent or '',
                    len(ns.classes),
                    len(ns.functions),
                    len(ns.variables),
                    len(ns.files)
                ])
        
        # Classes CSV
        classes_file = output_dir / 'classes.csv'
        with open(classes_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            header = ['Namespace', 'Class', 'Type', 'File', 'Line', 'Is Template', 'Base Classes']
            if self.include_git:
                header.extend(['First Commit', 'First Date', 'Last Commit', 'Last Date'])
            writer.writerow(header)
            
            for ns_name, ns in sorted(self.namespaces.items()):
                for cls in sorted(ns.classes, key=lambda x: x.name):
                    row = [
                        ns.full_name,
                        cls.name,
                        cls.type,
                        cls.file_path,
                        cls.line_number,
                        cls.is_template,
                        ', '.join(cls.base_classes)
                    ]
                    if self.include_git and cls.git_metadata:
                        row.extend([
                            cls.git_metadata.first_commit or '',
                            cls.git_metadata.first_commit_date or '',
                            cls.git_metadata.last_commit or '',
                            cls.git_metadata.last_commit_date or ''
                        ])
                    writer.writerow(row)
        
        # Functions CSV
        functions_file = output_dir / 'functions.csv'
        with open(functions_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            header = ['Namespace', 'Function', 'Signature', 'Return Type', 'File', 'Line', 'Static', 'Virtual', 'Const', 'Template']
            if self.include_git:
                header.extend(['First Commit', 'First Date'])
            writer.writerow(header)
            
            for ns_name, ns in sorted(self.namespaces.items()):
                for func in sorted(ns.functions, key=lambda x: x.name):
                    row = [
                        ns.full_name,
                        func.name,
                        func.signature,
                        func.return_type,
                        func.file_path,
                        func.line_number,
                        func.is_static,
                        func.is_virtual,
                        func.is_const,
                        func.is_template
                    ]
                    if self.include_git and func.git_metadata:
                        row.extend([
                            func.git_metadata.first_commit or '',
                            func.git_metadata.first_commit_date or ''
                        ])
                    writer.writerow(row)
        
        print(f"CSV reports saved:")
        print(f"  - {ns_file}")
        print(f"  - {classes_file}")
        print(f"  - {functions_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Analyze ThemisDB namespaces, classes, functions, and variables',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=Path('./namespace_analysis'),
        help='Output directory for reports (default: ./namespace_analysis)'
    )
    parser.add_argument(
        '--format',
        choices=['json', 'markdown', 'csv', 'all'],
        default='all',
        help='Output format (default: all)'
    )
    parser.add_argument(
        '--include-git',
        action='store_true',
        help='Include git metadata (timestamps, authors)'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '--repo-root',
        type=Path,
        help='Repository root directory (default: auto-detect)'
    )
    
    args = parser.parse_args()
    
    # Determine repository root
    if args.repo_root:
        repo_root = args.repo_root
    else:
        # Auto-detect: find the directory containing CMakeLists.txt
        current = Path.cwd()
        while current != current.parent:
            if (current / 'CMakeLists.txt').exists():
                repo_root = current
                break
            current = current.parent
        else:
            print("Error: Could not find repository root. Please specify --repo-root")
            sys.exit(1)
    
    print(f"Repository root: {repo_root}")
    
    # Create output directory
    args.output_dir.mkdir(parents=True, exist_ok=True)
    
    # Run analysis
    analyzer = NamespaceAnalyzer(repo_root, args.include_git, args.verbose)
    analyzer.analyze()
    
    # Generate reports
    if args.format in ['json', 'all']:
        analyzer.generate_json_report(args.output_dir)
    
    if args.format in ['markdown', 'all']:
        analyzer.generate_markdown_report(args.output_dir)
    
    if args.format in ['csv', 'all']:
        analyzer.generate_csv_report(args.output_dir)
    
    print(f"\nAnalysis complete! Reports saved to {args.output_dir}")


if __name__ == '__main__':
    main()
