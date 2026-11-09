#!/usr/bin/env python3
"""Select and run bounded deterministic smoke tests for changed parser targets."""

from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
TARGET_CORPORA = {
    "aql_parser": "aql",
    "http_parser": "http",
    "wire_protocol_parser": "wire_protocol",
    "postgres_importer": "importer",
}
REQUIRED_SEEDS = {
    "aql_parser": {"seed_select.aql"},
    "http_parser": {"seed_get.http"},
    "wire_protocol_parser": {
        "seed_invalid_magic.bin",
        "seed_oversized_length.bin",
        "seed_ping.bin",
        "seed_ping_checksum.bin",
    },
    "postgres_importer": {
        "seed_copy_row.bin",
        "seed_create_table.sql",
        "seed_full_dump.sql",
        "seed_insert_values.sql",
        "seed_utf8_copy.sql",
    },
}


def affected_targets(paths: list[str]) -> list[str]:
    """Return parser harnesses affected by repository-relative changed paths."""
    targets: set[str] = set()
    for path in paths:
        normalized = path.replace("\\", "/")
        if normalized.startswith("./"):
            normalized = normalized[2:]
        if normalized in (
            "fuzz/CMakeLists.txt",
            "cmake/CMakeLists.txt",
            "cmake/ModularBuild.cmake",
            "tools/ci/fuzz_smoke.py",
            ".github/workflows/security-fuzzing.yml",
        ):
            targets.update(TARGET_CORPORA)
            continue
        if (
            normalized.startswith(("src/aql/", "include/aql/", "tests/aql/"))
            or normalized.startswith(("src/query/aql_parser", "include/query/aql_parser"))
            or normalized.startswith("tests/query/aql_parser")
            or normalized.startswith("fuzz/corpus/aql/")
            or normalized == "fuzz/harnesses/aql_parser_harness.cpp"
        ):
            targets.add("aql_parser")
        if (
            normalized.startswith(("src/server/http", "include/server/http", "tests/server/http"))
            or normalized.startswith(
                (
                    "src/api/http",
                    "include/api/http",
                    "tests/network/http",
                    "tests/network/test_http",
                    "tests/integration/test_http",
                )
            )
            or normalized.startswith("fuzz/corpus/http/")
            or normalized == "fuzz/harnesses/http_parser_harness.cpp"
        ):
            targets.add("http_parser")
        if (
            normalized.startswith(
                (
                    "src/network/wire_protocol",
                    "src/network/websocket_binary_frame_parser",
                    "include/network/wire_protocol",
                )
            )
            or normalized.startswith("include/themis/network/wire_protocol")
            or normalized.startswith("src/themis/wire_protocol")
            or normalized.startswith(("tests/network/test_wire_protocol", "tests/themis/test_themis_wire_protocol"))
            or normalized.startswith("fuzz/corpus/wire_protocol/")
            or normalized == "fuzz/harnesses/wire_protocol_parser_harness.cpp"
        ):
            targets.add("wire_protocol_parser")
        if (
            normalized.startswith(("src/importers/", "include/importers/", "tests/importers/"))
            or normalized.startswith("fuzz/corpus/importer/")
            or normalized == "fuzz/harnesses/postgres_importer_harness.cpp"
        ):
            targets.add("postgres_importer")
    return sorted(targets)


def run_smoke(targets: list[str]) -> int:
    for target in targets:
        corpus = ROOT / "fuzz" / "corpus" / TARGET_CORPORA[target]
        binary = ROOT / "build-fuzz" / "fuzz" / "bin" / f"{target}_harness"
        if not binary.is_file():
            print(f"ERROR: selected harness missing: {binary}", file=sys.stderr)
            return 1
        if not corpus.is_dir():
            print(f"ERROR: selected corpus missing: {corpus}", file=sys.stderr)
            return 1
        seeds = sorted(path for path in corpus.iterdir() if path.is_file())
        missing = REQUIRED_SEEDS[target] - {seed.name for seed in seeds}
        if missing:
            print(f"ERROR: required corpus seeds missing for {target}: {sorted(missing)}", file=sys.stderr)
            return 1
        if not seeds:
            print(f"ERROR: empty corpus for {target}: {corpus}", file=sys.stderr)
            return 1

        print(f"Smoke-testing {target}: {len(seeds)} tracked/current seed(s), parser budget=300s")
        deadline = time.monotonic() + 300
        for seed in seeds:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                print(f"ERROR: {target} exceeded its 300-second parser budget", file=sys.stderr)
                return 1
            try:
                result = subprocess.run(
                    [str(binary), str(seed)],
                    cwd=ROOT,
                    capture_output=True,
                    text=True,
                    timeout=remaining,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                print(f"ERROR: {target} timed out on corpus seed {seed.name}", file=sys.stderr)
                return 1
            output = result.stdout + result.stderr
            if result.returncode != 0:
                print(f"ERROR: {target} failed on {seed.name} (exit {result.returncode})\n{output}", file=sys.stderr)
                return 1
            if "AddressSanitizer" in output or "UndefinedBehaviorSanitizer" in output or "runtime error:" in output:
                print(f"ERROR: sanitizer diagnostic from {target} on {seed.name}\n{output}", file=sys.stderr)
                return 1
    print(f"PASS: deterministic parser smoke completed for {', '.join(targets)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("targets", "run"), required=True)
    parser.add_argument("--target", default="changed", choices=("changed", "all", *TARGET_CORPORA))
    parser.add_argument("--changed-files", nargs="*", default=[])
    args = parser.parse_args()
    if args.target == "all":
        targets = list(TARGET_CORPORA)
    elif args.target == "changed":
        targets = affected_targets(args.changed_files)
    else:
        targets = [args.target]
    if args.mode == "targets":
        print("\n".join(targets))
        return 0
    if not targets:
        print("No parser target affected; no fuzz-smoke work required.")
        return 0
    return run_smoke(targets)


if __name__ == "__main__":
    raise SystemExit(main())
