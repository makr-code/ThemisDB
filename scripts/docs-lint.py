"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs-lint.py                                       ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Documentation Linting Script for ThemisDB
Validates markdown structure, syntax, and conventions in docs/ and compendium/
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Dict, Tuple
import json


class DocumentationLinter:
    def __init__(self, base_path: Path, config: Dict = None):
        self.base_path = base_path
        self.config = config or {}
        self.errors = []
        self.warnings = []
        self.stats = {
            'files_checked': 0,
            'errors': 0,
            'warnings': 0
        }

    def add_error(self, file_path: str, line_num: int, message: str):
        """Add an error to the error list"""
        self.errors.append({
            'file': file_path,
            'line': line_num,
            'severity': 'error',
            'message': message
        })
        self.stats['errors'] += 1

    def add_warning(self, file_path: str, line_num: int, message: str):
        """Add a warning to the warning list"""
        self.warnings.append({
            'file': file_path,
            'line': line_num,
            'severity': 'warning',
            'message': message
        })
        self.stats['warnings'] += 1

    def check_heading_hierarchy(self, file_path: Path, lines: List[str]):
        """Check that heading levels are properly nested (no skipping levels)"""
        prev_level = 0
        for i, line in enumerate(lines, 1):
            if line.startswith('#'):
                # Count heading level
                level = len(line) - len(line.lstrip('#'))
                if level > 0 and line[level:level+1] == ' ':
                    # Valid heading
                    if level > prev_level + 1:
                        self.add_warning(
                            str(file_path),
                            i,
                            f"Heading level skipped (from h{prev_level} to h{level})"
                        )
                    prev_level = level

    def check_markdown_syntax(self, file_path: Path, lines: List[str]):
        """Check for common markdown syntax issues"""
        in_code_block = False
        code_block_marker = None
        
        for i, line in enumerate(lines, 1):
            # Track code blocks
            stripped = line.strip()
            if stripped.startswith('```'):
                in_code_block = not in_code_block
                continue
            
            if in_code_block:
                continue
            
            # Check for broken links
            if '[' in line and '](' in line:
                # Find all markdown links
                link_pattern = r'\[([^\]]+)\]\(([^\)]+)\)'
                matches = re.finditer(link_pattern, line)
                for match in matches:
                    link_text = match.group(1)
                    link_url = match.group(2)
                    
                    # Check for empty link text
                    if not link_text.strip():
                        self.add_error(str(file_path), i, "Empty link text")
                    
                    # Check for empty URLs
                    if not link_url.strip():
                        self.add_error(str(file_path), i, "Empty link URL")
            
            # Check for incorrect heading format (missing space after #)
            # Only check lines that look like headings (start with #)
            if line.startswith('#') and not line.startswith('#' * 10):
                heading_match = re.match(r'^(#{1,6})([^\s#])', line)
                if heading_match:
                    self.add_error(
                        str(file_path),
                        i,
                        "Missing space after heading marker"
                    )
            
            # Check for trailing whitespace (spaces/tabs before newline)
            line_without_newline = line.rstrip('\n\r')
            if line_without_newline != line_without_newline.rstrip():
                self.add_warning(str(file_path), i, "Trailing whitespace")

    def check_required_sections(self, file_path: Path, lines: List[str], filename: str):
        """Check if important documentation files have required sections"""
        content = '\n'.join(lines).lower()
        
        # README files should have certain sections
        if filename.upper() == 'README.MD':
            required_sections = ['installation', 'usage']
            for section in required_sections:
                if section not in content:
                    self.add_warning(
                        str(file_path),
                        0,
                        f"README should contain '{section}' section"
                    )

    def check_file_naming(self, file_path: Path):
        """Check file naming conventions"""
        filename = file_path.name
        
        # Check for spaces in filename
        if ' ' in filename:
            self.add_error(
                str(file_path),
                0,
                "Filename contains spaces (use underscores or hyphens)"
            )
        
        # Check for uppercase extensions
        if filename.endswith('.MD') or filename.endswith('.Md'):
            self.add_warning(
                str(file_path),
                0,
                "File extension should be lowercase (.md)"
            )

    def lint_file(self, file_path: Path):
        """Lint a single markdown file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            self.stats['files_checked'] += 1
            
            # Run all checks
            self.check_file_naming(file_path)
            self.check_heading_hierarchy(file_path, lines)
            self.check_markdown_syntax(file_path, lines)
            self.check_required_sections(file_path, lines, file_path.name)
            
        except UnicodeDecodeError:
            self.add_error(str(file_path), 0, "File encoding error (not UTF-8)")
        except Exception as e:
            self.add_error(str(file_path), 0, f"Unexpected error: {str(e)}")

    def lint_directory(self, directory: Path, exclude_patterns: List[str] = None):
        """Lint all markdown files in a directory recursively"""
        exclude_patterns = exclude_patterns or []
        exclude_dirs = {'node_modules', '.git', 'site', 'build', '__pycache__', 'output'}
        
        for root, dirs, files in os.walk(directory):
            # Filter out excluded directories
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            
            root_path = Path(root)
            
            # Check if this path should be excluded
            should_exclude = False
            for pattern in exclude_patterns:
                if pattern in str(root_path):
                    should_exclude = True
                    break
            
            if should_exclude:
                continue
            
            for file in files:
                if file.endswith('.md'):
                    file_path = root_path / file
                    self.lint_file(file_path)

    def generate_report(self, output_format: str = 'text') -> str:
        """Generate a report of linting results"""
        if output_format == 'json':
            return json.dumps({
                'stats': self.stats,
                'errors': self.errors,
                'warnings': self.warnings
            }, indent=2)
        
        # Text format
        report = []
        report.append("=" * 80)
        report.append("Documentation Linting Report")
        report.append("=" * 80)
        report.append(f"Files checked: {self.stats['files_checked']}")
        report.append(f"Errors: {self.stats['errors']}")
        report.append(f"Warnings: {self.stats['warnings']}")
        report.append("")
        
        if self.errors:
            report.append("ERRORS:")
            report.append("-" * 80)
            for error in self.errors:
                line_info = f":{error['line']}" if error['line'] > 0 else ""
                report.append(f"{error['file']}{line_info}")
                report.append(f"  {error['message']}")
                report.append("")
        
        if self.warnings:
            report.append("WARNINGS:")
            report.append("-" * 80)
            for warning in self.warnings:
                line_info = f":{warning['line']}" if warning['line'] > 0 else ""
                report.append(f"{warning['file']}{line_info}")
                report.append(f"  {warning['message']}")
                report.append("")
        
        if not self.errors and not self.warnings:
            report.append("✓ No issues found!")
        
        report.append("=" * 80)
        return '\n'.join(report)


def main():
    parser = argparse.ArgumentParser(
        description='Lint ThemisDB documentation for structure and syntax issues'
    )
    parser.add_argument(
        'paths',
        nargs='*',
        default=['docs', 'compendium'],
        help='Paths to lint (default: docs and compendium)'
    )
    parser.add_argument(
        '--exclude',
        nargs='*',
        default=['ARCHIVED', 'archive'],
        help='Patterns to exclude from linting'
    )
    parser.add_argument(
        '--format',
        choices=['text', 'json'],
        default='text',
        help='Output format (default: text)'
    )
    parser.add_argument(
        '--output',
        '-o',
        help='Output file (default: stdout)'
    )
    parser.add_argument(
        '--fail-on-warnings',
        action='store_true',
        help='Exit with error code if warnings are found'
    )
    
    args = parser.parse_args()
    
    # Get base path
    base_path = Path(__file__).parent.parent
    
    # Create linter
    linter = DocumentationLinter(base_path)
    
    # Lint all specified paths
    for path_str in args.paths:
        path = base_path / path_str
        if not path.exists():
            print(f"Warning: Path does not exist: {path}", file=sys.stderr)
            continue
        
        if path.is_file():
            linter.lint_file(path)
        else:
            linter.lint_directory(path, args.exclude)
    
    # Generate report
    report = linter.generate_report(args.format)
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"Report written to: {args.output}")
    else:
        print(report)
    
    # Exit with appropriate code
    if linter.stats['errors'] > 0:
        sys.exit(1)
    elif args.fail_on_warnings and linter.stats['warnings'] > 0:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == '__main__':
    main()
