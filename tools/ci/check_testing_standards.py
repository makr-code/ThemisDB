#!/usr/bin/env python3
"""Inventory and fail-fast structural checks for C++ test and benchmark standards."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


TRIVIAL_GTEST_MAIN = re.compile(
    r"int\s+main\s*\([^{}]*\)\s*\{\s*"
    r"::?testing::InitGoogleTest\s*\([^{}]*\);\s*"
    r"return\s+RUN_ALL_TESTS\s*\(\s*\);\s*\}",
    re.MULTILINE | re.DOTALL,
)

CUSTOM_GTEST_MAIN = re.compile(r"\bint\s+main\s*\([^{}]*\)", re.MULTILINE)
BENCHMARK_MAIN = re.compile(r"\bBENCHMARK_MAIN\s*\(\s*\)", re.MULTILINE)
CUSTOM_BENCHMARK_MAIN = re.compile(
    r"\b(?:benchmark::Initialize|benchmark::RunSpecifiedBenchmarks)\s*\(", re.MULTILINE
)


def scan_cpp(root: Path, directory: str) -> dict[str, list[str]]:
    base = root / directory
    result: dict[str, list[str]] = {
        "trivial_gtest_main": [],
        "custom_gtest_main": [],
        "benchmark_main": [],
        "custom_benchmark_main": [],
    }

    for path in sorted(base.rglob("*")):
        if path.suffix not in {".c", ".cc", ".cpp", ".cxx"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        rel = path.relative_to(root).as_posix()
        if directory == "tests":
            if TRIVIAL_GTEST_MAIN.search(text):
                result["trivial_gtest_main"].append(rel)
            elif CUSTOM_GTEST_MAIN.search(text):
                result["custom_gtest_main"].append(rel)
        else:
            if BENCHMARK_MAIN.search(text):
                result["benchmark_main"].append(rel)
            if CUSTOM_BENCHMARK_MAIN.search(text):
                result["custom_benchmark_main"].append(rel)
    return result


def scan_raw_add_test(root: Path) -> list[str]:
    findings: list[str] = []
    tests_dir = root / "tests"
    for path in sorted(tests_dir.rglob("CMakeLists.txt")):
        text = path.read_text(encoding="utf-8", errors="replace")
        if re.search(r"\badd_test\s*\(", text):
            findings.append(path.relative_to(root).as_posix())
    return findings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository-root", type=Path, default=REPO_ROOT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    root = args.repository_root.resolve()
    tests = scan_cpp(root, "tests")
    benchmarks = scan_cpp(root, "benchmarks")
    raw_add_test = scan_raw_add_test(root)
    report = {
        "tests": tests,
        "benchmarks": benchmarks,
        "raw_add_test_cmake_files": raw_add_test,
    }

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    print(f"Trivial GoogleTest mains: {len(tests['trivial_gtest_main'])}")
    print(f"Custom GoogleTest mains: {len(tests['custom_gtest_main'])}")
    print(f"BENCHMARK_MAIN files: {len(benchmarks['benchmark_main'])}")
    print(f"Custom benchmark main calls: {len(benchmarks['custom_benchmark_main'])}")
    print(f"CMake files containing raw add_test: {len(raw_add_test)}")

    if args.strict:
        violations = (
            len(tests["trivial_gtest_main"])
            + len(tests["custom_gtest_main"])
            + len(raw_add_test)
        )
        return 1 if violations else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
