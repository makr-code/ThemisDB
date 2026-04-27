#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml


ACTIVE_WORKFLOWS_DIR = ".github/workflows/"
QUARANTINE_WORKFLOWS_DIR = ".github/no_workflows/"
GUIDELINES_FILE = ".github/WORKFLOW_GUIDELINES.md"
REGISTRY_FILE = ".github/WORKFLOW_REGISTRY.md"
ALLOWED_BROAD_PATH_PREFIXES = {
    ".github/workflows/",
    ".github/no_workflows/",
    ".github/actions/",
    ".github/scripts/",
    "tools/ci/",
    "tools/tests/",
}
FORBIDDEN_EXACT_PATHS = {
    "**",
    "**/*",
    "src/**",
    "include/**",
    "tests/**",
    "benchmarks/**",
    "docs/**",
    "cmake/**",
    "tools/**",
    "*.md",
    "**/*.md",
}


def _normalize(path: str) -> str:
    return path.replace("\\", "/").strip()


def _load_workflow(path: Path) -> dict:
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    if not isinstance(data, dict):
        raise ValueError(f"{path}: workflow root must be a mapping")
    return data


def _workflow_on(data: dict):
    if "on" in data:
        return data["on"]
    return data.get(True)


def _event_config(on_config, event_name: str):
    if isinstance(on_config, dict):
        if event_name in on_config:
            return on_config[event_name]
        return None
    if isinstance(on_config, list):
        return {} if event_name in on_config else None
    if isinstance(on_config, str):
        return {} if on_config == event_name else None
    return None


def _has_event(on_config, event_name: str) -> bool:
    if isinstance(on_config, dict):
        return event_name in on_config
    if isinstance(on_config, list):
        return event_name in on_config
    if isinstance(on_config, str):
        return on_config == event_name
    return False


def _extract_paths(event_config) -> list[str]:
    if isinstance(event_config, dict):
        raw_paths = event_config.get("paths", [])
        if isinstance(raw_paths, list):
            return [_normalize(p) for p in raw_paths]
    return []


def _extract_branches(event_config) -> list[str]:
    if isinstance(event_config, dict):
        raw_branches = event_config.get("branches", [])
        if isinstance(raw_branches, list):
            return [_normalize(b) for b in raw_branches]
    return []


def _is_broad_path(path_pattern: str) -> bool:
    if path_pattern in FORBIDDEN_EXACT_PATHS:
        return True
    if path_pattern.startswith("!"):
        path_pattern = path_pattern[1:]
    if path_pattern in FORBIDDEN_EXACT_PATHS:
        return True
    if any(path_pattern.startswith(prefix) for prefix in ALLOWED_BROAD_PATH_PREFIXES):
        return False
    if path_pattern.count("/") == 1 and path_pattern.endswith("/**"):
        return True
    return False


def _validate_permissions(data: dict, workflow_path: str, violations: list[str]) -> None:
    permissions = data.get("permissions")
    if permissions in (None, ""):
        violations.append(f"{workflow_path}: missing explicit permissions block")


def _validate_concurrency(data: dict, workflow_path: str, violations: list[str]) -> None:
    concurrency = data.get("concurrency")
    if not isinstance(concurrency, dict):
        violations.append(f"{workflow_path}: pull_request workflows require concurrency with cancel-in-progress")
        return
    if concurrency.get("cancel-in-progress") is not True:
        violations.append(f"{workflow_path}: pull_request workflows must set cancel-in-progress: true")


def validate_workflow_file(path: Path) -> list[str]:
    workflow_path = _normalize(str(path))
    try:
        data = _load_workflow(path)
    except Exception as exc:
        return [f"{workflow_path}: failed to parse workflow YAML: {exc}"]

    violations: list[str] = []
    on_config = _workflow_on(data)
    if on_config is None:
        return [f"{workflow_path}: missing 'on' trigger configuration"]

    _validate_permissions(data, workflow_path, violations)

    if _has_event(on_config, "pull_request"):
        pull_request = _event_config(on_config, "pull_request")
        branches = _extract_branches(pull_request)
        paths = _extract_paths(pull_request)
        if not branches:
            violations.append(f"{workflow_path}: pull_request trigger must define branches")
        if not paths:
            violations.append(f"{workflow_path}: pull_request trigger must define paths")
        broad_paths = [p for p in paths if _is_broad_path(p)]
        if broad_paths:
            violations.append(
                f"{workflow_path}: pull_request paths too broad: {', '.join(sorted(broad_paths))}"
            )
        _validate_concurrency(data, workflow_path, violations)

    return violations


