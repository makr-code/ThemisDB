"""Tests for the incremental CMake test-registration guard."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from tools.ci.check_test_registration import (
    find_added_test_source_guards,
    build_test_source_inventory,
    find_deregistered_sources,
    find_duplicate_target_sources,
    find_unregistered_sources,
)


class TestRegistrationGuardTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.test_source = self.root / "tests" / "sample" / "test_sample.cpp"
        self.test_source.parent.mkdir(parents=True)
        self.test_source.write_text("TEST(Sample, Runs) {}", encoding="utf-8")
        self.changed_files = self.root / "changed-files.txt"
        self.changed_files.write_text("tests/sample/test_sample.cpp\n", encoding="utf-8")
        self.compilation_database = self.root / "compile_commands.json"

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_compilation_database(
        self, sources: list[Path], targets: list[str] | None = None
    ) -> None:
        targets = targets or ["sample_tests"] * len(sources)
        entries = [
            {
                "directory": str(self.root),
                "file": str(source),
                "output": str(
                    self.root / "build" / "CMakeFiles" / f"{target}.dir" / "sample.obj"
                ),
            }
            for source, target in zip(sources, targets, strict=True)
        ]
        self.compilation_database.write_text(json.dumps(entries), encoding="utf-8")

    def test_registered_source_is_accepted(self) -> None:
        self.write_compilation_database([self.test_source])

        self.assertEqual(
            find_unregistered_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            [],
        )

    def test_unregistered_source_is_reported(self) -> None:
        self.write_compilation_database([])

        self.assertEqual(
            find_unregistered_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            [self.test_source],
        )

    def test_multiple_focused_targets_for_changed_source_are_reported(self) -> None:
        self.write_compilation_database(
            [self.test_source, self.test_source],
            ["sample_focused", "sample_integration"],
        )

        self.assertEqual(
            find_duplicate_target_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            {self.test_source: ["sample_focused", "sample_integration"]},
        )

    def test_aggregate_and_one_focused_target_are_allowed(self) -> None:
        self.write_compilation_database(
            [self.test_source] * 4,
            [
                "themis_tests",
                "themis_tests_critical",
                "sample_focused",
                "sample_focused",
            ],
        )

        self.assertEqual(
            find_duplicate_target_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            {},
        )

    def test_inventory_does_not_count_critical_aggregate_as_focused(self) -> None:
        self.write_compilation_database(
            [self.test_source, self.test_source],
            ["module_sample_test_sample_focused", "themis_tests_critical"],
        )

        inventory = build_test_source_inventory(self.root, self.compilation_database)

        self.assertEqual(inventory["summary"]["multiple_focused_target_sources"], 0)

    def test_missing_target_metadata_fails_closed(self) -> None:
        self.compilation_database.write_text(
            json.dumps([{"directory": str(self.root), "file": str(self.test_source)}]),
            encoding="utf-8",
        )

        self.assertEqual(
            find_duplicate_target_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            {self.test_source: ["<unresolved CMake target>"]},
        )

    def test_shared_gtest_runner_is_allowed_in_multiple_focused_targets(self) -> None:
        runner_source = self.root / "tests" / "test_gtest_main.cpp"
        runner_source.parent.mkdir(parents=True, exist_ok=True)
        runner_source.write_text("int main() { return 0; }", encoding="utf-8")
        self.changed_files.write_text("tests/test_gtest_main.cpp\n", encoding="utf-8")
        self.write_compilation_database(
            [runner_source] * 6,
            [
                "module_network_test_transport_security_checker_focused",
                "module_security_test_hsm_security_checker_focused",
                "module_updates_test_updates_production_focused",
                "test_auto_failover_recovery_focused",
                "test_production_mode_security_focused",
                "test_wal_storage_focused",
            ],
        )

        self.assertEqual(
            find_duplicate_target_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            {},
        )

    def test_shared_gtest_runner_still_requires_target_metadata(self) -> None:
        runner_source = self.root / "tests" / "test_gtest_main.cpp"
        runner_source.parent.mkdir(parents=True, exist_ok=True)
        runner_source.write_text("int main() { return 0; }", encoding="utf-8")
        self.changed_files.write_text("tests/test_gtest_main.cpp\n", encoding="utf-8")
        self.compilation_database.write_text(
            json.dumps([{"directory": str(self.root), "file": str(runner_source)}]),
            encoding="utf-8",
        )

        self.assertEqual(
            find_duplicate_target_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            {runner_source: ["<unresolved CMake target>"]},
        )

    def test_inventory_reports_profile_coverage_and_duplicate_targets(self) -> None:
        another_source = self.root / "tests" / "sample" / "test_not_in_profile.cpp"
        another_source.write_text("TEST(Sample, NotBuilt) {}", encoding="utf-8")
        self.write_compilation_database(
            [self.test_source, self.test_source],
            ["sample_focused", "sample_integration"],
        )

        inventory = build_test_source_inventory(self.root, self.compilation_database)

        self.assertEqual(
            inventory["summary"],
            {
                "total_test_sources": 2,
                "compiled_sources": 1,
                "not_in_profile_sources": 1,
                "unknown_target_sources": 0,
                "multiple_focused_target_sources": 1,
            },
        )
        self.assertEqual(
            inventory["sources"],
            [
                {
                    "path": "tests/sample/test_not_in_profile.cpp",
                    "status": "not_in_profile",
                    "targets": [],
                },
                {
                    "path": "tests/sample/test_sample.cpp",
                    "status": "compiled",
                    "targets": ["sample_focused", "sample_integration"],
                },
            ],
        )
        self.assertEqual(
            inventory["build_profile"], self.compilation_database.parent.name
        )
        self.assertIsInstance(inventory["platform"], str)

    def test_inventory_distinguishes_compiled_sources_without_target_metadata(self) -> None:
        self.compilation_database.write_text(
            json.dumps(
                [{"directory": str(self.root), "file": str(self.test_source)}]
            ),
            encoding="utf-8",
        )

        inventory = build_test_source_inventory(self.root, self.compilation_database)

        self.assertEqual(
            inventory["summary"],
            {
                "total_test_sources": 1,
                "compiled_sources": 1,
                "not_in_profile_sources": 0,
                "unknown_target_sources": 1,
                "multiple_focused_target_sources": 0,
            },
        )
        self.assertEqual(inventory["sources"][0]["status"], "compiled")
        self.assertEqual(inventory["sources"][0]["targets"], [])

    def test_non_test_cpp_changes_are_ignored(self) -> None:
        self.changed_files.write_text("tests/sample/helper.cpp\n", encoding="utf-8")
        self.write_compilation_database([])

        self.assertEqual(
            find_unregistered_sources(
                self.root, self.compilation_database, self.changed_files
            ),
            [],
        )

    def test_missing_changed_test_source_fails_closed(self) -> None:
        self.test_source.unlink()
        self.write_compilation_database([])

        with self.assertRaisesRegex(ValueError, "does not exist"):
            find_unregistered_sources(
                self.root, self.compilation_database, self.changed_files
            )

    def test_added_test_source_existence_guard_is_rejected(self) -> None:
        diff = self.root / "cmake.diff"
        diff.write_text(
            '+if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_missing.cpp")\n'
            "+elseif(UNIX AND EXISTS test_platform.cpp)\n"
            "+if(\n"
            '+    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multiline.cpp"\n'
            "+)\n"
            " if(EXISTS old_test.cpp)\n",
            encoding="utf-8",
        )

        self.assertEqual(
            find_added_test_source_guards(diff),
            [
                'if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_missing.cpp")',
                "elseif(UNIX AND EXISTS test_platform.cpp)",
                'if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multiline.cpp" )',
            ],
        )

    def test_removed_registration_for_existing_uncompiled_source_is_reported(self) -> None:
        diff = self.root / "cmake.diff"
        diff.write_text(
            "diff --git a/tests/sample/CMakeLists.txt b/tests/sample/CMakeLists.txt\n"
            "--- a/tests/sample/CMakeLists.txt\n"
            "+++ b/tests/sample/CMakeLists.txt\n"
            "@@ -1 +0,0 @@\n"
            "-add_executable(sample_tests test_sample.cpp)\n",
            encoding="utf-8",
        )
        self.write_compilation_database([])

        self.assertEqual(
            find_deregistered_sources(diff, self.compilation_database, self.root),
            ["tests/sample/test_sample.cpp"],
        )

    def test_removed_registration_is_accepted_when_source_remains_compiled(self) -> None:
        diff = self.root / "cmake.diff"
        diff.write_text(
            "diff --git a/tests/sample/CMakeLists.txt b/tests/sample/CMakeLists.txt\n"
            "--- a/tests/sample/CMakeLists.txt\n"
            "+++ b/tests/sample/CMakeLists.txt\n"
            "-add_executable(sample_tests test_sample.cpp)\n",
            encoding="utf-8",
        )
        self.write_compilation_database([self.test_source])

        self.assertEqual(
            find_deregistered_sources(diff, self.compilation_database, self.root),
            [],
        )

    def test_removed_registration_for_deleted_source_is_ignored(self) -> None:
        diff = self.root / "cmake.diff"
        diff.write_text(
            "diff --git a/tests/sample/CMakeLists.txt b/tests/sample/CMakeLists.txt\n"
            "--- a/tests/sample/CMakeLists.txt\n"
            "+++ b/tests/sample/CMakeLists.txt\n"
            "-add_executable(sample_tests test_sample.cpp)\n",
            encoding="utf-8",
        )
        self.test_source.unlink()
        self.write_compilation_database([])

        self.assertEqual(
            find_deregistered_sources(diff, self.compilation_database, self.root),
            [],
        )


if __name__ == "__main__":
    unittest.main()
