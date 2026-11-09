#!/usr/bin/env python3
"""Validate the schema and run-reference consistency of a C5 evidence manifest."""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlsplit


REQUIRED_FIELDS = {
    "generated_at",
    "release_id",
    "workflow",
    "run_id",
    "ref",
    "scope",
    "control_status",
    "evidence_references",
    "notes",
}


def validate_manifest(manifest: object) -> list[str]:
    if not isinstance(manifest, dict):
        return ["manifest root must be a JSON object"]

    errors = [f"missing required field: {field}" for field in sorted(REQUIRED_FIELDS - manifest.keys())]
    for field in ("generated_at", "release_id", "workflow", "run_id", "ref", "scope", "notes"):
        value = manifest.get(field)
        if not isinstance(value, str) or not value.strip():
            errors.append(f"{field} must be a non-empty string")

    generated_at = manifest.get("generated_at")
    if isinstance(generated_at, str) and generated_at.strip():
        try:
            parsed = datetime.fromisoformat(generated_at.replace("Z", "+00:00"))
            if parsed.tzinfo is None:
                errors.append("generated_at must include a timezone")
        except ValueError:
            errors.append("generated_at must be an ISO 8601 timestamp")

    run_id = manifest.get("run_id")
    if isinstance(run_id, str) and run_id.strip():
        if run_id.startswith("manual-"):
            errors.append("run_id must not use a manual-* placeholder")
        elif not re.fullmatch(r"[0-9]+", run_id):
            errors.append("run_id must be a numeric GitHub Actions run ID")

    for field in ("control_status", "evidence_references"):
        value = manifest.get(field)
        if not isinstance(value, dict):
            errors.append(f"{field} must be a JSON object")
        elif any(
            not isinstance(key, str)
            or not key.strip()
            or not isinstance(reference, str)
            or not reference.strip()
            for key, reference in value.items()
        ):
            errors.append(f"{field} must contain non-empty string keys and values")

    evidence = manifest.get("evidence_references")
    if isinstance(evidence, dict):
        workflow_run = evidence.get("workflow_run")
        if not isinstance(workflow_run, str) or not workflow_run.strip():
            errors.append("evidence_references.workflow_run is required")
        else:
            parsed_url = urlsplit(workflow_run)
            match = re.fullmatch(r"/[^/]+/[^/]+/actions/runs/([0-9]+)", parsed_url.path)
            if parsed_url.scheme != "https" or parsed_url.netloc != "github.com" or not match:
                errors.append("evidence_references.workflow_run must be a GitHub Actions run URL")
            elif isinstance(run_id, str) and match.group(1) != run_id:
                errors.append("evidence_references.workflow_run run ID must match run_id")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--report-json", type=Path)
    args = parser.parse_args()

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        errors = [f"unable to read manifest: {exc}"]
        manifest = None
    else:
        errors = validate_manifest(manifest)

    report = {
        "validated_at": datetime.now(timezone.utc).isoformat(),
        "manifest": args.manifest.name,
        "pass": not errors,
        "errors": errors,
        "scope": "schema and cross-field consistency only; evidence authenticity, signatures, runtime results, and human approval are not verified",
    }

    if args.report_json:
        try:
            args.report_json.parent.mkdir(parents=True, exist_ok=True)
            args.report_json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        except OSError as exc:
            print(f"error: unable to write validation report: {exc}", file=sys.stderr)
            return 1

    if errors:
        print("FAIL: C5 evidence manifest validation")
        for error in errors:
            print(f"- {error}")
        return 1

    print("PASS: C5 evidence manifest schema and cross-field consistency")
    print("Note: this does not verify evidence authenticity or establish compliance.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
