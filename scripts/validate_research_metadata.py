"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_research_metadata.py                      ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-04-15 18:48:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
validate_research_metadata.py

Validates that every research documentation file in research/
contains the required frontmatter fields.

Exit codes:
  0 — all files pass validation
  1 — one or more files are missing required fields
"""

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
RESEARCH_DIR = REPO_ROOT / "research"

# Subdirectories and their required fields (checked in the Markdown body as
# "- <field>:" or "**Field:**" patterns).
#
# Note: Paper templates deliberately use German field names (e.g. "Jahr" for
# year, "Konferenz/Journal") to match the project's bilingual documentation
# convention. Other fields remain in English for broader tool compatibility.
REQUIRED_FIELDS: dict[str, list[str]] = {
    "papers": ["Author(en)", "Jahr", "Tags", "ThemisDB-Versionen", "Status"],
    "best_practices": ["Source", "Tags", "ThemisDB-Versionen", "Status"],
    "architecture_decisions": ["Status", "Date", "Modules Affected"],
}

# Files to skip (templates, indexes, logs)
SKIP_NAMES = {"README.md", "TEMPLATES.md", "decision_log.md"}
SKIP_PREFIXES = {"_template"}


def is_skipped(path: Path) -> bool:
    if path.name in SKIP_NAMES:
        return True
    for prefix in SKIP_PREFIXES:
        if path.name.startswith(prefix):
            return True
    return False


def validate_file(path: Path, required: list[str]) -> list[str]:
    """Return list of missing required fields."""
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return [f"Cannot read file: {exc}"]

    missing = []
    aliases: dict[str, list[str]] = {
        "Author(en)": ["Author(en)", "Author"],
    }
    for field in required:
        field_variants = aliases.get(field, [field])
        # Match "- Field:" or "**Field:**" or "Field:" at start of line
        patterns = []
        for variant in field_variants:
            patterns.extend([
                f"- {variant}:",
                f"**{variant}:**",
                f"- **{variant}:**",
                f"{variant}:",
            ])
        found = any(p.lower() in text.lower() for p in patterns)
        if not found:
            missing.append(field)
    return missing


def main() -> int:
    errors: list[str] = []

    for subdir_name, required_fields in REQUIRED_FIELDS.items():
        subdir = RESEARCH_DIR / subdir_name
        if not subdir.exists():
            continue
        for md_file in sorted(subdir.glob("*.md")):
            if is_skipped(md_file):
                continue
            missing = validate_file(md_file, required_fields)
            if missing:
                rel = md_file.relative_to(REPO_ROOT)
                errors.append(f"  {rel}: missing fields: {', '.join(missing)}")

    print("Research Metadata Validation")
    print("=" * 60)

    if errors:
        print(f"❌ {len(errors)} file(s) with missing required fields:")
        for e in errors:
            print(e)
        print()
        print("See research/RESEARCH_GUIDE.md for required field definitions.")
        return 1

    print("✅ All research files have required metadata fields.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
