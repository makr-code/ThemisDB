#!/usr/bin/env python3
"""validate_wiki_links.py — Validate internal links in a generated wiki staging
directory before it is pushed to the GitHub Wiki remote.

Usage:
    python scripts/validate_wiki_links.py \
        --wiki-dir /tmp/wiki-staging \
        [--fail-on-broken] \
        [--report /tmp/wiki-link-report.txt]

Checks performed:
    1. [[WikiPageName]] / [[Label|WikiPageName]] links → page file exists?
    2. [[Label|WikiPageName#anchor]] — page exists? (anchors not validated)
    3. Absolute https:// links are skipped (no network check performed).
    4. Relative Markdown links [text](path) that survived transformation
       are flagged as warnings (they indicate a missed rewrite).

Exit codes:
    0  — all checks passed (or only warnings and --fail-on-broken not set)
    1  — broken wiki links found (only when --fail-on-broken is set)
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# GitHub Wiki uses the page file stem as the page name.
# [[WikiPageName]] resolves to <WikiPageName>.md in the wiki root.
WIKI_LINK_RE = re.compile(
    r"\[\[(?P<label>[^\]|]+)(?:\|(?P<target>[^\]]+))?\]\]"
)

# Relative Markdown link — these should have been rewritten by build_wiki.py.
# Flag them as warnings.
RELATIVE_MD_LINK_RE = re.compile(
    r"\[(?:[^\]]+)\]\((?!https?://)(?!#)([^)]+)\)"
)

# HTML comment provenance header — excluded from link checks
HTML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)

# Inline code spans: `...`  — strip content before wiki-link scanning
INLINE_CODE_RE = re.compile(r"`[^`\n]+`")

# Fenced code block: ```...```  (multiline, non-greedy)
FENCED_CODE_RE = re.compile(r"```.*?```", re.DOTALL)


def _normalize_page_name(raw: str) -> str:
    """Normalise a wiki page reference to the expected filename stem.

    GitHub Wiki page names are case-insensitive and treat spaces and
    hyphens interchangeably when resolving, but the file system is
    case-sensitive on Linux runners. We normalise to lowercase for
    comparison to stay safe.
    """
    return raw.strip().split("#")[0].strip().lower()


def validate(wiki_dir: Path, fail_on_broken: bool, report_path: Path | None) -> int:
    """Run all link validation checks on the wiki staging directory.

    Returns 0 on success, 1 on broken links when fail_on_broken is True.
    """
    md_files = sorted(wiki_dir.glob("*.md"))
    if not md_files:
        print(f"ERROR: No .md files found in {wiki_dir}", file=sys.stderr)
        return 1

    # Build a set of all valid page name stems (lower-cased for comparison)
    valid_pages: set[str] = {f.stem.lower() for f in md_files}

    broken: list[tuple[str, int, str]] = []    # (file, line, link_target)
    warnings: list[tuple[str, int, str]] = []  # (file, line, raw_link)
    checked = 0

    for md_file in md_files:
        try:
            text = md_file.read_text(encoding="utf-8")
        except OSError as exc:
            print(f"WARNING: cannot read {md_file}: {exc}", file=sys.stderr)
            continue

        # Strip HTML comment blocks before scanning
        text_clean = HTML_COMMENT_RE.sub("", text)
        # Strip fenced code blocks — they may contain [[...]] that are not
        # wiki links (e.g. C++ [[nodiscard]], shell [[-z ...]])
        text_clean = FENCED_CODE_RE.sub("", text_clean)
        # Strip inline code spans for the same reason
        text_clean = INLINE_CODE_RE.sub("", text_clean)

        for lineno, line in enumerate(text_clean.splitlines(), start=1):
            # Check [[WikiLinks]]
            for m in WIKI_LINK_RE.finditer(line):
                raw_target = m.group("target") or m.group("label")
                norm = _normalize_page_name(raw_target)
                checked += 1
                if norm and norm not in valid_pages:
                    broken.append((md_file.name, lineno, raw_target.strip()))

            # Warn on surviving relative .md links (missed rewrites)
            for m in RELATIVE_MD_LINK_RE.finditer(line):
                href = m.group(1)
                if href.endswith(".md") or ".md#" in href:
                    warnings.append((md_file.name, lineno, href))

    # ---- Report ---------------------------------------------------------------
    report_lines: list[str] = []

    def _out(msg: str) -> None:
        print(msg)
        report_lines.append(msg)

    _out(f"Wiki link validation — {len(md_files)} pages, {checked} [[links]] checked")
    _out(f"{'='*60}")

    if broken:
        _out(f"\n❌  BROKEN LINKS ({len(broken)}):\n")
        for fname, lineno, target in broken:
            _out(f"  {fname}:{lineno}  → [[{target}]]  (page not found)")
    else:
        _out("\n✅  No broken [[wiki links]] found.")

    if warnings:
        _out(f"\n⚠️   RELATIVE .md LINKS not rewritten ({len(warnings)}):\n")
        for fname, lineno, href in warnings:
            _out(f"  {fname}:{lineno}  → {href}  (should be [[WikiLink]] or absolute URL)")

    _out(f"\n{'='*60}")
    summary_icon = "✅" if not broken else "❌"
    _out(
        f"{summary_icon}  Summary: {len(broken)} broken, "
        f"{len(warnings)} warnings, {checked} checked across {len(md_files)} pages."
    )

    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8")
        print(f"\nReport written to: {report_path}")

    if broken and fail_on_broken:
        return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Validate internal [[wiki links]] in a generated wiki staging directory."
    )
    parser.add_argument(
        "--wiki-dir",
        default="/tmp/wiki-staging",
        metavar="DIR",
        help="Directory containing generated wiki .md pages (default: /tmp/wiki-staging).",
    )
    parser.add_argument(
        "--fail-on-broken",
        action="store_true",
        help="Exit with code 1 if any broken links are found.",
    )
    parser.add_argument(
        "--report",
        metavar="FILE",
        help="Write a plain-text report to FILE in addition to stdout.",
    )
    args = parser.parse_args(argv)

    wiki_dir = Path(args.wiki_dir).resolve()
    if not wiki_dir.is_dir():
        print(f"ERROR: wiki directory not found: {wiki_dir}", file=sys.stderr)
        return 1

    report_path = Path(args.report).resolve() if args.report else None
    return validate(wiki_dir, args.fail_on_broken, report_path)


if __name__ == "__main__":
    sys.exit(main())
