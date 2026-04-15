"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            source_audit.py                                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     421                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
        r'::Windows',
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
    
    def _check_platform_guards(self, file_path: Path, content: str,
                               lines: List[str]) -> List[SourceIssue]:
        """Check for platform-specific code without proper guards"""
        issues = []
        
        # Find all platform-specific code locations
        platform_code_locations = []
        for pattern in self.PLATFORM_SPECIFIC_CODE:
            for match in re.finditer(pattern, content, re.IGNORECASE):
                line_num = content[:match.start()].count('\n') + 1
                platform_code_locations.append((line_num, match.group(0)))
        
        # Check if each location is within a platform guard
        for line_num, code in platform_code_locations:
            # Look backwards for a platform guard
            has_guard = False
            for i in range(max(0, line_num - 20), line_num):
                if i < len(lines):
                    for guard_pattern in self.PLATFORM_GUARDS:
                        if re.search(guard_pattern, lines[i]):
                            has_guard = True
                            break
                if has_guard:
                    break
            
            if not has_guard:
                issues.append(SourceIssue(
                    file_path=str(file_path.relative_to(self.root_path)),
                    line_number=line_num,
                    issue_type="UNGUARDED_PLATFORM_CODE",
                    severity="medium",
                    description=f"Platform-specific code '{code}' without preprocessor guard",
                    suggestion="Add #ifdef for platform (e.g., #ifdef _WIN32)"
                ))
                self.stats["unguarded_platform_code"] += 1
        
        return issues
    
    def _check_intrinsics(self, file_path: Path, content: str,
                         lines: List[str]) -> List[SourceIssue]:
        """Check for compiler intrinsics without fallbacks"""
        issues = []
        
        for pattern in self.INTRINSICS:
            for match in re.finditer(pattern, content):
                line_num = content[:match.start()].count('\n') + 1
                intrinsic = match.group(0)
                
                # Check if there's a fallback implementation nearby
                # Look for #ifdef or #if defined within 10 lines before
                has_fallback = False
                for i in range(max(0, line_num - 10), line_num):
                    if i < len(lines):
                        if re.search(r'#(?:if|ifdef|ifndef|else)', lines[i]):
                            has_fallback = True
                            break
                
                if not has_fallback:
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
