#!/usr/bin/env python3
"""Validate full CTest JUnit coverage and write a commit-linked module summary."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import Any


def _properties(test: dict[str, Any]) -> dict[str, Any]:
    properties = test.get("properties", [])
    if isinstance(properties, dict):
        return properties
    result: dict[str, Any] = {}
    if isinstance(properties, list):
        for item in properties:
            if isinstance(item, dict) and "name" in item:
                result[str(item["name"])] = item.get("value")
    return result


def _labels(test: dict[str, Any]) -> set[str]:
    value = _properties(test).get("LABELS", [])
    if isinstance(value, str):
        return {label.strip() for label in value.split(";") if label.strip()}
    if isinstance(value, list):
        return {str(label).strip() for label in value if str(label).strip()}
    return set()


def _is_disabled(test: dict[str, Any]) -> bool:
    value = _properties(test).get("DISABLED", False)
    return value is True or str(value).strip().lower() in {"true", "on", "1"}


def _module_for(test_name: str, labels: set[str], modules: set[str]) -> str:
    normalized_labels = {
        label.removeprefix("module_").removeprefix("module:")
        for label in labels
    }
    matching_labels = normalized_labels & modules
    if matching_labels:
        return max(matching_labels, key=len)

    for module in sorted(modules, key=len, reverse=True):
        if (
            test_name.startswith(f"module_{module}_")
            or test_name.startswith(f"test_{module}_")
            or f"_{module}_" in test_name
        ):
            return module
    return "unclassified"


def _read_expected_tests(
    build_dir: Path,
    *,
    ctest_preset: str = "",
    repo_root: Path | None = None,
) -> list[dict[str, Any]]:
    command = ["ctest"]
    if ctest_preset:
        command.extend(["--preset", ctest_preset])
    else:
        command.extend(["--test-dir", str(build_dir)])
    command.append("--show-only=json-v1")
    result = subprocess.run(
        command,
        check=True,
        capture_output=True,
        text=True,
        cwd=repo_root,
    )
    payload = json.loads(result.stdout)
    tests = payload.get("tests")
    if not isinstance(tests, list):
        raise ValueError("CTest JSON did not contain a tests array")
    return [test for test in tests if isinstance(test, dict) and not _is_disabled(test)]


def _summarize(
    expected_tests: list[dict[str, Any]],
    junit_path: Path,
    modules: set[str],
    *,
    platform: str,
    commit_sha: str,
    workflow_run_url: str,
) -> tuple[str, bool]:
    issues: list[str] = []
    actual: dict[str, str] = {}
    try:
        root = ET.parse(junit_path).getroot()
        cases = list(root.iter("testcase"))
    except (OSError, ET.ParseError) as exc:
        cases = []
        issues.append(f"Unable to read complete CTest JUnit output: {exc}")

    for case in cases:
        name = case.attrib.get("name", "")
        if not name:
            issues.append("JUnit contains a testcase without a name")
            continue
        if name in actual:
            issues.append(f"JUnit contains duplicate testcase '{name}'")
            continue
        if case.find("failure") is not None or case.find("error") is not None:
            status = "failed"
        elif case.find("skipped") is not None:
            status = "skipped"
        else:
            status = "passed"
        actual[name] = status

    expected_by_name: dict[str, dict[str, Any]] = {}
    for test in expected_tests:
        name = str(test.get("name", ""))
        if not name or name in expected_by_name:
            issues.append(f"CTest inventory contains an empty or duplicate test name: {name!r}")
            continue
        expected_by_name[name] = test

    missing = sorted(set(expected_by_name) - set(actual))
    unexpected = sorted(set(actual) - set(expected_by_name))
    if not expected_by_name:
        issues.append("CTest reported no enabled registered tests")
    if missing:
        issues.append(f"JUnit is missing {len(missing)} registered test(s)")
    if unexpected:
        issues.append(f"JUnit contains {len(unexpected)} test(s) absent from the CTest inventory")
    failed = sorted(name for name, status in actual.items() if status == "failed")
    skipped = sorted(name for name, status in actual.items() if status == "skipped")
    if failed:
        issues.append(f"{len(failed)} test(s) failed")
    if skipped:
        issues.append(f"{len(skipped)} test(s) were skipped; skips are not counted as passes")

    module_counts: dict[str, dict[str, int]] = defaultdict(
        lambda: {"passed": 0, "failed": 0, "skipped": 0, "missing": 0}
    )
    for name, test in expected_by_name.items():
        module = _module_for(name, _labels(test), modules)
        status = actual.get(name)
        if status is None:
            module_counts[module]["missing"] += 1
        else:
            module_counts[module][status] += 1

    for name in unexpected:
        module = _module_for(name, set(), modules)
        module_counts[module]["failed"] += 1

    total = len(expected_by_name)
    passed = sum(counts["passed"] for counts in module_counts.values())
    status = "PASS" if not issues else "FAIL"
    lines = [
        f"## Full CTest Evidence — {platform}",
        "",
        f"- **Status:** {status}",
        f"- **Commit:** `{commit_sha}`",
        f"- **Workflow run:** {workflow_run_url}",
        f"- **Enabled registered tests:** {total}",
        f"- **JUnit testcases:** {len(cases)}",
        f"- **Passed:** {passed}",
        f"- **Failed:** {len(failed)}",
        f"- **Skipped:** {len(skipped)}",
        "",
        "| Module | Passed | Failed | Skipped | Missing |",
        "|---|---:|---:|---:|---:|",
    ]
    for module in sorted(module_counts):
        counts = module_counts[module]
        lines.append(
            f"| `{module}` | {counts['passed']} | {counts['failed']} | "
            f"{counts['skipped']} | {counts['missing']} |"
        )
    if issues:
        lines.extend(["", "### Evidence problems", ""])
        lines.extend(f"- {issue}" for issue in issues)
    return "\n".join(lines) + "\n", not issues


def summarize(
    build_dir: Path,
    junit_path: Path,
    output_path: Path,
    *,
    platform: str,
    commit_sha: str,
    workflow_run_url: str,
    repo_root: Path,
    ctest_preset: str = "",
) -> bool:
    modules = {
        path.name
        for path in (repo_root / "src").iterdir()
        if path.is_dir()
    }
    try:
        expected_tests = _read_expected_tests(
            build_dir,
            ctest_preset=ctest_preset,
            repo_root=repo_root,
        )
        summary, valid = _summarize(
            expected_tests,
            junit_path,
            modules,
            platform=platform,
            commit_sha=commit_sha,
            workflow_run_url=workflow_run_url,
        )
    except (OSError, ValueError, json.JSONDecodeError, subprocess.SubprocessError) as exc:
        summary = (
            f"## Full CTest Evidence — {platform}\n\n"
            "- **Status:** FAIL\n"
            f"- **Commit:** `{commit_sha}`\n"
            f"- **Workflow run:** {workflow_run_url}\n\n"
            f"Evidence validation could not complete: {exc}\n"
        )
        valid = False

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(summary, encoding="utf-8")
    return valid


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--commit-sha", required=True)
    parser.add_argument("--workflow-run-url", required=True)
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    parser.add_argument("--ctest-preset", default="")
    args = parser.parse_args(argv)
    valid = summarize(
        args.build_dir,
        args.junit,
        args.output,
        platform=args.platform,
        commit_sha=args.commit_sha,
        workflow_run_url=args.workflow_run_url,
        repo_root=args.repo_root.resolve(),
        ctest_preset=args.ctest_preset,
    )
    print(args.output.read_text(encoding="utf-8"), end="")
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
