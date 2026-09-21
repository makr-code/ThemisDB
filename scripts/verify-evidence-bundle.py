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

REQUIRED_TOP_LEVEL_FIELDS = [
    "generated_at",
    "window_start",
    "window_end",
    "endpoint",
    "events",
]

REQUIRED_EVENT_FIELDS = [
    "timestamp",
    "event_type",
]


def fail(msg: str) -> None:
    print(f"[FAIL] {msg}", file=sys.stderr)
    sys.exit(1)


def warn(msg: str) -> None:
    print(f"[WARN] {msg}", file=sys.stderr)


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

    # Check required top-level fields
    for field in REQUIRED_TOP_LEVEL_FIELDS:
        if field not in bundle:
            fail(f"Bundle is missing required field: '{field}'")

    events = bundle.get("events", [])
    if not isinstance(events, list):
        fail("'events' field must be a JSON array")

    if len(events) == 0:
        warn("Evidence bundle contains zero events — export may have failed or window is empty")

    # Validate individual event records
    for idx, event in enumerate(events):
        if not isinstance(event, dict):
            fail(f"Event at index {idx} is not a JSON object")
        for field in REQUIRED_EVENT_FIELDS:
            if field not in event:
                fail(f"Event at index {idx} is missing required field: '{field}'")

    print(
        f"[OK] Evidence bundle verified: {len(events)} event(s), "
        f"window {bundle.get('window_start')} → {bundle.get('window_end')}"
    )


if __name__ == "__main__":
    main()
