#!/usr/bin/env python3
"""Generate Mermaid class/function maps from Doxygen XML output."""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable
import xml.etree.ElementTree as ET


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_XML_DIR = REPO_ROOT / "build" / "doxygen" / "xml"
DEFAULT_OUTPUT_MD = REPO_ROOT / "build" / "doxygen" / "themisdb_doxygen_mermaid_overview.md"
DEFAULT_CLASS_MMD = REPO_ROOT / "build" / "doxygen" / "themisdb_class_map.mmd"
DEFAULT_FUNC_MMD = REPO_ROOT / "build" / "doxygen" / "themisdb_function_map.mmd"

VISIBILITY = {
    "public": "+",
    "protected": "#",
    "private": "-",
    "package": "~",
}

BUILTIN_TYPE_EXCLUSIONS = {
    "std",
    "size_t",
    "uint32_t",
    "uint64_t",
}


def _safe_text(value: str) -> str:
    return value.replace('"', "'").replace("\n", " ").strip()


def _node_id(raw: str, prefix: str) -> str:
    sanitized = re.sub(r"[^A-Za-z0-9_]", "_", raw)
    if not sanitized:
        sanitized = "node"
    return f"{prefix}_{sanitized}"


def _mermaid_member(line: str) -> str:
    # Mermaid class members cannot contain some punctuation safely.
    compact = re.sub(r"\s+", " ", line).strip()
    compact = compact.replace('"', "'")
    return compact


@dataclass
class MethodInfo:
    name: str
    args: str
    return_type: str
    visibility: str


@dataclass
class ClassInfo:
    name: str
    kind: str
    methods: list[MethodInfo] = field(default_factory=list)
    variable_types: set[str] = field(default_factory=set)
    bases: set[str] = field(default_factory=set)


@dataclass
class NamespaceInfo:
    name: str
    functions: list[MethodInfo] = field(default_factory=list)


class DoxygenXmlScanner:
    def __init__(self, xml_dir: Path):
        self.xml_dir = xml_dir

    def _iter_compounds(self) -> Iterable[tuple[str, str]]:
        index_file = self.xml_dir / "index.xml"
        if not index_file.exists():
            raise FileNotFoundError(f"Doxygen index.xml not found: {index_file}")

        root = ET.parse(index_file).getroot()
        for compound in root.findall("compound"):
            kind = (compound.attrib.get("kind") or "").strip()
            refid = (compound.attrib.get("refid") or "").strip()
            if kind and refid:
                yield kind, refid

    def scan(self) -> tuple[dict[str, ClassInfo], dict[str, NamespaceInfo]]:
        classes: dict[str, ClassInfo] = {}
        namespaces: dict[str, NamespaceInfo] = {}

        for kind, refid in self._iter_compounds():
            file_path = self.xml_dir / f"{refid}.xml"
            if not file_path.exists():
                continue

            try:
                root = ET.parse(file_path).getroot()
            except ET.ParseError:
                continue

            compounddef = root.find("compounddef")
            if compounddef is None:
                continue

            compound_name = (compounddef.findtext("compoundname") or "").strip()
            if not compound_name:
                continue

            if kind in {"class", "struct"}:
                info = ClassInfo(name=compound_name, kind=kind)
                for base in compounddef.findall("basecompoundref"):
                    base_name = (base.text or "").strip()
                    if base_name:
                        info.bases.add(base_name)

                for section in compounddef.findall("sectiondef"):
                    for member in section.findall("memberdef"):
                        member_kind = (member.attrib.get("kind") or "").strip()
                        if member_kind == "function":
                            info.methods.append(
                                MethodInfo(
                                    name=(member.findtext("name") or "").strip(),
                                    args=(member.findtext("argsstring") or "").strip(),
                                    return_type=(member.findtext("type") or "").strip(),
                                    visibility=(member.attrib.get("prot") or "public").strip(),
                                )
                            )
                        elif member_kind == "variable":
                            vtype = (member.findtext("type") or "").strip()
                            if vtype:
                                info.variable_types.add(vtype)

                classes[compound_name] = info

            elif kind == "namespace":
                ns = NamespaceInfo(name=compound_name)
                for section in compounddef.findall("sectiondef"):
                    for member in section.findall("memberdef"):
                        if (member.attrib.get("kind") or "").strip() != "function":
                            continue
                        ns.functions.append(
                            MethodInfo(
                                name=(member.findtext("name") or "").strip(),
                                args=(member.findtext("argsstring") or "").strip(),
                                return_type=(member.findtext("type") or "").strip(),
                                visibility="public",
                            )
                        )
                if ns.functions:
                    namespaces[compound_name] = ns

        return classes, namespaces


