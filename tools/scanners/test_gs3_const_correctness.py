#!/usr/bin/env python3
"""Unit tests for the const-correctness scanner."""

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from scanners.gs3_step04_design_const_correctness import ConstCorrectnessApiScan


def _run_scan(code: str):
    with tempfile.TemporaryDirectory() as tmpdir:
        root = Path(tmpdir)
        src_dir = root / "src" / "demo"
        src_dir.mkdir(parents=True)
        test_file = src_dir / "sample.cpp"
        test_file.write_text(code)

        scanner = ConstCorrectnessApiScan(str(root))
        gaps = scanner.scan_files([test_file])
        return gaps


def test_const_cast_detection():
    gaps = _run_scan(
        """
        class Widget {
        public:
            int value() const {
                return const_cast<Widget*>(this)->value_;
            }
        private:
            int value_{0};
        };
        """
    )
    assert any(g["pattern"] == "const_cast_in_const_method" for g in gaps), gaps


def test_nonconst_ref_return_from_const():
    gaps = _run_scan(
        """
        class Cache {
        public:
            std::vector<int>& items() const { return items_; }
        private:
            std::vector<int> items_;
        };
        """
    )
    assert any(g["pattern"] == "mutable_collection_return_from_const" for g in gaps), gaps


def test_mutable_member_write_detection():
    gaps = _run_scan(
        """
        class QueryCache {
        public:
            void refresh() const {
                cache_.clear();
            }
        private:
            mutable std::vector<int> cache_;
        };
        """
    )
    assert any(g["pattern"] == "mutable_member_written_in_const_method" for g in gaps), gaps


def test_heavy_param_by_value_detection():
    gaps = _run_scan(
        """
        void logQuery(std::string query) {
            if (query.empty()) {
                return;
            }
        }
        """
    )
    assert any(g["pattern"] == "heavy_param_by_value" for g in gaps), gaps


def test_safe_const_patterns_not_flagged():
    gaps = _run_scan(
        """
        class SafeView {
        public:
            const std::vector<int>& items() const { return items_; }

            void takeOwnership(std::string value) {
                stored_ = std::move(value);
            }
        private:
            std::vector<int> items_;
            std::string stored_;
        };
        """
    )
    flagged = {g["pattern"] for g in gaps}
    assert "mutable_collection_return_from_const" not in flagged, gaps
    assert "nonconst_ref_return_from_const" not in flagged, gaps
    assert "heavy_param_by_value" not in flagged, gaps


if __name__ == "__main__":
    tests = [
        test_const_cast_detection,
        test_nonconst_ref_return_from_const,
        test_mutable_member_write_detection,
        test_heavy_param_by_value_detection,
        test_safe_const_patterns_not_flagged,
    ]

    failures = []
    for test in tests:
        try:
            test()
            print(f"PASS: {test.__name__}")
        except AssertionError as exc:
            print(f"FAIL: {test.__name__}: {exc}")
            failures.append(test.__name__)

    sys.exit(1 if failures else 0)
