"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            perf_coverage_top10_audit.py                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-14 19:10:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   58.0/100                                       ║
    • Total Lines:     1053                                           ║
    • Open Issues:     TODOs: 1, Stubs: 11                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 42c0aaa20a  2026-04-14  feat(benchmarks): standardize LLM/RAG/LoRA artifact prefl... ║
    • cf3e31ffa9  2026-04-13  feat(governance): Disabled-Stub-Policy für Benchmarks ein... ║
    • aac9b9ed5a  2026-04-14  feat(benchmarks): standardize LLM/RAG/LoRA artifact prefl... ║
    • 1071f1d20f  2026-04-13  feat(governance): Disabled-Stub-Policy für Benchmarks ein... ║
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
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PERF_DOC_DEFAULT = "PERFORMANCE_EXPECTATIONS.md"
OUTPUT_DIR_DEFAULT = "artifacts/perf_coverage_top10_audit"

_COL = {
    "red": "\033[31m",
    "yellow": "\033[33m",
    "green": "\033[32m",
    "cyan": "\033[36m",
    "bold": "\033[1m",
    "reset": "\033[0m",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _col(name: str, text: str, use_color: bool) -> str:
    if not use_color:
        return text
    return f"{_COL[name]}{text}{_COL['reset']}"


def _read_file(path: Path) -> Optional[str]:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


def _file_exists(repo_root: Path, rel: str) -> bool:
    return (repo_root / rel).is_file()


def _glob_files(repo_root: Path, rel_dir: str, pattern: str) -> List[Path]:
    base = repo_root / rel_dir
    if not base.is_dir():
        return []
    return sorted(base.glob(pattern))


def _collect_benchmark_lines(path: Path) -> List[str]:
    """Return every BENCHMARK / BENCHMARK_DEFINE_F / BENCHMARK_REGISTER_F line."""
    content = _read_file(path)
    if content is None:
        return []
    pattern = re.compile(
        r"^\s*(BENCHMARK(?:_DEFINE_F|_REGISTER_F)?)\s*\(", re.MULTILINE
    )
    lines: List[str] = []
    for m in pattern.finditer(content):
        start = m.start(1)
        end = content.find("\n", start)
        lines.append(content[start: end if end != -1 else start + 200].strip())
    return lines


def _cmake_has_target(cmake_content: str, name: str) -> bool:
    return bool(re.search(
        r"add_executable\s*\(\s*" + re.escape(name) + r"\b",
        cmake_content, re.MULTILINE,
    ))


def _load_workflows(repo_root: Path) -> Dict[str, str]:
    """Return {filename: content} for every .yml file under .github/workflows/."""
    wf_dir = repo_root / ".github" / "workflows"
    result: Dict[str, str] = {}
    if wf_dir.is_dir():
        for p in sorted(wf_dir.glob("*.yml")):
            c = _read_file(p)
            if c:
                result[p.name] = c
    return result


def _load_cmake(repo_root: Path) -> str:
    parts: List[str] = []
    for p in (repo_root / "benchmarks").rglob("CMakeLists.txt"):
        c = _read_file(p)
        if c:
            parts.append(c)
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Finding
# ---------------------------------------------------------------------------

class Finding:
    LEVELS = ("OK", "INFO", "WARN", "FAIL")

    def __init__(
        self,
        measure_id: str,
        label: str,
        level: str,
        code: str,
        message: str,
        evidence: Optional[str] = None,
    ):
        assert level in self.LEVELS, f"Unknown level: {level}"
        self.measure_id = measure_id
        self.label = label
        self.level = level
        self.code = code
        self.message = message
        self.evidence = evidence

    def to_dict(self) -> Dict:
        d = {
            "measure_id": self.measure_id,
            "label": self.label,
            "level": self.level,
            "code": self.code,
            "message": self.message,
        }
        if self.evidence:
            d["evidence"] = self.evidence
        return d


# ---------------------------------------------------------------------------
# Individual measure checks
# ---------------------------------------------------------------------------

def _m01_pagination_benchmarks(repo_root: Path, cmake: str) -> List[Finding]:
    """
    M1: bench_query.cpp Pagination-Benchmarks wieder registrieren/stabilisieren.
    Success criteria: BM_Pagination_Offset AND BM_Pagination_Cursor registered with
    BENCHMARK() macros, AND the stale comment "not currently registered" is removed.
    """
    fid, label = "M01", "Pagination Benchmarks (bench_query.cpp)"
    findings: List[Finding] = []
    src = repo_root / "benchmarks/bench_query.cpp"
    content = _read_file(src)
    if content is None:
        findings.append(Finding(fid, label, "FAIL", "missing_source_file",
                                "benchmarks/bench_query.cpp not found."))
        return findings

    bench_lines = _collect_benchmark_lines(src)
    bench_text = "\n".join(bench_lines)

    has_offset = bool(re.search(r"BENCHMARK\s*\(\s*BM_Pagination_Offset\b", bench_text))
    has_cursor = bool(re.search(r"BENCHMARK\s*\(\s*BM_Pagination_Cursor\b", bench_text))
    stale_comment = bool(re.search(
        r"not currently registered with BENCHMARK", content, re.IGNORECASE
    ))

    if has_offset and has_cursor:
        findings.append(Finding(fid, label, "OK", "pagination_registered",
                                "BM_Pagination_Offset and BM_Pagination_Cursor are registered."))
    elif has_offset or has_cursor:
        missing = "BM_Pagination_Cursor" if has_offset else "BM_Pagination_Offset"
        findings.append(Finding(fid, label, "FAIL", "pagination_partially_registered",
                                f"Only one pagination benchmark is registered; {missing} is missing.",
                                evidence="benchmarks/bench_query.cpp"))
    else:
        findings.append(Finding(fid, label, "FAIL", "pagination_not_registered",
                                "Neither BM_Pagination_Offset nor BM_Pagination_Cursor is registered.",
                                evidence="benchmarks/bench_query.cpp"))

    if stale_comment:
        findings.append(Finding(
            fid, label, "WARN", "stale_comment_present",
            "Comment 'not currently registered with BENCHMARK() macros' still present in "
            "bench_query.cpp (line ~144). Should be removed now that they are registered.",
            evidence="benchmarks/bench_query.cpp:144",
        ))
    else:
        findings.append(Finding(fid, label, "OK", "stale_comment_removed",
                                "Stale 'not currently registered' comment is gone."))

    return findings


def _m02_olap_analytics_real_cases(repo_root: Path, cmake: str) -> List[Finding]:
    """
    M2: bench_olap_analytics.cpp from disabled stub to real cases.
    Success criteria: at least 4 real BENCHMARK cases beyond BM_OLAP_Disabled.
    """
    fid, label = "M02", "OLAP Analytics real cases (bench_olap_analytics.cpp)"
    src = repo_root / "benchmarks/bench_olap_analytics.cpp"
    content = _read_file(src)
    if content is None:
        return [Finding(fid, label, "FAIL", "missing_source_file",
                        "benchmarks/bench_olap_analytics.cpp not found.")]

    bench_lines = _collect_benchmark_lines(src)
    real_cases = [l for l in bench_lines if not re.search(r"OLAP_Disabled", l)]
    has_disabled = any(re.search(r"OLAP_Disabled", l) for l in bench_lines)

    findings: List[Finding] = []
    if len(real_cases) >= 4:
        findings.append(Finding(
            fid, label, "OK", "olap_real_cases_present",
            f"Found {len(real_cases)} real OLAP benchmark case(s) "
            f"(target: >= 4). Disabled stub {'still present' if has_disabled else 'removed'}.",
        ))
    elif len(real_cases) > 0:
        findings.append(Finding(
            fid, label, "WARN", "olap_real_cases_insufficient",
            f"Found {len(real_cases)} real OLAP case(s) but target is >= 4. "
            "Need more production cases in bench_olap_analytics.cpp.",
            evidence=f"Real cases found: {real_cases}",
        ))
    else:
        if has_disabled:
            findings.append(Finding(
                fid, label, "FAIL", "olap_only_disabled_stub",
                "bench_olap_analytics.cpp only contains BM_OLAP_Disabled. "
                "Must be converted to >= 4 real analytics benchmark cases.",
                evidence="benchmarks/bench_olap_analytics.cpp",
            ))
        else:
            findings.append(Finding(
                fid, label, "FAIL", "olap_no_cases",
                "bench_olap_analytics.cpp has no benchmark cases at all.",
                evidence="benchmarks/bench_olap_analytics.cpp",
            ))
    return findings


def _m03_security_governance_binaries(repo_root: Path, cmake: str) -> List[Finding]:
    """
    M3: Security/Governance binaries + runtime DLL-sync (ERLEDIGT).
    Verifies that the four benchmark binaries mentioned in §1.4 have CMake targets.
    """
    fid, label = "M03", "Security/Governance binaries (ERLEDIGT check)"
    required_targets = [
        "bench_security",
        "bench_governance_policy_latency",
        "bench_compliance_security_governance",
    ]
    findings: List[Finding] = []
    all_ok = True
    for t in required_targets:
        if _cmake_has_target(cmake, t):
            findings.append(Finding(
                fid, label, "OK", "cmake_target_found",
                f"CMake target found: {t}",
            ))
        else:
            all_ok = False
            findings.append(Finding(
                fid, label, "WARN", "cmake_target_missing",
                f"CMake target not found: {t} — may have been renamed or removed.",
                evidence="benchmarks/CMakeLists.txt",
            ))
    if all_ok:
        findings.append(Finding(fid, label, "OK", "security_governance_complete",
                                "All security/governance benchmark targets present (ERLEDIGT confirmed)."))
    return findings


def _m04_voice_benchmark_ci(repo_root: Path, workflows: Dict[str, str]) -> List[Finding]:
    """
    M4: Voice-benchmark path for CI via THEMIS_ENABLE_VOICE_ASSISTANT optional job.
    Success criteria: a workflow job exists that references bench_voice_assistant
    and sets THEMIS_ENABLE_VOICE_ASSISTANT=ON.
    """
    fid, label = "M04", "Voice benchmark CI job (THEMIS_ENABLE_VOICE_ASSISTANT)"
    # Also check CMake target exists
    cmake = _load_cmake(repo_root)
    has_cmake = _cmake_has_target(cmake, "bench_voice_assistant")

    findings: List[Finding] = []
    if not has_cmake:
        findings.append(Finding(fid, label, "WARN", "voice_cmake_target_missing",
                                "bench_voice_assistant has no CMake target with THEMIS_ENABLE_VOICE_ASSISTANT guard."))
    else:
        findings.append(Finding(fid, label, "OK", "voice_cmake_target_found",
                                "CMake target bench_voice_assistant found (gated on THEMIS_ENABLE_VOICE_ASSISTANT)."))

    voice_wf_file: Optional[str] = None
    for fname, content in workflows.items():
        if re.search(r"THEMIS_ENABLE_VOICE_ASSISTANT", content) and re.search(
            r"bench_voice_assistant", content
        ):
            voice_wf_file = fname
            break

    # Also check for any workflow that sets VOICE_ASSISTANT=ON (looser check)
    voice_wf_loose: Optional[str] = None
    if not voice_wf_file:
        for fname, content in workflows.items():
            if re.search(r"THEMIS_ENABLE_VOICE_ASSISTANT\s*=\s*ON", content):
                voice_wf_loose = fname
                break

    if voice_wf_file:
        findings.append(Finding(
            fid, label, "OK", "voice_ci_job_found",
            f"CI workflow with THEMIS_ENABLE_VOICE_ASSISTANT + bench_voice_assistant found: {voice_wf_file}",
        ))
    elif voice_wf_loose:
        findings.append(Finding(
            fid, label, "WARN", "voice_ci_job_partial",
            f"Workflow sets THEMIS_ENABLE_VOICE_ASSISTANT=ON ({voice_wf_loose}) "
            "but does not explicitly run bench_voice_assistant. "
            "Add an explicit build+run step for the voice benchmark.",
            evidence=f".github/workflows/{voice_wf_loose}",
        ))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "voice_ci_job_missing",
            "No CI workflow found that sets THEMIS_ENABLE_VOICE_ASSISTANT=ON and "
            "builds/runs bench_voice_assistant. "
            "Add an optional CI job for the Voice benchmark path.",
            evidence="benchmarks/bench_voice_assistant.cpp exists; "
                     "benchmarks/CMakeLists.txt has the target gated on THEMIS_ENABLE_VOICE_ASSISTANT.",
        ))
    return findings


