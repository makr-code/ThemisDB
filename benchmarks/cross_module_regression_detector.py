#!/usr/bin/env python3
"""
Cross-Module Performance Regression Detector for ThemisDB

Aggregates performance regression results from multiple benchmark modules
(e.g. acceleration, chimera) into a single unified cross-module report.
Designed to be called from the cross-module-performance-regression-ci.yml
GitHub Actions workflow after individual module detectors have produced
their JSON summaries.

Per-module JSON input format (produced by chimera_regression_detector.py
and performance_regression_detector.py)::

    {
        "summary": {
            "critical":     <int>,
            "major":        <int>,
            "minor":        <int>,
            "improvements": <int>,
            "total":        <int>
        },
        "comparisons": [
            {
                "severity":     "<none|minor|major|critical>",
                "is_regression": <bool>,
                ...
            },
            ...
        ]
    }

Exit codes:
    0  No blocking regressions across all modules.
    1  Blocking regressions detected at or above --fail-on threshold.
    2  Fatal error (bad arguments, missing files, JSON parse error).
"""
# ThemisDB - Hybrid Database System
# File: cross_module_regression_detector.py  Version: 0.0.14
# Status: ✅ Production Ready
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Any


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class ModuleResult:
    """Regression summary for a single benchmark module."""

    module: str
    critical: int
    major: int
    minor: int
    improvements: int
    total: int
    source_path: str

    def has_blocking_regressions(self, block_threshold: str = "major") -> bool:
        """Return True if this module has regressions at or above the threshold."""
        _order = ["minor", "major", "critical"]
        block_idx = _order.index(block_threshold)
        for level in _order[block_idx:]:
            if getattr(self, level) > 0:
                return True
        return False


@dataclass
class CrossModuleReport:
    """Aggregated regression report across all modules."""

    modules: List[ModuleResult] = field(default_factory=list)

    # Aggregated totals
    @property
    def total_critical(self) -> int:
        return sum(m.critical for m in self.modules)

    @property
    def total_major(self) -> int:
        return sum(m.major for m in self.modules)

    @property
    def total_minor(self) -> int:
        return sum(m.minor for m in self.modules)

    @property
    def total_improvements(self) -> int:
        return sum(m.improvements for m in self.modules)

    @property
    def total_comparisons(self) -> int:
        return sum(m.total for m in self.modules)

    def has_blocking_regressions(self, block_threshold: str = "major") -> bool:
        """Return True if ANY module has blocking regressions."""
        return any(m.has_blocking_regressions(block_threshold) for m in self.modules)

    def blocking_modules(self, block_threshold: str = "major") -> List[ModuleResult]:
        """Return all modules that have blocking regressions."""
        return [m for m in self.modules if m.has_blocking_regressions(block_threshold)]


# ---------------------------------------------------------------------------
# Detector
# ---------------------------------------------------------------------------