def _build_class_aliases(classes: dict[str, ClassInfo]) -> dict[str, str]:
    aliases: dict[str, str] = {}
    for idx, name in enumerate(sorted(classes.keys()), start=1):
        aliases[name] = f"C{idx}"
    return aliases


def _possible_type_refs(type_text: str) -> set[str]:
    cleaned = type_text.replace("*", " ").replace("&", " ").replace("const", " ")
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_:]*", cleaned)
    return {tok for tok in tokens if tok and tok not in BUILTIN_TYPE_EXCLUSIONS}


def build_class_diagram(classes: dict[str, ClassInfo], max_methods_per_class: int) -> str:
    aliases = _build_class_aliases(classes)
    class_names = set(classes.keys())
    simple_to_full: dict[str, set[str]] = defaultdict(set)

    for full in class_names:
        simple_to_full[full.split("::")[-1]].add(full)

    lines: list[str] = ["classDiagram"]

    for class_name in sorted(class_names):
        alias = aliases[class_name]
        lines.append(f'    class "{_safe_text(class_name)}" as {alias}')
        methods = classes[class_name].methods
        if methods:
            lines.append(f"    {alias} : <<{classes[class_name].kind}>>")
            for method in methods[:max_methods_per_class]:
                vis = VISIBILITY.get(method.visibility, "~")
                signature = f"{vis}{method.name}{method.args} {method.return_type}".strip()
                lines.append(f"    {alias} : {_mermaid_member(signature)}")

    for class_name, info in classes.items():
        src_alias = aliases[class_name]
        for base in sorted(info.bases):
            if base in aliases:
                lines.append(f"    {src_alias} --|> {aliases[base]}")

        seen_targets: set[str] = set()
        for vtype in info.variable_types:
            for token in _possible_type_refs(vtype):
                candidate = None
                if token in aliases:
                    candidate = token
                elif token in simple_to_full and len(simple_to_full[token]) == 1:
                    candidate = next(iter(simple_to_full[token]))
                if candidate and candidate != class_name and candidate not in seen_targets:
                    seen_targets.add(candidate)
                    lines.append(f"    {src_alias} --> {aliases[candidate]} : uses")

    return "\n".join(lines) + "\n"


def _method_node(ns_or_class: str, method: MethodInfo) -> tuple[str, str]:
    label = f"{method.name}{method.args}".strip()
    node = _node_id(f"{ns_or_class}_{label}", "M")
    return node, _safe_text(label)


def build_function_map(classes: dict[str, ClassInfo], namespaces: dict[str, NamespaceInfo], max_methods_total: int) -> str:
    lines: list[str] = ["flowchart TB"]
    emitted = 0

    for class_name in sorted(classes.keys()):
        cnode = _node_id(class_name, "CL")
        lines.append(f'    {cnode}["{_safe_text(class_name)}"]')
        for method in classes[class_name].methods:
            if emitted >= max_methods_total:
                break
            mnode, mlabel = _method_node(class_name, method)
            lines.append(f'    {mnode}("{mlabel}")')
            lines.append(f"    {cnode} --> {mnode}")
            emitted += 1
        if emitted >= max_methods_total:
            break

    if emitted < max_methods_total:
        for ns_name in sorted(namespaces.keys()):
            nnode = _node_id(ns_name, "NS")
            lines.append(f'    {nnode}["{_safe_text(ns_name)}"]')
            for fn in namespaces[ns_name].functions:
                if emitted >= max_methods_total:
                    break
                mnode, mlabel = _method_node(ns_name, fn)
                lines.append(f'    {mnode}("{mlabel}")')
                lines.append(f"    {nnode} --> {mnode}")
                emitted += 1
            if emitted >= max_methods_total:
                break

    if emitted >= max_methods_total:
        lines.append(
            f"    LIMIT_REACHED[\"Method mapping limit reached at {max_methods_total}; "
            "increase --max-methods-total\"]"
        )

    return "\n".join(lines) + "\n"


