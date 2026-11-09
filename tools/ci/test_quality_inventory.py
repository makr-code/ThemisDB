#!/usr/bin/env python3
"""Generate a reproducible, source-backed test-quality inventory."""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import shlex
import subprocess
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


SCHEMA_VERSION = 1
SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp"}
TEST_CASE_PATTERN = re.compile(
    r"\b(?:TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P)\s*\("
)
SKIP_PATTERN = re.compile(r"\bGTEST_SKIP\s*\(\s*\)")
TRIVIAL_ASSERTION_PATTERNS = (
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*true\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_FALSE\s*\(\s*false\s*\)"),
    re.compile(
        r'\b(?:EXPECT|ASSERT)_EQ\s*\(\s*("(?:\\.|[^"\\])*"|[-+]?\d+(?:\.\d+)?|'
        r"nullptr|NULL|true|false)\s*,\s*\1\s*\)"
    ),
)
CMK_TARGET_PATTERN = re.compile(
    r"(?:^|[/\\])CMakeFiles[/\\]([^/\\]+)\.dir(?:[/\\]|$)"
)

MODULE_ISSUES = {
    "server": 7543,
    "distributed_tensor": 7512,
    "prompt_engineering": 7538,
    "timeseries": 7549,
    "scheduler": 7541,
    "whisper": 7558,
    "llama_cpp": 7526,
    "analytics": 7500,
}

