#!/usr/bin/env python3
"""Build and validate the registered Wave B transaction suites on one runner."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

import summarize_ctest_evidence as evidence


SMOKE_TESTS = {"TransactionWaveAClosureTests"}
CHAOS_TESTS = SMOKE_TESTS | {
    "TransactionDistributedPhase2Tests",
    "TransactionSAGACompensationPhase2Tests",
    "TransactionFaultInjectionPhase3Tests",
    "TransactionProcessCrashRecoveryTests",
    "DistributedTxnApiHandlerRecoveryTests",
}
REQUIRED_TESTS = CHAOS_TESTS | {
    "TransactionLifecyclePhase1Tests",
    "TransactionIsolationContentionPhase1Tests",
    "TransactionErrorPathDeterminismPhase1Tests",
}


def select_tests(inventory: list[dict[str, Any]], suite: str) -> list[dict[str, Any]]:
    required = {"smoke": SMOKE_TESTS, "chaos": CHAOS_TESTS, "full": REQUIRED_TESTS}[suite]
    enabled = [test for test in inventory if not evidence._is_disabled(test)]
    names = [str(test.get("name", "")) for test in enabled]
    if len(names) != len(set(names)):
        raise ValueError("CTest inventory contains duplicate enabled test names")
    missing = required - set(names)
    if missing:
        raise ValueError(f"Missing enabled transaction suites: {', '.join(sorted(missing))}")
    selected = [
        test for test in enabled
        if test["name"] in required
        or (suite == "full" and "module:transaction" in evidence._labels(test))
    ]
    for test in selected:
        target_metadata(test)
    return selected


def target_metadata(test: dict[str, Any]) -> tuple[str, Path]:
    command = test.get("command", [])
    targets = [arg.removeprefix("-DTARGET_NAME=") for arg in command if arg.startswith("-DTARGET_NAME=")]
    files = [arg.removeprefix("-DTARGET_FILE=") for arg in command if arg.startswith("-DTARGET_FILE=")]
    if len(targets) != 1 or not targets[0] or len(files) != 1 or not files[0]:
        raise ValueError(f"{test.get('name')}: missing unambiguous registered target/binary metadata")
    return targets[0], Path(files[0])


def run(
    build_dir: Path,
    suite: str,
    output_dir: Path,
    *,
    parallel: int,
    build_only: bool,
    commit_sha: str,
    run_url: str,
) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)
    junit = output_dir / "results.xml"
    summary = output_dir / "summary.md"
    # Never accept a cached report from an earlier execution.
    junit.unlink(missing_ok=True)
    summary.unlink(missing_ok=True)
    (output_dir / "inventory.json").unlink(missing_ok=True)
    selected = select_tests(evidence._read_expected_tests(build_dir), suite)
    (output_dir / "inventory.json").write_text(json.dumps(selected, indent=2) + "\n", encoding="utf-8")
    targets = sorted({target_metadata(test)[0] for test in selected})
    subprocess.run(
        ["cmake", "--build", str(build_dir), "--target", *targets, "--parallel", str(parallel)],
        check=True,
    )
    for test in selected:
        _, binary = target_metadata(test)
        if not binary.is_file():
            raise ValueError(f"{test['name']}: built executable is missing: {binary}")
    if build_only:
        return 0

    regex = "^(" + "|".join(re.escape(test["name"]) for test in selected) + ")$"
    result = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--tests-regex", regex,
         "--no-tests=error", "--output-on-failure", "--verbose", "--parallel", "1",
         "--output-junit", str(junit)],
        env={**os.environ, "THEMIS_CTEST_DISABLE_ON_DEMAND_BUILD": "1",
             "THEMIS_CTEST_STRICT_EVIDENCE": "1"},
        check=False,
    )
    report, valid = evidence._summarize(
        selected, junit, {"transaction"},
        platform=f"Linux / transaction / {suite}",
        commit_sha=commit_sha,
        workflow_run_url=run_url,
    )
    if result.returncode:
        report = report.replace("- **Status:** PASS", "- **Status:** FAIL", 1)
    report += f"\n- **CTest exit code:** {result.returncode}\n"
    summary.write_text(report, encoding="utf-8")
    print(report, flush=True)
    return result.returncode if result.returncode else (0 if valid else 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--suite", choices=("smoke", "full", "chaos"), required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--parallel", type=int, default=4)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--commit-sha", required=True)
    parser.add_argument("--run-url", required=True)
    args = parser.parse_args()
    if args.parallel < 1:
        parser.error("--parallel must be positive")
    try:
        return run(
            args.build_dir.resolve(), args.suite, args.output_dir.resolve(),
            parallel=args.parallel, build_only=args.build_only,
            commit_sha=args.commit_sha, run_url=args.run_url,
        )
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f"::error::Transaction evidence failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
