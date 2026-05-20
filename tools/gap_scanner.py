#!/usr/bin/env python3
"""
ThemisDB Implementation Gap Scanner

Scanns for:
- Stubs, mocks, simulations
- TODO/FIXME comments
- Empty/trivial implementations
- Incomplete feature gates
- Unused/untested code paths
"""

import os
import re
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from collections import defaultdict

@dataclass
class CodeGap:
    file: str
    line_num: int
    category: str  # "stub", "todo", "empty", "unimplemented", "unused", "incomplete"
    pattern: str
    context: str  # surrounding code snippet
    severity: str  # "critical", "high", "medium", "low"
    notes: str = ""

class GapScanner:
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.src_dir = self.repo_root / "src"
        self.include_dir = self.repo_root / "include"
        self.tests_dir = self.repo_root / "tests"
        self.benchmarks_dir = self.repo_root / "benchmarks"
        
        # Patterns to detect gaps
        self.patterns = {
            "stub": [
                r"//\s*STUB(?:\s|:|NOTE)?",
                r"//\s*SIMULATION(?:\s|:|NOTE)?",
                r"//\s*MOCK(?:\s|:|NOTE)?",
                r"//\s*PLACEHOLDER",
                r"//\s*NOT_IMPLEMENTED",
                r"//\s*FAKE:",
            ],
            "todo": [
                r"//\s*TODO[:\s]",
                r"//\s*FIXME[:\s]",
                r"//\s*XXX[:\s]",
                r"/\*\s*TODO[:\s]",
            ],
            "unimplemented": [
                r'throw\s+std::runtime_error\s*\(\s*["\'].*not\s+implement',
                r'throw\s+std::logic_error\s*\(\s*["\'].*not\s+implement',
                r'return\s+std::make_optional\s*\(\s*\);',
                r'return\s+Result::\s*error',
                r'return\s+\{\s*\};',  # empty return
            ],
            "empty_body": [
                r'{\s*(?://.*?)?\s*}',  # empty block
                r'{\s*return\s*;\s*}',   # just return
            ],
        }
        
        # Multi-line patterns
        self.multiline_patterns = {
            "incomplete_switch": r'switch\s*\([^)]+\)\s*{[^}]*(?<!default\s*:)[^}]*}',
        }
    
    def get_modules(self) -> List[str]:
        """Get list of all modules in src/"""
        modules = []
        for item in self.src_dir.iterdir():
            if item.is_dir() and not item.name.startswith('.'):
                modules.append(item.name)
        return sorted(modules)
    
    def scan_file(self, filepath: Path) -> List[CodeGap]:
        """Scan a single file for gaps"""
        gaps = []
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Warning: Could not read {filepath}: {e}")
            return gaps
        
        for idx, line in enumerate(lines, 1):
            # Check stub patterns
            for pattern in self.patterns.get("stub", []):
                if re.search(pattern, line, re.IGNORECASE):
                    gap = CodeGap(
                        file=str(filepath.relative_to(self.repo_root)),
                        line_num=idx,
                        category="stub",
                        pattern=pattern,
                        context=line.strip(),
                        severity="high"
                    )
                    gaps.append(gap)
            
            # Check TODO patterns
            for pattern in self.patterns.get("todo", []):
                if re.search(pattern, line, re.IGNORECASE):
                    gap = CodeGap(
                        file=str(filepath.relative_to(self.repo_root)),
                        line_num=idx,
                        category="todo",
                        pattern=pattern,
                        context=line.strip(),
                        severity="medium"
                    )
                    gaps.append(gap)
            
            # Check unimplemented patterns
            for pattern in self.patterns.get("unimplemented", []):
                if re.search(pattern, line):
                    gap = CodeGap(
                        file=str(filepath.relative_to(self.repo_root)),
                        line_num=idx,
                        category="unimplemented",
                        pattern=pattern,
                        context=line.strip(),
                        severity="critical"
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[CodeGap]]:
        """Scan all files in a module"""
        result = {
            "src": [],
            "include": [],
            "tests": [],
            "benchmarks": []
        }
        
        # Scan src/
        src_path = self.src_dir / module
        if src_path.exists():
            for filepath in src_path.rglob("*.cpp"):
                result["src"].extend(self.scan_file(filepath))
            for filepath in src_path.rglob("*.hpp"):
                result["src"].extend(self.scan_file(filepath))
            for filepath in src_path.rglob("*.h"):
                result["src"].extend(self.scan_file(filepath))
        
        # Scan include/
        inc_path = self.include_dir / module
        if inc_path.exists():
            for filepath in inc_path.rglob("*.hpp"):
                result["include"].extend(self.scan_file(filepath))
            for filepath in inc_path.rglob("*.h"):
                result["include"].extend(self.scan_file(filepath))
        
        # Scan tests/
        for filepath in self.tests_dir.glob(f"*{module}*.cpp"):
            result["tests"].extend(self.scan_file(filepath))
        
        # Scan benchmarks/
        for filepath in self.benchmarks_dir.glob(f"*{module}*.cpp"):
            result["benchmarks"].extend(self.scan_file(filepath))
        
        return result
    
    def run_full_scan(self) -> Dict[str, Dict]:
        """Scan all modules"""
        modules = self.get_modules()
        results = {}
        
        for module in modules:
            print(f"Scanning module: {module}...", end=" ", flush=True)
            gaps = self.scan_module(module)
            
            # Count gaps
            total_gaps = sum(len(v) for v in gaps.values())
            print(f"{total_gaps} gaps found")
            
            results[module] = {
                "gaps_by_location": gaps,
                "summary": {
                    "total": total_gaps,
                    "critical": sum(1 for v in gaps.values() for g in v if g.severity == "critical"),
                    "high": sum(1 for v in gaps.values() for g in v if g.severity == "high"),
                    "medium": sum(1 for v in gaps.values() for g in v if g.severity == "medium"),
                    "low": sum(1 for v in gaps.values() for g in v if g.severity == "low"),
                }
            }
        
        return results
    
    def save_results(self, results: Dict, output_dir: str = "ai_working"):
        """Save scan results to JSON files"""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        # Save per-module reports
        for module, data in results.items():
            module_report = {
                "module": module,
                "summary": data["summary"],
                "gaps": []
            }
            
            for location, gaps in data["gaps_by_location"].items():
                for gap in gaps:
                    module_report["gaps"].append({
                        **asdict(gap),
                        "location": location
                    })
            
            output_file = output_path / f"gap_scan_{module}.json"
            with open(output_file, 'w') as f:
                json.dump(module_report, f, indent=2)
            print(f"Saved: {output_file}")
        
        # Save aggregate report
        aggregate = {}
        for module, data in results.items():
            aggregate[module] = data["summary"]
        
        agg_file = output_path / "gap_scan_aggregate.json"
        with open(agg_file, 'w') as f:
            json.dump(aggregate, f, indent=2)
        print(f"Saved: {agg_file}")
        
        return output_path

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Scan ThemisDB for implementation gaps")
    parser.add_argument("--repo", default=".", help="Repository root")
    parser.add_argument("--output", default="ai_working", help="Output directory")
    parser.add_argument("--module", help="Scan specific module only")
    args = parser.parse_args()
    
    scanner = GapScanner(args.repo)
    
    if args.module:
        print(f"Scanning module: {args.module}")
        gaps = scanner.scan_module(args.module)
        results = {args.module: {"gaps_by_location": gaps, "summary": {}}}
    else:
        results = scanner.run_full_scan()
    
    scanner.save_results(results, args.output)
    
    # Print summary
    print("\n" + "="*60)
    print("GAP SCAN SUMMARY")
    print("="*60)
    
    for module, data in results.items():
        summary = data["summary"]
        total = summary["total"]
        if total > 0:
            print(f"\n{module:25} {total:3} gaps "
                  f"(C:{summary['critical']} H:{summary['high']} M:{summary['medium']})")

if __name__ == "__main__":
    main()
