"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            verify_documentation_todos.py                      ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:59:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   70.0/100                                       ║
    • Total Lines:     416                                            ║
    • Open Issues:     TODOs: 21, Stubs: 0                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Documentation TODO Verification Script

This script analyzes documentation files to extract and categorize TODO markers,
cross-referencing them with the codebase to determine if features are:
1. Already implemented (code exists)
2. Not implemented (actual gap)
3. Partially implemented (some code exists)
4. Outdated/Invalid (no longer relevant)

Usage:
    python3 verify_documentation_todos.py --doc=<path> [--all] [--output=<file>]
"""

import os
import re
import json
import argparse
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime


@dataclass
class TodoItem:
    """Represents a single TODO/TBD/checkbox item from documentation"""
    file_path: str
    line_number: int
    content: str
    marker_type: str  # 'checkbox', 'TODO', 'TBD', 'FIXME'
    status: str = 'unknown'  # 'implemented', 'gap', 'partial', 'outdated', 'doc-only'
    evidence: List[str] = None
    confidence: str = 'low'  # 'low', 'medium', 'high'
    category: str = ''
    notes: str = ''
    
    def __post_init__(self):
        if self.evidence is None:
            self.evidence = []


class TodoVerifier:
    """Verifies TODO items against codebase implementation"""
    
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.src_dirs = ['src', 'include', 'tests', 'plugins']
        
    def extract_todos_from_file(self, file_path: str) -> List[TodoItem]:
        """Extract all TODO markers from a single markdown file"""
        todos = []
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
                
            for line_num, line in enumerate(lines, 1):
                # Pattern 1: Unchecked checkboxes
                if re.search(r'-\s*\[\s*\]', line):
                    todos.append(TodoItem(
                        file_path=file_path,
                        line_number=line_num,
                        content=line.strip(),
                        marker_type='checkbox'
                    ))
                
                # Pattern 2: TODO markers
                todo_match = re.search(r'TODO[:\s]+(.*)', line, re.IGNORECASE)
                if todo_match:
                    todos.append(TodoItem(
                        file_path=file_path,
                        line_number=line_num,
                        content=line.strip(),
                        marker_type='TODO'
                    ))
                
                # Pattern 3: TBD markers
                tbd_match = re.search(r'TBD[:\s]+(.*)', line, re.IGNORECASE)
                if tbd_match:
                    todos.append(TodoItem(
                        file_path=file_path,
                        line_number=line_num,
                        content=line.strip(),
                        marker_type='TBD'
                    ))
                
                # Pattern 4: FIXME markers
                fixme_match = re.search(r'FIXME[:\s]+(.*)', line, re.IGNORECASE)
                if fixme_match:
                    todos.append(TodoItem(
                        file_path=file_path,
                        line_number=line_num,
                        content=line.strip(),
                        marker_type='FIXME'
                    ))
        
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
        
        return todos
    
    def extract_keywords_from_todo(self, todo: TodoItem) -> List[str]:
        """Extract meaningful keywords from TODO content for searching"""
        # Remove common markdown and task list syntax
        content = todo.content
        content = re.sub(r'-\s*\[\s*\]', '', content)
        content = re.sub(r'TODO[:\s]+', '', content, flags=re.IGNORECASE)
        content = re.sub(r'TBD[:\s]+', '', content, flags=re.IGNORECASE)
        content = re.sub(r'FIXME[:\s]+', '', content, flags=re.IGNORECASE)
        
        # Extract potential function/class/file names
        keywords = []
        
        # Look for CamelCase or snake_case identifiers
        identifiers = re.findall(r'\b[A-Z][a-zA-Z0-9]*\b|\b[a-z_][a-z0-9_]+\b', content)
        keywords.extend([k for k in identifiers if len(k) > 3])
        
        # Look for file extensions/patterns
        file_patterns = re.findall(r'\b\w+\.(cpp|h|hpp|py|md)\b', content)
        keywords.extend(file_patterns)
        
        # Look for quoted terms
        quoted = re.findall(r'["`]([^"`]+)["`]', content)
        keywords.extend(quoted)
        
        return list(set(keywords))[:10]  # Limit to 10 most relevant
    
    def search_codebase(self, keywords: List[str]) -> List[str]:
        """Search codebase for implementation evidence"""
        evidence = []
        
        for keyword in keywords:
            if len(keyword) < 4:  # Skip very short keywords
                continue
            
            try:
                # Search in source directories
                for src_dir in self.src_dirs:
                    search_path = self.repo_root / src_dir
                    if not search_path.exists():
                        continue
                    
                    # Use grep for fast searching
                    cmd = [
                        'grep', '-r', '-l', '-i',
                        '--include=*.cpp', '--include=*.h', '--include=*.hpp',
                        keyword, str(search_path)
                    ]
                    
                    result = subprocess.run(
                        cmd,
                        capture_output=True,
                        text=True,
                        timeout=10
                    )
                    
                    if result.returncode == 0 and result.stdout.strip():
                        files = result.stdout.strip().split('\n')
                        evidence.extend([f"{keyword} found in {f}" for f in files[:3]])
            
            except Exception as e:
                print(f"Warning: Error searching for '{keyword}': {e}")
        
        return list(set(evidence))[:10]  # Limit evidence to 10 items
    
    def categorize_todo(self, todo: TodoItem) -> TodoItem:
        """Categorize TODO based on content analysis"""
        content_lower = todo.content.lower()
        
        # Categorize by keywords
        if any(word in content_lower for word in ['document', 'doc', 'write', 'update doc', 'add doc']):
            todo.category = 'documentation'
        elif any(word in content_lower for word in ['security', 'encrypt', 'audit', 'pki', 'hsm']):
            todo.category = 'security'
        elif any(word in content_lower for word in ['enterprise', 'license', 'commercial']):
            todo.category = 'enterprise'
        elif any(word in content_lower for word in ['test', 'unittest', 'integration test']):
            todo.category = 'testing'
        elif any(word in content_lower for word in ['performance', 'optimize', 'cache', 'benchmark']):
            todo.category = 'performance'
        elif any(word in content_lower for word in ['llm', 'ai', 'model', 'inference']):
            todo.category = 'llm-ai'
        elif any(word in content_lower for word in ['analytics', 'process mining', 'graph']):
            todo.category = 'analytics'
        else:
            todo.category = 'general'
        
        return todo
    
    def verify_todo(self, todo: TodoItem) -> TodoItem:
        """Verify a single TODO item against the codebase"""
        # Extract keywords and search for implementation
        keywords = self.extract_keywords_from_todo(todo)
        evidence = self.search_codebase(keywords)
        
        # Categorize the TODO
        todo = self.categorize_todo(todo)
        
        # Determine status based on evidence
        if len(evidence) >= 3:
            todo.status = 'likely_implemented'
            todo.confidence = 'medium'
            todo.notes = f"Found {len(evidence)} code references"
        elif len(evidence) >= 1:
            todo.status = 'partial'
            todo.confidence = 'low'
            todo.notes = f"Found {len(evidence)} code references, needs manual review"
        else:
            # Check if it's documentation-only
            content_lower = todo.content.lower()
            if any(word in content_lower for word in ['document', 'doc', 'write doc', 'update doc']):
                todo.status = 'doc-only'
                todo.confidence = 'medium'
                todo.notes = "Appears to be documentation task"
            else:
                todo.status = 'possible_gap'
                todo.confidence = 'low'
                todo.notes = "No code references found, needs manual verification"
        
        todo.evidence = evidence
        return todo


class VerificationReport:
    """Generates verification reports"""
    
    def __init__(self):
        self.stats = {
            'total_files': 0,
            'total_todos': 0,
            'by_status': {},
            'by_category': {},
            'by_confidence': {}
        }
    
    def generate_markdown_report(self, todos: List[TodoItem], output_file: str):
        """Generate a markdown verification report"""
        # Calculate statistics
        self.stats['total_todos'] = len(todos)
        
        for todo in todos:
            self.stats['by_status'][todo.status] = self.stats['by_status'].get(todo.status, 0) + 1
            self.stats['by_category'][todo.category] = self.stats['by_category'].get(todo.category, 0) + 1
            self.stats['by_confidence'][todo.confidence] = self.stats['by_confidence'].get(todo.confidence, 0) + 1
        
        # Generate report
        report = []
        report.append("# Documentation TODO Verification Report\n")
        report.append(f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        report.append(f"**Total TODOs Analyzed:** {self.stats['total_todos']}\n")
        report.append("\n---\n")
        
        # Statistics
        report.append("## 📊 Summary Statistics\n")
        report.append(f"### Status Breakdown\n")
        for status, count in sorted(self.stats['by_status'].items(), key=lambda x: x[1], reverse=True):
            percentage = (count / self.stats['total_todos'] * 100) if self.stats['total_todos'] > 0 else 0
            report.append(f"- **{status}**: {count} ({percentage:.1f}%)\n")
        
        report.append(f"\n### Category Breakdown\n")
        for category, count in sorted(self.stats['by_category'].items(), key=lambda x: x[1], reverse=True):
            percentage = (count / self.stats['total_todos'] * 100) if self.stats['total_todos'] > 0 else 0
            report.append(f"- **{category}**: {count} ({percentage:.1f}%)\n")
        
        report.append("\n---\n")
        
        # Detailed findings
        report.append("## 📋 Detailed Findings\n")
        
        # Group by file
        todos_by_file = {}
        for todo in todos:
            if todo.file_path not in todos_by_file:
                todos_by_file[todo.file_path] = []
            todos_by_file[todo.file_path].append(todo)
        
        for file_path, file_todos in sorted(todos_by_file.items()):
            report.append(f"\n### File: `{file_path}`\n")
            report.append(f"**Total Items:** {len(file_todos)}\n\n")
            
            # Group by status
            status_groups = {}
            for todo in file_todos:
                if todo.status not in status_groups:
                    status_groups[todo.status] = []
                status_groups[todo.status].append(todo)
            
            for status, status_todos in sorted(status_groups.items()):
                report.append(f"#### {status.upper()} ({len(status_todos)} items)\n")
                for todo in status_todos[:10]:  # Limit to 10 per status
                    report.append(f"- Line {todo.line_number}: `{todo.content[:100]}...`\n")
                    if todo.evidence:
                        report.append(f"  - Evidence: {', '.join(todo.evidence[:3])}\n")
                    report.append(f"  - Category: {todo.category}, Confidence: {todo.confidence}\n")
                if len(status_todos) > 10:
                    report.append(f"  - ... and {len(status_todos) - 10} more\n")
                report.append("\n")
        
        # Write report
        with open(output_file, 'w', encoding='utf-8') as f:
            f.writelines(report)
        
        print(f"✅ Report generated: {output_file}")
    
    def generate_json_report(self, todos: List[TodoItem], output_file: str):
        """Generate a JSON verification report"""
        data = {
            'generated': datetime.now().isoformat(),
            'total_todos': len(todos),
            'statistics': self.stats,
            'todos': [asdict(todo) for todo in todos]
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        
        print(f"✅ JSON report generated: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Verify documentation TODOs against codebase implementation'
    )
    parser.add_argument(
        '--doc',
        help='Path to specific documentation file to verify'
    )
    parser.add_argument(
        '--all',
        action='store_true',
        help='Verify all documentation files in docs/ directory'
    )
    parser.add_argument(
        '--output',
        default='verification_report',
        help='Output file basename (without extension)'
    )
    parser.add_argument(
        '--repo',
        default='/home/runner/work/ThemisDB/ThemisDB',
        help='Repository root path'
    )
    
    args = parser.parse_args()
    
    # Initialize verifier
    verifier = TodoVerifier(args.repo)
    
    # Collect files to verify
    files_to_verify = []
    if args.doc:
        files_to_verify.append(args.doc)
    elif args.all:
        docs_dir = Path(args.repo) / 'docs'
        files_to_verify = list(docs_dir.rglob('*.md'))
    else:
        print("Error: Please specify --doc or --all")
        return 1
    
    print(f"🔍 Verifying {len(files_to_verify)} documentation file(s)...")
    
    # Extract and verify all TODOs
    all_todos = []
    for file_path in files_to_verify:
        print(f"  Processing {file_path}...")
        todos = verifier.extract_todos_from_file(str(file_path))
        
        # Verify each TODO
        for todo in todos:
            verified_todo = verifier.verify_todo(todo)
            all_todos.append(verified_todo)
    
    print(f"✅ Found {len(all_todos)} TODO items")
    
    # Generate reports
    report = VerificationReport()
    report.generate_markdown_report(all_todos, f"{args.output}.md")
    report.generate_json_report(all_todos, f"{args.output}.json")
    
    print(f"\n📊 Verification Summary:")
    print(f"  Total TODOs: {len(all_todos)}")
    print(f"  Likely Implemented: {sum(1 for t in all_todos if t.status == 'likely_implemented')}")
    print(f"  Possible Gaps: {sum(1 for t in all_todos if t.status == 'possible_gap')}")
    print(f"  Partial: {sum(1 for t in all_todos if t.status == 'partial')}")
    print(f"  Doc-only: {sum(1 for t in all_todos if t.status == 'doc-only')}")
    
    return 0


if __name__ == '__main__':
    exit(main())
