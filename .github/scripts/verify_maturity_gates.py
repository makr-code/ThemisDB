#!/usr/bin/env python3
"""Verify maturity evidence entries from the repository manifest."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def _safe_artifact_path(repo_root: Path, artifact_path: Any) -> Path:
    if not isinstance(artifact_path, str) or not artifact_path.strip():
        raise ValueError("artifact_path must be a non-empty repository-relative path")
    candidate = (repo_root / artifact_path).resolve()
    if not candidate.is_relative_to(repo_root):
        raise ValueError("artifact_path resolves outside the repository")
    return candidate


def _artifact_modified_at(path: Path, repo_root: Path) -> datetime:
    relative_path = path.relative_to(repo_root).as_posix()
    result = subprocess.run(
        ["git", "-C", str(repo_root), "log", "-1", "--format=%ct", "--", relative_path],
        capture_output=True,
        check=False,
        text=True,
    )
    if result.returncode == 0 and result.stdout.strip().isdigit():
        return datetime.fromtimestamp(int(result.stdout.strip()), tz=timezone.utc)
    if (repo_root / ".git").exists():
        raise ValueError(f"unable to determine Git modification time for {relative_path}")
    return datetime.fromtimestamp(path.stat().st_mtime, tz=timezone.utc)


def _section_text(contents: str, marker: str) -> str:
    lines = contents.splitlines()
    marker_index = next((i for i, line in enumerate(lines) if marker in line), None)
    if marker_index is None:
        return ""
    heading = re.match(r"^(#{1,6})\s", lines[marker_index])
    if heading is None:
        return lines[marker_index]
    level = len(heading.group(1))
    for index in range(marker_index + 1, len(lines)):
        next_heading = re.match(r"^(#{1,6})\s", lines[index])
        if next_heading and len(next_heading.group(1)) <= level:
            return "\n".join(lines[marker_index:index])
    return "\n".join(lines[marker_index:])


def _split_rules(rule: str) -> list[str]:
    return [part.strip() for part in rule.split(" AND ") if part.strip()]


def _evaluate_rule(
    rule: str,
    entry: dict[str, Any],
    repo_root: Path,
    now: datetime,
) -> tuple[bool, str]:
    artifact_path = _safe_artifact_path(repo_root, entry.get("artifact_path"))
    if not artifact_path.is_file():
        return False, f"artifact does not exist: {entry.get('artifact_path')}"

    contents: str | None = None
    for clause in _split_rules(rule):
        if clause == "file_exists":
            continue

        freshness = re.fullmatch(r"file_modified_within\((\d+)_days\)", clause)
        if freshness:
            age_days = (now - _artifact_modified_at(artifact_path, repo_root)).total_seconds() / 86400
            limit = int(freshness.group(1))
            if age_days > limit:
                return False, f"artifact is {age_days:.1f} days old; maximum is {limit} days"
            continue

        if clause == "contains_gate_id":
            if contents is None:
                contents = artifact_path.read_text(encoding="utf-8")
            gate_id = entry.get("gate_id")
            if not isinstance(gate_id, str) or not gate_id:
                return False, "contains_gate_id requires a non-empty gate_id"
            if gate_id not in contents:
                return False, f"artifact does not contain gate ID {gate_id}"
            continue

        section_match = re.fullmatch(r"section_contains\('([^']+)'\)", clause)
        if section_match:
            if contents is None:
                contents = artifact_path.read_text(encoding="utf-8")
            marker = entry.get("section_marker")
            if not isinstance(marker, str) or not marker:
                return False, "section_contains requires a non-empty section_marker"
            if section_match.group(1) not in _section_text(contents, marker):
                return False, f"section {marker!r} does not contain {section_match.group(1)!r}"
            continue

        contains_match = re.fullmatch(r"contains\('((?:\\.|[^'])*)'\)", clause)
        if contains_match:
            if contents is None:
                contents = artifact_path.read_text(encoding="utf-8")
            pattern = contains_match.group(1)
            if re.search(pattern, contents, flags=re.MULTILINE) is None:
                return False, f"artifact does not match pattern {pattern!r}"
            continue

        return False, f"unsupported validation rule: {clause}"

    return True, "all validation rules passed"


def evaluate_manifest(
    manifest: Any,
    repo_root: Path,
    *,
    groups: list[str] | None = None,
    now: datetime | None = None,
) -> dict[str, Any]:
    checked_at = now or datetime.now(timezone.utc)
    report: dict[str, Any] = {
        "checked_at": checked_at.isoformat(),
        "pass": False,
        "groups": {},
        "checks": [],
        "errors": [],
    }

    if not isinstance(manifest, dict) or not isinstance(manifest.get("gates"), dict):
        report["errors"].append("manifest must contain a gates object")
        return report

    available_groups = manifest["gates"]
    selected_groups = groups if groups is not None else sorted(available_groups)
    unknown_groups = sorted(set(selected_groups) - set(available_groups))
    report["errors"].extend(f"unknown gate group: {name}" for name in unknown_groups)

    for group_name in selected_groups:
        entries = available_groups.get(group_name)
        if not isinstance(entries, list):
            report["errors"].append(f"gate group {group_name!r} must be a list")
            continue

        group_counts = {"pass": 0, "fail": 0, "pending": 0}
        report["groups"][group_name] = group_counts
        for index, entry in enumerate(entries):
            label = None
            if isinstance(entry, dict):
                label = entry.get("gate_id") or entry.get("module_name")
            check: dict[str, Any] = {
                "group": group_name,
                "id": label or f"entry-{index + 1}",
                "status": "FAIL",
                "details": "",
            }

            if not isinstance(entry, dict):
                check["details"] = "gate entry must be an object"
            else:
                declared_status = str(entry.get("status", "")).upper()
                if declared_status == "PENDING":
                    check.update(status="PENDING", details="verification is not yet implemented or evidence is pending")
                elif declared_status != "PASS":
                    check["details"] = f"unsupported or non-passing manifest status: {declared_status or '(missing)'}"
                else:
                    rule = entry.get("validation_rule")
                    if not isinstance(rule, str) or not rule.strip():
                        check["details"] = "PASS entry must declare validation_rule"
                    else:
                        try:
                            passed, details = _evaluate_rule(rule, entry, repo_root, checked_at)
                            check.update(status="PASS" if passed else "FAIL", details=details)
                        except (OSError, ValueError, re.error) as error:
                            check["details"] = str(error)

            group_counts[check["status"].lower()] += 1
            report["checks"].append(check)

    report["pass"] = not report["errors"] and all(
        check["status"] != "FAIL" for check in report["checks"]
    )
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Verify maturity evidence manifest entries.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument(
        "--manifest",
        default="docs/governance/MATURITY_EVIDENCE_MANIFEST.json",
        help="Manifest path, absolute or relative to the repository root.",
    )
    parser.add_argument("--output", required=True, help="Path for the JSON verification report.")
    parser.add_argument(
        "--group",
        action="append",
        dest="groups",
        help="Gate group to verify; may be supplied more than once. Defaults to all groups.",
    )
    args = parser.parse_args(argv)

    repo_root = Path(args.repo_root).resolve()
    manifest_path = Path(args.manifest)
    if not manifest_path.is_absolute():
        manifest_path = repo_root / manifest_path
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        report = evaluate_manifest(manifest, repo_root, groups=args.groups)
    except (OSError, json.JSONDecodeError) as error:
        report = {
            "checked_at": datetime.now(timezone.utc).isoformat(),
            "pass": False,
            "groups": {},
            "checks": [],
            "errors": [f"unable to load manifest: {error}"],
        }

    output_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    for error in report["errors"]:
        print(f"ERROR: {error}")
    for check in report["checks"]:
        print(f"{check['status']}: {check['group']}/{check['id']}: {check['details']}")
    print(f"Report: {output_path}")
    print(f"Result: {'PASS' if report['pass'] else 'FAIL'}")
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
