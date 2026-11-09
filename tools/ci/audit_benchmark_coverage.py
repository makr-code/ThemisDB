#!/usr/bin/env python3
"""
Enhanced Benchmark Coverage Audit
Validates three-state coverage: source -> CMake -> CTest
Produces detailed JSON reports and actionable findings.
"""

from __future__ import annotations

import argparse
import json
import platform
import re
import subprocess
import sys
from dataclasses import dataclass, asdict, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Set, List


REPO_ROOT = Path(__file__).resolve().parents[2]
BENCHMARKS_DIR = REPO_ROOT / "benchmarks"
MODULE_BENCHMARK_PRIORITY = {
    "server", "distributed_tensor", "prompt_engineering", "timeseries",
    "scheduler", "whisper", "llama_cpp", "analytics", "gpu", "llm",
    "query", "storage", "transaction", "sharding", "geo", "process",
    "performance",
}
MODULE_BENCHMARK_OWNER_ISSUES = {
    "analytics": 7500,
    "gpu": 7492,
    "performance": 7534,
    "prompt_engineering": 7538,
    "query": 7488,
    "scheduler": 7541,
    "sharding": 7810,
    "timeseries": 7549,
    "transaction": 7810,
    "whisper": 7558,
}
BENCHMARK_CI_WORKFLOWS = [
    {
        "path": ".github/workflows/benchmark-performance-gate.yml",
        "purpose": "Weekly benchmark build/run and comparison; schedule and manual dispatch",
        "trigger": "schedule: Monday 02:00 UTC; workflow_dispatch",
        "build_profile": "benchmark-quality preset; THEMIS_BUILD_BENCHMARKS=ON; THEMIS_BUILD_TESTS=OFF",
        "artifact_retention_days": 14,
    },
    {
        "path": ".github/workflows/gate-pr-benchmark-ctest.yml",
        "purpose": "PR benchmark source/CTest registration audit; does not execute benchmark workloads",
        "trigger": "pull_request on benchmark source/CMake paths; workflow_dispatch",
        "branches": ["develop", "community", "enterprise", "hyperscaler", "military", "minimal"],
        "pull_request_types": ["opened", "synchronize", "reopened", "ready_for_review"],
        "build_profile": None,
        "artifact_retention_days": None,
    },
]
RELEASE_CLAIM_EVIDENCE = [
    {
        "id": "query-limit-scale",
        "module": "query",
        "claim": "LIMIT 10 at 100,000 and 10,000,000 rows must bound latency and peak memory.",
        "source": "audit/FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-01",
        "benchmark_source": "benchmarks/query/bench_query_limit_scaling.cpp",
        "owner_issue": 7488,
        "issue_assignees": ["makr-code", "Copilot"],
        "target": "v2.5.0 M1 by 2027-03-31",
        "status": "gap",
        "evidence_url": "https://github.com/makr-code/ThemisDB/pull/7804",
        "evidence_sha": "f2f5a84fac9745407ac5b4ed172282ad8fc1a190",
        "reported_measurements": [
            {"input_rows": 100000, "latency_ns": 10078, "maximum_rss_kb": 4736},
            {"input_rows": 10000000, "latency_ns": 10438, "maximum_rss_kb": 4912},
        ],
        "measurement_scope": "Synthetic pull-source only; not a storage scan or retained raw result.",
        "required_workload": {
            "input_rows": [100000, 10000000],
            "input_distribution": None,
            "warmup_and_repetitions": None,
            "hardware_runtime_build": None,
            "correctness_checks": "Verify LIMIT 10 returns the expected ten rows.",
            "metrics": ["latency", "peak memory"],
        },
        "evidence_note": (
            "The merged PR reports synthetic pull-source measurements, not a storage scan, "
            "and does not attach a retained raw result artifact."
        ),
        "follow_up": (
            "Run the 100K/10M workload against the production scan path; retain raw JSON, "
            "latency, peak-memory, configuration, and correctness evidence."
        ),
    },
    {
        "id": "performance-regression-gates",
        "module": "performance",
        "claim": "Performance instrumentation and regression thresholds are reproducible and noise-aware.",
        "source": "docs/performance/PERFORMANCE_EXPECTATIONS.md",
        "benchmark_source": "benchmarks/performance/bench_performance_release_gates.cpp",
        "owner_issue": 7534,
        "issue_assignees": [],
        "target": "v2.5.0 M1 by 2027-03-31",
        "status": "gap",
        "required_workload": {
            "input_scale_and_distribution": None,
            "warmup_and_repetitions": None,
            "hardware_runtime_build": None,
            "correctness_checks": None,
            "metrics": ["throughput", "p50/p95/p99 latency", "resource use as applicable"],
        },
        "evidence_url": None,
        "evidence_sha": None,
        "evidence_note": "The owner issue remains open; no current run/result evidence was verified.",
        "follow_up": (
            "Close the issue's methodology and baseline tasks, including an explicit noise "
            "policy and maintainer-approved thresholds."
        ),
    },
    {
        "id": "wave-a-representative-hardware",
        "module": "transaction",
        "claim": "Wave A release-critical hardware baselines include reproducible workload and p95/p99 evidence.",
        "source": "audit/evidence/waves/manifests/",
        "benchmark_source": "benchmarks/wave8/",
        "owner_issue": 7810,
        "issue_assignees": ["makr-code", "Copilot"],
        "target": "v2.5.0 M0 / Wave A exit criteria",
        "status": "blocked",
        "modules": ["transaction", "gpu", "voice", "sharding", "replication"],
        "required_workload": {
            "input_scale_and_distribution": None,
            "warmup_and_repetitions": None,
            "hardware_runtime_build": None,
            "correctness_checks": None,
            "metrics": ["throughput", "p95/p99 latency", "CPU fallback behavior as applicable"],
        },
        "evidence_url": "https://github.com/makr-code/ThemisDB/pull/8012",
        "evidence_sha": "ee94d5c6028d5ddc80288cf5d91d6269d660408e",
        "evidence_note": (
            "The merged refresh explicitly leaves transaction, GPU, voice, sharding, and "
            "replication measurements pending; its latest baseline job was skipped and the "
            "referenced artifact is absent."
        ),
        "follow_up": (
            "Run the existing Wave A lanes on representative hardware and attach run URL, "
            "source SHA, configuration, raw result, and summary before promotion."
        ),
    },
    {
        "id": "gpu-capability-fallback",
        "module": "gpu",
        "claim": "GPU capability and fallback behavior is measured on supported hardware and distinguishes CPU degradation.",
        "source": "src/gpu/ROADMAP.md",
        "benchmark_source": "benchmarks/gpu/",
        "owner_issue": 7492,
        "issue_assignees": [],
        "target": "v2.5.0 M1 by 2027-03-31",
        "status": "blocked",
        "required_workload": {
            "input_scale_and_distribution": None,
            "warmup_and_repetitions": None,
            "hardware_runtime_build": None,
            "correctness_checks": "Exercise GPU success, unavailable-device fallback, and forced failure.",
            "metrics": ["latency", "resource use", "CPU fallback/degraded mode"],
        },
        "evidence_url": "https://github.com/makr-code/ThemisDB/issues/7492",
        "evidence_sha": None,
        "evidence_note": "The GPU-specific capability/fallback evidence issue remains open; hardware evidence is not available.",
        "follow_up": (
            "Capture CPU-only and GPU-capable lane results with hardware metadata, fallback mode, "
            "and valid benchmark exit status."
        ),
    },
    {
        "id": "llm-1024-concurrency-claim",
        "module": "llm",
        "claim": "“1,024 concurrent requests on a single GPU”",
        "source": "src/llm/THREADING.md",
        "benchmark_source": None,
        "owner_issue": 7490,
        "issue_assignees": ["makr-code", "Copilot"],
        "target": "v2.5.0 preview hardening; withdrawn claim",
        "status": "not applicable",
        "claim_disposition": "withdrawn",
        "benchmark_applicability": "not applicable",
        "evidence_url": "https://github.com/makr-code/ThemisDB/pull/7805",
        "evidence_sha": "59aed9c5f7011f54c825bb78587a3f5dad6d30d5",
        "evidence_note": (
            "The merged PR removes the unsupported concurrency claim; no hardware result is "
            "claimed, so a benchmark is not applicable to the withdrawn statement."
        ),
        "follow_up": None,
    },
]

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


