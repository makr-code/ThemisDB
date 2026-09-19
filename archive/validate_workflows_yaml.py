#!/usr/bin/env python3
"""
CI/CD Workflow Validator using YAML parsing
Better indentation detection by respecting YAML structure
"""

import sys
from pathlib import Path

try:
    import yaml
    YAML_AVAILABLE = True
except ImportError:
    YAML_AVAILABLE = False

class WorkflowValidator:
    def __init__(self, workflow_dir: str = '.github/workflows'):
        self.workflow_dir = Path(workflow_dir)
        self.errors = []
        
    def validate(self) -> int:
        """Run all validations"""
        print("=" * 70)
        print("CI/CD Workflow Validator (YAML-aware)")
        print("=" * 70 + "\n")
        
        if not YAML_AVAILABLE:
            print("WARNING: PyYAML not installed. Running basic checks only.\n")
        
        workflows = sorted(self.workflow_dir.glob('*.yml'))
        if not workflows:
            print(f"ERROR: No workflow files found in {self.workflow_dir}")
            return 1
        
        print(f"Found {len(workflows)} workflow(s)\n")
        passed = 0
        failed = 0
        
        for workflow in workflows:
            result = self._validate_file(workflow)
            if result:
                failed += 1
                print(f"  FAILED: {workflow.name}")
                for issue in result:
                    print(f"    - {issue}")
            else:
                passed += 1
                print(f"  PASS: {workflow.name}")
        
        print("\n" + "=" * 70)
        print(f"RESULTS: {passed} passed, {failed} failed")
        print("=" * 70)
        
        return 1 if failed > 0 else 0
    
    def _validate_file(self, filepath: Path) -> list:
        """Validate single workflow file"""
        issues = []
        
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        # Try YAML parsing if available
        if YAML_AVAILABLE:
            try:
                yaml.safe_load(content)
                # YAML is valid
            except yaml.YAMLError as e:
                issues.append(f"YAML Parse Error: {str(e)[:100]}")
                return issues
        
        # Basic checks
        if 'name:' not in content:
            issues.append("Missing 'name:' field")
        
        if 'on:' not in content:
            issues.append("Missing 'on:' trigger section")
        
        if 'jobs:' not in content:
            issues.append("Missing 'jobs:' section")
        
        if 'permissions:' not in content:
            issues.append("Missing 'permissions:' field")
        
        return issues

def main():
    validator = WorkflowValidator()
    return validator.validate()

if __name__ == '__main__':
    sys.exit(main())
