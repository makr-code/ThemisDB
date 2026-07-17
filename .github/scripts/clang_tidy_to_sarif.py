#!/usr/bin/env python3
"""Convert clang-tidy text output to SARIF 2.1.0 format.

Usage:
    python3 clang_tidy_to_sarif.py <clang-tidy-output.txt> <output.sarif> \
        [--repo-root <path>] [--tool-version <X>]

Parses diagnostic lines of the form:
    /abs/path/to/file.cpp:LINE:COL: LEVEL: MESSAGE [CHECK-NAME]

and emits a SARIF v2.1.0 document suitable for upload to GitHub Code Scanning.
Note- and remark-level diagnostics are filtered to reduce Code Scanning noise.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path

# Matches the primary diagnostic line emitted by clang-tidy:
#   /abs/path/file.cpp:123:45: warning: some message [check-name]
_DIAG_RE = re.compile(
    r"^(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):\s+"
    r"(?P<level>error|warning|note|remark):\s+"
    r"(?P<message>.+?)(?:\s+\[(?P<rule>[^\]]+)\])?\s*$"
)

# clang-tidy maps severity to SARIF level
_LEVEL_MAP: dict[str, str] = {
    "error": "error",
    "warning": "warning",
    "note": "note",
    "remark": "note",
}


def _make_uri(filepath: str, repo_root: str) -> tuple[str, str]:
    """Return (uri, uriBaseId) relative to repo_root, using '%SRCROOT%' base."""
    try:
        rel = os.path.relpath(filepath, repo_root)
    except ValueError:
        # Different drives on Windows — fall back to absolute path
        return filepath.replace("\\", "/"), ""
    # Always use forward slashes; must not start with '../' for SARIF
    rel_fwd = rel.replace("\\", "/")
    if rel_fwd.startswith("../"):
        return filepath.replace("\\", "/"), ""
    return rel_fwd, "%SRCROOT%"


def parse_clang_tidy_output(text: str, repo_root: str) -> list[dict]:
    """Parse clang-tidy text output and return a list of diagnostic dicts."""
    diagnostics: list[dict] = []
    for line in text.splitlines():
        m = _DIAG_RE.match(line)
        if not m:
            continue
        level_raw = m.group("level")
        if level_raw in ("note", "remark"):
            # Notes and remarks are usually related to a previous diagnostic; skip as
            # top-level results to avoid noise in the Code Scanning view.
            continue
        rule_id = m.group("rule") or "clang-tidy"
        uri, base_id = _make_uri(m.group("file"), repo_root)
        diagnostics.append(
            {
                "rule_id": rule_id,
                "message": m.group("message").strip(),
                "level": _LEVEL_MAP.get(level_raw, "warning"),
                "uri": uri,
                "uri_base_id": base_id,
                "line": int(m.group("line")),
                "col": int(m.group("col")),
            }
        )
    return diagnostics


def build_sarif(diagnostics: list[dict], tool_version: str = "unknown") -> dict:
    """Build a SARIF 2.1.0 document from parsed diagnostics."""
    # Collect unique rule IDs
    rule_ids: list[str] = []
    seen_rules: set[str] = set()
    for d in diagnostics:
        if d["rule_id"] not in seen_rules:
            seen_rules.add(d["rule_id"])
            rule_ids.append(d["rule_id"])

    # Build rule descriptors
    rules = []
    for rule_id in rule_ids:
        # Derive a docs URL for known clang-tidy check families
        parts = rule_id.split("-", 1)
        help_uri = (
            f"https://clang.llvm.org/extra/clang-tidy/checks/{parts[0]}/{parts[1]}.html"
            if len(parts) == 2
            else "https://clang.llvm.org/extra/clang-tidy/"
        )
        rules.append(
            {
                "id": rule_id,
                "name": rule_id,
                "shortDescription": {"text": f"clang-tidy: {rule_id}"},
                "helpUri": help_uri,
            }
        )

    # Build results
    results = []
    for d in diagnostics:
        location: dict = {
            "physicalLocation": {
                "artifactLocation": {"uri": d["uri"]},
                "region": {
                    "startLine": d["line"],
                    "startColumn": d["col"],
                },
            }
        }
        if d["uri_base_id"]:
            location["physicalLocation"]["artifactLocation"]["uriBaseId"] = d["uri_base_id"]

        results.append(
            {
                "ruleId": d["rule_id"],
                "message": {"text": d["message"]},
                "locations": [location],
                "level": d["level"],
            }
        )

    return {
        "$schema": "https://json.schemastore.org/sarif-2.1.0.json",
        "version": "2.1.0",
        "runs": [
            {
                "tool": {
                    "driver": {
                        "name": "clang-tidy",
                        "version": tool_version,
                        "informationUri": "https://clang.llvm.org/extra/clang-tidy/",
                        "rules": rules,
                    }
                },
                "results": results,
            }
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Convert clang-tidy text output to SARIF 2.1.0."
    )
    parser.add_argument("input", help="clang-tidy output file (text)")
    parser.add_argument("output", help="Destination SARIF file path")
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root directory for computing relative paths (default: .)",
    )
    parser.add_argument(
        "--tool-version",
        default="unknown",
        help="clang-tidy version string to embed in SARIF tool metadata",
    )
    args = parser.parse_args(argv)

    input_path = Path(args.input)
    if not input_path.exists():
        print(f"ERROR: input file not found: {input_path}", file=sys.stderr)
        return 1

    repo_root = str(Path(args.repo_root).resolve())
    text = input_path.read_text(encoding="utf-8", errors="replace")
    diagnostics = parse_clang_tidy_output(text, repo_root)
    sarif = build_sarif(diagnostics, tool_version=args.tool_version)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(sarif, indent=2, ensure_ascii=False), encoding="utf-8")

    print(
        f"Converted {len(diagnostics)} diagnostic(s) → {output_path} "
        f"({len(sarif['runs'][0]['tool']['driver']['rules'])} unique rule(s))"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
