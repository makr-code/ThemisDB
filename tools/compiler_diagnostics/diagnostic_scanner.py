"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            diagnostic_scanner.py                              ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:10:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     502                                            ║
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
Diagnostic Scanner - Parse and categorize compiler error logs

This tool parses compiler error logs from various compilers (MSVC, GCC, Clang)
and categorizes errors into actionable groups for systematic debugging.

Usage:
    python diagnostic_scanner.py <log_file> [--output <output_file>]
    python diagnostic_scanner.py --scan-workflows [--days <n>]
"""

import re
import json
import argparse
import sqlite3
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime
from collections import defaultdict

# Import package constants
try:
    from . import ERROR_CATEGORIES, THEMIS_ROOT
except ImportError:
    # Fallback for standalone execution
    ERROR_CATEGORIES = {
        "SYMBOL_VISIBILITY": "Symbol visibility and export issues",
        "LINKER": "Linker errors and undefined references",
        "ABI": "ABI compatibility issues",
        "PLATFORM_SPECIFIC": "Platform-specific code problems",
        "TEMPLATE": "Template instantiation issues",
        "INTRINSICS": "Compiler intrinsics without fallbacks",
        "STANDARD_LIBRARY": "Standard library compatibility",
        "WARNING": "Compiler warnings",
        "OTHER": "Uncategorized errors"
    }
    THEMIS_ROOT = Path(__file__).parent.parent.parent


@dataclass
class CompilerError:
    """Represents a single compiler error or warning"""
    file_path: str
    line_number: int
    column_number: int
    severity: str  # error, warning, note
    message: str
    category: str
    compiler: str  # msvc, gcc, clang
    platform: str  # windows, linux, macos, arm
    full_context: str


class ErrorPattern:
    """Patterns for categorizing compiler errors"""
    
    PATTERNS = {
        "SYMBOL_VISIBILITY": [
            r"undefined reference to",
            r"unresolved external symbol",
            r"symbol\(s\) not found",
            r"__declspec\(dllexport\)",
            r"__declspec\(dllimport\)",
            r"visibility attribute",
            r"hidden symbol.*cannot be undefined",
        ],
        "LINKER": [
            r"ld returned \d+ exit status",
            r"link\.exe.*failed",
            r"cannot find -l",
            r"undefined reference",
            r"multiply defined",
            r"fatal error LNK\d+",
            r"duplicate symbol",
        ],
        "TEMPLATE": [
            r"undefined.*template",
            r"explicit instantiation",
            r"implicit instantiation",
            r"template.*not found",
            r"template argument deduction",
        ],
        "INTRINSICS": [
            r"__builtin_",
            r"_mm_",
            r"__popcnt",
            r"intrinsic.*not available",
            r"__sync_",
            r"__atomic_",
        ],
        "ABI": [
            r"calling convention",
            r"ABI.*mismatch",
            r"incompatible.*function.*type",
            r"mangled name",
            r"name mangling",
        ],
        "PLATFORM_SPECIFIC": [
            r"#ifdef.*WIN32",
            r"#ifdef.*LINUX",
            r"platform.*not.*supported",
            r"_WIN32.*undefined",
        ],
        "STANDARD_LIBRARY": [
            r"std::.*not found",
            r"no member named.*in.*std",
            r"libstdc\+\+",
            r"libc\+\+",
            r"MSVC.*STL",
        ],
        "WARNING": [
            r"warning:",
            r"note:",
            r"remark:",
        ]
    }
    
    @classmethod
    def categorize(cls, message: str) -> str:
        """Categorize an error message"""
        for category, patterns in cls.PATTERNS.items():
            for pattern in patterns:
                if re.search(pattern, message, re.IGNORECASE):
                    return category
        return "OTHER"


class CompilerLogParser:
    """Parse compiler logs from different compilers"""
    
    # MSVC error format: file(line,col): severity CODE: message
    MSVC_PATTERN = re.compile(
        r"(?P<file>[^(]+)\((?P<line>\d+),(?P<col>\d+)\):\s*"
        r"(?P<severity>error|warning|note)\s+[A-Z]+\d+:\s*(?P<message>.+)"
    )
    
    # GCC/Clang error format: file:line:col: severity: message
    GCC_PATTERN = re.compile(
        r"(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):\s*"
        r"(?P<severity>error|warning|note):\s*(?P<message>.+)"
    )
    
    # Linker error patterns
    LINKER_PATTERN = re.compile(
        r"(?:ld|link\.exe|lld).*:\s*(?P<severity>error|warning):\s*(?P<message>.+)"
    )
    
    def __init__(self, compiler: str = "auto", platform: str = "auto"):
        self.compiler = compiler
        self.platform = platform
        
    def detect_compiler(self, log_content: str) -> str:
        """Detect compiler from log content"""
        if "MSVC" in log_content or "link.exe" in log_content or "cl.exe" in log_content:
            return "msvc"
        elif "g++" in log_content or "gcc" in log_content:
            return "gcc"
        elif "clang" in log_content:
            return "clang"
        return "unknown"
    
    def detect_platform(self, log_content: str) -> str:
        """Detect platform from log content"""
        if "win32" in log_content.lower() or "windows" in log_content.lower():
            return "windows"
        elif "linux" in log_content.lower():
            return "linux"
        elif "darwin" in log_content.lower() or "macos" in log_content.lower():
            return "macos"
        elif "arm" in log_content.lower() or "aarch64" in log_content.lower():
            return "arm"
        return "unknown"
    
    def parse_log(self, log_content: str) -> List[CompilerError]:
        """Parse a compiler log and extract errors"""
        errors = []
        
        # Auto-detect compiler and platform if needed
        compiler = self.compiler if self.compiler != "auto" else self.detect_compiler(log_content)
        platform = self.platform if self.platform != "auto" else self.detect_platform(log_content)
        
        lines = log_content.split('\n')
        for i, line in enumerate(lines):
            # Try MSVC format
            match = self.MSVC_PATTERN.search(line)
            if match:
                error = self._create_error(match, line, compiler, platform, lines, i)
                errors.append(error)
                continue
            
            # Try GCC/Clang format
            match = self.GCC_PATTERN.search(line)
            if match:
                error = self._create_error(match, line, compiler, platform, lines, i)
                errors.append(error)
                continue
            
            # Try linker format
            match = self.LINKER_PATTERN.search(line)
            if match:
                error = self._create_linker_error(match, line, compiler, platform, lines, i)
                errors.append(error)
        
        return errors
    
    def _create_error(self, match, line: str, compiler: str, platform: str, 
                     all_lines: List[str], line_idx: int) -> CompilerError:
        """Create a CompilerError from a regex match"""
        file_path = match.group('file').strip()
        line_num = int(match.group('line'))
        col_num = int(match.group('col'))
        severity = match.group('severity')
        message = match.group('message').strip()
        
        # Get context (next 3 lines)
        context_lines = [line]
        for i in range(1, 4):
            if line_idx + i < len(all_lines):
                context_lines.append(all_lines[line_idx + i])
        full_context = '\n'.join(context_lines)
        
        category = ErrorPattern.categorize(message)
        
        return CompilerError(
            file_path=file_path,
            line_number=line_num,
            column_number=col_num,
            severity=severity,
            message=message,
            category=category,
            compiler=compiler,
            platform=platform,
            full_context=full_context
        )
    
    def _create_linker_error(self, match, line: str, compiler: str, platform: str,
                            all_lines: List[str], line_idx: int) -> CompilerError:
        """Create a CompilerError for a linker error"""
        severity = match.group('severity')
        message = match.group('message').strip()
        
        # Get context
        context_lines = [line]
        for i in range(1, 4):
            if line_idx + i < len(all_lines):
                context_lines.append(all_lines[line_idx + i])
        full_context = '\n'.join(context_lines)
        
        category = "LINKER"
        
        return CompilerError(
            file_path="<linker>",
            line_number=0,
            column_number=0,
            severity=severity,
            message=message,
            category=category,
            compiler=compiler,
            platform=platform,
            full_context=full_context
        )


class ErrorDatabase:
    """SQLite database for storing compiler errors"""
    
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.conn = None
        self._init_db()
    
    def _init_db(self):
        """Initialize the database schema"""
        self.conn = sqlite3.connect(self.db_path)
        cursor = self.conn.cursor()
        
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS errors (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path TEXT,
                line_number INTEGER,
                column_number INTEGER,
                severity TEXT,
                message TEXT,
                category TEXT,
                compiler TEXT,
                platform TEXT,
                full_context TEXT,
                timestamp TEXT,
                resolved BOOLEAN DEFAULT 0
            )
        """)
        
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_category ON errors(category)
        """)
        
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_file ON errors(file_path)
        """)
        
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_platform ON errors(platform)
        """)
        
        self.conn.commit()
    
    def insert_error(self, error: CompilerError):
        """Insert an error into the database"""
        cursor = self.conn.cursor()
        cursor.execute("""
            INSERT INTO errors 
            (file_path, line_number, column_number, severity, message, 
             category, compiler, platform, full_context, timestamp)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            error.file_path,
            error.line_number,
            error.column_number,
            error.severity,
            error.message,
            error.category,
            error.compiler,
            error.platform,
            error.full_context,
            datetime.now().isoformat()
        ))
        self.conn.commit()
    
    def get_stats(self) -> Dict:
        """Get error statistics"""
        cursor = self.conn.cursor()
        
        # Total errors by category
        cursor.execute("""
            SELECT category, COUNT(*) 
            FROM errors 
            GROUP BY category 
            ORDER BY COUNT(*) DESC
        """)
        by_category = dict(cursor.fetchall())
        
        # Errors by platform
        cursor.execute("""
            SELECT platform, COUNT(*) 
            FROM errors 
            GROUP BY platform 
            ORDER BY COUNT(*) DESC
        """)
        by_platform = dict(cursor.fetchall())
        
        # Errors by compiler
        cursor.execute("""
            SELECT compiler, COUNT(*) 
            FROM errors 
            GROUP BY compiler 
            ORDER BY COUNT(*) DESC
        """)
        by_compiler = dict(cursor.fetchall())
        
        # Top problematic files
        cursor.execute("""
            SELECT file_path, COUNT(*) as count
            FROM errors 
            GROUP BY file_path 
            ORDER BY count DESC
            LIMIT 20
        """)
        top_files = dict(cursor.fetchall())
        
        return {
            "by_category": by_category,
            "by_platform": by_platform,
            "by_compiler": by_compiler,
            "top_files": top_files
        }
    
    def close(self):
        """Close database connection"""
        if self.conn:
            self.conn.close()


