#!/usr/bin/env python3
"""Inspect declared benchmark identity and raw evidence; not a performance gate."""

import argparse
from datetime import datetime, timedelta
import hashlib
import json
import math
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "benchmarks" / "scripts"))
from scientific_evaluation_framework import BaselineFreeze
from validate_benchmark_smoke import validate as validate_smoke
from capture_benchmark_identity import validate_host_fingerprint

HASH = re.compile(r"[0-9a-f]{64}\Z")
REVISION = re.compile(r"(?:[0-9a-f]{40}|[0-9a-f]{64})\Z")
WORKLOAD = re.compile(r"themisdb:(?:micro|component|integration|e2e|reference_workload)/[a-z0-9._-]+@[1-9][0-9]*\Z")
UNKNOWN = {"unknown", "unset", "n/a", "na", "not available", "not_available", "none known"}
ABSENT = object()
HOST_SCOPE = ("OS, CPU model, architecture, logical processor count, and reported memory; "
              "storage and accelerators are not observed")


def _object(value, keys, label):
    if not isinstance(value, dict) or set(value) != set(keys):
        raise ValueError(f"{label}: expected exactly {', '.join(keys)}")
    return value


def _text(value, label, allow_none=False):
    if (not isinstance(value, str) or not value.strip() or value != value.strip()
            or value.casefold() in UNKNOWN or (value.casefold() == "none" and not allow_none)):
        raise ValueError(f"{label}: missing or unknown")
    return value


def _hash(value, label):
    if not isinstance(value, str) or not HASH.fullmatch(value):
        raise ValueError(f"{label}: expected lowercase SHA-256")
    return value


def _integer(value, label, minimum=0):
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise ValueError(f"{label}: invalid integer")
    return value


def _no_duplicates(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path):
    raw = path.read_bytes()
    value = json.loads(raw.decode("utf-8-sig"), object_pairs_hook=_no_duplicates,
                       parse_constant=lambda value: (_ for _ in ()).throw(ValueError(f"invalid JSON number: {value}")))
    return value, hashlib.sha256(raw).hexdigest()


def validate_policy(policy):
    _object(policy, ("schema_version", "workload_id", "repetitions", "required_cases"), "policy")
    if type(policy["schema_version"]) is not int or policy["schema_version"] != 1:
        raise ValueError("policy: unsupported schema version")
    if not isinstance(policy["workload_id"], str) or not WORKLOAD.fullmatch(policy["workload_id"]):
        raise ValueError("policy: invalid workload ID")
    _integer(policy["repetitions"], "policy repetitions", 1)
    cases = policy["required_cases"]
    if not isinstance(cases, dict) or not cases:
        raise ValueError("policy: no required cases")
    for name, metrics in cases.items():
        _text(name, "case name")
        if (not isinstance(metrics, list) or not metrics
                or any(not isinstance(metric, str) or not metric.strip()
                       or metric != metric.strip() for metric in metrics)
                or len(set(metrics)) != len(metrics)):
            raise ValueError(f"{name}: invalid required metrics")
    return cases


def validate_catalog(catalog, workload_id):
    if (not isinstance(catalog, dict) or type(catalog.get("schema_version")) is not int
            or catalog["schema_version"] != 1 or not isinstance(catalog.get("profiles"), list)):
        raise ValueError("invalid workload catalog")
    profiles = [p for p in catalog["profiles"]
                if isinstance(p, dict) and p.get("workload_id") == workload_id]
    if len(profiles) != 1 or profiles[0].get("status") != "source-described":
        raise ValueError("workload must resolve uniquely to a source-described profile")