def _m05_gpu_benchmark_matrix(repo_root: Path, workflows: Dict[str, str]) -> List[Finding]:
    """
    M5: GPU benchmark matrix (CUDA/HIP/Vulkan) as separate runner.
    Success criteria: a workflow with a matrix strategy that includes a GPU runner
    and builds/runs benchmarks with THEMIS_ENABLE_CUDA=ON or THEMIS_ENABLE_HIP=ON.
    """
    fid, label = "M05", "GPU benchmark CI matrix (CUDA/HIP/Vulkan runner)"
    findings: List[Finding] = []

    gpu_wf_full: Optional[str] = None
    gpu_wf_partial: Optional[str] = None
    for fname, content in workflows.items():
        has_gpu_runner = bool(re.search(
            r"runs-on\s*:.*(?:cuda|gpu|nvidia|amd|hip|vulkan)", content, re.IGNORECASE
        ))
        has_gpu_cmake = bool(re.search(
            r"THEMIS_ENABLE_(?:CUDA|HIP)\s*=\s*ON", content, re.IGNORECASE
        ))
        has_matrix = "matrix:" in content
        has_bench = bool(re.search(r"bench_(?:fused|gpu|cuda)", content, re.IGNORECASE))

        if has_gpu_runner and has_gpu_cmake:
            gpu_wf_full = fname
            break
        if has_gpu_cmake and has_bench and has_matrix:
            gpu_wf_partial = fname

    if gpu_wf_full:
        findings.append(Finding(
            fid, label, "OK", "gpu_ci_matrix_found",
            f"GPU CI matrix with dedicated runner and CUDA/HIP=ON found: {gpu_wf_full}",
        ))
    elif gpu_wf_partial:
        findings.append(Finding(
            fid, label, "WARN", "gpu_ci_matrix_partial",
            f"Found workflow with GPU benchmarks and matrix strategy ({gpu_wf_partial}) "
            "but no explicit GPU/CUDA/HIP runner label detected. "
            "Add a self-hosted GPU runner to the matrix.",
            evidence=f".github/workflows/{gpu_wf_partial}",
        ))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "gpu_ci_matrix_missing",
            "No CI workflow found that builds GPU benchmarks with THEMIS_ENABLE_CUDA=ON "
            "or THEMIS_ENABLE_HIP=ON on a dedicated GPU runner. "
            "All GPU benchmark targets (bench_fused_kernels, bench_gpu_training_cycle, ...) "
            "remain unverified in CI.",
            evidence="benchmarks/CMakeLists.txt has multiple GPU-gated targets.",
        ))
    return findings


