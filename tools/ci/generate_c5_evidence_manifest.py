#!/usr/bin/env python3
"""
Generate BSI C5 evidence manifest artifacts for a release/run.
"""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path


def parse_pairs(values: list[str]) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for raw in values:
        if "=" not in raw:
            raise ValueError(f"Expected KEY=VALUE, got: {raw}")
        key, value = raw.split("=", 1)
        key = key.strip()
        if not key:
            raise ValueError(f"Empty key in pair: {raw}")
        parsed[key] = value.strip()
    return parsed


def write_markdown(path: Path, manifest: dict) -> None:
    controls_rows = "\n".join(
        f"| `{control}` | `{status}` |"
        for control, status in manifest["control_status"].items()
    )
    evidence_rows = "\n".join(
        f"| `{name}` | `{ref}` |"
        for name, ref in manifest["evidence_references"].items()
    )

    content = [
        f"# BSI C5 Evidence Manifest — {manifest['release_id']}",
        "",
        f"- Generated at: {manifest['generated_at']}",
        f"- Workflow: `{manifest['workflow']}`",
        f"- Run ID: `{manifest['run_id']}`",
        f"- Branch/Ref: `{manifest['ref']}`",
        f"- Scope: `{manifest['scope']}`",
        "",
        "## C5 Control Status",
        "",
        "| Control | Status |",
        "| --- | --- |",
        controls_rows or "| _none_ | n/a |",
        "",
        "## Evidence References",
        "",
        "| Type | Reference |",
        "| --- | --- |",
        evidence_rows or "| _none_ | n/a |",
        "",
        "## Notes",
        "",
        manifest["notes"] or "_none_",
        "",
    ]
    path.write_text("\n".join(content), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release-id", required=True)
    parser.add_argument("--workflow", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--ref", required=True)
    parser.add_argument("--scope", default="bsi-c5-2026-delta")
    parser.add_argument(
        "--control",
        action="append",
        default=[],
        help="Control state as KEY=VALUE (repeatable)",
    )
    parser.add_argument(
        "--evidence",
        action="append",
        default=[],
        help="Evidence reference as KEY=VALUE (repeatable)",
    )
    parser.add_argument("--notes", default="")
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-md")
    args = parser.parse_args()

    try:
        control_status = parse_pairs(args.control)
        evidence_references = parse_pairs(args.evidence)
        manifest = {
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "release_id": args.release_id,
            "workflow": args.workflow,
            "run_id": args.run_id,
            "ref": args.ref,
            "scope": args.scope,
            "control_status": control_status,
            "evidence_references": evidence_references,
            "notes": args.notes,
        }

        output_json = Path(args.output_json)
        output_json.parent.mkdir(parents=True, exist_ok=True)
        output_json.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

        if args.output_md:
            output_md = Path(args.output_md)
            output_md.parent.mkdir(parents=True, exist_ok=True)
            write_markdown(output_md, manifest)
    except (ValueError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