def validate_sidecar(sidecar, result_hash, policy, policy_hash, catalog_hash):
    _object(sidecar, ("schema_version", "baseline_freeze", "hardware", "build", "workload", "execution"), "sidecar")
    if type(sidecar["schema_version"]) is not int or sidecar["schema_version"] != 1:
        raise ValueError("sidecar: unsupported schema version")
    freeze_keys = ("compiler", "compiler_flags", "preset", "hardware_profile", "os_image")
    freeze = _object(sidecar["baseline_freeze"], freeze_keys, "baseline_freeze")
    for key in freeze_keys:
        _text(freeze[key], key)
    BaselineFreeze(**freeze).validate()

    hardware = _object(sidecar["hardware"], (
        "cpu_model", "architecture", "logical_processors", "memory_bytes",
        "storage_identity", "accelerator_identity"), "hardware")
    for key in ("cpu_model", "architecture", "storage_identity", "accelerator_identity"):
        _text(hardware[key], key, allow_none=key in ("storage_identity", "accelerator_identity"))
    for key in ("logical_processors", "memory_bytes"):
        _integer(hardware[key], key, 1)

    build = _object(sidecar["build"], ("build_type", "configuration_sha256", "dependencies_sha256"), "build")
    _text(build["build_type"], "build_type")
    for key in ("configuration_sha256", "dependencies_sha256"):
        _hash(build[key], key)

    workload = _object(sidecar["workload"], (
        "workload_id", "catalog_sha256", "dataset_sha256", "configuration_sha256",
        "seed", "warmup", "minimum_measurement_time", "repetitions"), "workload")
    if workload["workload_id"] != policy["workload_id"]:
        raise ValueError("workload ID differs from policy")
    for key in ("catalog_sha256", "dataset_sha256", "configuration_sha256"):
        _hash(workload[key], key)
    if workload["catalog_sha256"] != catalog_hash:
        raise ValueError("workload catalog hash mismatch")
    _integer(workload["seed"], "seed")
    for key in ("warmup", "minimum_measurement_time"):
        _text(workload[key], key)
    if _integer(workload["repetitions"], "repetitions", 1) != policy["repetitions"]:
        raise ValueError("workload repetitions differ from policy")

    execution = _object(sidecar["execution"], (
        "status", "exit_code", "run_id", "source_revision", "worktree_dirty",
        "binary_sha256", "runner_sha256", "result_sha256", "policy_sha256"), "execution")
    if (execution["status"] != "completed" or type(execution["exit_code"]) is not int
            or execution["exit_code"] != 0 or execution["worktree_dirty"] is not False):
        raise ValueError("execution not clean and successful")
    _text(execution["run_id"], "run_id")
    revision = execution["source_revision"]
    if not isinstance(revision, str) or not REVISION.fullmatch(revision):
        raise ValueError("invalid source revision")
    for key in ("binary_sha256", "runner_sha256", "result_sha256", "policy_sha256"):
        _hash(execution[key], key)
    if execution["result_sha256"] != result_hash:
        raise ValueError("raw result hash mismatch")
    if execution["policy_sha256"] != policy_hash:
        raise ValueError("policy hash mismatch")
    return {key: sidecar[key] for key in ("baseline_freeze", "hardware", "build", "workload")}


def validate_repetitions(data, cases, repetitions):
    validate_smoke(data)
    seen = {name: set() for name in cases}
    for row in data["benchmarks"]:
        if row.get("run_type", "iteration") == "aggregate":
            continue
        name = row["name"]
        if name not in cases:
            raise ValueError(f"unexpected iteration case: {name}")
        if type(row.get("repetitions")) is not int or row["repetitions"] != repetitions:
            raise ValueError(f"{name}: repetition count mismatch")
        index = _integer(row.get("repetition_index"), f"{name} repetition_index")
        if index >= repetitions or index in seen[name]:
            raise ValueError(f"{name}: duplicate or out-of-range repetition")
        seen[name].add(index)
        for metric in cases[name]:
            value = row.get(metric)
            if (isinstance(value, bool) or not isinstance(value, (int, float))
                    or not math.isfinite(value)):
                raise ValueError(f"{name}: missing or invalid metric {metric}")
    for name, indices in seen.items():
        if len(indices) != repetitions:
            raise ValueError(f"{name}: incomplete repetitions")


def validate_observation(observation, sidecar, result_hash, catalog_hash, label):
    _object(observation, ("schema_version", "identity_scope", "captured_at_utc",
                          "host_fingerprint_scope", "workload", "artifacts", "host"), label)
    if type(observation["schema_version"]) is not int or observation["schema_version"] != 1:
        raise ValueError(f"{label}: unsupported schema version")
    if observation["identity_scope"] != "local-observation-not-attestation":
        raise ValueError(f"{label}: invalid identity scope")
    if observation["host_fingerprint_scope"] != HOST_SCOPE:
        raise ValueError(f"{label}: invalid host fingerprint scope")
    timestamp = _text(observation["captured_at_utc"], f"{label} capture time")
    try:
        parsed = datetime.fromisoformat(timestamp)
    except ValueError as exc:
        raise ValueError(f"{label}: invalid capture time") from exc
    if parsed.tzinfo is None or parsed.utcoffset() != timedelta(0) or "T" not in timestamp:
        raise ValueError(f"{label}: capture time must be UTC")

    workload = _object(observation["workload"], ("workload_id", "catalog_sha256"),
                       f"{label} workload")
    if not isinstance(workload["workload_id"], str) or not WORKLOAD.fullmatch(workload["workload_id"]):
        raise ValueError(f"{label}: invalid workload ID")
    _hash(workload["catalog_sha256"], f"{label} catalog hash")
    artifacts = _object(observation["artifacts"], (
        "executable_sha256", "runner_sha256", "raw_result_sha256", "configuration_sha256"),
        f"{label} artifacts")
    for key, value in artifacts.items():
        _hash(value, f"{label} {key}")
    host = _object(observation["host"], ("os", "hardware"), f"{label} host")
    validate_host_fingerprint(host["os"], host["hardware"])
    for key in ("system", "release", "distribution_version"):
        _text(host["os"][key], f"{label} OS {key}")
    if host["os"]["system"] == "Linux":
        _text(host["os"]["distribution"], f"{label} OS distribution")
    for key in ("architecture", "cpu_model"):
        _text(host["hardware"][key], f"{label} hardware {key}")

    comparisons = (
        ("workload_id", workload["workload_id"], sidecar["workload"]["workload_id"]),
        ("catalog_sha256", workload["catalog_sha256"], catalog_hash),
        ("catalog_sha256", workload["catalog_sha256"], sidecar["workload"]["catalog_sha256"]),
        ("executable_sha256", artifacts["executable_sha256"], sidecar["execution"]["binary_sha256"]),
        ("runner_sha256", artifacts["runner_sha256"], sidecar["execution"]["runner_sha256"]),
        ("raw_result_sha256", artifacts["raw_result_sha256"], result_hash),
        ("raw_result_sha256", artifacts["raw_result_sha256"], sidecar["execution"]["result_sha256"]),
    )
    for field, observed, declared in comparisons:
        if observed != declared:
            raise ValueError(f"{label}: {field} mismatch")
    for key in ("cpu_model", "architecture", "logical_processors", "memory_bytes"):
        if host["hardware"][key] != sidecar["hardware"][key]:
            raise ValueError(f"{label}: hardware {key} mismatch")


