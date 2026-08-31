#!/usr/bin/env python3
"""build_wiki.py — Generate GitHub Wiki content from repository documentation.

Usage:
    python scripts/build_wiki.py --output /tmp/wiki-staging [--repo-root .]

Source mapping:
    README.md                                   → Home.md
    docs/architecture/*.md                      → Architecture-<name>.md
    docs/governance/*.md                        → Governance-<name>.md
    src/*/ROADMAP.md                            → Module-<module>-Roadmap.md
    src/*/ARCHITECTURE.md                       → Module-<module>-Architecture.md
    src/*/CHANGELOG.md                          → Module-<module>-Changelog.md
    src/*/FUTURE_ENHANCEMENTS.md                → Module-<module>-Future.md
    plugins/*/ROADMAP.md (public only)          → Plugin-<name>-Roadmap.md
    ai_context/developer_llm_wiki/*.md          → Developer-<name>.md
    ai_context/developer_llm_wiki/API_REF*.md   → (included via Developer mapping)
    <generated>                                 → Module-Index.md

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
# Status extraction: pull the first "## Current Status" section from a
# ROADMAP.md so the Module-Index table can show a one-line status badge.
# ---------------------------------------------------------------------------
_STATUS_SECTION_RE = re.compile(
    r"##\s+Current Status\s*\n(.*?)(?=\n##\s|\Z)", re.DOTALL
)
_CHECKBOX_RE = re.compile(r"- \[([x~I!?P])\]\s+(.+)")


def _extract_status(text: str) -> str:
    """Return a short status string from a ROADMAP.md Current Status section."""
    m = _STATUS_SECTION_RE.search(text)
    if not m:
        return "—"
    section = m.group(1).strip()
    # Grab first non-empty line as headline
    for line in section.splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            # Truncate to 80 chars to stay readable in a table cell
            return line[:80] + ("…" if len(line) > 80 else "")
    return "—"


def _count_checkboxes(text: str) -> tuple[int, int]:
    """Return (done, total) checkbox counts for a quick progress indicator."""
    total = 0
    done = 0
    for m in _CHECKBOX_RE.finditer(text):
        total += 1
        if m.group(1) == "x":
            done += 1
    return done, total


# ---------------------------------------------------------------------------
# Source → Wiki page mapping rules
# ---------------------------------------------------------------------------

# Per-module doc types collected alongside ROADMAP for the index table.
_MODULE_DOC_TYPES: list[tuple[str, str, str]] = [
    # (filename, wiki_suffix, sidebar_section)
    ("ROADMAP.md", "Roadmap", "Modules"),
    ("ARCHITECTURE.md", "Architecture", "Modules"),
    ("CHANGELOG.md", "Changelog", "Modules"),
    ("FUTURE_ENHANCEMENTS.md", "Future", "Modules"),
]


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

    # src/*/<doc>.md — ROADMAP, ARCHITECTURE, CHANGELOG, FUTURE_ENHANCEMENTS
    src_dir = repo_root / "src"
    for filename, suffix, _section in _MODULE_DOC_TYPES:
        for doc in sorted(src_dir.glob(f"*/{filename}")):
            module = doc.parent.name
            entries.append((doc, f"Module-{_slug(module)}-{suffix}"))

    # plugins/*/ROADMAP.md (public — private guard applied later per content)
    plugins_dir = repo_root / "plugins"
    if plugins_dir.is_dir():
        for roadmap in sorted(plugins_dir.glob("*/ROADMAP.md")):
            # Skip explicitly private plugin directories by path name
            if "private" in roadmap.parts:
                continue
            plugin_name = roadmap.parent.name
            entries.append((roadmap, f"Plugin-{_slug(plugin_name)}-Roadmap"))

    # ai_context/developer_llm_wiki/ → Developer-<stem>
    dev_wiki_dir = repo_root / "ai_context" / "developer_llm_wiki"
    if dev_wiki_dir.is_dir():
        for f in sorted(dev_wiki_dir.glob("*.md")):
            entries.append((f, _wiki_page_name("Developer", f.stem)))

    return entries


# ---------------------------------------------------------------------------
# Module-Index page builder
# ---------------------------------------------------------------------------

def _build_module_index(repo_root: Path, wiki_names_set: set[str]) -> str:
    """Generate a Module-Index.md page linking all module wiki pages.

    For each module that has a ROADMAP.md we include:
    - links to all available doc pages (Roadmap, Architecture, Changelog, Future)
    - a short status extracted from Current Status section
    - a progress indicator (done/total checkboxes)
    """
    src_dir = repo_root / "src"
    modules = sorted(d.name for d in src_dir.iterdir() if d.is_dir())

    lines: list[str] = [
        "# Module Index\n\n",
        "Auto-generated overview of all ThemisDB source modules.\n\n",
        "| Module | Roadmap | Architecture | Changelog | Future | Status | Progress |\n",
        "|--------|---------|--------------|-----------|--------|--------|----------|\n",
    ]

    for module in modules:
        mod_dir = src_dir / module
        slug = _slug(module)

        # Build link cells for each doc type
        cells: dict[str, str] = {}
        for filename, suffix, _section in _MODULE_DOC_TYPES:
            wiki_name = f"Module-{slug}-{suffix}"
            if wiki_name in wiki_names_set:
                cells[suffix] = f"[[{suffix}|{wiki_name}]]"
            else:
                cells[suffix] = "—"

        # Status and progress from ROADMAP if available
        roadmap_path = mod_dir / "ROADMAP.md"
        status = "—"
        progress = "—"
        if roadmap_path.exists():
            try:
                text = roadmap_path.read_text(encoding="utf-8")
                status = _extract_status(text)
                done, total = _count_checkboxes(text)
                progress = f"{done}/{total}" if total > 0 else "—"
            except OSError:
                pass

        lines.append(
            f"| **{module}** | {cells['Roadmap']} | {cells['Architecture']} |"
            f" {cells['Changelog']} | {cells['Future']} | {status} | {progress} |\n"
        )

    return "".join(lines)


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
        "Plugins": [],
        "Developer": [],
    }

    # Track which wiki page names were actually written (for Module-Index links)
    written_names: set[str] = set()

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
        elif wiki_name.startswith("Plugin-"):
            sidebar_sections["Plugins"].append(wiki_name)
        elif wiki_name.startswith("Developer-"):
            sidebar_sections["Developer"].append(wiki_name)

        dest = output_dir / f"{wiki_name}.md"
        if args.dry_run:
            print(f"DRY-RUN: {source_path.relative_to(repo_root)} → {dest.name}")
        else:
            dest.write_text(transformed, encoding="utf-8")
            written.append(dest.name)
        written_names.add(wiki_name)

    # Generate Module-Index.md
    module_index_content = _build_module_index(repo_root, written_names)
    module_index_dest = output_dir / "Module-Index.md"
    if args.dry_run:
        print(f"DRY-RUN: Module-Index.md ({len(sidebar_sections['Modules'])} module entries)")
    else:
        module_index_dest.write_text(module_index_content, encoding="utf-8")
        written.append("Module-Index.md")
    # Insert Module-Index at front of Modules nav section
    sidebar_sections["Modules"].insert(0, "Module-Index")

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