def main():
    parser = argparse.ArgumentParser(
        description="Parse and categorize compiler error logs"
    )
    parser.add_argument(
        "log_file",
        type=Path,
        help="Path to compiler log file"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=THEMIS_ROOT / "tools" / "compiler_diagnostics" / "compiler_diagnostics.db",
        help="Output database file"
    )
    parser.add_argument(
        "--compiler",
        choices=["msvc", "gcc", "clang", "auto"],
        default="auto",
        help="Compiler type (auto-detect if not specified)"
    )
    parser.add_argument(
        "--platform",
        choices=["windows", "linux", "macos", "arm", "auto"],
        default="auto",
        help="Platform (auto-detect if not specified)"
    )
    parser.add_argument(
        "--json",
        type=Path,
        help="Also output results as JSON"
    )
    
    args = parser.parse_args()
    
    # Read log file
    if not args.log_file.exists():
        print(f"Error: Log file not found: {args.log_file}")
        return 1
    
    log_content = args.log_file.read_text(encoding='utf-8', errors='ignore')
    
    # Parse errors
    parser = CompilerLogParser(compiler=args.compiler, platform=args.platform)
    errors = parser.parse_log(log_content)
    
    print(f"Found {len(errors)} errors/warnings in {args.log_file}")
    
    # Store in database
    db = ErrorDatabase(args.output)
    for error in errors:
        db.insert_error(error)
    
    # Print statistics
    stats = db.get_stats()
    
    print("\n=== Error Statistics ===")
    print("\nBy Category:")
    for category, count in stats["by_category"].items():
        print(f"  {category}: {count}")
    
    print("\nBy Platform:")
    for platform, count in stats["by_platform"].items():
        print(f"  {platform}: {count}")
    
    print("\nBy Compiler:")
    for compiler, count in stats["by_compiler"].items():
        print(f"  {compiler}: {count}")
    
    print("\nTop Problematic Files:")
    for file_path, count in list(stats["top_files"].items())[:10]:
        print(f"  {file_path}: {count}")
    
    # Output JSON if requested
    if args.json:
        output_data = {
            "errors": [asdict(e) for e in errors],
            "stats": stats,
            "metadata": {
                "source_file": str(args.log_file),
                "timestamp": datetime.now().isoformat(),
                "total_errors": len(errors)
            }
        }
        args.json.write_text(json.dumps(output_data, indent=2))
        print(f"\nJSON output written to {args.json}")
    
    db.close()
    print(f"\nDatabase written to {args.output}")
    
    return 0


if __name__ == "__main__":
    exit(main())
