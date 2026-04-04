#!/usr/bin/env python3
"""
Doc Header Check (ThemisDB)

Enforces the markdown header-block standard for documentation under docs/**.
Designed for "changed-only" enforcement in CI (PRs against develop).

Checks for (within the first N lines, taken from schema):
- clickable breadcrumb link-chain in the first non-empty line
- **Datum:** YYYY-MM-DD
- **Status:** <allowed>
- **Primary (Quelle der Wahrheit):** + at least 1 bullet item
- **Bezug / Reference:** + at least 1 non-empty content line

Schema source of truth:
- docs/_standards/doc_header.schema.yml
"""

from __future__ import annotations

import argparse
import fnmatch
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, List

try:
    import yaml  # type: ignore
except Exception:
    yaml = None


RE_MD_LINK = re.compile(r"\[[^\]]+\]\([^)]+\)")
RE_BREADCRUMB_CHAIN = re.compile(r"\[[^\]]+\]\([^)]+\)\s*>\s*\[[^\]]+\]\([^)]+\)")
RE_DATE_LINE = re.compile(r"^\*\*Datum:\*\*\s*\d{4}-\d{2}-\d{2}\s*$")
RE_STATUS_LINE = re.compile(r"^\*\*Status:\*\*\s*(\S.+?)\s*$")


@dataclass
class Schema:
    header_first_n_lines: int
    doc_kind_allowed: set[str]
    label_date: str
    label_status: str
    label_primary: str
    label_reference: str
    status_allowed: set[str]


