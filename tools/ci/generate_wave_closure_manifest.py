#!/usr/bin/env python3
"""
Generate a source-validated Wave closure package manifest.
"""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path


def parse_pairs(values: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    for raw in values:
        if "=" not in raw:
            raise ValueError(f"Expected KEY=VALUE, got: {raw}")
        key, value = raw.split("=", 1)
        key = key.strip()
        if not key:
            raise ValueError(f"Empty key in pair: {raw}")
        result[key] = value.strip()
    return result


def parse_bool(value: str) -> bool:
    normalized = value.strip().lower()
    if normalized in {"1", "true", "yes", "y", "on"}:
        return True
    if normalized in {"0", "false", "no", "n", "off"}:
        return False
    raise ValueError(f"Invalid boolean value: {value}")


def write_markdown(
    output_path: Path,
    manifest: dict,
) -> None:
    gate_rows = "\n".join(
        f"| `{name}` | {status} |" for name, status in manifest["gates"].items()
    )
    evidence_rows = "\n".join(
        f"| `{name}` | `{value}` |"
        for name, value in manifest["evidence_references"].items()
    )
    source_rows = "\n".join(
        f"| `{name}` | {'yes' if enabled else 'no'} |"
        for name, enabled in manifest["source_validation"].items()
    )
    lines = [
        f"# {manifest['wave']} Closure Package — {manifest['module']}",
        "",
        f"- Timestamp: {manifest['generated_at']}",
        f"- Workflow: `{manifest['workflow']}`",
        f"- Run ID: `{manifest['run_id']}`",
        f"- Overall status: `{manifest['overall_status']}`",
        "",
        "## Gates",
        "",
        "| Gate | Status |",
        "| --- | --- |",
        gate_rows or "| _none_ | n/a |",
        "",
        "## Evidence references",
        "",
        "| Type | Reference |",
        "| --- | --- |",
        evidence_rows or "| _none_ | n/a |",
        "",
        "## Source validation",
        "",
        "| Signal | Present |",
        "| --- | --- |",
        source_rows or "| _none_ | no |",
        "",
        "## Notes",
        "",
        manifest["notes"] or "_none_",
        "",
    ]
    output_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wave", required=True)
    parser.add_argument("--module", required=True)
    parser.add_argument("--workflow", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--overall-status", required=True)
    parser.add_argument(
        "--gate",
        action="append",
        default=[],
        help="Gate status as KEY=VALUE (repeatable)",
    )
    parser.add_argument(
        "--evidence",
        action="append",
        default=[],
        help="Evidence reference as KEY=VALUE (repeatable)",
    )
    parser.add_argument(
        "--source-validation",
        action="append",
        default=[],
        help="Source validation flag as KEY=BOOL (repeatable)",
    )
    parser.add_argument("--notes", default="")
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-md")

    args = parser.parse_args()

    try:
        gates = parse_pairs(args.gate)
        evidence = parse_pairs(args.evidence)
        source_validation_raw = parse_pairs(args.source_validation)
        source_validation = {
            key: parse_bool(value) for key, value in source_validation_raw.items()
        }

        manifest = {
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "wave": args.wave,
            "module": args.module,
            "workflow": args.workflow,
            "run_id": args.run_id,
            "overall_status": args.overall_status,
            "gates": gates,
            "evidence_references": evidence,
            "source_validation": source_validation,
            "notes": args.notes,
        }

        json_path = Path(args.output_json)
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

        if args.output_md:
            md_path = Path(args.output_md)
            md_path.parent.mkdir(parents=True, exist_ok=True)
            write_markdown(md_path, manifest)
    except (ValueError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
