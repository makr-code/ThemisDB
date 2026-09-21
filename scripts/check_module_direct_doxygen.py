#!/usr/bin/env python3
"""Check direct Doxygen coverage and produce gate artifacts.

Compares source symbols (preferred: sourcecode graph; fallback: direct Doxygen markers)
against module Doxygen XML and emits:
- detailed module report JSON/MD
- missing symbols/classification JSON for release gates
- module coverage summary MD
"""

from __future__ import annotations

import argparse
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

CPP_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp", ".c", ".cc", ".cpp", ".cxx"}
INTERNAL_ROOTS = [
    "src",
    "include",
    "tests",
    "benchmarks",
    "examples",
    "tools",
    "scripts",
    "cmake",
    "ai_context",
    "ai_working",
    "docs",
    ".github",
]
EXTERNAL_HINTS = [
    "vcpkg",
    "vcpkg_installed",
    "third_party",
    "external",
    "vendor",
    "deps",
    "submodules",
    "packages",
    "libs",
]
NOISE_NAME_PATTERNS = [
    re.compile(r"^BENCHMARK(?:_MAIN)?$"),
    re.compile(r"^BM_[A-Za-z0-9_]+$"),
    re.compile(r"^TEST(?:_F|_P)?$"),
    re.compile(r"^INSTANTIATE_TEST_SUITE_P$"),
    re.compile(r"^SetUp$"),
    re.compile(r"^TearDown$"),
]


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Check direct module Doxygen coverage")
    p.add_argument("--repo-root", default=".")
    p.add_argument("--module", required=True)
    p.add_argument(
        "--source-graph-json",
        default="ai_working/sourcecode_graph.json",
        help="Preferred source graph input (optional)",
    )
    p.add_argument(
        "--doxygen-artifacts-root",
        default="ai_context/developer_llm_wiki/module_doxygen_artifacts_batches",
        help="Root with batch artifacts",
    )
    p.add_argument(
        "--report-json",
        default="ai_context/developer_llm_wiki/MODULE_DIRECT_DOXYGEN_CHECK.json",
    )
    p.add_argument(
        "--summary-md",
        default="ai_context/developer_llm_wiki/MODULE_DIRECT_DOXYGEN_CHECK.md",
    )
    p.add_argument(
        "--missing-report-json",
        default="ai_context/developer_llm_wiki/MISSING_DOXYGEN_SYMBOLS.json",
    )
    p.add_argument(
        "--coverage-summary-md",
        default="ai_context/developer_llm_wiki/MODULE_DOXYGEN_COVERAGE_SUMMARY.md",
    )
    p.add_argument("--sample-limit", type=int, default=25)
    return p.parse_args()


def detect_scope(file_path: str) -> str:
    norm = file_path.replace("\\", "/")
    if norm.startswith("src/") or "/src/" in norm:
        return "src"
    if norm.startswith("tests/") or "/tests/" in norm:
        return "tests"
    if norm.startswith("benchmarks/") or "/benchmarks/" in norm:
        return "benchmarks"
    if norm.startswith("include/") or "/include/" in norm:
        return "include"
    return "other"


def classify_origin_by_path(file_path: str) -> str:
    norm = file_path.replace("\\", "/")
    lowered = norm.lower()

    for root in INTERNAL_ROOTS:
        root_l = root.lower()
        if lowered == root_l or lowered.startswith(root_l + "/") or lowered.startswith("./" + root_l + "/"):
            return "internal"
        if "/" + root_l + "/" in lowered:
            return "internal"

    for hint in EXTERNAL_HINTS:
        hint_l = hint.lower()
        if lowered == hint_l or lowered.startswith(hint_l + "/") or lowered.startswith("./" + hint_l + "/"):
            return "external"
        if "/" + hint_l + "/" in lowered:
            return "external"

    return "external"


def count_origin(items: list[dict[str, Any]]) -> dict[str, int]:
    counts = {"internal": 0, "external": 0}
    for item in items:
        origin = classify_origin_by_path(str(item.get("file", "")))
        counts[origin] = counts.get(origin, 0) + 1
    return counts


def is_noise_symbol(name: str, kind: str, scope: str) -> bool:
    for pat in NOISE_NAME_PATTERNS:
        if pat.match(name):
            return True
    if scope in {"tests", "benchmarks"} and kind == "function" and re.fullmatch(r"[A-Z0-9_]+", name):
        return True
    return False