HOTSPOT_EVIDENCE = {
    "server": {
        "owner_issue": 7543,
        "tests": [
            "tests/server/test_server_phase1_hardening.cpp",
            "tests/server/test_server_openapi_drift_focused.cpp",
            "tests/server/test_server_gateway_resilience_focused.cpp",
        ],
        "focused_ctest": [
            "test_server_phase1_hardening_server_FocusedTests",
            "test_server_openapi_drift_focused_server_FocusedTests",
            "test_server_gateway_resilience_focused_server_FocusedTests",
        ],
        "follow_up": "Complete route/authZ and failure-path matrix; cross-cutting route matrix also links SEC-13.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "distributed_tensor": {
        "owner_issue": 7512,
        "tests": [
            "tests/distributed_tensor/test_tensor_operations.cpp",
            "tests/distributed_tensor/test_distributed_tensor_highcardinality_stress.cpp",
        ],
        "focused_ctest": [
            "test_tensor_operations_distributed_tensor_FocusedTests",
            "test_distributed_tensor_highcardinality_stress_distributed_tensor_FocusedTests",
        ],
        "follow_up": "Add persistence/restart/concurrency evidence; retain the owner issue for module implementation.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "prompt_engineering": {
        "owner_issue": 7538,
        "tests": [
            "tests/prompt_engineering/test_prompt_engineering_validation_hardening.cpp",
            "tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp",
        ],
        "focused_ctest": [
            "test_prompt_engineering_validation_hardening_prompt_engineering_FocusedTests",
            "test_prompt_engineering_adversarial_focused_prompt_engineering_FocusedTests",
        ],
        "follow_up": "Complete validation, safety, and degraded-path behavior evidence.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "timeseries": {
        "owner_issue": 7549,
        "tests": [
            "tests/timeseries/test_timeseries_error_path_phase3.cpp",
            "tests/timeseries/test_timeseries_contract_hardening_focused.cpp",
            "tests/timeseries/test_timeseries_retention.cpp",
        ],
        "focused_ctest": [
            "test_timeseries_error_path_phase3_TimeseriesFocusedTests",
            "test_timeseries_contract_hardening_focused_TimeseriesFocusedTests",
            "test_timeseries_retention_TimeseriesFocusedTests",
        ],
        "follow_up": "Close retention, compaction, concurrency, restart, and recovery evidence.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "scheduler": {
        "owner_issue": 7541,
        "tests": [
            "tests/scheduler/test_scheduler_coordination_failures.cpp",
            "tests/scheduler/test_scheduler_concurrency_edge_cases.cpp",
            "tests/scheduler/test_scheduler_contract_hardening_focused.cpp",
        ],
        "focused_ctest": [
            "test_scheduler_coordination_failures_scheduler_FocusedTests",
            "test_scheduler_concurrency_edge_cases_scheduler_FocusedTests",
            "test_scheduler_contract_hardening_focused_scheduler_FocusedTests",
        ],
        "follow_up": "Complete fairness, capacity, timeout/cancellation, distributed failure, and recovery tests.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "whisper": {
        "owner_issue": 7558,
        "tests": [
            "tests/whisper/test_whisper_stub_transcribe_bridge.cpp",
            "tests/whisper/test_whisper_highcardinality_stress.cpp",
        ],
        "focused_ctest": [
            "test_whisper_stub_transcribe_bridge_whisper_FocusedTests",
            "test_whisper_highcardinality_stress_whisper_FocusedTests",
        ],
        "follow_up": "Add backend/model lifecycle and preview behavior validation; stub-path coverage is not real-backend evidence.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "llama_cpp": {
        "owner_issue": 7526,
        "tests": [
            "src/llama_cpp/tests/test_llama_cpp_model_loading.cpp",
            "src/llama_cpp/tests/test_llama_cpp_inference_execution.cpp",
            "tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp",
        ],
        "focused_ctest": [
            "test_llama_cpp_model_loading_llamacpp_FocusedTests",
            "test_llama_cpp_inference_execution_llamacpp_FocusedTests",
            "test_llama_cpp_highcardinality_stress_llamacpp_FocusedTests",
        ],
        "follow_up": "Validate model/runtime lifecycle against the real backend; stub-mode and stress tests do not establish that evidence.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
    "analytics": {
        "owner_issue": 7500,
        "tests": [
            "tests/analytics/test_analytics_contract_hardening_focused.cpp",
            "tests/analytics/test_analytics_concurrency_safety_focused.cpp",
            "tests/analytics/test_analytics_error_handling_focused.cpp",
        ],
        "focused_ctest": [
            "test_analytics_contract_hardening_focused_AnalyticsFocusedTests",
            "test_analytics_concurrency_safety_focused_AnalyticsFocusedTests",
            "test_analytics_error_handling_focused_AnalyticsFocusedTests",
        ],
        "follow_up": "Disposition assertion-quality findings and publish module line/branch evidence.",
        "target": "v2.5.0 M1 (2027-03-31)",
    },
}


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def _relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def _source_module(path: Path, root: Path) -> str | None:
    relative = path.relative_to(root)
    if len(relative.parts) < 3 or relative.parts[0] != "src":
        return None
    return relative.parts[1]


def _test_module(path: Path, root: Path) -> tuple[str, bool]:
    relative = path.relative_to(root)
    if relative.parts[0] == "src":
        return relative.parts[1], False
    if len(relative.parts) > 1 and relative.parts[1] == "legacy":
        return "legacy", True
    if relative.parts[0] == "tests" and len(relative.parts) == 2:
        return "root", False
    if len(relative.parts) > 1:
        return relative.parts[1], False
    return "root", False


def _line_count(path: Path) -> int:
    with path.open(encoding="utf-8", errors="replace") as source:
        return sum(1 for _ in source)


def _source_counts(root: Path) -> dict[str, dict[str, int]]:
    counts: dict[str, dict[str, int]] = defaultdict(lambda: {"files": 0, "loc": 0})
    source_root = root / "src"
    if not source_root.is_dir():
        return counts
    for path in source_root.rglob("*"):
        module = _source_module(path, root) if path.is_file() else None
        if module is None or path.suffix.lower() not in SOURCE_EXTENSIONS:
            continue
        counts[module]["files"] += 1
        counts[module]["loc"] += _line_count(path)
    return counts


def _test_sources(root: Path) -> list[Path]:
    candidates = list((root / "tests").rglob("test_*")) + list((root / "src").glob("*/tests/test_*"))
    return sorted(
        {
            path
            for path in candidates
            if path.is_file() and path.suffix.lower() in SOURCE_EXTENSIONS
        },
        key=lambda path: _relative(path, root),
    )


def _compile_database(path: Path, root: Path) -> dict[str, set[str]]:
    entries = json.loads(_read_text(path))
    if not isinstance(entries, list):
        raise ValueError("compile-commands file must contain a JSON array")
    targets_by_source: dict[str, set[str]] = defaultdict(set)
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("file"), str):
            raise ValueError(f"invalid compile-commands entry at index {index}")
        source = Path(entry["file"])
        if not source.is_absolute():
            directory = Path(entry.get("directory", root))
            if not directory.is_absolute():
                directory = root / directory
            source = directory / source
        match = CMK_TARGET_PATTERN.search(
            str(entry.get("output", "")) + " " + str(entry.get("command", ""))
        )
        if match:
            targets_by_source[os.path.normcase(str(source.resolve()))].add(match.group(1))
    return targets_by_source


