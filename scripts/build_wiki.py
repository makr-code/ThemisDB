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
import json
import re
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path
from time import time

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

# Term-linking patterns and configuration
GLOSSARY_INDEX_PATH = Path("docs/glossary_index.json")
GLOSSARY_TERM_LINK_PATTERN = re.compile(r"\b([A-Z][A-Z0-9]{2,})\b")  # CAPS-words
CODE_BLOCK_RE = re.compile(r"```[\s\S]*?```|`[^`]+`")  # Code blocks and inline code

# ---------------------------------------------------------------------------
# Dynamic glossary term-linking system
# ---------------------------------------------------------------------------

def _load_glossary_index(glossary_path: Path | None = None) -> dict[str, dict] | None:
    """Load glossary index for term-linking."""
    if glossary_path is None:
        glossary_path = GLOSSARY_INDEX_PATH
    
    if not glossary_path.exists():
        return None
    
    try:
        import json
        with open(glossary_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data.get("terms", {})
    except Exception:
        return None


def _inject_term_links(
    text: str,
    glossary_terms: dict[str, dict],
    min_priority: str = "high",
) -> str:
    """Inject wiki links to glossary terms in markdown text.
    
    Args:
        text: Markdown content to process
        glossary_terms: Glossary index (term_id → term data)
        min_priority: Minimum term priority to link ('high', 'medium', 'low')
    
    Returns:
        Markdown with injected glossary term links
    """
    priority_order = {"high": 2, "medium": 1, "low": 0}
    min_priority_level = priority_order.get(min_priority, 0)
    
    # Filter glossary terms by priority
    linkable_terms = {
        term_name: term_data
        for term_name, term_data in glossary_terms.items()
        if priority_order.get(term_data.get("priority", "low"), 0) >= min_priority_level
    }
    
    if not linkable_terms:
        return text
    
    # Identify code blocks to preserve
    code_blocks = []
    def preserve_code(match):
        idx = len(code_blocks)
        code_blocks.append(match.group(0))
        return f"__CODE_BLOCK_{idx}__"
    
    text = CODE_BLOCK_RE.sub(preserve_code, text)
    
    # Inject term links
    def replace_term(match):
        term_name = match.group(1)
        
        # Find matching term (case-insensitive)
        for term_id, term_data in linkable_terms.items():
            if term_data.get("name", "").lower() == term_name.lower():
                # Check if already linked
                full_match_start = match.start()
                if full_match_start > 0 and text[full_match_start - 1] == "[":
                    # Already inside a link
                    return match.group(0)
                
                # Create wiki link
                display_name = term_data.get("name", term_name)
                # Wiki link format: [[Display Name|Wiki-Page-Name]] or [[Display Name]]
                return f"[[{display_name}]]"
        
        return match.group(0)
    
    # Apply term linking
    text = GLOSSARY_TERM_LINK_PATTERN.sub(replace_term, text)
    
    # Restore code blocks
    for idx, code_block in enumerate(code_blocks):
        text = text.replace(f"__CODE_BLOCK_{idx}__", code_block)
    
    return text


# Map wiki page prefixes and patterns to their breadcrumb hierarchies
# Format: (category_name, breadcrumb_path_items, page_prefix_pattern)
_BREADCRUMB_HIERARCHY: dict[str, tuple[str, list[str]]] = {
    # Getting Started
    "Home": ("Getting Started", ["Home"]),
    "Quickstart": ("Getting Started", ["Home", "Getting Started"]),
    "Setup": ("Getting Started", ["Home", "Getting Started"]),
    "Repository-README": ("Getting Started", ["Home", "Getting Started"]),
    "FAQ": ("Getting Started", ["Home", "Getting Started"]),
    "Quick-Reference": ("Getting Started", ["Home", "Getting Started"]),
    # Tutorials & Guides
    "Tutorial-": ("Tutorials", ["Home", "Learning", "Tutorials"]),
    "Guide-": ("Guides", ["Home", "Learning", "Guides"]),
    # Operations & Deployment
    "Ops-": ("Operations", ["Home", "Operations"]),
    "Deploy-": ("Deployment", ["Home", "Operations", "Deployment"]),
    "Docker-": ("Docker", ["Home", "Operations", "Docker"]),
    "Helm-": ("Kubernetes", ["Home", "Operations", "Kubernetes"]),
    "Packaging-": ("Packaging", ["Home", "Operations", "Packaging"]),
    # Architecture & Design
    "Architecture-": ("Architecture", ["Home", "Architecture"]),
    "Root-Architecture": ("Architecture", ["Home", "Architecture"]),
    "Module-": ("Modules", ["Home", "Modules"]),
    "ADR-": ("Architecture", ["Home", "Architecture", "ADRs"]),
    "Plugin-": ("Plugins", ["Home", "Plugins"]),
    # Security & Operations
    "Security-": ("Security", ["Home", "Security"]),
    "Root-Security": ("Security", ["Home", "Security"]),
    # API & Integration
    "API-Reference": ("API Reference", ["Home", "API & Integration", "API Reference"]),
    "AQL-": ("AQL", ["Home", "API & Integration", "AQL"]),
    "Integration-Guide": ("API & Integration", ["Home", "API & Integration"]),
    "Migration-Guide": ("API & Integration", ["Home", "API & Integration", "Migration"]),
    # Governance & Contributing
    "Governance-": ("Governance", ["Home", "Contributing", "Governance"]),
    "Root-Governance": ("Governance", ["Home", "Contributing", "Governance"]),
    "Contributing": ("Contributing", ["Home", "Contributing"]),
    "Root-Contributing": ("Contributing", ["Home", "Contributing"]),
    "Code-of-Conduct": ("Contributing", ["Home", "Contributing"]),
    # Release & Versioning
    "Root-Roadmap": ("Roadmap", ["Home", "Roadmap"]),
    "Root-Changelog": ("Changelog", ["Home", "Changelog"]),
    "Versioning": ("Release", ["Home", "Release", "Versioning"]),
    "Release-Strategy": ("Release", ["Home", "Release", "Release Strategy"]),
    "Branching-Strategy": ("Release", ["Home", "Release", "Branching"]),
    # SDKs & Clients
    "Client-": ("SDKs", ["Home", "SDKs", "Client"]),
    "SDK-": ("SDKs", ["Home", "SDKs"]),
    # Developer Resources
    "Developer-": ("Developer Resources", ["Home", "Developer Resources"]),
    "DevGuide-": ("Developer Resources", ["Home", "Developer Resources"]),
    # Examples & Training
    "Example-": ("Examples", ["Home", "Examples"]),
    "Training-": ("Training", ["Home", "Training"]),
    "Demo-": ("Demo", ["Home", "Demo"]),
}


def _get_breadcrumb_path(wiki_name: str) -> tuple[str, list[str]]:
    """Determine breadcrumb path for a wiki page based on its name pattern."""
    # Exact match first
    if wiki_name in _BREADCRUMB_HIERARCHY:
        return _BREADCRUMB_HIERARCHY[wiki_name]
    # Prefix match
    for prefix, breadcrumb_info in _BREADCRUMB_HIERARCHY.items():
        if prefix.endswith("-") and wiki_name.startswith(prefix):
            return breadcrumb_info
    # Default
    return ("Pages", ["Home", "Pages"])


def _format_breadcrumb_nav(wiki_name: str, all_wiki_names: set[str]) -> str:
    """Generate breadcrumb navigation markdown for a wiki page."""
    category_label, breadcrumb_items = _get_breadcrumb_path(wiki_name)
    
    # Build breadcrumb with [[Wiki Links]] for valid pages
    breadcrumb_parts = []
    for item in breadcrumb_items:
        if item == wiki_name:
            # Current page (no link)
            breadcrumb_parts.append(f"**{item}**")
        elif item == "Home":
            # Special case: always linkable
            breadcrumb_parts.append("[[Home|Home]]")
        elif f"{item}-Index" in all_wiki_names:
            # Category index page exists
            breadcrumb_parts.append(f"[[{item}|{item}-Index]]")
        else:
            # Fallback: plain text
            breadcrumb_parts.append(item)
    
    breadcrumb_line = " > ".join(breadcrumb_parts)
    return f"> **Navigation:** {breadcrumb_line}\n"


# ---------------------------------------------------------------------------
# Document currency and sorting system
# ---------------------------------------------------------------------------

_FRESHNESS_CUTOFF_DAYS = 30  # Documents modified within this period are "fresh"


def _get_file_mtime_timestamp(path: Path) -> float:
    """Get file modification time as Unix timestamp."""
    try:
        return path.stat().st_mtime
    except OSError:
        return 0.0


def _extract_content_currency(text: str) -> tuple[bool, str]:
    """Extract currency indicators from document content.
    
    Returns: (is_fresh, extracted_date_str)
    """
    # Look for markdown metadata patterns
    patterns = [
        r"[Ll]ast[\s-]?[Mm]odified:\s*(\d{4}-\d{2}-\d{2})",
        r"[Uu]pdated[\s-]?at:\s*(\d{4}-\d{2}-\d{2})",
        r"[Uu]pdated:\s*(\d{4}-\d{2}-\d{2})",
        r"[Gg]enerated[\s-]?at:\s*(\d{4}-\d{2}-\d{2})",
        r"date:\s*(\d{4}-\d{2}-\d{2})",
    ]
    
    for pattern in patterns:
        m = re.search(pattern, text)
        if m:
            date_str = m.group(1)
            try:
                from datetime import datetime as dt_module
                doc_date = dt_module.strptime(date_str, "%Y-%m-%d")
                now = dt_module.now()
                age_days = (now - doc_date).days
                is_fresh = age_days <= _FRESHNESS_CUTOFF_DAYS
                return is_fresh, date_str
            except (ValueError, AttributeError):
                pass
    
    return False, ""


def _compute_currency_score(source_path: Path, text: str) -> float:
    """Compute a currency score for sorting documents.
    
    Higher scores = more recent/fresher documents.
    Score range: [0.0, 100.0]
    """
    mtime = _get_file_mtime_timestamp(source_path)
    now = time()
    age_seconds = now - mtime
    age_days = age_seconds / (24 * 3600)
    
    # Base score: older files get lower scores
    # 0-7 days: 90-100
    # 7-30 days: 70-90
    # 30-90 days: 50-70
    # 90+ days: 0-50
    if age_days <= 7:
        base_score = 90 + (7 - age_days) / 7 * 10
    elif age_days <= 30:
        base_score = 70 + (30 - age_days) / 23 * 20
    elif age_days <= 90:
        base_score = 50 + (90 - age_days) / 60 * 20
    else:
        base_score = max(0, 50 - (age_days - 90) / 365 * 50)
    
    # Bonus: check for content freshness markers
    is_content_fresh, _ = _extract_content_currency(text)
    if is_content_fresh:
        base_score = min(100, base_score + 10)
    
    return min(100.0, base_score)


def _sort_entries_by_currency(
    entries: list[tuple[Path, str]],
    repo_root: Path,
) -> list[tuple[Path, str, float]]:
    """Sort entries by document currency (modification date + content freshness).
    
    Returns list of (path, wiki_name, currency_score) tuples sorted by score descending.
    """
    entries_with_scores: list[tuple[Path, str, float]] = []
    
    for source_path, wiki_name in entries:
        try:
            text = source_path.read_text(encoding="utf-8")
            score = _compute_currency_score(source_path, text)
        except OSError:
            score = 0.0
        entries_with_scores.append((source_path, wiki_name, score))
    
    # Sort by score descending (fresher documents first)
    entries_with_scores.sort(key=lambda x: (-x[2], x[1]))
    return entries_with_scores



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
    enable_breadcrumbs: bool = True,
) -> str:
    """Apply all transformations to markdown content for wiki publication.
    
    Transformations include:
    - Remove existing HTML comments
    - Strip Doxygen tags
    - Remove CI badges
    - Rewrite relative links to wiki links
    - Collapse multiple blank lines
    - Add breadcrumb navigation (if enabled)
    """
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
    "docs/aql/README.md": "AQL-Overview",
    "docs/aql/AQL_COMPLETE_FEATURE_ROADMAP.md": "AQL-Feature-Roadmap",
    "docs/aql/AQL_GEOSPATIAL_OPTIMIZATION_GUIDE.md": "AQL-Geospatial-Guide",
    "docs/aql/AQL_LLM_INTEGRATION_MIGRATION_GUIDE.md": "AQL-LLM-Migration-Guide",
    "docs/aql/API.md": "AQL-API",
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

    # Primary wiki home is generated by _build_home_page() in main(); skip static file.
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

    # ---- Root-level governance & meta docs ------------------------------------

    _ROOT_DOCS: list[tuple[str, str]] = [
        ("CHANGELOG.md",          "Root-Changelog"),
        ("ARCHITECTURE.md",       "Root-Architecture"),
        ("ROADMAP.md",            "Root-Roadmap"),
        ("FUTURE_ENHANCEMENTS.md","Root-Future-Enhancements"),
        ("CONTRIBUTING.md",       "Root-Contributing"),
        ("SECURITY.md",           "Root-Security"),
        ("CODE_OF_CONDUCT.md",    "Code-of-Conduct"),
        ("SUPPORT.md",            "Support"),
        ("MAINTAINERS.md",        "Maintainers"),
        ("GOVERNANCE.md",         "Root-Governance"),
        ("VERSIONING.md",         "Versioning"),
        ("BRANCHING_STRATEGY.md", "Branching-Strategy"),
        ("RELEASE_STRATEGY.md",   "Release-Strategy"),
        ("QUICKSTART.md",         "Quickstart"),
        ("SETUP.md",              "Setup"),
        ("CTEST.md",              "CTest-Guide"),
    ]
    for filename, wiki_name in _ROOT_DOCS:
        _add(repo_root / filename, wiki_name)

    # ---- Client SDKs ----------------------------------------------------------

    clients_dir = repo_root / "clients"
    if clients_dir.is_dir():
        # Per-language READMEs
        for f in sorted(clients_dir.glob("*/README.md")):
            lang = _slug(f.parent.name)
            _add(f, f"Client-{lang}")
        # Top-level summary docs
        for f in sorted(clients_dir.glob("*.md")):
            if f.stem.upper() == "README":
                _add(f, "Client-Overview")
            else:
                _add(f, f"Client-{_slug(f.stem)}")

    # ---- SDKs -----------------------------------------------------------------

    sdks_dir = repo_root / "sdks"
    if sdks_dir.is_dir():
        _add(sdks_dir / "README.md", "SDK-Overview")
        for f in sorted(sdks_dir.glob("*/README.md")):
            _add(f, f"SDK-{_slug(f.parent.name)}")

    # ---- Examples (one page per example directory, use its README) ------------

    examples_dir = repo_root / "examples"
    if examples_dir.is_dir():
        # Top-level ROADMAP/ARCHITECTURE of examples module if present
        for fname, suffix in _MODULE_DOC_TYPES:
            _add(examples_dir / fname, f"Examples-{suffix}")
        # Per-example READMEs
        for readme in sorted(examples_dir.glob("*/README.md")):
            ex_name = _slug(readme.parent.name)
            _add(readme, f"Example-{ex_name}")

    # ---- Demo -----------------------------------------------------------------

    demo_dir = repo_root / "demo"
    if demo_dir.is_dir():
        for f in sorted(demo_dir.glob("*.md")):
            stem = f.stem.upper()
            name_map = {
                "README": "Demo-Overview",
                "QUICKSTART": "Demo-Quickstart",
                "DEMO_QUERIES": "Demo-Queries",
            }
            _add(f, name_map.get(stem, f"Demo-{_slug(f.stem)}"))
        # Setup sub-dir
        for f in sorted((demo_dir / "setup").glob("*.md")) if (demo_dir / "setup").is_dir() else []:
            _add(f, f"Demo-{_slug(f.stem)}")

    # ---- DeveloperGuide -------------------------------------------------------

    devguide_dir = repo_root / "DeveloperGuide"
    if devguide_dir.is_dir():
        for f in sorted(devguide_dir.glob("*.md")):
            _add(f, f"DevGuide-{_slug(f.stem)}")

    # ---- Adapters -------------------------------------------------------------

    adapters_dir = repo_root / "adapters"
    if adapters_dir.is_dir():
        for adapter_dir in sorted(d for d in adapters_dir.iterdir() if d.is_dir()):
            aname = _slug(adapter_dir.name)
            for fname, suffix in _MODULE_DOC_TYPES:
                _add(adapter_dir / fname, f"Adapter-{aname}-{suffix}")
            _add(adapter_dir / "README.md", f"Adapter-{aname}-README")

    # ---- API module (root-level, like src modules) ----------------------------

    api_dir = repo_root / "api"
    if api_dir.is_dir():
        for fname, suffix in _MODULE_DOC_TYPES:
            _add(api_dir / fname, f"Api-Module-{suffix}")
        _add(api_dir / "README.md", "Api-Module-README")

    # ---- Operator (Kubernetes operator) ---------------------------------------

    operator_dir = repo_root / "operator"
    if operator_dir.is_dir():
        _add(operator_dir / "README.md", "Operator-Overview")
        for sub in sorted(d for d in operator_dir.iterdir() if d.is_dir()):
            sname = _slug(sub.name)
            for fname, suffix in _MODULE_DOC_TYPES:
                _add(sub / fname, f"Operator-{sname}-{suffix}")
            _add(sub / "README.md", f"Operator-{sname}-README")

    # ---- Deploy / Docker / Helm / Packaging -----------------------------------

    # deploy/
    deploy_dir = repo_root / "deploy"
    if deploy_dir.is_dir():
        _add(deploy_dir / "README.md", "Deploy-Overview")
        for f in sorted(deploy_dir.glob("*/README.md")):
            _add(f, f"Deploy-{_slug(f.parent.name)}")

    # docker/
    docker_dir = repo_root / "docker"
    if docker_dir.is_dir():
        _add(docker_dir / "DOCKERHUB_README.md", "Docker-Hub-README")
        _add(docker_dir / "README.md", "Docker-Overview")
        for f in sorted(docker_dir.glob("*/README.md")):
            _add(f, f"Docker-{_slug(f.parent.name)}")

    # helm/
    helm_dir = repo_root / "helm"
    if helm_dir.is_dir():
        _add(helm_dir / "README.md", "Helm-Overview")
        for f in sorted(helm_dir.glob("*/README.md")):
            _add(f, f"Helm-{_slug(f.parent.name)}")

    # packaging/
    pkg_dir = repo_root / "packaging"
    if pkg_dir.is_dir():
        _add(pkg_dir / "README.md", "Packaging-Overview")
        for sub in sorted(d for d in pkg_dir.iterdir() if d.is_dir()):
            sname = _slug(sub.name)
            for fname, suffix in _MODULE_DOC_TYPES:
                _add(sub / fname, f"Packaging-{sname}-{suffix}")
            _add(sub / "README.md", f"Packaging-{sname}-README")

    # ---- OpenAPI / Proto ------------------------------------------------------

    _add(repo_root / "openapi" / "README.md", "OpenAPI-Overview")
    _add(repo_root / "proto" / "README.md", "Proto-Overview")

    # ---- Tools (selective: READMEs + ROADMAP for named tools) -----------------

    tools_dir = repo_root / "tools"
    if tools_dir.is_dir():
        _add(tools_dir / "STATUS.md", "Tools-Status")
        for sub in sorted(d for d in tools_dir.iterdir() if d.is_dir()):
            sname = _slug(sub.name)
            _add(sub / "README.md", f"Tool-{sname}-README")
            _add(sub / "ROADMAP.md", f"Tool-{sname}-Roadmap")

    # ---- Scripts (BUILD_QUICK_REF only) ---------------------------------------

    _add(repo_root / "scripts" / "BUILD_QUICK_REF.md", "Scripts-Build-Quick-Ref")

    # ---- Audit (current baseline reports only, not dated point-in-time files) -

    audit_dir = repo_root / "audit"
    if audit_dir.is_dir():
        _AUDIT_KEEP = {
            "MATURITY_REPORT_2026-08.md":           "Audit-Maturity-Report",
            "PRODUCTION_READINESS_ASSESSMENT_2026-08-18.md": "Audit-Production-Readiness",
            "BSI_C5_2026_THEMISDB_AUDIT.md":        "Audit-BSI-C5",
        }
        for fname, wiki_name in _AUDIT_KEEP.items():
            _add(audit_dir / fname, wiki_name)

    # ---- Security (public compliance docs only) -------------------------------

    security_dir = repo_root / "security"
    if security_dir.is_dir():
        _add(security_dir / "DSGVO_SOC2_COMPLIANCE_CHECKLIST.md", "Security-DSGVO-SOC2-Checklist")

    # ---- docs/operations — operational runbooks & guides ----------------------

    ops_dir = repo_root / "docs" / "operations"
    if ops_dir.is_dir():
        # Top-level ops docs
        for f in sorted(ops_dir.glob("*.md")):
            stem = f.stem.upper()
            name_map = {
                "README": "Ops-Overview",
                "OPERATIONS_RUNBOOK": "Ops-Runbook",
                "OPERATIONS_HANDBOOK": "Ops-Handbook",
                "THEMISCTL_ADMIN_GUIDE": "Ops-ThemisCtl-Admin-Guide",
                "PIPELINE_E2E_SOPS": "Ops-Pipeline-E2E-SOPs",
                "ACCESS_MODEL_RUNBOOKS": "Ops-Access-Model-Runbooks",
                "ACCESS_MODEL_DASHBOARD_GUIDE": "Ops-Access-Model-Dashboard",
                "MATURITY_AUTOMATION_RUNBOOK": "Ops-Maturity-Automation",
                "PENTEST_AUTOMATION_SCHEDULE": "Ops-Pentest-Automation",
            }
            _add(f, name_map.get(stem, f"Ops-{_slug(f.stem)}"))
        # LLM ops runbooks
        llm_ops = ops_dir / "llm"
        if llm_ops.is_dir():
            for f in sorted(llm_ops.glob("*.md")):
                _add(f, f"Ops-LLM-{_slug(f.stem)}")
        # Disaster recovery
        dr_dir = ops_dir / "disaster-recovery"
        if dr_dir.is_dir():
            for f in sorted(dr_dir.glob("*.md")):
                _add(f, f"Ops-DR-{_slug(f.stem)}")
        # Access management
        am_dir = ops_dir / "access-management"
        if am_dir.is_dir():
            for f in sorted(am_dir.glob("*.md")):
                _add(f, f"Ops-Access-{_slug(f.stem)}")
        # Incident response
        ir_dir = ops_dir / "incident-response"
        if ir_dir.is_dir():
            for f in sorted(ir_dir.glob("*.md")):
                _add(f, f"Ops-IR-{_slug(f.stem)}")
        # Logging
        log_dir = ops_dir / "logging"
        if log_dir.is_dir():
            for f in sorted(log_dir.glob("*.md")):
                _add(f, f"Ops-Logging-{_slug(f.stem)}")

    # ---- docs/security — public security docs --------------------------------

    docs_security_dir = repo_root / "docs" / "security"
    if docs_security_dir.is_dir():
        _SECURITY_KEEP = {
            "INFORMATION_SECURITY_POLICY.md":      "Security-Policy",
            "PRODUCTION_HARDENING_CHECKLIST.md":   "Security-Hardening-Checklist",
            "ENCRYPTION_KEY_MANAGEMENT_POLICY.md": "Security-Key-Management",
            "access_control_framework.md":         "Security-Access-Control",
            "THEMISDB_SECURITY_HARDENING_GUIDE.md":"Security-Hardening-Guide",
            "zero_trust_policy_enforcer.md":       "Security-Zero-Trust",
            "api_authentication_authorization.md": "Security-API-Auth",
            "HSM_PRODUCTION_SETUP.md":             "Security-HSM-Setup",
            "PKCS11_INTEGRATION.md":               "Security-PKCS11",
        }
        for fname, wiki_name in _SECURITY_KEEP.items():
            _add(docs_security_dir / fname, wiki_name)

    # ---- AQL Grammar (EBNF → Markdown wrapper) --------------------------------

    aql_root = repo_root / "aql"
    if aql_root.is_dir():
        _add(aql_root / "README.md", "AQL-Root-Overview")
        _add(aql_root / "examples" / "README.md", "AQL-Examples-Root")
        # EBNF grammar: generate a Markdown wrapper page on the fly
        ebnf_v13 = aql_root / "AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf"
        ebnf_base = aql_root / "AQL_GRAMMAR.ebnf"
        # Prefer extended version; fall back to base
        grammar_file = ebnf_v13 if ebnf_v13.exists() else ebnf_base
        if grammar_file.exists():
            # Synthetic entry: file exists but needs EBNF→MD wrapping
            _add(grammar_file, "AQL-Grammar")

    # ---- schulung/ — training materials (German) ------------------------------

    schulung_dir = repo_root / "schulung"
    if schulung_dir.is_dir():
        _add(schulung_dir / "README.md", "Training-Overview")
        # Structured documents
        dok_dir = schulung_dir / "dokumente"
        if dok_dir.is_dir():
            for f in sorted(dok_dir.glob("*.md")):
                if f.stem.upper() == "README":
                    _add(f, "Training-Docs-Overview")
                else:
                    _add(f, f"Training-Doc-{_slug(f.stem)}")
        # Presentations
        pres_dir = schulung_dir / "praesentation"
        if pres_dir.is_dir():
            for f in sorted(pres_dir.glob("*.md")):
                if f.stem.upper() == "README":
                    _add(f, "Training-Pres-Overview")
                else:
                    _add(f, f"Training-Pres-{_slug(f.stem)}")
        # Per-example READMEs (already partially added above)
        for readme in sorted(schulung_dir.glob("examples/*/README.md")):
            _add(readme, f"Training-{_slug(readme.parent.name)}")

    return entries


