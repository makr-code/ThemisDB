#!/usr/bin/env python3
"""
Content Module Production TODO Validation Script

Validates that all TODOs in content module files:
1. Are properly classified (Optimization/Feature/Vendor/Other)
2. Have GitHub issue references (e.g., #5751)
3. Have documented rationale in CONTENT_DEFERRED_FEATURES.md

Usage:
    python3 cmake/scripts/validate_content_todos.py [--strict]

Exit codes:
    0 = All TODOs valid (or --strict with warnings only)
    1 = Critical validation failures (blocking)
    2 = Warning-level issues (non-blocking with --strict OFF)
"""

import os
import re
import sys
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# ============================================================================
# Configuration
# ============================================================================

REPO_ROOT = Path(__file__).parent.parent.parent
CONTENT_MODULE_DIR = REPO_ROOT / "src" / "content"
DEFERRED_FEATURES_FILE = REPO_ROOT / "CONTENT_DEFERRED_FEATURES.md"
CLASSIFICATION_REPORT = REPO_ROOT / "CONTENT_PRODUCTION_TODO_CLASSIFICATION_REPORT.md"

# Patterns for TODO detection
TODO_PATTERN = re.compile(r'//\s*TODO\s*\(([^)]+)\):\s*(.+)$', re.MULTILINE)
ISSUE_PATTERN = re.compile(r'#(\d+)')

# Valid categories for TODOs
VALID_CATEGORIES = {'Optimization', 'Feature', 'Vendor', 'Documentation', 'Other'}

# ============================================================================
# Validation Functions
# ============================================================================

