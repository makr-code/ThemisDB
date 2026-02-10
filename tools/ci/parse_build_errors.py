#!/usr/bin/env python3
"""
Build Error Parser für ThemisDB CI/CD Pipeline
Analysiert Build-Logs und extrahiert strukturierte Fehlerinformationen
"""

import re
import json
import sys
import argparse
from pathlib import Path
from collections import defaultdict
from typing import List, Dict, Tuple
from dataclasses import dataclass, asdict


@dataclass
class BuildError:
    """Repräsentiert einen einzelnen Build-Fehler"""
    file: str
    line: int
    column: int
    severity: str
    message: str
    category: str = "other"
    
    def to_dict(self):
        return asdict(self)


class BuildErrorParser:
    """Parser für verschiedene Compiler-Ausgaben"""  
    
    # Regex-Patterns für verschiedene Compiler
    GCC_CLANG_PATTERN = re.compile(
        r'(?P<file>[\\w./-]+):(?P<line>\d+):(?P<col>\d+):\s+'
        r'(?P<severity>error|warning|note):\s+(?P<message>.*?)(?:\n|$)'
    )
    
    MSVC_PATTERN = re.compile(
        r'(?P<file>[\\w:./-]+)\((?P<line>\d+)\):\s+'
        r'(?P<severity>error|warning)\s+\w+:\s+(?P<message>.*?)(?:\n|$)'
    )
    
    # Kategorisierungs-Keywords
    CATEGORIES = {
        'linking': ['undefined reference', 'unresolved external', 'cannot find -l', 'ld returned'],
        'syntax': ['syntax error', 'expected', 'unexpected token', 'invalid syntax'],
        'missing_file': ['no such file', 'cannot find', 'file not found', 'cannot open'],
        'type': ['cannot convert', 'type mismatch', 'incompatible types', 'invalid conversion'],
        'ambiguity': ['ambiguous', 'overload', 'multiple definitions'],
        'deprecated': ['deprecated', 'is deprecated'],
        'template': ['template', 'instantiation', 'substitution failure'],
        'nullptr': ['nullptr', 'null pointer', 'segmentation fault'],
        'memory': ['memory', 'allocation failed', 'out of memory'],
    }
    
    def __init__(self, log_file: Path, compiler: str = 'gcc'):
        self.log_file = log_file
        self.compiler = compiler.lower()
        self.errors: List[BuildError] = []
        self.warnings: List[BuildError] = []
        
    def parse(self) -> Tuple[List[BuildError], List[BuildError]]:
        """Hauptmethode zum Parsen der Build-Logs"""
        if not self.log_file.exists():
            print(f"⚠️  Log-Datei nicht gefunden: {self.log_file}", file=sys.stderr)
            return [], []
            
        content = self.log_file.read_text(errors='ignore')
        
        if self.compiler in ['gcc', 'clang', 'g++', 'clang++']:
            self._parse_gcc_clang(content)
        elif self.compiler == 'msvc':
            self._parse_msvc(content)
        else:
            # Versuche beide Patterns
            self._parse_gcc_clang(content)
            if not self.errors and not self.warnings:
                self._parse_msvc(content)
        
        return self.errors, self.warnings
    
    def _parse_gcc_clang(self, content: str):
        """Parse GCC/Clang Fehler"""
        for match in self.GCC_CLANG_PATTERN.finditer(content):
            error = BuildError(
                file=match.group('file'),
                line=int(match.group('line')), 
                column=int(match.group('col')),
                severity=match.group('severity'),
                message=match.group('message').strip()
            )
            
            error.category = self._categorize_error(error.message)
            
            if error.severity == 'error':
                self.errors.append(error)
            elif error.severity == 'warning':
                self.warnings.append(error)
    
    def _parse_msvc(self, content: str):
        """Parse MSVC Fehler"""
        for match in self.MSVC_PATTERN.finditer(content):
            error = BuildError(
                file=match.group('file'),
                line=int(match.group('line')),
                column=0,  # MSVC gibt keine Spalte an
                severity=match.group('severity'),
                message=match.group('message').strip()
            )
            
            error.category = self._categorize_error(error.message)
            
            if error.severity == 'error':
                self.errors.append(error)
            elif error.severity == 'warning':
                self.warnings.append(error)
    
    def _categorize_error(self, message: str) -> str:
        """Kategorisiert Fehler basierend auf der Fehlermeldung"""
        message_lower = message.lower()
        
        for category, keywords in self.CATEGORIES.items():
            if any(keyword in message_lower for keyword in keywords):
                return category
        
        return 'other'
    
    def get_statistics(self) -> Dict:
        """Erstellt Statistiken über die gefundenen Fehler"""
        error_by_category = defaultdict(int)
        error_by_file = defaultdict(int)
        
        for error in self.errors:
            error_by_category[error.category] += 1
            error_by_file[error.file] += 1
        
        return {
            'total_errors': len(self.errors),
            'total_warnings': len(self.warnings),
            'errors_by_category': dict(error_by_category),
            'errors_by_file': dict(error_by_file),
            'most_affected_files': sorted(
                error_by_file.items(), 
                key=lambda x: x[1], 
                reverse=True
            )[:10]
        }
    
    def generate_summary(self, output_file: Path = None) -> Dict:
        """Generiert eine Zusammenfassung für GitHub Actions"""
        stats = self.get_statistics()
        
        summary = {
            'total_errors': stats['total_errors'],
            'total_warnings': stats['total_warnings'],
            'categories': stats['errors_by_category'],
            'top_errors': [e.to_dict() for e in self.errors[:10]],
            'affected_files': list(stats['errors_by_file'].keys())
        }
        
        if output_file:
            output_file.write_text(json.dumps(summary, indent=2))
        
        return summary
    
    def generate_markdown_report(self, output_file: Path = None) -> str:
        """Erstellt einen Markdown-Report"""
        stats = self.get_statistics()
        
        md = "# 🔨 Build Error Report\n\n"
        md += f"## Zusammenfassung\n\n"
        md += f"- **Fehler:** {stats['total_errors']}\n"
        md += f"- **Warnungen:** {stats['total_warnings']}\n\n"
        
        if stats['errors_by_category']:
            md += "## Fehler nach Kategorie\n\n"
            for category, count in sorted(
                stats['errors_by_category'].items(), 
                key=lambda x: x[1], 
                reverse=True
            ):
                md += f"- **{category}:** {count}\n"
            md += "\n"
        
        if stats['most_affected_files']:
            md += "## Am meisten betroffene Dateien\n\n"
            for file, count in stats['most_affected_files']:
                md += f"- `{file}` ({count} Fehler)\n"
            md += "\n"
        
        if self.errors:
            md += "## Top 10 Fehler\n\n"
            for i, error in enumerate(self.errors[:10], 1):
                md += f"### {i}. {error.file}:{error.line}\n\n"
                md += f"**Kategorie:** {error.category}\n\n"
                md += f"```
{error.message}
```
\n"
        
        if output_file:
            output_file.write_text(md)
        
        return md


def main():
    parser = argparse.ArgumentParser(
        description='Parse build logs and extract error information'
    )
    parser.add_argument(
        'log_file',
        type=Path,
        help='Path to the build log file'
    )
    parser.add_argument(
        '--compiler',
        choices=['gcc', 'clang', 'msvc', 'auto'],
        default='auto',
        help='Compiler type (default: auto-detect)'
    )
    parser.add_argument(
        '--json',
        type=Path,
        help='Output JSON summary to file'
    )
    parser.add_argument(
        '--markdown',
        type=Path,
        help='Output Markdown report to file'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Print detailed information'
    )
    
    args = parser.parse_args()
    
    # Parse Fehler
    parser_instance = BuildErrorParser(args.log_file, args.compiler)
    errors, warnings = parser_instance.parse()
    
    if args.verbose:
        print(f"✅ Parsed {len(errors)} errors and {len(warnings)} warnings")
    
    # Generiere Outputs
    if args.json:
        summary = parser_instance.generate_summary(args.json)
        if args.verbose:
            print(f"📄 JSON summary written to {args.json}")
    
    if args.markdown:
        report = parser_instance.generate_markdown_report(args.markdown)
        if args.verbose:
            print(f"📝 Markdown report written to {args.markdown}")
    
    # Für GitHub Actions Output
    if not args.json and not args.markdown:
        summary = parser_instance.generate_summary()
        print(json.dumps(summary, indent=2))
    
    # Exit code basierend auf Fehleranzahl
    return min(len(errors), 1)


if __name__ == '__main__':
    sys.exit(main())
