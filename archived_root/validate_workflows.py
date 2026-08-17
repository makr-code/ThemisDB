#!/usr/bin/env python3
"""
GitHub CI/CD Workflow Validator
Analyzes ThemisDB .github/workflows/ for common errors and structural issues.
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple

class WorkflowValidator:
    def __init__(self, workflow_dir: str = '.github/workflows'):
        self.workflow_dir = Path(workflow_dir)
        self.results: Dict[str, List[str]] = {}
        self.errors_found = 0
        
    def validate(self) -> int:
        """Run all validations"""
        print("=" * 70)
        print("CI/CD Workflow Validator (Python)")
        print("=" * 70 + "\n")
        
        workflows = sorted(self.workflow_dir.glob('*.yml'))
        if not workflows:
            print(f"❌ No workflow files found in {self.workflow_dir}")
            return 1
        
        print(f"📊 Found {len(workflows)} workflow(s)\n")
        
        for workflow in workflows:
            try:
                self._validate_file(workflow)
            except Exception as e:
                print(f"  ⚠️  {workflow.name}: Error reading file: {e}")
                self.errors_found += 1
        
        self._print_summary()
        return 1 if self.errors_found > 0 else 0
    
    def _validate_file(self, filepath: Path):
        """Validate single workflow file"""
        print(f"📋 {filepath.name}", end=" ")
        
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        issues = []
        
        # 1. Check required fields
        if not re.search(r'^\s*name:\s*', content, re.MULTILINE):
            issues.append("Missing 'name:' field")
        
        if not re.search(r'^\s*on:\s*', content, re.MULTILINE):
            issues.append("Missing 'on:' trigger section")
        
        if not re.search(r'^\s*jobs:\s*$', content, re.MULTILINE):
            issues.append("Missing 'jobs:' section")
        
        if not re.search(r'^\s*permissions:\s*', content, re.MULTILINE):
            issues.append("Missing 'permissions:' field")
        
        # 2. Check for common issues
        issues.extend(self._check_actions(filepath.name, content))
        issues.extend(self._check_indentation(content))
        issues.extend(self._check_logic(content))
        
        if issues:
            print(f" ❌")
            for issue in issues:
                print(f"    └─ {issue}")
            self.errors_found += len(issues)
        else:
            print("✅")
    
    def _check_actions(self, filename: str, content: str) -> List[str]:
        """Check GitHub action usage"""
        issues = []
        
        # Find all 'uses:' lines
        uses_pattern = r'uses:\s+([a-zA-Z0-9/_.\-]+)(?:@([a-fA-F0-9]{40}|v?\d+\.\d+\.\d+|v?\d+))?'
        matches = re.findall(uses_pattern, content)
        
        unpinned = []
        for action, version in matches:
            # Skip local actions (./.github/actions/...)
            if action.startswith('./'):
                continue
            # Skip if already pinned
            if version:
                continue
            # Check if it's an external action
            if '/' in action:
                unpinned.append(action)
        
        if unpinned:
            issues.append(f"⚠️  Found {len(unpinned)} unpinned action(s): {', '.join(unpinned[:3])}")
        
        return issues
    
    def _check_indentation(self, content: str) -> List[str]:
        """Check for YAML indentation issues"""
        issues = []
        lines = content.split('\n')
        indent_errors = []
        
        for i, line in enumerate(lines, 1):
            # Skip empty lines and comments
            if not line.strip() or line.strip().startswith('#'):
                continue
            
            # Check for odd-number indentation (YAML requires multiples of 2)
            match = re.match(r'^( +)', line)
            if match:
                indent = len(match.group(1))
                # Allow 2, 4, 6, 8 spaces
                if indent % 2 != 0:
                    indent_errors.append(f"Line {i}: odd indent ({indent} spaces)")
        
        if indent_errors:
            issues.append(f"⚠️  YAML indentation: {len(indent_errors)} line(s) with odd spacing ({indent_errors[0]}...)")
        
        return issues
    
    def _check_logic(self, content: str) -> List[str]:
        """Check for logical issues"""
        issues = []
        
        # Check for 'if:' conditions without proper value
        if_pattern = r'if:\s*([^\n]+)'
        if_matches = re.findall(if_pattern, content)
        
        suspicious_ifs = []
        for cond in if_matches:
            cond = cond.strip()
            # Check for incomplete conditions
            if cond.endswith('==') or cond.endswith('!=') or cond.endswith('||'):
                suspicious_ifs.append(cond[:50])
        
        if suspicious_ifs:
            issues.append(f"⚠️  Found {len(suspicious_ifs)} incomplete 'if:' condition(s)")
        
        # Check for undefined step outputs
        step_outputs = re.findall(r'steps\.(\w+)\.outputs\.(\w+)', content)
        if step_outputs:
            # This is just informational
            pass
        
        return issues
    
    def _print_summary(self):
        """Print summary"""
        print("\n" + "="*70)
        print("VALIDATION SUMMARY")
        print("="*70)
        print(f"Total issues found: {self.errors_found}")
        if self.errors_found == 0:
            print("✅ All workflows passed validation!")
        else:
            print("⚠️  Please fix the issues above before deploying")
        print("="*70)

def main():
    validator = WorkflowValidator()
    return validator.validate()

if __name__ == '__main__':
    sys.exit(main())
