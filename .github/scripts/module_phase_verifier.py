#!/usr/bin/env python3
"""
Module Phase Verifier

Scans ROADMAP.md files to verify phase state matches implementation evidence.
Detects discrepancies between roadmap checkboxes and actual code/tests/benchmarks.

Usage:
  python module_phase_verifier.py --module <name> --phase <N> --verify-acceptance
  python module_phase_verifier.py --module <name> --phase <N> --check-test-ratio
  python module_phase_verifier.py --all-modules --generate-report
  python module_phase_verifier.py --detect-cycles
"""

import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from enum import Enum

# Repository root
REPO_ROOT = Path(__file__).parent.parent.parent
SRC_DIR = REPO_ROOT / "src"
TESTS_DIR = REPO_ROOT / "tests"
BENCHMARKS_DIR = REPO_ROOT / "benchmarks"


class PhaseStatus(Enum):
    PENDING = "[ ]"
    IN_PROGRESS = "[~]"
    COMPLETE = "[x]"


@dataclass
class PhaseInfo:
    module: str
    phase_num: int
    status: PhaseStatus
    test_count: int = 0
    benchmark_count: int = 0
    acceptance_criteria: List[str] = None
    evidence_links: List[str] = None


def find_module_roadmap(module_name: str) -> Optional[Path]:
    """Find ROADMAP.md for a module."""
    roadmap_path = SRC_DIR / module_name / "ROADMAP.md"
    if roadmap_path.exists():
        return roadmap_path
    return None


def extract_phase_section(roadmap_text: str, phase_num: int) -> Optional[Tuple[int, int]]:
    """Extract line numbers for Phase X section."""
    lines = roadmap_text.split('\n')
    phase_pattern = rf"^## Phase {phase_num}[:\s]"
    start_idx = None
    
    for idx, line in enumerate(lines):
        if re.match(phase_pattern, line):
            start_idx = idx
            break
    
    if start_idx is None:
        return None
    
    # Find next phase or end of document
    end_idx = len(lines)
    next_phase_pattern = rf"^## Phase {phase_num + 1}[:\s]"
    for idx in range(start_idx + 1, len(lines)):
        if re.match(next_phase_pattern, lines[idx]):
            end_idx = idx
            break
    
    return (start_idx, end_idx)


def get_phase_status(roadmap_text: str, phase_num: int) -> Optional[PhaseStatus]:
    """Extract phase status checkbox."""
    section = extract_phase_section(roadmap_text, phase_num)
    if not section:
        return None
    
    start, end = section
    lines = roadmap_text.split('\n')[start:end]
    
    # Look for first checkbox in section
    for line in lines:
        if "[ ]" in line:
            return PhaseStatus.PENDING
        elif "[~]" in line:
            return PhaseStatus.IN_PROGRESS
        elif "[x]" in line:
            return PhaseStatus.COMPLETE
    
    return None


def extract_acceptance_criteria(roadmap_text: str, phase_num: int) -> List[str]:
    """Extract acceptance criteria from phase section."""
    section = extract_phase_section(roadmap_text, phase_num)
    if not section:
        return []
    
    start, end = section
    lines = roadmap_text.split('\n')[start:end]
    criteria = []
    
    for line in lines:
        # Match bullet points with checkbox
        if re.match(r"\s*- \[[x~\s]\]", line):
            # Extract text after checkbox
            text = re.sub(r"\s*- \[[x~\s]\]\s*", "", line)
            criteria.append(text.strip())
    
    return criteria


def count_focused_tests(module_name: str, phase_num: int = None) -> int:
    """Count focused test files for a module."""
    test_dir = TESTS_DIR / module_name
    if not test_dir.exists():
        return 0
    
    count = 0
    pattern = f"test_*_focused.cpp"
    if phase_num:
        pattern = f"test_*_phase{phase_num}*_focused.cpp"
    
    for test_file in test_dir.glob(pattern):
        if test_file.is_file():
            count += 1
    
    return count


