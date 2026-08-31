#!/usr/bin/env python3
"""build_wiki.py — Generate GitHub Wiki content from repository documentation.

Usage:
    python scripts/build_wiki.py --output /tmp/wiki-staging [--repo-root .]

Source → Wiki page mapping (audience-first):
    docs/en/Home.md                           → Home.md          (primary wiki home)
    README.md                                 → Repository-README.md
    docs/FAQ.md                               → FAQ.md
    docs/QUICK_REFERENCE.md                   → Quick-Reference.md
    docs/EDITION_COMPARISON.md                → Edition-Comparison.md
    docs/INTEGRATION_GUIDE.md                 → Integration-Guide.md
    docs/MIGRATION_GUIDE.md                   → Migration-Guide.md
    docs/OPERATIONS.md                        → Operations.md
    docs/CONTRIBUTING_PLATFORM_GUIDELINES.md  → Contributing.md
    docs/tutorials/*.md                       → Tutorial-<stem>.md
    docs/guides/*.md                          → Guide-<stem>.md
    docs/aql/AQL_API_REFERENCE.md             → AQL-Reference.md
    docs/aql/AQL_QUERY_EXAMPLES.md            → AQL-Examples.md
    docs/api/API_REFERENCE.md                 → API-Reference.md
    docs/security/INFORMATION_SECURITY_POLICY.md → Security-Policy.md
    docs/adr/*.md                             → ADR-<stem>.md
    docs/architecture/*.md                    → Architecture-<stem>.md
    docs/governance/*.md                      → Governance-<stem>.md
    src/*/ROADMAP.md                          → Module-<module>-Roadmap.md
    src/*/ARCHITECTURE.md                     → Module-<module>-Architecture.md
    src/*/CHANGELOG.md                        → Module-<module>-Changelog.md
    src/*/FUTURE_ENHANCEMENTS.md              → Module-<module>-Future.md
    plugins/*/ROADMAP.md (public only)        → Plugin-<name>-Roadmap.md
    ai_context/developer_llm_wiki/*.md        → Developer-<stem>.md
    <generated>                               → Module-Index.md

Community guardrail:
    Files containing private plugin path patterns are blocked from the
    wiki output to prevent accidental disclosure of private implementation
    details (mirrors the Community Fail-Closed gate logic).

Header / Footer:
    Every generated page receives a lightweight HTML comment header
    (machine-readable provenance) and a Markdown footer with navigation
    back-links and the ThemisDB version string.
"""
from __future__ import annotations

import argparse
import re
import sys
from datetime import UTC, datetime
from pathlib import Path

# ---------------------------------------------------------------------------
# Version banner — updated by release automation; keep on one line.
# ---------------------------------------------------------------------------
THEMISDB_VERSION = "1.9.0-beta"

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

# HTML comments (remove from source docs before wiki publication)
HTML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)


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


# ---------------------------------------------------------------------------
# Status extraction helpers (for Module-Index)
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
    for line in section.splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            return line[:80] + ("…" if len(line) > 80 else "")
    return "—"


def _count_checkboxes(text: str) -> tuple[int, int]:
    """Return (done, total) checkbox counts."""
    total = 0
    done = 0
    for m in _CHECKBOX_RE.finditer(text):
        total += 1
        if m.group(1) == "x":
            done += 1
    return done, total


# ---------------------------------------------------------------------------
# Header / Footer generation
# ---------------------------------------------------------------------------

def _page_header(source_rel: str, wiki_name: str) -> str:
    """Return a machine-readable HTML comment provenance header."""
    ts = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
    return (
        f"<!-- wiki-page: {wiki_name} | source: {source_rel} "
        f"| generated: {ts} | themisdb: {THEMISDB_VERSION} -->\n"
    )


def _page_footer(wiki_name: str) -> str:
    """Return a Markdown footer with back-to-top and home navigation."""
    return (
        "\n\n---\n\n"
        f"_ThemisDB {THEMISDB_VERSION}_ · "
        "[[Home]] · "
        "[[Module-Index]] · "
        "[GitHub](https://github.com/makr-code/ThemisDB) · "
        "[Issues](https://github.com/makr-code/ThemisDB/issues)\n"
    )