def normalize_cpp_token(
    token: str,
    cmake_dir: Path,
    repo_root: Path = REPO_ROOT,
) -> str | None:
    """Normalize a C++ file path to benchmarks-relative form."""
    benchmarks_dir = repo_root / "benchmarks"
    token = token.strip().strip('"').strip("'")
    if not token.endswith(".cpp"):
        return None

    token = token.replace("${CMAKE_CURRENT_SOURCE_DIR}", str(cmake_dir))
    token = token.replace("${CMAKE_SOURCE_DIR}", str(repo_root))

    resolved = Path(token)
    if not resolved.is_absolute():
        resolved = (cmake_dir / resolved).resolve()
    else:
        resolved = resolved.resolve()

    try:
        return str(resolved.relative_to(benchmarks_dir.resolve())).replace("\\", "/")
    except ValueError:
        return None


def collect_benchmark_sources(repo_root: Path = REPO_ROOT) -> Dict[str, BenchmarkSource]:
    """Discover all benchmark .cpp files."""
    benchmarks_dir = repo_root / "benchmarks"
    sources: Dict[str, BenchmarkSource] = {}
    
    for path in benchmarks_dir.rglob("*.cpp"):
        if is_benchmark_source(path):
            rel_path = str(path.relative_to(benchmarks_dir)).replace("\\", "/")
            sources[rel_path] = BenchmarkSource(
                path=rel_path,
                is_excluded=rel_path in INTENTIONAL_EXCLUSIONS
            )
    
    return sources


