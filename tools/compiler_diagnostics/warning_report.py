"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            warning_report.py                                  ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:10:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     419                                            ║
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
Warning Report Generator - Analyze and categorize compiler warnings

This tool extends diagnostic_scanner.py to specifically track and report
on compiler warnings (C4244, C4267, C4018, C4100, C4101, etc.)

Usage:
    python warning_report.py <log_file> [--output <report.md>]
    python warning_report.py --scan-source [--fix-suggestions]
"""

import re
import argparse
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from collections import defaultdict

try:
    from . import THEMIS_ROOT
except ImportError:
    THEMIS_ROOT = Path(__file__).parent.parent.parent


@dataclass
class CompilerWarning:
    """Represents a single compiler warning"""
    file_path: str
    line_number: int
    warning_code: str  # C4244, C4267, etc.
    severity: str
    message: str
    category: str
    suggested_fix: Optional[str] = None


class WarningPattern:
    """Patterns for detecting specific warning types"""
    
    PATTERNS = {
        "C4244": {
            "regex": r"C4244|conversion from '(\w+)' to '(\w+)', possible loss of data",
            "category": "Type Conversion - Narrowing",
            "description": "Conversion with possible data loss (e.g., double → float)",
            "fix_template": "Use safe_double_to_float() or clamp_double_to_float()"
        },
        "C4267": {
            "regex": r"C4267|conversion from 'size_t' to '\w+', possible loss of data",
            "category": "Type Conversion - size_t",
            "description": "Conversion from size_t to smaller integer type",
            "fix_template": "Use safe_size_to_int() or safe_size_to_int32()"
        },
        "C4018": {
            "regex": r"C4018|signed/unsigned mismatch",
            "category": "Signed/Unsigned Mismatch",
            "description": "Signed/unsigned comparison or operation",
            "fix_template": "Use size_t for loop counters, or static_cast with validation"
        },
        "C4100": {
            "regex": r"C4100|unreferenced formal parameter",
            "category": "Unreferenced Parameter",
            "description": "Function parameter not used in body",
            "fix_template": "Use [[maybe_unused]] attribute or (void)param;"
        },
        "C4101": {
            "regex": r"C4101|unreferenced local variable",
            "category": "Unreferenced Variable",
            "description": "Local variable declared but not used",
            "fix_template": "Remove variable or use [[maybe_unused]] if conditionally used"
        },
        "C4189": {
            "regex": r"C4189|local variable is initialized but not referenced",
            "category": "Unreferenced Variable",
            "description": "Local variable initialized but never used",
            "fix_template": "Remove variable or verify intended usage"
        },
        "C4305": {
            "regex": r"C4305|truncation from '(\w+)' to '(\w+)'",
            "category": "Type Conversion - Truncation",
            "description": "Truncation in initialization (e.g., double literal → float)",
            "fix_template": "Use float literals (3.14f) or safe conversion"
        },
        "C4996": {
            "regex": r"C4996|deprecated",
            "category": "Deprecated API",
            "description": "Use of deprecated functions",
            "fix_template": "Replace with modern equivalent as suggested"
        }
    }
    
    @classmethod
    def categorize(cls, message: str) -> Tuple[str, str, str]:
        """
        Categorize a warning message
        Returns: (warning_code, category, suggested_fix)
        """
        for code, pattern_info in cls.PATTERNS.items():
            if re.search(pattern_info["regex"], message, re.IGNORECASE):
                return (code, pattern_info["category"], pattern_info["fix_template"])
        return ("UNKNOWN", "Other Warning", "Manual review required")


class SourceScanner:
    """Scan source files for potential warning sources"""
    
    def __init__(self, root_path: Path):
        self.root_path = root_path
        self.issues: List[Dict] = []
    
    def scan_static_casts(self) -> List[Dict]:
        """Find static_cast<int>() from size_t"""
        pattern = re.compile(r'static_cast<int[^>]*>\s*\(\s*[^)]*\.size\(\)|static_cast<int[^>]*>\s*\(\s*sizeof')
        issues = []
        
        for cpp_file in self.root_path.rglob("*.cpp"):
            try:
                content = cpp_file.read_text(encoding='utf-8', errors='ignore')
                for line_num, line in enumerate(content.split('\n'), 1):
                    if pattern.search(line):
                        issues.append({
                            "file": str(cpp_file.relative_to(self.root_path)),
                            "line": line_num,
                            "type": "static_cast<int> from size_t",
                            "code_snippet": line.strip()[:80],
                            "suggested_fix": "Replace with safe_size_to_int()"
                        })
            except Exception as e:
                print(f"[WARN] scan_static_casts: skipping file due to error: {e}")
        
        return issues
    
    def scan_float_assignments(self) -> List[Dict]:
        """Find float assignments from double"""
        pattern = re.compile(r'float\s+\w+\s*=\s*[^;]*(?<!f)[0-9]+\.[0-9]+[^;]*;')
        issues = []
        
        for cpp_file in self.root_path.rglob("*.cpp"):
            try:
                content = cpp_file.read_text(encoding='utf-8', errors='ignore')
                for line_num, line in enumerate(content.split('\n'), 1):
                    if pattern.search(line) and 'safe_double_to_float' not in line:
                        issues.append({
                            "file": str(cpp_file.relative_to(self.root_path)),
                            "line": line_num,
                            "type": "float assignment from double",
                            "code_snippet": line.strip()[:80],
                            "suggested_fix": "Use safe_double_to_float() or add 'f' suffix"
                        })
            except Exception as e:
                print(f"[WARN] scan_float_assignments: skipping file due to error: {e}")
        
        return issues
    
    def scan_pragma_suppressions(self) -> List[Dict]:
        """Find pragma warning suppressions"""
        pattern = re.compile(r'#pragma\s+warning\s*\(\s*disable\s*:\s*(\d+)', re.IGNORECASE)
        issues = []
        
        for file_path in list(self.root_path.rglob("*.cpp")) + list(self.root_path.rglob("*.h")):
            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')
                for line_num, line in enumerate(content.split('\n'), 1):
                    match = pattern.search(line)
                    if match:
                        warning_code = match.group(1)
                        issues.append({
                            "file": str(file_path.relative_to(self.root_path)),
                            "line": line_num,
                            "type": f"pragma disable warning C{warning_code}",
                            "code_snippet": line.strip(),
                            "suggested_fix": "Replace pragma with proper fix or [[maybe_unused]]"
                        })
            except Exception as e:
                print(f"[WARN] scan_pragma_suppressions: skipping file due to error: {e}")
        
        return issues
    
    def scan_signed_unsigned_loops(self) -> List[Dict]:
        """Find int loops comparing with .size()"""
        pattern = re.compile(r'for\s*\(\s*int\s+\w+\s*=.*\w+\s*<\s*[^;]*\.size\(\)')
        issues = []
        
        for cpp_file in self.root_path.rglob("*.cpp"):
            try:
                content = cpp_file.read_text(encoding='utf-8', errors='ignore')
                for line_num, line in enumerate(content.split('\n'), 1):
                    if pattern.search(line):
                        issues.append({
                            "file": str(cpp_file.relative_to(self.root_path)),
                            "line": line_num,
                            "type": "int loop variable vs size_t comparison",
                            "code_snippet": line.strip()[:80],
                            "suggested_fix": "Use size_t or range-based for loop"
                        })
            except Exception as e:
                print(f"[WARN] scan_signed_unsigned_loops: skipping file due to error: {e}")
        
        return issues


class ReportGenerator:
    """Generate markdown reports for compiler warnings"""
    
    def __init__(self):
        self.warnings: List[CompilerWarning] = []
        self.source_issues: Dict[str, List[Dict]] = {}
    
    def add_warnings(self, warnings: List[CompilerWarning]):
        """Add warnings from log parsing"""
        self.warnings.extend(warnings)
    
    def add_source_issues(self, issue_type: str, issues: List[Dict]):
        """Add issues from source scanning"""
        self.source_issues[issue_type] = issues
    
    def generate_markdown(self) -> str:
        """Generate comprehensive markdown report"""
        report = []
        
        # Header
        report.append("# Compiler Warnings Report\n")
        report.append(f"Generated: {Path.cwd()}\n")
        report.append("---\n\n")
        
        # Summary statistics
        if self.warnings:
            report.append("## Summary from Build Logs\n")
            by_code = defaultdict(int)
            by_category = defaultdict(int)
            
            for warning in self.warnings:
                by_code[warning.warning_code] += 1
                by_category[warning.category] += 1
            
            report.append("### Warnings by Code\n")
            for code, count in sorted(by_code.items(), key=lambda x: x[1], reverse=True):
                report.append(f"- **{code}**: {count} occurrences\n")
            
            report.append("\n### Warnings by Category\n")
            for category, count in sorted(by_category.items(), key=lambda x: x[1], reverse=True):
                report.append(f"- **{category}**: {count} occurrences\n")
            
            report.append("\n")
        
        # Source code analysis
        if self.source_issues:
            report.append("## Source Code Analysis\n\n")
            
            for issue_type, issues in self.source_issues.items():
                if not issues:
                    continue
                
                report.append(f"### {issue_type.replace('_', ' ').title()}\n")
                report.append(f"Found **{len(issues)}** instances\n\n")
                
                # Group by file
                by_file = defaultdict(list)
                for issue in issues:
                    by_file[issue["file"]].append(issue)
                
                # Show top 10 files
                sorted_files = sorted(by_file.items(), key=lambda x: len(x[1]), reverse=True)
                report.append("**Top affected files:**\n")
                for file_path, file_issues in sorted_files[:10]:
                    report.append(f"- `{file_path}`: {len(file_issues)} issues\n")
                
                if len(sorted_files) > 10:
                    report.append(f"- ... and {len(sorted_files) - 10} more files\n")
                
                report.append("\n")
        
        # Recommendations
        report.append("## Recommendations\n\n")
        report.append("### Priority 1: Critical Type Conversions\n")
        report.append("- Migrate `static_cast<int>(size_t)` to `safe_size_to_int()`\n")
        report.append("- Fix `double → float` with `safe_double_to_float()`\n")
        report.append("- Review all pragma warning suppressions\n\n")
        
        report.append("### Priority 2: Code Quality\n")
        report.append("- Mark unused parameters with `[[maybe_unused]]`\n")
        report.append("- Remove genuinely unused local variables\n")
        report.append("- Use `size_t` for container iteration\n\n")
        
        report.append("### Priority 3: Best Practices\n")
        report.append("- Adopt range-based for loops where possible\n")
        report.append("- Document why parameters must remain unused\n")
        report.append("- Add CI checks to prevent regression\n\n")
        
        # Migration guide reference
        report.append("## Migration Guide\n\n")
        report.append("See `docs/de/guides/TYPE_CONVERSION_GUIDE.md` for detailed examples.\n\n")
        report.append("### Quick Reference\n\n")
        report.append("```cpp\n")
        report.append("// BEFORE (warnings)\n")
        report.append("int count = vector.size();              // C4267\n")
        report.append("float val = some_double;                // C4244\n")
        report.append("for (int i = 0; i < vec.size(); ++i)   // C4018\n\n")
        report.append("// AFTER (safe)\n")
        report.append("#include \"utils/type_conversion.h\"\n")
        report.append("using themis::utils::conversion::safe_size_to_int;\n")
        report.append("using themis::utils::conversion::safe_double_to_float;\n\n")
        report.append("int count = safe_size_to_int(vector.size());\n")
        report.append("float val = safe_double_to_float(some_double, true);\n")
        report.append("for (size_t i = 0; i < vec.size(); ++i)\n")
        report.append("```\n")
        
        return "".join(report)
    
    def write_report(self, output_path: Path):
        """Write report to file"""
        report_content = self.generate_markdown()
        output_path.write_text(report_content, encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(
        description="Generate compiler warning reports and fix suggestions"
    )
    parser.add_argument(
        "--scan-source",
        action="store_true",
        help="Scan source files for potential warning sources"
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=THEMIS_ROOT,
        help="Root directory to scan (default: ThemisDB root)"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=THEMIS_ROOT / "docs" / "de" / "reports" / "COMPILER_WARNINGS_REPORT.md",
        help="Output markdown report file"
    )
    parser.add_argument(
        "--json",
        type=Path,
        help="Also output results as JSON"
    )
    
    args = parser.parse_args()
    
    # Create report generator
    report_gen = ReportGenerator()
    
    # Scan source files if requested
    if args.scan_source:
        print("Scanning source files for potential warnings...")
        scanner = SourceScanner(args.root / "src")
        
        print("  - Scanning static_cast<int>() from size_t...")
        static_cast_issues = scanner.scan_static_casts()
        report_gen.add_source_issues("static_cast_size_t", static_cast_issues)
        print(f"    Found {len(static_cast_issues)} instances")
        
        print("  - Scanning float assignments from double...")
        float_issues = scanner.scan_float_assignments()
        report_gen.add_source_issues("float_from_double", float_issues)
        print(f"    Found {len(float_issues)} instances")
        
        print("  - Scanning pragma warning suppressions...")
        pragma_issues = scanner.scan_pragma_suppressions()
        report_gen.add_source_issues("pragma_suppressions", pragma_issues)
        print(f"    Found {len(pragma_issues)} instances")
        
        print("  - Scanning signed/unsigned loop comparisons...")
        loop_issues = scanner.scan_signed_unsigned_loops()
        report_gen.add_source_issues("signed_unsigned_loops", loop_issues)
        print(f"    Found {len(loop_issues)} instances")
    
    # Generate report
    print(f"\nGenerating report...")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    report_gen.write_report(args.output)
    print(f"Report written to {args.output}")
    
    # Output JSON if requested
    if args.json:
        output_data = {
            "source_issues": report_gen.source_issues,
            "summary": {
                issue_type: len(issues)
                for issue_type, issues in report_gen.source_issues.items()
            }
        }
        args.json.write_text(json.dumps(output_data, indent=2), encoding='utf-8')
        print(f"JSON output written to {args.json}")
    
    return 0


if __name__ == "__main__":
    exit(main())