# ---------------------------------------------------------------------------
# Content transformation
# ---------------------------------------------------------------------------

def _transform(
    text: str,
    source_path: Path,
    repo_root: Path,
    wiki_name: str,
    all_wiki_names: set[str],
) -> str:
    """Apply all transformations to markdown content for wiki publication."""
    # Remove existing HTML comments (provenance will be re-added as header)
    text = HTML_COMMENT_RE.sub("", text)

    # Strip Doxygen tags (noise in user-facing docs)
    text = DOXYGEN_TAG_RE.sub("", text)

    # Strip CI badges
    text = BADGE_RE.sub("", text)

    # Rewrite relative .md links to [[Wiki Page Name]] if target is in wiki,
    # otherwise convert to absolute GitHub URL to avoid dead wiki links.
    def _rewrite_link(m: re.Match[str]) -> str:
        label = m.group(1)
        raw_target = m.group(2).split("#")[0].strip()
        target_path = (source_path.parent / raw_target).resolve()
        try:
            rel = target_path.relative_to(repo_root)
        except ValueError:
            return m.group(0)

        # Try to find which wiki page this target maps to
        target_wiki = _path_to_wiki_name(target_path, repo_root)
        if target_wiki and target_wiki in all_wiki_names:
            return f"[[{label}|{target_wiki}]]"

        # Not in wiki — rewrite as absolute GitHub blob link
        github_url = (
            f"https://github.com/makr-code/ThemisDB/blob/develop/{rel}"
        )
        return f"[{label}]({github_url})"

    text = RELATIVE_LINK_RE.sub(_rewrite_link, text)

    # Collapse multiple blank lines
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip() + "\n"


# ---------------------------------------------------------------------------
# Reverse-lookup: file path → wiki page name
# (needed for intelligent link rewriting)
# ---------------------------------------------------------------------------

# Per-module doc types collected alongside ROADMAP for the index table.
_MODULE_DOC_TYPES: list[tuple[str, str]] = [
    ("ROADMAP.md", "Roadmap"),
    ("ARCHITECTURE.md", "Architecture"),
    ("CHANGELOG.md", "Changelog"),
    ("FUTURE_ENHANCEMENTS.md", "Future"),
]

# Single-file explicit mappings (path relative to repo root → wiki name)
_EXPLICIT_MAPPINGS: dict[str, str] = {
    "docs/en/Home.md": "Home",
    "README.md": "Repository-README",
    "docs/FAQ.md": "FAQ",
    "docs/QUICK_REFERENCE.md": "Quick-Reference",
    "docs/EDITION_COMPARISON.md": "Edition-Comparison",
    "docs/INTEGRATION_GUIDE.md": "Integration-Guide",
    "docs/MIGRATION_GUIDE.md": "Migration-Guide",
    "docs/OPERATIONS.md": "Operations",
    "docs/CONTRIBUTING_PLATFORM_GUIDELINES.md": "Contributing",
    "docs/aql/AQL_API_REFERENCE.md": "AQL-Reference",
    "docs/aql/AQL_QUERY_EXAMPLES.md": "AQL-Examples",
    "docs/api/API_REFERENCE.md": "API-Reference",
    "docs/security/INFORMATION_SECURITY_POLICY.md": "Security-Policy",
}

# Inverted: wiki name → repo-relative path
_WIKI_TO_PATH: dict[str, str] = {v: k for k, v in _EXPLICIT_MAPPINGS.items()}


