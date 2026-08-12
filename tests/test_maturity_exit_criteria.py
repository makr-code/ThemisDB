#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "verification" / "check_maturity_exit_criteria.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("check_maturity_exit_criteria", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


checker = _load_module()


def _write(path: Path, content: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return path


def _base_report(
    *,
    technical: int,
    governance: int,
    zero_tests: int,
    zero_bench: int,
    d1_cell: str = "🟢 80%",
    compliance_line: str = "| Model Cards | ✅ | Abgedeckt |",
) -> str:
    return textwrap.dedent(
        f"""\
        # ThemisDB — Maturity Report

        | **GA-Blocker (technisch)** | **{technical}** | ✅ |
        | **GA-Blocker (Governance)** | **{governance}** | ✅ |

        ## 2. Matrix
        | Modul | LOC | Stubs | D1 | Tests | D2 | Bench-Files | D3 | Gesamt |
        |-------|-----|-------|----|-------|----|-----------|----|--------|
        | `retrieval` | 100 | 0 | {d1_cell} | 2 | 🟢 | 1 | 🟢 | 🟢 |

        ## 4. Compliance
        {compliance_line}

        ## 10. Abschlusskennzahlen
        | Kennzahl | Wert |
        |----------|------|
        | Module mit 0 Tests | {zero_tests} |
        | Module mit 0 Benchmarks | {zero_bench} |
        """
    )


class MaturityExitCriteriaTests(unittest.TestCase):
    def test_evaluate_passes_when_all_criteria_green(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            _write(repo / "ROADMAP.md", "- [x] done\n")
            _write(repo / "src" / "query" / "ROADMAP.md", "- [x] done\n")
            report = _write(
                repo / "audit" / "MATURITY.md",
                _base_report(technical=0, governance=0, zero_tests=0, zero_bench=0),
            )

            result = checker.evaluate(repo_root=repo, maturity_report_path=report)

            self.assertTrue(result["pass"])
            self.assertTrue(all(c["passed"] for c in result["checks"]))
            self.assertEqual(result["details"]["placeholder_modules"], [])

    def test_evaluate_fails_for_open_items_placeholder_and_compliance_markers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            _write(repo / "ROADMAP.md", "- [ ] open item\n")
            _write(repo / "src" / "query" / "ROADMAP.md", "- [~] in progress\n")
            report = _write(
                repo / "audit" / "MATURITY.md",
                _base_report(
                    technical=0,
                    governance=1,
                    zero_tests=2,
                    zero_bench=1,
                    d1_cell="⬛ 20%",
                    compliance_line="| Model Cards | 🔴 Fehlend | offen |",
                ),
            )

            result = checker.evaluate(repo_root=repo, maturity_report_path=report)
            by_name = {c["name"]: c for c in result["checks"]}

            self.assertFalse(result["pass"])
            self.assertFalse(by_name["ga_blockers_governance"]["passed"])
            self.assertFalse(by_name["modules_with_zero_tests"]["passed"])
            self.assertFalse(by_name["modules_with_zero_benchmarks"]["passed"])
            self.assertFalse(by_name["roadmap_open_items"]["passed"])
            self.assertFalse(by_name["roadmap_in_progress_items"]["passed"])
            self.assertFalse(by_name["placeholder_scaffold_modules"]["passed"])
            self.assertFalse(by_name["compliance_gap_markers"]["passed"])
            self.assertEqual(result["details"]["placeholder_modules"], ["retrieval"])

    def test_main_writes_json_and_nonzero_exit_on_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            _write(repo / "ROADMAP.md", "- [ ] open\n")
            _write(repo / "src" / "a" / "ROADMAP.md", "- [x] done\n")
            report = _write(
                repo / "audit" / "MATURITY.md",
                _base_report(technical=1, governance=0, zero_tests=0, zero_bench=0),
            )
            output = repo / "artifacts" / "maturity.json"

            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "check_maturity_exit_criteria.py",
                    "--repo-root",
                    str(repo),
                    "--maturity-report",
                    str(report),
                    "--output-json",
                    str(output),
                ]
                code = checker.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(
                any(c["name"] == "ga_blockers_technical" and c["value"] == 1 for c in payload["checks"])
            )


if __name__ == "__main__":
    unittest.main()
