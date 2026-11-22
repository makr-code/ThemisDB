"""Regression tests for test and benchmark standards inventory tooling."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools" / "ci"))

from check_testing_standards import scan_cpp, scan_raw_add_test


class TestingStandardsInventoryTests(unittest.TestCase):
    def test_trivial_gtest_main_is_detected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "tests").mkdir()
            (root / "tests" / "sample.cpp").write_text(
                "int main(int argc, char** argv) {\n"
                "  ::testing::InitGoogleTest(&argc, argv);\n"
                "  return RUN_ALL_TESTS();\n"
                "}\n",
                encoding="utf-8",
            )
            result = scan_cpp(root, "tests")
        self.assertEqual(len(result["trivial_gtest_main"]), 1)

    def test_custom_gtest_main_is_distinguished(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "tests").mkdir()
            (root / "tests" / "worker.cpp").write_text(
                "int main(int argc, char** argv) {\n"
                "  if (argc > 1) return 7;\n"
                "  ::testing::InitGoogleTest(&argc, argv);\n"
                "  return RUN_ALL_TESTS();\n"
                "}\n",
                encoding="utf-8",
            )
            result = scan_cpp(root, "tests")
        self.assertEqual(len(result["custom_gtest_main"]), 1)

    def test_benchmark_main_is_standardized(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "benchmarks").mkdir()
            (root / "benchmarks" / "bench_sample.cpp").write_text(
                "BENCHMARK_MAIN();\n",
                encoding="utf-8",
            )
            result = scan_cpp(root, "benchmarks")
        self.assertEqual(len(result["benchmark_main"]), 1)

    def test_raw_add_test_inventory_finds_cmake_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "tests").mkdir()
            (root / "tests" / "CMakeLists.txt").write_text(
                "add_test(NAME Sample COMMAND sample)\n",
                encoding="utf-8",
            )
            result = scan_raw_add_test(root)
        self.assertEqual(result, ["tests/CMakeLists.txt"])


if __name__ == "__main__":
    unittest.main()