def _ctest_targets(path: Path) -> dict[str, list[str]]:
    data = json.loads(_read_text(path))
    tests = data.get("tests") if isinstance(data, dict) else None
    if not isinstance(tests, list):
        raise ValueError("CTest JSON must contain a tests array")
    result: dict[str, list[str]] = defaultdict(list)
    for test in tests:
        if not isinstance(test, dict) or not isinstance(test.get("name"), str):
            continue
        command = test.get("command", [])
        if not isinstance(command, list) or not command or not isinstance(command[0], str):
            continue
        target = Path(command[0]).stem
        result[target].append(test["name"])
    return result


def _skip_findings(source: str, text: str, issue: int) -> list[dict[str, object]]:
    findings = []
    for match in SKIP_PATTERN.finditer(text):
        statement_end = text.find(";", match.end())
        if statement_end < 0:
            statement_end = text.find("\n", match.end())
        if statement_end < 0:
            statement_end = len(text)
        statement = text[match.end() : statement_end]
        reason_match = re.search(r'"((?:\\.|[^"\\])*)"', statement)
        reason = reason_match.group(1) if reason_match else None
        findings.append(
            {
                "path": source,
                "line": text.count("\n", 0, match.start()) + 1,
                "reason": reason,
                "reason_status": "documented" if reason else "missing",
                "issue": issue,
                "issue_url": f"https://github.com/makr-code/ThemisDB/issues/{issue}",
            }
        )
    return findings


def _assertion_findings(source: str, text: str) -> list[dict[str, object]]:
    findings = []
    for line_number, line in enumerate(text.splitlines(), start=1):
        for pattern in TRIVIAL_ASSERTION_PATTERNS:
            for match in pattern.finditer(line):
                findings.append(
                    {
                        "path": source,
                        "line": line_number,
                        "expression": match.group(0),
                        "classification": "lexical_vacuous_assertion_candidate",
                        "manual_review_required": True,
                    }
                )
    return findings


def _tool_version(command: list[str]) -> str:
    try:
        result = subprocess.run(
            command, check=False, capture_output=True, text=True, timeout=15
        )
    except (OSError, subprocess.TimeoutExpired):
        return "unavailable"
    output = (result.stdout or result.stderr).strip().splitlines()
    return output[0] if output else "unavailable"


def _git_value(root: Path, *args: str) -> str | None:
    result = subprocess.run(
        ["git", *args], cwd=root, check=False, capture_output=True, text=True
    )
    return result.stdout.strip() if result.returncode == 0 else None