def parse_name_status(diff_file: Path) -> list[dict[str, str]]:
    entries: list[dict[str, str]] = []
    for raw_line in diff_file.read_text(encoding="utf-8").splitlines():
        if not raw_line.strip():
            continue
        parts = raw_line.split("\t")
        status = parts[0]
        if status.startswith("R") and len(parts) >= 3:
            entries.append({
                "status": status,
                "old": _normalize(parts[1]),
                "new": _normalize(parts[2]),
            })
        elif len(parts) >= 2:
            entries.append({
                "status": status,
                "path": _normalize(parts[1]),
            })
    return entries


def _changed_paths(entries: list[dict[str, str]]) -> set[str]:
    changed: set[str] = set()
    for entry in entries:
        if "path" in entry:
            changed.add(entry["path"])
        else:
            changed.add(entry["old"])
            changed.add(entry["new"])
    return changed


def _reactivated_targets(entries: list[dict[str, str]], repo_root: Path) -> set[str]:
    targets: set[str] = set()
    for entry in entries:
        if entry["status"].startswith("R"):
            old_path = entry.get("old", "")
            new_path = entry.get("new", "")
            if old_path.startswith(QUARANTINE_WORKFLOWS_DIR) and new_path.startswith(ACTIVE_WORKFLOWS_DIR):
                targets.add(new_path)
            continue

        path = entry.get("path", "")
        if entry["status"] == "A" and path.startswith(ACTIVE_WORKFLOWS_DIR):
            quarantine_peer = repo_root / QUARANTINE_WORKFLOWS_DIR / Path(path).name
            if quarantine_peer.exists():
                targets.add(path)
    return targets


def _active_workflow_changes(entries: list[dict[str, str]]) -> set[str]:
    targets: set[str] = set()
    for entry in entries:
        if entry["status"].startswith("R"):
            new_path = entry.get("new", "")
            if new_path.startswith(ACTIVE_WORKFLOWS_DIR):
                targets.add(new_path)
            continue
        path = entry.get("path", "")
        if path.startswith(ACTIVE_WORKFLOWS_DIR):
            targets.add(path)
    return targets


def evaluate(diff_entries: list[dict[str, str]], repo_root: Path) -> list[str]:
    violations: list[str] = []
    changed_paths = _changed_paths(diff_entries)
    active_workflow_changes = _active_workflow_changes(diff_entries)

    for rel_path in sorted(active_workflow_changes):
        workflow_path = repo_root / rel_path
        if workflow_path.exists():
            violations.extend(validate_workflow_file(workflow_path))

    reactivated = _reactivated_targets(diff_entries, repo_root)
    if reactivated:
        if GUIDELINES_FILE not in changed_paths:
            violations.append("Workflow reactivation requires updating .github/WORKFLOW_GUIDELINES.md in the same PR")
        if REGISTRY_FILE not in changed_paths:
            violations.append("Workflow reactivation requires updating .github/WORKFLOW_REGISTRY.md in the same PR")

    for rel_path in sorted(reactivated):
        workflow_path = repo_root / rel_path
        if not workflow_path.exists():
            violations.append(f"{rel_path}: reactivated workflow file missing after rename/add")
            continue
        data = _load_workflow(workflow_path)
        on_config = _workflow_on(data)
        if not _has_event(on_config, "workflow_dispatch"):
            violations.append(f"{rel_path}: reactivated workflows must include workflow_dispatch for first rollout")

    return violations


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Enforce hard workflow reactivation boundaries.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--diff-file", required=True, help="git diff --name-status output file")
    args = parser.parse_args(argv)

    repo_root = Path(args.repo_root).resolve()
    diff_entries = parse_name_status(Path(args.diff_file))
    violations = evaluate(diff_entries, repo_root)

    if violations:
        print("Workflow boundary check failed:", file=sys.stderr)
        for violation in violations:
            print(f" - {violation}", file=sys.stderr)
        return 1

    print("Workflow boundary check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())