def _m06_llm_model_artifact_preparation(repo_root: Path) -> List[Finding]:
    """
    M6: Standardize model/artifact preparation (LLM, LoRA, gguf).
    Success criteria: a setup script or documented procedure exists that
    downloads/prepares models for benchmarks without missing-artifact errors.
    """
    fid, label = "M06", "LLM/LoRA/gguf model artifact preparation"
    findings: List[Finding] = []

    # Check for setup scripts (canonical names checked by this audit +
    # the ThemisDB implementation script)
    setup_patterns = [
        "scripts/download_models.sh",
        "scripts/setup_llm_models.sh",
        "scripts/prepare_models.py",
        "scripts/prepare_artifacts.sh",
        "scripts/download_gguf.sh",
        "scripts/setup_benchmarks.sh",
        "scripts/model_setup.sh",
        "scripts/model_setup.py",
        # ThemisDB implementation script (Maßnahme #6)
        "scripts/prepare_llm_bench_artifacts.sh",
    ]
    found_scripts = [p for p in setup_patterns if _file_exists(repo_root, p)]

    # Also check for the standardised C++ preflight header
    has_preflight_header = _file_exists(
        repo_root, "benchmarks/benchmark_artifact_preflight.h"
    )

    # Check for LLM benchmark docs mentioning setup
    llm_doc_paths = [
        "src/llm/gguf_loader_README.md",
        "src/llm/llama_lora_adapter_README.md",
        "docs/BENCHMARK_RUNBOOK.md",
        "benchmarks/QUICKSTART.md",
        "benchmarks/README.md",
    ]
    docs_with_model_prep: List[str] = []
    for rel in llm_doc_paths:
        c = _read_file(repo_root / rel)
        if c and re.search(r"gguf|model.*download|download.*model|benchmark.*model|artifact.*prep", c, re.IGNORECASE):
            docs_with_model_prep.append(rel)

    # Check for CI workflow that sets up models
    workflows_dir = repo_root / ".github" / "workflows"
    model_setup_wf: List[str] = []
    if workflows_dir.is_dir():
        for wf in workflows_dir.glob("*.yml"):
            c = _read_file(wf)
            if c and re.search(r"gguf|download.*model|model.*download|benchmark.*model|download_models|prepare_llm_bench", c, re.IGNORECASE):
                model_setup_wf.append(wf.name)

    if found_scripts:
        findings.append(Finding(
            fid, label, "OK", "model_setup_script_found",
            f"Model setup script(s) found: {found_scripts}",
        ))
    elif docs_with_model_prep:
        findings.append(Finding(
            fid, label, "WARN", "model_setup_documented_only",
            "No dedicated model-setup script found; model preparation is documented "
            f"in: {docs_with_model_prep}. "
            "Consider a script (e.g. scripts/download_models.sh) for reproducible CI setup.",
            evidence=str(docs_with_model_prep),
        ))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "model_setup_missing",
            "No model-setup script and no documentation describing how to prepare "
            "gguf/LoRA/LLM model artefacts for benchmarks. "
            "LLM/RAG/LoRA benchmarks will fail with missing-artifact errors in a clean CI environment.",
            evidence="Benchmark files: bench_llm_inference_performance.cpp, "
                     "bench_lora_framework.cpp, bench_multi_lora_fusion.cpp etc.",
        ))

    if has_preflight_header:
        findings.append(Finding(
            fid, label, "OK", "benchmark_preflight_header_found",
            "benchmarks/benchmark_artifact_preflight.h present – "
            "benchmarks can call LLMArtifactPreflight::create() for clear error messages.",
        ))
    else:
        findings.append(Finding(
            fid, label, "WARN", "benchmark_preflight_header_missing",
            "benchmarks/benchmark_artifact_preflight.h not found. "
            "Without it, benchmarks may fail with opaque errors when artefacts are absent.",
        ))

    if model_setup_wf:
        findings.append(Finding(
            fid, label, "INFO", "model_setup_in_ci",
            f"CI workflow(s) with model-download logic found: {model_setup_wf[:3]}",
        ))
    return findings