def run_git(args: List[str]) -> str:
    p = subprocess.run(["git", *args], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError(p.stderr.strip() or f"git {' '.join(args)} failed")
    return p.stdout


def load_schema(schema_path: Path) -> Schema:
    if yaml is None:
        raise RuntimeError("PyYAML missing. Install: pip install pyyaml")
    if not schema_path.exists():
        raise FileNotFoundError(f"Schema not found: {schema_path}")

    data: dict[str, Any] = yaml.safe_load(schema_path.read_text(encoding="utf-8"))

    header_n = int(data.get("enforcement", {}).get("header_must_be_within_first_n_lines", 40))
    doc_kinds = set(data.get("breadcrumb", {}).get("doc_kind_allowed", []))

    fields = data.get("header_fields", {})
    label_date = fields.get("date", {}).get("label", "Datum")
    label_status = fields.get("status", {}).get("label", "Status")
    label_primary = fields.get("primary_sources", {}).get("label", "Primary (Quelle der Wahrheit)")
    label_reference = fields.get("reference", {}).get("label", "Bezug / Reference")
    status_allowed = set(fields.get("status", {}).get("allowed", []))

    return Schema(
        header_first_n_lines=header_n,
        doc_kind_allowed=doc_kinds,
        label_date=label_date,
        label_status=label_status,
        label_primary=label_primary,
        label_reference=label_reference,
        status_allowed=status_allowed,
    )


def matches_any(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatch(path, pat) for pat in patterns)


def get_changed_docs(base_ref: str, head_ref: str) -> list[str]:
    out = run_git(["diff", "--name-only", f"{base_ref}...{head_ref}"])
    files = [l.strip() for l in out.splitlines() if l.strip()]
    return [f for f in files if f.startswith("docs/") and f.endswith(".md")]


def first_nonempty_line(lines: list[str]) -> tuple[int | None, str | None]:
    for i, l in enumerate(lines):
        if l.strip():
            return i, l.strip()
    return None, None


def find_line_index(window: list[str], prefix: str) -> int | None:
    for i, l in enumerate(window):
        if l.strip().startswith(prefix):
            return i
    return None


def validate_doc(path: Path, schema: Schema) -> list[str]:
    errors: list[str] = []

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except UnicodeDecodeError:
        return [f"{path}: not UTF-8 decodable"]

    window = lines[: schema.header_first_n_lines]
    idx, breadcrumb = first_nonempty_line(window)

    if breadcrumb is None:
        return [f"{path}: empty file (no header)"]

    # 1) Breadcrumb: clickable link-chain, >= 4 links
    if not RE_BREADCRUMB_CHAIN.search(breadcrumb):
        errors.append(f"{path}: breadcrumb must be a clickable link-chain in first non-empty line")
    link_count = len(RE_MD_LINK.findall(breadcrumb))
    if link_count < 4:
        errors.append(f"{path}: breadcrumb must contain >= 4 markdown links; found {link_count}")

    # doc_kind allowed (heuristic: last link text)
    if schema.doc_kind_allowed:
        link_texts = re.findall(r"\[([^\]]+)\]\([^)]+\)", breadcrumb)
        if link_texts:
            doc_kind = link_texts[-1].strip().lower()
            if doc_kind not in schema.doc_kind_allowed:
                errors.append(
                    f"{path}: breadcrumb doc_kind '{doc_kind}' not allowed (allowed: {sorted(schema.doc_kind_allowed)})"
                )

    # 2) Datum
    date_prefix = f"**{schema.label_date}:**"
    date_idx = find_line_index(window, date_prefix)
    if date_idx is None or not RE_DATE_LINE.match(window[date_idx].strip()):
        errors.append(f"{path}: missing or invalid '**{schema.label_date}:** YYYY-MM-DD' within first {schema.header_first_n_lines} lines")

    # 3) Status
    status_prefix = f"**{schema.label_status}:**"
    status_idx = find_line_index(window, status_prefix)
    if status_idx is None or not RE_STATUS_LINE.match(window[status_idx].strip()):
        errors.append(f"{path}: missing '**{schema.label_status}:** <value>' within first {schema.header_first_n_lines} lines")
    else:
        status_val = RE_STATUS_LINE.match(window[status_idx].strip()).group(1).strip().split()[0].lower()  # type: ignore[union-attr]
        if schema.status_allowed and status_val not in schema.status_allowed:
            errors.append(f"{path}: status '{status_val}' not allowed (allowed: {sorted(schema.status_allowed)})")

    # 4) Primary + at least one bullet item after
    primary_prefix = f"**{schema.label_primary}:**"
    primary_idx = find_line_index(window, primary_prefix)
    if primary_idx is None:
        errors.append(f"{path}: missing '**{schema.label_primary}:**' within first {schema.header_first_n_lines} lines")
    else:
        bullet_found = False
        for l in window[primary_idx + 1 :]:
            s = l.strip()
            if not s:
                continue
            if s.startswith("**"):  # next field
                break
            if s.startswith("- "):
                bullet_found = True
                break
        if not bullet_found:
            errors.append(f"{path}: '{schema.label_primary}' must have at least one bullet item")

    # 5) Bezug/Reference + at least one line content after
    ref_prefix = f"**{schema.label_reference}:**"
    ref_idx = find_line_index(window, ref_prefix)
    if ref_idx is None:
        errors.append(f"{path}: missing '**{schema.label_reference}:**' within first {schema.header_first_n_lines} lines")
    else:
        content_found = False
        for l in window[ref_idx + 1 :]:
            s = l.strip()
            if not s:
                continue
            if s.startswith("---") or s.startswith("**"):
                break
            content_found = True
            break
        if not content_found:
            errors.append(f"{path}: '{schema.label_reference}' must include at least one content line")

    return errors


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--schema", default="docs/_standards/doc_header.schema.yml")
    ap.add_argument("--mode", choices=["changed-only", "all"], default="changed-only")
    ap.add_argument("--base-ref", default="origin/develop")
    ap.add_argument("--head-ref", default="HEAD")
    args = ap.parse_args()

    schema = load_schema(Path(args.schema))

    if args.mode == "changed-only":
        targets = get_changed_docs(args.base_ref, args.head_ref)
    else:
        targets = [str(p) for p in Path("docs").rglob("*.md")]

    # apply schema include/exclude patterns (basic)
    include = ["docs/**/*.md"]
    exclude = ["docs/**/ARCHIVED/**", "docs/**/archive/**"]
    filtered = [t for t in targets if matches_any(t, include) and not matches_any(t, exclude)]

    if not filtered:
        print("doc-header-check: no docs/*.md files to validate")
        return 0

    all_errors: list[str] = []
    for t in filtered:
        all_errors.extend(validate_doc(Path(t), schema))

    if all_errors:
        print("doc-header-check: FAILED\n")
        for e in all_errors:
            print(f"- {e}")
        return 1

    print(f"doc-header-check: OK ({len(filtered)} files validated)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
