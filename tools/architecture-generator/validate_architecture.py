#!/usr/bin/env python3
"""
ThemisDB Architecture Validator
=================================
Validates the generated ARCHITECTURE.JSON and ARCHITECTURE.md files.

Checks:
  - JSON schema structure (required top-level keys, types, non-empty arrays)
  - Mermaid syntax sanity in ARCHITECTURE.md
  - Source hash presence
  - statistics block completeness

Usage:
  python tools/architecture-generator/validate_architecture.py [--repo-root PATH]

Exit codes:
  0 — all checks passed
  1 — one or more validation errors
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


REQUIRED_TOP_LEVEL_KEYS = [
    "schema_version",
    "generated_at",
    "generator",
    "source_hashes",
    "modules",
    "tier_summary",
    "relationships",
    "llm_wiki",
    "ai_working_artifacts",
    "documentation_sources",
    "statistics",
]

REQUIRED_MODULE_KEYS = [
    "name",
    "namespace",
    "tier",
    "purpose",
    "src_path",
]

REQUIRED_STATS_KEYS = [
    "total_modules",
    "total_relationships",
]


def validate_json(json_path: Path) -> list[str]:
    """Validate ARCHITECTURE.JSON structure and return a list of error messages."""
    errors: list[str] = []

    if not json_path.exists():
        return [f"Missing file: {json_path}"]

    try:
        model = json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"JSON parse error in {json_path}: {exc}"]

    # Top-level keys
    for key in REQUIRED_TOP_LEVEL_KEYS:
        if key not in model:
            errors.append(f"Missing top-level key: '{key}'")

    # modules array
    modules = model.get("modules", [])
    if not isinstance(modules, list):
        errors.append("'modules' must be a list")
    elif len(modules) == 0:
        errors.append("'modules' array is empty — generator may have failed to scan sources")
    else:
        for i, mod in enumerate(modules[:5]):  # spot-check first 5
            for k in REQUIRED_MODULE_KEYS:
                if k not in mod:
                    errors.append(f"Module[{i}] missing key '{k}': {mod.get('name', '?')}")

    # relationships array
    relationships = model.get("relationships", [])
    if not isinstance(relationships, list):
        errors.append("'relationships' must be a list")
    elif len(relationships) == 0:
        errors.append("'relationships' array is empty")

    # tier_summary
    tier_summary = model.get("tier_summary", {})
    if not isinstance(tier_summary, dict) or len(tier_summary) == 0:
        errors.append("'tier_summary' is empty or not a dict")

    # source_hashes
    source_hashes = model.get("source_hashes", {})
    if not isinstance(source_hashes, dict) or len(source_hashes) == 0:
        errors.append("'source_hashes' is empty or not a dict")

    # statistics
    stats = model.get("statistics", {})
    for k in REQUIRED_STATS_KEYS:
        if k not in stats:
            errors.append(f"'statistics' missing key '{k}'")
        elif not isinstance(stats[k], int) or stats[k] < 0:
            errors.append(f"'statistics.{k}' must be a non-negative integer")

    # generated_at ISO format
    gen_at = model.get("generated_at", "")
    if not re.match(r"\d{4}-\d{2}-\d{2}T", gen_at):
        errors.append(f"'generated_at' does not look like an ISO timestamp: {gen_at!r}")

    return errors


def validate_markdown(md_path: Path) -> list[str]:
    """Validate ARCHITECTURE.md for Mermaid block presence and basic syntax."""
    errors: list[str] = []

    if not md_path.exists():
        return [f"Missing file: {md_path}"]

    text = md_path.read_text(encoding="utf-8")

    # Must contain a Mermaid code block
    if "```mermaid" not in text:
        errors.append("ARCHITECTURE.md does not contain a ```mermaid code block")
        return errors

    # Extract Mermaid block
    mermaid_match = re.search(r"```mermaid\n(.*?)```", text, re.DOTALL)
    if not mermaid_match:
        errors.append("Could not extract Mermaid block from ARCHITECTURE.md")
        return errors

    mermaid_content = mermaid_match.group(1).strip()

    # Must start with a graph or flowchart directive
    if not re.match(r"^(flowchart|graph)\s+", mermaid_content):
        errors.append(
            f"Mermaid block does not start with 'flowchart' or 'graph': {mermaid_content[:80]!r}"
        )

    # Must contain at least a few node definitions
    node_count = len(re.findall(r'\w+\s*[\[({]', mermaid_content))
    if node_count < 5:
        errors.append(
            f"Mermaid diagram has very few nodes ({node_count}); generation may have failed"
        )

    # Must contain at least a few edges
    edge_count = len(re.findall(r"-->", mermaid_content))
    if edge_count == 0:
        errors.append("Mermaid diagram has no edges (-->); relationships may not have been rendered")

    # Check for unbalanced quotes (common Mermaid syntax error)
    for i, line in enumerate(mermaid_content.splitlines(), 1):
        if line.count('"') % 2 != 0:
            errors.append(f"Mermaid line {i} has unbalanced double-quotes: {line!r}")
            break  # report only first

    # All node IDs must be prefixed with mod_ (no bare reserved keywords)
    _MERMAID_RESERVED = {"graph", "flowchart", "subgraph", "end", "index", "config", "process"}
    node_id_pattern = re.compile(r"^\s+(\w+)\[")
    for i, line in enumerate(mermaid_content.splitlines(), 1):
        m = node_id_pattern.match(line)
        if m:
            nid = m.group(1)
            if not nid.startswith("mod_"):
                errors.append(
                    f"Mermaid line {i}: node ID '{nid}' is not prefixed with 'mod_' — "
                    "may collide with Mermaid reserved keywords"
                )
            if nid.lower() in _MERMAID_RESERVED:
                errors.append(
                    f"Mermaid line {i}: node ID '{nid}' is a reserved Mermaid keyword"
                )

    # Must contain click directives (node links to docs)
    click_count = mermaid_content.count("click mod_")
    if click_count == 0:
        errors.append("Mermaid diagram has no 'click' directives — node links to docs are missing")

    # Required sections in markdown
    for heading in ["## Statistics", "## Tier Classification", "## Consumer / Provider Dependencies"]:
        if heading not in text:
            errors.append(f"ARCHITECTURE.md missing expected section: '{heading}'")

    # Auto-generated banner
    if "Auto-generated" not in text:
        errors.append("ARCHITECTURE.md missing 'Auto-generated' banner")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate generated ARCHITECTURE.JSON and ARCHITECTURE.md"
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Path to repository root (default: inferred from script location)",
    )
    parser.add_argument(
        "--json-path",
        default=None,
        help="Explicit path to ARCHITECTURE.JSON",
    )
    parser.add_argument(
        "--md-path",
        default=None,
        help="Explicit path to ARCHITECTURE.md",
    )
    args = parser.parse_args()

    if args.repo_root:
        repo_root = Path(args.repo_root).resolve()
    else:
        repo_root = Path(__file__).resolve().parent.parent.parent

    json_path = Path(args.json_path) if args.json_path else repo_root / "ARCHITECTURE.JSON"
    md_path = Path(args.md_path) if args.md_path else repo_root / "ARCHITECTURE.md"

    all_errors: list[str] = []

    print(f"[validate] checking {json_path} …")
    json_errors = validate_json(json_path)
    all_errors.extend(json_errors)

    print(f"[validate] checking {md_path} …")
    md_errors = validate_markdown(md_path)
    all_errors.extend(md_errors)

    if all_errors:
        print(f"\n[validate] FAILED — {len(all_errors)} error(s):", file=sys.stderr)
        for err in all_errors:
            print(f"  ✗ {err}", file=sys.stderr)
        return 1

    print(f"[validate] OK — all checks passed for {json_path.name} and {md_path.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
