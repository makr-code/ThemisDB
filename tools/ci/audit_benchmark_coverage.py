#!/usr/bin/env python3
"""
Enhanced Benchmark Coverage Audit
Validates three-state coverage: source -> CMake -> CTest
Produces detailed JSON reports and actionable findings.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, asdict, field
from pathlib import Path
from typing import Dict, Set, List


REPO_ROOT = Path(__file__).resolve().parents[2]
BENCHMARKS_DIR = REPO_ROOT / "benchmarks"

# Intentionally excluded from default benchmark registration
INTENTIONAL_EXCLUSIONS = {
    "performance_optimizations/phase2/benchmark_phase2.cpp",
}

# CTest entry pattern in CMakeLists.txt (simplified — exact pattern varies)
CTEST_REGISTRATION_PATTERNS = [
    r"themis_register_benchmark_ctest\(\s*NAME\s+([^\s]+)\s+TARGET\s+([^\s]+)",
    r"add_test\(\s*NAME\s+([^\s]+)\s+COMMAND.*?bench",
]


@dataclass
class BenchmarkTarget:
    """Represents a benchmark CMake target."""
    name: str
    source_file: str | None = None
    ctest_entries: List[str] = field(default_factory=list)
    module: str | None = None
    timeout: int | None = None
    labels: List[str] = field(default_factory=list)
    
    def is_registered_in_ctest(self) -> bool:
        return len(self.ctest_entries) > 0


@dataclass
class BenchmarkSource:
    """Represents a benchmark .cpp source file."""
    path: str
    cmake_target: str | None = None
    exists: bool = True
    is_excluded: bool = False


@dataclass
class AuditFinding:
    """Individual audit finding."""
    severity: str  # "error", "warning", "info"
    code: str  # e.g., "UNREGISTERED_SOURCE", "ORPHANED_CTEST"
    message: str
    affected_items: List[str] = field(default_factory=list)
    remediation: str = ""


def is_benchmark_source(path: Path) -> bool:
    """Check if a file looks like a benchmark source."""
    name = path.name
    return (
        name.startswith("bench")
        or name.startswith("benchmark_")
        or name == "llm_bench.cpp"
        or name.endswith("_bench.cpp")
    )


def normalize_cpp_token(token: str, cmake_dir: Path) -> str | None:
    """Normalize a C++ file path to benchmarks-relative form."""
    token = token.strip().strip('"').strip("'")
    if not token.endswith(".cpp"):
        return None

    token = token.replace("${CMAKE_CURRENT_SOURCE_DIR}", str(cmake_dir))
    token = token.replace("${CMAKE_SOURCE_DIR}", str(REPO_ROOT))

    resolved = Path(token)
    if not resolved.is_absolute():
        resolved = (cmake_dir / resolved).resolve()
    else:
        resolved = resolved.resolve()

    try:
        return str(resolved.relative_to(BENCHMARKS_DIR)).replace("\\", "/")
    except ValueError:
        return None


def collect_benchmark_sources() -> Dict[str, BenchmarkSource]:
    """Discover all benchmark .cpp files."""
    sources: Dict[str, BenchmarkSource] = {}
    
    for path in BENCHMARKS_DIR.rglob("*.cpp"):
        if is_benchmark_source(path):
            rel_path = str(path.relative_to(BENCHMARKS_DIR)).replace("\\", "/")
            sources[rel_path] = BenchmarkSource(
                path=rel_path,
                is_excluded=rel_path in INTENTIONAL_EXCLUSIONS
            )
    
    return sources


def collect_cmake_targets() -> Dict[str, BenchmarkTarget]:
    """Discover all benchmark CMake targets and their source mappings."""
    targets: Dict[str, BenchmarkTarget] = {}
    
    for cmake_file in BENCHMARKS_DIR.rglob("CMakeLists.txt"):
        cmake_dir = cmake_file.parent.resolve()
        text = cmake_file.read_text(encoding="utf-8", errors="ignore")
        
        # Pattern 1: themis_add_standard_benchmark(target source.cpp ...)
        for match in re.finditer(
            r"themis_add_standard_benchmark\(\s*([^\s\)]+)\s+([^\s\)]+\.cpp)\s*[^\)]*\)",
            text
        ):
            target_name = match.group(1).strip()
            source_token = match.group(2).strip()
            source_path = normalize_cpp_token(source_token, cmake_dir)
            
            if target_name not in targets:
                targets[target_name] = BenchmarkTarget(name=target_name)
            targets[target_name].source_file = source_path
        
        # Pattern 2: wave5_add_benchmark(target source.cpp ...)
        for match in re.finditer(
            r"wave5_add_benchmark\(\s*([^\s\)]+)\s+([^\s\)]+\.cpp)\s*[^\)]*\)",
            text
        ):
            target_name = match.group(1).strip()
            source_token = match.group(2).strip()
            source_path = normalize_cpp_token(source_token, cmake_dir)
            
            if target_name not in targets:
                targets[target_name] = BenchmarkTarget(name=target_name)
            targets[target_name].source_file = source_path
        
        # Pattern 3: add_w7_benchmark(target source.cpp ...)
        for match in re.finditer(
            r"add_w7_benchmark\(\s*([^\s\)]+)\s+([^\s\)]+\.cpp)\s*[^\)]*\)",
            text
        ):
            target_name = match.group(1).strip()
            source_token = match.group(2).strip()
            source_path = normalize_cpp_token(source_token, cmake_dir)
            
            if target_name not in targets:
                targets[target_name] = BenchmarkTarget(name=target_name)
            targets[target_name].source_file = source_path
        
        # Pattern 4: add_executable(target source.cpp ...) for standalone benchmarks
        for match in re.finditer(
            r"add_executable\(\s*([^\s\)]+)\s*(.*?)\)",
            text,
            re.S
        ):
            target_name = match.group(1).strip()
            if not (target_name.startswith("bench") or "bench" in target_name.lower()):
                continue
            
            body = match.group(2)
            for token in re.findall(r"([A-Za-z0-9_./${}-]+\.cpp)", body):
                source_path = normalize_cpp_token(token, cmake_dir)
                if source_path and is_benchmark_source(Path(source_path)):
                    if target_name not in targets:
                        targets[target_name] = BenchmarkTarget(name=target_name)
                    targets[target_name].source_file = source_path
                    break
        
        # Pattern 5: themis_register_benchmark_ctest(NAME ... TARGET ...)
        for match in re.finditer(
            r"themis_register_benchmark_ctest\(\s*NAME\s+([^\s]+)\s+TARGET\s+([^\s\)]+)",
            text
        ):
            ctest_name = match.group(1).strip()
            target_name = match.group(2).strip()
            
            if target_name not in targets:
                targets[target_name] = BenchmarkTarget(name=target_name)
            targets[target_name].ctest_entries.append(ctest_name)
    
    return targets


def extract_module_from_path(path: str) -> str | None:
    """Extract module name from benchmark path."""
    parts = path.split("/")
    if len(parts) >= 1:
        # Check if first part is a known module directory
        potential_module = parts[0]
        if not potential_module.startswith("wave"):
            return potential_module
        elif len(parts) >= 2:
            return parts[1]
    return None


def audit_coverage() -> tuple[List[AuditFinding], Dict[str, any]]:
    """Run the three-state audit and generate findings."""
    
    sources = collect_benchmark_sources()
    cmake_targets = collect_cmake_targets()
    
    findings: List[AuditFinding] = []
    stats = {
        "total_sources_discovered": len(sources),
        "intentionally_excluded": sum(1 for s in sources.values() if s.is_excluded),
        "cmake_targets": len(cmake_targets),
        "ctest_registered": sum(1 for t in cmake_targets.values() if t.is_registered_in_ctest()),
        "errors": 0,
        "warnings": 0,
        "info": 0,
    }
    
    # ===== CHECK 1: Unregistered Sources (source not in CMake) =====
    unregistered_sources = []
    for rel_path, source in sources.items():
        if source.is_excluded:
            continue
        
        # Check if any target references this source
        found_in_cmake = any(
            target.source_file == rel_path
            for target in cmake_targets.values()
        )
        
        if not found_in_cmake:
            unregistered_sources.append(rel_path)
            source.cmake_target = None
    
    if unregistered_sources:
        finding = AuditFinding(
            severity="error",
            code="UNREGISTERED_SOURCE",
            message=f"{len(unregistered_sources)} benchmark source(s) not registered in CMake",
            affected_items=unregistered_sources,
            remediation="Add source to benchmarks/CMakeLists.txt or module subdirectory CMakeLists.txt using themis_add_standard_benchmark()"
        )
        findings.append(finding)
        stats["errors"] += 1
    
    # ===== CHECK 2: Orphaned CTest Entries (CTest references missing target) =====
    orphaned_ctest = []
    for target_name, target in cmake_targets.items():
        if target.source_file is None and target.ctest_entries:
            orphaned_ctest.extend(target.ctest_entries)
    
    if orphaned_ctest:
        finding = AuditFinding(
            severity="error",
            code="ORPHANED_CTEST",
            message=f"{len(orphaned_ctest)} CTest entry(ies) reference benchmark target(s) without source mapping",
            affected_items=orphaned_ctest,
            remediation="Either register the benchmark source in CMake or remove the orphaned CTest entry"
        )
        findings.append(finding)
        stats["errors"] += 1
    
    # ===== CHECK 3: CMake Targets Without CTest (target not exposed to CTest) =====
    not_ctest_registered = []
    for target_name, target in cmake_targets.items():
        if target.source_file and not target.is_registered_in_ctest():
            not_ctest_registered.append(target_name)
    
    if not_ctest_registered:
        finding = AuditFinding(
            severity="warning",
            code="MISSING_CTEST_REGISTRATION",
            message=f"{len(not_ctest_registered)} CMake benchmark target(s) not registered in CTest",
            affected_items=not_ctest_registered,
            remediation="Add themis_register_benchmark_ctest() call in benchmarks CMakeLists.txt for each target"
        )
        findings.append(finding)
        stats["warnings"] += 1
    
    # ===== CHECK 4: Broken Targets (source doesn't exist on disk) =====
    broken_targets = []
    for target_name, target in cmake_targets.items():
        if target.source_file:
            source_disk_path = BENCHMARKS_DIR / target.source_file
            if not source_disk_path.exists():
                broken_targets.append({
                    "target": target_name,
                    "source": target.source_file,
                    "exists": False
                })
    
    if broken_targets:
        finding = AuditFinding(
            severity="error",
            code="BROKEN_TARGET",
            message=f"{len(broken_targets)} CMake target(s) reference missing source file(s)",
            affected_items=[bt["target"] for bt in broken_targets],
            remediation="Update CMakeLists.txt to reference correct source path, or remove dead targets"
        )
        findings.append(finding)
        stats["errors"] += 1
    
    # ===== Module-Level Coverage Breakdown =====
    targets_by_module = {}
    for target_name, target in cmake_targets.items():
        if not target.source_file:
            continue
        module = extract_module_from_path(target.source_file) or "unknown"
        if module not in targets_by_module:
            targets_by_module[module] = {
                "total": 0,
                "with_source": 0,
                "ctest_registered": 0,
            }
        targets_by_module[module]["total"] += 1
        targets_by_module[module]["with_source"] += 1
        if target.is_registered_in_ctest():
            targets_by_module[module]["ctest_registered"] += 1
    
    # Add module data to findings for release gate
    findings_dict = {}
    for finding in findings:
        code = finding.code
        if code not in findings_dict:
            findings_dict[code] = finding
    
    # Store findings with additional module data
    findings_with_modules = findings
    
    # ===== Summary Statistics =====
    stats["cmake_registered_with_source"] = sum(
        1 for t in cmake_targets.values() if t.source_file is not None
    )
    stats["ctest_coverage_percent"] = (
        100 * stats["ctest_registered"] // stats["cmake_registered_with_source"]
        if stats["cmake_registered_with_source"] > 0
        else 0
    )
    stats["unregistered_sources"] = len(unregistered_sources)
    stats["orphaned_ctest"] = len(orphaned_ctest)
    stats["missing_ctest"] = len(not_ctest_registered)
    stats["broken_targets"] = len(broken_targets)
    
    return findings_with_modules, stats, targets_by_module


def generate_target_mapping() -> Dict[str, Dict]:
    """Generate benchmark_target_mapping.json (like test_target_mapping.json)."""
    cmake_targets = collect_cmake_targets()
    sources = collect_benchmark_sources()
    
    mapping = {}
    
    for target_name, target in cmake_targets.items():
        if not target.source_file:
            continue
        
        module = extract_module_from_path(target.source_file)
        
        mapping[target_name] = {
            "name": target_name,
            "source": target.source_file,
            "module": module or "unknown",
            "ctest_entries": target.ctest_entries,
            "is_registered_ctest": target.is_registered_in_ctest(),
            "timeout_seconds": target.timeout or 120,
            "labels": [f"module:{module or 'unknown'}", "tier:benchmark", "kind:smoke"]
        }
    
    return mapping


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Enhanced benchmark coverage audit (3-state: source -> CMake -> CTest)"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="JSON output file for findings and stats"
    )
    parser.add_argument(
        "--target-mapping",
        type=Path,
        default=None,
        help="JSON output file for benchmark_target_mapping.json"
    )
    parser.add_argument(
        "--fail-on-warning",
        action="store_true",
        help="Exit with code 1 if warnings present (default: exit 0 if only warnings)"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print detailed audit output"
    )
    
    args = parser.parse_args()
    
    # Run audit
    findings, stats, targets_by_module = audit_coverage()
    
    # Print summary
    print("=" * 80)
    print("BENCHMARK COVERAGE AUDIT REPORT")
    print("=" * 80)
    print(f"Sources discovered:       {stats['total_sources_discovered']}")
    print(f"Intentionally excluded:   {stats['intentionally_excluded']}")
    print(f"CMake targets:            {stats['cmake_targets']}")
    print(f"  - with source mapping:  {stats['cmake_registered_with_source']}")
    print(f"CTest registered:         {stats['ctest_registered']}")
    print(f"  - coverage:             {stats['ctest_coverage_percent']}%")
    print()
    print(f"Findings Summary:")
    print(f"  - Errors:               {stats['errors']}")
    print(f"  - Warnings:             {stats['warnings']}")
    print(f"  - Info:                 {stats['info']}")
    print()
    
    # Print findings
    for finding in findings:
        print(f"[{finding.severity.upper()}] {finding.code}")
        print(f"  Message:     {finding.message}")
        if args.verbose and finding.affected_items:
            print(f"  Affected items ({len(finding.affected_items)}):")
            for item in finding.affected_items[:10]:  # Limit output
                print(f"    - {item}")
            if len(finding.affected_items) > 10:
                print(f"    ... and {len(finding.affected_items) - 10} more")
        if finding.remediation:
            print(f"  Remediation: {finding.remediation}")
        print()
    
    # Save JSON output
    if args.output:
        output = {
            "timestamp": str(Path(__file__).stat().st_mtime),
            "summary": stats,
            "findings": {
                "unregistered_sources": [],
                "orphaned_ctest": [],
                "missing_ctest_registration": [],
                "broken_targets": [],
            },
            "targets_by_module": targets_by_module,
        }
        # Populate findings from audit results
        for finding in findings:
            if finding.code == "UNREGISTERED_SOURCE":
                output["findings"]["unregistered_sources"] = finding.affected_items
            elif finding.code == "ORPHANED_CTEST":
                output["findings"]["orphaned_ctest"] = finding.affected_items
            elif finding.code == "MISSING_CTEST_REGISTRATION":
                output["findings"]["missing_ctest_registration"] = finding.affected_items
            elif finding.code == "BROKEN_TARGET":
                output["findings"]["broken_targets"] = finding.affected_items
        
        args.output.write_text(json.dumps(output, indent=2))
        print(f"Audit report saved to: {args.output}")
    
    # Save target mapping
    if args.target_mapping:
        mapping = generate_target_mapping()
        args.target_mapping.write_text(json.dumps(mapping, indent=2))
        print(f"Target mapping saved to: {args.target_mapping}")
    
    # Determine exit code
    has_errors = stats["errors"] > 0
    has_warnings = stats["warnings"] > 0
    
    if has_errors:
        print("\n❌ AUDIT FAILED: Errors detected")
        return 1
    elif has_warnings and args.fail_on_warning:
        print("\n⚠️  AUDIT FAILED: Warnings detected (--fail-on-warning)")
        return 2
    else:
        print("\n✅ AUDIT PASSED")
        return 0


if __name__ == "__main__":
    sys.exit(main())
