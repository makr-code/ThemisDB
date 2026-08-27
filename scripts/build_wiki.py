#!/usr/bin/env python3
"""build_wiki.py — Generate GitHub Wiki content from repository documentation.

Usage:
    python scripts/build_wiki.py --output /tmp/wiki-staging [--repo-root .]

Source mapping:
    README.md                         → Home.md
    docs/architecture/*.md            → Architecture-<name>.md
    docs/governance/*.md              → Governance-<name>.md
    src/*/ROADMAP.md                  → Module-<module>-Roadmap.md
    ai_context/developer_llm_wiki/*.md → Developer-<name>.md

Community guardrail:
    Files containing private plugin path patterns are blocked from the
    wiki output to prevent accidental disclosure of private implementation
    details (mirrors the Community Fail-Closed gate logic).
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Private-content guardrail patterns (mirrors gate-pr-community-failclosed)
# ---------------------------------------------------------------------------
PRIVATE_PATTERNS: list[re.Pattern[str]] = [
    re.compile(r"plugins/private/", re.IGNORECASE),
    re.compile(r"internal/private", re.IGNORECASE),
    re.compile(r"PRIVATE_PLUGIN", re.IGNORECASE),
    re.compile(r"private_api_key\s*=", re.IGNORECASE),
    re.compile(r"SECRET\s*=\s*['\"][^'\"]+['\"]", re.IGNORECASE),
]

# Doxygen tag patterns to strip from output
DOXYGEN_TAG_RE = re.compile(r"\\(?:brief|param|return|throws|tparam|requires)\b[^\n]*")

# Badge markdown: [![label](url)](target)  or  ![label](url)
BADGE_RE = re.compile(r"!\[(?:[^\]]*)\]\([^)]+\)(?:\([^)]*\))?")

# Relative markdown link: [text](path/to/file.md)  →  [[Wiki Page Name]]
RELATIVE_LINK_RE = re.compile(r"\[([^\]]+)\]\((?!https?://)([^)]+\.md[^)]*)\)")


def _slug(name: str) -> str:
    """Convert a filename stem to a GitHub Wiki-compatible page name."""
    return name.replace("_", "-").replace(" ", "-")


def _wiki_page_name(prefix: str, stem: str) -> str:
    """Compose wiki page name from prefix and file stem."""
    if prefix:
        return f"{prefix}-{_slug(stem)}"
    return _slug(stem)


def _contains_private(text: str) -> bool:
    """Return True if the content matches any private guardrail pattern."""
    return any(pat.search(text) for pat in PRIVATE_PATTERNS)


def _transform(text: str, source_path: Path, repo_root: Path) -> str:
    """Apply all transformations to markdown content for wiki publication."""
    # Strip Doxygen tags (they are noise in user-facing docs)
    text = DOXYGEN_TAG_RE.sub("", text)

    # Strip CI badges
    text = BADGE_RE.sub("", text)

    # Rewrite relative .md links to [[Wiki Page Name]] references
    def _rewrite_link(m: re.Match[str]) -> str:
        label = m.group(1)
        raw_target = m.group(2).split("#")[0].strip()
        target_path = (source_path.parent / raw_target).resolve()
        try:
            rel = target_path.relative_to(repo_root)
        except ValueError:
            # Outside repo root — keep as-is (becomes dead link, not harmful)
            return m.group(0)
        stem = target_path.stem
        wiki_ref = _slug(stem)
        return f"[[{label}|{wiki_ref}]]"

    text = RELATIVE_LINK_RE.sub(_rewrite_link, text)

    # Collapse multiple blank lines
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip() + "\n"


# ---------------------------------------------------------------------------
# Source → Wiki page mapping rules
# ---------------------------------------------------------------------------
def _collect_entries(repo_root: Path) -> list[tuple[Path, str]]:
    """Collect (source_path, wiki_page_name) pairs from the repository.

    Returns a list sorted by wiki page name.
    """
    entries: list[tuple[Path, str]] = []

    # README → Home
    readme = repo_root / "README.md"
    if readme.exists():
        entries.append((readme, "Home"))

    # docs/architecture/ → Architecture-<stem>
    arch_dir = repo_root / "docs" / "architecture"
    if arch_dir.is_dir():
        for f in sorted(arch_dir.glob("*.md")):
            entries.append((f, _wiki_page_name("Architecture", f.stem)))

    # docs/governance/ → Governance-<stem>
    gov_dir = repo_root / "docs" / "governance"
    if gov_dir.is_dir():
        for f in sorted(gov_dir.glob("*.md")):
            entries.append((f, _wiki_page_name("Governance", f.stem)))

    # src/*/ROADMAP.md → Module-<module>-Roadmap
    for roadmap in sorted((repo_root / "src").glob("*/ROADMAP.md")):
        module = roadmap.parent.name
        entries.append((roadmap, f"Module-{_slug(module)}-Roadmap"))

    # ai_context/developer_llm_wiki/ → Developer-<stem>
    dev_wiki_dir = repo_root / "ai_context" / "developer_llm_wiki"
    if dev_wiki_dir.is_dir():
        for f in sorted(dev_wiki_dir.glob("*.md")):
            entries.append((f, _wiki_page_name("Developer", f.stem)))

    return entries


def _build_sidebar(sections: dict[str, list[str]]) -> str:
    """Generate _Sidebar.md content from section → [wiki page names] map."""
    lines: list[str] = ["# Navigation\n"]
    for section_title, pages in sections.items():
        lines.append(f"\n## {section_title}\n")
        for page in pages:
            lines.append(f"- [[{page}]]\n")
    return "".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Build GitHub Wiki staging directory from repository docs."
    )
    parser.add_argument(
        "--output",
        required=True,
        metavar="DIR",
        help="Output directory for generated wiki pages.",
    )
    parser.add_argument(
        "--repo-root",
        default=".",
        metavar="DIR",
        help="Repository root (default: current directory).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be generated without writing files.",
    )
    args = parser.parse_args(argv)

    repo_root = Path(args.repo_root).resolve()
    output_dir = Path(args.output).resolve()

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    entries = _collect_entries(repo_root)

    skipped: list[str] = []
    written: list[str] = []

    # Build sidebar sections as we process entries
    sidebar_sections: dict[str, list[str]] = {
        "Home": [],
        "Architecture": [],
        "Governance": [],
        "Modules": [],
        "Developer": [],
    }

    for source_path, wiki_name in entries:
        try:
            text = source_path.read_text(encoding="utf-8")
        except OSError as exc:
            print(f"WARNING: cannot read {source_path}: {exc}", file=sys.stderr)
            continue

        if _contains_private(text):
            skipped.append(str(source_path.relative_to(repo_root)))
            print(
                f"BLOCKED (private content): {source_path.relative_to(repo_root)} → {wiki_name}.md",
                file=sys.stderr,
            )
            continue

        transformed = _transform(text, source_path, repo_root)

        # Categorize for sidebar
        if wiki_name == "Home":
            sidebar_sections["Home"].append(wiki_name)
        elif wiki_name.startswith("Architecture-"):
            sidebar_sections["Architecture"].append(wiki_name)
        elif wiki_name.startswith("Governance-"):
            sidebar_sections["Governance"].append(wiki_name)
        elif wiki_name.startswith("Module-"):
            sidebar_sections["Modules"].append(wiki_name)
        elif wiki_name.startswith("Developer-"):
            sidebar_sections["Developer"].append(wiki_name)

        dest = output_dir / f"{wiki_name}.md"
        if args.dry_run:
            print(f"DRY-RUN: {source_path.relative_to(repo_root)} → {dest.name}")
        else:
            dest.write_text(transformed, encoding="utf-8")
            written.append(dest.name)

    # Generate _Sidebar.md
    sidebar_content = _build_sidebar(sidebar_sections)
    sidebar_dest = output_dir / "_Sidebar.md"
    if args.dry_run:
        print(f"DRY-RUN: _Sidebar.md ({sum(len(v) for v in sidebar_sections.values())} entries)")
    else:
        sidebar_dest.write_text(sidebar_content, encoding="utf-8")
        written.append("_Sidebar.md")

    print(f"\nWiki build complete: {len(written)} pages written, {len(skipped)} blocked.")
    if skipped:
        print(f"Blocked files (private content guardrail): {', '.join(skipped)}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