def collect_cmake_targets(repo_root: Path = REPO_ROOT) -> Dict[str, BenchmarkTarget]:
    """Discover all benchmark CMake targets and their source mappings."""
    benchmarks_dir = repo_root / "benchmarks"
    targets: Dict[str, BenchmarkTarget] = {}
    
    for cmake_file in benchmarks_dir.rglob("CMakeLists.txt"):
        cmake_dir = cmake_file.parent.resolve()
        text = cmake_file.read_text(encoding="utf-8", errors="ignore")
        
        # Pattern 1: themis_add_standard_benchmark(target source.cpp ...)
        for match in re.finditer(
            r"themis_add_standard_benchmark\(\s*([^\s\)]+)\s+([^\s\)]+\.cpp)\s*[^\)]*\)",
            text
        ):
            target_name = match.group(1).strip()
            source_token = match.group(2).strip()
            source_path = normalize_cpp_token(source_token, cmake_dir, repo_root)
            
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
            source_path = normalize_cpp_token(source_token, cmake_dir, repo_root)
            
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
            source_path = normalize_cpp_token(source_token, cmake_dir, repo_root)
            
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
                source_path = normalize_cpp_token(token, cmake_dir, repo_root)
                if source_path and is_benchmark_source(Path(source_path)):
                    if target_name not in targets:
                        targets[target_name] = BenchmarkTarget(name=target_name)
                    targets[target_name].source_file = source_path
                    break
        
        # Pattern 5: themis_register_benchmark_ctest(NAME ... TARGET ...)
        for match in re.finditer(
            r"themis_register_benchmark_ctest\(\s*NAME\s+([^\s]+)\s+TARGET\s+([^\s\)]+)(.*?)\)",
            text,
            re.S,
        ):
            ctest_name = match.group(1).strip()
            target_name = match.group(2).strip()
            ctest_options = match.group(3)
            
            ctest_name = ctest_name.strip('"').strip("'")
            target_name = target_name.strip('"').strip("'")

            if target_name not in targets:
                targets[target_name] = BenchmarkTarget(name=target_name)
            targets[target_name].ctest_entries.append(ctest_name)
            if "release_critical" in ctest_options:
                targets[target_name].labels.append("release_critical")
    
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


