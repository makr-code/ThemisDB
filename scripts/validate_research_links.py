"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_research_links.py                         ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-04-15 18:48:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
validate_research_links.py

Scans all source files and documentation for patterns that suggest a
scientific paper, best practice, or external reference was used as a basis
for an implementation, then checks whether a corresponding research
documentation file exists in research/.

By default the script runs in warn-only mode (exit 0) so that it never
blocks PRs for references that pre-date the research documentation system.
Pass --strict to exit 1 when undocumented references are found (useful once
Phase 2 backfilling is complete).

Exit codes:
  0 — success (or warn-only mode regardless of findings)
  1 — undocumented references found AND --strict flag was passed
"""

import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent

# Directories to scan for research references in source/docs
SCAN_DIRS = [
    REPO_ROOT / "src",
    REPO_ROOT / "include",
    REPO_ROOT / "docs",
]

# File extensions to scan
SCAN_EXTENSIONS = {".cpp", ".hpp", ".h", ".cc", ".cxx", ".py", ".md"}

# Research documentation root
RESEARCH_DIR = REPO_ROOT / "research"
PAPERS_DIR = RESEARCH_DIR / "papers"
BEST_PRACTICES_DIR = RESEARCH_DIR / "best_practices"
ARCH_DECISIONS_DIR = RESEARCH_DIR / "architecture_decisions"

# Patterns that indicate an undocumented research reference.
# These are intentionally broad to catch informal references.
REFERENCE_PATTERNS = [
    re.compile(r"\bbased\s+on\s+(paper|the\s+paper|algorithm)\b", re.IGNORECASE),
    re.compile(r"\binspired\s+by\b", re.IGNORECASE),
    re.compile(r"\bas\s+described\s+in\b", re.IGNORECASE),
    re.compile(r"\bsee\s+paper\b", re.IGNORECASE),
    re.compile(r"\bfollowing\s+(the\s+)?(paper|approach\s+of|algorithm\s+(in|from))\b", re.IGNORECASE),
    re.compile(r"\bimplementation\s+of\s+(the\s+)?\w+\s+algorithm\b", re.IGNORECASE),
    re.compile(r"\baccording\s+to\s+(the\s+)?(paper|study|research)\b", re.IGNORECASE),
    re.compile(r"\bcf\.\s+\[", re.IGNORECASE),
    re.compile(r"\barXiv:[0-9]{4}\.[0-9]{4,5}", re.IGNORECASE),
    re.compile(r"\bdoi:\s*10\.", re.IGNORECASE),
]

# Patterns that, when present in the same file, indicate the reference IS
# already documented (i.e., suppress the warning).
DOCUMENTED_PATTERNS = [
    re.compile(r"research/(papers|best_practices|architecture_decisions)/\S+\.md"),
]

# Files / directories to exclude from scanning
EXCLUDE_PATHS = {
    REPO_ROOT / "research",  # avoid self-references
    REPO_ROOT / ".git",
    REPO_ROOT / "vcpkg",
    REPO_ROOT / "llama.cpp",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def should_exclude(path: Path) -> bool:
    for exc in EXCLUDE_PATHS:
        try:
            path.relative_to(exc)
            return True
        except ValueError:
            pass
    return False


def scan_file(path: Path) -> list[str]:
    """Return a list of warning messages for undocumented references in *path*."""
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []

    # If the file already contains a link to a research doc, skip it.
    for dp in DOCUMENTED_PATTERNS:
        if dp.search(text):
            return []

    warnings = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        for pattern in REFERENCE_PATTERNS:
            if pattern.search(line):
                rel = path.relative_to(REPO_ROOT)
                warnings.append(f"  {rel}:{lineno}: {line.strip()[:120]}")
                break  # one warning per line is enough

    return warnings


def collect_documented_sources() -> dict[str, int]:
    """Return a dict mapping research subdirectory name to file count."""
    counts: dict[str, int] = {"papers": 0, "best_practices": 0, "architecture_decisions": 0}
    for subdir, key in [
        (PAPERS_DIR, "papers"),
        (BEST_PRACTICES_DIR, "best_practices"),
        (ARCH_DECISIONS_DIR, "architecture_decisions"),
    ]:
        if subdir.exists():
            counts[key] = sum(
                1 for f in subdir.glob("*.md")
                if not f.name.startswith("_") and f.name not in {"README.md", "TEMPLATES.md", "decision_log.md"}
            )
    return counts


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    strict = "--strict" in sys.argv
    all_warnings: list[str] = []

    for scan_dir in SCAN_DIRS:
        if not scan_dir.exists():
            continue
        for path in scan_dir.rglob("*"):
            if path.is_file() and path.suffix in SCAN_EXTENSIONS and not should_exclude(path):
                all_warnings.extend(scan_file(path))

    counts = collect_documented_sources()
    print("Research Documentation Validation")
    print("=" * 60)
    print(f"Documented papers:              {counts['papers']}")
    print(f"Documented best practices:      {counts['best_practices']}")
    print(f"Documented arch. decisions:     {counts['architecture_decisions']}")
    print()

    if all_warnings:
        print(f"⚠️  Found {len(all_warnings)} potentially undocumented research reference(s):")
        for w in all_warnings:
            print(w)
        print()
        print("For each reference above, either:")
        print("  1. Create a research file in research/<type>/ and link it, OR")
        print("  2. Add a link to an existing research file in the affected source file.")
        print()
        print("See research/RESEARCH_GUIDE.md for the full workflow.")
        if strict:
            return 1
        print("ℹ️  Running in warn-only mode (pass --strict to fail CI). "
              "Backfilling existing references is tracked in Phase 2.")
        return 0

    print("✅ No undocumented research references detected.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
