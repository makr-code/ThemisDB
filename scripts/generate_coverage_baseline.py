#!/usr/bin/env python3
"""
generate_coverage_baseline.py - Create baseline coverage metrics from lcov/gcov output

Usage:
  python3 generate_coverage_baseline.py coverage.lcov [--output baseline.txt] [--tier-mapping tiers.json]

Output format:
  # Module Coverage Baseline (YYYY-MM-DD HH:MM:SS UTC)
  # Format: module_name | lines_total | lines_executable | baseline_coverage_% | tier
  server | 45230 | 38200 | 75.2 | A
  ...

This baseline serves as the reference point for coverage delta tracking in nightly reports
and optional PR gates.
"""

import json
import re
import sys
import argparse
from datetime import datetime
from pathlib import Path
from collections import defaultdict
from dataclasses import dataclass

@dataclass
class ModuleCoverage:
    """Coverage metrics for a single module"""
    name: str
    lines_total: int
    lines_executable: int
    lines_covered: int
    branches_total: int
    branches_covered: int
    tier: str
    
    @property
    def coverage_percent(self) -> float:
        """Calculate coverage percentage"""
        if self.lines_executable == 0:
            return 0.0
        return round(100.0 * self.lines_covered / self.lines_executable, 1)
    
    @property
    def branch_coverage_percent(self) -> float:
        """Calculate branch coverage percentage"""
        if self.branches_total == 0:
            return 0.0
        return round(100.0 * self.branches_covered / self.branches_total, 1)


class CoverageParser:
    """Parse lcov output files and extract per-module metrics"""
    
    # Module tier mapping (override with --tier-mapping)
    DEFAULT_TIERS = {
        'server': 'A', 'auth': 'A', 'storage': 'A', 'query': 'A', 
        'transaction': 'A', 'sharding': 'A',
        'index': 'B', 'rag': 'B', 'llm': 'B', 'security': 'B',
        'geo': 'B', 'analytics': 'B',
        'distributed_tensor': 'C', 'llama_cpp': 'C', 'stable_diffusion': 'C',
        'execution': 'C',
    }
    
    def __init__(self, tier_mapping=None):
        """Initialize parser with optional tier mapping override"""
        self.tiers = tier_mapping or self.DEFAULT_TIERS
        self.modules = defaultdict(lambda: ModuleCoverage(
            name='', lines_total=0, lines_executable=0, lines_covered=0,
            branches_total=0, branches_covered=0, tier='?'
        ))
    
    def parse_lcov_file(self, lcov_path: str) -> dict:
        """
        Parse lcov format coverage file
        
        Format:
        TN:<test_name>
        SF:<source_file>
        FN:<line>,<func_name>
        FNDA:<execution_count>,<func_name>
        FNF:<number_of_functions>
        FNH:<number_of_functions_hit>
        DA:<line>,<execution_count>
        LH:<lines_hit>
        LF:<lines_found>
        BRDA:<line>,<block>,<branch>,<taken>
        BRH:<branches_hit>
        BRF:<branches_found>
        end_of_record
        """
        
        with open(lcov_path, 'r') as f:
            content = f.read()
        
        # Split by end_of_record markers
        records = content.split('end_of_record')
        
        for record in records:
            if not record.strip():
                continue
            
            lines = record.strip().split('\n')
            current_file = None
            lh = lf = brh = brf = 0
            
            for line in lines:
                if line.startswith('SF:'):
                    current_file = line[3:].strip()
                elif line.startswith('LH:'):
                    lh = int(line[3:].strip())
                elif line.startswith('LF:'):
                    lf = int(line[3:].strip())
                elif line.startswith('BRH:'):
                    brh = int(line[4:].strip())
                elif line.startswith('BRF:'):
                    brf = int(line[4:].strip())
            
            if current_file:
                self._add_file_coverage(current_file, lh, lf, brh, brf)
        
        return self.modules
    
    def _add_file_coverage(self, file_path: str, lines_hit: int, lines_found: int,
                          branches_hit: int, branches_found: int):
        """
        Add coverage metrics for a single file to the appropriate module
        """
        # Extract module name from file path
        # e.g., src/server/query_engine.cpp -> server
        # tests/storage/test_index.cpp -> storage
        # include/auth/auth_handler.h -> auth
        
        module_name = self._extract_module_name(file_path)
        if module_name not in self.modules:
            self.modules[module_name] = ModuleCoverage(
                name=module_name,
                lines_total=0,
                lines_executable=0,
                lines_covered=0,
                branches_total=0,
                branches_covered=0,
                tier=self.tiers.get(module_name, '?')
            )
        
        m = self.modules[module_name]
        m.lines_total += lines_found
        m.lines_executable += lines_found  # Executable = found in lcov
        m.lines_covered += lines_hit
        m.branches_total += branches_found
        m.branches_covered += branches_hit
    
    @staticmethod
    def _extract_module_name(file_path: str) -> str:
        """
        Extract module name from file path
        Examples:
          src/server/query_engine.cpp -> server
          tests/auth/test_token.cpp -> auth
          include/storage/db.h -> storage
          benchmarks/bench_server.cpp -> server
        """
        
        # Try direct pattern match: src/<module>/ or tests/<module>/
        match = re.search(r'(src|tests|include|benchmarks)/([^/]+)/', file_path)
        if match:
            return match.group(2)
        
        # Fallback: use first directory component
        parts = file_path.split('/')
        if len(parts) > 1 and parts[0] != '.':
            return parts[1]
        
        return 'unknown'