def audit_coverage(repo_root: Path = REPO_ROOT) -> tuple[List[AuditFinding], Dict[str, any]]:
    """Run the three-state audit and generate findings."""
    
    sources = collect_benchmark_sources(repo_root)
    cmake_targets = collect_cmake_targets(repo_root)
    
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
            severity="warning",
            code="ORPHANED_CTEST",
            message=f"{len(orphaned_ctest)} CTest entry(ies) reference benchmark target(s) without source mapping",
            affected_items=orphaned_ctest,
            remediation="Either register the benchmark source in CMake or remove the orphaned CTest entry"
        )
        findings.append(finding)
        stats["warnings"] += 1
    
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
            source_disk_path = repo_root / "benchmarks" / target.source_file
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


def generate_target_mapping(repo_root: Path = REPO_ROOT) -> Dict[str, Dict]:
    """Generate benchmark_target_mapping.json (like test_target_mapping.json)."""
    cmake_targets = collect_cmake_targets(repo_root)
    sources = collect_benchmark_sources(repo_root)
    
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


def _status_for_benchmark(
    targets: List[BenchmarkTarget],
    latest_attempt: Dict | None,
    workload: Dict,
) -> str:
    if not targets or any(not target.source_file for target in targets):
        return "gap"
    if not any(target.is_registered_in_ctest() for target in targets):
        return "gap"
    if not latest_attempt or latest_attempt.get("run_conclusion") != "success":
        return "blocked"
    if not latest_attempt.get("benchmark_results_available", False):
        return "blocked"
    if latest_attempt.get("run_sha") is None or latest_attempt.get("run_url") is None:
        return "blocked"
    if any(value is None for value in workload.values()):
        return "gap"
    return "covered"