# ---------------------------------------------------------------------------
# Home page generator
# ---------------------------------------------------------------------------

def _build_home_page(repo_root: Path, all_wiki_names: set[str]) -> str:
    """Generate the wiki Home page from multiple repository sources.

    Aggregates tagline (README), badges (README), key capabilities (README/ARCHITECTURE),
    getting-started snippet (QUICKSTART/SETUP), module overview table (src/*/ROADMAP.md),
    latest changelog entry (CHANGELOG.md), roadmap snapshot (ROADMAP.md),
    and contributing hint (CONTRIBUTING.md).
    """
    import re as _re
    import datetime as _dt

    generated_at = _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%d")

    def _read(relpath: str) -> str:
        p = repo_root / relpath
        try:
            return p.read_text(encoding="utf-8")
        except OSError:
            return ""

    def _section_text(text: str, heading: str, max_lines: int = 30) -> str:
        """Extract lines from a markdown section starting at `heading`."""
        lines = text.splitlines()
        in_section = False
        result: list[str] = []
        for line in lines:
            if _re.match(rf"^#{'{1,3}'}\s+{_re.escape(heading)}", line):
                in_section = True
                continue
            if in_section:
                if _re.match(r"^#{1,3}\s", line) and result:
                    break
                result.append(line)
                if len(result) >= max_lines:
                    break
        return "\n".join(result).strip()

    # ---- 1. Tagline & Description -------------------------------------------
    readme = _read("README.md")
    tagline = "High-performance multi-model database with native AI/LLM integration"
    description_lines: list[str] = []
    m = _re.search(r"\*\*(.+?)\*\*\s*\n", readme)
    if m:
        tagline = m.group(1).strip()

    # Extract module status counts from README
    pc_count = harden_count = exp_count = thin_count = total_count = 0
    m_pc = _re.search(r"\*\*(\d+)\s+modules?\*\*\s+are\s+`PRODUCTION_CANDIDATE`", readme)
    m_hrd = _re.search(r"\*\*(\d+)\s+modules?\*\*\s+are\s+`HARDENING`", readme)
    m_exp = _re.search(r"\*\*(\d+)\s+modules?\*\*\s+are\s+`EXPERIMENTAL`", readme)
    m_thn = _re.search(r"\*\*(\d+)\s+modules?\*\*\s+are\s+`THIN", readme)
    if m_pc:
        pc_count = int(m_pc.group(1))
    if m_hrd:
        harden_count = int(m_hrd.group(1))
    if m_exp:
        exp_count = int(m_exp.group(1))
    if m_thn:
        thin_count = int(m_thn.group(1))
    total_count = pc_count + harden_count + exp_count + thin_count

    # ---- 2. Badges ----------------------------------------------------------
    # Extract badge lines from README (lines containing shields.io)
    badge_lines: list[str] = []
    for line in readme.splitlines():
        if "shields.io" in line or "github.com/makr-code/ThemisDB/actions" in line:
            stripped = line.strip()
            if stripped.startswith("[!["):
                badge_lines.append(stripped)
    badges_md = "\n".join(badge_lines[:8]) if badge_lines else ""

    # ---- 3. Version + maturity from README badges ---------------------------
    version = "2.x"
    m_ver = _re.search(r"version-([^-\"]+(?:-[a-z]+)?)-blue", readme)
    if m_ver:
        version = m_ver.group(1).replace("_", ".")

    # ---- 4. Key capabilities bullets ----------------------------------------
    # From README "Key Capabilities" section or fallback ARCHITECTURE.md
    caps_section = _section_text(readme, "Key Capabilities")
    if not caps_section:
        arch_text = _read("docs/ARCHITECTURE.md") or _read("docs/en/ARCHITECTURE.md")
        caps_section = _section_text(arch_text, "Key Features") or _section_text(arch_text, "Capabilities")
    # Keep only bullet lines
    cap_bullets = [
        ln for ln in caps_section.splitlines()
        if ln.strip().startswith(("-", "*", "•", "🔒", "🔍", "🚀", "🛡", "🌐", "🧠"))
    ][:8]
    caps_md = "\n".join(cap_bullets) if cap_bullets else (
        "- 🔒 ACID transactions with snapshot isolation (MVCC)\n"
        "- 🔍 Multi-model: relational, graph, vector, document in one system\n"
        "- 🚀 High-performance storage engine built on RocksDB\n"
        "- 🧠 AI-Ready: hybrid search, embedding cache, LLM integration\n"
        "- 🌐 Distributed: sharding, replication, Kubernetes-ready\n"
        "- 🛡️ Enterprise security: TLS 1.3, RBAC, encryption, audit logging"
    )

    # ---- 5. Getting Started snippet -----------------------------------------
    quickstart = _read("QUICKSTART.md") or _read("docs/QUICK_START.md") or _read("SETUP.md")
    gs_snippet = ""
    if quickstart:
        # Find first fenced code block
        m_code = _re.search(r"```(?:bash|sh|console)?\n(.+?)```", quickstart, _re.DOTALL)
        if m_code:
            gs_snippet = "```bash\n" + m_code.group(1).strip() + "\n```"

    # ---- 6. Module overview table (top modules by status) -------------------
    src_dir = repo_root / "src"
    module_rows: list[tuple[str, str, str, str, int]] = []
    # Parse production-candidate list from README for accurate status
    pc_modules: set[str] = set()
    m_pclist = _re.search(
        r"production-candidate modules\*\*\s+verified:\s*([^\n]+)", readme, _re.IGNORECASE
    )
    if m_pclist:
        pc_modules = {n.strip() for n in m_pclist.group(1).split(",")}
    exp_modules: set[str] = set()
    m_explist = _re.search(r"EXPERIMENTAL`\s+\(([^)]+)\)", readme)
    if m_explist:
        exp_modules = {n.strip().strip("`'\"") for n in m_explist.group(1).split(",")}
    thin_modules: set[str] = set()
    m_thinlist = _re.search(r"THIN/PLACEHOLDER`\s+\(([^)]+)\)", readme)
    if m_thinlist:
        thin_modules = {n.strip().strip("`'\"") for n in m_thinlist.group(1).split(",")}

    if src_dir.is_dir():
        for mod_dir in sorted(src_dir.iterdir()):
            if not mod_dir.is_dir():
                continue
            module = mod_dir.name
            roadmap_path = mod_dir / "ROADMAP.md"
            status_text = "—"
            if roadmap_path.exists():
                try:
                    rt = roadmap_path.read_text(encoding="utf-8")
                    status_text = _extract_status(rt)
                except OSError:
                    pass
            slug = _slug(module)
            wiki_name = f"Module-{slug}-Roadmap"
            link = f"[[{module}|{wiki_name}]]" if wiki_name in all_wiki_names else f"`{module}`"
            if module in pc_modules:
                emoji, order = "✅", 0
            elif module in exp_modules:
                emoji, order = "🔴", 2
            elif module in thin_modules:
                emoji, order = "⚪", 3
            else:
                emoji, order = "🟡", 1  # default: hardening
            module_rows.append((module, emoji, status_text[:60], link, order))
    # Sort: production first, then hardening, then rest; alpha within group
    module_rows.sort(key=lambda r: (r[4], r[0]))

    module_table_lines = [
        "| Module | Status | Summary |\n",
        "|--------|:------:|--------|\n",
    ]
    for module, emoji, status_text, link, _ in module_rows:
        module_table_lines.append(f"| {link} | {emoji} | {status_text} |\n")
    module_table = "".join(module_table_lines)

    # ---- 7. Changelog snapshot ----------------------------------------------
    changelog = _read("CHANGELOG.md")
    changelog_snippet = ""
    changelog_version = ""
    if changelog:
        # Find first versioned release block (## [x.y.z])
        m_cl = _re.search(
            r"^(## \[(\d+\.\d+[\.\d]*.*?)\] - .+?)\n(.*?)(?=^## \[|\Z)",
            changelog, _re.MULTILINE | _re.DOTALL
        )
        if m_cl:
            changelog_version = m_cl.group(2)
            block_lines = [m_cl.group(1)] + m_cl.group(3).strip().splitlines()
            changelog_snippet = "\n".join(block_lines[:12]).strip()
            if len(m_cl.group(3).strip().splitlines()) > 12:
                changelog_snippet += "\n\n_[…see [[Root-Changelog]] for full details]_"

    # ---- 8. Roadmap snapshot ------------------------------------------------
    roadmap = _read("ROADMAP.md")
    roadmap_wave = ""
    roadmap_active: list[str] = []
    if roadmap:
        # Current wave indicator
        m_wave = _re.search(r"Wave\s+([A-D])\s*[–—-]\s*(.+)", roadmap)
        if m_wave:
            roadmap_wave = f"**Wave {m_wave.group(1)}** — {m_wave.group(2).strip()[:80]}"
        # Active items [~]
        for line in roadmap.splitlines():
            if line.strip().startswith("- [~]"):
                item = line.strip()[6:].strip()
                roadmap_active.append(f"- 🔄 {item[:100]}")
            if len(roadmap_active) >= 5:
                break

    # ---- 9. Contributing blurb ----------------------------------------------
    contributing = _read("CONTRIBUTING.md")
    contrib_blurb = "Contributions are welcome! See the [[Contributing]] guide for guidelines."
    if contributing:
        # Find first real non-header paragraph
        for line in contributing.splitlines():
            stripped = line.strip()
            if stripped and not stripped.startswith("#") and not stripped.startswith("|") \
                    and not stripped.startswith("<") and len(stripped) > 30:
                contrib_blurb = stripped[:200]
                break

    # ---- Assemble -----------------------------------------------------------
    # Quick-links bar: plain links (no display alias) to avoid validator false-positives
    quick_links = (
        "| [[Demo-Quickstart]] "
        "| [[Training-Pres-04-installation-und-setup]] "
        "| [[AQL-Overview]] "
        "| [[Module-Index]] "
        "| [[Root-Changelog]] "
        "| [[Root-Future-Enhancements]] "
        "| [[Wiki-Index]] |"
    )
    quick_links_header = (
        "| 🚀 Quick Start "
        "| 📦 Install "
        "| 📖 AQL Guide "
        "| 🔧 Module Index "
        "| 📋 Changelog "
        "| 🗺️ Roadmap "
        "| 📑 All Pages |"
    )

    parts: list[str] = [
        f"# ThemisDB\n\n",
        f"> {tagline}\n\n",
    ]

    if badges_md:
        parts.append(badges_md + "\n\n")

    parts.append("---\n\n")

    # Quick Links table — header row with labels, then link row
    parts.append(f"{quick_links_header}\n|---|---|---|---|---|---|---|\n{quick_links}\n\n")
    parts.append("---\n\n")

    # What is ThemisDB
    parts.append("## What is ThemisDB?\n\n")
    parts.append(caps_md + "\n\n")

    # Module status snapshot
    if total_count > 0:
        parts.append("## Module Status Snapshot\n\n")
        parts.append(
            f"**{total_count} modules** across the codebase: "
            f"✅ {pc_count} Production-Candidate · "
            f"🟡 {harden_count} Hardening · "
            f"🔴 {exp_count} Experimental · "
            f"⚪ {thin_count} Thin/Placeholder\n\n"
        )
        parts.append(module_table + "\n")
        parts.append(f"_Full details: [[Module-Index]]_\n\n")

    # Getting Started
    if gs_snippet:
        parts.append("## Getting Started\n\n")
        parts.append(gs_snippet + "\n\n")
        parts.append("➡️ Full guide: [[Demo-Quickstart]] · [[Training-Pres-04-installation-und-setup]]\n\n")

    # Architecture
    parts.append("## Architecture Overview\n\n")
    arch_links = [
        p for p in ("Architecture-OVERVIEW", "Architecture-SYSTEM-OVERVIEW", "Architecture-COMPONENTS")
        if p in all_wiki_names
    ]
    if arch_links:
        parts.append(" · ".join(f"[[{p}]]" for p in arch_links) + "\n\n")
    parts.append("See the [[Module-Index]] for per-module architecture pages.\n\n")

    # Changelog
    parts.append("## Latest Release\n\n")
    if changelog_snippet:
        parts.append(changelog_snippet + "\n\n")
    parts.append("➡️ Full history: [[Root-Changelog]]\n\n")

    # Roadmap
    parts.append("## Roadmap & Current Focus\n\n")
    if roadmap_wave:
        parts.append(roadmap_wave + "\n\n")
    if roadmap_active:
        parts.append("**Active work:**\n" + "\n".join(roadmap_active) + "\n\n")
    parts.append("➡️ Full roadmap: [[Root-Future-Enhancements]]\n\n")

    # Contributing
    parts.append("## Contributing\n\n")
    parts.append(contrib_blurb + "\n\n")
    parts.append(
        "- [[Root-Contributing]]\n"
        "- [[Developer-INDEX]]\n"
        "- [[Branching-Strategy]]\n\n"
    )

    # Footer note
    parts.append("---\n\n")
    parts.append(
        f"_This page is auto-generated on every wiki publish from repository sources. "
        f"Last generated: {generated_at}_\n"
    )

    return "".join(parts)


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
        "Root-Changelog",
        "Root-Roadmap",
        "Versioning",
    ]),
    # 2. Getting Started — first steps for new users
    ("🚀 Getting Started", [
        "Quick-Reference",
        "Quickstart",
        "Setup",
        "Integration-Guide",
        "Migration-Guide",
        "Demo-Overview",
        "Demo-Quickstart",
    ]),
    # 3. Tutorials — hands-on walkthroughs (broad → narrow)
    ("📖 Tutorials", []),       # populated dynamically from Tutorial-* pages
    # 4. User Guide — reference material users need day-to-day
    ("📗 User Guide", [
        "AQL-Reference",
        "AQL-Examples",
        "AQL-Overview",
        "AQL-Feature-Roadmap",
        "AQL-Geospatial-Guide",
        "AQL-LLM-Migration-Guide",
        "AQL-API",
        "AQL-Grammar",
        "AQL-Root-Overview",
        "AQL-Examples-Root",
        "API-Reference",
        "Api-Module-README",
        "OpenAPI-Overview",
        "Client-Overview",
        "SDK-Overview",
    ]),
    # 5. Operations & Security — running ThemisDB in production
    ("⚙️ Operations & Security", [
        "Operations",
        "Ops-Overview",
        "Ops-Runbook",
        "Ops-Handbook",
        "Ops-ThemisCtl-Admin-Guide",
        "Ops-Pipeline-E2E-SOPs",
        "Deploy-Overview",
        "Docker-Overview",
        "Docker-Hub-README",
        "Helm-Overview",
        "Packaging-Overview",
        "Operator-Overview",
        "Security-Policy",
        "Security-Hardening-Checklist",
        "Security-Hardening-Guide",
        "Security-Key-Management",
        "Security-Access-Control",
        "Security-Zero-Trust",
        "Security-API-Auth",
        "Security-HSM-Setup",
        "Security-PKCS11",
        "Security-DSGVO-SOC2-Checklist",
        "Ops-Access-Model-Runbooks",
        "Ops-Access-Model-Dashboard",
        "Ops-Maturity-Automation",
    ]),
    # 5b. Ops Runbooks — sub-section for operational runbooks (dynamic)
    ("📟 Ops Runbooks", []),     # populated dynamically from Ops-LLM-*, Ops-DR-*, Ops-IR-*, Ops-Logging-*, Ops-Access-*
    # 6. Architecture — system design (after users understand what it does)
    ("🏗️ Architecture", []),    # populated dynamically from Architecture-* pages (curated subset)
    # 7. ADRs — architecture decision records (linked from Architecture)
    ("📐 ADRs", []),             # populated dynamically from ADR-* pages
    # 8. Contributing / Developer — internal orientation
    ("🔧 Contributing", [
        "Contributing",
        "Root-Contributing",
        "Code-of-Conduct",
        "Support",
        "Maintainers",
        "CTest-Guide",
        "Scripts-Build-Quick-Ref",
        "Developer-INDEX",
        "Developer-BUILD-TEST-CI-AND-OPERATIONS",
        "Module-Index",
        "Branching-Strategy",
        "Release-Strategy",
    ]),
    # 9. Governance — policy, compliance, release gates
    ("📋 Governance", []),       # populated dynamically from Governance-* pages
    # 10. Audit — baseline maturity & compliance reports
    ("🔍 Audit", [
        "Audit-Maturity-Report",
        "Audit-Production-Readiness",
        "Audit-BSI-C5",
    ]),
    # 11. Plugins — extension points
    ("🧩 Plugins", []),          # populated dynamically from Plugin-* pages
    # 12. Adapters — integration adapters
    ("🔌 Adapters", []),         # populated dynamically from Adapter-* pages
    # 13. Examples — runnable application examples
    ("💡 Examples", []),         # populated dynamically from Example-* pages
    # 14. Client SDKs
    ("📦 Client SDKs", []),      # populated dynamically from Client-* + SDK-* pages
    # 15. Training / Schulung — training materials
    ("🎓 Training", []),         # populated dynamically from Training-* pages
    # 16. Tools — developer tooling
    ("🛠️ Tools", []),            # populated dynamically from Tool-* pages
    # 17. Developer LLM Wiki — AI-context artifacts (most internal)
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
        "AQL-Overview": "AQL Overview",
        "AQL-Root-Overview": "AQL Root Overview",
        "AQL-Feature-Roadmap": "AQL Feature Roadmap",
        "AQL-Geospatial-Guide": "AQL Geospatial Guide",
        "AQL-LLM-Migration-Guide": "AQL LLM Migration Guide",
        "AQL-API": "AQL API",
        "AQL-Examples-Root": "AQL Examples (root)",
        "AQL-Grammar": "AQL Grammar (EBNF)",
        "API-Reference": "API Reference",
        "Integration-Guide": "Integration Guide",
        "Migration-Guide": "Migration Guide",
        "Operations": "Operations",
        "Ops-Overview": "Operations Overview",
        "Ops-Runbook": "Operations Runbook",
        "Ops-Handbook": "Operations Handbook",
        "Ops-ThemisCtl-Admin-Guide": "ThemisCtl Admin Guide",
        "Ops-Pipeline-E2E-SOPs": "Pipeline E2E SOPs",
        "Ops-Access-Model-Runbooks": "Access Model Runbooks",
        "Ops-Access-Model-Dashboard": "Access Model Dashboard",
        "Ops-Maturity-Automation": "Maturity Automation Runbook",
        "Ops-Pentest-Automation": "Pentest Automation Schedule",
        "Security-Policy": "Security Policy",
        "Security-Hardening-Checklist": "Production Hardening Checklist",
        "Security-Key-Management": "Encryption Key Management",
        "Security-Access-Control": "Access Control Framework",
        "Security-Hardening-Guide": "Security Hardening Guide",
        "Security-Zero-Trust": "Zero Trust Policy",
        "Security-API-Auth": "API Authentication & Authorization",
        "Security-HSM-Setup": "HSM Production Setup",
        "Security-PKCS11": "PKCS11 Integration",
        "Training-Overview": "Training Overview",
        "Training-Docs-Overview": "Training Documents",
        "Training-Pres-Overview": "Training Presentations",
        "Deploy-Overview": "Deploy Overview",
        "Docker-Overview": "Docker Overview",
        "Docker-Hub-README": "Docker Hub README",
        "Helm-Overview": "Helm Overview",
        "Packaging-Overview": "Packaging Overview",
        "Operator-Overview": "Operator Overview",
        "Security-DSGVO-SOC2-Checklist": "DSGVO / SOC2 Checklist",
        "Client-Overview": "Client SDK Overview",
        "SDK-Overview": "SDK Overview",
        "Api-Module-README": "API Module README",
        "OpenAPI-Overview": "OpenAPI Overview",
        "Demo-Overview": "Demo Overview",
        "Demo-Quickstart": "Demo Quickstart",
        "Demo-Queries": "Demo Queries",
        "Audit-Maturity-Report": "Maturity Report",
        "Audit-Production-Readiness": "Production Readiness",
        "Audit-BSI-C5": "BSI C5 Audit",
        "Tools-Status": "Tools Status",
        "Scripts-Build-Quick-Ref": "Build Quick Reference",
        "Root-Contributing": "Contributing (root)",
        "Root-Changelog": "Changelog",
        "Root-Architecture": "Architecture Overview",
        "Root-Roadmap": "Roadmap",
        "Root-Future-Enhancements": "Future Enhancements",
        "Root-Security": "Security Policy (root)",
        "Root-Governance": "Governance",
        "Branching-Strategy": "Branching Strategy",
        "Release-Strategy": "Release Strategy",
        "Versioning": "Versioning",
        "Code-of-Conduct": "Code of Conduct",
        "Support": "Support",
        "Maintainers": "Maintainers",
        "CTest-Guide": "CTest Guide",
        "Proto-Overview": "Protobuf Overview",
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
            "Client-", "SDK-", "Example-", "Adapter-",
            "Tool-", "Deploy-", "Docker-", "Helm-", "Packaging-",
            "Operator-", "Audit-", "Training-Doc-", "Training-Pres-", "Training-",
            "DevGuide-", "Demo-", "Api-Module-", "Root-",
            "Ops-LLM-", "Ops-DR-", "Ops-IR-", "Ops-Access-", "Ops-Logging-",
            "Ops-", "Security-", "AQL-",
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


