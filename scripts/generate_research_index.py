"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_research_index.py                         ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-04-15 18:48:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     359                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
generate_research_index.py

Parses all research documentation files in research/ and generates:

 - research/implementation_influence/by_module.md
 - research/implementation_influence/by_paper.md
 - research/implementation_influence/by_version.md

Also updates the statistics table in:
 - research/implementation_influence/README.md

Each research file is expected to contain frontmatter-style fields:
  - Tags: tag1, tag2, ...
  - ThemisDB-Versionen: v1.4.1+
  - Status: Fully Implemented / Partially Implemented / Not Started

And a section listing affected modules:
  ### Affected Modules
  - [ ] Module 1 → `src/module1/`

Usage:
  python3 scripts/generate_research_index.py [--dry-run]
"""

import re
import sys
import os
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
RESEARCH_DIR = REPO_ROOT / "research"
INFLUENCE_DIR = RESEARCH_DIR / "implementation_influence"

PAPERS_DIR = RESEARCH_DIR / "papers"
BEST_PRACTICES_DIR = RESEARCH_DIR / "best_practices"
ARCH_DECISIONS_DIR = RESEARCH_DIR / "architecture_decisions"

# Files to exclude from parsing
SKIP_NAMES = {"README.md", "TEMPLATES.md", "decision_log.md"}
SKIP_PREFIXES = {"_template"}

# Source type labels
TYPE_PAPER = "Paper"
TYPE_BEST_PRACTICE = "Best Practice"
TYPE_ARCH_DECISION = "Architecture Decision"


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

class ResearchEntry:
    def __init__(self, path: Path, entry_type: str):
        self.path = path
        self.entry_type = entry_type
        self.title: str = ""
        self.tags: list[str] = []
        self.versions: list[str] = []
        self.status: str = ""
        self.modules: list[str] = []

    @property
    def rel_path(self) -> str:
        return str(self.path.relative_to(REPO_ROOT))

    @property
    def link(self) -> str:
        rel = os.path.relpath(self.path, INFLUENCE_DIR)
        return f"[{self.title}]({rel})"

    @property
    def status_icon(self) -> str:
        s = self.status.lower()
        if "fully" in s or "adopted" in s:
            return "✅"
        if "partial" in s:
            return "🔄"
        return "⬜"


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

_TITLE_RE = re.compile(r"^#\s+(.+)$", re.MULTILINE)
_TAGS_RE = re.compile(r"[-\*]\s+\*{0,2}Tags\*{0,2}:\s*(.+)$", re.MULTILINE | re.IGNORECASE)
_VERSIONS_RE = re.compile(r"[-\*]\s+\*{0,2}ThemisDB-Versionen\*{0,2}:\s*(.+)$", re.MULTILINE | re.IGNORECASE)
_STATUS_RE = re.compile(r"[-\*]\s+\*{0,2}Status\*{0,2}:\s*(.+)$", re.MULTILINE | re.IGNORECASE)
_MODULE_RE = re.compile(r"`(src/[^\`]+)`", re.MULTILINE)


def parse_entry(path: Path, entry_type: str) -> ResearchEntry:
    entry = ResearchEntry(path, entry_type)
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return entry

    # Title: first H1 heading
    m = _TITLE_RE.search(text)
    entry.title = m.group(1).strip() if m else path.stem.replace("_", " ").title()

    # Tags
    m = _TAGS_RE.search(text)
    if m:
        raw = m.group(1).strip().strip("[]")
        entry.tags = [t.strip().strip("[]").strip() for t in re.split(r"[,\s]+", raw) if t.strip()]

    # Versions
    m = _VERSIONS_RE.search(text)
    if m:
        raw = m.group(1).strip().strip("[]")
        entry.versions = [v.strip().strip("[]").strip() for v in re.split(r"[,\s]+", raw) if v.strip()]

    # Status — take first non-checkbox word(s)
    m = _STATUS_RE.search(text)
    if m:
        raw = m.group(1).strip()
        # Remove checkbox syntax like "[ ] Not Started | [x] Fully Implemented"
        statuses = re.findall(r"\[x\]\s*([^\|]+)", raw, re.IGNORECASE)
        if statuses:
            entry.status = statuses[-1].strip()
        else:
            entry.status = re.sub(r"\[.\]\s*", "", raw).split("|")[0].strip()

    # Affected modules — scan `src/...` backtick references
    entry.modules = list(dict.fromkeys(_MODULE_RE.findall(text)))

    return entry


def load_all_entries() -> list[ResearchEntry]:
    entries: list[ResearchEntry] = []
    pairs = [
        (PAPERS_DIR, TYPE_PAPER),
        (BEST_PRACTICES_DIR, TYPE_BEST_PRACTICE),
        (ARCH_DECISIONS_DIR, TYPE_ARCH_DECISION),
    ]
    for directory, entry_type in pairs:
        if not directory.exists():
            continue
        for md in sorted(directory.glob("*.md")):
            if md.name in SKIP_NAMES or any(md.name.startswith(p) for p in SKIP_PREFIXES):
                continue
            entries.append(parse_entry(md, entry_type))
    return entries


# ---------------------------------------------------------------------------
# Generators
# ---------------------------------------------------------------------------

GENERATED_HEADER = (
    "*This file is auto-generated by `scripts/generate_research_index.py`"
    " — do not edit manually.*\n\n"
    f"*Last generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}*\n\n---\n\n"
)


def generate_by_module(entries: list[ResearchEntry]) -> str:
    module_map: dict[str, list[ResearchEntry]] = {}
    for entry in entries:
        for mod in entry.modules:
            module_map.setdefault(mod, []).append(entry)

    lines = [
        "# Implementation Influence — By Module\n\n",
        GENERATED_HEADER,
    ]

    if not module_map:
        lines.append(
            "*(No entries yet. Add research files and re-run "
            "`scripts/generate_research_index.py`.)*\n"
        )
        return "".join(lines)

    for mod in sorted(module_map):
        lines.append(f"## `{mod}`\n\n")
        lines.append("| Category | Source | Version | Status |\n")
        lines.append("|----------|--------|---------|--------|\n")
        for e in module_map[mod]:
            ver = ", ".join(e.versions) if e.versions else "—"
            lines.append(
                f"| {e.entry_type} | {e.link} | {ver} | {e.status_icon} {e.status or '—'} |\n"
            )
        lines.append("\n")

    return "".join(lines)


def generate_by_paper(entries: list[ResearchEntry]) -> str:
    lines = [
        "# Implementation Influence — By Paper / Source\n\n",
        GENERATED_HEADER,
    ]

    if not entries:
        lines.append(
            "*(No entries yet. Add research files and re-run "
            "`scripts/generate_research_index.py`.)*\n"
        )
        return "".join(lines)

    for e in sorted(entries, key=lambda x: x.title.lower()):
        tags = ", ".join(e.tags) if e.tags else "—"
        ver = ", ".join(e.versions) if e.versions else "—"
        lines.append(f"## {e.title}\n\n")
        lines.append(f"**File:** [{e.rel_path}](../../{e.rel_path})  \n")
        lines.append(f"**Type:** {e.entry_type}  \n")
        lines.append(f"**Tags:** {tags}  \n")
        lines.append(f"**ThemisDB Versions:** {ver}\n\n")
        if e.modules:
            lines.append("| Module | Status |\n|--------|--------|\n")
            for mod in e.modules:
                lines.append(f"| `{mod}` | {e.status_icon} {e.status or '—'} |\n")
        else:
            lines.append("*Modules: not yet specified in source file.*\n")
        lines.append("\n")

    return "".join(lines)


def generate_by_version(entries: list[ResearchEntry]) -> str:
    version_map: dict[str, list[ResearchEntry]] = {}
    for entry in entries:
        for ver in entry.versions if entry.versions else ["unversioned"]:
            version_map.setdefault(ver, []).append(entry)

    lines = [
        "# Implementation Influence — By Version\n\n",
        GENERATED_HEADER,
    ]

    if not version_map or list(version_map.keys()) == ["unversioned"]:
        lines.append(
            "*(No versioned entries yet. Add research files and re-run "
            "`scripts/generate_research_index.py`.)*\n"
        )
        return "".join(lines)

    for ver in sorted(version_map):
        lines.append(f"## {ver}\n\n")
        lines.append("| Source | Type | Module(s) | Status |\n")
        lines.append("|--------|------|-----------|--------|\n")
        for e in version_map[ver]:
            mods = ", ".join(f"`{m}`" for m in e.modules) if e.modules else "—"
            lines.append(f"| {e.link} | {e.entry_type} | {mods} | {e.status_icon} {e.status or '—'} |\n")
        lines.append("\n")

    return "".join(lines)


def update_readme_stats(entries: list[ResearchEntry], readme_path: Path) -> str:
    """Return updated content of implementation_influence/README.md with fresh stats."""
    try:
        content = readme_path.read_text(encoding="utf-8")
    except OSError:
        return ""

    papers = sum(1 for e in entries if e.entry_type == TYPE_PAPER)
    bps = sum(1 for e in entries if e.entry_type == TYPE_BEST_PRACTICE)
    adrs = sum(1 for e in entries if e.entry_type == TYPE_ARCH_DECISION)
    modules = len({m for e in entries for m in e.modules})

    def replace_stat(text: str, label: str, value: int) -> str:
        pattern = re.compile(
            rf"(\|\s*{re.escape(label)}\s*\|\s*)\d+(\s*\|)", re.IGNORECASE
        )
        return pattern.sub(rf"\g<1>{value}\2", text)

    content = replace_stat(content, "Scientific Papers", papers)
    content = replace_stat(content, "Best Practices", bps)
    content = replace_stat(content, "Architecture Decisions", adrs)
    content = replace_stat(content, "Modules with Research Links", modules)
    return content


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    dry_run = "--dry-run" in sys.argv

    entries = load_all_entries()

    counts = {
        TYPE_PAPER: sum(1 for e in entries if e.entry_type == TYPE_PAPER),
        TYPE_BEST_PRACTICE: sum(1 for e in entries if e.entry_type == TYPE_BEST_PRACTICE),
        TYPE_ARCH_DECISION: sum(1 for e in entries if e.entry_type == TYPE_ARCH_DECISION),
    }

    print("Research Index Generator")
    print("=" * 60)
    print(f"Found {counts[TYPE_PAPER]} paper(s), "
          f"{counts[TYPE_BEST_PRACTICE]} best practice(s), "
          f"{counts[TYPE_ARCH_DECISION]} architecture decision(s).")
    print()

    outputs = {
        INFLUENCE_DIR / "by_module.md": generate_by_module(entries),
        INFLUENCE_DIR / "by_paper.md": generate_by_paper(entries),
        INFLUENCE_DIR / "by_version.md": generate_by_version(entries),
    }

    readme_path = INFLUENCE_DIR / "README.md"
    updated_readme = update_readme_stats(entries, readme_path)

    if dry_run:
        print("[dry-run] Would write:")
        for p in outputs:
            print(f"  {p.relative_to(REPO_ROOT)}")
        print(f"  {readme_path.relative_to(REPO_ROOT)} (stats update)")
        return 0

    INFLUENCE_DIR.mkdir(parents=True, exist_ok=True)

    for path, content in outputs.items():
        path.write_text(content, encoding="utf-8")
        print(f"✅ Written: {path.relative_to(REPO_ROOT)}")

    if updated_readme:
        readme_path.write_text(updated_readme, encoding="utf-8")
        print(f"✅ Updated stats: {readme_path.relative_to(REPO_ROOT)}")

    print()
    print("Done. Commit the updated files with:")
    print("  git add research/implementation_influence/")
    print("  git commit -m 'docs(research): regenerate influence index'")
    return 0


if __name__ == "__main__":
    sys.exit(main())