def inspect(policy, policy_hash, catalog, catalog_hash, baseline_sidecar, baseline, baseline_hash,
            current_sidecar, current, current_hash, baseline_observation=ABSENT,
            current_observation=ABSENT):
    if (baseline_observation is ABSENT) != (current_observation is ABSENT):
        raise ValueError("baseline and current observations must be provided together")
    cases = validate_policy(policy)
    validate_catalog(catalog, policy["workload_id"])
    baseline_identity = validate_sidecar(baseline_sidecar, baseline_hash, policy, policy_hash, catalog_hash)
    current_identity = validate_sidecar(current_sidecar, current_hash, policy, policy_hash, catalog_hash)
    for key in baseline_identity:
        if baseline_identity[key] != current_identity[key]:
            raise ValueError(f"{key}: baseline/current identity mismatch")
    if baseline_sidecar["execution"]["run_id"] == current_sidecar["execution"]["run_id"]:
        raise ValueError("baseline and current run IDs must differ")
    if baseline_sidecar["execution"]["runner_sha256"] != current_sidecar["execution"]["runner_sha256"]:
        raise ValueError("runner hash mismatch")
    validate_repetitions(baseline, cases, policy["repetitions"])
    validate_repetitions(current, cases, policy["repetitions"])
    if baseline_observation is not ABSENT:
        validate_observation(baseline_observation, baseline_sidecar, baseline_hash,
                             catalog_hash, "baseline observation")
        validate_observation(current_observation, current_sidecar, current_hash,
                             catalog_hash, "current observation")
        if baseline_observation["host"]["os"] != current_observation["host"]["os"]:
            raise ValueError("observation OS: baseline/current mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("policy", "catalog", "baseline-sidecar", "baseline-result",
                   "current-sidecar", "current-result"):
        parser.add_argument("--" + option, type=Path, required=True)
    parser.add_argument("--baseline-observation", type=Path)
    parser.add_argument("--current-observation", type=Path)
    args = parser.parse_args()
    try:
        if (args.baseline_observation is None) != (args.current_observation is None):
            raise ValueError("baseline and current observations must be provided together")
        policy, policy_hash = read_json(args.policy)
        catalog, catalog_hash = read_json(args.catalog)
        baseline_sidecar, _ = read_json(args.baseline_sidecar)
        baseline, baseline_hash = read_json(args.baseline_result)
        current_sidecar, _ = read_json(args.current_sidecar)
        current, current_hash = read_json(args.current_result)
        observations = {}
        if args.baseline_observation is not None:
            observations = {
                "baseline_observation": read_json(args.baseline_observation)[0],
                "current_observation": read_json(args.current_observation)[0],
            }
        inspect(policy, policy_hash, catalog, catalog_hash, baseline_sidecar, baseline, baseline_hash,
                current_sidecar, current, current_hash, **observations)
    except (OSError, UnicodeError, ValueError, OverflowError) as exc:
        print(f"Evidence preflight failed: {exc}", file=sys.stderr)
        return 1
    print("Declared identity and raw evidence are consistent; NOT a performance or release decision.")
    if args.baseline_observation is not None:
        print("Local observations match the comparable declarations; NOT attestation or host assignment.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
