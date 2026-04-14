"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            parse_build_errors.py                              ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:10:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
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
Build Error Parser for CI/CD
Parst Compiler-Ausgaben und erstellt strukturierte Fehlerberichte
"""

import re
import json
import sys
import argparse
from pathlib import Path
from collections import defaultdict
from typing import List, Dict, Tuple

def parse_gcc_clang_errors(log_file: str) -> Tuple[List[Dict], List[Dict]]:
    """Parst GCC/Clang Fehler und Warnungen"""
    errors = []
    warnings = []
    
    error_pattern = re.compile(
        r'(?P<file>[\w./\-]+):(?P<line>\d+):(?P<col>\d+): '
        r'(?P<severity>error|warning): (?P<message>.*?)(?:\n|$)'
    )
    
    try:
        with open(log_file, 'r', errors='ignore') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: Log file {log_file} not found", file=sys.stderr)
        return errors, warnings
        
    for match in error_pattern.finditer(content):
        entry = {
            'file': match.group('file'),
            'line': int(match.group('line')),
            'column': int(match.group('col')),
            'severity': match.group('severity'),
            'message': match.group('message').strip()
        }
        
        if entry['severity'] == 'error':
            errors.append(entry)
        else:
            warnings.append(entry)
    
    return errors, warnings

def parse_msvc_errors(log_file: str) -> Tuple[List[Dict], List[Dict]]:
    """Parst MSVC Fehler und Warnungen"""
    errors = []
    warnings = []
    
    # MSVC Format: file(line): error C1234: message
    error_pattern = re.compile(
        r'(?P<file>[\w:/\\.\-]+)\((?P<line>\d+)\): '
        r'(?P<severity>error|warning) (?P<code>[A-Z]\d+): (?P<message>.*?)(?:\n|$)'
    )
    
    try:
        with open(log_file, 'r', errors='ignore') as f:
            content = f.read()
    except FileNotFoundError:
        return errors, warnings
        
    for match in error_pattern.finditer(content):
        entry = {
            'file': match.group('file'),
            'line': int(match.group('line')),
            'column': 0,  # MSVC doesn't always provide column
            'severity': match.group('severity'),
            'code': match.group('code'),
            'message': match.group('message').strip()
        }
        
        if entry['severity'] == 'error':
            errors.append(entry)
        else:
            warnings.append(entry)
    
    return errors, warnings

def categorize_errors(errors: List[Dict]) -> Dict[str, List[Dict]]:
    """Kategorisiert Fehler nach Typ"""
    categories = defaultdict(list)
    
    for error in errors:
        msg = error['message'].lower()
        
        if 'undefined reference' in msg or 'unresolved external' in msg:
            categories['linking'].append(error)
        elif 'syntax error' in msg or 'expected' in msg:
            categories['syntax'].append(error)
        elif 'no such file' in msg or 'cannot find' in msg:
            categories['missing_file'].append(error)
        elif 'ambiguous' in msg or 'overload' in msg:
            categories['ambiguity'].append(error)
        elif 'deprecated' in msg:
            categories['deprecated'].append(error)
        else:
            categories['other'].append(error)
    
    return dict(categories)

def generate_summary(errors: List[Dict], warnings: List[Dict], 
                    categories: Dict[str, List[Dict]]) -> Dict:
    """Generiert Zusammenfassung"""
    summary = {
        'total_errors': len(errors),
        'total_warnings': len(warnings),
        'categories': {k: len(v) for k, v in categories.items()},
        'top_errors': errors[:10],  # Top 10
        'affected_files': list(set(e['file'] for e in errors))
    }
    return summary

def generate_markdown_report(summary: Dict) -> str:
    """Generiert Markdown-Report"""
    md = "## Build Fehler Zusammenfassung\n\n"
    md += f"- **Fehler:** {summary['total_errors']}\n"
    md += f"- **Warnungen:** {summary['total_warnings']}\n"
    md += f"- **Betroffene Dateien:** {len(summary['affected_files'])}\n\n"
    
    if summary['categories']:
        md += "### Fehler nach Kategorie\n\n"
        for cat, count in summary['categories'].items():
            md += f"- **{cat}:** {count}\n"
        md += "\n"
    
    if summary['top_errors']:
        md += "### Top Fehler\n\n"
        for idx, error in enumerate(summary['top_errors'], 1):
            md += f"{idx}. `{error['file']}:{error['line']}` - {error['message'][:100]}\n"
    
    return md

def main():
    parser = argparse.ArgumentParser(
        description='Parse build errors from compiler output'
    )
    parser.add_argument('log_file', help='Build log file to parse')
    parser.add_argument('--compiler', choices=['gcc', 'clang', 'msvc'], 
                       default='gcc', help='Compiler type')
    parser.add_argument('--json', help='Output JSON file')
    parser.add_argument('--markdown', help='Output Markdown file')
    
    args = parser.parse_args()
    
    # Parse errors based on compiler
    if args.compiler in ['gcc', 'clang']:
        errors, warnings = parse_gcc_clang_errors(args.log_file)
    elif args.compiler == 'msvc':
        errors, warnings = parse_msvc_errors(args.log_file)
    
    # Categorize and summarize
    categories = categorize_errors(errors)
    summary = generate_summary(errors, warnings, categories)
    
    # Output JSON
    if args.json:
        with open(args.json, 'w') as f:
            json.dump(summary, f, indent=2)
        print(f"JSON report written to {args.json}")
    
    # Output Markdown
    if args.markdown:
        md = generate_markdown_report(summary)
        with open(args.markdown, 'w') as f:
            f.write(md)
        print(f"Markdown report written to {args.markdown}")
    
    # Console output
    print(f"\nBuild Error Summary:")
    print(f"  Errors: {summary['total_errors']}")
    print(f"  Warnings: {summary['total_warnings']}")
    print(f"  Affected files: {len(summary['affected_files'])}")
    
    # Exit code based on errors
    sys.exit(1 if summary['total_errors'] > 0 else 0)

if __name__ == '__main__':
    main()