def _m07_goal_id_benchmark_mapping(repo_root: Path) -> List[Finding]:
    """
    M7: Enforce goal-ID to benchmark mapping file (per module).
    Success criteria: a mapping file (JSON/YAML/CSV/MD) exists that maps
    goal/target IDs (e.g. Q-1, AN-10) to benchmark cases.
    """
    fid, label = "M07", "Goal-ID to benchmark mapping file"
    findings: List[Finding] = []

    candidate_paths = [
        "benchmarks/goal_benchmark_mapping.json",
        "benchmarks/goal_benchmark_mapping.yaml",
        "benchmarks/goal_benchmark_mapping.yml",
        "benchmarks/goal_benchmark_mapping.csv",
        "benchmarks/kpi_benchmark_map.json",
        "benchmarks/kpi_benchmark_map.yaml",
        "docs/benchmark_goal_mapping.md",
        "docs/ci-cd/benchmark_goal_mapping.md",
    ]
    found = [p for p in candidate_paths if _file_exists(repo_root, p)]

    # Also check for module-level mapping files
    module_maps = list((repo_root / "benchmarks").glob("*goal*map*")) + \
                  list((repo_root / "benchmarks").glob("*kpi*map*")) + \
                  list((repo_root / "benchmarks").glob("*target*map*"))

    if found:
        findings.append(Finding(
            fid, label, "OK", "mapping_file_found",
            f"Goal-ID to benchmark mapping file(s) found: {found}",
        ))
    elif module_maps:
        findings.append(Finding(
            fid, label, "OK", "mapping_file_found",
            f"Mapping file(s) found: {[str(p.relative_to(repo_root)) for p in module_maps[:3]]}",
        ))
    else:
        # Check if any doc or file contains a goal-ID mapping table
        perf_doc = repo_root / "PERFORMANCE_EXPECTATIONS.md"
        perf_content = _read_file(perf_doc) or ""
        has_mapping_table = bool(re.search(
            r"n\/v-Zeile.*Benchmark-File\|Benchmark-File.*n\/v-Zeile", perf_content
        ))
        if has_mapping_table:
            findings.append(Finding(
                fid, label, "WARN", "mapping_in_doc_only",
                "Goal-ID to benchmark mapping exists only in PERFORMANCE_EXPECTATIONS.md "
                "(§1.7.13 n/v-zu-Quelle-Matrix). "
                "A dedicated, machine-readable mapping file is missing. "
                "Create benchmarks/goal_benchmark_mapping.json to enforce 1:1 coverage.",
                evidence="PERFORMANCE_EXPECTATIONS.md §1.7.13",
            ))
        else:
            findings.append(Finding(
                fid, label, "FAIL", "mapping_file_missing",
                "No goal-ID to benchmark mapping file found. "
                "Every target-ID in the performance tables should map to exactly one "
                "primary benchmark case. "
                "Create benchmarks/goal_benchmark_mapping.json.",
            ))
    return findings


