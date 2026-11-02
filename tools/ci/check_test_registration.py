#!/usr/bin/env python3
"""Fail when changed C++ test sources are absent from a configured CMake build."""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Iterable


_AGGREGATE_TEST_TARGETS = {"themis_tests", "themis_tests_critical"}
_SHARED_TEST_SUPPORT_SOURCES = {"tests/test_gtest_main.cpp"}


def _normalized(path: Path) -> str:
    return os.path.normcase(str(path.resolve()))


def _test_sources_from_changes(changed_files: Path, repository_root: Path) -> list[Path]:
    tests_root = (repository_root / "tests").resolve()
    sources: set[Path] = set()
    for line in changed_files.read_text(encoding="utf-8").splitlines():
        value = line.strip().replace("\\", "/")
        if not value or not value.startswith("tests/") or not value.endswith(".cpp"):
            continue
        if not Path(value).name.startswith("test_"):
            continue

        source = (repository_root / Path(value)).resolve()
        try:
            source.relative_to(tests_root)
        except ValueError as exc:
            raise ValueError(f"test source is outside tests/: {value}") from exc
        if not source.is_file():
            raise ValueError(f"changed test source does not exist: {value}")
        sources.add(source)
    return sorted(sources)


def _compiled_sources(compilation_database: Path, repository_root: Path) -> set[str]:
    entries = json.loads(compilation_database.read_text(encoding="utf-8"))
    if not isinstance(entries, list):
        raise ValueError("compilation database root must be a JSON array")

    compiled: set[str] = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("file"), str):
            raise ValueError(f"invalid compilation database entry at index {index}")
        source = Path(entry["file"])
        if not source.is_absolute():
            directory = Path(entry.get("directory", repository_root))
            if not directory.is_absolute():
                directory = repository_root / directory
            source = directory / source
        compiled.add(_normalized(source))
    return compiled


def _compiled_targets(
    compilation_database: Path, repository_root: Path
) -> dict[str, set[str]]:
    entries = json.loads(compilation_database.read_text(encoding="utf-8"))
    if not isinstance(entries, list):
        raise ValueError("compilation database root must be a JSON array")

    target_pattern = re.compile(r"(?:^|[/\\])CMakeFiles[/\\]([^/\\]+)\.dir(?:[/\\]|$)")
    targets_by_source: dict[str, set[str]] = {}
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("file"), str):
            raise ValueError(f"invalid compilation database entry at index {index}")

        source = Path(entry["file"])
        if not source.is_absolute():
            directory = Path(entry.get("directory", repository_root))
            if not directory.is_absolute():
                directory = repository_root / directory
            source = directory / source

        output = str(entry.get("output", ""))
        command = str(entry.get("command", ""))
        target_match = target_pattern.search(output) or target_pattern.search(command)
        if target_match:
            targets_by_source.setdefault(_normalized(source), set()).add(target_match.group(1))

    return targets_by_source


def find_unregistered_sources(
    repository_root: Path,
    compilation_database: Path,
    changed_files: Path,
) -> list[Path]:
    """Return changed test sources missing from CMake's compilation database."""
    root = repository_root.resolve()
    changed_tests = _test_sources_from_changes(changed_files, root)
    compiled = _compiled_sources(compilation_database, root)
    return [source for source in changed_tests if _normalized(source) not in compiled]


def find_duplicate_target_sources(
    repository_root: Path,
    compilation_database: Path,
    changed_files: Path,
) -> dict[Path, list[str]]:
    """Find changed test sources registered in multiple non-aggregate CMake targets."""
    root = repository_root.resolve()
    compiled = _compiled_sources(compilation_database, root)
    targets = _compiled_targets(compilation_database, root)
    duplicates: dict[Path, list[str]] = {}
    for source in _test_sources_from_changes(changed_files, root):
        normalized_source = _normalized(source)
        if normalized_source not in compiled:
            continue
        source_targets = targets.get(normalized_source, set())
        if not source_targets:
            duplicates[source] = ["<unresolved CMake target>"]
            continue
        if source.relative_to(root).as_posix() in _SHARED_TEST_SUPPORT_SOURCES:
            # The shared GTest runner is intentionally compiled into each test executable.
            continue
        focused_targets = sorted(
            target
            for target in source_targets
            if target not in _AGGREGATE_TEST_TARGETS
        )
        if len(focused_targets) > 1:
            duplicates[source] = focused_targets
    return duplicates


