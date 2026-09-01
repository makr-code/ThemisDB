#!/usr/bin/env python3
"""extract_doxygen_wiki.py — Extract Doxygen XML output into per-module API
reference Markdown files for the developer LLM wiki.

Usage:
    python scripts/extract_doxygen_wiki.py \
        --xml-dir build/doxygen/xml \
        --output-dir ai_context/developer_llm_wiki \
        [--dry-run]

Output:
    ai_context/developer_llm_wiki/API_REFERENCE_<module>.md
    — one file per namespace/module, containing:
      • Class / struct list with @brief descriptions
      • Public methods table (name, signature stub, @brief, @return)

Community guardrail:
    Files whose namespace path contains "private" or "plugins/private" are
    skipped silently to prevent private implementation details leaking into
    the developer wiki (mirrors the Community Fail-Closed gate logic).

Prerequisites:
    • Doxygen must have been run with GENERATE_XML = YES.
    • xml/index.xml must exist in the --xml-dir.
    • Python standard library only — no external dependencies.
"""
from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# Private-path guard — same logic as in build_wiki.py / community fail-closed
# ---------------------------------------------------------------------------
_PRIVATE_NAME_RE = re.compile(r"private", re.IGNORECASE)

# Doxygen compound kinds we care about
_CLASS_KINDS = {"class", "struct"}
_NS_KINDS = {"namespace"}


def _is_private(compound_name: str) -> bool:
    """Return True if the compound belongs to a private namespace / path."""
    return bool(_PRIVATE_NAME_RE.search(compound_name))


# ---------------------------------------------------------------------------
# XML helpers
# ---------------------------------------------------------------------------

def _text(element: ET.Element | None, default: str = "") -> str:
    """Extract concatenated text content from an XML element."""
    if element is None:
        return default
    parts: list[str] = []
    if element.text:
        parts.append(element.text.strip())
    for child in element:
        parts.append(_text(child))
        if child.tail:
            parts.append(child.tail.strip())
    return " ".join(p for p in parts if p)


def _brief(member_el: ET.Element) -> str:
    """Return brief description from a memberdef or compounddef element."""
    brief_el = member_el.find("briefdescription")
    if brief_el is None:
        return ""
    raw = _text(brief_el)
    # Collapse whitespace and truncate
    raw = re.sub(r"\s+", " ", raw).strip()
    return raw[:120] + ("…" if len(raw) > 120 else "")


def _param_summary(member_el: ET.Element) -> str:
    """Return a compact parameter list string '(type name, …)'."""
    params: list[str] = []
    for param in member_el.findall("param"):
        type_el = param.find("type")
        name_el = param.find("declname")
        type_str = _text(type_el) if type_el is not None else ""
        name_str = _text(name_el) if name_el is not None else ""
        params.append(f"{type_str} {name_str}".strip())
    return "(" + ", ".join(params) + ")"


def _return_type(member_el: ET.Element) -> str:
    """Return the return type string from a memberdef."""
    type_el = member_el.find("type")
    return _text(type_el) if type_el is not None else ""


# ---------------------------------------------------------------------------
# Per-module data structures
# ---------------------------------------------------------------------------

class _ClassInfo:
    def __init__(self, name: str, brief: str) -> None:
        self.name = name
        self.brief = brief
        self.methods: list[tuple[str, str, str, str]] = []
        # (method_name, params, return_type, brief)


class _ModuleInfo:
    def __init__(self, module: str) -> None:
        self.module = module
        self.classes: list[_ClassInfo] = []
        self.free_functions: list[tuple[str, str, str, str]] = []
        # (function_name, params, return_type, brief)


# ---------------------------------------------------------------------------
# XML parsing
# ---------------------------------------------------------------------------

def _parse_compound(xml_dir: Path, refid: str) -> ET.Element | None:
    """Load and return the root element of a compound XML file."""
    compound_file = xml_dir / f"{refid}.xml"
    if not compound_file.exists():
        return None
    try:
        tree = ET.parse(compound_file)
        return tree.getroot()
    except ET.ParseError as exc:
        print(f"WARNING: XML parse error in {compound_file}: {exc}", file=sys.stderr)
        return None


def _extract_members(
    compounddef: ET.Element, kind_filter: str | None = None
) -> list[ET.Element]:
    """Return all public memberdef elements, optionally filtered by kind."""
    members: list[ET.Element] = []
    for section in compounddef.findall("sectiondef"):
        for member in section.findall("memberdef"):
            if member.get("prot") not in ("public", None):
                continue
            if kind_filter and member.get("kind") != kind_filter:
                continue
            members.append(member)
    return members