def _m08_source_exists_binary_missing_guard(repo_root: Path, workflows: Dict[str, str]) -> List[Finding]:
    """
    M8: CI guard 'source exists but binary missing'.
    Success criteria: a CI step or script checks that every bench_*.cpp has a
    corresponding CMake add_executable target.
    """
    fid, label = "M08", "CI guard: source exists but binary missing"
    findings: List[Finding] = []

    # Check for cmake-source-coverage-audit workflow
    guard_wf: Optional[str] = None
    for fname, content in workflows.items():
        if re.search(
            r"source.*(?:binary|target|executable)|(?:binary|target|executable).*missing|"
            r"bench.*cpp.*(?:no|without).*target|cmake.*source.*coverage|source.*coverage.*audit",
            content, re.IGNORECASE
        ):
            guard_wf = fname
            break

    # Check for cmake-source-coverage-audit.yml specifically
    if "cmake-source-coverage-audit.yml" in workflows:
        content = workflows["cmake-source-coverage-audit.yml"]
        if re.search(r"bench.*\.cpp|benchmark", content, re.IGNORECASE):
            guard_wf = "cmake-source-coverage-audit.yml"

    # Check for any audit script that counts bench cpp vs cmake targets
    audit_scripts = list((repo_root / "tools").glob("*coverage*audit*")) + \
                    list((repo_root / "tools").glob("*cmake*audit*")) + \
                    list((repo_root / "tools").glob("*source*missing*"))

    # Count actual coverage gap as evidence
    cmake_content = _load_cmake(repo_root)
    bench_dir = repo_root / "benchmarks"
    cpp_files = [f.name for f in bench_dir.glob("bench_*.cpp")]
    no_target = [f for f in cpp_files
                 if not re.search(r"add_executable\s*\(\s*" + re.escape(f.replace(".cpp", "")),
                                  cmake_content)]
    gap_evidence = (f"{len(no_target)}/{len(cpp_files)} bench_*.cpp files lack a CMake target "
                    f"(e.g. {', '.join(no_target[:5])}{'…' if len(no_target) > 5 else ''})")

    if guard_wf and re.search(r"bench.*\.cpp|bench_\*", workflows.get(guard_wf, ""), re.IGNORECASE):
        findings.append(Finding(
            fid, label, "OK", "source_binary_guard_found",
            f"CI guard for 'source exists but binary missing' found in: {guard_wf}",
            evidence=gap_evidence,
        ))
    elif guard_wf:
        findings.append(Finding(
            fid, label, "WARN", "source_binary_guard_partial",
            f"Workflow {guard_wf} exists but may not explicitly check bench_*.cpp coverage. "
            "Verify it catches the case where a bench_*.cpp has no CMake target.",
            evidence=gap_evidence,
        ))
    elif audit_scripts:
        findings.append(Finding(
            fid, label, "WARN", "source_binary_guard_script_only",
            f"Audit script(s) found ({[str(s.relative_to(repo_root)) for s in audit_scripts[:3]]}) "
            "but no CI workflow detected that runs it for bench_*.cpp coverage.",
            evidence=gap_evidence,
        ))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "source_binary_guard_missing",
            "No CI guard found for 'source exists but binary missing'. "
            f"Currently {gap_evidence}. "
            "Add a CI step or script that fails when a bench_*.cpp has no CMake target.",
        ))
    return findings