def _path_to_wiki_name(path: Path, repo_root: Path) -> str | None:
    """Return the wiki page name for a given absolute file path, or None."""
    try:
        rel = str(path.relative_to(repo_root))
    except ValueError:
        return None

    # Explicit mappings
    if rel in _EXPLICIT_MAPPINGS:
        return _EXPLICIT_MAPPINGS[rel]

    # tutorials
    if rel.startswith("docs/tutorials/") and rel.endswith(".md"):
        stem = path.stem
        if stem.upper() != "README":
            return f"Tutorial-{_slug(stem)}"

    # guides
    if rel.startswith("docs/guides/") and rel.endswith(".md"):
        stem = path.stem
        if stem.upper() != "README":
            return f"Guide-{_slug(stem)}"

    # ADRs
    if rel.startswith("docs/adr/") and rel.endswith(".md"):
        return f"ADR-{_slug(path.stem)}"

    # docs/architecture/
    if rel.startswith("docs/architecture/") and rel.endswith(".md"):
        return _wiki_page_name("Architecture", path.stem)

    # docs/governance/
    if rel.startswith("docs/governance/") and rel.endswith(".md"):
        return _wiki_page_name("Governance", path.stem)

    # src module docs
    parts = path.parts
    if "src" in parts:
        src_idx = list(parts).index("src")
        if len(parts) > src_idx + 2:
            module = parts[src_idx + 1]
            filename = parts[-1]
            for fname, suffix in _MODULE_DOC_TYPES:
                if filename == fname:
                    return f"Module-{_slug(module)}-{suffix}"

    # plugins
    if rel.startswith("plugins/") and not ("private" in rel) and rel.endswith("ROADMAP.md"):
        plugin = path.parent.name
        return f"Plugin-{_slug(plugin)}-Roadmap"

    # ai_context/developer_llm_wiki/
    if rel.startswith("ai_context/developer_llm_wiki/") and rel.endswith(".md"):
        return _wiki_page_name("Developer", path.stem)

    return None


# ---------------------------------------------------------------------------
# Source → Wiki page entry collection
# ---------------------------------------------------------------------------

def _collect_entries(repo_root: Path) -> list[tuple[Path, str]]:
    """Collect (source_path, wiki_page_name) pairs from the repository.

    Order determines sidebar grouping priority.
    """
    entries: list[tuple[Path, str]] = []
    seen: set[str] = set()

    def _add(path: Path, name: str) -> None:
        if name not in seen and path.exists():
            entries.append((path, name))
            seen.add(name)

    # ---- Getting Started / User-facing ----------------------------------------

    # Primary wiki home: docs/en/Home.md (wiki-optimised, has nav structure)
    _add(repo_root / "docs" / "en" / "Home.md", "Home")

    # Fallback: root README if no en/Home.md
    _add(repo_root / "README.md", "Repository-README")

    # Core user-facing root docs
    for rel, wiki_name in _EXPLICIT_MAPPINGS.items():
        if wiki_name in ("Home", "Repository-README"):
            continue
        _add(repo_root / rel, wiki_name)

    # Tutorials
    tut_dir = repo_root / "docs" / "tutorials"
    if tut_dir.is_dir():
        for f in sorted(tut_dir.glob("*.md")):
            if f.stem.upper() == "README":
                continue
            name = f"Tutorial-{_slug(f.stem)}"
            _add(f, name)

    # Guides
    guides_dir = repo_root / "docs" / "guides"
    if guides_dir.is_dir():
        for f in sorted(guides_dir.glob("*.md")):
            if f.stem.upper() == "README":
                continue
            name = f"Guide-{_slug(f.stem)}"
            _add(f, name)

    # ADRs
    adr_dir = repo_root / "docs" / "adr"
    if adr_dir.is_dir():
        for f in sorted(adr_dir.glob("*.md")):
            _add(f, f"ADR-{_slug(f.stem)}")

    # ---- Architecture & Governance (developer-oriented) -----------------------

    arch_dir = repo_root / "docs" / "architecture"
    if arch_dir.is_dir():
        for f in sorted(arch_dir.glob("*.md")):
            _add(f, _wiki_page_name("Architecture", f.stem))

    gov_dir = repo_root / "docs" / "governance"
    if gov_dir.is_dir():
        for f in sorted(gov_dir.glob("*.md")):
            _add(f, _wiki_page_name("Governance", f.stem))

    # ---- Source modules -------------------------------------------------------

    src_dir = repo_root / "src"
    for filename, suffix in _MODULE_DOC_TYPES:
        for doc in sorted(src_dir.glob(f"*/{filename}")):
            module = doc.parent.name
            _add(doc, f"Module-{_slug(module)}-{suffix}")

    # ---- Plugins (public only) ------------------------------------------------

    plugins_dir = repo_root / "plugins"
    if plugins_dir.is_dir():
        for roadmap in sorted(plugins_dir.glob("*/ROADMAP.md")):
            if "private" in roadmap.parts:
                continue
            _add(roadmap, f"Plugin-{_slug(roadmap.parent.name)}-Roadmap")

    # ---- Developer LLM Wiki artifacts ----------------------------------------

    dev_wiki_dir = repo_root / "ai_context" / "developer_llm_wiki"
    if dev_wiki_dir.is_dir():
        for f in sorted(dev_wiki_dir.glob("*.md")):
            _add(f, _wiki_page_name("Developer", f.stem))

    return entries


