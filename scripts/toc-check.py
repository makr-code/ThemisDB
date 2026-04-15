"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            toc-check.py                                       ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:15:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     324                                            ║
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
TOC Validation Script for ThemisDB
Validates table of contents in mkdocs.yml and cross-references
"""

import os
import sys
import yaml
import argparse
import json
from pathlib import Path
from typing import List, Dict, Set


class TOCValidator:
    def __init__(self, base_path: Path):
        self.base_path = base_path
        self.errors = []
        self.warnings = []
        self.stats = {
            'nav_entries': 0,
            'files_in_nav': 0,
            'files_on_disk': 0,
            'orphaned_files': 0,
            'missing_files': 0,
            'warnings': 0
        }

    def add_error(self, message: str, file_path: str = ""):
        """Add an error to the error list"""
        self.errors.append({
            'severity': 'error',
            'message': message,
            'file': file_path
        })

    def add_warning(self, message: str, file_path: str = ""):
        """Add a warning to the warning list"""
        self.warnings.append({
            'severity': 'warning',
            'message': message,
            'file': file_path
        })
        self.stats['warnings'] += 1

    def load_mkdocs_config(self, config_path: Path) -> Dict:
        """Load mkdocs configuration file"""
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                config = yaml.safe_load(f)
            return config
        except Exception as e:
            print(f"[ERROR] Failed to load mkdocs config from {config_path}: {e}", file=sys.stderr)
            self.add_error(f"Failed to load mkdocs config: {str(e)}", str(config_path))
            return {}

    def extract_nav_files(self, nav_structure, prefix: str = "") -> Set[str]:
        """Extract all file paths from navigation structure"""
        files = set()
        
        if isinstance(nav_structure, list):
            for item in nav_structure:
                files.update(self.extract_nav_files(item, prefix))
        elif isinstance(nav_structure, dict):
            for key, value in nav_structure.items():
                if isinstance(value, str):
                    # This is a file reference
                    files.add(value)
                    self.stats['nav_entries'] += 1
                elif isinstance(value, list) or isinstance(value, dict):
                    files.update(self.extract_nav_files(value, prefix))
        elif isinstance(nav_structure, str):
            files.add(nav_structure)
            self.stats['nav_entries'] += 1
        
        return files

    def scan_markdown_files(self, docs_dir: Path, exclude_patterns: List[str] = None) -> Set[str]:
        """Scan directory for all markdown files"""
        exclude_patterns = exclude_patterns or []
        exclude_dirs = {'node_modules', '.git', 'site', 'build', '__pycache__', 'output', 'ARCHIVED', 'archive'}
        files = set()
        
        if not docs_dir.exists():
            return files
        
        for root, dirs, filenames in os.walk(docs_dir):
            # Filter excluded directories
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            
            root_path = Path(root)
            
            # Check if path should be excluded
            should_exclude = False
            for pattern in exclude_patterns:
                if pattern in str(root_path):
                    should_exclude = True
                    break
            
            if should_exclude:
                continue
            
            for filename in filenames:
                if filename.endswith('.md'):
                    file_path = root_path / filename
                    # Get relative path from docs_dir
                    rel_path = file_path.relative_to(docs_dir)
                    files.add(str(rel_path))
        
        return files

    def validate_toc(self, config_path: Path, exclude_patterns: List[str] = None):
        """Validate TOC against actual files"""
        # Load config
        config = self.load_mkdocs_config(config_path)
        if not config:
            return
        
        # Get docs directory
        docs_dir_name = config.get('docs_dir', 'docs')
        if config_path.parent.name == 'compendium':
            # Special case for compendium
            docs_dir = config_path.parent if docs_dir_name == '.' else config_path.parent / docs_dir_name
        else:
            docs_dir = self.base_path / docs_dir_name
        
        # Extract files from navigation
        nav = config.get('nav', [])
        nav_files = self.extract_nav_files(nav)
        self.stats['files_in_nav'] = len(nav_files)
        
        # Scan actual files
        disk_files = self.scan_markdown_files(docs_dir, exclude_patterns)
        self.stats['files_on_disk'] = len(disk_files)
        
        # Check for missing files (in nav but not on disk)
        for nav_file in nav_files:
            file_path = docs_dir / nav_file
            if not file_path.exists():
                self.add_error(
                    f"File referenced in nav but not found on disk: {nav_file}",
                    str(config_path)
                )
                self.stats['missing_files'] += 1
        
        # Check for orphaned files (on disk but not in nav)
        # Only warn about files in main directories, not all subdirectories
        important_patterns = ['README.md', 'index.md', 'INDEX.md', 'Home.md']
        
        for disk_file in disk_files:
            if disk_file not in nav_files:
                # Check if it's an important file
                filename = Path(disk_file).name
                if any(pattern.lower() == filename.lower() for pattern in important_patterns):
                    self.add_warning(
                        f"Important file not in navigation: {disk_file}",
                        str(config_path)
                    )
                    self.stats['orphaned_files'] += 1

    def check_duplicate_entries(self, config_path: Path):
        """Check for duplicate entries in navigation"""
        config = self.load_mkdocs_config(config_path)
        if not config:
            return
        
        nav = config.get('nav', [])
        nav_files = self.extract_nav_files(nav)
        
        # Count occurrences
        file_counts = {}
        for file in nav_files:
            file_counts[file] = file_counts.get(file, 0) + 1
        
        # Report duplicates
        for file, count in file_counts.items():
            if count > 1:
                self.add_warning(
                    f"File appears {count} times in navigation: {file}",
                    str(config_path)
                )

    def validate_cross_references(self, config_paths: List[Path]):
        """Check cross-references between different documentation sets"""
        # This is a placeholder for more advanced cross-reference checking
        # Could check links between docs and compendium
        pass

    def generate_report(self, output_format: str = 'text') -> str:
        """Generate validation report"""
        if output_format == 'json':
            return json.dumps({
                'stats': self.stats,
                'errors': self.errors,
                'warnings': self.warnings
            }, indent=2)
        
        # Text format
        report = []
        report.append("=" * 80)
        report.append("TOC Validation Report")
        report.append("=" * 80)
        report.append(f"Navigation entries: {self.stats['nav_entries']}")
        report.append(f"Files in navigation: {self.stats['files_in_nav']}")
        report.append(f"Files on disk: {self.stats['files_on_disk']}")
        report.append(f"Missing files: {self.stats['missing_files']}")
        report.append(f"Orphaned important files: {self.stats['orphaned_files']}")
        report.append(f"Warnings: {self.stats['warnings']}")
        report.append("")
        
        if self.errors:
            report.append("ERRORS:")
            report.append("-" * 80)
            for error in self.errors:
                report.append(f"[{error['file']}]")
                report.append(f"  {error['message']}")
                report.append("")
        
        if self.warnings:
            report.append("WARNINGS:")
            report.append("-" * 80)
            for warning in self.warnings:
                report.append(f"[{warning['file']}]")
                report.append(f"  {warning['message']}")
                report.append("")
        
        if not self.errors and not self.warnings:
            report.append("✓ TOC validation passed!")
        
        report.append("=" * 80)
        return '\n'.join(report)


def main():
    parser = argparse.ArgumentParser(
        description='Validate TOC (table of contents) in ThemisDB documentation'
    )
    parser.add_argument(
        '--configs',
        nargs='*',
        default=['mkdocs.yml', 'compendium/mkdocs-compendium.yml'],
        help='MkDocs config files to validate'
    )
    parser.add_argument(
        '--exclude',
        nargs='*',
        default=['ARCHIVED', 'archive', 'node_modules'],
        help='Patterns to exclude from validation'
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
    
    args = parser.parse_args()
    
    # Get base path
    base_path = Path(__file__).parent.parent
    
    # Create validator
    validator = TOCValidator(base_path)
    
    # Validate each config
    for config_str in args.configs:
        config_path = base_path / config_str
        if not config_path.exists():
            print(f"Warning: Config file not found: {config_path}", file=sys.stderr)
            continue
        
        print(f"Validating: {config_path}")
        validator.validate_toc(config_path, args.exclude)
        validator.check_duplicate_entries(config_path)
    
    # Generate report
    report = validator.generate_report(args.format)
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"Report written to: {args.output}")
    else:
        print(report)
    
    # Exit with appropriate code
    if len(validator.errors) > 0:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == '__main__':
    main()