class CrossModuleRegressionDetector:
    """Aggregates per-module regression JSON summaries into a cross-module report."""

    def load_module_result(self, module: str, json_path: str) -> ModuleResult:
        """
        Load a single module's regression JSON summary.

        Args:
            module: Human-readable module name (e.g. 'acceleration', 'chimera').
            json_path: Path to the module's regression JSON summary file.

        Returns:
            ModuleResult populated from the JSON.

        Raises:
            FileNotFoundError: If json_path does not exist.
            json.JSONDecodeError: If json_path is not valid JSON.
            KeyError: If the JSON is missing required fields.
        """
        with open(json_path, encoding="utf-8") as fh:
            data = json.load(fh)

        summary = data["summary"]
        return ModuleResult(
            module=module,
            critical=int(summary.get("critical", 0)),
            major=int(summary.get("major", 0)),
            minor=int(summary.get("minor", 0)),
            improvements=int(summary.get("improvements", 0)),
            total=int(summary.get("total", 0)),
            source_path=json_path,
        )

    def aggregate(self, module_results: List[ModuleResult]) -> CrossModuleReport:
        """Aggregate a list of module results into a cross-module report."""
        return CrossModuleReport(modules=list(module_results))

    def generate_report(
        self,
        report: CrossModuleReport,
        fail_on: str = "major",
    ) -> str:
        """
        Generate a formatted plain-text cross-module regression report.

        Args:
            report: The aggregated cross-module report.
            fail_on: The blocking severity threshold.

        Returns:
            Formatted report string.
        """
        sep = "=" * 80
        lines: List[str] = []

        lines += [sep, "CROSS-MODULE PERFORMANCE REGRESSION REPORT", sep, ""]

        lines += [
            "MODULES ANALYSED:",
            *[f"  [{i+1}] {m.module}  (source: {m.source_path})"
              for i, m in enumerate(report.modules)],
            "",
        ]

        lines += [
            "AGGREGATE SUMMARY:",
            f"  Critical regressions : {report.total_critical}",
            f"  Major regressions    : {report.total_major}",
            f"  Minor regressions    : {report.total_minor}",
            f"  Improvements         : {report.total_improvements}",
            f"  Total compared       : {report.total_comparisons}",
            "",
            f"  Block threshold      : {fail_on}",
            "",
        ]

        # Per-module breakdown
        lines += [sep, "PER-MODULE BREAKDOWN", sep]
        for m in report.modules:
            status = (
                "FAIL" if m.has_blocking_regressions(fail_on) else
                ("WARN" if m.minor > 0 else "PASS")
            )
            lines.append(
                f"\n  Module   : {m.module}\n"
                f"  Status   : {status}\n"
                f"  Critical : {m.critical}\n"
                f"  Major    : {m.major}\n"
                f"  Minor    : {m.minor}\n"
                f"  Improve  : {m.improvements}\n"
                f"  Total    : {m.total}"
            )

        # Verdict
        lines += ["", sep, "VERDICT:"]
        blocking = report.blocking_modules(fail_on)
        if blocking:
            module_names = ", ".join(m.module for m in blocking)
            lines.append(
                f"  FAILED – Blocking regressions detected in: {module_names}"
            )
            lines.append(
                f"  {report.total_critical} critical, {report.total_major} major "
                f"regression(s) across {len(report.modules)} module(s)."
            )
        elif report.total_minor > 0:
            lines.append("  WARNING – Minor regressions detected across modules.")
            lines.append(
                f"  {report.total_minor} minor regression(s) across "
                f"{len(report.modules)} module(s)."
            )
        else:
            lines.append(
                "  PASSED – No significant regressions across "
                f"{len(report.modules)} module(s)."
            )
        lines.append(sep)

        return "\n".join(lines)

    def save_report(
        self,
        report_text: str,
        report: CrossModuleReport,
        output_path: Path,
        fail_on: str = "major",
    ) -> None:
        """Write the text report and a JSON summary to disk."""
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(report_text, encoding="utf-8")
        print(f"Report saved    : {output_path}")

        json_path = output_path.with_suffix(".json")
        json_summary = {
            "summary": {
                "critical":     report.total_critical,
                "major":        report.total_major,
                "minor":        report.total_minor,
                "improvements": report.total_improvements,
                "total":        report.total_comparisons,
                "modules":      len(report.modules),
                "blocking":     report.has_blocking_regressions(fail_on),
            },
            "modules": [
                {
                    "module":       m.module,
                    "critical":     m.critical,
                    "major":        m.major,
                    "minor":        m.minor,
                    "improvements": m.improvements,
                    "total":        m.total,
                    "blocking":     m.has_blocking_regressions(fail_on),
                    "source_path":  m.source_path,
                }
                for m in report.modules
            ],
        }
        json_path.write_text(json.dumps(json_summary, indent=2), encoding="utf-8")
        print(f"JSON summary    : {json_path}")


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------

def _build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Aggregate per-module performance regression JSON summaries into a "
            "cross-module report."
        )
    )
    p.add_argument(
        "--module",
        dest="modules",
        metavar="NAME:PATH",
        action="append",
        required=True,
        help=(
            "Module name and path to its regression JSON summary, "
            "e.g. --module acceleration:results/accel.json. "
            "Repeat for each module."
        ),
    )
    p.add_argument(
        "--output",
        default="cross_module_regression_report.txt",
        help="Path for the plain-text report (default: cross_module_regression_report.txt).",
    )
    p.add_argument(
        "--fail-on",
        default="major",
        choices=["minor", "major", "critical"],
        help="Exit 1 if regressions at or above this severity are found in any module.",
    )
    return p


def main(argv: Optional[List[str]] = None) -> int:  # noqa: D401
    """CLI entry-point; returns an exit code."""
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    detector = CrossModuleRegressionDetector()
    module_results: List[ModuleResult] = []

    for spec in args.modules:
        if ":" not in spec:
            print(
                f"ERROR: --module value must be NAME:PATH, got: {spec!r}",
                file=sys.stderr,
            )
            return 2

        name, path = spec.split(":", 1)
        name = name.strip()
        path = path.strip()

        try:
            result = detector.load_module_result(name, path)
            print(f"Loaded module   : {name}  ({path})")
            module_results.append(result)
        except FileNotFoundError as exc:
            print(f"ERROR loading module {name!r}: {exc}", file=sys.stderr)
            return 2
        except (json.JSONDecodeError, KeyError) as exc:
            print(
                f"ERROR parsing module {name!r} JSON ({path}): {exc}",
                file=sys.stderr,
            )
            return 2

    if not module_results:
        print("ERROR: no module results loaded.", file=sys.stderr)
        return 2

    aggregated = detector.aggregate(module_results)
    report_text = detector.generate_report(aggregated, args.fail_on)
    output_path = Path(args.output)
    detector.save_report(report_text, aggregated, output_path, args.fail_on)

    print(report_text)

    if aggregated.has_blocking_regressions(args.fail_on):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