# ---------------------------------------------------------------------------
# Module-Index page builder
# ---------------------------------------------------------------------------

def _build_module_index(repo_root: Path, wiki_names_set: set[str]) -> str:
    """Generate a Module-Index.md page linking all module wiki pages."""
    src_dir = repo_root / "src"
    modules = sorted(d.name for d in src_dir.iterdir() if d.is_dir())

    lines: list[str] = [
        "# Module Reference Index\n\n",
        "_Auto-generated overview of all ThemisDB source modules. "
        "Click a link to jump to the detailed wiki page for that module._\n\n",
        "| Module | Roadmap | Architecture | Changelog | Future | Status | Progress |\n",
        "|--------|:-------:|:------------:|:---------:|:------:|--------|:--------:|\n",
    ]

    for module in modules:
        mod_dir = src_dir / module
        slug = _slug(module)

        cells: dict[str, str] = {}
        for _filename, suffix in _MODULE_DOC_TYPES:
            wiki_name = f"Module-{slug}-{suffix}"
            cells[suffix] = f"[[{suffix}|{wiki_name}]]" if wiki_name in wiki_names_set else "—"

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


# ---------------------------------------------------------------------------
# Sidebar builder (audience-first)
# ---------------------------------------------------------------------------

# Sidebar section definitions: (title_with_emoji, page_name_prefix_or_list)
# The second element is either a list of explicit wiki names or a prefix string
# used to filter dynamically collected names.

_SIDEBAR_SECTIONS: list[tuple[str, list[str]]] = [
    # 1. Overview — single entry point; everything else links from here
    ("🏠 Overview", [
        "Home",
        "Wiki-Index",
        "FAQ",
        "Edition-Comparison",
        "Repository-README",
    ]),
    # 2. Getting Started — first steps for new users
    ("🚀 Getting Started", [
        "Quick-Reference",
        "Integration-Guide",
        "Migration-Guide",
    ]),
    # 3. Tutorials — hands-on walkthroughs (broad → narrow)
    ("📖 Tutorials", []),       # populated dynamically from Tutorial-* pages
    # 4. User Guide — reference material users need day-to-day
    ("📗 User Guide", [
        "AQL-Reference",
        "AQL-Examples",
        "API-Reference",
    ]),
    # 5. Operations & Security — running ThemisDB in production
    ("⚙️ Operations & Security", [
        "Operations",
        "Security-Policy",
    ]),
    # 6. Architecture — system design (after users understand what it does)
    ("🏗️ Architecture", []),    # populated dynamically from Architecture-* pages (curated subset)
    # 7. ADRs — architecture decision records (linked from Architecture)
    ("📐 ADRs", []),             # populated dynamically from ADR-* pages
    # 8. Contributing / Developer — internal orientation
    ("🔧 Contributing", [
        "Contributing",
        "Developer-INDEX",
        "Developer-BUILD-TEST-CI-AND-OPERATIONS",
        "Module-Index",
    ]),
    # 9. Governance — policy, compliance, release gates
    ("📋 Governance", []),       # populated dynamically from Governance-* pages
    # 10. Plugins — extension points
    ("🧩 Plugins", []),          # populated dynamically from Plugin-* pages
    # 11. Developer LLM Wiki — AI-context artifacts (most internal)
    ("🤖 Developer LLM Wiki", []),    # populated dynamically from Developer-* pages
]