def build_test_source_inventory(
    repository_root: Path, compilation_database: Path
) -> dict[str, object]:
    """Describe test source coverage for one configured CMake build profile."""
    root = repository_root.resolve()
    compiled_sources = _compiled_sources(compilation_database, root)
    targets_by_source = _compiled_targets(compilation_database, root)
    test_sources = sorted(
        (source for source in (root / "tests").rglob("test_*.cpp") if source.is_file()),
        key=lambda source: source.relative_to(root).as_posix(),
    )
    sources: list[dict[str, object]] = []
    not_in_profile = 0
    unknown_target_sources = 0
    multiple_focused_targets = 0

    for source in test_sources:
        relative_source = source.relative_to(root).as_posix()
        normalized_source = _normalized(source)
        targets = sorted(targets_by_source.get(normalized_source, set()))
        if normalized_source not in compiled_sources:
            not_in_profile += 1
            status = "not_in_profile"
        else:
            status = "compiled"
            if not targets:
                unknown_target_sources += 1
            focused_targets = [
                target for target in targets if target not in _AGGREGATE_TEST_TARGETS
            ]
            if len(focused_targets) > 1:
                multiple_focused_targets += 1

        sources.append(
            {
                "path": relative_source,
                "status": status,
                "targets": targets,
            }
        )

    return {
        "compilation_database": str(compilation_database.resolve()),
        "build_profile": compilation_database.parent.name,
        "platform": sys.platform,
        "summary": {
            "total_test_sources": len(test_sources),
            "compiled_sources": len(test_sources) - not_in_profile,
            "not_in_profile_sources": not_in_profile,
            "unknown_target_sources": unknown_target_sources,
            "multiple_focused_target_sources": multiple_focused_targets,
        },
        "sources": sources,
    }


def find_added_test_source_guards(cmake_diff: Path) -> list[str]:
    """Return added CMake lines that make test source registration existence-optional."""
    command_pattern = re.compile(r"^\s*(?:if|elseif)\s*\(", re.IGNORECASE)
    guard_pattern = re.compile(
        r"\b(?:if|elseif)\s*\(.*\bEXISTS\b.*"
        r"\btest_[A-Za-z0-9_.-]+\.(?:cpp|cc|cxx|c)\b",
        re.IGNORECASE | re.DOTALL,
    )
    diff_lines = re.compile(
        r"^(?P<kind>[ +])(?P<text>.*)$",
        re.IGNORECASE,
    )
    findings: list[str] = []
    hunk: list[tuple[str, bool]] = []

    def scan_hunk(lines: list[tuple[str, bool]]) -> None:
        index = 0
        while index < len(lines):
            text, added = lines[index]
            if not command_pattern.match(text):
                index += 1
                continue

            expression = [text]
            has_added_line = added
            depth = text.count("(") - text.count(")")
            index += 1
            while depth > 0 and index < len(lines):
                text, added = lines[index]
                expression.append(text)
                has_added_line = has_added_line or added
                depth += text.count("(") - text.count(")")
                index += 1

            command = "\n".join(expression)
            if has_added_line and guard_pattern.search(command):
                findings.append(" ".join(part.strip() for part in expression))

    for line in cmake_diff.read_text(encoding="utf-8").splitlines():
        if line.startswith(("+++", "---")):
            continue
        if line.startswith("@@"):
            scan_hunk(hunk)
            hunk = []
            continue
        match = diff_lines.match(line)
        if match:
            hunk.append((match.group("text"), match.group("kind") == "+"))
    scan_hunk(hunk)
    return findings