class TodoValidator:
    """Validates production TODOs in content module."""
    
    def __init__(self, strict_mode: bool = False):
        self.strict_mode = strict_mode
        self.errors: List[str] = []
        self.warnings: List[str] = []
        self.todos_found: Dict[str, List[Dict]] = {}
        self.classified_todos: Dict[str, List[str]] = {}
        
    def validate(self) -> int:
        """Run all validation checks. Returns exit code."""
        print("=" * 70)
        print("Content Module Production TODO Validation")
        print("=" * 70)
        
        # Step 1: Scan all C++ files for TODOs
        self._scan_todos()
        print(f"\n✓ Scanned {len(self.todos_found)} files")
        print(f"  Total TODOs found: {sum(len(v) for v in self.todos_found.values())}")
        
        # Step 2: Validate TODO format and references
        self._validate_todo_format()
        
        # Step 3: Load classified TODOs from CONTENT_DEFERRED_FEATURES.md
        self._load_classified_todos()
        
        # Step 4: Validate coverage
        self._validate_coverage()
        
        # Print results
        return self._print_results()
    
    def _scan_todos(self):
        """Scan all content module .cpp files for TODO comments."""
        if not CONTENT_MODULE_DIR.exists():
            self.errors.append(f"Content module directory not found: {CONTENT_MODULE_DIR}")
            return
        
        for cpp_file in CONTENT_MODULE_DIR.rglob("*.cpp"):
            try:
                with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    
                todos = []
                for match in TODO_PATTERN.finditer(content):
                    issue_ref = match.group(1)
                    description = match.group(2)
                    line_no = content[:match.start()].count('\n') + 1
                    
                    todos.append({
                        'issue': issue_ref,
                        'description': description,
                        'line': line_no,
                        'file': str(cpp_file.relative_to(REPO_ROOT))
                    })
                
                if todos:
                    self.todos_found[str(cpp_file.relative_to(REPO_ROOT))] = todos
                    
            except Exception as e:
                self.warnings.append(f"Error scanning {cpp_file}: {e}")
    
    def _validate_todo_format(self):
        """Validate that each TODO has proper format and issue reference."""
        total_todos = 0
        valid_todos = 0
        
        for file_path, todos in self.todos_found.items():
            for todo in todos:
                total_todos += 1
                issue = todo['issue']
                
                # Check for issue reference
                if not issue:
                    self.errors.append(
                        f"{file_path}:{todo['line']} — TODO missing issue reference\n"
                        f"  Expected: TODO(#XXXX): description\n"
                        f"  Got: {todo['description'][:60]}..."
                    )
                elif ISSUE_PATTERN.match(issue):
                    valid_todos += 1
                else:
                    self.warnings.append(
                        f"{file_path}:{todo['line']} — TODO has non-standard issue reference: {issue}\n"
                        f"  Expected format: #XXXX or similar"
                    )
        
        print(f"\n✓ TODO Format Validation")
        print(f"  Valid TODOs: {valid_todos}/{total_todos}")
        
        if valid_todos < total_todos:
            if self.strict_mode:
                print(f"  ⚠ {total_todos - valid_todos} TODOs with format issues")
    
    def _load_classified_todos(self):
        """Load classified TODOs from CONTENT_DEFERRED_FEATURES.md."""
        if not DEFERRED_FEATURES_FILE.exists():
            self.warnings.append(
                f"CONTENT_DEFERRED_FEATURES.md not found at {DEFERRED_FEATURES_FILE}"
            )
            return
        
        try:
            with open(DEFERRED_FEATURES_FILE, 'r', encoding='utf-8') as f:
                content = f.read()
                
            # Extract issue references from document
            for category in VALID_CATEGORIES:
                pattern = f"## {category} CATEGORY" if category != 'Documentation' else "## DOCUMENTATION"
                if category == 'Documentation':
                    pattern = "## DOCUMENTATION & CLEANUP CATEGORY"
                
                if pattern in content or category in content:
                    # Extract all issue numbers in this section
                    issues = ISSUE_PATTERN.findall(content[content.find(category):])
                    self.classified_todos[category] = [f"#{issue}" for issue in issues[:20]]  # Limit to section
                    
        except Exception as e:
            self.warnings.append(f"Error loading CONTENT_DEFERRED_FEATURES.md: {e}")
    
    def _validate_coverage(self):
        """Validate that all found TODOs are classified in CONTENT_DEFERRED_FEATURES.md."""
        unclassified = []
        
        for file_path, todos in self.todos_found.items():
            for todo in todos:
                issue = todo['issue']
                
                # Check if issue is in any classification
                found = False
                for category, issues in self.classified_todos.items():
                    if issue in issues:
                        found = True
                        break
                
                if not found and issue:
                    unclassified.append((file_path, todo['line'], issue))
        
        if unclassified:
            if self.strict_mode:
                for file_path, line, issue in unclassified:
                    self.errors.append(
                        f"{file_path}:{line} — TODO {issue} not classified in CONTENT_DEFERRED_FEATURES.md"
                    )
            else:
                for file_path, line, issue in unclassified:
                    self.warnings.append(
                        f"{file_path}:{line} — TODO {issue} not classified in CONTENT_DEFERRED_FEATURES.md"
                    )
    
    def _print_results(self) -> int:
        """Print validation results and return exit code."""
        print("\n" + "=" * 70)
        print("Validation Results")
        print("=" * 70)
        
        if self.errors:
            print(f"\n❌ CRITICAL ERRORS ({len(self.errors)}):")
            for error in self.errors[:10]:  # Show first 10
                print(f"  • {error}")
            if len(self.errors) > 10:
                print(f"  ... and {len(self.errors) - 10} more errors")
        
        if self.warnings:
            print(f"\n⚠ WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings[:10]:  # Show first 10
                print(f"  • {warning}")
            if len(self.warnings) > 10:
                print(f"  ... and {len(self.warnings) - 10} more warnings")
        
        # Summary
        print("\n" + "-" * 70)
        total_todos = sum(len(v) for v in self.todos_found.values())
        print(f"Summary:")
        print(f"  Files scanned: {len(self.todos_found)}")
        print(f"  Total TODOs: {total_todos}")
        print(f"  Classified TODOs: {sum(len(v) for v in self.classified_todos.values())}")
        print(f"  Critical errors: {len(self.errors)}")
        print(f"  Warnings: {len(self.warnings)}")
        
        # Determine exit code
        if self.errors:
            print("\n❌ Validation FAILED (blocking errors)")
            return 1
        elif self.warnings and self.strict_mode:
            print("\n⚠ Validation passed with warnings (strict mode)")
            return 0
        elif self.warnings:
            print("\n✓ Validation passed with warnings")
            return 0
        else:
            print("\n✓ Validation PASSED")
            return 0

# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    """Main entry point."""
    strict_mode = '--strict' in sys.argv
    
    validator = TodoValidator(strict_mode=strict_mode)
    exit_code = validator.validate()
    
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