# Architecture pages to include in the curated sidebar subset (conceptual ones)
_ARCH_CURATED = {
    "Architecture-SOURCE-DIRECTORY-GUIDE",
    "Architecture-MODULARIZATION-GUIDE",
    "Architecture-MODULAR-ARCHITECTURE-ROADMAP",
    "Architecture-MODULE-ARCHITECTURE-INDEX",
    "Architecture-POSTGRESQL-WIRE-PROTOCOL",
    "Architecture-RAFT-CONSENSUS-DESIGN",
    "Architecture-UNIFIED-ACCESS-MODEL",
    "Architecture-CONTENT-MODEL",
    "Architecture-CRYPTO-AND-KEYS",
    "Architecture-FEATURE-FLAGS-REFERENCE",
    "Architecture-QUERY-SCHEDULING",
    "Architecture-RESOURCE-POOLING",
}


def _sidebar_label(wiki_name: str) -> str:
    """Convert a wiki page name into a human-readable sidebar display label."""
    # Known acronyms / abbreviations to keep UPPER
    _ACRONYMS = frozenset({
        "AQL", "API", "CRUD", "FAQ", "GRPC", "RPC", "HTTP", "HTTPS",
        "TLS", "RBAC", "MVCC", "ACID", "CDC", "SSM", "ADR", "GPU",
        "CI", "CD", "LLM", "RAG", "ORM", "HA", "HSM", "SBOM",
        "WAL", "MTLS", "GA", "PR", "ONNX",
    })

    # Explicit overrides for pages whose auto-label is ambiguous or ugly
    _OVERRIDES: dict[str, str] = {
        "Home": "Home",
        "Wiki-Index": "All Wiki Pages",
        "Repository-README": "Repository README",
        "FAQ": "FAQ",
        "Quick-Reference": "Quick Reference",
        "Edition-Comparison": "Edition Comparison",
        "AQL-Reference": "AQL Reference",
        "AQL-Examples": "AQL Examples",
        "API-Reference": "API Reference",
        "Integration-Guide": "Integration Guide",
        "Migration-Guide": "Migration Guide",
        "Operations": "Operations",
        "Security-Policy": "Security Policy",
        "Contributing": "Contributing",
        "Module-Index": "Module Index",
        "Developer-INDEX": "Developer Wiki Index",
        "Developer-BUILD-TEST-CI-AND-OPERATIONS": "Build / Test / CI",
        # Architecture curated pages — proper names
        "Architecture-POSTGRESQL-WIRE-PROTOCOL": "PostgreSQL Wire Protocol",
        "Architecture-RAFT-CONSENSUS-DESIGN": "Raft Consensus Design",
        "Architecture-UNIFIED-ACCESS-MODEL": "Unified Access Model",
        "Architecture-MODULARIZATION-GUIDE": "Modularization Guide",
        "Architecture-MODULAR-ARCHITECTURE-ROADMAP": "Modular Architecture Roadmap",
        "Architecture-MODULE-ARCHITECTURE-INDEX": "Module Architecture Index",
        "Architecture-SOURCE-DIRECTORY-GUIDE": "Source Directory Guide",
        "Architecture-CONTENT-MODEL": "Content Model",
        "Architecture-CRYPTO-AND-KEYS": "Crypto & Keys",
        "Architecture-FEATURE-FLAGS-REFERENCE": "Feature Flags Reference",
        "Architecture-QUERY-SCHEDULING": "Query Scheduling",
        "Architecture-RESOURCE-POOLING": "Resource Pooling",
    }

    if wiki_name in _OVERRIDES:
        return _OVERRIDES[wiki_name]

    name = wiki_name

    # Plugin: strip both Plugin- prefix and -Roadmap suffix
    if name.startswith("Plugin-") and name.endswith("-Roadmap"):
        name = name[len("Plugin-"):-len("-Roadmap")]
    else:
        for prefix in (
            "Tutorial-", "Guide-", "ADR-", "Architecture-",
            "Governance-", "Developer-", "Module-",
        ):
            if name.startswith(prefix):
                name = name[len(prefix):]
                break

    def _format_word(w: str) -> str:
        upper = w.upper()
        if upper in _ACRONYMS:
            return upper
        return w.capitalize() if w else w

    words = [_format_word(w) for w in name.replace("-", " ").replace("_", " ").split()]

    # For ADR page labels: the stem after stripping "ADR-" prefix starts with "adr-"
    # again (e.g. "adr-e1-001-..."). Drop the leading "ADR" word to avoid "ADR E1 001..."
    # in the ADRs section where the header already says "ADRs".
    if words and words[0] == "ADR":
        words = words[1:]

    return " ".join(words)


