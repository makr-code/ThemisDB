#!/usr/bin/env python3
"""Validate repeated DIS-11 CTest JUnit evidence for the transaction chaos lane."""

from __future__ import annotations

import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path


EXPECTED_TEST = "DIS10RealTransportProcessRecoveryTests"


def validate_evidence(
    evidence_dir: Path,
    *,
    expected_runs: int,
    commit_sha: str,
    workflow_run_url: str,
) -> tuple[dict, bool]:
    issues: list[str] = []
    runs: list[dict] = []

    for index in range(1, expected_runs + 1):
        junit = evidence_dir / f"dis11-run-{index}.xml"
        run: dict[str, object] = {"run": index, "junit": junit.name}
        if not junit.is_file():
            issues.append(f"missing JUnit evidence: {junit.name}")
            runs.append(run)
            continue

        try:
            root = ET.parse(junit).getroot()
        except ET.ParseError as exc:
            issues.append(f"{junit.name}: malformed XML: {exc}")
            runs.append(run)
            continue

        cases = [
            case
            for suite in ([root] if root.tag == "testsuite" else root.findall(".//testsuite"))
            for case in (
                ([suite] if suite.tag == "testcase" else suite.findall(".//testcase"))
            )
        ]
        passed = len(cases) == 1
        skipped = 0
        failures = 0
        if len(cases) != 1:
            issues.append(f"{junit.name}: expected exactly one testcase, found {len(cases)}")
        else:
            case = cases[0]
            name = case.get("name", "")
            if name != EXPECTED_TEST:
                passed = False
                issues.append(f"{junit.name}: unexpected testcase '{name}'")
            failures = len(case.findall(".//failure")) + len(case.findall(".//error"))
            skipped = len(case.findall(".//skipped"))
            if failures:
                passed = False
                issues.append(f"{junit.name}: testcase contains failure/error elements")
            if skipped:
                passed = False
                issues.append(f"{junit.name}: testcase was skipped")

        run.update({
            "test": EXPECTED_TEST if len(cases) == 1 else None,
            "passed": bool(passed and not failures and not skipped),
            "failures": failures,
            "skipped": skipped,
        })
        runs.append(run)

    valid = not issues
    evidence = {
        "schema": "themisdb.dis11.network-chaos.v1",
        "commit_sha": commit_sha,
        "workflow_run_url": workflow_run_url,
        "expected_runs": expected_runs,
        "runs": runs,
        "passed": valid,
    }
    return evidence, valid


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    parser.add_argument("--commit-sha", required=True)
    parser.add_argument("--workflow-run-url", required=True)
    parser.add_argument("--expected-runs", type=int, default=3)
    args = parser.parse_args()

    evidence_dir = args.evidence_dir
    evidence_dir.mkdir(parents=True, exist_ok=True)
    evidence, valid = validate_evidence(
        evidence_dir,
        expected_runs=args.expected_runs,
        commit_sha=args.commit_sha,
        workflow_run_url=args.workflow_run_url,
    )
    json_path = evidence_dir / "dis11-evidence.json"
    json_path.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")

    summary = [
        "# DIS-11 Network Chaos Evidence",
        "",
        f"- **Status:** {'PASS' if valid else 'FAIL'}",
        f"- **Commit:** `{evidence['commit_sha']}`",
        f"- **Workflow:** {evidence['workflow_run_url']}",
        f"- **Required runs:** {evidence['expected_runs']}",
        "",
        "| Run | JUnit | Test | Result |",
        "| ---: | --- | --- | --- |",
    ]
    for run in evidence["runs"]:
        result = "PASS" if run.get("passed") else "FAIL"
        summary.append(
            f"| {run['run']} | `{run['junit']}` | "
            f"`{run.get('test') or '-'}` | {result} |"
        )
    (evidence_dir / "DIS11_EVIDENCE_SUMMARY.md").write_text(
        "\n".join(summary) + "\n", encoding="utf-8"
    )
    print("\n".join(summary), end="")
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
