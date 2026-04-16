"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            perf_expectations_rootcause_audit.py               ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:58:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   59.0/100                                       ║
    • Total Lines:     811                                            ║
    • Open Issues:     TODOs: 0, Stubs: 10                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6262db4796  2026-04-15  perf(olap): remove BM_OLAP_Disabled stub, activate 4 prod... ║
    • 68ce40a2f9  2026-04-13  feat: automated §1.5 root-cause audit for PERFORMANCE_EXP... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants / configuration
# ---------------------------------------------------------------------------

PERF_DOC_DEFAULT = "PERFORMANCE_EXPECTATIONS.md"
OUTPUT_DIR_DEFAULT = "artifacts/perf_expectations_rootcause_audit"

# ANSI colours (disabled by --no-color)
_COL = {
    "red": "\033[31m",
    "yellow": "\033[33m",
    "green": "\033[32m",
    "cyan": "\033[36m",
    "bold": "\033[1m",
    "reset": "\033[0m",
}


# ---------------------------------------------------------------------------
# KPI definitions
# ---------------------------------------------------------------------------
# Each entry describes one row in section 1.5.  The audit checks each item
# for evidence in code/artefacts.
#
# Fields:
#   kpi_id       Short unique identifier used in the report
#   label        Human-readable label (mirrors document column "Bereich")
#   hard_claims  List of strings that appear in the document and constitute
#                a *hard* claim whose staleness triggers exit-code 1.
#                A claim is "stale" when supporting evidence exists in the
#                repo that contradicts it.
#   benchmark_files   Relative paths (from repo-root) to the benchmark source
#                files that should register cases for this KPI.
#   required_benchmark_cases  Regex patterns that must match at least one
#                BENCHMARK/BENCHMARK_DEFINE_F/BENCHMARK_REGISTER_F line.
#   cmake_targets  add_executable target names that must be present in
#                any CMakeLists.txt under benchmarks/.
#   artefact_paths  Relative paths (from repo-root) of expected JSON artefacts.
#                Absence is reported as WARN (missing_artifact), not FAIL.
#   proxy_claim  True if the document calls this a Proxy measurement; the
#                script then checks whether a dedicated 1:1 case now exists.
#   blocked_claim  True if the document marks this as "blocked" or "n/v"
#                (platform-specific); a FAIL is raised when a real benchmark
#                case is found and registered.

