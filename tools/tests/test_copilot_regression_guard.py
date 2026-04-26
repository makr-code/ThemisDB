#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

_HERE = Path(__file__).resolve().parent
_TOOLS = _HERE.parent
_CI = _TOOLS / "ci"

for _p in (_TOOLS, _CI):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

import copilot_regression_guard as guard  # noqa: E402


def _write_repo(tmp_path: Path) -> Path:
    (tmp_path / "src" / "replication").mkdir(parents=True)
    (tmp_path / "include").mkdir(parents=True)
    (tmp_path / "tests").mkdir(parents=True)
    (tmp_path / "cmake").mkdir(parents=True)

    (tmp_path / "src" / "replication" / "crdt_types.cpp").write_text("int x = 1;\n", encoding="utf-8")

    (tmp_path / "include" / "themis_export.h").write_text(
        """
#pragma once
#ifdef _WIN32
#if defined(THEMIS_BASE_EXPORTS) || defined(THEMIS_TEST_BUILD)
#define THEMIS_BASE_API __declspec(dllexport)
#else
#define THEMIS_BASE_API __declspec(dllimport)
#endif
#else
#define THEMIS_BASE_API
#endif
""".strip()
        + "\n",
        encoding="utf-8",
    )

    (tmp_path / "tests" / "CMakeLists.txt").write_text(
        """
add_executable(test_replication_crdt_types
  test_replication_crdt_types.cpp
)
target_link_libraries(test_replication_crdt_types PRIVATE Threads::Threads)
target_compile_definitions(test_replication_crdt_types PRIVATE THEMIS_TEST_BUILD=1)
""".strip()
        + "\n",
        encoding="utf-8",
    )

    (tmp_path / "cmake" / "CMakeLists.txt").write_text(
        "target_compile_definitions(themis_core PRIVATE THEMIS_BASE_EXPORTS)\n",
        encoding="utf-8",
    )
    return tmp_path


def test_inventory_detects_test_only_target_and_probable_source(tmp_path: Path):
    repo = _write_repo(tmp_path)
    inventory, affected = guard.inventory_test_targets(repo, repo / "tests" / "CMakeLists.txt")

    assert len(inventory) == 1
    assert inventory[0].target == "test_replication_crdt_types"
    assert any(path.endswith("src/replication/crdt_types.cpp") for path in inventory[0].probable_sources)
    assert len(affected) == 1


def test_macro_validation_passes_for_expected_contract(tmp_path: Path):
    repo = _write_repo(tmp_path)
    failures = guard.validate_export_macros(repo)
    assert failures == []


def test_lnk_parser_suggests_related_source(tmp_path: Path):
    repo = _write_repo(tmp_path)
    source_index = guard._index_source_files(repo)
    log = repo / "build.log"
    log.write_text(
        'error LNK2019: unresolved external symbol "public: void CRDTTypes::merge(void)" referenced in function main\n',
        encoding="utf-8",
    )

    findings = guard.parse_lnk_errors(log, source_index)
    assert len(findings) == 1
    assert findings[0]["code"] == "LNK2019"
    assert "src/replication/crdt_types.cpp" in findings[0]["suggested_sources"]


def test_main_strict_mode_fails_for_missing_source_mapping(tmp_path: Path):
    repo = _write_repo(tmp_path)
    rc = guard.main([
        "--repo-root",
        str(repo),
        "--cmake-file",
        "tests/CMakeLists.txt",
        "--strict-missing-sources",
    ])
    assert rc == 1


def test_main_writes_json_report(tmp_path: Path):
    repo = _write_repo(tmp_path)
    out = repo / "artifacts" / "copilot_guard.json"
    rc = guard.main([
        "--repo-root",
        str(repo),
        "--cmake-file",
        "tests/CMakeLists.txt",
        "--output-json",
        str(out),
    ])
    assert rc == 0
    data = json.loads(out.read_text(encoding="utf-8"))
    assert data["inventory_count"] == 1
    assert data["potential_missing_sources_count"] == 1


def test_exempt_interface_target_not_flagged_as_affected(tmp_path: Path):
    repo = _write_repo(tmp_path)
    (repo / "tests" / "CMakeLists.txt").write_text(
        """
add_executable(test_cache_interfaces
  test_cache_interfaces.cpp
)
target_link_libraries(test_cache_interfaces PRIVATE Threads::Threads)
target_compile_definitions(test_cache_interfaces PRIVATE THEMIS_TEST_BUILD=1)
""".strip()
        + "\n",
        encoding="utf-8",
    )
    inventory, affected = guard.inventory_test_targets(repo, repo / "tests" / "CMakeLists.txt")
    assert len(inventory) == 1
    assert inventory[0].exempt is True
    assert affected == []


def test_main_fails_when_build_log_contains_lnk_errors(tmp_path: Path):
    repo = _write_repo(tmp_path)
    log = repo / "build.log"
    log.write_text("fatal error LNK2001: unresolved external symbol _X\n", encoding="utf-8")
    rc = guard.main([
        "--repo-root", str(repo),
        "--cmake-file", "tests/CMakeLists.txt",
        "--build-log", str(log),
    ])
    assert rc == 1
