#!/usr/bin/env python3
"""Reject newly introduced Git blobs larger than the repository file-size limit."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

MAX_FILE_SIZE_BYTES = 5 * 1024 * 1024
REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST_PATH = REPOSITORY_ROOT / ".github" / "large_file_allowlist.json"


def _git(*args: str, cwd: Path = REPOSITORY_ROOT) -> bytes:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=False,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(
            result.stderr.decode("utf-8", errors="replace").strip()
            or f"git {' '.join(args)} failed with exit code {result.returncode}"
        )
    return result.stdout


def _validate_commit(ref: str, cwd: Path) -> None:
    _git("rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}", cwd=cwd)


def _blob_size(ref: str, path: str, cwd: Path) -> int | None:
    object_id = subprocess.run(
        ["git", "rev-parse", "--verify", "--quiet", f"{ref}:{path}"],
        cwd=cwd,
        check=False,
        capture_output=True,
    )
    if object_id.returncode:
        return None
    size = _git("cat-file", "-s", object_id.stdout.decode("ascii").strip(), cwd=cwd)
    return int(size.strip())


def _changed_paths(base: str, head: str, cwd: Path) -> list[tuple[str, str, str]]:
    output = _git(
        "diff",
        "--name-status",
        "-z",
        "--find-renames",
        "--diff-filter=AMRTC",
        f"{base}...{head}",
        cwd=cwd,
    )
    fields = output.split(b"\0")
    changes: list[tuple[str, str, str]] = []
    index = 0
    while index < len(fields) and fields[index]:
        status = fields[index].decode("ascii")
        index += 1
        if status[0] in {"R", "C"}:
            old_path = fields[index].decode("utf-8", errors="surrogateescape")
            new_path = fields[index + 1].decode("utf-8", errors="surrogateescape")
            index += 2
        else:
            new_path = fields[index].decode("utf-8", errors="surrogateescape")
            old_path = new_path
            index += 1
        changes.append((status[0], old_path, new_path))
    return changes


def _load_allowlist(path: Path = ALLOWLIST_PATH) -> set[str]:
    try:
        entries = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"Unable to read large-file allowlist {path}: {exc}") from exc
    if not isinstance(entries, list) or any(
        not isinstance(entry, str) or not entry for entry in entries
    ):
        raise ValueError("Large-file allowlist must be a JSON array of exact non-empty paths")
    if len(entries) != len(set(entries)):
        raise ValueError("Large-file allowlist contains duplicate paths")
    return set(entries)


def find_oversized_additions(
    base: str,
    head: str,
    *,
    cwd: Path = REPOSITORY_ROOT,
    allowlist: set[str] | None = None,
    max_bytes: int = MAX_FILE_SIZE_BYTES,
) -> list[tuple[str, int]]:
    """Return paths newly exceeding the limit between two commits."""
    _validate_commit(base, cwd)
    _validate_commit(head, cwd)
    allowed_paths = _load_allowlist() if allowlist is None else allowlist
    oversized: list[tuple[str, int]] = []

    for status, old_path, new_path in _changed_paths(base, head, cwd):
        if new_path in allowed_paths:
            continue
        new_size = _blob_size(head, new_path, cwd)
        if new_size is None or new_size <= max_bytes:
            continue
        if status in {"A", "R", "C", "T"}:
            oversized.append((new_path, new_size))
            continue
        old_size = _blob_size(base, old_path, cwd)
        if old_size is None or old_size <= max_bytes:
            oversized.append((new_path, new_size))

    return oversized


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, help="Base commit SHA")
    parser.add_argument("--head", required=True, help="Pull request head commit SHA")
    args = parser.parse_args(argv)

    try:
        oversized = find_oversized_additions(args.base, args.head)
    except (RuntimeError, ValueError) as exc:
        print(f"::error::File-size policy could not be evaluated: {exc}", file=sys.stderr)
        return 2

    if oversized:
        print(f"::error::New or enlarged files may not exceed {MAX_FILE_SIZE_BYTES} bytes (5 MiB):")
        for path, size in oversized:
            print(f"  {path}: {size} bytes")
        print("Add an exact, reviewed path to .github/large_file_allowlist.json only when justified.")
        return 1

    print("File-size policy passed: no newly introduced files exceed 5 MiB.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