def _m09_disabled_stub_policy(repo_root: Path) -> List[Finding]:
    """
    M9: Disabled-stub policy (max 1 release, then mandatory ticket).
    Success criteria: every *_Disabled benchmark registration has a
    deadline comment and an issue reference in the same file, and the
    policy document docs/governance/DISABLED_STUB_POLICY.md exists.
    """
    fid, label = "M09", "Disabled-stub policy (issue + deadline in each *_Disabled)"
    findings: List[Finding] = []

    # Check that the policy document exists.
    policy_doc = repo_root / "docs" / "governance" / "DISABLED_STUB_POLICY.md"
    if policy_doc.exists():
        findings.append(Finding(fid, label, "OK", "policy_doc_present",
                                f"Policy document found: {policy_doc.relative_to(repo_root)}"))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "policy_doc_missing",
            "Policy document docs/governance/DISABLED_STUB_POLICY.md not found. "
            "Create it per the Disabled-Stub-Policy governance requirement.",
        ))

    # Check that the standalone CI guard exists.
    guard_script = repo_root / "tools" / "check_disabled_stubs.py"
    if guard_script.exists():
        findings.append(Finding(fid, label, "OK", "ci_guard_present",
                                f"CI guard found: {guard_script.relative_to(repo_root)}"))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "ci_guard_missing",
            "CI guard tools/check_disabled_stubs.py not found. "
            "Create it per the Disabled-Stub-Policy governance requirement.",
        ))

    bench_dir = repo_root / "benchmarks"
    files_with_disabled: List[str] = []
    files_missing_policy: List[str] = []
    files_compliant: List[str] = []

    for cpp in sorted(bench_dir.rglob("*.cpp")):
        content = _read_file(cpp)
        if content is None:
            continue
        if not re.search(r"BENCHMARK\s*\(BM_\w*_Disabled\b", content):
            continue
        rel = str(cpp.relative_to(repo_root))
        files_with_disabled.append(rel)

        has_issue = bool(re.search(
            r"(?:issue[:\s#]|ticket[:\s#]|jira[:\s#]|gh-\d+|github\.com.*/issues/\d+|"
            r"#\d{3,}|TODO.*deadline|FIXME.*deadline|DEADLINE\s*:|"
            r"max\s*\d+\s*release|release.*deadline|stub.*deadline|disabled.*issue)",
            content, re.IGNORECASE
        ))
        has_deadline = bool(re.search(
            r"(?:Deadline\s*:|deadline\s*:|DEADLINE\s*:|due[_\s]*date\s*:|sunset\s*:)",
            content, re.IGNORECASE
        ))
        if has_issue and has_deadline:
            files_compliant.append(rel)
        else:
            files_missing_policy.append(rel)

    if not files_with_disabled:
        findings.append(Finding(fid, label, "OK", "no_disabled_stubs",
                                "No *_Disabled benchmark stubs found."))
        return findings

    findings.append(Finding(
        fid, label, "INFO", "disabled_stub_count",
        f"Found {len(files_with_disabled)} file(s) with *_Disabled benchmarks: "
        f"{len(files_compliant)} compliant, {len(files_missing_policy)} missing policy.",
    ))

    if files_missing_policy:
        findings.append(Finding(
            fid, label, "FAIL", "disabled_stubs_without_policy",
            f"{len(files_missing_policy)} file(s) have *_Disabled stubs without a "
            "deadline comment or issue reference:\n  " +
            "\n  ".join(files_missing_policy[:10]) +
            ("\n  …" if len(files_missing_policy) > 10 else ""),
            evidence="Each *_Disabled BENCHMARK must include a comment with "
                     "an issue number (e.g. // Issue: #1234) and a deadline "
                     "(e.g. // Deadline: v1.9.0) per docs/governance/DISABLED_STUB_POLICY.md.",
        ))
    if files_compliant:
        findings.append(Finding(
            fid, label, "OK", "disabled_stubs_compliant",
            f"{len(files_compliant)} file(s) with *_Disabled stubs have policy references: "
            f"{', '.join(files_compliant[:5])}{'…' if len(files_compliant) > 5 else ''}",
        ))
    return findings


