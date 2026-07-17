#!/usr/bin/env python3
"""Compare EPIC 3 Phase 5 benchmark results against the gate manifest."""

from __future__ import annotations

import argparse
import json
import operator
import pathlib
import sys
from typing import Any


OPERATORS = {
    "<=": operator.le,
    ">=": operator.ge,
    "<": operator.lt,
    ">": operator.gt,
    "==": operator.eq,
}


_SCRIPT_DIR = pathlib.Path(__file__).resolve().parent


def load_json(path: pathlib.Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def normalize_results(results_payload: dict[str, Any]) -> dict[str, dict[str, float]]:
    results = {}
    for entry in results_payload.get("results", []):
        profile_id = entry.get("profile_id")
        metrics = entry.get("metrics", {})
        if profile_id:
            results[profile_id] = metrics
    return results


def evaluate_gate(gate: dict[str, Any], metrics_by_profile: dict[str, dict[str, float]]) -> dict[str, Any]:
    profile_id = gate["profile_id"]
    metric_name = gate["metric"]
    operator_symbol = gate["operator"]

    comparator = OPERATORS.get(operator_symbol)
    if comparator is None:
        return {
            "gate": gate["id"],
            "profile_id": profile_id,
            "metric": metric_name,
            "status": "invalid_operator",
            "message": f"unknown operator '{operator_symbol}' in gate '{gate['id']}'; "
                       f"supported: {sorted(OPERATORS)}",
        }

    profile_metrics = metrics_by_profile.get(profile_id)
    if profile_metrics is None:
        return {
            "gate": gate["id"],
            "profile_id": profile_id,
            "metric": metric_name,
            "status": "missing_profile",
            "message": f"missing profile result: {profile_id}",
        }

    if metric_name not in profile_metrics:
        return {
            "gate": gate["id"],
            "profile_id": profile_id,
            "metric": metric_name,
            "status": "missing_metric",
            "message": f"missing metric '{metric_name}' for profile '{profile_id}'",
        }

    actual_raw = profile_metrics[metric_name]
    threshold_raw = gate["threshold"]
    try:
        actual_value = float(actual_raw)
        threshold = float(threshold_raw)
    except (TypeError, ValueError) as exc:
        return {
            "gate": gate["id"],
            "profile_id": profile_id,
            "metric": metric_name,
            "status": "type_error",
            "message": (
                f"could not coerce metric values to float for gate '{gate['id']}': {exc}"
            ),
        }
    passed = comparator(actual_value, threshold)
    return {
        "gate": gate["id"],
        "profile_id": profile_id,
        "metric": metric_name,
        "status": "pass" if passed else "fail",
        "actual": actual_value,
        "operator": operator_symbol,
        "threshold": threshold,
        "unit": gate.get("unit", ""),
    }


def build_summary(manifest: dict[str, Any], profiles: dict[str, Any], results_payload: dict[str, Any]) -> dict[str, Any]:
    metrics_by_profile = normalize_results(results_payload)
    profile_ids = {profile["id"] for profile in profiles.get("profiles", [])}

    evaluations = []
    for gate in manifest.get("gates", []):
        evaluations.append(evaluate_gate(gate, metrics_by_profile))

    missing_profiles = sorted(profile_ids - set(metrics_by_profile))
    failed = [entry for entry in evaluations if entry["status"] != "pass"]

    return {
        "module": manifest.get("module"),
        "phase": manifest.get("phase"),
        "canonical_seed": manifest.get("canonical_seed"),
        "gate_count": len(evaluations),
        "failed_gate_count": len(failed),
        "missing_profiles": missing_profiles,
        "status": "pass" if not failed and not missing_profiles else "fail",
        "evaluations": evaluations,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest",
        default=_SCRIPT_DIR / "release_gate_manifest_epic3.json",
        type=pathlib.Path,
        help="Path to the EPIC 3 gate manifest JSON (default: alongside this script).",
    )
    parser.add_argument(
        "--profiles",
        default=_SCRIPT_DIR / "phase5_workload_profiles.json",
        type=pathlib.Path,
        help="Path to the EPIC 3 workload profile JSON (default: alongside this script).",
    )
    parser.add_argument(
        "--input",
        required=True,
        type=pathlib.Path,
        help="Path to the benchmark results JSON bundle.",
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        help="Optional path for the generated summary JSON.",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Return non-zero when any gate fails or expected data is missing.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = load_json(args.manifest)
    profiles = load_json(args.profiles)
    results_payload = load_json(args.input)

    summary = build_summary(manifest, profiles, results_payload)

    rendered = json.dumps(summary, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    else:
        print(rendered)

    if args.strict and summary["status"] != "pass":
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