def count_benchmark_files(module_name: str, phase_num: int = None) -> int:
    """Count benchmark files for a module."""
    bench_dir = BENCHMARKS_DIR / module_name
    if not bench_dir.exists():
        return 0
    
    count = 0
    pattern = f"bench_*_gates.cpp"
    if phase_num:
        pattern = f"bench_*_phase{phase_num}*_gates.cpp"
    
    for bench_file in bench_dir.glob(pattern):
        if bench_file.is_file():
            count += 1
    
    return count


def count_source_code_lines(module_name: str) -> Tuple[int, int]:
    """Count total vs non-stub LOC in module source."""
    src_dir = SRC_DIR / module_name
    if not src_dir.exists():
        return 0, 0
    
    total_lines = 0
    non_stub_lines = 0
    
    for cpp_file in src_dir.glob("*.cpp"):
        with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                total_lines += 1
                # Exclude stub markers
                if "// STUB:" not in line and "STUB/SIMULATION NOTE" not in line:
                    non_stub_lines += 1
    
    return total_lines, non_stub_lines


def verify_acceptance_criteria(module_name: str, phase_num: int) -> Tuple[bool, str]:
    """Verify all acceptance criteria have evidence."""
    roadmap_path = find_module_roadmap(module_name)
    if not roadmap_path:
        return False, f"No ROADMAP.md found for module {module_name}"
    
    with open(roadmap_path, 'r') as f:
        roadmap_text = f.read()
    
    criteria = extract_acceptance_criteria(roadmap_text, phase_num)
    if not criteria:
        return False, f"No acceptance criteria found for Phase {phase_num}"
    
    missing_evidence = []
    for criterion in criteria:
        # Check if criterion contains evidence marker (test count, commit hash, file reference, etc.)
        has_evidence = any([
            re.search(r"test.*focused", criterion, re.IGNORECASE),
            re.search(r"bench.*gate", criterion, re.IGNORECASE),
            re.search(r"#\d+|commit", criterion),  # PR or commit reference
            re.search(r"\.(md|h|cpp)", criterion),  # File reference
            re.search(r"\d+\s*test", criterion),  # Test count
            re.search(r"\d+%|coverage", criterion),  # Coverage
        ])
        
        if not has_evidence:
            missing_evidence.append(criterion)
    
    if missing_evidence:
        msg = f"Missing evidence for {len(missing_evidence)} criteria:\n"
        for crit in missing_evidence:
            msg += f"  - {crit}\n"
        return False, msg
    
    return True, "All acceptance criteria have evidence"


def check_test_ratio(module_name: str, phase_num: int) -> Tuple[bool, Dict]:
    """Verify test count >= 80% of acceptance criteria."""
    roadmap_path = find_module_roadmap(module_name)
    if not roadmap_path:
        return False, {"error": f"No ROADMAP.md found for module {module_name}"}
    
    with open(roadmap_path, 'r') as f:
        roadmap_text = f.read()
    
    criteria = extract_acceptance_criteria(roadmap_text, phase_num)
    test_count = count_focused_tests(module_name, phase_num)
    
    if not criteria:
        return False, {"error": f"No acceptance criteria found for Phase {phase_num}"}
    
    criteria_count = len(criteria)
    ratio = test_count / criteria_count if criteria_count > 0 else 0.0
    threshold = 0.80
    
    passed = ratio >= threshold
    
    return passed, {
        "module": module_name,
        "phase": phase_num,
        "criteria_count": criteria_count,
        "test_count": test_count,
        "ratio": ratio,
        "threshold": threshold,
        "passed": passed,
        "gap": max(0, int(criteria_count * threshold) - test_count),
    }


