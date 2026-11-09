#!/usr/bin/env python3
"""Require explicit ownership and expiry for tolerated failures in gate workflows."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from datetime import date
from pathlib import Path
from typing import Any

import yaml


WORKFLOWS_DIR = Path(".github/workflows")
ALLOWLIST_PATH = Path("tools/ci/workflow_fail_open_allowlist.json")
GATE_WORKFLOW_NAMES = {
    "compliance-governance-gates.yml",
    "release-evidence-chain.yml",
    "wiki-pr-gate.yml",
}


def _is_gate_workflow(path: Path) -> bool:
    name = path.name
    return (
        name.startswith("gate-")
        or name.endswith("-gate.yml")
        or name.endswith("-gates.yml")
        or name in GATE_WORKFLOW_NAMES
    )


def _workflow_path(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def _exceptions_in_workflow(path: Path, relative_path: str) -> list[dict[str, str]]:
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    if not isinstance(data, dict):
        raise ValueError(f"{relative_path}: workflow root must be a mapping")

    exceptions: list[dict[str, str]] = []
    if "continue-on-error" in data and data["continue-on-error"] is not False:
        exceptions.append({
            "workflow": relative_path,
            "job": "<workflow>",
            "step": "<workflow>",
            "kind": "continue-on-error",
            "match": str(data["continue-on-error"]),
        })

    jobs = data.get("jobs") or {}
    if not isinstance(jobs, dict):
        raise ValueError(f"{relative_path}: jobs must be a mapping")
    for job_name, job in jobs.items():
        if not isinstance(job, dict):
            continue
        if "continue-on-error" in job and job["continue-on-error"] is not False:
            exceptions.append({
                "workflow": relative_path,
                "job": str(job_name),
                "step": "<job>",
                "kind": "continue-on-error",
                "match": str(job["continue-on-error"]),
            })
        steps = job.get("steps") or []
        job_with = job.get("with") or {}
        if isinstance(job_with, dict):
            for command in job_with.values():
                if not isinstance(command, str):
                    continue
                for line in command.splitlines():
                    normalized = line.strip()
                    if normalized and not normalized.startswith("#") and re.search(r"\|\|\s*true\b", normalized):
                        exceptions.append({
                            "workflow": relative_path,
                            "job": str(job_name),
                            "step": "<job-call>",
                            "kind": "shell-or-true",
                            "match": normalized,
                        })
        if not isinstance(steps, list):
            continue
        for index, step in enumerate(steps):
            if not isinstance(step, dict):
                continue
            step_name = str(step.get("name", f"<step-{index + 1}>"))
            if "continue-on-error" in step and step["continue-on-error"] is not False:
                exceptions.append({
                    "workflow": relative_path,
                    "job": str(job_name),
                    "step": step_name,
                    "kind": "continue-on-error",
                    "match": str(step["continue-on-error"]),
                })
            command_fields = [step.get("run")]
            with_values = step.get("with") or {}
            if isinstance(with_values, dict):
                command_fields.extend(with_values.values())
            for command in command_fields:
                if not isinstance(command, str):
                    continue
                for line in command.splitlines():
                    normalized = line.strip()
                    if normalized and not normalized.startswith("#") and re.search(r"\|\|\s*true\b", normalized):
                        exceptions.append({
                            "workflow": relative_path,
                            "job": str(job_name),
                            "step": step_name,
                            "kind": "shell-or-true",
                            "match": normalized,
                        })
    return exceptions


def _exception_key(exception: dict[str, Any]) -> tuple[str, str, str, str, str]:
    return tuple(
        str(exception.get(key, ""))
        for key in ("workflow", "job", "step", "kind", "match")
    )


def validate_fail_open_exceptions(
    repo_root: Path,
    *,
    allowlist_path: Path | None = None,
    as_of: date | None = None,
) -> list[str]:
    repo_root = repo_root.resolve()
    allowlist_path = allowlist_path or (repo_root / ALLOWLIST_PATH)
    as_of = as_of or date.today()
    errors: list[str] = []

    try:
        policy = json.loads(allowlist_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return [f"{allowlist_path}: unable to load fail-open allowlist: {exc}"]
    entries = policy.get("exceptions") if isinstance(policy, dict) else None
    if not isinstance(entries, list):
        return [f"{allowlist_path}: expected an 'exceptions' array"]

    allowed: dict[tuple[str, str, str, str, str], dict[str, Any]] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            errors.append(f"{allowlist_path}: each exception entry must be an object")
            continue
        key = _exception_key(entry)
        if any(not value for value in key):
            errors.append(f"{allowlist_path}: exception entry is missing its exact match fields")
            continue
        if key in allowed:
            errors.append(f"{allowlist_path}: duplicate exception entry for {key}")
            continue
        allowed[key] = entry

        for field in ("rationale", "owner"):
            if not isinstance(entry.get(field), str) or not entry[field].strip():
                errors.append(f"{allowlist_path}: {key} requires a non-empty {field}")
        try:
            expiry = date.fromisoformat(str(entry.get("expires", "")))
            if expiry <= as_of:
                errors.append(f"{allowlist_path}: {key} expired on {expiry.isoformat()}")
        except ValueError:
            errors.append(f"{allowlist_path}: {key} requires an ISO YYYY-MM-DD expiry")
        if not isinstance(entry.get("count"), int) or entry["count"] < 1:
            errors.append(f"{allowlist_path}: {key} requires a positive integer count")

    found: Counter[tuple[str, str, str, str, str]] = Counter()
    for workflow in sorted((repo_root / WORKFLOWS_DIR).glob("*.y*ml")):
        if not _is_gate_workflow(workflow):
            continue
        relative_path = _workflow_path(repo_root, workflow)
        try:
            exceptions = _exceptions_in_workflow(workflow, relative_path)
        except Exception as exc:
            errors.append(f"{relative_path}: unable to inspect workflow exceptions: {exc}")
            continue
        for exception in exceptions:
            key = _exception_key(exception)
            found[key] += 1
            if key not in allowed:
                errors.append(
                    f"{relative_path}: unclassified {exception['kind']} in "
                    f"{exception['job']}/{exception['step']}: {exception['match']}"
                )

    for key, entry in allowed.items():
        actual = found[key]
        if actual != entry.get("count"):
            errors.append(
                f"{key[0]}: allowlist expects {entry.get('count')} occurrence(s), found {actual}"
            )

    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    parser.add_argument("--allowlist", type=Path)
    parser.add_argument("--as-of", type=date.fromisoformat, help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    errors = validate_fail_open_exceptions(
        args.repo_root,
        allowlist_path=args.allowlist,
        as_of=args.as_of,
    )
    if errors:
        for error in errors:
            print(f"::error::{error}", file=sys.stderr)
        return 1
    workflow_files = [
        path
        for path in (args.repo_root.resolve() / WORKFLOWS_DIR).glob("*.y*ml")
        if _is_gate_workflow(path)
    ]
    exception_count = sum(
        len(_exceptions_in_workflow(path, _workflow_path(args.repo_root.resolve(), path)))
        for path in workflow_files
    )
    print(
        "Gate workflow failure-tolerance inventory passed: "
        f"{exception_count} classified exception(s) across {len(workflow_files)} gate workflow(s)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
