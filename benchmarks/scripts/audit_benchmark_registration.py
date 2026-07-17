#!/usr/bin/env python3
"""Audit benchmark C++ source registration in benchmarks CMake files."""

from __future__ import annotations

import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BENCHMARKS_DIR = REPO_ROOT / "benchmarks"

# Intentionally excluded from default benchmark registration.
INTENTIONAL_EXCLUSIONS = {
    "performance_optimizations/phase2/benchmark_phase2.cpp",
}


def is_benchmark_source(path: Path) -> bool:
    name = path.name
    return (
        name.startswith("bench")
        or name.startswith("benchmark_")
        or name == "llm_bench.cpp"
        or name.endswith("_bench.cpp")
    )


def normalize_cpp_token(token: str, cmake_dir: Path) -> str | None:
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


NAMED_REGISTRATION_PATTERNS = [
    # themis_add_standard_benchmark(target source.cpp)
    r"themis_add_standard_benchmark\(\s*[^\s\)]+\s+([^\s\)]+\.cpp)\s*\)",
    # wave5_add_benchmark(target source.cpp label)
    r"wave5_add_benchmark\(\s*[^\s\)]+\s+([^\s\)]+\.cpp)\s*[^\)]*\)",
    # add_w7_benchmark(target source.cpp)
    r"add_w7_benchmark\(\s*[^\s\)]+\s+([^\s\)]+\.cpp)\s*\)",
]


def collect_registered_sources() -> set[str]:
    registered: set[str] = set()

    for cmake_file in BENCHMARKS_DIR.rglob("CMakeLists.txt"):
        cmake_dir = cmake_file.parent.resolve()
        text = cmake_file.read_text(encoding="utf-8", errors="ignore")

        for pattern in NAMED_REGISTRATION_PATTERNS:
            for match in re.finditer(pattern, text):
                normalized = normalize_cpp_token(match.group(1), cmake_dir)
                if normalized:
                    registered.add(normalized)

        for match in re.finditer(r"add_executable\(\s*[^\s\)]+(.*?)\)", text, re.S):
            for token in re.findall(r"([A-Za-z0-9_./${}-]+\.cpp)", match.group(1)):
                normalized = normalize_cpp_token(token, cmake_dir)
                if normalized:
                    registered.add(normalized)

    return registered


def main() -> int:
    benchmark_sources = sorted(
        str(path.relative_to(BENCHMARKS_DIR)).replace("\\", "/")
        for path in BENCHMARKS_DIR.rglob("*.cpp")
        if is_benchmark_source(path)
    )

    registered = collect_registered_sources()
    excluded = sorted(path for path in benchmark_sources if path in INTENTIONAL_EXCLUSIONS)
    unregistered = sorted(
        path
        for path in benchmark_sources
        if path not in registered and path not in INTENTIONAL_EXCLUSIONS
    )

    print(f"Benchmark sources discovered: {len(benchmark_sources)}")
    print(f"Registered in CMake: {len([s for s in benchmark_sources if s in registered])}")
    print(f"Intentionally excluded: {len(excluded)}")
    print(f"Unregistered: {len(unregistered)}")

    if excluded:
        print("\nIntentional exclusions:")
        for path in excluded:
            print(f"  - {path}")

    if unregistered:
        print("\nUnregistered benchmark sources:")
        for path in unregistered:
            print(f"  - {path}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
