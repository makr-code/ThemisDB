#!/usr/bin/env python3
"""Validate a Google Benchmark smoke artifact, not performance evidence."""

import argparse
import json
import math
import sys
from pathlib import Path


def wave4_required_cases(manifest):
    """Read W4A smoke cases without allowing an empty or truncated manifest."""
    if not isinstance(manifest, dict) or not isinstance(manifest.get("gates"), list):
        raise ValueError("invalid Wave 4 manifest gates")
    cases = {}
    for gate in manifest["gates"]:
        if not isinstance(gate, dict):
            raise ValueError("invalid Wave 4 gate")
        gate_id = gate.get("id")
        name = gate.get("benchmark_filter")
        if (not isinstance(gate_id, str) or not gate_id.startswith("W4A-")
                or not isinstance(name, str) or not name.strip()
                or gate.get("benchmark") != "bench_w4a_release_gates"
                or gate_id in cases or name in cases.values()):
            raise ValueError("invalid or duplicate Wave 4 gate ID or case")
        cases[gate_id] = name
    if not {f"W4A-{number:02d}" for number in range(1, 7)} <= cases.keys():
        raise ValueError("Wave 4 manifest lacks required W4A gate IDs")
    return set(cases.values())


def validate(data, required_cases=None):
    if not isinstance(data, dict):
        raise ValueError("result must be an object")
    if data.get("error_occurred") or data.get("error"):
        raise ValueError("run-level error")
    rows = data.get("benchmarks")
    if not isinstance(rows, list) or not rows:
        raise ValueError("no benchmark rows")
    iterations = 0
    observed = set()
    for row in rows:
        if not isinstance(row, dict) or not isinstance(row.get("name"), str) or not row["name"].strip():
            raise ValueError("invalid benchmark row or name")
        if (row.get("error_occurred") or row.get("error_message")
                or row.get("skipped") or row.get("skip_message")):
            raise ValueError(f"{row['name']}: failed or skipped")
        if row.get("run_type", "iteration") == "aggregate":
            continue
        if row.get("run_type", "iteration") != "iteration":
            raise ValueError(f"{row['name']}: unknown run type")
        count = row.get("iterations")
        if isinstance(count, bool) or not isinstance(count, int) or count <= 0:
            raise ValueError(f"{row['name']}: invalid iteration count")
        for field in ("real_time", "cpu_time"):
            value = row.get(field)
            if (isinstance(value, bool) or not isinstance(value, (int, float))
                    or not math.isfinite(value) or value < 0):
                raise ValueError(f"{row['name']}: invalid {field}")
        if row.get("time_unit") not in ("ns", "us", "ms", "s"):
            raise ValueError(f"{row['name']}: invalid time unit")
        if "run_name" in row and row["run_name"] != row["name"]:
            raise ValueError(f"{row['name']}: inconsistent iteration run_name")
        # W4A registers UseRealTime(), which Google Benchmark appends to names.
        name = row["name"].removesuffix("/real_time")
        observed.add(name)
        iterations += 1
    if not iterations:
        raise ValueError("no iteration rows")
    missing = (required_cases or set()) - observed
    if missing:
        raise ValueError("missing required cases: " + ", ".join(sorted(missing)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result", type=Path)
    parser.add_argument("--wave4-manifest", type=Path,
                        help="Require every W4A case declared in this manifest.")
    args = parser.parse_args()
    try:
        required = (wave4_required_cases(json.loads(args.wave4_manifest.read_text(encoding="utf-8")))
                    if args.wave4_manifest else None)
        validate(json.loads(args.result.read_text(encoding="utf-8")), required)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
        print(f"Invalid benchmark smoke artifact: {exc}", file=sys.stderr)
        return 1
    print("Smoke artifact validated; not performance evidence.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
