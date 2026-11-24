#!/usr/bin/env python3
"""Fail-closed completeness validation for the Wave 4A Google Benchmark smoke."""

import argparse
import json
from pathlib import Path
import sys


def _reject_json_constant(value):
    raise ValueError(f"non-standard JSON constant: {value}")


def _load_json(path):
    with Path(path).open(encoding="utf-8") as stream:
        return json.load(stream, parse_constant=_reject_json_constant)


def _read_contract(contract):
    if not isinstance(contract, dict):
        raise ValueError("smoke contract must be a JSON object")
    if contract.get("schema") != "themisdb.wave4_smoke_contract":
        raise ValueError("unsupported smoke contract schema")
    if type(contract.get("schema_version")) is not int or contract["schema_version"] != 1:
        raise ValueError("unsupported smoke contract version")
    if contract.get("benchmark_filter") != "W4A":
        raise ValueError("benchmark_filter must be W4A for this smoke")

    repetitions = contract.get("required_repetitions")
    if isinstance(repetitions, bool) or not isinstance(repetitions, int) or repetitions < 1:
        raise ValueError("required_repetitions must be a positive integer")

    cases = contract.get("expected_cases")
    if (not isinstance(cases, list) or not cases
            or any(not isinstance(case, str) or not case for case in cases)):
        raise ValueError("expected_cases must be a non-empty list of names")
    if len(cases) != len(set(cases)):
        raise ValueError("expected_cases contains duplicate names")

    return cases, repetitions


def validate_results(results, contract):
    """Return all completeness errors; aggregate rows are never repetitions."""
    try:
        expected_cases, repetitions = _read_contract(contract)
    except ValueError as exc:
        return [f"invalid smoke contract: {exc}"]

    if not isinstance(results, dict):
        return ["Google Benchmark output must be a JSON object"]
    rows = results.get("benchmarks")
    if not isinstance(rows, list) or not rows:
        return ["Google Benchmark output has no benchmark rows"]

    expected = set(expected_cases)
    observed = {name: set() for name in expected_cases}
    skipped = {}
    errors = []

    for row_index, row in enumerate(rows):
        if not isinstance(row, dict):
            errors.append(f"benchmark row {row_index} is not an object")
            continue

        name = row.get("name")
        if not isinstance(name, str) or not name:
            errors.append(f"benchmark row {row_index} has no valid name")
            continue
        case_name = name.removesuffix("/real_time")

        for field in ("error_occurred", "skipped"):
            if field in row and not isinstance(row[field], bool):
                errors.append(f"{name}: {field} must be a boolean")

        if row.get("error_occurred") is True:
            detail = row.get("error_message")
            errors.append(f"{name}: benchmark error{': ' + str(detail) if detail else ''}")
            continue

        if row.get("skipped") is True:
            detail = row.get("skip_message")
            skipped.setdefault(case_name, []).append(
                str(detail) if detail else "no skip reason supplied"
            )
            if case_name not in expected:
                errors.append(f"{name}: unexpected skipped benchmark case")
            continue

        run_type = row.get("run_type")
        if run_type == "aggregate":
            aggregate_case = row.get("run_name")
            if not isinstance(aggregate_case, str) or not aggregate_case:
                errors.append(f"{name}: aggregate row has no valid run_name")
            elif aggregate_case.removesuffix("/real_time") not in expected:
                errors.append(f"{name}: aggregate references unexpected case {aggregate_case}")
            aggregate_repetitions = row.get("repetitions")
            if (isinstance(aggregate_repetitions, bool)
                    or not isinstance(aggregate_repetitions, int)
                    or aggregate_repetitions != repetitions):
                errors.append(
                    f"{name}: aggregate repetitions {aggregate_repetitions!r} "
                    f"do not match required count {repetitions}"
                )
            continue
        if run_type != "iteration":
            errors.append(f"{name}: unexpected or missing run_type {run_type!r}")
            continue
        if case_name not in expected:
            errors.append(f"{name}: unexpected benchmark case")
            continue

        reported_repetitions = row.get("repetitions")
        if (isinstance(reported_repetitions, bool)
                or not isinstance(reported_repetitions, int)
                or reported_repetitions != repetitions):
            errors.append(
                f"{name}: reported repetitions {reported_repetitions!r} "
                f"do not match required count {repetitions}"
            )
            continue

        repetition_index = row.get("repetition_index")
        if (isinstance(repetition_index, bool)
                or not isinstance(repetition_index, int)
                or repetition_index < 0):
            errors.append(f"{name}: raw iteration has no valid repetition_index")
            continue
        if repetition_index >= repetitions:
            errors.append(
                f"{name}: repetition_index {repetition_index} is outside "
                f"the required range 0..{repetitions - 1}"
            )
            continue
        if repetition_index in observed[case_name]:
            errors.append(f"{name}: duplicate repetition_index {repetition_index}")
            continue
        observed[case_name].add(repetition_index)

    for name in expected_cases:
        if name in skipped:
            reasons = ", ".join(skipped[name])
            errors.append(f"{name}: skipped and is not complete ({reasons})")
        missing = sorted(set(range(repetitions)) - observed[name])
        if missing:
            errors.append(f"{name}: missing raw repetitions {missing}")

    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="Google Benchmark JSON output")
    parser.add_argument(
        "--contract",
        default=str(Path(__file__).with_name("smoke_contract.json")),
        help="explicit expected-case and repetition contract",
    )
    args = parser.parse_args(argv)

    try:
        contract = _load_json(args.contract)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"Invalid smoke contract: {exc}", file=sys.stderr)
        return 2
    try:
        results = _load_json(args.input)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"Invalid Google Benchmark output: {exc}", file=sys.stderr)
        return 1

    errors = validate_results(results, contract)
    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        return 1

    cases, repetitions = _read_contract(contract)
    print(
        f"PASS: {len(cases)} W4A smoke cases each have {repetitions} raw repetitions; "
        "aggregate rows excluded. This is execution completeness, not performance evidence."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
