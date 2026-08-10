#!/usr/bin/env python3
"""
Scaffold Missing Test Suites for Zero-Test Modules

Purpose:
  Identifies modules with zero automated tests and generates boilerplate test
  files with acceptance criteria extracted from ROADMAP.md. Enables rapid
  test coverage gap closure.

Usage:
  # Generate scaffold for single module
  python scaffold_missing_test_suites.py --module ethics_ai --output-dir /tmp/scaffold

  # Generate scaffolds for all zero-test modules
  python scaffold_missing_test_suites.py --all-zero --output-dir /tmp/scaffold

  # Generate and create PR branch
  python scaffold_missing_test_suites.py --module ethics_ai --create-pr-branch

Inputs:
  - src/<module>/ROADMAP.md (acceptance criteria)
  - tests/<module>/ (check for existing tests)
  - CMakeLists.txt (to understand test registration pattern)

Outputs:
  - tests/<module>/test_<module>_phase<N>_focused.cpp (template)
  - Optional: PR branch with generated scaffolds

Author: Platform Release Team
License: Apache 2.0
"""

import argparse
import os
import re
import sys
import subprocess
import json
from pathlib import Path
from typing import List, Dict, Optional, Tuple


class ZeroTestModuleScaffold:
    """Generates test file scaffolds for modules with zero automated tests."""

    # Modules identified in maturity review as having zero focused tests
    ZERO_TEST_MODULES = [
        'ethics_ai',
        'retrieval',
        'evaluation',
        'prompt_engineering',
        'knowledge_graph',
        'data_ingestion',
        'compliance_audit',
        'performance_tuning',
    ]

    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.src_root = repo_root / 'src'
        self.tests_root = repo_root / 'tests'
        self.benchmarks_root = repo_root / 'benchmarks'

    def find_zero_test_modules(self) -> List[str]:
        """Identify modules with zero focused test files.
        
        Returns:
          List of module names with no test_*_focused.cpp files.
        """
        zero_test = []
        for module in self.ZERO_TEST_MODULES:
            test_dir = self.tests_root / module
            if not test_dir.exists():
                zero_test.append(module)
                continue
            
            # Check for focused test files
            focused_tests = list(test_dir.glob('test_*_focused.cpp'))
            if not focused_tests:
                zero_test.append(module)
        
        return zero_test

    def extract_phase_info(self, module: str) -> Dict[str, List[str]]:
        """Extract phase definitions and acceptance criteria from ROADMAP.md.
        
        Args:
          module: Module name (e.g., 'ethics_ai')
        
        Returns:
          Dict mapping phase_N -> [acceptance_criteria_list]
        
        Example:
          {
            'phase_1': ['Initialize module structure', 'Define core interfaces'],
            'phase_2': ['Implement core algorithms', 'Add error handling']
          }
        """
        roadmap_path = self.src_root / module / 'ROADMAP.md'
        if not roadmap_path.exists():
            return {}
        
        phase_info = {}
        current_phase = None
        criteria_lines = []
        
        with open(roadmap_path, 'r') as f:
            for line in f:
                line = line.rstrip()
                
                # Detect phase header: "## Implementation Phases" or "### Phase N"
                if match := re.match(r'^### Phase (\d+)', line):
                    # Save previous phase
                    if current_phase:
                        phase_info[current_phase] = criteria_lines
                    
                    current_phase = f'phase_{match.group(1)}'
                    criteria_lines = []
                
                # Collect acceptance criteria (lines with checkboxes)
                elif current_phase and re.match(r'^\s*-\s*\[\s*[\[\]~x?!I|P]\s*\]', line):
                    # Extract text after checkbox
                    if match := re.search(r'\]\s*(.+?)(?:\s*\(Target:|$)', line):
                        criteria = match.group(1).strip()
                        if criteria:
                            criteria_lines.append(criteria)
        
        # Save last phase
        if current_phase:
            phase_info[current_phase] = criteria_lines
        
        return phase_info

    def generate_test_template(
        self, module: str, phase: int, criteria: List[str]
    ) -> str:
        """Generate C++ test file template for acceptance criteria.
        
        Args:
          module: Module name
          phase: Phase number
          criteria: List of acceptance criteria
        
        Returns:
          C++ source code as string
        """
        # Sanitize module name for C++ identifier
        cpp_module = re.sub(r'[^a-z0-9_]', '_', module.lower())
        
        # Build test case names from criteria
        test_cases = []
        for i, criterion in enumerate(criteria[:8], 1):  # Limit to 8 tests
            # Convert criterion to CamelCase test name
            test_name = re.sub(
                r'[^a-zA-Z0-9]',
                ' ',
                criterion
            ).title().replace(' ', '')
            test_name = f'{cpp_module}Phase{phase}Criterion{i}_{test_name}'[:60]
            test_cases.append((test_name, criterion))
        
        # Generate C++ code
        template = f'''// Generated test scaffold: Phase {phase} acceptance criteria
// Module: {module}
// Generated: {subprocess.check_output(['date', '-Iseconds']).decode().strip()}
//
// This file contains placeholder test cases for Phase {phase} acceptance criteria.
// Implement the actual test logic to verify acceptance criteria.
//
// ACCEPTANCE CRITERIA (from ROADMAP.md Phase {phase}):
'''
        
        for i, criterion in enumerate(criteria, 1):
            template += f'// {i}. {criterion}\n'
        
        template += f'''//

#include <gtest/gtest.h>
#include <memory>
#include "{cpp_module}/{cpp_module}.h"

namespace themisdb::{cpp_module} {{

// Test fixture for Phase {phase} acceptance criteria
class {cpp_module.capitalize()}Phase{phase}Test : public ::testing::Test {{
 protected:
  void SetUp() override {{
    // TODO: Initialize test fixtures for Phase {phase}
    // Example: module_ = std::make_unique<{cpp_module.capitalize()}Module>();
  }}

  void TearDown() override {{
    // TODO: Clean up resources if needed
  }}

  // TODO: Add member variables for test fixtures
}};

'''
        
        for test_name, criterion in test_cases:
            template += f'''// Acceptance Criterion: {criterion}
TEST_F({cpp_module.capitalize()}Phase{phase}Test, {test_name}) {{
  // TODO: Implement test for acceptance criterion
  // This test should verify: {criterion}
  
  // Placeholder: assert something meaningful
  EXPECT_TRUE(true);
}}

'''
        
        template += f'''}}  // namespace themisdb::{cpp_module}

// Phase {phase} test group
// Gate: GATE-PHASE{phase}-<module>
// Target Acceptance: All tests PASS
// Baseline: 0% (new module)
'''
        
        return template

    def get_existing_test_count(self, module: str) -> Tuple[int, int]:
        """Count existing unit and focused tests for a module.
        
        Returns:
          (unit_test_count, focused_test_count)
        """
        test_dir = self.tests_root / module
        if not test_dir.exists():
            return 0, 0
        
        unit_tests = len(list(test_dir.glob('test_*.cpp'))) - len(
            list(test_dir.glob('test_*_focused.cpp'))
        )
        focused_tests = len(list(test_dir.glob('test_*_focused.cpp')))
        
        return unit_tests, focused_tests

    def generate_scaffold(
        self,
        module: str,
        output_dir: Path,
    ) -> Optional[Path]:
        """Generate test scaffold for a single module.
        
        Args:
          module: Module name
          output_dir: Directory to write scaffold files
        
        Returns:
          Path to generated test file, or None if unable to generate
        """
        phase_info = self.extract_phase_info(module)
        if not phase_info:
            print(f"⚠ No ROADMAP.md found for {module}; skipping")
            return None
        
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate test file for Phase 1 (most critical)
        if 'phase_1' in phase_info:
            criteria = phase_info['phase_1']
            test_code = self.generate_test_template(module, 1, criteria)
            
            # Determine output file name
            test_file = output_dir / f'test_{module}_phase1_focused.cpp'
            
            with open(test_file, 'w') as f:
                f.write(test_code)
            
            print(f"✅ Generated: {test_file}")
            print(f"   Criteria count: {len(criteria)}")
            print(f"   Test cases: {min(8, len(criteria))}")
            
            return test_file
        
        return None

    def update_cmakelists(
        self, module: str, tests_dir: Path
    ) -> bool:
        """Update CMakeLists.txt to register generated test.
        
        Args:
          module: Module name
          tests_dir: Tests directory
        
        Returns:
          True if updated successfully
        """
        cmake_file = tests_dir / module / 'CMakeLists.txt'
        if not cmake_file.exists():
            print(f"⚠ CMakeLists.txt not found for {module}; manual registration needed")
            return False
        
        test_name = f'test_{module}_phase1_focused'
        
        with open(cmake_file, 'r') as f:
            content = f.read()
        
        # Check if already registered
        if test_name in content:
            print(f"ℹ {test_name} already registered in CMakeLists.txt")
            return True
        
        # Add registration line
        registration = (
            f'\nthemis_register_module_focused_test('
            f'\n  TARGET {test_name}'
            f'\n  SOURCE {test_name}.cpp'
            f'\n  MODULE_PATH "{module}"'
            f'\n  TIMEOUT 120'
            f'\n)\n'
        )
        
        content += registration
        
        with open(cmake_file, 'w') as f:
            f.write(content)
        
        print(f"✅ Updated CMakeLists.txt: {cmake_file}")
        return True

    def create_pr_branch(self, module: str, scaffolds: List[Path]) -> str:
        """Create a git branch and commit scaffolds.
        
        Args:
          module: Module name
          scaffolds: List of generated test file paths
        
        Returns:
          Branch name created
        """
        branch_name = f'scaffold/test-gap-{module}'
        
        # Create branch
        subprocess.run(
            ['git', 'checkout', '-b', branch_name],
            cwd=self.repo_root,
            check=True,
            capture_output=True
        )
        
        # Stage files
        for scaffold in scaffolds:
            subprocess.run(
                ['git', 'add', str(scaffold)],
                cwd=self.repo_root,
                check=True,
                capture_output=True
            )
        
        # Commit
        commit_msg = (
            f"test: scaffold Phase 1 tests for {module}\n\n"
            f"Generated test stubs from ROADMAP.md acceptance criteria.\n"
            f"Implementation required to complete Phase 1 gate closure.\n\n"
            f"Related: [test-gap] {module}\n"
            f"Autogenerated: scaffold_missing_test_suites.py"
        )
        
        subprocess.run(
            ['git', 'commit', '-m', commit_msg],
            cwd=self.repo_root,
            check=True,
            capture_output=True
        )
        
        print(f"✅ Created branch: {branch_name}")
        return branch_name


