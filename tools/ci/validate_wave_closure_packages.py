#!/usr/bin/env python3
"""Validate Wave A→D closure package manifests and ordering."""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

REQUIRED_FIELDS = {
    "generated_at",
    "wave",
    "module",
    "workflow",
    "run_id",
    "overall_status",
    "gates",
    "evidence_references",
    "source_validation",
    "notes",
}

WAVE_ORDER = ["A", "B", "C", "D"]
REQUIRED_SOURCE_FLAGS = ["code", "tests", "ci", "benchmarks"]
DEFAULT_POLICY_PATH = Path("audit/evidence/waves/closure_manifest_policy.json")


def normalize_wave(value: str) -> str | None:
    token = value.strip().upper()
    if token.startswith("WAVE "):
        token = token.replace("WAVE ", "", 1)
    if token in WAVE_ORDER:
        return token
    return None


def load_manifest(path: Path) -> tuple[dict[str, Any] | None, list[str]]:
    errors: list[str] = []
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return None, [f"{path}: unable to parse json ({exc})"]

    missing = sorted(REQUIRED_FIELDS - set(payload.keys()))
    if missing:
        errors.append(f"{path}: missing required fields: {', '.join(missing)}")

    wave_code = normalize_wave(str(payload.get("wave", "")))
    if wave_code is None:
        errors.append(f"{path}: unsupported wave value '{payload.get('wave')}'")
    else:
        payload["wave_code"] = wave_code

    if not isinstance(payload.get("gates"), dict):
        errors.append(f"{path}: 'gates' must be an object")
    if not isinstance(payload.get("evidence_references"), dict):
        errors.append(f"{path}: 'evidence_references' must be an object")

    source_validation = payload.get("source_validation")
    if not isinstance(source_validation, dict):
        errors.append(f"{path}: 'source_validation' must be an object")
    else:
        for key in REQUIRED_SOURCE_FLAGS:
            if key not in source_validation:
                errors.append(f"{path}: source_validation missing key '{key}'")
            elif not isinstance(source_validation[key], bool):
                errors.append(f"{path}: source_validation['{key}'] must be boolean")

    run_id = str(payload.get("run_id", "")).strip()
    if not run_id:
        errors.append(f"{path}: run_id must not be empty")

    return payload, errors


def check_markdown_sidecar(path: Path) -> list[str]:
    errors: list[str] = []
    md_path = path.with_suffix(".md")
    if not md_path.is_file():
        errors.append(f"{path}: missing markdown sidecar '{md_path.name}'")
        return errors
    try:
        if not md_path.read_text(encoding="utf-8").strip():
            errors.append(f"{path}: markdown sidecar '{md_path.name}' is empty")
    except OSError as exc:
        errors.append(f"{path}: unable to read markdown sidecar '{md_path.name}' ({exc})")
    return errors


def load_policy(path: Path | None) -> tuple[list[dict[str, str]], list[str]]:
    if path is None:
        return [], []
    if not path.exists():
        return [], [f"Policy file not found: {path}"]
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [], [f"{path}: unable to parse policy json ({exc})"]

    required = payload.get("required_manifests")
    if not isinstance(required, list):
        return [], [f"{path}: 'required_manifests' must be an array"]

    entries: list[dict[str, str]] = []
    errors: list[str] = []
    for idx, item in enumerate(required):
        if not isinstance(item, dict):
            errors.append(f"{path}: required_manifests[{idx}] must be an object")
            continue
        file_name = str(item.get("file", "")).strip()
        wave = str(item.get("wave", "")).strip()
        module = str(item.get("module", "")).strip()
        if not file_name:
            errors.append(f"{path}: required_manifests[{idx}] missing 'file'")
        if normalize_wave(wave) is None:
            errors.append(f"{path}: required_manifests[{idx}] has invalid wave '{wave}'")
        if not module:
            errors.append(f"{path}: required_manifests[{idx}] missing 'module'")
        if file_name and module and normalize_wave(wave) is not None:
            entries.append({"file": file_name, "wave": wave, "module": module})
    return entries, errors


def validate_required_manifests(
    required_entries: list[dict[str, str]],
    loaded_manifests: dict[str, dict[str, Any]],
    manifest_dir: Path,
    invalid_manifest_names: set[str] | None = None,
) -> list[str]:
    errors: list[str] = []
    for entry in required_entries:
        file_name = entry["file"]
        expected_wave = entry["wave"]
        expected_wave_code = normalize_wave(expected_wave)
        expected_module = entry["module"]

        payload = loaded_manifests.get(file_name)
        if payload is None:
            if invalid_manifest_names and file_name in invalid_manifest_names:
                # File exists but failed to parse or is missing wave_code; errors
                # already reported during loading — skip duplicate "missing" report.
                continue
            errors.append(f"Missing required closure manifest: {manifest_dir / file_name}")
            continue

        actual_wave = str(payload.get("wave", "")).strip()
        actual_wave_code = normalize_wave(actual_wave)
        actual_module = str(payload.get("module", "")).strip()

        if actual_wave_code != expected_wave_code:
            errors.append(
                f"{manifest_dir / file_name}: expected wave '{expected_wave}', found '{actual_wave}'"
            )
        if actual_module != expected_module:
            errors.append(
                f"{manifest_dir / file_name}: expected module '{expected_module}', found '{actual_module}'"
            )
    return errors