KPI_DEFINITIONS: List[Dict] = [
    {
        "kpi_id": "QUERY_THROUGHPUT",
        "label": "Query Engine Throughput",
        "hard_claims": [],
        "benchmark_files": ["benchmarks/bench_query.cpp"],
        "required_benchmark_cases": [
            r"BENCHMARK\s*\(\s*BM_SimpleWhere\b",
            r"BENCHMARK\s*\(\s*BM_ComplexWhere\b",
            r"BENCHMARK\s*\(\s*BM_JoinUsersPosts\b",
        ],
        "cmake_targets": [],  # bench_query.cpp uses direct linking; see bench_query_lazy_eval
        "artefact_paths": [
            "artifacts/perf_nv/targeted_validation/bench_query_targeted.json",
        ],
        "proxy_claim": False,
        "blocked_claim": False,
    },
    {
        "kpi_id": "VECTOR_INSERT",
        "label": "Vector Insert",
        "hard_claims": [],
        "benchmark_files": ["benchmarks/bench_vector_search.cpp"],
        "required_benchmark_cases": [
            r"BENCHMARK\s*\(\s*BM_VectorInsert_Batch100\b",
        ],
        "cmake_targets": ["bench_vector_search"],
        "artefact_paths": [
            "artifacts/perf_nv/targeted_validation/bench_vector_search_targeted.json",
        ],
        "proxy_claim": False,
        "blocked_claim": False,
    },
    {
        "kpi_id": "SECONDARY_INDEX_INSERT",
        "label": "Secondary Index Insert",
        "hard_claims": [],
        "benchmark_files": [
            "benchmarks/bench_hotspots_micro.cpp",
            "benchmarks/bench_insert_profiling.cpp",
            "benchmarks/bench_crud.cpp",
        ],
        "required_benchmark_cases": [
            r"BENCHMARK(?:_REGISTER_F)?\s*\(\s*(?:BM_SecondaryIndex_Write\b|ProfiledInsertFixture,\s*IndexInsert_AllIndexes\b|CRUDFixture,\s*InsertWithAllIndexes\b)",
        ],
        "cmake_targets": [],
        "artefact_paths": [],
        "proxy_claim": False,
        "blocked_claim": False,
    },
    {
        "kpi_id": "STORAGE_SUSTAINED_WRITE",
        "label": "Storage Sustained Write",
        "hard_claims": [
            "Proxy",
            "kein 1:1 SLO-Case",
        ],
        # A "proxy" claim: stale if a dedicated 1:1 sustained-write benchmark
        # now exists (e.g. BM_SustainedWrite_NVMe or similar).
        "benchmark_files": ["benchmarks/bench_storage_performance.cpp"],
        "required_benchmark_cases": [],
        "cmake_targets": ["bench_storage_performance"],
        "artefact_paths": [],
        # proxy_claim = True: check for dedicated 1:1 NVMe sustained-write case
        "proxy_claim": True,
        "proxy_check_files": [
            "benchmarks/bench_storage_performance.cpp",
            "benchmarks/bench_hotspots_micro.cpp",
        ],
        "proxy_check_pattern": r"BENCHMARK\s*\(\s*BM_(?:SustainedWrite\b|StorageSustained\b|NVMe_Sustained\b|SustainedNVMe\b)",
        "blocked_claim": False,
    },
    {
        "kpi_id": "ANALYTICS_AN10",
        "label": "Analytics AN-10 (ARM NEON blocked)",
        "hard_claims": [
            "AN-10 weiterhin n/v",
            "plattformblockiert",
        ],
        # Blocked claim: A FAIL is raised if bench_arm_simd.cpp has BENCHMARK
        # registrations AND the cmake target exists (meaning AN-10 is measurable).
        "benchmark_files": ["benchmarks/bench_arm_simd.cpp"],
        "required_benchmark_cases": [],
        "cmake_targets": [],
        "artefact_paths": [
            "artifacts/perf_nv/targeted_validation/bench_olap_targeted.json",
        ],
        "proxy_claim": False,
        "blocked_claim": True,
        "blocked_check_files": ["benchmarks/bench_arm_simd.cpp"],
        "blocked_check_pattern": r"BENCHMARK\s*\(\s*BM_ARM_",
        "blocked_cmake_target": "bench_arm_simd",
    },
    {
        "kpi_id": "TPCC_YCSB",
        "label": "System-Level TPC-C / YCSB (Lite-Profile)",
        "hard_claims": [],
        "benchmark_files": [
            "benchmarks/bench_tpcc.cpp",
            "benchmarks/bench_ycsb.cpp",
        ],
        "required_benchmark_cases": [
            r"BENCHMARK(?:_REGISTER_F)?\s*\(\s*(?:TPCCFixture,\s*\w+|YCSBFixture,\s*Workload\w+)",
        ],
        "cmake_targets": [],
        "artefact_paths": [
            "artifacts/perf_nv/targeted_validation/bench_tpcc_targeted_v2.json",
            "artifacts/perf_nv/targeted_validation/bench_ycsb_targeted_v2.json",
        ],
        "proxy_claim": False,
        "blocked_claim": False,
    },
]

