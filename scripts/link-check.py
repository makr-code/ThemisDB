"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            link-check.py                                      ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     467                                            ║
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
Link Validation Script for ThemisDB
Checks internal and external links in markdown documentation
"""

import os
import re
import sys
import argparse
import json
from pathlib import Path
from typing import List, Dict, Set, Tuple
from urllib.parse import urlparse, unquote
import time


class LinkChecker:
    def __init__(self, base_path: Path, config: Dict = None):
        self.base_path = base_path
        self.config = config or {}
        self.errors = []
        self.warnings = []
        self.all_files = set()
        self.checked_external_links = {}
        self.stats = {
            'files_checked': 0,
            'internal_links': 0,
            'external_links': 0,
            'broken_internal': 0,
            'broken_external': 0,
            'warnings': 0
        }

    def add_error(self, file_path: str, line_num: int, message: str, link: str = ""):
        """Add an error to the error list"""
        self.errors.append({
            'file': file_path,
            'line': line_num,
            'severity': 'error',
            'message': message,
            'link': link
        })

    def add_warning(self, file_path: str, line_num: int, message: str, link: str = ""):
        """Add a warning to the warning list"""
        self.warnings.append({
            'file': file_path,
            'line': line_num,
            'severity': 'warning',
            'message': message,
            'link': link
        })
        self.stats['warnings'] += 1

    def scan_all_files(self, directories: List[Path]):
        """Scan directories to build a list of all markdown files"""
        exclude_dirs = {'node_modules', '.git', 'site', 'build', '__pycache__', 'output'}
        
        for directory in directories:
            if not directory.exists():
                continue
                
            for root, dirs, files in os.walk(directory):
                dirs[:] = [d for d in dirs if d not in exclude_dirs]
                
                root_path = Path(root)
                for file in files:
                    if file.endswith('.md'):
                        file_path = root_path / file
                        rel_path = file_path.relative_to(self.base_path)
                        self.all_files.add(str(rel_path))

    def is_external_link(self, link: str) -> bool:
        """Check if a link is external (HTTP/HTTPS)"""
        return link.startswith('http://') or link.startswith('https://')

    def is_anchor_link(self, link: str) -> bool:
        """Check if a link is just an anchor (starts with #)"""
        return link.startswith('#')

    def resolve_link_path(self, source_file: Path, link: str) -> Path:
        """Resolve a relative link path to absolute path"""
        # Remove anchor if present
        link_path = link.split('#')[0]
        
        if not link_path:
            # Just an anchor, refers to current file
            return source_file
        
        # Handle relative paths
        if link_path.startswith('/'):
            # Absolute from repo root
            return self.base_path / link_path.lstrip('/')
        else:
            # Relative to current file
            return (source_file.parent / link_path).resolve()

    def check_internal_link(self, source_file: Path, link: str, line_num: int) -> bool:
        """Check if an internal link is valid"""
        try:
            # Parse link and anchor
            link_parts = link.split('#')
            link_path = link_parts[0]
            anchor = link_parts[1] if len(link_parts) > 1 else None
            
            # If it's just an anchor, we check the current file
            if not link_path:
                target_file = source_file
            else:
                target_file = self.resolve_link_path(source_file, link)
            
            # Check if file exists
            if not target_file.exists():
                self.add_error(
                    str(source_file.relative_to(self.base_path)),
                    line_num,
                    f"Broken internal link: target file not found",
                    link
                )
                self.stats['broken_internal'] += 1
                return False
            
            # If there's an anchor, verify it exists in the target file
            if anchor:
                if not self.check_anchor_exists(target_file, anchor):
                    self.add_warning(
                        str(source_file.relative_to(self.base_path)),
                        line_num,
                        f"Anchor not found in target file: #{anchor}",
                        link
                    )
            
            return True
            
        except Exception as e:
            print(f"[ERROR] Error checking internal link {link}: {e}", file=sys.stderr)
            self.add_error(
                str(source_file.relative_to(self.base_path)),
                line_num,
                f"Error checking internal link: {str(e)}",
                link
            )
            self.stats['broken_internal'] += 1
            return False

    def check_anchor_exists(self, file_path: Path, anchor: str) -> bool:
        """Check if an anchor/heading exists in a file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Convert anchor to heading format
            # GitHub converts headings to anchors by:
            # 1. Lowercase
            # 2. Replace spaces with hyphens
            # 3. Remove special characters
            
            # Look for heading that matches
            anchor_lower = anchor.lower()
            
            # Find all headings in the file
            heading_pattern = r'^#{1,6}\s+(.+)$'
            for line in content.split('\n'):
                match = re.match(heading_pattern, line)
                if match:
                    heading_text = match.group(1).strip()
                    # Convert to anchor format
                    heading_anchor = re.sub(r'[^\w\s-]', '', heading_text)
                    heading_anchor = re.sub(r'\s+', '-', heading_anchor)
                    heading_anchor = heading_anchor.lower()
                    
                    if heading_anchor == anchor_lower:
                        return True
            
            # Also check for explicit anchor tags
            if f'<a name="{anchor}"' in content or f'id="{anchor}"' in content:
                return True
            
            return False
            
        except Exception:
            print("[WARN] Anchor check failed; could not read target file", file=sys.stderr)
            return False

    def check_external_link(self, source_file: Path, link: str, line_num: int) -> bool:
        """Check if an external link is valid (basic check, doesn't actually request)"""
        # For CI/CD, we only do basic validation
        # Actual HTTP checks would be done by markdown-link-check action
        
        # Check for common issues
        if ' ' in link:
            self.add_error(
                str(source_file.relative_to(self.base_path)),
                line_num,
                "External link contains spaces",
                link
            )
            self.stats['broken_external'] += 1
            return False
        
        # Parse URL
        try:
            parsed = urlparse(link)
            if not parsed.scheme or not parsed.netloc:
                self.add_error(
                    str(source_file.relative_to(self.base_path)),
                    line_num,
                    "Invalid external link format",
                    link
                )
                self.stats['broken_external'] += 1
                return False
        except Exception:
            print(f"[WARN] Malformed external link: {link}", file=sys.stderr)
            self.add_error(
                str(source_file.relative_to(self.base_path)),
                line_num,
                "Malformed external link",
                link
            )
            self.stats['broken_external'] += 1
            return False
        
        return True

    def extract_links(self, content: str) -> List[Tuple[str, int]]:
        """Extract all markdown links from content with their line numbers"""
        links = []
        lines = content.split('\n')
        
        # Pattern for markdown links
        link_pattern = r'\[([^\]]*)\]\(([^\)]+)\)'
        
        in_code_block = False
        for line_num, line in enumerate(lines, 1):
            # Track code blocks properly
            stripped = line.strip()
            if stripped.startswith('```'):
                in_code_block = not in_code_block
                continue
            
            # Skip lines inside code blocks
            if in_code_block:
                continue
            
            matches = re.finditer(link_pattern, line)
            for match in matches:
                link_url = match.group(2).strip()
                links.append((link_url, line_num))
        
        return links

    def check_file(self, file_path: Path):
        """Check all links in a file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            self.stats['files_checked'] += 1
            
            # Extract all links
            links = self.extract_links(content)
            
            for link, line_num in links:
                if self.is_external_link(link):
                    self.stats['external_links'] += 1
                    # Only check external links if not in internal-only mode
                    if not self.config.get('internal_only', False):
                        self.check_external_link(file_path, link, line_num)
                else:
                    self.stats['internal_links'] += 1
                    self.check_internal_link(file_path, link, line_num)
                    
        except UnicodeDecodeError:
            self.add_error(
                str(file_path.relative_to(self.base_path)),
                0,
                "File encoding error (not UTF-8)",
                ""
            )
        except Exception as e:
            print(f"[ERROR] Unexpected error checking {file_path}: {e}", file=sys.stderr)
            self.add_error(
                str(file_path.relative_to(self.base_path)),
                0,
                f"Unexpected error: {str(e)}",
                ""
            )

    def check_directory(self, directory: Path, exclude_patterns: List[str] = None):
        """Check all markdown files in a directory"""
        exclude_patterns = exclude_patterns or []
        exclude_dirs = {'node_modules', '.git', 'site', 'build', '__pycache__', 'output'}
        
        for root, dirs, files in os.walk(directory):
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            
            root_path = Path(root)
            
            # Check exclusion
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
                    self.check_file(file_path)

    def generate_report(self, output_format: str = 'text') -> str:
        """Generate a report of link checking results"""
        if output_format == 'json':
            return json.dumps({
                'stats': self.stats,
                'errors': self.errors,
                'warnings': self.warnings
            }, indent=2)
        
        # Text format
        report = []
        report.append("=" * 80)
        report.append("Link Validation Report")
        report.append("=" * 80)
        report.append(f"Files checked: {self.stats['files_checked']}")
        report.append(f"Internal links: {self.stats['internal_links']}")
        report.append(f"External links: {self.stats['external_links']}")
        report.append(f"Broken internal links: {self.stats['broken_internal']}")
        report.append(f"Broken external links: {self.stats['broken_external']}")
        report.append(f"Warnings: {self.stats['warnings']}")
        report.append("")
        
        if self.errors:
            report.append("ERRORS:")
            report.append("-" * 80)
            for error in self.errors:
                line_info = f":{error['line']}" if error['line'] > 0 else ""
                report.append(f"{error['file']}{line_info}")
                report.append(f"  {error['message']}")
                if error['link']:
                    report.append(f"  Link: {error['link']}")
                report.append("")
        
        if self.warnings:
            report.append("WARNINGS:")
            report.append("-" * 80)
            for warning in self.warnings:
                line_info = f":{warning['line']}" if warning['line'] > 0 else ""
                report.append(f"{warning['file']}{line_info}")
                report.append(f"  {warning['message']}")
                if warning['link']:
                    report.append(f"  Link: {warning['link']}")
                report.append("")
        
        if not self.errors and not self.warnings:
            report.append("✓ No broken links found!")
        
        report.append("=" * 80)
        return '\n'.join(report)


def main():
    parser = argparse.ArgumentParser(
        description='Check links in ThemisDB documentation'
    )
    parser.add_argument(
        'paths',
        nargs='*',
        default=['docs', 'compendium'],
        help='Paths to check (default: docs and compendium)'
    )
    parser.add_argument(
        '--exclude',
        nargs='*',
        default=['ARCHIVED', 'archive'],
        help='Patterns to exclude from checking'
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
        '--internal-only',
        action='store_true',
        help='Only check internal links'
    )
    
    args = parser.parse_args()
    
    # Get base path
    base_path = Path(__file__).parent.parent
    
    # Create checker with config
    config = {'internal_only': args.internal_only}
    checker = LinkChecker(base_path, config)
    
    # Scan all files first
    scan_paths = [base_path / p for p in args.paths]
    checker.scan_all_files(scan_paths)
    
    # Check all specified paths
    for path_str in args.paths:
        path = base_path / path_str
        if not path.exists():
            print(f"Warning: Path does not exist: {path}", file=sys.stderr)
            continue
        
        if path.is_file():
            checker.check_file(path)
        else:
            checker.check_directory(path, args.exclude)
    
    # Generate report
    report = checker.generate_report(args.format)
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"Report written to: {args.output}")
    else:
        print(report)
    
    # Exit with appropriate code
    if checker.stats['broken_internal'] > 0 or checker.stats['broken_external'] > 0:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == '__main__':
    main()
