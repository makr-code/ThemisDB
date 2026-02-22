#!/usr/bin/env python3
"""
classify_ci_scope.py – CI Scope Classifier for ThemisDB

Reads a list of changed files and classifies them against the scopes
defined in .github/ci-scope-config.yaml.  Outputs GitHub Actions step
outputs (key=value pairs) that downstream workflows use to decide which
jobs to run.

Usage:
    python3 classify_ci_scope.py \
        --config   .github/ci-scope-config.yaml \
        --changed-files /tmp/changed_files.txt \
        --output-file   /tmp/ci_scope_outputs.env

The output file contains lines in the form:
    has_code_changes=true
    has_security_changes=false
    …

Exit codes:
    0  Classification succeeded (even if all scopes are false).
    1  Fatal error (e.g. config file not found, YAML parse failure).
"""

from __future__ import annotations

import argparse
import fnmatch
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is not installed. Run: pip install pyyaml", file=sys.stderr)
    sys.exit(1)


# ---------------------------------------------------------------------------
# Pattern matching
# ---------------------------------------------------------------------------

def _match_pattern(file_path: str, pattern: str) -> bool:
    """Return True if *file_path* matches *pattern*.

    Supports ``**`` as a multi-segment wildcard in addition to the standard
    ``fnmatch`` single-segment wildcards.  Examples:

        src/**            matches src/foo.cpp, src/a/b/c.h
        tests/test_jwt_*.cpp  matches tests/test_jwt_validator.cpp
        *.md              matches README.md  (top-level only)
    """
    # Normalise separators
    file_path = file_path.replace("\\", "/")
    pattern = pattern.replace("\\", "/")

    if "**" in pattern:
        # Split at ** and check prefix / suffix independently
        parts = pattern.split("**/")
        if len(parts) == 2:
            prefix, suffix = parts
            # prefix must match the start of the path
            if prefix and not file_path.startswith(prefix):
                return False
            # suffix (which may itself contain * or ?) must match the tail
            remainder = file_path[len(prefix):]
            return fnmatch.fnmatch(remainder, suffix) if suffix else True
        # Fallback: treat ** as * for simple cases
        flat = pattern.replace("**", "*")
        return fnmatch.fnmatch(file_path, flat)

    return fnmatch.fnmatch(file_path, pattern)


def matches_any(file_path: str, patterns: list[str]) -> bool:
    """Return True if *file_path* matches at least one of *patterns*."""
    return any(_match_pattern(file_path, p) for p in patterns)


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------

# Scopes that indicate "real" code or config was changed (used to determine
# has_doc_only_changes).
_NON_DOC_SCOPES = {
    "has_code_changes",
    "has_security_changes",
    "has_gpu_changes",
    "has_llm_changes",
    "has_config_changes",
    "has_grafana_changes",
}


def classify(changed_files: list[str], scopes_config: dict) -> dict[str, bool]:
    """Classify *changed_files* against *scopes_config*.

    Returns a dict mapping each scope name to a bool.
    ``has_doc_only_changes`` is set to True only when no non-doc scope fired
    and at least one doc pattern matched.
    """
    results: dict[str, bool] = {}

    for scope_name, scope_def in scopes_config.items():
        patterns: list[str] = scope_def.get("patterns", [])
        hit = any(matches_any(f, patterns) for f in changed_files)
        results[scope_name] = hit

    # Override has_doc_only_changes: only true if NO non-doc scope fired.
    non_doc_fired = any(results.get(s, False) for s in _NON_DOC_SCOPES)
    if non_doc_fired:
        results["has_doc_only_changes"] = False
    # If nothing fired at all (empty diff or unknown files) keep as-is.

    return results


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------

def read_changed_files(path: str) -> list[str]:
    """Read changed file paths from *path*, one per line."""
    p = Path(path)
    if not p.exists():
        print(f"WARNING: changed-files file not found: {path}", file=sys.stderr)
        return []
    lines = p.read_text(encoding="utf-8").splitlines()
    return [ln.strip() for ln in lines if ln.strip()]


def write_outputs(results: dict[str, bool], output_file: str) -> None:
    """Write ``key=true|false`` lines to *output_file*."""
    lines = [f"{k}={'true' if v else 'false'}" for k, v in sorted(results.items())]
    Path(output_file).write_text("\n".join(lines) + "\n", encoding="utf-8")


def print_summary(changed_files: list[str], results: dict[str, bool]) -> None:
    """Print a human-readable summary to stdout."""
    print(f"Changed files ({len(changed_files)}):")
    for f in changed_files:
        print(f"  {f}")
    print()
    print("CI Scope Classification:")
    for scope, active in sorted(results.items()):
        icon = "✅" if active else "⬜"
        print(f"  {icon}  {scope}={str(active).lower()}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Classify CI scope from a list of changed files."
    )
    parser.add_argument(
        "--config",
        default=".github/ci-scope-config.yaml",
        help="Path to ci-scope-config.yaml (default: .github/ci-scope-config.yaml)",
    )
    parser.add_argument(
        "--changed-files",
        required=True,
        help="Path to a text file containing changed file paths, one per line.",
    )
    parser.add_argument(
        "--output-file",
        required=True,
        help="Path where key=value outputs are written (for GitHub Actions).",
    )
    args = parser.parse_args(argv)

    # Load config
    config_path = Path(args.config)
    if not config_path.exists():
        print(f"ERROR: Config file not found: {args.config}", file=sys.stderr)
        return 1
    try:
        with config_path.open(encoding="utf-8") as fh:
            full_config = yaml.safe_load(fh)
    except yaml.YAMLError as exc:
        print(f"ERROR: Failed to parse config YAML: {exc}", file=sys.stderr)
        return 1

    scopes_config: dict = full_config.get("scopes", {})
    if not scopes_config:
        print("ERROR: No 'scopes' key found in config.", file=sys.stderr)
        return 1

    # Read changed files
    changed_files = read_changed_files(args.changed_files)

    # Classify
    results = classify(changed_files, scopes_config)

    # Output
    print_summary(changed_files, results)
    write_outputs(results, args.output_file)
    print(f"\nOutputs written to: {args.output_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