def main():
    parser = argparse.ArgumentParser(
        description='Generate test file scaffolds for zero-test modules'
    )
    parser.add_argument(
        '--module',
        help='Module name (e.g., ethics_ai)'
    )
    parser.add_argument(
        '--all-zero',
        action='store_true',
        help='Generate scaffolds for all zero-test modules'
    )
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=Path('/tmp/scaffold'),
        help='Output directory for scaffold files'
    )
    parser.add_argument(
        '--create-pr-branch',
        action='store_true',
        help='Create git branch and commit scaffolds'
    )
    parser.add_argument(
        '--list-zero-modules',
        action='store_true',
        help='List modules with zero tests and exit'
    )

    args = parser.parse_args()

    repo_root = Path(__file__).parent.parent.parent
    scaffolder = ZeroTestModuleScaffold(repo_root)

    if args.list_zero_modules:
        zero_modules = scaffolder.find_zero_test_modules()
        print(f"Modules with zero focused tests ({len(zero_modules)}):")
        for module in zero_modules:
            unit, focused = scaffolder.get_existing_test_count(module)
            print(f"  • {module:25} (unit: {unit}, focused: {focused})")
        return 0

    modules_to_scaffold = []
    if args.all_zero:
        modules_to_scaffold = scaffolder.find_zero_test_modules()
        print(f"📋 Generating scaffolds for {len(modules_to_scaffold)} zero-test modules...")
    elif args.module:
        modules_to_scaffold = [args.module]
    else:
        parser.print_help()
        return 1

    all_scaffolds = []
    for module in modules_to_scaffold:
        try:
            test_file = scaffolder.generate_scaffold(module, args.output_dir / module)
            if test_file:
                all_scaffolds.append(test_file)
                
                # Update CMakeLists.txt if creating PR branch
                if args.create_pr_branch:
                    tests_dir = repo_root / 'tests'
                    scaffolder.update_cmakelists(module, tests_dir)
        
        except Exception as e:
            print(f"❌ Error processing {module}: {e}")
            return 1

    if args.create_pr_branch and all_scaffolds:
        if len(modules_to_scaffold) == 1:
            branch = scaffolder.create_pr_branch(
                modules_to_scaffold[0],
                all_scaffolds
            )
            print(f"\n✅ Ready to push: git push origin {branch}")
        else:
            print(f"\n⚠ Multiple modules; create separate branches:")
            for module in modules_to_scaffold:
                print(f"  git checkout -b scaffold/test-gap-{module}")
    else:
        print(f"\n✅ Generated {len(all_scaffolds)} scaffold files in {args.output_dir}")
        print("Next: implement test logic and run: pytest tests/")

    return 0


if __name__ == '__main__':
    sys.exit(main())