def detect_phase_regressions(module_name: str, baseline_log: Path = None) -> List[Dict]:
    """Detect if a module has regressed (tests dropped, LOC dropped, etc.)."""
    regressions = []
    
    # Check test count regression
    current_tests = count_focused_tests(module_name)
    if baseline_log and baseline_log.exists():
        with open(baseline_log) as f:
            baseline = json.load(f)
            baseline_tests = baseline.get("test_count", 0)
            if current_tests < baseline_tests * 0.90:  # > 10% drop
                regressions.append({
                    "type": "test_count_drop",
                    "baseline": baseline_tests,
                    "current": current_tests,
                    "percent_drop": (1 - current_tests / baseline_tests) * 100,
                })
    
    # Check LOC regression
    total_loc, non_stub_loc = count_source_code_lines(module_name)
    if baseline_log and baseline_log.exists():
        with open(baseline_log) as f:
            baseline = json.load(f)
            baseline_loc = baseline.get("non_stub_loc", 0)
            if non_stub_loc < baseline_loc * 0.85:  # > 15% drop
                regressions.append({
                    "type": "loc_drop",
                    "baseline": baseline_loc,
                    "current": non_stub_loc,
                    "percent_drop": (1 - non_stub_loc / baseline_loc) * 100,
                })
    
    return regressions


def generate_phase_report(module_name: str) -> Dict:
    """Generate complete phase status report for a module."""
    roadmap_path = find_module_roadmap(module_name)
    if not roadmap_path:
        return {"error": f"No ROADMAP.md found for module {module_name}"}
    
    with open(roadmap_path) as f:
        roadmap_text = f.read()
    
    report = {
        "module": module_name,
        "phases": [],
    }
    
    # Check phases 1-6
    for phase_num in range(1, 7):
        status = get_phase_status(roadmap_text, phase_num)
        if status is None:
            continue
        
        test_count = count_focused_tests(module_name, phase_num)
        bench_count = count_benchmark_files(module_name, phase_num)
        criteria = extract_acceptance_criteria(roadmap_text, phase_num)
        
        phase_info = {
            "phase_number": phase_num,
            "status": status.value,
            "acceptance_criteria_count": len(criteria),
            "test_count": test_count,
            "benchmark_count": bench_count,
            "criteria": criteria[:3],  # First 3 for report
        }
        
        # Check ratio
        ratio = test_count / len(criteria) if criteria else 0
        phase_info["test_ratio"] = ratio
        phase_info["ratio_passed"] = ratio >= 0.80
        
        report["phases"].append(phase_info)
    
    return report


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Module Phase Verifier")
    parser.add_argument("--module", help="Module name to verify")
    parser.add_argument("--phase", type=int, help="Phase number")
    parser.add_argument("--verify-acceptance", action="store_true", help="Verify acceptance criteria")
    parser.add_argument("--check-test-ratio", action="store_true", help="Check test ratio >= 80%")
    parser.add_argument("--all-modules", action="store_true", help="Generate report for all modules")
    parser.add_argument("--generate-report", action="store_true", help="Generate phase report")
    parser.add_argument("--output-json", help="Output JSON file")
    
    args = parser.parse_args()
    
    if args.verify_acceptance and args.module and args.phase:
        passed, msg = verify_acceptance_criteria(args.module, args.phase)
        print(msg)
        sys.exit(0 if passed else 1)
    
    elif args.check_test_ratio and args.module and args.phase:
        passed, result = check_test_ratio(args.module, args.phase)
        print(json.dumps(result, indent=2))
        sys.exit(0 if passed else 1)
    
    elif args.generate_report and args.module:
        report = generate_phase_report(args.module)
        print(json.dumps(report, indent=2))
        if args.output_json:
            with open(args.output_json, 'w') as f:
                json.dump(report, f, indent=2)
    
    elif args.all_modules and args.generate_report:
        reports = {}
        for module_dir in sorted(SRC_DIR.iterdir()):
            if module_dir.is_dir() and (module_dir / "ROADMAP.md").exists():
                reports[module_dir.name] = generate_phase_report(module_dir.name)
        print(json.dumps(reports, indent=2))
        if args.output_json:
            with open(args.output_json, 'w') as f:
                json.dump(reports, f, indent=2)
    
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
