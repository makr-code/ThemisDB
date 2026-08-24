#!/usr/bin/env python3
"""
Sprint 8 Batch D - Move Semantics Gap Scanner
Identifies and categorizes use-after-move vulnerabilities (CWE-416)
"""

import json
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from collections import defaultdict
import ast

@dataclass
class MoveGap:
    file: str
    line_no: int
    category: str  # A: Transaction, B: Coordinator, C: Reference
    pattern: str
    code_snippet: str
    severity: str  # Critical, High, Medium
    affected_modules: List[str]
    user_controlled: bool
    state_access: bool
    description: str

class MoveGapScanner:
    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.src_dir = self.repo_root / "src"
        self.include_dir = self.repo_root / "include"
        self.gaps: List[MoveGap] = []
        
        # Pattern detection regex
        self.patterns = {
            'move_semantics': {
                'std_move': r'std::move\s*\(',
                'move_assign': r'=\s*std::move\s*\(',
                'move_param': r'\(std::move\s*\(',
            },
            'post_move_access': {
                'method_call': r'(\w+)\s*\.\s*\w+\s*\(',
                'property_access': r'(\w+)\s*\.\s*\w+(?!\()',
                'arrow_access': r'(\w+)\s*->\s*\w+',
                'subscript': r'(\w+)\s*\[',
            },
            'high_risk_modules': {
                'transaction': r'(transaction|executor|coordinator)',
                'distributed': r'(distributed|sharding|replication)',
                'llm': r'(llm|model|inference)',
            },
        }
        
        # High-risk patterns
        self.high_risk_patterns = [
            r'Transaction\s+\w+\s*=\s*std::move\s*\(',
            r'std::move\s*\(\s*transaction',
            r'std::move\s*\(\s*coord',
            r'std::move\s*\(\s*model',
            r'auto\s+\w+\s*=\s*std::move\s*\(\s*\w+\s*\);',
        ]

    def scan_files(self) -> List[MoveGap]:
        """Scan source files for move semantics vulnerabilities"""
        for cpp_file in self.src_dir.rglob("*.cpp"):
            self._scan_file(cpp_file)
        for header_file in self.include_dir.rglob("*.h"):
            self._scan_file(header_file)
        
        return self.gaps

    def _scan_file(self, file_path: Path):
        """Scan a single file for move semantics issues"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return

        # Extract module name from path
        try:
            if 'src' in file_path.parts:
                module = file_path.parts[file_path.parts.index('src')+1]
            else:
                module = 'unknown'
        except:
            module = 'unknown'

        # Look for move patterns
        for line_no, line in enumerate(lines, 1):
            if 'std::move' not in line:
                continue

            # Get context (prev + current + next lines)
            start = max(0, line_no - 3)
            end = min(len(lines), line_no + 2)
            code_context = '\n'.join(lines[start:end])

            # Check for suspicious patterns
            for pattern in self.high_risk_patterns:
                if re.search(pattern, line):
                    # Extract variable name
                    var_match = re.search(r'std::move\s*\(\s*(\w+)', line)
                    if var_match:
                        var_name = var_match.group(1)
                        
                        # Check if variable is accessed after move
                        if self._check_post_move_access(lines, line_no, var_name):
                            severity = self._assess_severity(line, var_name, module)
                            
                            gap = MoveGap(
                                file=str(file_path).replace('\\', '/'),
                                line_no=line_no,
                                category=self._categorize_gap(line, var_name),
                                pattern=line.strip(),
                                code_snippet=code_context,
                                severity=severity,
                                affected_modules=[module],
                                user_controlled=self._check_user_controlled(code_context),
                                state_access=self._check_state_access(code_context),
                                description=self._describe_gap(line, var_name),
                            )
                            self.gaps.append(gap)

    def _check_post_move_access(self, lines: List[str], move_line_no: int, var_name: str) -> bool:
        """Check if variable is accessed after move"""
        # Look ahead up to 10 lines for post-move access
        for i in range(move_line_no, min(move_line_no + 10, len(lines))):
            line = lines[i]
            # Skip comments and move statement itself
            if i == move_line_no - 1 or line.strip().startswith('//'):
                continue
                
            # Check for access patterns
            if re.search(rf'{var_name}\s*\.', line) or re.search(rf'{var_name}\s*->', line):
                # Verify it's not in a new declaration
                if not re.search(rf'auto\s+\w+\s*=\s*std::move\s*\(\s*{var_name}', line):
                    return True
        
        return False

    def _categorize_gap(self, line: str, var_name: str) -> str:
        """Categorize gap by type"""
        if 'transaction' in line.lower() or 'transaction' in var_name.lower():
            return 'A_Transaction'
        elif 'coord' in line.lower() or 'coord' in var_name.lower():
            return 'B_Coordinator'
        elif 'model' in line.lower() or 'model' in var_name.lower():
            return 'C_Model'
        else:
            return 'D_Other'

    def _assess_severity(self, line: str, var_name: str, module: str) -> str:
        """Assess gap severity"""
        if 'transaction' in line.lower() or module in ['transaction', 'sharding', 'distributed']:
            return 'CRITICAL'
        elif 'model' in line.lower():
            return 'HIGH'
        else:
            return 'MEDIUM'

    def _check_user_controlled(self, code_context: str) -> bool:
        """Check if gap involves user-controlled data"""
        user_patterns = [
            r'parseFrom\s*\(',
            r'fromProto\s*\(',
            r'fromJson\s*\(',
            r'request\.',
            r'param\.',
            r'input\.',
        ]
        return any(re.search(p, code_context) for p in user_patterns)

    def _check_state_access(self, code_context: str) -> bool:
        """Check if gap involves state access after move"""
        state_patterns = [
            r'\.status\s*\(',
            r'\.state\s*\(',
            r'\.get\s*\(',
            r'\.id\s*\(',
        ]
        return any(re.search(p, code_context) for p in state_patterns)

    def _describe_gap(self, line: str, var_name: str) -> str:
        """Generate gap description"""
        if 'transaction' in line.lower():
            return f"Use-after-move: transaction '{var_name}' accessed after std::move()"
        elif 'coord' in line.lower():
            return f"Use-after-move: coordinator '{var_name}' accessed after std::move()"
        else:
            return f"Use-after-move: variable '{var_name}' accessed after std::move()"

def main():
    scanner = MoveGapScanner(".")
    gaps = scanner.scan_files()
    
    # Sort by severity and line number
    gaps.sort(key=lambda g: (
        {'CRITICAL': 0, 'HIGH': 1, 'MEDIUM': 2}.get(g.severity, 3),
        g.file,
        g.line_no
    ))
    
    # Generate reports
    print(f"Total move-after-use gaps found: {len(gaps)}")
    
    # Categorize gaps
    by_category = defaultdict(list)
    for gap in gaps:
        by_category[gap.category].append(gap)
    
    print("\nGaps by category:")
    for cat, cat_gaps in sorted(by_category.items()):
        print(f"  {cat}: {len(cat_gaps)} gaps")
    
    # Output JSON
    with open('move_gaps_analysis.json', 'w') as f:
        json.dump([asdict(g) for g in gaps], f, indent=2)
    
    # Output top 30 as separate file
    top_30 = gaps[:30]
    with open('top_move_gaps.json', 'w') as f:
        json.dump([asdict(g) for g in top_30], f, indent=2)
    
    print(f"\nOutputs written:")
    print(f"  - move_gaps_analysis.json ({len(gaps)} gaps)")
    print(f"  - top_move_gaps.json ({len(top_30)} top gaps)")

if __name__ == "__main__":
    main()