def _build_wiki_index(all_wiki_names: set[str]) -> str:
    """Generate Wiki-Index.md — a full directory of all wiki pages, grouped by section.

    Every section from _SIDEBAR_SECTIONS is represented.  Pages are sorted
    alphabetically within each section.  Pages that belong to no section are
    listed in a final "Other" group.
    """
    lines: list[str] = [
        "# ThemisDB Wiki — All Pages\n\n",
        f"This page lists all **{len(all_wiki_names)}** pages in the wiki, "
        "grouped by section.\n\n",
    ]

    # Build section → page mapping using the same logic as _build_sidebar
    assigned: set[str] = set()
    section_entries: list[tuple[str, list[str]]] = []

    for section_title, explicit_pages in _SIDEBAR_SECTIONS:
        if section_title == "🏠 Overview":
            pages = [p for p in explicit_pages if p in all_wiki_names]
        elif explicit_pages:
            pages = [p for p in explicit_pages if p in all_wiki_names]
        else:
            prefix_map = {
                "📖 Tutorials": "Tutorial-",
                "🏗️ Architecture": None,
                "📐 ADRs": "ADR-",
                "📋 Governance": "Governance-",
                "🧩 Plugins": "Plugin-",
                "🤖 Developer LLM Wiki": "Developer-",
            }
            prefix = prefix_map.get(section_title)
            if prefix:
                pages = sorted(p for p in all_wiki_names if p.startswith(prefix))
            elif section_title == "🏗️ Architecture":
                pages = sorted(p for p in _ARCH_CURATED if p in all_wiki_names)
            else:
                pages = []

        section_entries.append((section_title, pages))
        assigned.update(pages)

    # Collect pages not assigned to any section
    internal = {"_Sidebar", "_Footer", "Wiki-Index"}
    unassigned = sorted(
        p for p in all_wiki_names
        if p not in assigned and p not in internal
    )

    # Render sections
    for section_title, pages in section_entries:
        if not pages:
            continue
        lines.append(f"\n## {section_title}\n\n")
        for page in sorted(pages):
            label = _sidebar_label(page)
            lines.append(f"- [[{label}|{page}]]\n")

    if unassigned:
        lines.append("\n## Other\n\n")
        for page in unassigned:
            label = _sidebar_label(page)
            lines.append(f"- [[{label}|{page}]]\n")

    return "".join(lines)


def _build_sidebar(
    all_wiki_names: set[str],
    collected_by_section: dict[str, list[str]],
) -> str:
    """Generate _Sidebar.md content with audience-first thematic sections."""
    lines: list[str] = [
        "<!-- wiki-sidebar: auto-generated | do not edit manually -->\n",
        "# ThemisDB Wiki\n",
    ]

    for section_title, explicit_pages in _SIDEBAR_SECTIONS:
        section_pages: list[str] = []

        if explicit_pages:
            # Use explicit list, keep only those that were actually written
            section_pages = [p for p in explicit_pages if p in all_wiki_names]
        else:
            # Populate dynamically from prefix matching
            prefix_map = {
                "📖 Tutorials": "Tutorial-",
                "🏗️ Architecture": None,    # curated set
                "📐 ADRs": "ADR-",
                "📋 Governance": "Governance-",
                "🧩 Plugins": "Plugin-",
                "🤖 Developer LLM Wiki": "Developer-",
            }
            prefix = prefix_map.get(section_title)
            if prefix:
                section_pages = sorted(
                    p for p in all_wiki_names if p.startswith(prefix)
                )
            elif section_title == "🏗️ Architecture":
                # Only curated architectural pages in the sidebar
                section_pages = sorted(
                    p for p in _ARCH_CURATED if p in all_wiki_names
                )

        if not section_pages:
            continue

        lines.append(f"\n## {section_title}\n\n")
        for page in section_pages:
            label = _sidebar_label(page)
            lines.append(f"- [[{label}|{page}]]\n")

    return "".join(lines)


# ---------------------------------------------------------------------------
# Footer builder
# ---------------------------------------------------------------------------