# Shared dynamic-prefix map: section title → wiki page name prefix.
# Sections not in this map are either curated (Architecture) or use explicit lists.
_SECTION_DYNAMIC_PREFIX: dict[str, str] = {
    "📖 Tutorials":          "Tutorial-",
    "📐 ADRs":               "ADR-",
    "📋 Governance":         "Governance-",
    "🧩 Plugins":            "Plugin-",
    "🔌 Adapters":           "Adapter-",
    "💡 Examples":           "Example-",
    "📦 Client SDKs":        "Client-",
    "🛠️ Tools":              "Tool-",
    "🎓 Training":           "Training-",
    "🤖 Developer LLM Wiki": "Developer-",
}


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
            prefix = _SECTION_DYNAMIC_PREFIX.get(section_title)
            if prefix:
                pages = sorted(p for p in all_wiki_names if p.startswith(prefix))
            elif section_title == "📟 Ops Runbooks":
                _OPS_RUNBOOK_PREFIXES = ("Ops-LLM-", "Ops-DR-", "Ops-IR-", "Ops-Access-", "Ops-Logging-")
                pages = sorted(p for p in all_wiki_names if any(p.startswith(pfx) for pfx in _OPS_RUNBOOK_PREFIXES))
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
            prefix = _SECTION_DYNAMIC_PREFIX.get(section_title)
            if prefix:
                section_pages = sorted(
                    p for p in all_wiki_names if p.startswith(prefix)
                )
            elif section_title == "📟 Ops Runbooks":
                # Collect all ops sub-runbook pages (excludes top-level Ops-* already in Operations)
                _OPS_RUNBOOK_PREFIXES = ("Ops-LLM-", "Ops-DR-", "Ops-IR-", "Ops-Access-", "Ops-Logging-")
                section_pages = sorted(
                    p for p in all_wiki_names
                    if any(p.startswith(pfx) for pfx in _OPS_RUNBOOK_PREFIXES)
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
    parser.add_argument(
        "--enable-breadcrumbs",
        action="store_true",
        default=True,
        help="Add breadcrumb navigation to each wiki page (default: enabled).",
    )
    parser.add_argument(
        "--disable-breadcrumbs",
        action="store_false",
        dest="enable_breadcrumbs",
        help="Disable breadcrumb navigation.",
    )
    parser.add_argument(
        "--sort-by",
        choices=["currency", "modification-date", "none"],
        default="currency",
        help="Sort documents by: currency (freshness+mtime), modification-date (mtime only), or none (default: currency).",
    )
    parser.add_argument(
        "--enable-term-linking",
        action="store_true",
        help="Enable automatic glossary term-linking in wiki pages (requires docs/glossary_index.json).",
    )
    parser.add_argument(
        "--term-link-priority",
        choices=["high", "medium", "low"],
        default="high",
        help="Minimum glossary term priority to auto-link (default: high).",
    )
    args = parser.parse_args(argv)

    repo_root = Path(args.repo_root).resolve()
    output_dir = Path(args.output).resolve()

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    # Load glossary index if term-linking is enabled
    glossary_terms = None
    if args.enable_term_linking:
        glossary_path = repo_root / GLOSSARY_INDEX_PATH
        glossary_terms = _load_glossary_index(glossary_path)
        if glossary_terms:
            print(f"📚 Loaded {len(glossary_terms)} glossary terms for term-linking", file=sys.stderr)
        else:
            print(f"⚠️ Term-linking enabled but glossary index not found at {glossary_path}", file=sys.stderr)

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

    # Sort entries if requested
    if args.sort_by != "none":
        entries_scored = _sort_entries_by_currency(entries, repo_root)
        entries = [(p, w) for p, w, _ in entries_scored]
        if not args.dry_run and args.sort_by == "currency":
            print(f"📊 Documents sorted by {args.sort_by}", file=sys.stderr)

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

        # Special handling for non-Markdown sources (e.g. EBNF grammar files)
        if source_path.suffix.lower() == ".ebnf":
            transformed = (
                f"# {_sidebar_label(wiki_name)}\n\n"
                f"> Source: `{source_rel}`\n\n"
                "```ebnf\n"
                + text
                + "\n```\n"
            )
        else:
            transformed = _transform(
                text, source_path, repo_root, wiki_name, all_wiki_names,
                enable_breadcrumbs=args.enable_breadcrumbs
            )
        
        # Apply term-linking if enabled
        if glossary_terms and args.enable_term_linking:
            transformed = _inject_term_links(
                transformed,
                glossary_terms,
                min_priority=args.term_link_priority,
            )
        
        # Inject breadcrumb navigation if enabled
        if args.enable_breadcrumbs and wiki_name not in {"Home", "Wiki-Index", "Module-Index"}:
            breadcrumb = _format_breadcrumb_nav(wiki_name, all_wiki_names)
            transformed = breadcrumb + "\n" + transformed
        
        header = _page_header(source_rel, wiki_name)
        footer = _page_footer(wiki_name)
        final_content = header + transformed + footer

        dest = output_dir / f"{wiki_name}.md"
        if args.dry_run:
            print(f"DRY-RUN: {source_rel} → {dest.name}")
        else:
            dest.write_text(final_content, encoding="utf-8")
            written.append(dest.name)

    # Generate Home.md (aggregated wiki start page — overrides docs/en/Home.md)
    home_content = (
        _page_header("generated:Home", "Home")
        + _build_home_page(repo_root, all_wiki_names)
        + _page_footer("Home")
    )
    if args.dry_run:
        print("DRY-RUN: <generated> → Home.md")
    else:
        (output_dir / "Home.md").write_text(home_content, encoding="utf-8")
        written.append("Home.md")
    all_wiki_names.add("Home")

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