def collect_module_files(repo_root: Path, module: str) -> list[Path]:
    roots = [
        repo_root / "include" / module,
        repo_root / "src" / module,
        repo_root / "tests" / module,
        repo_root / "benchmarks" / module,
    ]
    files: list[Path] = []
    seen: set[str] = set()
    for root in roots:
        if not root.exists():
            continue
        for p in root.rglob("*"):
            if p.is_file() and p.suffix.lower() in CPP_SUFFIXES:
                rp = str(p.resolve())
                if rp not in seen:
                    seen.add(rp)
                    files.append(p)
    return sorted(files)


CLASS_RE = re.compile(r"\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)")
FUNC_RE = re.compile(
    r"(?:^|\s)([~A-Za-z_][A-Za-z0-9_:<>~]*)\s*\([^;{}]*\)\s*(?:const\b)?\s*(?:noexcept\b)?\s*(?:=\s*(?:0|default|delete))?\s*[;{]"
)


def declaration_kind_and_name(line: str) -> tuple[str, str] | None:
    s = line.strip()
    if not s or s.startswith("//"):
        return None
    m_cls = CLASS_RE.search(s)
    if m_cls:
        return (m_cls.group(1), m_cls.group(2))
    m_fn = FUNC_RE.search(s)
    if m_fn:
        full = m_fn.group(1)
        name = full.split("::")[-1]
        if name in {"if", "for", "while", "switch", "catch", "return"}:
            return None
        return ("function", name)
    return None


def extract_direct_doxygen_decls(path: Path) -> list[dict[str, Any]]:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    out: list[dict[str, Any]] = []

    pending_doc = False
    in_block = False
    i = 0
    while i < len(lines):
        s = lines[i].strip()

        if in_block:
            if "*/" in s:
                in_block = False
                pending_doc = True
            i += 1
            continue

        if s.startswith("/**"):
            if "*/" in s and s.find("*/") > s.find("/**"):
                pending_doc = True
            else:
                in_block = True
            i += 1
            continue

        if s.startswith("///"):
            while i < len(lines) and lines[i].strip().startswith("///"):
                i += 1
            pending_doc = True
            continue

        if pending_doc:
            if not s or s.startswith("//"):
                i += 1
                continue
            decl = declaration_kind_and_name(lines[i])
            if decl is not None:
                kind, name = decl
                out.append(
                    {
                        "file": str(path),
                        "line": i + 1,
                        "kind": kind,
                        "name": name,
                        "scope": detect_scope(str(path)),
                    }
                )
            pending_doc = False

        i += 1

    return out


def parse_source_graph(module: str, source_graph_path: Path) -> list[dict[str, Any]]:
    if not source_graph_path.exists():
        return []
    try:
        obj = json.loads(source_graph_path.read_text(encoding="utf-8", errors="replace"))
    except json.JSONDecodeError:
        return []

    out: list[dict[str, Any]] = []

    def try_add(item: dict[str, Any]) -> None:
        name = str(item.get("name", "")).strip()
        kind = str(item.get("kind", item.get("type", ""))).strip().lower()
        file_path = str(item.get("file", item.get("path", ""))).strip()
        item_module = str(item.get("module", "")).strip()

        if item_module and item_module != module:
            return
        if not item_module and file_path:
            fp = file_path.replace("\\", "/")
            if f"/{module}/" not in fp:
                return

        if not name:
            return
        if kind not in {"class", "struct", "function", "method"}:
            return

        out.append(
            {
                "name": name.split("::")[-1],
                "kind": "function" if kind == "method" else kind,
                "file": file_path,
                "line": int(item.get("line", 0) or 0),
                "scope": detect_scope(file_path),
                "source": "graph",
            }
        )

    if isinstance(obj, dict):
        if isinstance(obj.get("symbols"), list):
            for item in obj["symbols"]:
                if isinstance(item, dict):
                    try_add(item)
        if isinstance(obj.get("nodes"), list):
            for item in obj["nodes"]:
                if isinstance(item, dict):
                    try_add(item)

    return out