def write_baseline_file(modules: dict, output_path: str):
    """Write baseline metrics to file in standard format"""
    
    timestamp = datetime.utcnow().isoformat() + 'Z'
    
    with open(output_path, 'w') as f:
        f.write(f"# Module Coverage Baseline ({timestamp})\n")
        f.write("# Format: module_name | lines_total | lines_executable | lines_covered | coverage_% | branches_covered/branches_total | branch_% | tier\n")
        f.write("# Generated by: generate_coverage_baseline.py\n")
        f.write("#\n")
        
        # Sort by tier, then by coverage descending
        sorted_modules = sorted(modules.values(), 
                               key=lambda m: (m.tier, -m.coverage_percent))
        
        for m in sorted_modules:
            if m.lines_executable == 0:
                continue
            
            line = f"{m.name} | {m.lines_total} | {m.lines_executable} | {m.lines_covered} | {m.coverage_percent}% | {m.branches_covered}/{m.branches_total} | {m.branch_coverage_percent}% | {m.tier}\n"
            f.write(line)
    
    print(f"✓ Baseline written to {output_path}")


def generate_json_report(modules: dict, output_path: str):
    """Generate detailed JSON report for analysis"""
    
    report = {
        'generated_at': datetime.utcnow().isoformat() + 'Z',
        'summary': {},
        'modules': {}
    }
    
    total_lines_covered = 0
    total_lines_executable = 0
    total_branches_covered = 0
    total_branches_total = 0
    
    for tier in ['A', 'B', 'C']:
        tier_modules = [m for m in modules.values() if m.tier == tier and m.lines_executable > 0]
        
        if tier_modules:
            tier_cov = sum(m.lines_covered for m in tier_modules)
            tier_exec = sum(m.lines_executable for m in tier_modules)
            tier_pct = 100.0 * tier_cov / tier_exec if tier_exec > 0 else 0.0
            
            report['summary'][f'tier_{tier}'] = {
                'module_count': len(tier_modules),
                'lines_covered': tier_cov,
                'lines_executable': tier_exec,
                'coverage_percent': round(tier_pct, 1)
            }
            
            total_lines_covered += tier_cov
            total_lines_executable += tier_exec
            total_branches_covered += sum(m.branches_covered for m in tier_modules)
            total_branches_total += sum(m.branches_total for m in tier_modules)
    
    # Overall summary
    overall_pct = 100.0 * total_lines_covered / total_lines_executable if total_lines_executable > 0 else 0.0
    report['summary']['overall'] = {
        'lines_covered': total_lines_covered,
        'lines_executable': total_lines_executable,
        'coverage_percent': round(overall_pct, 1),
        'branches_covered': total_branches_covered,
        'branches_total': total_branches_total,
        'branch_coverage_percent': round(100.0 * total_branches_covered / total_branches_total, 1) if total_branches_total > 0 else 0.0
    }
    
    # Per-module details
    for m in sorted(modules.values(), key=lambda x: x.name):
        if m.lines_executable == 0:
            continue
        
        report['modules'][m.name] = {
            'tier': m.tier,
            'lines_covered': m.lines_covered,
            'lines_executable': m.lines_executable,
            'coverage_percent': m.coverage_percent,
            'branches_covered': m.branches_covered,
            'branches_total': m.branches_total,
            'branch_coverage_percent': m.branch_coverage_percent
        }
    
    with open(output_path, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"✓ JSON report written to {output_path}")


def main():
    """Main entry point"""
    
    parser = argparse.ArgumentParser(
        description='Generate coverage baseline from lcov output'
    )
    parser.add_argument('lcov_file', 
                       help='Path to lcov coverage file (from lcov --capture)')
    parser.add_argument('--output', '-o', 
                       default='.github/coverage-baseline.txt',
                       help='Output baseline file (default: .github/coverage-baseline.txt)')
    parser.add_argument('--json', '-j', 
                       help='Also generate JSON report to this path')
    parser.add_argument('--tier-mapping', '-t',
                       help='JSON file with module->tier mapping (overrides defaults)')
    
    args = parser.parse_args()
    
    # Load tier mapping if provided
    tier_mapping = None
    if args.tier_mapping:
        with open(args.tier_mapping) as f:
            tier_mapping = json.load(f)
    
    # Parse coverage data
    print(f"Parsing {args.lcov_file}...")
    parser = CoverageParser(tier_mapping)
    modules = parser.parse_lcov_file(args.lcov_file)
    
    print(f"Found {len(modules)} modules with coverage data")
    
    # Write output files
    write_baseline_file(modules, args.output)
    
    if args.json:
        generate_json_report(modules, args.json)
    
    # Print summary
    print("\n=== Coverage Baseline Summary ===")
    for tier in ['A', 'B', 'C']:
        tier_mods = [m for m in modules.values() if m.tier == tier and m.lines_executable > 0]
        if tier_mods:
            avg_cov = sum(m.coverage_percent for m in tier_mods) / len(tier_mods)
            print(f"Tier {tier}: {len(tier_mods)} modules, avg coverage {avg_cov:.1f}%")


if __name__ == '__main__':
    main()