def wave_status(manifests: list[dict[str, Any]]) -> str:
    if not manifests:
        return "missing"

    all_evidence = True
    for manifest in manifests:
        if manifest.get("overall_status") != "evidence-captured":
            all_evidence = False
            break
        source_validation = manifest.get("source_validation", {})
        if not all(source_validation.get(flag) is True for flag in REQUIRED_SOURCE_FLAGS):
            all_evidence = False
            break

    return "evidence-captured" if all_evidence else "open"


def check_order(status_map: dict[str, str]) -> list[str]:
    violations: list[str] = []
    for idx, wave in enumerate(WAVE_ORDER):
        if status_map.get(wave) != "evidence-captured":
            continue
        for previous in WAVE_ORDER[:idx]:
            if status_map.get(previous) != "evidence-captured":
                violations.append(
                    f"Wave {wave} marked evidence-captured while Wave {previous} is {status_map.get(previous, 'missing')}"
                )
    return violations


def render_markdown(result: dict[str, Any]) -> str:
    lines = [
        "# Wave Closure Validation Report",
        "",
        f"- Generated at: {result['generated_at']}",
        f"- Validation result: {'PASS' if result['pass'] else 'FAIL'}",
        "",
        "## Wave Status",
        "",
        "| Wave | Status |",
        "| --- | --- |",
    ]
    for wave in WAVE_ORDER:
        lines.append(f"| Wave {wave} | `{result['wave_status'].get(wave, 'missing')}` |")

    lines.extend([
        "",
        "## Manifests",
        "",
        "| File | Wave | Module | Overall status | Run ID |",
        "| --- | --- | --- | --- | --- |",
    ])

    manifests = result.get("manifests", [])
    if manifests:
        for item in manifests:
            lines.append(
                f"| `{item['file']}` | `{item['wave']}` | `{item['module']}` | `{item['overall_status']}` | `{item['run_id']}` |"
            )
    else:
        lines.append("| _none_ | n/a | n/a | n/a | n/a |")

    lines.extend(["", "## Violations", ""])
    violations = result.get("violations", [])
    if violations:
        for violation in violations:
            lines.append(f"- ❌ {violation}")
    else:
        lines.append("- ✅ None")

    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest-dir",
        default="audit/evidence/waves/manifests",
        help="Directory containing *_closure_manifest.json files",
    )
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-md")
    parser.add_argument(
        "--policy-file",
        help="Optional policy json with required_manifests list",
    )
    args = parser.parse_args()

    manifest_dir = Path(args.manifest_dir)
    manifest_files = sorted(manifest_dir.glob("*_closure_manifest.json"))
    default_manifest_dir = Path("audit/evidence/waves/manifests")
    use_default_policy = (
        args.policy_file is None
        and DEFAULT_POLICY_PATH.exists()
        and manifest_dir.resolve() == default_manifest_dir.resolve()
    )
    policy_file = Path(args.policy_file) if args.policy_file else (
        DEFAULT_POLICY_PATH if use_default_policy else None
    )

    errors: list[str] = []
    manifests_by_wave: dict[str, list[dict[str, Any]]] = {wave: [] for wave in WAVE_ORDER}
    manifest_rows: list[dict[str, str]] = []
    loaded_manifests: dict[str, dict[str, Any]] = {}
    invalid_manifest_names: set[str] = set()

    if not manifest_files:
        errors.append(f"No closure manifests found in {manifest_dir}")

    for manifest_file in manifest_files:
        errors.extend(check_markdown_sidecar(manifest_file))
        payload, load_errors = load_manifest(manifest_file)
        errors.extend(load_errors)
        if payload is None or "wave_code" not in payload:
            invalid_manifest_names.add(manifest_file.name)
            continue

        loaded_manifests[manifest_file.name] = payload
        wave_code = payload["wave_code"]
        manifests_by_wave[wave_code].append(payload)
        manifest_rows.append(
            {
                "file": str(manifest_file),
                "wave": str(payload.get("wave")),
                "module": str(payload.get("module")),
                "overall_status": str(payload.get("overall_status")),
                "run_id": str(payload.get("run_id")),
            }
        )

    required_entries, policy_errors = load_policy(policy_file)
    errors.extend(policy_errors)
    if required_entries:
        errors.extend(validate_required_manifests(required_entries, loaded_manifests, manifest_dir, invalid_manifest_names))

    status_map = {wave: wave_status(manifests_by_wave[wave]) for wave in WAVE_ORDER}
    ordering_violations = check_order(status_map)
    validation_errors = list(errors)
    violations = [*validation_errors, *ordering_violations]

    result = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "pass": len(violations) == 0,
        "wave_status": status_map,
        "manifests": manifest_rows,
        "validation_errors": validation_errors,
        "ordering_violations": ordering_violations,
        "violations": violations,
    }

    output_json = Path(args.output_json)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

    if args.output_md:
        output_md = Path(args.output_md)
        output_md.parent.mkdir(parents=True, exist_ok=True)
        output_md.write_text(render_markdown(result), encoding="utf-8")

    return 0 if result["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