def _build_footer() -> str:
    """Generate _Footer.md for the wiki."""
    ts = datetime.now(UTC).strftime("%Y-%m-%d")
    return (
        f"<!-- wiki-footer: auto-generated | updated: {ts} -->\n\n"
        "---\n\n"
        f"**ThemisDB {THEMISDB_VERSION}** · "
        "[[Home]] · "
        "[[Wiki-Index|Wiki-Index]] · "
        "[[Module-Index]] · "
        "[[FAQ]] · "
        "[[Quick-Reference]] · "
        "[GitHub](https://github.com/makr-code/ThemisDB) · "
        "[Issues](https://github.com/makr-code/ThemisDB/issues) · "
        "[Discussions](https://github.com/makr-code/ThemisDB/discussions) · "
        "[License](https://github.com/makr-code/ThemisDB/blob/develop/LICENSE)\n"
    )


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

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
    all_wiki_names: set[str] = set()

    # First pass: determine which names pass the guardrail (for link rewriting)
    for source_path, wiki_name in entries:
        try:
            text = source_path.read_text(encoding="utf-8")
        except OSError:
            continue
        if not _contains_private(text):
            all_wiki_names.add(wiki_name)
    # Also add generated pages
    all_wiki_names.update({"Module-Index", "Wiki-Index", "_Sidebar", "_Footer"})

    # Second pass: transform and write
    for source_path, wiki_name in entries:
        try:
            text = source_path.read_text(encoding="utf-8")
        except OSError as exc:
            print(f"WARNING: cannot read {source_path}: {exc}", file=sys.stderr)
            continue

        if _contains_private(text):
            skipped.append(str(source_path.relative_to(repo_root)))
            print(
                f"BLOCKED (private content): "
                f"{source_path.relative_to(repo_root)} → {wiki_name}.md",
                file=sys.stderr,
            )
            continue

        source_rel = str(source_path.relative_to(repo_root))
        transformed = _transform(text, source_path, repo_root, wiki_name, all_wiki_names)
        header = _page_header(source_rel, wiki_name)
        footer = _page_footer(wiki_name)
        final_content = header + transformed + footer

        dest = output_dir / f"{wiki_name}.md"
        if args.dry_run:
            print(f"DRY-RUN: {source_rel} → {dest.name}")
        else:
            dest.write_text(final_content, encoding="utf-8")
            written.append(dest.name)

    # Generate Module-Index.md
    module_index_content = (
        _page_header("generated:Module-Index", "Module-Index")
        + _build_module_index(repo_root, all_wiki_names)
        + _page_footer("Module-Index")
    )
    if args.dry_run:
        print(f"DRY-RUN: <generated> → Module-Index.md")
    else:
        (output_dir / "Module-Index.md").write_text(module_index_content, encoding="utf-8")
        written.append("Module-Index.md")
    all_wiki_names.add("Module-Index")

    # Generate Wiki-Index.md (all pages directory)
    wiki_index_content = (
        _page_header("generated:Wiki-Index", "Wiki-Index")
        + _build_wiki_index(all_wiki_names)
        + _page_footer("Wiki-Index")
    )
    if args.dry_run:
        print("DRY-RUN: <generated> → Wiki-Index.md")
    else:
        (output_dir / "Wiki-Index.md").write_text(wiki_index_content, encoding="utf-8")
        written.append("Wiki-Index.md")
    all_wiki_names.add("Wiki-Index")

    # Generate _Sidebar.md
    sidebar_content = _build_sidebar(all_wiki_names, {})
    if args.dry_run:
        print(f"DRY-RUN: <generated> → _Sidebar.md")
    else:
        (output_dir / "_Sidebar.md").write_text(sidebar_content, encoding="utf-8")
        written.append("_Sidebar.md")

    # Generate _Footer.md
    footer_content = _build_footer()
    if args.dry_run:
        print(f"DRY-RUN: <generated> → _Footer.md")
    else:
        (output_dir / "_Footer.md").write_text(footer_content, encoding="utf-8")
        written.append("_Footer.md")

    print(f"\nWiki build complete: {len(written)} pages written, {len(skipped)} blocked.")
    if skipped:
        print(
            f"Blocked files (private content guardrail): {', '.join(skipped)}",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