def _latest_source_revision(repo_root: Path) -> tuple[str | None, str | None]:
    try:
        revision = subprocess.run(
            ["git", "-C", str(repo_root), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        branch = subprocess.run(
            ["git", "-C", str(repo_root), "branch", "--show-current"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        return revision, branch or None
    except (OSError, subprocess.CalledProcessError):
        return None, None


def _performance_claim_inventory(repo_root: Path, module: str, benchmark_rows: List[Dict]) -> Dict:
    """Link module performance documents and declared target mappings without implying evidence."""
    source_documents = [
        f"src/{module}/{name}"
        for name in (
            "PERFORMANCE_EXPECTATIONS.md",
            "PERFORMANCE_BASELINE.md",
            "ROADMAP.md",
            "FUTURE_ENHANCEMENTS.md",
        )
        if (repo_root / "src" / module / name).is_file()
    ]
    target_mapping_path = repo_root / "benchmarks" / "benchmark_target_mapping.json"
    mapped_targets = []
    if target_mapping_path.is_file():
        try:
            target_mapping = json.loads(target_mapping_path.read_text(encoding="utf-8"))
            module_targets = target_mapping.get("modules", {}).get(module, {})
            for target_id, target in sorted(module_targets.items()):
                mapped_targets.append({
                    "target_id": target_id,
                    "label": target.get("label"),
                    "benchmark_file": target.get("file"),
                    "primary_case": target.get("primary_case")
                    or target.get("primary_benchmark"),
                    "fallback_file": target.get("fallback_file"),
                    "fallback_case": target.get("fallback_case"),
                    "declared_target": target.get("slo_target") or target.get("slo"),
                    "mapping_status": target.get("status"),
                })
        except (OSError, ValueError, AttributeError):
            mapped_targets = []

    owner_issue = MODULE_BENCHMARK_OWNER_ISSUES.get(module, 7828)
    owner_scope = (
        "module workload/evidence follow-up"
        if owner_issue != 7828
        else "portfolio claim-triage owner; no workload-specific benchmark issue was verified"
    )
    return {
        "status": "gap",
        "owner_issue": owner_issue,
        "owner_issue_scope": owner_scope,
        "target": "v2.5.0 M1 by 2027-03-31",
        "source_documents": source_documents,
        "target_mapping_source": (
            "benchmarks/benchmark_target_mapping.json"
            if target_mapping_path.is_file()
            else None
        ),
        "mapping_completeness": (
            "Declared target mappings are a partial index, not proof that every roadmap or "
            "documented claim is inventoried."
        ),
        "mapped_performance_targets": mapped_targets,
        "benchmark_source_candidates": [
            benchmark["source"] for benchmark in benchmark_rows
        ],
        "follow_up": (
            "Reconcile every documented and roadmap performance claim with one primary "
            "workload; record scale/distribution, warmup/repetitions, hardware/runtime/build, "
            "correctness checks, metrics, CI trigger, retained raw result, run URL, and source "
            "SHA. Keep this gap open until each claim has a named workload owner and reproducible "
            "evidence or a reviewed withdrawal/non-applicability rationale."
        ),
    }


def generate_module_matrix(
    repo_root: Path = REPO_ROOT,
    source_revision: str | None = None,
    source_branch: str | None = None,
    latest_attempt: Dict | None = None,
) -> Dict:
    """Build the evidence-oriented benchmark matrix for every source module."""
    modules_dir = repo_root / "src"
    modules = sorted(path.name for path in modules_dir.iterdir() if path.is_dir())
    sources = collect_benchmark_sources(repo_root)
    targets = collect_cmake_targets(repo_root)
    findings, registration_stats, _ = audit_coverage(repo_root)
    source_targets: Dict[str, List[BenchmarkTarget]] = {}
    for target in targets.values():
        if target.source_file:
            source_targets.setdefault(target.source_file, []).append(target)

    assigned: Dict[str, List[str]] = {module: [] for module in modules}
    unassigned: List[str] = []
    for source in sorted(sources):
        module = source.split("/", 1)[0]
        if module in assigned:
            assigned[module].append(source)
        else:
            unassigned.append(source)

    known_claims: Dict[str, List[Dict]] = {}
    for claim in RELEASE_CLAIM_EVIDENCE:
        for module in claim.get("modules", [claim["module"]]):
            known_claims.setdefault(module, []).append(claim)

    module_rows = []
    for module in modules:
        benchmark_rows = []
        for source in assigned[module]:
            source_targets_for_file = source_targets.get(source, [])
            ctest_entries = sorted({
                entry
                for target in source_targets_for_file
                for entry in target.ctest_entries
            })
            workload = {
                "input_scale_and_distribution": None,
                "warmup_and_repetition_policy": None,
                "hardware_runtime_configuration": None,
                "correctness_checks": None,
                "metrics": None,
            }
            status = _status_for_benchmark(
                source_targets_for_file, latest_attempt, workload
            )
            benchmark_rows.append({
                "source": f"benchmarks/{source}",
                "targets": sorted(target.name for target in source_targets_for_file),
                "ctest_entries": ctest_entries,
                "release_critical": any(
                    "release_critical" in target.labels
                    for target in source_targets_for_file
                ),
                "target_declared": bool(source_targets_for_file),
                "ctest_registered": bool(ctest_entries),
                "build_profile": (
                    "benchmark-quality preset; per-target eligibility not verified"
                ),
                "ci_workflows": [workflow["path"] for workflow in BENCHMARK_CI_WORKFLOWS],
                "workload": workload,
                "output_artifacts": [
                    f"${{CMAKE_BINARY_DIR}}/bench_results/current/{entry}.json"
                    for entry in ctest_entries
                ],
                "latest_evidence_ref": "latest_ci_attempt",
                "status": status,
            })

        statuses = [benchmark["status"] for benchmark in benchmark_rows]
        if not benchmark_rows:
            status = "gap"
        elif "gap" in statuses:
            status = "gap"
        elif "blocked" in statuses:
            status = "blocked"
        else:
            status = "covered"

        claims = known_claims.get(module, [])
        module_release_critical = any(
            "release_critical" in target.labels
            for source in assigned[module]
            for target in source_targets.get(source, [])
        )
        high_risk = module in MODULE_BENCHMARK_PRIORITY or module_release_critical
        if high_risk and not claims:
            claim_review = {
                "status": "gap",
                "owner_issue": 7828,
                "triage_owner": ["makr-code", "Copilot"],
                "target": "v2.5.0 M1 by 2027-03-31",
                "follow_up": (
                    "Review this module's documented and roadmap performance claims; link each "
                    "claim to an evidence workload and a named module owner issue, or record "
                    "a source-backed withdrawal/non-applicability rationale."
                ),
            }
        elif claims:
            claim_statuses = {claim["status"] for claim in claims}
            claim_review = {
                "status": (
                    "gap" if any(claim["status"] == "gap" for claim in claims)
                    else "blocked" if any(claim["status"] == "blocked" for claim in claims)
                    else "not applicable" if claim_statuses == {"not applicable"}
                    else "covered"
                ),
                "claim_ids": [claim["id"] for claim in claims],
            }
        else:
            claim_review = {
                "status": "gap",
                "owner_issue": 7828,
                "triage_owner": ["makr-code", "Copilot"],
                "target": "v2.5.0 M1 by 2027-03-31",
                "follow_up": (
                    "Review applicability and documented performance claims; record a specific "
                    "rationale before marking this module not applicable."
                ),
            }

        module_rows.append({
            "module": module,
            "priority": "high-risk" if high_risk else "standard",
            "benchmark_source_count": len(benchmark_rows),
            "benchmarks": benchmark_rows,
            "claim_review": claim_review,
            "claim_inventory_review": (
                _performance_claim_inventory(repo_root, module, benchmark_rows)
                if high_risk else None
            ),
            "claim_ids": [claim["id"] for claim in claims],
            "status": status,
        })

    discovered_revision, branch = _latest_source_revision(repo_root)
    revision = source_revision or discovered_revision
    ctest_target_count = sum(
        1 for target in targets.values()
        if target.source_file and target.is_registered_in_ctest()
    )
    ctest_entry_count = sum(len(target.ctest_entries) for target in targets.values())
    target_count = sum(1 for target in targets.values() if target.source_file)
    return {
        "schema_version": "1.1.0",
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "source_snapshot": {
            "revision": revision,
            "branch": source_branch or branch,
            "modules_root": "src/",
            "benchmark_root": "benchmarks/",
        },
        "tooling": {
            "audit_tool": "tools/ci/audit_benchmark_coverage.py",
            "audit_tool_version": "3",
            "python_version": platform.python_version(),
            "git_version": (
                subprocess.run(
                    ["git", "--version"],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout.strip()
            ),
            "cmake_version": "not invoked by this static inventory",
            "source_selection": (
                "Recursive benchmarks/**/*.cpp whose basename starts with bench or "
                "benchmark_, equals llm_bench.cpp, or ends with _bench.cpp."
            ),
            "module_assignment": (
                "The first benchmark-relative path component must match an immediate "
                "directory under src/; all other benchmark sources are listed as unassigned."
            ),
            "intentional_exclusions": sorted(INTENTIONAL_EXCLUSIONS),
        },
        "summary": {
            "source_module_count": len(modules),
            "module_associated_benchmark_sources": sum(map(len, assigned.values())),
            "all_discovered_benchmark_sources": len(sources),
            "unassigned_benchmark_sources": len(unassigned),
            "cmake_targets_with_source": target_count,
            "ctest_registered_targets_with_source": ctest_target_count,
            "ctest_entry_count": ctest_entry_count,
            "module_status_counts": {
                status: sum(1 for row in module_rows if row["status"] == status)
                for status in ("covered", "gap", "not applicable", "blocked")
            },
        },
        "registration_audit": {
            "summary": registration_stats,
            "findings": [asdict(finding) for finding in findings],
        },
        "ci_workflows": BENCHMARK_CI_WORKFLOWS,
        "latest_ci_attempt": latest_attempt,
        "status_legend": {
            "covered": "A retained, successful benchmark result and required workload metadata are present.",
            "gap": "Source/target/CTest mapping or claim applicability/evidence is missing.",
            "not applicable": "A specific, source-backed non-applicability rationale has been reviewed.",
            "blocked": "Execution/evidence cannot be established because a required run or external condition is blocked.",
        },
        "release_claims": RELEASE_CLAIM_EVIDENCE,
        "unassigned_benchmark_sources": [
            f"benchmarks/{source}" for source in unassigned
        ],
        "modules": module_rows,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Enhanced benchmark coverage audit (3-state: source -> CMake -> CTest)"
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=REPO_ROOT,
        help="Repository snapshot to audit (defaults to the current checkout)",
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
        "--module-matrix",
        type=Path,
        default=None,
        help="Machine-readable 73-module benchmark evidence matrix output",
    )
    parser.add_argument(
        "--source-revision",
        default=None,
        help="Explicit revision for the audited source snapshot",
    )
    parser.add_argument(
        "--source-branch",
        default=None,
        help="Explicit branch name for the audited source snapshot",
    )
    parser.add_argument(
        "--latest-run-url",
        default=None,
        help="URL of the latest benchmark CI attempt (not necessarily benchmark evidence)",
    )
    parser.add_argument(
        "--latest-run-sha",
        default=None,
        help="Commit SHA for the latest benchmark CI attempt",
    )
    parser.add_argument(
        "--latest-run-result",
        choices=("success", "failure", "skipped"),
        default=None,
        help="Conclusion of the latest benchmark CI attempt",
    )
    parser.add_argument(
        "--latest-run-note",
        default=None,
        help="Short explanation of the latest benchmark CI attempt outcome",
    )
    parser.add_argument(
        "--latest-run-config-json",
        default=None,
        help="JSON object describing the latest benchmark CI runner/build configuration",
    )
    parser.add_argument(
        "--benchmark-results-available",
        action="store_true",
        help="Assert that the latest run retained machine-readable benchmark results",
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
    findings, stats, targets_by_module = audit_coverage(args.repo_root)
    
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
            "stats": stats,
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
        mapping = generate_target_mapping(args.repo_root)
        args.target_mapping.write_text(json.dumps(mapping, indent=2))
        print(f"Target mapping saved to: {args.target_mapping}")

    if args.module_matrix:
        latest_attempt = None
        if any((
            args.latest_run_url,
            args.latest_run_sha,
            args.latest_run_result,
            args.latest_run_note,
            args.latest_run_config_json,
            args.benchmark_results_available,
        )):
            latest_attempt = {
                "run_url": args.latest_run_url,
                "run_sha": args.latest_run_sha,
                "run_conclusion": args.latest_run_result,
                "note": args.latest_run_note,
                "configuration": (
                    json.loads(args.latest_run_config_json)
                    if args.latest_run_config_json else None
                ),
                "benchmark_results_available": args.benchmark_results_available,
            }
        matrix = generate_module_matrix(
            args.repo_root,
            source_revision=args.source_revision,
            source_branch=args.source_branch,
            latest_attempt=latest_attempt,
        )
        args.module_matrix.parent.mkdir(parents=True, exist_ok=True)
        args.module_matrix.write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
        print(f"Module benchmark matrix saved to: {args.module_matrix}")
    
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
