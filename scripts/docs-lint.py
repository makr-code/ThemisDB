"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs-lint.py                                       ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     491                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7776caef87  2026-03-09  feat: add doc metadata & drift conventions (DOC_METADATA.... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import json

try:
    import yaml
    _YAML_AVAILABLE = True
except ImportError:
    _YAML_AVAILABLE = False

# Valid values for the `status` metadata field
VALID_DOC_STATUSES = {"current", "drifting", "stale", "archived"}

# Default directories that require YAML front-matter metadata
DEFAULT_METADATA_PATHS = [
    "docs/de/development",
    "docs/de/features",
    "docs/de/guides",
    "docs/de/architecture",
    "docs/de/security",
]


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

    def _parse_frontmatter(self, lines: List[str]) -> Optional[Dict]:
        """Parse YAML front matter from a list of lines.

        Returns the parsed dict on success, or None if no valid front matter
        was found.  The YAML block must start on line 1 (index 0) and be
        delimited by ``---`` markers.
        """
        if not lines or lines[0].strip() != "---":
            return None

        end_index = None
        for i in range(1, len(lines)):
            if lines[i].strip() == "---":
                end_index = i
                break

        if end_index is None:
            return None

        yaml_text = "".join(lines[1:end_index])

        if _YAML_AVAILABLE:
            try:
                data = yaml.safe_load(yaml_text)
                return data if isinstance(data, dict) else None
            except yaml.YAMLError:
                return None
        else:
            # Minimal fallback: parse simple "key: value" pairs without PyYAML.
            # Only simple scalar values on a single line are supported; complex
            # (multi-line / nested) YAML structures are silently ignored.
            print(
                "Warning: PyYAML not available; YAML front-matter parsed with limited "
                "fallback parser. Install pyyaml for full support.",
                file=sys.stderr,
            )
            data = {}
            for line in lines[1:end_index]:
                # Match: key: value  OR  key: "value"  OR  key: 'value'
                m = re.match(
                    r'^([A-Za-z_][A-Za-z0-9_]*):\s*'
                    r'(?:"([^"]*)"'          # double-quoted
                    r"|'([^']*)'"            # single-quoted
                    r'|([^#\n]*))'           # unquoted
                    r'\s*(?:#.*)?$',
                    line,
                )
                if m:
                    key = m.group(1).strip()
                    value = (m.group(2) or m.group(3) or (m.group(4) or "")).strip()
                    data[key] = value
            return data if data else None

    def check_doc_metadata(self, file_path: Path, lines: List[str]):
        """Validate required YAML front-matter metadata fields.

        Required fields:
        - ``status``: one of current | drifting | stale | archived
        - ``doc_version``: non-empty semver tag or branch name

        Optional:
        - ``validated``: date in YYYY-MM-DD format
        """
        frontmatter = self._parse_frontmatter(lines)

        if frontmatter is None:
            self.add_error(
                str(file_path),
                0,
                "Missing YAML front matter (required: status, doc_version)"
            )
            return

        # Validate `status`
        status = frontmatter.get("status")
        if not status:
            self.add_error(str(file_path), 0, "Metadata field 'status' is missing")
        elif str(status) not in VALID_DOC_STATUSES:
            self.add_error(
                str(file_path),
                0,
                f"Invalid metadata 'status': '{status}' "
                f"(allowed: {', '.join(sorted(VALID_DOC_STATUSES))})"
            )

        # Validate `doc_version`
        doc_version = frontmatter.get("doc_version")
        if not doc_version:
            self.add_error(str(file_path), 0, "Metadata field 'doc_version' is missing")

        # Validate optional `validated` field format
        validated = frontmatter.get("validated")
        if validated is not None:
            date_str = str(validated)
            try:
                datetime.strptime(date_str, "%Y-%m-%d")
            except ValueError:
                self.add_warning(
                    str(file_path),
                    0,
                    f"Metadata field 'validated' must be a valid date in YYYY-MM-DD format, "
                    f"got: '{date_str}'"
                )

    def lint_file(self, file_path: Path, check_metadata: bool = False):
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
            if check_metadata:
                self.check_doc_metadata(file_path, lines)
            
        except UnicodeDecodeError:
            self.add_error(str(file_path), 0, "File encoding error (not UTF-8)")
        except Exception as e:
            print(f"[ERROR] Unexpected error in {file_path}: {e}", file=sys.stderr)
            self.add_error(str(file_path), 0, f"Unexpected error: {str(e)}")

    def lint_directory(self, directory: Path, exclude_patterns: List[str] = None,
                       check_metadata: bool = False):
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
                    self.lint_file(file_path, check_metadata=check_metadata)

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
    parser.add_argument(
        '--check-metadata',
        action='store_true',
        help=(
            'Enforce YAML front-matter metadata (status, doc_version) on Secondary Docs. '
            'Applies to paths given by --metadata-paths.'
        )
    )
    parser.add_argument(
        '--metadata-paths',
        nargs='*',
        default=DEFAULT_METADATA_PATHS,
        metavar='PATH',
        help=(
            'Directories that require YAML front-matter metadata when --check-metadata is set '
            f'(default: {", ".join(DEFAULT_METADATA_PATHS)})'
        )
    )
    
    args = parser.parse_args()
    
    # Get base path
    base_path = Path(__file__).parent.parent
    
    # Create linter
    linter = DocumentationLinter(base_path)

    if args.check_metadata:
        # Metadata-check mode: scan only the designated secondary-doc paths
        # and enforce YAML front-matter on every file found there.
        for mp_str in args.metadata_paths:
            mp = base_path / mp_str
            if not mp.exists():
                print(f"Warning: Metadata path does not exist: {mp}", file=sys.stderr)
                continue
            if mp.is_file():
                linter.lint_file(mp, check_metadata=True)
            else:
                linter.lint_directory(mp, args.exclude, check_metadata=True)
    else:
        # Standard lint mode: scan the positional paths without metadata checks.
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
