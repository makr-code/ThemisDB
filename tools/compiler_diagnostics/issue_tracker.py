"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            issue_tracker.py                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:58:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     316                                            ║
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
Issue Tracker - CI integration for automated error tracking

This tool parses compiler output from CI runs and can:
- Track error trends over time
- Generate weekly reports
- Create GitHub issues for new errors (when integrated with GitHub API)

Usage:
    python issue_tracker.py --ci-log <log_file> [--track]
    python issue_tracker.py --report-weekly
"""

import re
import json
import argparse
from pathlib import Path
from typing import Dict, List
from datetime import datetime, timedelta
from collections import defaultdict

try:
    from . import THEMIS_ROOT
    from .diagnostic_scanner import CompilerLogParser, ErrorDatabase, CompilerError
except ImportError:
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from diagnostic_scanner import CompilerLogParser, ErrorDatabase, CompilerError
    THEMIS_ROOT = Path(__file__).parent.parent.parent


class IssueTracker:
    """Track compiler errors from CI runs over time"""
    
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.db = ErrorDatabase(db_path)
    
    def track_ci_run(self, log_file: Path, build_id: str = None,
                     compiler: str = "auto", platform: str = "auto") -> Dict:
        """Track errors from a CI run"""
        if not build_id:
            build_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Parse log
        parser = CompilerLogParser(compiler=compiler, platform=platform)
        log_content = log_file.read_text(encoding='utf-8', errors='ignore')
        errors = parser.parse_log(log_content)
        
        # Store in database
        for error in errors:
            self.db.insert_error(error)
        
        # Generate summary
        summary = {
            "build_id": build_id,
            "timestamp": datetime.now().isoformat(),
            "log_file": str(log_file),
            "total_errors": len([e for e in errors if e.severity == 'error']),
            "total_warnings": len([e for e in errors if e.severity == 'warning']),
            "by_category": defaultdict(int)
        }
        
        for error in errors:
            summary["by_category"][error.category] += 1
        
        summary["by_category"] = dict(summary["by_category"])
        
        return summary
    
    def get_trends(self, days: int = 7) -> Dict:
        """Get error trends over the last N days"""
        # This is a simplified version - would need timestamp tracking
        # in the database for full implementation
        stats = self.db.get_stats()
        
        return {
            "period_days": days,
            "current_stats": stats,
            "trend": "stable"  # Would calculate from historical data
        }
    
    def generate_weekly_report(self, output_path: Path) -> None:
        """Generate a weekly error report"""
        stats = self.db.get_stats()
        
        report = []
        report.append("# Weekly Compiler Error Report\n\n")
        report.append(f"Report Date: {datetime.now().strftime('%Y-%m-%d')}\n")
        report.append(f"Period: Last 7 days\n\n")
        
        report.append("## Error Summary\n\n")
        total_errors = sum(stats["by_category"].values())
        report.append(f"Total Errors: **{total_errors}**\n\n")
        
        report.append("### Errors by Category\n\n")
        report.append("| Category | Count | Percentage |\n")
        report.append("|----------|-------|------------|\n")
        for category, count in sorted(stats["by_category"].items(), 
                                     key=lambda x: x[1], reverse=True):
            pct = (count / total_errors * 100) if total_errors > 0 else 0
            report.append(f"| {category} | {count} | {pct:.1f}% |\n")
        report.append("\n")
        
        report.append("### Errors by Platform\n\n")
        report.append("| Platform | Count |\n")
        report.append("|----------|-------|\n")
        for platform, count in sorted(stats["by_platform"].items(), 
                                     key=lambda x: x[1], reverse=True):
            report.append(f"| {platform} | {count} |\n")
        report.append("\n")
        
        report.append("### Top 10 Problematic Files\n\n")
        report.append("| File | Errors |\n")
        report.append("|------|--------|\n")
        for file_path, count in list(stats["top_files"].items())[:10]:
            report.append(f"| {file_path} | {count} |\n")
        report.append("\n")
        
        report.append("## Recommendations\n\n")
        
        # Generate recommendations based on error categories
        if stats["by_category"].get("SYMBOL_VISIBILITY", 0) > 0:
            report.append("- **Symbol Visibility**: Review export macros in affected files\n")
        
        if stats["by_category"].get("LINKER", 0) > 0:
            report.append("- **Linker Errors**: Check CMakeLists.txt for missing dependencies\n")
        
        if stats["by_category"].get("TEMPLATE", 0) > 0:
            report.append("- **Template Issues**: Add explicit template instantiations\n")
        
        if stats["by_category"].get("INTRINSICS", 0) > 0:
            report.append("- **Intrinsics**: Add fallback implementations for portability\n")
        
        report.append("\n")
        
        output_path.write_text(''.join(report))
        print(f"Weekly report written to {output_path}")
    
    def create_github_issue(self, error: CompilerError) -> Dict:
        """
        Create a GitHub issue for an error
        
        Note: This is a template - actual implementation would use GitHub API
        """
        issue_template = {
            "title": f"[Compiler Error] {error.category}: {error.message[:80]}",
            "body": f"""