def _m10_nightly_benchmark_sweeps(repo_root: Path, workflows: Dict[str, str]) -> List[Finding]:
    """
    M10: Module-wise benchmark sweeps (modules 2..33) as plannable nightly presets.
    Success criteria: a nightly scheduled workflow exists that covers multiple
    benchmark modules and produces a coverage report with per-module status.
    """
    fid, label = "M10", "Nightly benchmark sweeps (modules 2..33)"
    findings: List[Finding] = []

    nightly_bench_wf: Optional[str] = None
    nightly_coverage_wf: Optional[str] = None
    for fname, content in workflows.items():
        if "schedule" not in content:
            continue
        has_bench = bool(re.search(r"bench(?:mark)?", content, re.IGNORECASE))
        has_coverage = bool(re.search(r"coverage.*report|sweep|module.*\d+\.\.\d+", content, re.IGNORECASE))
        if has_bench and has_coverage:
            nightly_bench_wf = fname
            break
        if has_bench and "cron" in content:
            nightly_coverage_wf = fname

    # Also check cross-module performance regression workflow
    regression_wf = "05-quality_build_cross-module-performance-regression-ci.yml"
    has_regression_wf = regression_wf in workflows

    if nightly_bench_wf:
        findings.append(Finding(
            fid, label, "OK", "nightly_sweep_found",
            f"Nightly benchmark sweep workflow found: {nightly_bench_wf}",
        ))
    elif has_regression_wf:
        content = workflows[regression_wf]
        has_nightly = "schedule" in content
        if has_nightly:
            findings.append(Finding(
                fid, label, "WARN", "nightly_regression_only",
                f"Cross-module regression workflow ({regression_wf}) with schedule found, "
                "but it appears to check regression only, not a full per-module coverage sweep. "
                "Extend it or add a dedicated nightly sweep for modules 2..33.",
                evidence=f".github/workflows/{regression_wf}",
            ))
        else:
            findings.append(Finding(
                fid, label, "WARN", "nightly_regression_no_schedule",
                f"Cross-module regression workflow ({regression_wf}) exists but has no "
                "nightly schedule. Add a cron trigger and per-module coverage reporting.",
                evidence=f".github/workflows/{regression_wf}",
            ))
    elif nightly_coverage_wf:
        findings.append(Finding(
            fid, label, "WARN", "nightly_bench_no_coverage",
            f"Nightly benchmark workflow found ({nightly_coverage_wf}) but no per-module "
            "coverage report / sweep detected. "
            "Add module 2..33 iteration with per-module pass/fail and delta comparison.",
            evidence=f".github/workflows/{nightly_coverage_wf}",
        ))
    else:
        findings.append(Finding(
            fid, label, "FAIL", "nightly_sweep_missing",
            "No nightly scheduled benchmark sweep workflow found for modules 2..33. "
            "There is no automated daily coverage report with per-module status. "
            "Add a workflow with a cron schedule that runs benchmark subsets per module "
            "and emits a coverage report.",
        ))
    return findings


# ---------------------------------------------------------------------------
# Section 1.4 text extraction
# ---------------------------------------------------------------------------

def _extract_section_14(content: str) -> str:
    m = re.search(r"(###\s+1\.4\b.*?)(?=\n###\s+1\.\d|\Z)", content, re.DOTALL)
    return m.group(1) if m else ""


# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------

def _build_report(
    findings: List[Finding],
    repo_root: Path,
    section_14_text: str,
    generated_at: str,
) -> Dict:
    fail_count = sum(1 for f in findings if f.level == "FAIL")
    warn_count = sum(1 for f in findings if f.level == "WARN")
    ok_count = sum(1 for f in findings if f.level == "OK")
    info_count = sum(1 for f in findings if f.level == "INFO")

    overall = "PASS"
    if fail_count > 0:
        overall = "FAIL"
    elif warn_count > 0:
        overall = "WARN"

    return {
        "tool": "perf_coverage_top10_audit",
        "version": "1.0.0",
        "generated_at": generated_at,
        "repo_root": str(repo_root),
        "summary": {
            "overall": overall,
            "total": len(findings),
            "fail": fail_count,
            "warn": warn_count,
            "ok": ok_count,
            "info": info_count,
        },
        "findings": [f.to_dict() for f in findings],
        "section_14_excerpt": section_14_text[:2000] + ("…" if len(section_14_text) > 2000 else ""),
    }