# Benchmarks that the CMakeLists.txt at benchmarks/CMakeLists.txt should
# register as add_executable targets (for KPIs that have cmake_targets set).
CMAKELISTS_PATHS = [
    "benchmarks/CMakeLists.txt",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _col(name: str, text: str, use_color: bool) -> str:
    if not use_color:
        return text
    return f"{_COL[name]}{text}{_COL['reset']}"


def _read_file(path: Path) -> Optional[str]:
    """Return file contents or None if the file does not exist."""
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


def _file_exists(repo_root: Path, rel_path: str) -> bool:
    return (repo_root / rel_path).is_file()


def _collect_benchmark_registrations(path: Path) -> List[str]:
    """Return every BENCHMARK / BENCHMARK_DEFINE_F / BENCHMARK_REGISTER_F line."""
    content = _read_file(path)
    if content is None:
        return []
    pattern = re.compile(
        r"^\s*(BENCHMARK(?:_DEFINE_F|_REGISTER_F)?)\s*\(", re.MULTILINE
    )
    lines = []
    for m in pattern.finditer(content):
        # Use group(1) start to skip any blank lines matched by ^\s*
        start = m.start(1)
        end = content.find("\n", start)
        lines.append(content[start: end if end != -1 else start + 200].strip())
    return lines


def _cmake_has_target(cmake_content: str, target_name: str) -> bool:
    """Return True if add_executable(target_name ...) appears in cmake_content."""
    pattern = re.compile(
        r"add_executable\s*\(\s*" + re.escape(target_name) + r"\b",
        re.MULTILINE,
    )
    return bool(pattern.search(cmake_content))


def _extract_section_15(content: str) -> str:
    """Extract the raw text of section 1.5 from the document."""
    # Match from "### 1.5" up to the next "###" section header
    m = re.search(
        r"(###\s+1\.5\b.*?)(?=\n###\s+1\.\d|\Z)", content, re.DOTALL
    )
    return m.group(1) if m else ""


def _load_cmake_contents(repo_root: Path) -> str:
    """Concatenate all CMakeLists.txt contents from the benchmarks directory."""
    parts: List[str] = []
    for rel in CMAKELISTS_PATHS:
        c = _read_file(repo_root / rel)
        if c:
            parts.append(c)
    # Also collect any sub-directory CMakeLists.txt under benchmarks/
    bench_dir = repo_root / "benchmarks"
    if bench_dir.is_dir():
        for p in bench_dir.rglob("CMakeLists.txt"):
            c = _read_file(p)
            if c:
                parts.append(c)
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Per-KPI check
# ---------------------------------------------------------------------------

class Finding:
    """A single audit finding for one KPI."""

    LEVELS = ("OK", "WARN", "FAIL")

    def __init__(
        self,
        kpi_id: str,
        label: str,
        level: str,
        code: str,
        message: str,
        evidence: Optional[str] = None,
    ):
        assert level in self.LEVELS
        self.kpi_id = kpi_id
        self.label = label
        self.level = level
        self.code = code
        self.message = message
        self.evidence = evidence

    def to_dict(self) -> Dict:
        d = {
            "kpi_id": self.kpi_id,
            "label": self.label,
            "level": self.level,
            "code": self.code,
            "message": self.message,
        }
        if self.evidence:
            d["evidence"] = self.evidence
        return d


def _check_kpi(
    kpi: Dict,
    repo_root: Path,
    cmake_content: str,
    section_15_text: str,
) -> List[Finding]:
    findings: List[Finding] = []
    kpi_id = kpi["kpi_id"]
    label = kpi["label"]

    # ------------------------------------------------------------------
    # 1. Check benchmark source files exist
    # ------------------------------------------------------------------
    for rel in kpi.get("benchmark_files", []):
        if not _file_exists(repo_root, rel):
            findings.append(Finding(
                kpi_id, label, "WARN", "missing_benchmark_file",
                f"Benchmark source file not found: {rel}",
            ))

    # ------------------------------------------------------------------
    # 2. Check BENCHMARK registrations in source files
    # ------------------------------------------------------------------
    all_reg_lines: List[str] = []
    for rel in kpi.get("benchmark_files", []):
        p = repo_root / rel
        all_reg_lines.extend(_collect_benchmark_registrations(p))

    reg_text = "\n".join(all_reg_lines)

    for pattern_str in kpi.get("required_benchmark_cases", []):
        pat = re.compile(pattern_str, re.IGNORECASE)
        if not pat.search(reg_text):
            findings.append(Finding(
                kpi_id, label, "WARN", "missing_benchmark_registration",
                f"No BENCHMARK registration matching pattern: {pattern_str}",
                evidence=f"Scanned files: {kpi.get('benchmark_files', [])}",
            ))
        else:
            findings.append(Finding(
                kpi_id, label, "OK", "benchmark_registration_found",
                f"BENCHMARK registration found for pattern: {pattern_str}",
            ))

    # ------------------------------------------------------------------
    # 3. Check CMake targets
    # ------------------------------------------------------------------
    for target in kpi.get("cmake_targets", []):
        if _cmake_has_target(cmake_content, target):
            findings.append(Finding(
                kpi_id, label, "OK", "cmake_target_found",
                f"CMake add_executable target found: {target}",
            ))
        else:
            findings.append(Finding(
                kpi_id, label, "WARN", "missing_cmake_target",
                f"CMake add_executable target not found: {target}",
                evidence="Checked benchmarks/CMakeLists.txt",
            ))

    # ------------------------------------------------------------------
    # 4. Check artefact paths
    # ------------------------------------------------------------------
    for rel in kpi.get("artefact_paths", []):
        if _file_exists(repo_root, rel):
            findings.append(Finding(
                kpi_id, label, "OK", "artefact_found",
                f"Artefact present: {rel}",
            ))
        else:
            findings.append(Finding(
                kpi_id, label, "WARN", "missing_artifact",
                f"Artefact not present in repository: {rel}",
                evidence="Run the benchmark and commit the JSON output, "
                         "or update the document if the path changed.",
            ))

    # ------------------------------------------------------------------
    # 5. Proxy-claim check
    # ------------------------------------------------------------------
    if kpi.get("proxy_claim"):
        # Stale if a dedicated 1:1 benchmark now exists in any of the
        # proxy_check_files.
        proxy_files = kpi.get("proxy_check_files", kpi.get("benchmark_files", []))
        proxy_pat = re.compile(kpi.get("proxy_check_pattern", ""), re.IGNORECASE)
        dedicated_found = False
        for rel in proxy_files:
            p = repo_root / rel
            reg_lines = _collect_benchmark_registrations(p)
            for line in reg_lines:
                if proxy_pat.search(line):
                    dedicated_found = True
                    findings.append(Finding(
                        kpi_id, label, "FAIL", "stale_proxy_claim",
                        f"Document claims only a Proxy measurement exists, but a "
                        f"dedicated 1:1 benchmark was found: '{line.strip()}'",
                        evidence=f"File: {rel}",
                    ))
                    break
            if dedicated_found:
                break
        if not dedicated_found:
            findings.append(Finding(
                kpi_id, label, "WARN", "proxy_no_dedicated_case",
                "Proxy claim still valid: no dedicated 1:1 SLO benchmark found. "
                "Document is consistent but a 1:1 benchmark is still needed.",
            ))

    # ------------------------------------------------------------------
    # 6. Blocked/n/v claim check
    # ------------------------------------------------------------------
    if kpi.get("blocked_claim"):
        # Collect benchmark registrations from the blocked_check_files
        blocked_files = kpi.get("blocked_check_files", kpi.get("benchmark_files", []))
        blocked_pat = re.compile(kpi.get("blocked_check_pattern", ""), re.IGNORECASE)
        any_registered = False
        for rel in blocked_files:
            p = repo_root / rel
            reg_lines = _collect_benchmark_registrations(p)
            for line in reg_lines:
                if blocked_pat.search(line):
                    any_registered = True
                    # Now check whether a CMake target also exists
                    target = kpi.get("blocked_cmake_target", "")
                    cmake_target_exists = bool(
                        target and _cmake_has_target(cmake_content, target)
                    )
                    if cmake_target_exists:
                        findings.append(Finding(
                            kpi_id, label, "FAIL", "stale_blocked_claim",
                            f"Document claims KPI is still blocked/n/v, but "
                            f"a benchmark is registered ('{line.strip()}') AND "
                            f"the CMake target '{target}' exists. "
                            f"The document must be updated.",
                            evidence=f"File: {rel}, target: {target}",
                        ))
                    else:
                        # Registered but no cmake target: measurable in source
                        # but not built → soft WARN
                        findings.append(Finding(
                            kpi_id, label, "WARN", "blocked_bench_registered_no_cmake",
                            f"Benchmark cases exist for blocked KPI "
                            f"('{line.strip()}') but no CMake target found "
                            f"('{target}'). KPI may become measurable once target "
                            f"is added to CMakeLists.txt.",
                            evidence=f"File: {rel}",
                        ))
                    break
            if any_registered:
                break

        if not any_registered:
            findings.append(Finding(
                kpi_id, label, "OK", "blocked_claim_consistent",
                "Blocked/n/v claim is consistent: no benchmark registration found "
                "for the blocked KPI.",
            ))

    return findings


# ---------------------------------------------------------------------------
# Meta-cause checks (section 1.5.1)
# ---------------------------------------------------------------------------

def _check_meta_causes(repo_root: Path, cmake_content: str) -> List[Finding]:
    """
    Cross-check the 1.5.1 meta-causes against repository evidence.

    Meta-cause 1: Proxy instead of primary cases.
        -> verified per-KPI above; nothing extra here.
    Meta-cause 2: Build/feature gates disable benchmarks.
        -> Check that bench_olap_analytics.cpp only has the disabled stub,
           confirming the document's statement about it.
    Meta-cause 3: Runtime deps (GPU/models/HSM) block benchmarks.
        -> Informational; we just confirm bench_arm_simd.cpp exists.
    Meta-cause 4: Historical targets from different infra.
        -> Informational; nothing to check automatically.
    """
    findings: List[Finding] = []

    # Meta-cause 2: bench_olap_analytics.cpp disabled-stub check
    olap_file = repo_root / "benchmarks/bench_olap_analytics.cpp"
    olap_content = _read_file(olap_file)
    if olap_content is not None:
        has_disabled_stub = bool(re.search(r"BM_OLAP_Disabled", olap_content))
        has_real_cases = bool(re.search(
            r"BENCHMARK\s*\(\s*BM_OLAP_(?!Disabled)\w+", olap_content
        ))
        if has_disabled_stub and not has_real_cases:
            findings.append(Finding(
                "META_CAUSE_2", "bench_olap_analytics disabled-stub (meta-cause 2)",
                "WARN", "olap_disabled_stub_only",
                "bench_olap_analytics.cpp only contains BM_OLAP_Disabled; "
                "document claim about disabled OLAP benchmarks is consistent. "
                "No real AN-*/OLAP cases are registered yet.",
            ))
        elif has_disabled_stub and has_real_cases:
            findings.append(Finding(
                "META_CAUSE_2", "bench_olap_analytics disabled-stub (meta-cause 2)",
                "FAIL", "olap_disabled_stub_stale",
                "bench_olap_analytics.cpp now has real BENCHMARK cases beyond "
                "BM_OLAP_Disabled. The document's disabled-stub claim is STALE "
                "and must be updated.",
                evidence="File: benchmarks/bench_olap_analytics.cpp",
            ))
        elif has_real_cases:
            findings.append(Finding(
                "META_CAUSE_2", "bench_olap_analytics disabled-stub (meta-cause 2)",
                "OK", "olap_stub_removed_real_cases_present",
                "bench_olap_analytics.cpp has real BENCHMARK cases and no "
                "BM_OLAP_Disabled stub – meta-cause 2 resolved (Issue #5).",
            ))
        else:
            findings.append(Finding(
                "META_CAUSE_2", "bench_olap_analytics disabled-stub (meta-cause 2)",
                "OK", "olap_no_disabled_stub",
                "bench_olap_analytics.cpp has no disabled stub pattern – may have "
                "been cleaned up; verify document section 1.5.1 meta-cause 2.",
            ))
    else:
        findings.append(Finding(
            "META_CAUSE_2", "bench_olap_analytics disabled-stub (meta-cause 2)",
            "WARN", "missing_benchmark_file",
            "benchmarks/bench_olap_analytics.cpp not found.",
        ))

    # Meta-cause 3: ARM / GPU blocked
    arm_file = repo_root / "benchmarks/bench_arm_simd.cpp"
    if _read_file(arm_file) is not None:
        findings.append(Finding(
            "META_CAUSE_3", "ARM SIMD benchmark exists (meta-cause 3)",
            "OK", "arm_bench_file_exists",
            "benchmarks/bench_arm_simd.cpp exists – ARM benchmarks are present "
            "in source. Buildability is gated on CMake target availability.",
        ))
    else:
        findings.append(Finding(
            "META_CAUSE_3", "ARM SIMD benchmark exists (meta-cause 3)",
            "WARN", "missing_benchmark_file",
            "benchmarks/bench_arm_simd.cpp not found.",
        ))

    return findings


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def _build_report(
    findings: List[Finding],
    repo_root: Path,
    section_15_text: str,
    generated_at: str,
) -> Dict:
    fail_count = sum(1 for f in findings if f.level == "FAIL")
    warn_count = sum(1 for f in findings if f.level == "WARN")
    ok_count = sum(1 for f in findings if f.level == "OK")

    overall = "PASS"
    if fail_count > 0:
        overall = "FAIL"
    elif warn_count > 0:
        overall = "WARN"

    return {
        "tool": "perf_expectations_rootcause_audit",
        "version": "1.0.0",
        "generated_at": generated_at,
        "repo_root": str(repo_root),
        "summary": {
            "overall": overall,
            "total": len(findings),
            "fail": fail_count,
            "warn": warn_count,
            "ok": ok_count,
        },
        "findings": [f.to_dict() for f in findings],
        "section_15_excerpt": section_15_text[:2000] + ("…" if len(section_15_text) > 2000 else ""),
    }


def _write_json_report(report: Dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    out = output_dir / "report.json"
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    return out


def _write_markdown_report(report: Dict, output_dir: Path) -> Path:
    summary = report["summary"]
    lines: List[str] = [
        "# Performance Expectations Root-Cause Audit",
        "",
        f"**Generated:** {report['generated_at']}  ",
        f"**Tool:** {report['tool']} v{report['version']}  ",
        f"**Overall:** {summary['overall']}  ",
        f"**Findings:** {summary['total']} total — "
        f"{summary['fail']} FAIL / {summary['warn']} WARN / {summary['ok']} OK",
        "",
        "## Findings",
        "",
        "| KPI | Level | Code | Message |",
        "|-----|-------|------|---------|",
    ]
    for f in report["findings"]:
        level_icon = {"FAIL": "❌", "WARN": "⚠️", "OK": "✅"}.get(f["level"], f["level"])
        msg = f["message"].replace("|", "&#124;").replace("\n", " ")
        lines.append(
            f"| {f['label']} | {level_icon} {f['level']} | `{f['code']}` | {msg} |"
        )

    lines += [
        "",
        "## Section 1.5 Excerpt",
        "",
        "```",
        report.get("section_15_excerpt", "(not extracted)"),
        "```",
        "",
        "---",
        "*Generated by `tools/perf_expectations_rootcause_audit.py`.*",
    ]

    output_dir.mkdir(parents=True, exist_ok=True)
    out = output_dir / "report.md"
    out.write_text("\n".join(lines), encoding="utf-8")
    return out


# ---------------------------------------------------------------------------
# Terminal output
# ---------------------------------------------------------------------------

def _print_findings(
    findings: List[Finding],
    report: Dict,
    use_color: bool,
    quiet: bool,
) -> None:
    summary = report["summary"]

    if not quiet:
        print()
        print(_col("bold", "=== Performance-Expectations Root-Cause Audit ===", use_color))
        print()
        for f in findings:
            if f.level == "FAIL":
                col = "red"
            elif f.level == "WARN":
                col = "yellow"
            else:
                col = "green"
            prefix = _col(col, f"[{f.level:4}]", use_color)
            print(f"  {prefix}  {f.kpi_id:<30s}  {f.code}")
            print(f"             {f.message}")
            if f.evidence:
                print(f"             Evidence: {f.evidence}")
        print()

    overall_col = "red" if summary["overall"] == "FAIL" else (
        "yellow" if summary["overall"] == "WARN" else "green"
    )
    print(
        _col("bold", "Summary: ", use_color)
        + _col(overall_col, summary["overall"], use_color)
        + f"  ({summary['fail']} FAIL / {summary['warn']} WARN / {summary['ok']} OK)"
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Audit section 1.5 of PERFORMANCE_EXPECTATIONS.md against "
                    "current source and artefact evidence.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--repo-root",
        default=None,
        help="Repository root directory (default: auto-detect from script location).",
    )
    p.add_argument(
        "--perf-doc",
        default=PERF_DOC_DEFAULT,
        help=f"Path to PERFORMANCE_EXPECTATIONS.md relative to repo-root "
             f"(default: {PERF_DOC_DEFAULT}).",
    )
    p.add_argument(
        "--output-dir",
        default=OUTPUT_DIR_DEFAULT,
        help=f"Output directory for report artefacts, relative to repo-root "
             f"(default: {OUTPUT_DIR_DEFAULT}).",
    )
    p.add_argument(
        "--format",
        choices=["json", "text", "both"],
        default="both",
        help="Output format for the report (default: both).",
    )
    p.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI colour output.",
    )
    p.add_argument(
        "-q", "--quiet",
        action="store_true",
        help="Suppress per-finding detail; only print summary.",
    )
    return p


def _detect_repo_root(script_path: Path) -> Path:
    """Walk up from the script directory to find the repo root (contains .git)."""
    candidate = script_path.parent
    for _ in range(6):
        if (candidate / ".git").exists():
            return candidate
        candidate = candidate.parent
    # Fallback: parent of tools/
    return script_path.parent.parent


def main(argv: Optional[List[str]] = None) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    use_color = not args.no_color and sys.stdout.isatty()

    # Determine repo root
    script_path = Path(__file__).resolve()
    if args.repo_root:
        repo_root = Path(args.repo_root).resolve()
    else:
        repo_root = _detect_repo_root(script_path)

    if not repo_root.is_dir():
        print(f"ERROR: repo-root not a directory: {repo_root}", file=sys.stderr)
        return 2

    # Read performance expectations document
    perf_doc_path = repo_root / args.perf_doc
    if not perf_doc_path.is_file():
        print(
            f"ERROR: Performance doc not found: {perf_doc_path}", file=sys.stderr
        )
        return 2
    perf_doc_content = perf_doc_path.read_text(encoding="utf-8", errors="replace")

    # Extract section 1.5 for the report
    section_15_text = _extract_section_15(perf_doc_content)

    # Load all CMakeLists.txt content from benchmarks/
    cmake_content = _load_cmake_contents(repo_root)

    # Run per-KPI checks
    all_findings: List[Finding] = []
    for kpi in KPI_DEFINITIONS:
        all_findings.extend(
            _check_kpi(kpi, repo_root, cmake_content, section_15_text)
        )

    # Run meta-cause checks
    all_findings.extend(_check_meta_causes(repo_root, cmake_content))

    # Build report
    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    report = _build_report(all_findings, repo_root, section_15_text, generated_at)

    # Write report artefacts
    output_dir = repo_root / args.output_dir
    json_path: Optional[Path] = None
    md_path: Optional[Path] = None
    if args.format in ("json", "both"):
        json_path = _write_json_report(report, output_dir)
    if args.format in ("text", "both"):
        md_path = _write_markdown_report(report, output_dir)

    # Print to terminal
    _print_findings(all_findings, report, use_color, args.quiet)

    if json_path:
        print(f"JSON report: {json_path}")
    if md_path:
        print(f"MD report:   {md_path}")

    # Exit code
    return 1 if report["summary"]["overall"] == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