def parse_doxygen_members(xml_dir: Path) -> list[dict[str, Any]]:
    members: list[dict[str, Any]] = []

    def _flatten_text(el: ET.Element | None) -> str:
        if el is None:
            return ""
        return " ".join("".join(el.itertext()).split()).strip()

    for xml_file in sorted(xml_dir.glob("*.xml")):
        if xml_file.name == "index.xml":
            continue
        try:
            root = ET.parse(xml_file).getroot()
        except ET.ParseError:
            continue

        for m in root.findall(".//memberdef"):
            kind = (m.get("kind") or "").strip()
            if kind != "function":
                continue
            name = _flatten_text(m.find("name"))
            if not name:
                continue

            loc = m.find("location")
            file_path = loc.get("file", "") if loc is not None else ""
            line = int(loc.get("line", "0") or 0) if loc is not None else 0

            brief = _flatten_text(m.find("briefdescription"))
            ret_type = _flatten_text(m.find("type"))
            has_return = bool(ret_type and ret_type != "void")

            params = m.findall("param")
            param_total = len(params)
            param_docs_names: set[str] = set()
            for plist in m.findall(".//parameterlist"):
                if plist.get("kind") != "param":
                    continue
                for item in plist.findall("parameteritem"):
                    pdesc = _flatten_text(item.find("parameterdescription"))
                    if not pdesc:
                        continue
                    for p in item.findall("./parameternamelist/parametername"):
                        pname = _flatten_text(p)
                        if pname:
                            param_docs_names.add(pname)

            return_doc = ""
            for s in m.findall(".//simplesect"):
                if s.get("kind") == "return":
                    return_doc = _flatten_text(s)
                    if return_doc:
                        break

            members.append(
                {
                    "kind": "function",
                    "name": name,
                    "file": file_path,
                    "line": line,
                    "scope": detect_scope(file_path),
                    "brief": brief,
                    "brief_present": bool(brief),
                    "param_total": param_total,
                    "param_docs_count": len(param_docs_names),
                    "has_return": has_return,
                    "return_doc_present": bool(return_doc),
                }
            )

        comp = root.find("compounddef")
        if comp is not None and (comp.get("kind") or "") in {"class", "struct"}:
            cname = _flatten_text(comp.find("compoundname"))
            if cname:
                members.append(
                    {
                        "kind": comp.get("kind"),
                        "name": cname.split("::")[-1],
                        "file": "",
                        "line": 0,
                        "scope": "other",
                        "brief": _flatten_text(comp.find("briefdescription")),
                        "brief_present": bool(_flatten_text(comp.find("briefdescription"))),
                        "param_total": 0,
                        "param_docs_count": 0,
                        "has_return": False,
                        "return_doc_present": False,
                    }
                )

    return members


