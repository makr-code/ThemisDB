"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            source_audit.py                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     421                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Source Audit Tool - Analyze source files for common cross-compiler issues

This tool scans C++ source and header files to detect:
- Missing export macros on public symbols
- Platform-specific code without proper guards
- Inline/template instantiation issues
- Compiler-specific intrinsics without fallbacks

Usage:
    python source_audit.py [--output <report_file>] [--json]
"""

import re
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple
from dataclasses import dataclass, asdict
from collections import defaultdict
import json

# Import package constants
try:
    from . import THEMIS_ROOT
except ImportError:
    THEMIS_ROOT = Path(__file__).parent.parent.parent


@dataclass
class SourceIssue:
    """Represents an issue found in a source file"""
    file_path: str
    line_number: int
    issue_type: str
    severity: str  # high, medium, low
    description: str
    suggestion: str


class SourceAuditor:
    """Audit C++ source files for cross-compiler issues"""
    
    # Export macro patterns
    EXPORT_MACROS = [
        r'THEMIS_\w+_API',
        r'__declspec\(dllexport\)',
        r'__declspec\(dllimport\)',
        r'__attribute__\(\(visibility\("default"\)\)\)',
    ]
    
    # Public API patterns (class/function declarations that should be exported)
    PUBLIC_CLASS_PATTERN = re.compile(
        r'^\s*(?:class|struct)\s+(?!THEMIS_\w+_API)(\w+)\s*[:{]',
        re.MULTILINE
    )
    
    PUBLIC_FUNCTION_PATTERN = re.compile(
        r'^\s*(?:(?:inline|static|virtual|explicit|constexpr)\s+)*'
        r'(?:\w+(?:::|<[^>]+>)?(?:\s*\*|\s*&)?)\s+'
        r'(?!THEMIS_\w+_API)(\w+)\s*\([^)]*\)\s*(?:const)?(?:\s*=\s*0)?[;{]',
        re.MULTILINE
    )
    
    # Platform-specific code patterns
    PLATFORM_GUARDS = [
        r'#ifdef\s+(?:_WIN32|WIN32|_WIN64|WINDOWS)',
        r'#ifdef\s+(?:__linux__|LINUX)',
        r'#ifdef\s+(?:__APPLE__|__MACH__|MACOS)',
        r'#ifdef\s+(?:__arm__|__aarch64__|ARM)',
        r'#if\s+defined\(',
    ]
    
    PLATFORM_SPECIFIC_CODE = [
        r'::Windows\b',  # Windows namespace/class — word boundary avoids ::WindowSpec etc.
        r'win32',
        r'HMODULE',
        r'HWND',
        r'pthread_',
        r'<windows\.h>',
        r'<unistd\.h>',
    ]
    
    # Compiler intrinsics
    INTRINSICS = [
        r'__builtin_\w+',
        r'_mm_\w+',
        r'__popcnt',
        r'__sync_\w+',
        r'__atomic_\w+',
        r'_InterlockedExchange',
        r'_BitScanForward',
    ]
    
    # Template patterns
    TEMPLATE_DECLARATION = re.compile(
        r'^\s*template\s*<[^>]+>\s*(?:class|struct|typename)',
        re.MULTILINE
    )
    
    def __init__(self, root_path: Path):
        self.root_path = root_path
        self.issues: List[SourceIssue] = []
        self.stats = defaultdict(int)
        
    def audit_file(self, file_path: Path) -> List[SourceIssue]:
        """Audit a single source file"""
        issues = []
        
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
            return issues
        
        lines = content.split('\n')
        
        # Check for missing export macros
        if file_path.suffix in ['.h', '.hpp']:
            issues.extend(self._check_missing_exports(file_path, content, lines))
        
        # Check for unguarded platform-specific code
        issues.extend(self._check_platform_guards(file_path, content, lines))
        
        # Check for compiler intrinsics without fallbacks
        issues.extend(self._check_intrinsics(file_path, content, lines))
        
        # Check for template issues
        if file_path.suffix in ['.h', '.hpp']:
            issues.extend(self._check_templates(file_path, content, lines))
        
        return issues
    
    def _check_missing_exports(self, file_path: Path, content: str, 
                               lines: List[str]) -> List[SourceIssue]:
        """Check for public APIs without export macros"""
        issues = []
        
        # Skip test files and internal headers
        if any(x in str(file_path).lower() for x in ['test', 'internal', 'detail', 'impl']):
            return issues
        
        # Check if this is a public header (in include/ directory)
        if 'include' not in str(file_path):
            return issues
        
        # Check for classes without export macros
        for match in self.PUBLIC_CLASS_PATTERN.finditer(content):
            class_name = match.group(1)
            line_num = content[:match.start()].count('\n') + 1
            
            # Check if any export macro is present nearby (within 2 lines)
            start_line = max(0, line_num - 3)
            end_line = min(len(lines), line_num + 2)
            nearby_content = '\n'.join(lines[start_line:end_line])
            
            has_export = any(
                re.search(pattern, nearby_content) 
                for pattern in self.EXPORT_MACROS
            )
            
            if not has_export:
                issues.append(SourceIssue(
                    file_path=str(file_path.relative_to(self.root_path)),
                    line_number=line_num,
                    issue_type="MISSING_EXPORT_MACRO",
                    severity="high",
                    description=f"Class '{class_name}' in public header lacks export macro",
                    suggestion=f"Add THEMIS_*_API macro before class declaration"
                ))
                self.stats["missing_export_macros"] += 1
        
        return issues
    
    @staticmethod
    def _blank_non_code(src: str) -> str:
        """Return a copy of *src* where string/char literals, raw string literals,
        block comments (/* … */) and line comments (// …) are replaced with
        whitespace, preserving every newline so that line-number accounting remains
        accurate.

        This eliminates false positives that arise when platform tokens or
        intrinsic-like identifiers appear inside non-code regions (e.g. Prometheus
        metric names starting with '_mm_', GLSL shader source stored in raw
        string literals, or documentation comments).
        """
        out = []
        i = 0
        n = len(src)

        while i < n:
            # --- line comment: // … \n ---
            if src[i:i+2] == '//':
                while i < n and src[i] != '\n':
                    out.append('\n' if src[i] == '\n' else ' ')
                    i += 1
                continue

            # --- block comment: /* … */ ---
            if src[i:i+2] == '/*':
                i += 2
                while i < n:
                    if src[i] == '\n':
                        out.append('\n')
                    else:
                        out.append(' ')
                    if src[i:i+2] == '*/':
                        out.append(' ')  # cover the trailing '/'
                        i += 2
                        break
                    i += 1
                continue

            # --- raw string literal: R"delim( … )delim" ---
            if src[i] == 'R' and i + 1 < n and src[i+1] == '"':
                # Find the opening delimiter: R"DELIM(
                j = i + 2
                delim_start = j
                while j < n and src[j] not in ('(', '\n'):
                    j += 1
                if j < n and src[j] == '(':
                    delim = src[delim_start:j]  # content between " and (
                    closing = ')' + delim + '"'
                    i = j + 1  # skip past '('
                    out.append('R')
                    out.append('"')
                    # blank body, preserve newlines
                    while i < n:
                        if src[i:i+len(closing)] == closing:
                            # write closing sequence as spaces (keep its \n if any)
                            for ch in closing:
                                out.append('\n' if ch == '\n' else ' ')
                            i += len(closing)
                            break
                        out.append('\n' if src[i] == '\n' else ' ')
                        i += 1
                    continue
                # else: not a raw string, fall through to ordinary char handling

            # --- regular string literal: " … " ---
            if src[i] == '"':
                out.append('"')
                i += 1
                while i < n and src[i] != '"':
                    if src[i] == '\\' and i + 1 < n:
                        # escaped character: blank both chars, preserve \n
                        out.append('\n' if src[i] == '\n' else ' ')
                        out.append('\n' if src[i+1] == '\n' else ' ')
                        i += 2
                    else:
                        out.append('\n' if src[i] == '\n' else ' ')
                        i += 1
                if i < n:
                    out.append('"')
                    i += 1
                continue

            # --- character literal: ' … ' ---
            if src[i] == "'":
                out.append("'")
                i += 1
                while i < n and src[i] != "'":
                    if src[i] == '\\' and i + 1 < n:
                        out.append('\n' if src[i] == '\n' else ' ')
                        out.append('\n' if src[i+1] == '\n' else ' ')
                        i += 2
                    else:
                        out.append('\n' if src[i] == '\n' else ' ')
                        i += 1
                if i < n:
                    out.append("'")
                    i += 1
                continue

            out.append(src[i])
            i += 1

        return ''.join(out)

    @staticmethod
    def _build_guarded_lines(lines: List[str]) -> set:
        """Return the set of 1-based line numbers that are nested inside at least
        one preprocessor conditional (#if/#ifdef/#ifndef … #endif).

        Any line inside such a block is considered 'guarded' even if the condition
        is not specifically a platform guard.  This is intentional: a developer
        who writes platform-specific code inside ANY conditional has explicitly
        acknowledged the conditionality.  The only unguarded code we flag is code
        that appears at conditional depth 0.
        """
        guarded: set = set()
        depth = 0
        for lineno, line in enumerate(lines, 1):
            stripped = line.strip()
            if re.match(r'#\s*(?:if|ifdef|ifndef)\b', stripped):
                depth += 1
            if depth > 0:
                guarded.add(lineno)
            if re.match(r'#\s*endif\b', stripped):
                depth = max(0, depth - 1)
        return guarded

    def _check_platform_guards(self, file_path: Path, content: str,
                               lines: List[str]) -> List[SourceIssue]:
        """Check for platform-specific code without proper guards"""
        issues = []

        # Blank strings/comments so platform tokens inside non-code are ignored.
        stripped = self._blank_non_code(content)

        # Pre-compute which lines are inside a preprocessor conditional block.
        guarded = self._build_guarded_lines(lines)

        # Find all platform-specific code locations
        for pattern in self.PLATFORM_SPECIFIC_CODE:
            for match in re.finditer(pattern, stripped, re.IGNORECASE):
                line_num = stripped[:match.start()].count('\n') + 1
                line_text = lines[line_num - 1] if line_num <= len(lines) else ''
                # Skip matches that ARE a preprocessor directive (e.g. "#ifdef _WIN32")
                if re.match(r'^\s*#', line_text):
                    continue
                # Skip matches inside any conditional block
                if line_num in guarded:
                    continue
                issues.append(SourceIssue(
                    file_path=str(file_path.relative_to(self.root_path)),
                    line_number=line_num,
                    issue_type="UNGUARDED_PLATFORM_CODE",
                    severity="medium",
                    description=f"Platform-specific code '{match.group(0)}' without preprocessor guard",
                    suggestion="Add #ifdef for platform (e.g., #ifdef _WIN32)"
                ))
                self.stats["unguarded_platform_code"] += 1

        return issues

    def _check_intrinsics(self, file_path: Path, content: str,
                         lines: List[str]) -> List[SourceIssue]:
        """Check for compiler intrinsics without fallbacks"""
        issues = []

        # Blank strings/comments to avoid matching intrinsic-like tokens inside
        # quoted strings (e.g. Prometheus metric names starting with '_mm_') or
        # block-comment documentation.
        blanked = self._blank_non_code(content)

        # Pre-compute guarded lines (inside any #if/#ifdef/#ifndef block).
        guarded = self._build_guarded_lines(lines)

        for pattern in self.INTRINSICS:
            for match in re.finditer(pattern, blanked):
                line_num = blanked[:match.start()].count('\n') + 1
                intrinsic = match.group(0)

                line_text = lines[line_num - 1] if line_num <= len(lines) else ''
                # Skip intrinsic appearances on preprocessor directive lines
                if re.match(r'^\s*#', line_text):
                    continue
                # A guarded line already has a conditional fallback path
                if line_num in guarded:
                    continue

                issues.append(SourceIssue(
                    file_path=str(file_path.relative_to(self.root_path)),
                    line_number=line_num,
                    issue_type="INTRINSIC_NO_FALLBACK",
                    severity="medium",
                    description=f"Compiler intrinsic '{intrinsic}' without fallback",
                    suggestion="Add preprocessor check and fallback implementation"
                ))
                self.stats["intrinsics_no_fallback"] += 1

        return issues

    
    def _check_templates(self, file_path: Path, content: str,
                        lines: List[str]) -> List[SourceIssue]:
        """Check for template instantiation issues"""
        issues = []
        
        # Check for template declarations in headers
        template_count = len(self.TEMPLATE_DECLARATION.findall(content))
        
        if template_count > 0:
            # Check if there's an explicit instantiation section or .tpp file
            has_instantiation = (
                'extern template' in content or
                file_path.with_suffix('.tpp').exists() or
                file_path.with_suffix('.inl').exists()
            )
            
            if not has_instantiation:
                issues.append(SourceIssue(
                    file_path=str(file_path.relative_to(self.root_path)),
                    line_number=1,
                    issue_type="TEMPLATE_NO_INSTANTIATION",
                    severity="low",
                    description=f"Header has {template_count} template(s) without explicit instantiation",
                    suggestion="Consider adding explicit instantiations or .tpp file"
                ))
                self.stats["templates_no_instantiation"] += 1
        
        return issues
    
    def audit_directory(self, extensions: List[str] = None) -> None:
        """Audit all C++ files in the directory tree"""
        if extensions is None:
            extensions = ['.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx']
        
        # Find all source files
        source_files = []
        for ext in extensions:
            source_files.extend(self.root_path.rglob(f'*{ext}'))
        
        print(f"Auditing {len(source_files)} source files...")
        
        for i, file_path in enumerate(source_files):
            if i % 100 == 0:
                print(f"  Progress: {i}/{len(source_files)}")
            
            issues = self.audit_file(file_path)
            self.issues.extend(issues)
        
        print(f"Audit complete. Found {len(self.issues)} issues.")
    
    def generate_report(self, output_path: Path) -> None:
        """Generate a markdown report of audit findings"""
        report = []
        report.append("# ThemisDB Source Code Audit Report\n")
        report.append(f"Generated: {Path.cwd()}\n")
        report.append(f"Total Issues: {len(self.issues)}\n\n")
        
        # Summary statistics
        report.append("## Summary Statistics\n\n")
        for key, value in sorted(self.stats.items()):
            report.append(f"- **{key.replace('_', ' ').title()}**: {value}\n")
        report.append("\n")
        
        # Issues by severity
        by_severity = defaultdict(list)
        for issue in self.issues:
            by_severity[issue.severity].append(issue)
        
        for severity in ['high', 'medium', 'low']:
            if severity in by_severity:
                report.append(f"## {severity.upper()} Priority Issues ({len(by_severity[severity])})\n\n")
                
                # Group by issue type
                by_type = defaultdict(list)
                for issue in by_severity[severity]:
                    by_type[issue.issue_type].append(issue)
                
                for issue_type, issues in sorted(by_type.items()):
                    report.append(f"### {issue_type.replace('_', ' ').title()} ({len(issues)})\n\n")
                    
                    # Show first 20 issues of this type
                    for issue in issues[:20]:
                        report.append(f"**{issue.file_path}:{issue.line_number}**\n")
                        report.append(f"- Description: {issue.description}\n")
                        report.append(f"- Suggestion: {issue.suggestion}\n\n")
                    
                    if len(issues) > 20:
                        report.append(f"... and {len(issues) - 20} more\n\n")
        
        # Top problematic files
        file_issue_count = defaultdict(int)
        for issue in self.issues:
            file_issue_count[issue.file_path] += 1
        
        report.append("## Top 20 Problematic Files\n\n")
        report.append("| File | Issues |\n")
        report.append("|------|--------|\n")
        for file_path, count in sorted(file_issue_count.items(), 
                                      key=lambda x: x[1], reverse=True)[:20]:
            report.append(f"| {file_path} | {count} |\n")
        report.append("\n")
        
        # Write report
        output_path.write_text(''.join(report))
        print(f"Report written to {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Audit C++ source files for cross-compiler issues"
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=THEMIS_ROOT,
        help="Root directory to audit"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=THEMIS_ROOT / "docs" / "SOURCE_AUDIT_REPORT.md",
        help="Output report file"
    )
    parser.add_argument(
        "--json",
        type=Path,
        help="Also output results as JSON"
    )
    parser.add_argument(
        "--include",
        nargs='+',
        help="Only audit files matching these patterns"
    )
    
    args = parser.parse_args()
    
    auditor = SourceAuditor(args.root)
    auditor.audit_directory()
    auditor.generate_report(args.output)
    
    # Output JSON if requested
    if args.json:
        output_data = {
            "issues": [asdict(issue) for issue in auditor.issues],
            "stats": dict(auditor.stats),
            "total_issues": len(auditor.issues)
        }
        args.json.write_text(json.dumps(output_data, indent=2))
        print(f"JSON output written to {args.json}")
    
    return 0


if __name__ == "__main__":
    exit(main())