def find_deregistered_sources(
    cmake_diff: Path,
    compilation_database: Path,
    repository_root: Path,
) -> list[str]:
    """Find existing test sources removed from CMake and absent from the configured build."""
    root = repository_root.resolve()
    compiled = _compiled_sources(compilation_database, root)
    source_pattern = re.compile(
        r"(?:[A-Za-z0-9_.$/{\\}-]+[\\/])?test_[A-Za-z0-9_.-]+\.(?:cpp|cc|cxx|c)\b",
        re.IGNORECASE,
    )
    cmake_file: Path | None = None
    missing: set[str] = set()

    for line in cmake_diff.read_text(encoding="utf-8").splitlines():
        if line.startswith("diff --git a/"):
            old_path = line.split(" ", 3)[2][2:]
            cmake_file = root / Path(old_path)
            continue
        if cmake_file is None or not line.startswith("-") or line.startswith("---"):
            continue
        if cmake_file.name != "CMakeLists.txt":
            continue

        for match in source_pattern.finditer(line[1:]):
            reference = match.group(0).strip('"').replace("\\", "/")
            for variable in ("CMAKE_CURRENT_LIST_DIR", "CMAKE_CURRENT_SOURCE_DIR"):
                reference = reference.replace("${" + variable + "}", str(cmake_file.parent))
            for variable in ("CMAKE_SOURCE_DIR", "THEMIS_ROOT_DIR"):
                reference = reference.replace("${" + variable + "}", str(root))
            if "${" in reference:
                continue

            source = Path(reference)
            if not source.is_absolute():
                source = cmake_file.parent / source
            source = source.resolve()
            try:
                relative_source = source.relative_to(root).as_posix()
            except ValueError:
                continue
            if source.is_file() and _normalized(source) not in compiled:
                missing.add(relative_source)

    return sorted(missing)


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository-root", type=Path, default=Path.cwd())
    parser.add_argument("--compile-commands", type=Path, required=True)
    parser.add_argument("--changed-files", type=Path, required=True)
    parser.add_argument("--cmake-diff", type=Path, required=True)
    parser.add_argument(
        "--inventory-report",
        type=Path,
        help="write a JSON inventory of all tests/ test_*.cpp files for this build profile",
    )
    args = parser.parse_args(argv)

    try:
        if args.inventory_report:
            inventory = build_test_source_inventory(
                args.repository_root, args.compile_commands
            )
            args.inventory_report.parent.mkdir(parents=True, exist_ok=True)
            args.inventory_report.write_text(
                json.dumps(inventory, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
        missing = find_unregistered_sources(
            args.repository_root, args.compile_commands, args.changed_files
        )
        duplicate_targets = find_duplicate_target_sources(
            args.repository_root, args.compile_commands, args.changed_files
        )
        guards = find_added_test_source_guards(args.cmake_diff)
        deregistered = find_deregistered_sources(
            args.cmake_diff, args.compile_commands, args.repository_root
        )
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"test registration check failed: {exc}", file=sys.stderr)
        return 2

    if guards:
        print("New test-source existence guards can silently omit CMake registrations:")
        for guard in guards:
            print(f"  - {guard}")
        return 1

    if deregistered:
        print("Existing test sources were removed from CMake and are absent from this build:")
        for source in deregistered:
            print(f"  - {source}")
        print("Retain a target registration or document and review the intentional exclusion.")
        return 1

    if missing:
        print("Changed C++ test sources are absent from the configured CMake build:")
        for source in missing:
            print(f"  - {source.relative_to(args.repository_root.resolve()).as_posix()}")
        print(
            "Register each source in a CMake test target; configuration-gated sources "
            "require an explicit CI exclusion policy."
        )
        return 1

    if duplicate_targets:
        print("Changed test sources have duplicate or unresolved focused-target mappings:")
        for source, targets in sorted(duplicate_targets.items()):
            print(
                f"  - {source.relative_to(args.repository_root.resolve()).as_posix()}: "
                f"{', '.join(targets)}"
            )
        print(
            "Use at most one focused target per source; CMake target metadata must be "
            "available. The aggregate themis_tests target is excluded."
        )
        return 1

    print("All changed C++ test sources are present in the CMake compilation database.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
