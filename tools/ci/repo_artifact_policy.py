#!/usr/bin/env python3
"""Enforce the repository's large-file policy and create artifact inventories."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


MAX_FILE_BYTES = 5 * 1024 * 1024
DEFAULT_ALLOWLIST = Path("tools/ci/large_file_allowlist.json")
DOXYGEN_ROOTS = (
    "ai_context/developer_llm_wiki/module_doxygen_artifacts/",
    "ai_context/developer_llm_wiki/module_doxygen_artifacts_batches/",
)
HISTORICAL_SCAN_ROOT = "docs/ARCHIVED/ai-working-history/"
ROOT_BENCHMARK_FILES = {
    "bench_access_coordinator_gates.exe.json",
    "bench_access_coordinator_gates.json",
}


def _git(root: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        ["git", "-C", str(root), *args],
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def _resolve_allowlist(path: Path) -> dict[str, str]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read allowlist {path}: {exc}") from exc

    files = data.get("files") if isinstance(data, dict) else None
    if not isinstance(files, dict):
        raise ValueError(f"{path}: expected an object in the 'files' field")

    normalized: dict[str, str] = {}
    for file_path, rationale in files.items():
        if not isinstance(file_path, str) or not file_path or "\\" in file_path:
            raise ValueError(f"{path}: allowlist paths must be non-empty repository-relative paths")
        if file_path.startswith("/") or ".." in Path(file_path).parts:
            raise ValueError(f"{path}: allowlist path is not repository-relative: {file_path}")
        if any(character in file_path for character in "*?["):
            raise ValueError(f"{path}: glob patterns are not allowed: {file_path}")
        if not isinstance(rationale, str) or not rationale.strip():
            raise ValueError(f"{path}: allowlist rationale is required for {file_path}")
        normalized[file_path] = rationale.strip()
    return normalized


def _added_paths(root: Path, base_ref: str, head_ref: str) -> list[str]:
    result = _git(
        root,
        "diff",
        "--no-renames",
        "--diff-filter=A",
        "--name-only",
        "-z",
        f"{base_ref}...{head_ref}",
    )
    return [os.fsdecode(path) for path in result.stdout.split(b"\0") if path]


def _blob_size(root: Path, revision: str, file_path: str) -> int:
    result = _git(root, "cat-file", "-s", f"{revision}:{file_path}")
    return int(result.stdout.strip())


def check_added_files(
    root: Path,
    base_ref: str,
    head_ref: str,
    allowlist_path: Path,
) -> int:
    try:
        if not allowlist_path.is_absolute():
            allowlist_path = root / allowlist_path
        allowlist = _resolve_allowlist(allowlist_path)
        paths = _added_paths(root, base_ref, head_ref)
    except (ValueError, subprocess.CalledProcessError) as exc:
        detail = exc.stderr.decode(errors="replace").strip() if isinstance(exc, subprocess.CalledProcessError) else str(exc)
        print(f"artifact policy check failed: {detail}", file=sys.stderr)
        return 2

    failures: list[str] = []
    for file_path in paths:
        try:
            size = _blob_size(root, head_ref, file_path)
        except (ValueError, subprocess.CalledProcessError) as exc:
            detail = exc.stderr.decode(errors="replace").strip() if isinstance(exc, subprocess.CalledProcessError) else str(exc)
            print(f"artifact policy check failed for {file_path}: {detail}", file=sys.stderr)
            return 2

        if size <= MAX_FILE_BYTES:
            continue
        rationale = allowlist.get(file_path)
        if rationale:
            print(f"ALLOW {file_path}: {size} bytes ({rationale})")
            continue
        failures.append(f"{file_path}: {size} bytes exceeds {MAX_FILE_BYTES} bytes")

    if failures:
        for failure in failures:
            print(f"::error::{failure}", file=sys.stderr)
        print(
            "New files larger than 5 MiB require a reviewed exact-path entry in "
            f"{allowlist_path.as_posix()}.",
            file=sys.stderr,
        )
        return 1

    print(f"Artifact size gate passed: checked {len(paths)} added file(s).")
    return 0


def _tree_ref(root: Path, ref: str) -> str:
    if ref != "INDEX":
        return ref
    return _git(root, "write-tree").stdout.decode().strip()


def _tree_entries(root: Path, ref: str) -> dict[str, int]:
    output = _git(root, "ls-tree", "-r", "-l", "-z", "--full-tree", ref).stdout
    entries: dict[str, int] = {}
    for entry in output.split(b"\0"):
        if not entry:
            continue
        metadata, raw_path = entry.split(b"\t", 1)
        fields = metadata.decode("ascii").split()
        if fields[1] == "blob":
            entries[os.fsdecode(raw_path)] = int(fields[3])
        elif fields[1] == "commit":
            entries[os.fsdecode(raw_path)] = 0
    return entries


def _classify(path: str) -> dict[str, str]:
    if any(path.startswith(root) for root in DOXYGEN_ROOTS):
        return {
            "classification": "generated Doxygen output",
            "owner": "Developer documentation maintainers",
            "proposed_disposition": (
                "Keep source summaries only; generate XML in the Doxygen workflow and retrieve "
                "the module-doxygen-xml artifact (21-day retention)."
            ),
        }
    if path.startswith(HISTORICAL_SCAN_ROOT) and Path(path).suffix.lower() in {".json", ".xml"}:
        return {
            "classification": "historical generated scan/report output",
            "owner": "AI working-history maintainers",
            "proposed_disposition": (
                "Remove generated raw reports from the active tree; the archived Git history is "
                "unchanged, and fresh GS3 outputs are retained as CI artifacts for 30 days."
            ),
        }
    if path in ROOT_BENCHMARK_FILES:
        return {
            "classification": "root-level generated benchmark result",
            "owner": "Benchmark maintainers",
            "proposed_disposition": (
                "Remove from source; publish fresh benchmark runs through existing CI benchmark "
                "artifacts (30-day retention)."
            ),
        }
    if path == "archive/moved_files.txt":
        return {
            "classification": "generated file-move report",
            "owner": "Repository hygiene maintainers",
            "proposed_disposition": "Remove; no active consumer reference was found.",
        }
    if path == "ai_working/gs3_scan_current.json":
        return {
            "classification": "generated GS3 scan result",
            "owner": "GS3/quality maintainers",
            "proposed_disposition": (
                "Remove the stale working-tree copy; fresh scans are uploaded by "
                "maintenance-issues.yml with 30-day retention."
            ),
        }
    if path.endswith(".pch"):
        return {
            "classification": "compiler-generated precompiled header",
            "owner": "Build maintainers",
            "proposed_disposition": "Remove; regenerate in the local build directory.",
        }
    if path == "artifacts/embedded_llm_stub.i":
        return {
            "classification": "compiler-generated preprocessed source",
            "owner": "LLM/build maintainers",
            "proposed_disposition": "Remove; regenerate from its source during local build/debug work.",
        }
    if path == "docs/reports/search_vcpkg_pin.txt":
        return {
            "classification": "generated search report",
            "owner": "Dependency maintenance",
            "proposed_disposition": "Remove; regenerate the local search report when needed.",
        }
    if path.endswith(".msi"):
        return {
            "classification": "downloaded third-party installer",
            "owner": "Developer tooling maintainers",
            "proposed_disposition": "Remove; obtain the installer from its upstream release.",
        }
    if path == "tools/bin/actionlint":
        return {
            "classification": "vendored developer-tool binary",
            "owner": "CI maintainers",
            "proposed_disposition": "Remove the unused binary; local workflow lint uses the documented container tool.",
        }
    if path.startswith("docs/compendium/output/") or path.startswith("docs/ThemisDB-"):
        return {
            "classification": "published documentation",
            "owner": "Documentation maintainers",
            "proposed_disposition": "Retain; these are linked, versioned documentation deliverables.",
        }
    return {
        "classification": "oversized tracked file (review required)",
        "owner": "Maintainers of the containing path",
        "proposed_disposition": "Retain only with documented consumer need; otherwise migrate to CI/release artifacts.",
    }


def _generated_groups(entries: dict[str, int]) -> list[dict[str, Any]]:
    group_specs = (
        (
            "doxygen_outputs",
            "generated Doxygen output",
            lambda path: any(path.startswith(root) for root in DOXYGEN_ROOTS)
            and Path(path).suffix.lower() != ".md",
            "Developer documentation maintainers",
            "Generate in CI and publish module-doxygen-xml artifacts with 21-day retention.",
        ),
        (
            "archived_scan_reports",
            "historical generated scan/report output",
            lambda path: path.startswith(HISTORICAL_SCAN_ROOT)
            and Path(path).suffix.lower() in {".json", ".xml"},
            "AI working-history maintainers",
            "Remove raw generated reports from the active tree; preserve Git history without rewriting it.",
        ),
        (
            "root_benchmark_outputs",
            "root-level generated benchmark result",
            lambda path: path in ROOT_BENCHMARK_FILES,
            "Benchmark maintainers",
            "Publish fresh benchmark runs through existing CI artifacts with 30-day retention.",
        ),
    )

    groups: list[dict[str, Any]] = []
    for group_id, classification, matches, owner, disposition in group_specs:
        files = [(path, size) for path, size in entries.items() if matches(path)]
        if not files:
            continue
        groups.append(
            {
                "id": group_id,
                "classification": classification,
                "owner": owner,
                "proposed_disposition": disposition,
                "file_count": len(files),
                "blob_bytes": sum(size for _, size in files),
                "sample_paths": sorted(path for path, _ in files)[:5],
            }
        )
    return groups


def _git_directory_bytes(root: Path) -> int:
    git_dir = Path(_git(root, "rev-parse", "--git-dir").stdout.decode().strip())
    if not git_dir.is_absolute():
        git_dir = root / git_dir
    return sum(path.stat().st_size for path in git_dir.rglob("*") if path.is_file())


def _snapshot(root: Path, ref: str) -> dict[str, Any]:
    tree_ref = _tree_ref(root, ref)
    entries = _tree_entries(root, tree_ref)
    oversized = []
    for path, size in sorted(entries.items()):
        if size <= MAX_FILE_BYTES:
            continue
        oversized.append(
            {
                "path": path,
                "blob_bytes": size,
                **_classify(path),
            }
        )
    return {
        "tree": tree_ref,
        "tracked_entries": len(entries),
        "tracked_blob_bytes": sum(entries.values()),
        "files_over_5_mib": len(oversized),
        "oversized_files": oversized,
        "generated_output_groups": _generated_groups(entries),
    }


def write_inventory(root: Path, before_ref: str, after_ref: str, output: Path) -> int:
    try:
        before = _snapshot(root, before_ref)
        after = _snapshot(root, after_ref)
        baseline_sha = _git(root, "rev-parse", before_ref).stdout.decode().strip()
        head_sha = _git(root, "rev-parse", "HEAD").stdout.decode().strip()
    except subprocess.CalledProcessError as exc:
        detail = exc.stderr.decode(errors="replace").strip()
        print(f"cannot generate inventory: {detail}", file=sys.stderr)
        return 2

    historical_paths = (
        "docs/ARCHIVED/ai-working-history/scan_tests.json",
        "archive/moved_files.txt",
        "bench_access_coordinator_gates.exe.json",
        "bench_access_coordinator_gates.json",
    )
    baseline_entries = _tree_entries(root, _tree_ref(root, before_ref))
    historical_examples = []
    for path in historical_paths:
        historical_examples.append(
            {
                "path": path,
                "present": path in baseline_entries,
                "blob_bytes": baseline_entries.get(path),
                **_classify(path),
            }
        )

    payload = {
        "schema_version": 1,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "threshold": {
            "operator": ">",
            "bytes": MAX_FILE_BYTES,
            "display": "5 MiB",
        },
        "size_basis": "Git blob size from git ls-tree; Git directory bytes are physical file sizes.",
        "baseline_commit_sha": baseline_sha,
        "head_commit_sha_at_capture": head_sha,
        "after_tree_ref": after["tree"],
        "git_directory_bytes_at_capture": _git_directory_bytes(root),
        "reproduction": (
            "python3 tools/ci/repo_artifact_policy.py inventory "
            "--before-ref HEAD --after-ref INDEX --output <path>"
        ),
        "inventory_note": (
            "The after tree is measured from the staged index before this inventory is written, "
            "so this JSON report itself is excluded from after-tree counts and bytes. Git history "
            "is not rewritten; deleting files from the current tree does not remove historical blobs."
        ),
        "historical_examples": historical_examples,
        "before": before,
        "after": after,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"Inventory written to {output}: {before['tracked_entries']} -> "
        f"{after['tracked_entries']} tracked entries; "
        f"{before['tracked_blob_bytes']} -> {after['tracked_blob_bytes']} blob bytes."
    )
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    check = subparsers.add_parser("check", help="reject oversized files added by a change")
    check.add_argument("--repo-root", type=Path, default=Path("."))
    check.add_argument("--base-ref", required=True)
    check.add_argument("--head-ref", default="HEAD")
    check.add_argument("--allowlist", type=Path, default=DEFAULT_ALLOWLIST)

    inventory = subparsers.add_parser("inventory", help="write a generated/oversized artifact inventory")
    inventory.add_argument("--repo-root", type=Path, default=Path("."))
    inventory.add_argument("--before-ref", default="HEAD")
    inventory.add_argument("--after-ref", default="INDEX")
    inventory.add_argument(
        "--output",
        type=Path,
        default=Path("audit/evidence/repo-hygiene/HYG-01-inventory-2026-11-09.json"),
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    root = args.repo_root.resolve()
    if args.command == "check":
        return check_added_files(root, args.base_ref, args.head_ref, args.allowlist)
    return write_inventory(root, args.before_ref, args.after_ref, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
