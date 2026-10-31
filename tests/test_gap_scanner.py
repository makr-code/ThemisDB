import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from gap_scanner import GapScanner


class GapScannerTests(unittest.TestCase):
    def test_module_scan_recurses_and_summarizes(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            for directory in ("src/base", "include/base", "tests/base", "benchmarks/base"):
                (root / directory).mkdir(parents=True)

            (root / "tests/base/nested.cpp").write_text("// TODO: nested test\n", encoding="utf-8")
            (root / "tests/test_base_root.cpp").write_text("// TODO: root test\n", encoding="utf-8")
            (root / "benchmarks/base/nested.cpp").write_text("if (empty) return {};\n", encoding="utf-8")

            scanner = GapScanner(str(root))
            gaps = scanner.scan_module("base")

            self.assertEqual(len(gaps["tests"]), 2)
            self.assertEqual(len(gaps["benchmarks"]), 1)
            self.assertEqual(
                scanner.summarize_gaps(gaps),
                {"total": 3, "critical": 1, "high": 0, "medium": 2, "low": 0},
            )


if __name__ == "__main__":
    unittest.main()