def _process_index(xml_dir: Path) -> dict[str, _ModuleInfo]:
    """Parse index.xml and extract per-module API information.

    Returns a dict keyed by module name.
    """
    index_file = xml_dir / "index.xml"
    if not index_file.exists():
        print(f"ERROR: index.xml not found in {xml_dir}", file=sys.stderr)
        return {}

    try:
        tree = ET.parse(index_file)
    except ET.ParseError as exc:
        print(f"ERROR: Cannot parse index.xml: {exc}", file=sys.stderr)
        return {}

    root = tree.getroot()
    modules: dict[str, _ModuleInfo] = {}

    for compound in root.findall("compound"):
        kind = compound.get("kind", "")
        if kind not in _CLASS_KINDS and kind not in _NS_KINDS:
            continue

        name_el = compound.find("name")
        compound_name = _text(name_el) if name_el is not None else ""
        if not compound_name or _is_private(compound_name):
            continue

        refid = compound.get("refid", "")
        if not refid:
            continue

        # Derive module name from first namespace component
        # e.g.  "themis::cache::LRUCache"  → "cache"
        #        "themis_cache"              → "cache"
        #        "cache"                     → "cache"
        parts = re.split(r"::|_", compound_name)
        # Strip leading "themis" prefix if present
        if parts and parts[0].lower() in ("themis", "themisdb"):
            parts = parts[1:]
        module = parts[0].lower() if parts else "global"

        if module not in modules:
            modules[module] = _ModuleInfo(module)
        mod_info = modules[module]

        # Load the compound XML for detailed info
        comp_root = _parse_compound(xml_dir, refid)
        if comp_root is None:
            continue

        compounddef = comp_root.find("compounddef")
        if compounddef is None:
            continue

        if kind in _CLASS_KINDS:
            cls = _ClassInfo(compound_name, _brief(compounddef))
            for member in _extract_members(compounddef, "function"):
                method_name_el = member.find("name")
                method_name = _text(method_name_el) if method_name_el is not None else ""
                cls.methods.append((
                    method_name,
                    _param_summary(member),
                    _return_type(member),
                    _brief(member),
                ))
            mod_info.classes.append(cls)
        elif kind in _NS_KINDS:
            for member in _extract_members(compounddef, "function"):
                func_name_el = member.find("name")
                func_name = _text(func_name_el) if func_name_el is not None else ""
                mod_info.free_functions.append((
                    func_name,
                    _param_summary(member),
                    _return_type(member),
                    _brief(member),
                ))

    return modules


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

def _render_module_markdown(mod_info: _ModuleInfo) -> str:
    """Render a _ModuleInfo to an API reference Markdown string."""
    lines: list[str] = [
        f"# API Reference — `{mod_info.module}`\n\n",
        "_Auto-generated from Doxygen XML. Do not edit manually._\n\n",
    ]

    if mod_info.classes:
        lines.append("## Classes and Structs\n\n")
        for cls in sorted(mod_info.classes, key=lambda c: c.name):
            brief_suffix = f" — {cls.brief}" if cls.brief else ""
            lines.append(f"### `{cls.name}`{brief_suffix}\n\n")
            if cls.methods:
                lines.append("| Method | Parameters | Return | Description |\n")
                lines.append("|--------|------------|--------|-------------|\n")
                for mname, params, ret, brief in sorted(cls.methods, key=lambda m: m[0]):
                    # Escape pipe characters in table cells
                    params_cell = params.replace("|", "\\|")
                    ret_cell = ret.replace("|", "\\|")
                    brief_cell = brief.replace("|", "\\|")
                    lines.append(f"| `{mname}` | `{params_cell}` | `{ret_cell}` | {brief_cell} |\n")
                lines.append("\n")

    if mod_info.free_functions:
        lines.append("## Free Functions\n\n")
        lines.append("| Function | Parameters | Return | Description |\n")
        lines.append("|----------|------------|--------|-------------|\n")
        for fname, params, ret, brief in sorted(mod_info.free_functions, key=lambda f: f[0]):
            params_cell = params.replace("|", "\\|")
            ret_cell = ret.replace("|", "\\|")
            brief_cell = brief.replace("|", "\\|")
            lines.append(f"| `{fname}` | `{params_cell}` | `{ret_cell}` | {brief_cell} |\n")
        lines.append("\n")

    if not mod_info.classes and not mod_info.free_functions:
        lines.append("_No public API symbols found in Doxygen XML for this module._\n")

    return "".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Extract Doxygen XML into per-module API reference wiki pages."
    )
    parser.add_argument(
        "--xml-dir",
        default="build/doxygen/xml",
        metavar="DIR",
        help="Directory containing Doxygen XML output (must contain index.xml).",
    )
    parser.add_argument(
        "--output-dir",
        default="ai_context/developer_llm_wiki",
        metavar="DIR",
        help="Output directory for API_REFERENCE_<module>.md files.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be generated without writing files.",
    )
    args = parser.parse_args(argv)

    xml_dir = Path(args.xml_dir).resolve()
    output_dir = Path(args.output_dir).resolve()

    if not xml_dir.exists():
        print(
            f"ERROR: Doxygen XML directory not found: {xml_dir}\n"
            "Run Doxygen with GENERATE_XML = YES before this script.",
            file=sys.stderr,
        )
        return 1

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    modules = _process_index(xml_dir)
    if not modules:
        print("No public API symbols found — nothing to write.", file=sys.stderr)
        return 0

    written: list[str] = []
    for module_name in sorted(modules):
        mod_info = modules[module_name]
        content = _render_module_markdown(mod_info)
        out_file = output_dir / f"API_REFERENCE_{module_name}.md"
        if args.dry_run:
            class_count = len(mod_info.classes)
            fn_count = len(mod_info.free_functions)
            print(
                f"DRY-RUN: {out_file.name} "
                f"({class_count} classes, {fn_count} free functions)"
            )
        else:
            out_file.write_text(content, encoding="utf-8")
            written.append(out_file.name)

    print(
        f"\nDoxygen wiki extraction complete: {len(written)} API_REFERENCE_*.md files written."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
