from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / "ci"))

import test_quality_inventory as inventory  # noqa: E402


class TestQualityInventoryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        (self.root / "src/server").mkdir(parents=True)
        (self.root / "tests/server").mkdir(parents=True)
        (self.root / "tests/legacy/old").mkdir(parents=True)
        (self.root / "tests/test_root.cpp").write_text(
            "TEST(Root, Inventory) {}\n", encoding="utf-8"
        )
        (self.root / "src/server/handler.cpp").write_text(
            "int run() {\n  return 1;\n}\n", encoding="utf-8"
        )
        (self.root / "tests/server/test_handler.cpp").write_text(
            'TEST(Handler, ReturnsValue) { EXPECT_TRUE(true); '
            'EXPECT_EQ(1, 1); GTEST_SKIP() << "backend unavailable"; }\n',
            encoding="utf-8",
        )
        (self.root / "tests/legacy/old/test_old.cpp").write_text(
            "TEST(Old, Historical) { EXPECT_TRUE(false); }\n", encoding="utf-8"
        )

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def test_inventory_counts_and_flags_quality_candidates(self) -> None:
        report = inventory.build_inventory(self.root, source_commit="abc123")

        self.assertEqual(report["source_commit"], "abc123")
        self.assertEqual(report["summary"]["test_files"], 3)
        self.assertEqual(report["summary"]["test_case_declarations"], 3)
        self.assertEqual(report["summary"]["gtest_skip_calls"], 1)
        self.assertEqual(report["summary"]["skip_calls_with_literal_reason"], 1)
        self.assertEqual(report["summary"]["trivial_assertion_candidates"], 2)
        self.assertEqual(report["summary"]["legacy_test_files"], 1)
        server = report["modules"]["server"]
        self.assertEqual(server["source"]["loc"], 3)
        self.assertEqual(server["tests"]["skips_detail"][0]["reason"], "backend unavailable")
        self.assertEqual(server["skip_budget"]["m1_maximum"], 1)
        self.assertEqual(report["legacy_files"][0]["legacy_disposition"], "legacy_review_pending")
        self.assertEqual(report["modules"]["root"]["tests"]["files"], 1)

    def test_compile_database_and_ctest_json_provide_target_evidence(self) -> None:
        build = self.root / "build"
        target = "module_server_test_handler_focused"
        output = build / "tests/server/CMakeFiles" / f"{target}.dir/test_handler.cpp.o"
        output.parent.mkdir(parents=True)
        compile_commands = build / "compile_commands.json"
        compile_commands.write_text(
            json.dumps(
                [
                    {
                        "directory": str(self.root),
                        "file": str(self.root / "tests/server/test_handler.cpp"),
                        "output": str(output),
                        "command": "c++ -c test_handler.cpp",
                    }
                ]
            ),
            encoding="utf-8",
        )
        ctest_json = build / "ctest.json"
        ctest_json.write_text(
            json.dumps(
                {
                    "tests": [
                        {
                            "name": "HandlerFocusedTests",
                            "command": [str(build / "bin" / target)],
                        }
                    ]
                }
            ),
            encoding="utf-8",
        )

        report = inventory.build_inventory(
            self.root, compile_commands=compile_commands, ctest_json=ctest_json
        )

        handler = next(
            item
            for item in report["test_files"]
            if item["path"] == "tests/server/test_handler.cpp"
        )
        self.assertEqual(handler["registration_status"], "registered")
        self.assertEqual(handler["targets"], [target])
        self.assertEqual(handler["ctest_cases"], ["HandlerFocusedTests"])

    def test_coverage_is_explicitly_missing_or_taken_from_module_json(self) -> None:
        without_coverage = inventory.build_inventory(self.root)
        self.assertEqual(without_coverage["summary"]["coverage_status"], "unavailable")
        self.assertIsNone(without_coverage["modules"]["server"]["coverage"])

        coverage_json = self.root / "coverage.json"
        coverage_json.write_text(
            json.dumps(
                {
                    "modules": {
                        "server": {
                            "coverage_percent": 72.5,
                            "branch_coverage_percent": 61.0,
                        }
                    }
                }
            ),
            encoding="utf-8",
        )
        with_coverage = inventory.build_inventory(
            self.root, coverage_json=coverage_json
        )
        self.assertEqual(
            with_coverage["modules"]["server"]["coverage"]["coverage_percent"],
            72.5,
        )
        self.assertEqual(
            with_coverage["modules"]["server"]["coverage_status"], "measured"
        )


if __name__ == "__main__":
    unittest.main()
