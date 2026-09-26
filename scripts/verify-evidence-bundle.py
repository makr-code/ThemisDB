#!/usr/bin/env python3
"""Verify SOC2 evidence bundle integrity and required field presence.

Usage:
    python3 scripts/verify-evidence-bundle.py <bundle-file.json>

Exit codes:
    0  — bundle is valid
    1  — bundle is missing, malformed, or fails required-field checks
"""

import json
import sys
from pathlib import Path

CURRENT_REQUIRED_TOP_LEVEL_FIELDS = [
    "bundle_id",
    "collected_at_ms",
    "window_from_ms",
    "window_to_ms",
    "within_retention_window",
    "audit_log",
    "metrics",
    "key_rotations",
    "access_control",
]

LEGACY_REQUIRED_TOP_LEVEL_FIELDS = [
    "generated_at",
    "window_start",
    "window_end",
    "endpoint",
    "events",
]

LEGACY_REQUIRED_EVENT_FIELDS = [
    "timestamp",
    "event_type",
]

CURRENT_REQUIRED_AUDIT_LOG_FIELDS = [
    "from_ms",
    "to_ms",
    "entries",
]


def fail(msg: str) -> None:
    print(f"[FAIL] {msg}", file=sys.stderr)
    sys.exit(1)


def warn(msg: str) -> None:
    print(f"[WARN] {msg}", file=sys.stderr)


def validate_current_bundle(bundle: dict) -> None:
    for field in CURRENT_REQUIRED_TOP_LEVEL_FIELDS:
        if field not in bundle:
            fail(f"Bundle is missing required field: '{field}'")

    audit_log = bundle.get("audit_log")
    if not isinstance(audit_log, dict):
        fail("'audit_log' field must be a JSON object")

    for field in CURRENT_REQUIRED_AUDIT_LOG_FIELDS:
        if field not in audit_log:
            fail(f"'audit_log' is missing required field: '{field}'")

    entries = audit_log.get("entries", [])
    if not isinstance(entries, list):
        fail("'audit_log.entries' field must be a JSON array")

    print(
        f"[OK] Evidence bundle verified: id={bundle.get('bundle_id')}, "
        f"window {bundle.get('window_from_ms')} → {bundle.get('window_to_ms')}, "
        f"{len(entries)} audit entries"
    )


def validate_legacy_bundle(bundle: dict) -> None:
    for field in LEGACY_REQUIRED_TOP_LEVEL_FIELDS:
        if field not in bundle:
            fail(f"Bundle is missing required field: '{field}'")

    events = bundle.get("events", [])
    if not isinstance(events, list):
        fail("'events' field must be a JSON array")

    if len(events) == 0:
        warn("Evidence bundle contains zero events — export may have failed or window is empty")

    for idx, event in enumerate(events):
        if not isinstance(event, dict):
            fail(f"Event at index {idx} is not a JSON object")
        for field in LEGACY_REQUIRED_EVENT_FIELDS:
            if field not in event:
                fail(f"Event at index {idx} is missing required field: '{field}'")

    print(
        f"[OK] Evidence bundle verified: {len(events)} event(s), "
        f"window {bundle.get('window_start')} → {bundle.get('window_end')}"
    )


def main() -> None:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <bundle-file.json>", file=sys.stderr)
        sys.exit(1)

    bundle_path = Path(sys.argv[1])
    if not bundle_path.exists():
        fail(f"Bundle file not found: {bundle_path}")

    try:
        bundle = json.loads(bundle_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, ValueError, UnicodeDecodeError) as exc:
        fail(f"Bundle JSON is malformed: {exc}")

    if not isinstance(bundle, dict):
        fail("Bundle must be a JSON object")

    if "bundle_id" in bundle:
        validate_current_bundle(bundle)
        return

    validate_legacy_bundle(bundle)


if __name__ == "__main__":
    main()