def build_inventory(
    root: Path,
    *,
    compile_commands: Path | None = None,
    ctest_json: Path | None = None,
    coverage_json: Path | None = None,
    source_commit: str | None = None,
    command_line: str | None = None,
) -> dict[str, object]:
    root = root.resolve()
    sources = _source_counts(root)
    source_targets = (
        _compile_database(compile_commands, root) if compile_commands else {}
    )
    ctest_names = _ctest_targets(ctest_json) if ctest_json else {}
    source_path_to_targets = {
        key: sorted(value) for key, value in source_targets.items()
    }

    modules: dict[str, dict[str, object]] = defaultdict(
        lambda: {
            "source": {"files": 0, "loc": 0},
            "tests": {
                "files": 0,
                "cases": 0,
                "registration_verified": compile_commands is not None,
                "skips": 0,
                "skips_with_reason": 0,
                "assertion_candidates": 0,
                "skips_detail": [],
                "assertion_findings": [],
            },
        }
    )

    for module, metrics in sources.items():
        modules[module]["source"] = metrics

    test_files: list[dict[str, object]] = []
    legacy_files: list[dict[str, object]] = []
    for path in _test_sources(root):
        module, is_legacy = _test_module(path, root)
        relative = _relative(path, root)
        text = _read_text(path)
        case_count = len(TEST_CASE_PATTERN.findall(text))
        issue = MODULE_ISSUES.get(module, 7820)
        skips = _skip_findings(relative, text, issue)
        assertions = _assertion_findings(relative, text)
        targets = source_path_to_targets.get(
            os.path.normcase(str(path.resolve())), []
        )
        registered = bool(targets)
        ctest_cases = sorted(
            {
                name
                for target in targets
                for name in ctest_names.get(target, [])
            }
        )
        disposition = (
            "legacy_active_target" if registered else "legacy_review_pending"
        ) if is_legacy else "not_legacy"
        file_record = {
            "path": relative,
            "module": module,
            "test_case_declarations": case_count,
            "skip_count": len(skips),
            "assertion_candidate_count": len(assertions),
            "registration_status": (
                "registered" if registered else "not_in_profile"
            ) if compile_commands else "not_verified",
            "targets": targets,
            "ctest_cases": ctest_cases,
            "legacy_disposition": disposition,
            "legacy_follow_up_issue": 7820 if is_legacy else None,
            "legacy_follow_up_target": "v2.5.0 M1 (2027-03-31)" if is_legacy else None,
        }
        test_files.append(file_record)
        if is_legacy:
            legacy_files.append(file_record)
        module_metrics = modules[module]["tests"]
        module_metrics["files"] += 1
        module_metrics["cases"] += case_count
        module_metrics["skips"] += len(skips)
        module_metrics["skips_with_reason"] += sum(
            finding["reason_status"] == "documented" for finding in skips
        )
        module_metrics["assertion_candidates"] += len(assertions)
        module_metrics["skips_detail"].extend(skips)
        module_metrics["assertion_findings"].extend(assertions)

    coverage = None
    if coverage_json:
        coverage_data = json.loads(_read_text(coverage_json))
        coverage = coverage_data.get("modules", {})
        if not isinstance(coverage, dict):
            raise ValueError("coverage JSON must contain a modules object")

    module_rows = []
    for module in sorted(modules):
        item = modules[module]
        item["name"] = module
        source_loc = item["source"]["loc"]
        test_file_count = item["tests"]["files"]
        test_case_count = item["tests"]["cases"]
        item["density"] = {
            "source_loc_per_test_file": round(source_loc / test_file_count, 2)
            if test_file_count
            else None,
            "source_loc_per_test_case": round(source_loc / test_case_count, 2)
            if test_case_count
            else None,
        }
        item["skip_budget"] = {
            "baseline": item["tests"]["skips"],
            "m1_maximum": math.ceil(item["tests"]["skips"] / 2),
            "issue": MODULE_ISSUES.get(module, 7820),
            "reduction_goal": "at least 50% from this versioned baseline by v2.5.0 M1",
        }
        item["coverage"] = coverage.get(module) if coverage is not None else None
        item["coverage_status"] = (
            "measured" if item["coverage"] is not None else "not_available"
        )
        item["hotspot_evidence"] = HOTSPOT_EVIDENCE.get(module)
        module_rows.append(item)

    total_skips = sum(item["tests"]["skips"] for item in module_rows)
    total_assertions = sum(
        item["tests"]["assertion_candidates"] for item in module_rows
    )
    baseline = {
        "schema_version": SCHEMA_VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "source_commit": source_commit or _git_value(root, "rev-parse", "HEAD"),
        "branch": _git_value(root, "branch", "--show-current"),
        "command": command_line,
        "tool_versions": {
            "python": sys.version.split()[0],
            "cmake": _tool_version(["cmake", "--version"]),
            "ctest": _tool_version(["ctest", "--version"]),
            "lcov": _tool_version(["lcov", "--version"]),
            "compiler": _tool_version(
                shlex.split(os.environ.get("CXX", "g++")) + ["--version"]
            ),
        },
        "methodology": {
            "source_loc": "Physical lines in C/C++ source/header files under src/<module>/, including blank and comment lines.",
            "test_case_count": "Lexical count of TEST, TEST_F, TEST_P, TYPED_TEST, and TYPED_TEST_P declarations; parameterized runtime instances are not expanded.",
            "skip_reason": "First string literal after GTEST_SKIP() on the same C++ statement; missing literals are reported for manual disposition.",
            "assertion_quality": "Lexical vacuous-assertion candidates only; findings require manual review and are not an automatic deletion instruction.",
            "legacy_disposition": "CMake compile-database registration when supplied; legacy files without verified target mapping remain review-pending.",
            "coverage": "Production line/branch coverage from the supplied lcov-derived module JSON; null means no successful current run was available.",
            "comparability": "Source LOC and lexical test declarations are not comparable with the 2026-10-31 audit's undocumented counting method.",
        },
        "inputs": {
            "compile_commands": str(compile_commands.resolve()) if compile_commands else None,
            "ctest_json": str(ctest_json.resolve()) if ctest_json else None,
            "coverage_json": str(coverage_json.resolve()) if coverage_json else None,
        },
        "summary": {
            "modules": len(module_rows),
            "test_files": len(test_files),
            "test_case_declarations": sum(
                item["tests"]["cases"] for item in module_rows
            ),
            "gtest_skip_calls": total_skips,
            "skip_calls_with_literal_reason": sum(
                item["tests"]["skips_with_reason"] for item in module_rows
            ),
            "trivial_assertion_candidates": total_assertions,
            "legacy_test_files": len(legacy_files),
            "legacy_review_pending": sum(
                item["legacy_disposition"] == "legacy_review_pending"
                for item in legacy_files
            ),
            "coverage_status": (
                "measured" if coverage is not None else "unavailable"
            ),
        },
        "modules": {row["name"]: row for row in module_rows},
        "test_files": test_files,
        "legacy_files": legacy_files,
    }
    return baseline


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--compile-commands",
        type=Path,
        help="optional CMake compile_commands.json for target dispositions",
    )
    parser.add_argument(
        "--ctest-json",
        type=Path,
        help="optional output from ctest --show-only=json-v1",
    )
    parser.add_argument(
        "--coverage-json",
        type=Path,
        help="optional per-module JSON from generate_coverage_baseline.py",
    )
    parser.add_argument("--source-commit", help="commit SHA represented by this scan")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("audit/test_quality_baseline_v1.json"),
    )
    args = parser.parse_args(argv)
    root = args.root.resolve()
    output = args.output if args.output.is_absolute() else root / args.output
    try:
        report = build_inventory(
            root,
            compile_commands=args.compile_commands,
            ctest_json=args.ctest_json,
            coverage_json=args.coverage_json,
            source_commit=args.source_commit,
            command_line=shlex.join(sys.argv),
        )
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"test-quality inventory error: {exc}", file=sys.stderr)
        return 2
    print(
        f"Inventory: {report['summary']['test_files']} test files, "
        f"{report['summary']['test_case_declarations']} declarations, "
        f"{report['summary']['gtest_skip_calls']} skips; wrote {output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