def write_outputs(
    output_md: Path,
    class_mmd: Path,
    function_mmd: Path,
    class_diagram: str,
    function_map: str,
    classes: dict[str, ClassInfo],
    namespaces: dict[str, NamespaceInfo],
) -> None:
    output_md.parent.mkdir(parents=True, exist_ok=True)

    class_count = len(classes)
    method_count = sum(len(c.methods) for c in classes.values())
    namespace_count = len(namespaces)
    free_function_count = sum(len(n.functions) for n in namespaces.values())

    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    md = [
        "# ThemisDB Doxygen Mermaid Mapping\n\n",
        f"Generated: {timestamp}\n\n",
        "## Scan Summary\n\n",
        f"- Classes/Structs: **{class_count}**\n",
        f"- Member functions (class/struct): **{method_count}**\n",
        f"- Namespaces with free functions: **{namespace_count}**\n",
        f"- Free namespace functions: **{free_function_count}**\n\n",
        "## Class Mapping (Doxygen XML -> Mermaid classDiagram)\n\n",
        "```mermaid\n",
        class_diagram,
        "```\n\n",
        "## Function Mapping Overview (Classes + Namespaces)\n\n",
        "```mermaid\n",
        function_map,
        "```\n",
    ]

    output_md.write_text("".join(md), encoding="utf-8")
    class_mmd.write_text(class_diagram, encoding="utf-8")
    function_mmd.write_text(function_map, encoding="utf-8")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate Mermaid class/function maps from Doxygen XML.")
    parser.add_argument("--xml-dir", type=Path, default=DEFAULT_XML_DIR, help="Path to Doxygen XML directory")
    parser.add_argument("--output-md", type=Path, default=DEFAULT_OUTPUT_MD, help="Output markdown report")
    parser.add_argument("--output-class", type=Path, default=DEFAULT_CLASS_MMD, help="Output Mermaid class diagram file")
    parser.add_argument("--output-function", type=Path, default=DEFAULT_FUNC_MMD, help="Output Mermaid function map file")
    parser.add_argument("--max-methods-per-class", type=int, default=120, help="Max methods rendered per class")
    parser.add_argument("--max-methods-total", type=int, default=8000, help="Max total methods rendered in function map")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])

    scanner = DoxygenXmlScanner(args.xml_dir)
    classes, namespaces = scanner.scan()

    if not classes and not namespaces:
        print(
            "No parseable classes/namespaces found. "
            f"Verify Doxygen XML exists at '{args.xml_dir}' and run 'doxygen Doxyfile' first.",
            file=sys.stderr,
        )
        return 2

    class_diagram = build_class_diagram(classes, max_methods_per_class=max(1, args.max_methods_per_class))
    function_map = build_function_map(classes, namespaces, max_methods_total=max(1, args.max_methods_total))

    write_outputs(
        output_md=args.output_md,
        class_mmd=args.output_class,
        function_mmd=args.output_function,
        class_diagram=class_diagram,
        function_map=function_map,
        classes=classes,
        namespaces=namespaces,
    )

    print(f"Generated: {args.output_md}")
    print(f"Generated: {args.output_class}")
    print(f"Generated: {args.output_function}")
    print(f"Classes: {len(classes)} | Class methods: {sum(len(c.methods) for c in classes.values())}")
    print(f"Namespaces: {len(namespaces)} | Free functions: {sum(len(n.functions) for n in namespaces.values())}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
