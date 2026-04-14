"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cross_module_regression_detector.py           ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:35:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     490                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2ea3922499  2026-03-01  feat(performance): cross-module performance regression de... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for benchmarks/cross_module_regression_detector.py

Covers:
- ModuleResult blocking-regression gate
- CrossModuleReport aggregate properties
- CrossModuleRegressionDetector.load_module_result (happy path + errors)
- CrossModuleRegressionDetector.aggregate
- CrossModuleRegressionDetector.generate_report (smoke tests)
- CrossModuleRegressionDetector.save_report (file output)
- CLI exit codes via main()
- Edge cases: single module, all passing, mixed results
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Dict

import pytest

# ---------------------------------------------------------------------------
# Ensure benchmarks/ directory is importable
# ---------------------------------------------------------------------------
_BENCHMARKS = Path(__file__).resolve().parent.parent  # benchmarks/
if str(_BENCHMARKS) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS))

from cross_module_regression_detector import (  # noqa: E402
    CrossModuleRegressionDetector,
    CrossModuleReport,
    ModuleResult,
    main as detector_main,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _module_result(
    module: str = "test_module",
    critical: int = 0,
    major: int = 0,
    minor: int = 0,
    improvements: int = 0,
    total: int = 0,
) -> ModuleResult:
    return ModuleResult(
        module=module,
        critical=critical,
        major=major,
        minor=minor,
        improvements=improvements,
        total=total,
        source_path="/fake/path.json",
    )


def _write_module_json(
    tmp: Path,
    name: str,
    summary: Dict[str, Any],
) -> Path:
    """Write a minimal module regression JSON summary to a temp file."""
    data = {
        "summary": summary,
        "comparisons": [],
    }
    p = tmp / name
    p.write_text(json.dumps(data), encoding="utf-8")
    return p


# ---------------------------------------------------------------------------
# ModuleResult tests
# ---------------------------------------------------------------------------

class TestModuleResult:

    def test_no_regressions_does_not_block(self):
        m = _module_result(critical=0, major=0, minor=0)
        assert not m.has_blocking_regressions("major")

    def test_critical_blocks_at_major(self):
        m = _module_result(critical=1)
        assert m.has_blocking_regressions("major")

    def test_major_blocks_at_major(self):
        m = _module_result(major=2)
        assert m.has_blocking_regressions("major")

    def test_minor_does_not_block_at_major(self):
        m = _module_result(minor=3)
        assert not m.has_blocking_regressions("major")

    def test_minor_blocks_at_minor(self):
        m = _module_result(minor=1)
        assert m.has_blocking_regressions("minor")

    def test_critical_blocks_at_critical(self):
        m = _module_result(critical=1)
        assert m.has_blocking_regressions("critical")

    def test_major_does_not_block_at_critical(self):
        m = _module_result(major=5)
        assert not m.has_blocking_regressions("critical")


# ---------------------------------------------------------------------------
# CrossModuleReport tests
# ---------------------------------------------------------------------------

class TestCrossModuleReport:

    def test_aggregate_totals(self):
        report = CrossModuleReport(modules=[
            _module_result("accel", critical=1, major=2, minor=3,
                           improvements=4, total=10),
            _module_result("chimera", critical=0, major=1, minor=0,
                           improvements=2, total=3),
        ])
        assert report.total_critical == 1
        assert report.total_major == 3
        assert report.total_minor == 3
        assert report.total_improvements == 6
        assert report.total_comparisons == 13

    def test_no_modules_all_zeros(self):
        report = CrossModuleReport(modules=[])
        assert report.total_critical == 0
        assert report.total_major == 0
        assert not report.has_blocking_regressions()

    def test_has_blocking_returns_true_if_any_module_blocks(self):
        report = CrossModuleReport(modules=[
            _module_result("a", major=0),
            _module_result("b", major=1),
        ])
        assert report.has_blocking_regressions("major")

    def test_has_blocking_returns_false_when_all_pass(self):
        report = CrossModuleReport(modules=[
            _module_result("a", minor=2),
            _module_result("b", minor=1),
        ])
        assert not report.has_blocking_regressions("major")

    def test_blocking_modules_returns_only_failing_ones(self):
        m_pass = _module_result("pass_mod", major=0)
        m_fail = _module_result("fail_mod", major=1)
        report = CrossModuleReport(modules=[m_pass, m_fail])
        blocking = report.blocking_modules("major")
        assert len(blocking) == 1
        assert blocking[0].module == "fail_mod"

    def test_single_module_pass(self):
        report = CrossModuleReport(modules=[_module_result("only", major=0)])
        assert not report.has_blocking_regressions()

    def test_single_module_fail(self):
        report = CrossModuleReport(modules=[_module_result("only", critical=1)])
        assert report.has_blocking_regressions()


# ---------------------------------------------------------------------------
# CrossModuleRegressionDetector.load_module_result
# ---------------------------------------------------------------------------

class TestLoadModuleResult:

    def test_loads_valid_json(self, tmp_path):
        p = _write_module_json(tmp_path, "mod.json", {
            "critical": 2, "major": 1, "minor": 0,
            "improvements": 3, "total": 10,
        })
        det = CrossModuleRegressionDetector()
        result = det.load_module_result("mymod", str(p))
        assert result.module == "mymod"
        assert result.critical == 2
        assert result.major == 1
        assert result.minor == 0
        assert result.improvements == 3
        assert result.total == 10

    def test_missing_file_raises(self, tmp_path):
        det = CrossModuleRegressionDetector()
        with pytest.raises(FileNotFoundError):
            det.load_module_result("missing", str(tmp_path / "nonexistent.json"))

    def test_invalid_json_raises(self, tmp_path):
        bad = tmp_path / "bad.json"
        bad.write_text("not json!!", encoding="utf-8")
        det = CrossModuleRegressionDetector()
        with pytest.raises(json.JSONDecodeError):
            det.load_module_result("bad", str(bad))

    def test_missing_summary_key_raises(self, tmp_path):
        p = tmp_path / "no_summary.json"
        p.write_text(json.dumps({"other": {}}), encoding="utf-8")
        det = CrossModuleRegressionDetector()
        with pytest.raises(KeyError):
            det.load_module_result("bad", str(p))

    def test_partial_summary_defaults_to_zero(self, tmp_path):
        # If some counts are missing in the JSON, they default to 0.
        p = _write_module_json(tmp_path, "partial.json", {"critical": 1})
        det = CrossModuleRegressionDetector()
        result = det.load_module_result("partial", str(p))
        assert result.critical == 1
        assert result.major == 0
        assert result.minor == 0
        assert result.improvements == 0
        assert result.total == 0


# ---------------------------------------------------------------------------
# CrossModuleRegressionDetector.aggregate
# ---------------------------------------------------------------------------

class TestAggregate:

    def test_returns_cross_module_report(self):
        det = CrossModuleRegressionDetector()
        m1 = _module_result("a")
        m2 = _module_result("b")
        report = det.aggregate([m1, m2])
        assert isinstance(report, CrossModuleReport)
        assert len(report.modules) == 2

    def test_empty_list_is_valid(self):
        det = CrossModuleRegressionDetector()
        report = det.aggregate([])
        assert isinstance(report, CrossModuleReport)
        assert report.total_critical == 0


# ---------------------------------------------------------------------------
# CrossModuleRegressionDetector.generate_report
# ---------------------------------------------------------------------------

class TestGenerateReport:

    def test_report_contains_verdict_passed(self):
        report = CrossModuleReport(modules=[_module_result("a")])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert "PASSED" in text

    def test_report_contains_verdict_failed(self):
        report = CrossModuleReport(modules=[_module_result("a", critical=1)])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert "FAILED" in text

    def test_report_contains_warning_for_minor(self):
        report = CrossModuleReport(modules=[_module_result("a", minor=2)])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert "WARNING" in text

    def test_report_contains_module_name(self):
        report = CrossModuleReport(modules=[_module_result("my_special_module")])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert "my_special_module" in text

    def test_report_is_nonempty_string(self):
        report = CrossModuleReport(modules=[])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert isinstance(text, str)
        assert len(text) > 0

    def test_multiple_modules_all_listed(self):
        report = CrossModuleReport(modules=[
            _module_result("acceleration"),
            _module_result("chimera"),
        ])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        assert "acceleration" in text
        assert "chimera" in text


# ---------------------------------------------------------------------------
# CrossModuleRegressionDetector.save_report
# ---------------------------------------------------------------------------

class TestSaveReport:

    def test_text_and_json_files_created(self, tmp_path):
        report = CrossModuleReport(modules=[_module_result("a", major=1)])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        out = tmp_path / "report.txt"
        det.save_report(text, report, out, "major")
        assert out.exists()
        assert out.stat().st_size > 0

        json_out = tmp_path / "report.json"
        assert json_out.exists()

    def test_json_summary_structure(self, tmp_path):
        report = CrossModuleReport(modules=[
            _module_result("a", critical=1, major=2, minor=3, total=6),
        ])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        out = tmp_path / "report.txt"
        det.save_report(text, report, out, "major")

        data = json.loads((tmp_path / "report.json").read_text())
        assert "summary" in data
        assert "modules" in data
        assert data["summary"]["critical"] == 1
        assert data["summary"]["major"] == 2
        assert data["summary"]["total"] == 6
        assert data["summary"]["blocking"] is True
        assert len(data["modules"]) == 1

    def test_json_blocking_false_when_no_regressions(self, tmp_path):
        report = CrossModuleReport(modules=[_module_result("a")])
        det = CrossModuleRegressionDetector()
        text = det.generate_report(report, "major")
        out = tmp_path / "report.txt"
        det.save_report(text, report, out, "major")
        data = json.loads((tmp_path / "report.json").read_text())
        assert data["summary"]["blocking"] is False


# ---------------------------------------------------------------------------
# CLI tests
# ---------------------------------------------------------------------------

class TestCLI:

    def _write_module_file(
        self, tmp: Path, name: str, summary: Dict[str, Any]
    ) -> Path:
        return _write_module_json(tmp, name, summary)

    def test_exit_0_no_regressions(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "accel.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 5, "total": 5},
        )
        rc = detector_main([
            "--module", f"acceleration:{p}",
            "--output", str(tmp_path / "out.txt"),
        ])
        assert rc == 0

    def test_exit_1_major_regression(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "mod.json",
            {"critical": 0, "major": 2, "minor": 0, "improvements": 0, "total": 2},
        )
        rc = detector_main([
            "--module", f"accel:{p}",
            "--output", str(tmp_path / "out.txt"),
            "--fail-on", "major",
        ])
        assert rc == 1

    def test_exit_0_minor_with_major_gate(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "mod.json",
            {"critical": 0, "major": 0, "minor": 3, "improvements": 0, "total": 3},
        )
        rc = detector_main([
            "--module", f"mod:{p}",
            "--output", str(tmp_path / "out.txt"),
            "--fail-on", "major",
        ])
        assert rc == 0

    def test_exit_1_critical_with_major_gate(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "mod.json",
            {"critical": 1, "major": 0, "minor": 0, "improvements": 0, "total": 1},
        )
        rc = detector_main([
            "--module", f"mod:{p}",
            "--output", str(tmp_path / "out.txt"),
            "--fail-on", "major",
        ])
        assert rc == 1

    def test_exit_0_multiple_modules_all_pass(self, tmp_path):
        p1 = self._write_module_file(
            tmp_path, "a.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 1, "total": 4},
        )
        p2 = self._write_module_file(
            tmp_path, "b.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 2, "total": 6},
        )
        rc = detector_main([
            "--module", f"acceleration:{p1}",
            "--module", f"chimera:{p2}",
            "--output", str(tmp_path / "out.txt"),
        ])
        assert rc == 0

    def test_exit_1_one_of_two_modules_fails(self, tmp_path):
        p_pass = self._write_module_file(
            tmp_path, "pass.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 3, "total": 3},
        )
        p_fail = self._write_module_file(
            tmp_path, "fail.json",
            {"critical": 0, "major": 1, "minor": 0, "improvements": 0, "total": 1},
        )
        rc = detector_main([
            "--module", f"mod_a:{p_pass}",
            "--module", f"mod_b:{p_fail}",
            "--output", str(tmp_path / "out.txt"),
            "--fail-on", "major",
        ])
        assert rc == 1

    def test_exit_2_missing_file(self, tmp_path):
        rc = detector_main([
            "--module", f"mod:{tmp_path / 'nonexistent.json'}",
            "--output", str(tmp_path / "out.txt"),
        ])
        assert rc == 2

    def test_exit_2_invalid_module_spec(self, tmp_path):
        rc = detector_main([
            "--module", "no-colon-here",
            "--output", str(tmp_path / "out.txt"),
        ])
        assert rc == 2

    def test_report_file_created(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "mod.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 0, "total": 0},
        )
        out = tmp_path / "report.txt"
        detector_main([
            "--module", f"mod:{p}",
            "--output", str(out),
        ])
        assert out.exists()
        assert out.stat().st_size > 0

    def test_json_summary_file_created_alongside_report(self, tmp_path):
        p = self._write_module_file(
            tmp_path, "mod.json",
            {"critical": 0, "major": 0, "minor": 0, "improvements": 0, "total": 0},
        )
        out = tmp_path / "report.txt"
        detector_main([
            "--module", f"mod:{p}",
            "--output", str(out),
        ])
        json_out = tmp_path / "report.json"
        assert json_out.exists()
        data = json.loads(json_out.read_text())
        assert "summary" in data
        assert "modules" in data
