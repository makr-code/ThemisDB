#!/usr/bin/env python3
"""
Sprint 8: Move Semantics Gap Remediation Script
Identifies and helps fix moved-from usage patterns
"""

import re
import os
from pathlib import Path
from typing import List, Dict, Tuple
from collections import defaultdict


class MovedFromGapFinder:
    """Find and categorize moved-from usage patterns"""
    
    def __init__(self, src_root: str = "src"):
        self.src_root = src_root
        self.gaps = []
        self.patterns = {
            'clear_after_push': re.compile(
                r'(\w+)\.push_back\s*\(\s*std::move\s*\(\s*(\w+)\s*\)\s*\)\s*;\s*\n\s*\2\.clear\s*\(\)',
                re.MULTILINE
            ),
            'clear_after_emplace': re.compile(
                r'(\w+)\.emplace_back\s*\(\s*std::move\s*\(\s*(\w+)\s*\)\s*\)\s*;\s*\n\s*\2\.clear\s*\(\)',
                re.MULTILINE
            ),
            'access_after_move': re.compile(
                r'(\w+)\s*=\s*std::move\s*\(\s*(\w+)\s*\)\s*;\s*\n\s*(?:.*\n)*?\s*\2\s*[\.\[]',
                re.MULTILINE
            ),
        }
    
    def scan_file(self, filepath: str) -> List[Dict]:
        """Scan a single file for moved-from gaps"""
        gaps = []
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
                
                # Pattern 1: .clear() after move
                for i, line in enumerate(lines):
                    if 'std::move' in line and i + 1 < len(lines):
                        moves = re.findall(r'std::move\s*\(\s*(\w+)\s*\)', line)
                        for var in moves:
                            next_line = lines[i + 1] if i + 1 < len(lines) else ""
                            if var + '.clear()' in next_line:
                                gaps.append({
                                    'file': filepath,
                                    'line': i + 1,
                                    'type': 'clear_after_move',
                                    'var': var,
                                    'code': line.strip()[:100],
                                    'next_code': next_line.strip()[:100],
                                    'severity': 'LOW'  # Safe in practice for strings
                                })
                            elif var + '.' in next_line or var + '[' in next_line:
                                if f'std::move({var})' not in next_line:
                                    gaps.append({
                                        'file': filepath,
                                        'line': i + 1,
                                        'type': 'member_access_after_move',
                                        'var': var,
                                        'code': line.strip()[:100],
                                        'next_code': next_line.strip()[:100],
                                        'severity': 'MEDIUM'
                                    })
        except Exception as e:
            pass
        
        return gaps
    
    def scan_directory(self) -> List[Dict]:
        """Scan all source files"""
        all_gaps = []
        for root, dirs, files in os.walk(self.src_root):
            dirs[:] = [d for d in dirs if d not in ['vcpkg', '.git', '.github']]
            for file in files:
                if file.endswith(('.cpp', '.hpp', '.h')):
                    filepath = os.path.join(root, file)
                    gaps = self.scan_file(filepath)
                    all_gaps.extend(gaps)
        
        self.gaps = all_gaps
        return all_gaps
    
    def generate_fix_report(self, output_file: str = None):
        """Generate remediation report"""
        if not self.gaps:
            self.scan_directory()
        
        # Group by file
        by_file = defaultdict(list)
        for gap in self.gaps:
            by_file[gap['file']].append(gap)
        
        report = []
        report.append("# Sprint 8: Move Semantics Remediation Report\n")
        report.append(f"**Total Gaps:** {len(self.gaps)}\n")
        report.append(f"**Files Affected:** {len(by_file)}\n\n")
        
        # By type
        by_type = defaultdict(list)
        for gap in self.gaps:
            by_type[gap['type']].append(gap)
        
        report.append("## By Type\n")
        for gap_type, gaps in sorted(by_type.items()):
            report.append(f"- {gap_type}: {len(gaps)} gaps\n")
        
        report.append("\n## Top Files\n")
        sorted_files = sorted(by_file.items(), key=lambda x: len(x[1]), reverse=True)
        for filepath, gaps in sorted_files[:10]:
            report.append(f"\n### {filepath} ({len(gaps)} gaps)\n")
            for gap in gaps[:3]:
                report.append(f"- Line {gap['line']}: {gap['type']}\n")
                report.append(f"  Var: {gap['var']}, Severity: {gap['severity']}\n")
        
        report_text = "\n".join(report)
        
        if output_file:
            with open(output_file, 'w') as f:
                f.write(report_text)
        
        return report_text


if __name__ == '__main__':
    finder = MovedFromGapFinder()
    gaps = finder.scan_directory()
    print(f"Found {len(gaps)} moved-from gaps")
    
    # Group by severity
    by_severity = defaultdict(int)
    for gap in gaps:
        by_severity[gap['severity']] += 1
    
    print("\nBy Severity:")
    for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        if severity in by_severity:
            print(f"  {severity}: {by_severity[severity]}")
    
    # Group by type
    by_type = defaultdict(int)
    for gap in gaps:
        by_type[gap['type']] += 1
    
    print("\nBy Type:")
    for gap_type, count in sorted(by_type.items()):
        print(f"  {gap_type}: {count}")
    
    # Generate report
    report = finder.generate_fix_report('ai_working/SPRINT_8_REMEDIATION_REPORT.md')
    print(f"\nReport written to: ai_working/SPRINT_8_REMEDIATION_REPORT.md")
