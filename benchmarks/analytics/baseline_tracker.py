#!/usr/bin/env python3
"""
Phase 5 Performance Baseline Tracker
Maintains historical baseline data and detects regressions.

Usage:
    python3 baseline_tracker.py --results phase5_results.json --baseline baseline.json
"""

import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, Optional


class BaselineTracker:
    """Tracks performance baselines across benchmark runs."""

    def __init__(self, baseline_file: str):
        self.baseline_file = Path(baseline_file)
        self.baselines = {}

    def load_baseline(self) -> bool:
        """Load existing baseline data."""
        if not self.baseline_file.exists():
            print(f"Note: Baseline file not found, will create new: {self.baseline_file}")
            self.baselines = {
                "metadata": {
                    "version": "1.0",
                    "created": datetime.now().isoformat(),
                    "description": "Phase 5 Performance Baselines",
                },
                "runs": [],
            }
            return True

        try:
            with open(self.baseline_file, "r") as f:
                self.baselines = json.load(f)
            return True
        except json.JSONDecodeError as e:
            print(f"Error reading baseline: {e}")
            return False

    def save_baseline(self) -> bool:
        """Save baseline data."""
        try:
            with open(self.baseline_file, "w") as f:
                json.dump(self.baselines, f, indent=2)
            return True
        except IOError as e:
            print(f"Error saving baseline: {e}")
            return False

    def add_run(self, results: Dict) -> None:
        """Add a new benchmark run to the baseline."""
        run_entry = {
            "timestamp": datetime.now().isoformat(),
            "passed": results.get("passed", 0),
            "failed": results.get("failed", 0),
            "results": results.get("results", {}),
        }
        self.baselines["runs"].append(run_entry)

    def compare_to_previous(self) -> Dict[str, float]:
        """Compare current results to previous run."""
        if len(self.baselines.get("runs", [])) < 2:
            return {}

        previous = self.baselines["runs"][-2]["results"]
        current = self.baselines["runs"][-1]["results"]
        comparison = {}

        for bench_name, current_value in current.items():
            if bench_name in previous:
                prev_value = previous[bench_name]
                if prev_value > 0:
                    percent_change = ((current_value - prev_value) / prev_value) * 100
                    comparison[bench_name] = {
                        "previous": prev_value,
                        "current": current_value,
                        "change_percent": percent_change,
                    }

        return comparison

    def generate_report(self) -> str:
        """Generate baseline comparison report."""
        report = []
        report.append("=" * 80)
        report.append("Phase 5 Performance Baseline Report")
        report.append("=" * 80)

        if not self.baselines.get("runs"):
            report.append("No benchmark runs recorded yet.")
            return "\n".join(report)

        # Summary
        latest_run = self.baselines["runs"][-1]
        report.append(f"\nLatest Run: {latest_run['timestamp']}")
        report.append(f"  Passed: {latest_run['passed']}")
        report.append(f"  Failed: {latest_run['failed']}")

        # Comparison
        if len(self.baselines["runs"]) >= 2:
            report.append("\nComparison to Previous Run:")
            report.append("-" * 80)

            comparison = self.compare_to_previous()

            regressions = {k: v for k, v in comparison.items() if v["change_percent"] < -5}
            improvements = {k: v for k, v in comparison.items() if v["change_percent"] > 5}
            stable = {
                k: v
                for k, v in comparison.items()
                if -5 <= v["change_percent"] <= 5
            }

            if improvements:
                report.append("\n✓ Improvements:")
                for bench, data in improvements.items():
                    change = data["change_percent"]
                    report.append(f"  {bench}: +{change:.1f}%")

            if regressions:
                report.append("\n✗ Regressions:")
                for bench, data in regressions.items():
                    change = data["change_percent"]
                    report.append(f"  {bench}: {change:.1f}%")

            report.append(f"\n✓ Stable: {len(stable)} benchmarks within ±5%")

        # Historical trend
        if len(self.baselines["runs"]) > 1:
            report.append("\nHistorical Summary:")
            report.append("-" * 80)
            report.append(f"Total runs: {len(self.baselines['runs'])}")

            all_passed = sum(r["passed"] for r in self.baselines["runs"])
            all_failed = sum(r["failed"] for r in self.baselines["runs"])
            report.append(f"Cumulative: {all_passed} passed, {all_failed} failed")

        report.append("\n" + "=" * 80)
        return "\n".join(report)


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Phase 5 Performance Baseline Tracker")
    parser.add_argument(
        "--results", help="Current results JSON file to add to baseline"
    )
    parser.add_argument("--baseline", default="phase5_baseline.json", help="Baseline JSON file")
    parser.add_argument("--report", action="store_true", help="Generate and print report")

    args = parser.parse_args()

    tracker = BaselineTracker(args.baseline)

    if not tracker.load_baseline():
        return 1

    # Add new results if provided
    if args.results:
        try:
            with open(args.results, "r") as f:
                results = json.load(f)
            tracker.add_run(results)
            if not tracker.save_baseline():
                return 1
            print(f"✓ Added run to baseline: {args.baseline}")
        except (IOError, json.JSONDecodeError) as e:
            print(f"Error processing results: {e}")
            return 1

    # Print report
    if args.report or True:  # Always print for now
        print(tracker.generate_report())

    return 0


if __name__ == "__main__":
    sys.exit(main())