def resolve_module_xml_dir(artifacts_root: Path, module: str) -> Path:
    direct = artifacts_root / module / "xml"
    if direct.exists():
        return direct
    candidates = list(artifacts_root.glob(f"batch_*/{module}/xml"))
    if candidates:
        return sorted(candidates)[0]
    raise FileNotFoundError(f"No XML directory found for module '{module}' under {artifacts_root}")


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    artifacts_root = (repo_root / args.doxygen_artifacts_root).resolve()

    files = collect_module_files(repo_root, args.module)

    source_graph_symbols = parse_source_graph(args.module, (repo_root / args.source_graph_json).resolve())
    source_mode = "graph" if source_graph_symbols else "direct_doxygen_markers"

    if source_graph_symbols:
        source_symbols = source_graph_symbols
    else:
        source_symbols = []
        for f in files:
            source_symbols.extend(extract_direct_doxygen_decls(f))

    # Apply noise filter to source symbols.
    filtered_source = [
        s for s in source_symbols if not is_noise_symbol(str(s.get("name", "")), str(s.get("kind", "")), str(s.get("scope", "other")))
    ]

    xml_dir = resolve_module_xml_dir(artifacts_root, args.module)
    dox_members = parse_doxygen_members(xml_dir)

    filtered_dox = [
        m for m in dox_members if not is_noise_symbol(str(m.get("name", "")), str(m.get("kind", "")), str(m.get("scope", "other")))
    ]

    source_names = {str(d["name"]) for d in filtered_source}
    dox_names = {str(m["name"]) for m in filtered_dox}

    missing_symbols = sorted(source_names - dox_names)
    dox_only_symbols = sorted(dox_names - source_names)

    findings: list[dict[str, Any]] = []

    # S1 missing_symbol
    for name in missing_symbols:
        src_items = [s for s in filtered_source if str(s.get("name")) == name]
        scope_breakdown: dict[str, int] = {}
        for item in src_items:
            item_scope = str(item.get("scope", "other"))
            scope_breakdown[item_scope] = scope_breakdown.get(item_scope, 0) + 1
        non_zero_scopes = [k for k, v in scope_breakdown.items() if v > 0]
        dominant_scope = non_zero_scopes[0] if len(non_zero_scopes) == 1 else "mixed"
        findings.append(
            {
                "rule": "missing_symbol",
                "symbol": name,
                "count": len(src_items),
                "scope": dominant_scope,
                "scope_breakdown": scope_breakdown,
                "source_examples": src_items[:3],
            }
        )

    # S2/S3/S4 on Doxygen members
    for m in filtered_dox:
        kind = str(m.get("kind", ""))
        if kind in {"class", "struct"} and not bool(m.get("brief_present")):
            findings.append(
                {
                    "rule": "missing_brief",
                    "symbol": m.get("name"),
                    "kind": kind,
                    "file": m.get("file", ""),
                    "line": m.get("line", 0),
                    "scope": m.get("scope", "other"),
                }
            )
            continue

        if kind == "function":
            if not bool(m.get("brief_present")):
                findings.append(
                    {
                        "rule": "missing_brief",
                        "symbol": m.get("name"),
                        "kind": kind,
                        "file": m.get("file", ""),
                        "line": m.get("line", 0),
                        "scope": m.get("scope", "other"),
                    }
                )
            if int(m.get("param_total", 0)) > 0 and int(m.get("param_docs_count", 0)) < int(m.get("param_total", 0)):
                findings.append(
                    {
                        "rule": "missing_param_docs",
                        "symbol": m.get("name"),
                        "kind": kind,
                        "file": m.get("file", ""),
                        "line": m.get("line", 0),
                        "scope": m.get("scope", "other"),
                        "param_total": m.get("param_total", 0),
                        "param_docs_count": m.get("param_docs_count", 0),
                    }
                )
            if bool(m.get("has_return")) and not bool(m.get("return_doc_present")):
                findings.append(
                    {
                        "rule": "missing_return_docs",
                        "symbol": m.get("name"),
                        "kind": kind,
                        "file": m.get("file", ""),
                        "line": m.get("line", 0),
                        "scope": m.get("scope", "other"),
                    }
                )

    # scope split for gate relevance
    scope_counts: dict[str, int] = {"src": 0, "tests": 0, "benchmarks": 0, "include": 0, "other": 0}
    for f in findings:
        scope = str(f.get("scope", "other"))
        scope_counts[scope] = scope_counts.get(scope, 0) + 1

    by_rule: dict[str, int] = {}
    by_rule_scope: dict[str, dict[str, int]] = {}
    for f in findings:
        rule = f["rule"]
        by_rule[rule] = by_rule.get(rule, 0) + 1
        scope = str(f.get("scope", "other"))
        if rule not in by_rule_scope:
            by_rule_scope[rule] = {}
        by_rule_scope[rule][scope] = by_rule_scope[rule].get(scope, 0) + 1

    coverage = {
        "symbol_presence_ratio": 0.0 if not source_names else round((len(source_names & dox_names) / len(source_names)), 4),
        "missing_symbol_count": len(missing_symbols),
        "missing_brief_count": by_rule.get("missing_brief", 0),
        "missing_param_docs_count": by_rule.get("missing_param_docs", 0),
        "missing_return_docs_count": by_rule.get("missing_return_docs", 0),
    }

    ownership = {
        "path_policy": {
            "internal_roots": INTERNAL_ROOTS,
            "external_hints": EXTERNAL_HINTS,
            "classification_rule": "everything outside internal roots is treated as external / third-party boundary",
        },
        "source_symbols": count_origin(filtered_source),
        "doxygen_members": count_origin(filtered_dox),
        "externally_scoped_symbols": [
            s.get("name") for s in filtered_source if classify_origin_by_path(str(s.get("file", ""))) == "external"
        ][: args.sample_limit],
    }

    report = {
        "module": args.module,
        "source_mode": source_mode,
        "source_graph_used": source_mode == "graph",
        "source_graph_path": str((repo_root / args.source_graph_json).resolve()),
        "source_files_scanned": len(files),
        "source_symbols_total": len(source_symbols),
        "source_symbols_after_filter": len(filtered_source),
        "doxygen_members_total": len(dox_members),
        "doxygen_members_after_filter": len(filtered_dox),
        "source_symbol_names": len(source_names),
        "doxygen_symbol_names": len(dox_names),
        "source_not_in_doxygen_count": len(missing_symbols),
        "doxygen_not_in_source_count": len(dox_only_symbols),
        "coverage": coverage,
        "ownership": ownership,
        "by_rule": by_rule,
        "by_rule_scope": by_rule_scope,
        "by_scope": scope_counts,
        "samples": {
            "source_not_in_doxygen": missing_symbols[: args.sample_limit],
            "doxygen_not_in_source": dox_only_symbols[: args.sample_limit],
            "findings": findings[: args.sample_limit],
        },
    }

    # Detailed report json
    report_path = repo_root / args.report_json
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    # Missing findings artifact
    missing_payload = {
        "module": args.module,
        "generated_from": "check_module_direct_doxygen.py",
        "source_mode": source_mode,
        "findings": findings,
    }
    missing_path = repo_root / args.missing_report_json
    missing_path.parent.mkdir(parents=True, exist_ok=True)
    missing_path.write_text(json.dumps(missing_payload, indent=2), encoding="utf-8")

    # Module summary markdown
    md_lines = [
        f"# Direct Doxygen Check: {args.module}",
        "",
        f"- Source mode: {source_mode}",
        f"- Source files scanned: {report['source_files_scanned']}",
        f"- Source symbols (raw/filter): {report['source_symbols_total']} / {report['source_symbols_after_filter']}",
        f"- Doxygen members (raw/filter): {report['doxygen_members_total']} / {report['doxygen_members_after_filter']}",
        f"- Unique source symbol names: {report['source_symbol_names']}",
        f"- Unique doxygen symbol names: {report['doxygen_symbol_names']}",
        f"- Symbol presence ratio: {coverage['symbol_presence_ratio']}",
        f"- Ownership (internal/external): {report['ownership']['source_symbols']}",
        "",
        "## Rule counts",
    ]
    for rule in ["missing_symbol", "missing_brief", "missing_param_docs", "missing_return_docs"]:
        md_lines.append(f"- {rule}: {by_rule.get(rule, 0)}")

    md_lines.append("")
    md_lines.append("## Scope counts")
    for scope in ["src", "tests", "benchmarks", "include", "other"]:
        md_lines.append(f"- {scope}: {scope_counts.get(scope, 0)}")

    md_lines.append("")
    md_lines.append("## Sample missing symbols")
    if report["samples"]["source_not_in_doxygen"]:
        for s in report["samples"]["source_not_in_doxygen"]:
            md_lines.append(f"- {s}")
    else:
        md_lines.append("- none")

    summary_path = repo_root / args.summary_md
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text("\n".join(md_lines) + "\n", encoding="utf-8")

    # Coverage summary markdown artifact (gate-friendly)
    cov_lines = [
        "# Module Doxygen Coverage Summary",
        "",
        f"- Module: {args.module}",
        f"- Source mode: {source_mode}",
        f"- Symbol presence ratio: {coverage['symbol_presence_ratio']}",
        f"- Ownership: {report['ownership']['source_symbols']}",
        f"- missing_symbol: {by_rule.get('missing_symbol', 0)}",
        f"- missing_brief: {by_rule.get('missing_brief', 0)}",
        f"- missing_param_docs: {by_rule.get('missing_param_docs', 0)}",
        f"- missing_return_docs: {by_rule.get('missing_return_docs', 0)}",
    ]
    cov_path = repo_root / args.coverage_summary_md
    cov_path.parent.mkdir(parents=True, exist_ok=True)
    cov_path.write_text("\n".join(cov_lines) + "\n", encoding="utf-8")

    print("DIRECT_DOXYGEN_CHECK_OK")
    print(f"module={args.module}")
    print(f"source_mode={source_mode}")
    print(f"source_symbols_after_filter={len(filtered_source)}")
    print(f"doxygen_members_after_filter={len(filtered_dox)}")
    print(f"missing_symbol={by_rule.get('missing_symbol', 0)}")
    print(f"missing_brief={by_rule.get('missing_brief', 0)}")
    print(f"missing_param_docs={by_rule.get('missing_param_docs', 0)}")
    print(f"missing_return_docs={by_rule.get('missing_return_docs', 0)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