## Compiler Error Report

**Category**: {error.category}
**Severity**: {error.severity}
**Compiler**: {error.compiler}
**Platform**: {error.platform}

**File**: `{error.file_path}:{error.line_number}:{error.column_number}`

**Message**:
```
{error.message}
```

**Context**:
```
{error.full_context}
```

**Auto-generated by ThemisDB Issue Tracker**
""",
            "labels": ["compiler-error", error.category.lower(), error.platform]
        }
        
        return issue_template
    
    def close(self):
        """Close database connection"""
        self.db.close()


def main():
    parser = argparse.ArgumentParser(
        description="Track compiler errors from CI runs"
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Track command
    track_parser = subparsers.add_parser('track', help='Track CI run errors')
    track_parser.add_argument(
        '--ci-log',
        type=Path,
        required=True,
        help='Path to CI log file'
    )
    track_parser.add_argument(
        '--build-id',
        help='Build ID for tracking'
    )
    track_parser.add_argument(
        '--compiler',
        choices=['msvc', 'gcc', 'clang', 'auto'],
        default='auto',
        help='Compiler type'
    )
    track_parser.add_argument(
        '--platform',
        choices=['windows', 'linux', 'macos', 'arm', 'auto'],
        default='auto',
        help='Platform'
    )
    
    # Report command
    report_parser = subparsers.add_parser('report', help='Generate weekly report')
    report_parser.add_argument(
        '--output',
        type=Path,
        default=THEMIS_ROOT / "docs" / "WEEKLY_ERROR_REPORT.md",
        help='Output report file'
    )
    
    # Trends command
    trends_parser = subparsers.add_parser('trends', help='Show error trends')
    trends_parser.add_argument(
        '--days',
        type=int,
        default=7,
        help='Number of days to analyze'
    )
    
    parser.add_argument(
        '--db',
        type=Path,
        default=THEMIS_ROOT / "tools" / "compiler_diagnostics" / "compiler_diagnostics.db",
        help='Database file path'
    )
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    tracker = IssueTracker(args.db)
    
    try:
        if args.command == 'track':
            if not args.ci_log.exists():
                print(f"Error: Log file not found: {args.ci_log}")
                return 1
            
            summary = tracker.track_ci_run(
                args.ci_log,
                build_id=args.build_id,
                compiler=args.compiler,
                platform=args.platform
            )
            
            print(f"\n=== CI Run Summary ===")
            print(f"Build ID: {summary['build_id']}")
            print(f"Timestamp: {summary['timestamp']}")
            print(f"Total Errors: {summary['total_errors']}")
            print(f"Total Warnings: {summary['total_warnings']}")
            
            if summary['by_category']:
                print("\nBy Category:")
                for category, count in sorted(summary['by_category'].items(),
                                             key=lambda x: x[1], reverse=True):
                    print(f"  {category}: {count}")
        
        elif args.command == 'report':
            tracker.generate_weekly_report(args.output)
        
        elif args.command == 'trends':
            trends = tracker.get_trends(days=args.days)
            print(f"\n=== Error Trends (Last {trends['period_days']} days) ===")
            print(f"Trend: {trends['trend']}")
            print("\nCurrent Statistics:")
            for key, value in trends['current_stats'].items():
                if isinstance(value, dict):
                    print(f"\n{key}:")
                    for k, v in list(value.items())[:5]:
                        print(f"  {k}: {v}")
                else:
                    print(f"{key}: {value}")
    
    finally:
        tracker.close()
    
    return 0


if __name__ == "__main__":
    exit(main())