def _write_json_report(report: Dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    out = output_dir / "report.json"
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    return out


def _write_markdown_report(report: Dict, output_dir: Path) -> Path:
    summary = report["summary"]
    overall_icon = {"FAIL": "❌", "WARN": "⚠️", "PASS": "✅"}.get(summary["overall"], summary["overall"])
    lines = [
        "# Performance Coverage Top-10 Measures Audit (§1.4)",
        "",
        f"**Generated:** {report['generated_at']}  ",
        f"**Tool:** {report['tool']} v{report['version']}  ",
        f"**Overall:** {overall_icon} {summary['overall']}  ",
        f"**Findings:** {summary['total']} total — "
        f"{summary['fail']} FAIL / {summary['warn']} WARN / {summary['ok']} OK / {summary['info']} INFO",
        "",
        "## Findings",
        "",
        "| Measure | Level | Code | Message |",
        "|---------|-------|------|---------|",
    ]
    for f in report["findings"]:
        icon = {"FAIL": "❌", "WARN": "⚠️", "OK": "✅", "INFO": "ℹ️"}.get(f["level"], f["level"])
        msg = f["message"].replace("|", "&#124;").replace("\n", " ")[:200]
        lines.append(
            f"| {f['measure_id']} {f['label']} | {icon} {f['level']} | `{f['code']}` | {msg} |"
        )

    lines += [
        "",
        "## Measure Status Summary",
        "",
        "| Measure | Status | Notes |",
        "|---------|--------|-------|",
    ]
    # Aggregate per measure
    measure_status: Dict[str, str] = {}
    for f in report["findings"]:
        mid = f["measure_id"]
        cur = measure_status.get(mid, "OK")
        priority = {"FAIL": 3, "WARN": 2, "INFO": 1, "OK": 0}
        if priority.get(f["level"], 0) > priority.get(cur, 0):
            measure_status[mid] = f["level"]
    labels = {
        "M01": "Pagination Benchmarks registered",
        "M02": "OLAP Analytics real cases",
        "M03": "Security/Governance binaries",
        "M04": "Voice benchmark CI job",
        "M05": "GPU benchmark CI matrix",
        "M06": "LLM/LoRA/gguf model artifact prep",
        "M07": "Goal-ID to benchmark mapping file",
        "M08": "Source-exists-but-binary-missing guard",
        "M09": "Disabled-stub policy",
        "M10": "Nightly benchmark sweeps (modules 2..33)",
    }
    for mid, lbl in labels.items():
        status = measure_status.get(mid, "N/A")
        icon = {"FAIL": "❌", "WARN": "⚠️", "OK": "✅", "INFO": "ℹ️"}.get(status, status)
        lines.append(f"| {mid} | {icon} {status} | {lbl} |")

    lines += [
        "",
        "## §1.4 Excerpt",
        "",
        "```",
        report.get("section_14_excerpt", "(not extracted)"),
        "```",
        "",
        "---",
        "*Generated by `tools/perf_coverage_top10_audit.py`.*",
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
        print(_col("bold", "=== Performance Coverage Top-10 Measures Audit (§1.4) ===", use_color))
        print()
        for f in findings:
            col = {"FAIL": "red", "WARN": "yellow", "OK": "green", "INFO": "cyan"}.get(f.level, "reset")
            prefix = _col(col, f"[{f.level:4}]", use_color)
            print(f"  {prefix}  {f.measure_id:<4s}  {f.code}")
            print(f"             {f.message[:120]}")
            if f.evidence:
                print(f"             Evidence: {f.evidence[:100]}")
        print()

    overall_col = {"FAIL": "red", "WARN": "yellow", "PASS": "green"}.get(
        summary["overall"], "reset"
    )
    print(
        _col("bold", "Summary: ", use_color)
        + _col(overall_col, summary["overall"], use_color)
        + f"  ({summary['fail']} FAIL / {summary['warn']} WARN / "
          f"{summary['ok']} OK / {summary['info']} INFO)"
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Audit §1.4 Top-10 benchmark coverage measures against current source/CI.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("--repo-root", default=None,
                   help="Repository root (default: auto-detect from script location).")
    p.add_argument("--perf-doc", default=PERF_DOC_DEFAULT,
                   help=f"Path to PERFORMANCE_EXPECTATIONS.md relative to repo-root.")
    p.add_argument("--output-dir", default=OUTPUT_DIR_DEFAULT,
                   help=f"Output directory (relative to repo-root).")
    p.add_argument("--format", choices=["json", "text", "both"], default="both",
                   help="Output format.")
    p.add_argument("--no-color", action="store_true", help="Disable ANSI colour.")
    p.add_argument("-q", "--quiet", action="store_true", help="Suppress per-finding detail.")
    return p


def _detect_repo_root(script_path: Path) -> Path:
    candidate = script_path.parent
    for _ in range(6):
        if (candidate / ".git").exists():
            return candidate
        candidate = candidate.parent
    return script_path.parent.parent


def main(argv: Optional[List[str]] = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    use_color = not args.no_color and sys.stdout.isatty()

    script_path = Path(__file__).resolve()
    repo_root = Path(args.repo_root).resolve() if args.repo_root else _detect_repo_root(script_path)

    if not repo_root.is_dir():
        print(f"ERROR: repo-root not a directory: {repo_root}", file=sys.stderr)
        return 2

    perf_doc_path = repo_root / args.perf_doc
    if not perf_doc_path.is_file():
        print(f"ERROR: Performance doc not found: {perf_doc_path}", file=sys.stderr)
        return 2

    perf_doc_content = perf_doc_path.read_text(encoding="utf-8", errors="replace")
    section_14_text = _extract_section_14(perf_doc_content)
    cmake = _load_cmake(repo_root)
    workflows = _load_workflows(repo_root)

    all_findings: List[Finding] = []
    all_findings.extend(_m01_pagination_benchmarks(repo_root, cmake))
    all_findings.extend(_m02_olap_analytics_real_cases(repo_root, cmake))
    all_findings.extend(_m03_security_governance_binaries(repo_root, cmake))
    all_findings.extend(_m04_voice_benchmark_ci(repo_root, workflows))
    all_findings.extend(_m05_gpu_benchmark_matrix(repo_root, workflows))
    all_findings.extend(_m06_llm_model_artifact_preparation(repo_root))
    all_findings.extend(_m07_goal_id_benchmark_mapping(repo_root))
    all_findings.extend(_m08_source_exists_binary_missing_guard(repo_root, workflows))
    all_findings.extend(_m09_disabled_stub_policy(repo_root))
    all_findings.extend(_m10_nightly_benchmark_sweeps(repo_root, workflows))

    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    report = _build_report(all_findings, repo_root, section_14_text, generated_at)

    output_dir = repo_root / args.output_dir
    json_path: Optional[Path] = None
    md_path: Optional[Path] = None
    if args.format in ("json", "both"):
        json_path = _write_json_report(report, output_dir)
    if args.format in ("text", "both"):
        md_path = _write_markdown_report(report, output_dir)

    _print_findings(all_findings, report, use_color, args.quiet)

    if json_path:
        print(f"JSON report: {json_path}")
    if md_path:
        print(f"MD report:   {md_path}")

    return 1 if report["summary"]["overall"] == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
