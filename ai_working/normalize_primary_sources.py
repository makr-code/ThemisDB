from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(r"c:\Projects\ThemisDB")
DOCS = ROOT / "docs"

BASE_CANDIDATES = [
    "README.md",
    "ARCHITECTURE.md",
    "ROADMAP.md",
    "FUTURE_ENHANCEMENTS.md",
    "MODULE_GAPS.md",
    "SECURITY.md",
    "AUDIT.md",
    "PERFORMANCE_EXPECTATIONS.md",
    "CHANGELOG.md",
]


def build_primary_list(module: str) -> list[str]:
    src_dir = ROOT / "src" / module
    if not src_dir.exists() or not src_dir.is_dir():
        return []

    found = []
    seen = set()
    for name in BASE_CANDIDATES:
        p = src_dir / name
        if p.exists() and p.is_file():
            rel = str(Path("src") / module / name).replace("\\", "/")
            found.append(rel)
            seen.add(rel)

    # Keep additional top-level markdown docs as secondary authoritative references.
    for p in sorted(src_dir.glob("*.md")):
        rel = str(Path("src") / module / p.name).replace("\\", "/")
        if rel not in seen:
            found.append(rel)
            seen.add(rel)

    return found


def replace_block(text: str, locale: str, module: str, primary_files: list[str]) -> str:
    date_label = "Date" if locale == "en" else "Datum"
    status_line = "**Status:** current"
    primary_label = (
        "**Primary Source (source of truth):**"
        if locale == "en"
        else "**Primary (Quelle der Wahrheit):**"
    )
    ref_label = "**Reference:**" if locale == "en" else "**Bezug / Reference:**"
    ref_lines = [
        "- Inventory baseline: `ai_working/developer_docs_inventory_report.md`"
        if locale == "en"
        else "- Inventory-Baseline: `ai_working/developer_docs_inventory_report.md`",
        "- Alignment baseline: `ai_working/docs_module_alignment_report_2026-05-31.md`"
        if locale == "en"
        else "- Alignment-Baseline: `ai_working/docs_module_alignment_report_2026-05-31.md`",
        "- Policy: newer planning docs are prioritized over older historical docs."
        if locale == "en"
        else "- Regel: neuere Planungsdokumente sind fuehrend gegenueber aelteren historischen Dokumenten.",
    ]

    primary_lines = [f"- `{p}`" for p in primary_files]

    text = re.sub(r"^\*\*(Date|Datum):\*\*.*$", f"**{date_label}:** 2026-05-31", text, flags=re.MULTILINE)
    text = re.sub(r"^\*\*Status:\*\*.*$", status_line, text, flags=re.MULTILINE)

    # Replace primary block up to reference header.
    primary_header_pat = r"^\*\*Primary[^\n]*\*\*:?\s*$"
    reference_header_pat = r"^\*\*(Reference|Bezug\s*/\s*Reference):\*\*\s*$"

    m_primary = re.search(primary_header_pat, text, flags=re.MULTILINE)
    m_ref = re.search(reference_header_pat, text, flags=re.MULTILINE)
    if not m_primary or not m_ref or m_primary.start() > m_ref.start():
        return text

    before = text[: m_primary.start()]
    after_ref_header = text[m_ref.start():]

    # Find end of reference block (first '---' section separator after ref header).
    m_sep = re.search(r"^---\s*$", after_ref_header, flags=re.MULTILINE)
    if not m_sep:
        return text

    ref_tail = after_ref_header[m_sep.start():]

    new_mid = "\n".join([primary_label] + primary_lines + ["", ref_label] + ref_lines) + "\n"
    return before + new_mid + ref_tail


def main() -> None:
    updated = 0
    scanned = 0
    for locale in ("en", "de"):
        for p in sorted((DOCS / locale).glob("*/PRIMARY_SOURCES.md")):
            scanned += 1
            text = p.read_text(encoding="utf-8")
            if "**Status:** draft" not in text:
                continue

            module = p.parent.name
            primary_files = build_primary_list(module)
            if not primary_files:
                continue

            new_text = replace_block(text, locale, module, primary_files)
            if new_text != text:
                p.write_text(new_text, encoding="utf-8")
                updated += 1

    print(f"Scanned: {scanned}")
    print(f"Updated: {updated}")


if __name__ == "__main__":
    main()
