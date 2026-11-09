from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2] / "scripts"
sys.path.insert(0, str(SCRIPTS))

import generate_coverage_baseline as coverage  # noqa: E402


class CoverageBaselineTests(unittest.TestCase):
    def test_only_production_sources_and_headers_contribute_to_modules(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            lcov_file = Path(temporary_directory) / "coverage.info"
            lcov_file.write_text(
                "\n".join(
                    [
                        "SF:/repo/src/server/handler.cpp",
                        "LH:3",
                        "LF:5",
                        "BRH:1",
                        "BRF:2",
                        "end_of_record",
                        "SF:/repo/include/server/handler.h",
                        "LH:2",
                        "LF:3",
                        "BRH:1",
                        "BRF:1",
                        "end_of_record",
                        "SF:/repo/tests/server/test_handler.cpp",
                        "LH:8",
                        "LF:10",
                        "BRH:4",
                        "BRF:5",
                        "end_of_record",
                        "SF:/repo/benchmarks/server/bench_handler.cpp",
                        "LH:6",
                        "LF:8",
                        "BRH:2",
                        "BRF:3",
                        "end_of_record",
                    ]
                ),
                encoding="utf-8",
            )

            modules = coverage.CoverageParser().parse_lcov_file(str(lcov_file))

        self.assertEqual(set(modules), {"server"})
        self.assertEqual(modules["server"].lines_covered, 5)
        self.assertEqual(modules["server"].lines_executable, 8)
        self.assertEqual(modules["server"].branches_covered, 2)
        self.assertEqual(modules["server"].branches_total, 3)

    def test_module_extraction_normalizes_windows_paths(self) -> None:
        self.assertEqual(
            coverage.CoverageParser._extract_module_name(
                r"C:\repo\src\server\handler.cpp"
            ),
            "server",
        )
        self.assertIsNone(
            coverage.CoverageParser._extract_module_name(
                "/repo/tests/server/test_handler.cpp"
            )
        )


if __name__ == "__main__":
    unittest.main()
