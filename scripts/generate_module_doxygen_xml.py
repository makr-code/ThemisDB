#!/usr/bin/env python3
"""Generate per-module Doxygen XML artifacts plus a machine-checkable manifest.

Purpose:
- Produce XML proof artifacts per src/<module> for internal module structure/dependencies.
- Emit a JSON manifest and Markdown summary for CI artifact verification.

Example:
  python3 scripts/generate_module_doxygen_xml.py \
    --repo-root . \
    --modules query,storage,server \
    --output-root /tmp/module-doxygen \
    --manifest /tmp/module-doxygen/manifest.json \
    --summary-md /tmp/module-doxygen/summary.md
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import os
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

CPP_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp", ".c", ".cc", ".cpp", ".cxx"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate per-module Doxygen XML artifacts")
    parser.add_argument("--repo-root", default=".", help="Repository root")
    parser.add_argument("--modules", help="Comma-separated module names from src/<module>")
    parser.add_argument(
        "--all-modules",
        action="store_true",
        help="Discover all src/<module> directories and process them",
    )
    parser.add_argument("--output-root", required=True, help="Output directory for per-module artifacts")
    parser.add_argument("--manifest", required=True, help="Output manifest JSON path")
    parser.add_argument("--summary-md", required=True, help="Output Markdown summary path")
    parser.add_argument("--doxyfile", default="Doxyfile.audit", help="Base Doxyfile")
    parser.add_argument("--doxygen-bin", default="doxygen", help="Doxygen executable")
    parser.add_argument(
        "--module-timeout-seconds",
        type=int,
        default=0,
        help="Timeout per module Doxygen invocation (0 = no timeout)",
    )
    parser.add_argument("--workers", type=int, default=1, help="Parallel worker count for module runs")
    parser.add_argument("--batch-count", type=int, default=1, help="Split selected modules into N batches")
    parser.add_argument("--batch-index", type=int, default=0, help="0-based batch index to execute")
    parser.add_argument(
        "--include-root-fallback",
        action="store_true",
        help="Also include repo include/ when include/<module> does not exist (slower)",
    )
    parser.add_argument(
        "--write-module-markdown",
        action="store_true",
        help="Write derived src/<module>/DOXYGEN.md for PASS modules",
    )
    return parser.parse_args()


def normalize_modules(raw: str) -> list[str]:
    modules = [m.strip() for m in raw.split(",") if m.strip()]
    unique = []
    seen = set()
    for m in modules:
        if m not in seen:
            unique.append(m)
            seen.add(m)
    return unique


def discover_all_modules(repo_root: Path) -> list[str]:
    src_dir = repo_root / "src"
    if not src_dir.exists():
        return []
    return sorted([p.name for p in src_dir.iterdir() if p.is_dir()])


def slice_modules_for_batch(modules: list[str], batch_count: int, batch_index: int) -> list[str]:
    if batch_count <= 1:
        return modules
    # Round-robin split keeps large modules better distributed than contiguous chunks.
    return [m for idx, m in enumerate(modules) if idx % batch_count == batch_index]


def find_cpp_files(module_dir: Path) -> int:
    count = 0
    for path in module_dir.rglob("*"):
        if path.is_file() and path.suffix.lower() in CPP_SUFFIXES:
            count += 1
    return count


def parse_index_counts(index_xml: Path) -> dict[str, int]:
    counts = {
        "compound_total": 0,
        "class_struct_total": 0,
        "namespace_total": 0,
        "file_total": 0,
    }
    if not index_xml.exists():
        return counts

    root = ET.parse(index_xml).getroot()
    compounds = root.findall("compound")
    counts["compound_total"] = len(compounds)
    for comp in compounds:
        kind = (comp.get("kind") or "").strip()
        if kind in {"class", "struct"}:
            counts["class_struct_total"] += 1
        elif kind == "namespace":
            counts["namespace_total"] += 1
        elif kind == "file":
            counts["file_total"] += 1
    return counts


def parse_index_details(index_xml: Path) -> dict[str, list[str]]:
    details: dict[str, list[str]] = {
        "namespaces": [],
        "classes": [],
        "structs": [],
        "files": [],
    }
    if not index_xml.exists():
        return details

    root = ET.parse(index_xml).getroot()
    for comp in root.findall("compound"):
        kind = (comp.get("kind") or "").strip()
        name_el = comp.find("name")
        name = (name_el.text or "").strip() if name_el is not None else ""
        if not name:
            continue
        if kind == "namespace":
            details["namespaces"].append(name)
        elif kind == "class":
            details["classes"].append(name)
        elif kind == "struct":
            details["structs"].append(name)
        elif kind == "file":
            details["files"].append(name)

    for key in details:
        details[key] = sorted(details[key])
    return details


def _flatten_text(element: ET.Element | None) -> str:
    if element is None:
        return ""
    raw = "".join(element.itertext())
    return " ".join(raw.split()).strip()


def _escape_md(text: str) -> str:
    return text.replace("|", "\\|").strip()


def _extract_param_docs(member: ET.Element) -> dict[str, str]:
    out: dict[str, str] = {}
    for section_name in ("briefdescription", "detaileddescription"):
        section = member.find(section_name)
        if section is None:
            continue
        for plist in section.findall(".//parameterlist"):
            if plist.get("kind") != "param":
                continue
            for item in plist.findall("parameteritem"):
                pnames = [
                    _flatten_text(n)
                    for n in item.findall("./parameternamelist/parametername")
                    if _flatten_text(n)
                ]
                pdesc = _flatten_text(item.find("parameterdescription"))
                if not pdesc:
                    continue
                for pname in pnames:
                    out[pname] = pdesc
    return out


def _extract_return_doc(member: ET.Element) -> str:
    for section_name in ("briefdescription", "detaileddescription"):
        section = member.find(section_name)
        if section is None:
            continue
        for s in section.findall(".//simplesect"):
            if s.get("kind") == "return":
                txt = _flatten_text(s)
                if txt:
                    return txt
    return ""


def _extract_throws_docs(member: ET.Element) -> list[str]:
    throws: list[str] = []
    for section_name in ("briefdescription", "detaileddescription"):
        section = member.find(section_name)
        if section is None:
            continue
        for plist in section.findall(".//parameterlist"):
            if plist.get("kind") != "exception":
                continue
            for item in plist.findall("parameteritem"):
                names = [
                    _flatten_text(n)
                    for n in item.findall("./parameternamelist/parametername")
                    if _flatten_text(n)
                ]
                desc = _flatten_text(item.find("parameterdescription"))
                label = ", ".join(names) if names else "exception"
                if desc:
                    throws.append(f"{label}: {desc}")
                else:
                    throws.append(label)
        for s in section.findall(".//simplesect"):
            if s.get("kind") == "exception":
                txt = _flatten_text(s)
                if txt:
                    throws.append(txt)
    return throws


def parse_function_docs(module_output_dir: Path) -> list[dict[str, Any]]:
    xml_dir = module_output_dir / "xml"
    if not xml_dir.exists():
        return []

    docs: list[dict[str, Any]] = []
    for xml_file in sorted(xml_dir.glob("*.xml")):
        if xml_file.name == "index.xml":
            continue
        try:
            root = ET.parse(xml_file).getroot()
        except ET.ParseError:
            continue

        compound = root.find("compounddef")
        if compound is None:
            continue
        owner = _flatten_text(compound.find("compoundname")) or compound.get("kind", "unknown")

        for member in compound.findall(".//memberdef"):
            if member.get("kind") != "function":
                continue

            name = _flatten_text(member.find("name"))
            if not name:
                continue

            args = _flatten_text(member.find("argsstring"))
            ret_type = _flatten_text(member.find("type"))
            brief = _flatten_text(member.find("briefdescription"))
            detailed = _flatten_text(member.find("detaileddescription"))
            location = member.find("location")
            file_path = location.get("file", "") if location is not None else ""
            line = location.get("line", "") if location is not None else ""

            param_docs = _extract_param_docs(member)
            params: list[dict[str, str]] = []
            for param in member.findall("param"):
                ptype = _flatten_text(param.find("type"))
                pname = _flatten_text(param.find("declname"))
                if not pname and _flatten_text(param.find("defname")):
                    pname = _flatten_text(param.find("defname"))
                pdesc = param_docs.get(pname, "") if pname else ""
                params.append({"name": pname, "type": ptype, "desc": pdesc})

            docs.append(
                {
                    "owner": owner,
                    "name": name,
                    "args": args,
                    "return_type": ret_type,
                    "brief": brief,
                    "detailed": detailed,
                    "params": params,
                    "return_doc": _extract_return_doc(member),
                    "throws": _extract_throws_docs(member),
                    "file": file_path,
                    "line": line,
                }
            )

    docs.sort(key=lambda d: (d.get("owner", ""), d.get("name", ""), d.get("args", "")))
    return docs


def write_module_markdown(
    repo_root: Path,
    module: str,
    result: dict[str, Any],
    details: dict[str, list[str]],
    function_docs: list[dict[str, Any]],
    generated_at: str,
    output_root: Path,
) -> None:
    module_doc = repo_root / "src" / module / "DOXYGEN.md"
    module_doc.parent.mkdir(parents=True, exist_ok=True)

    rel_index = result.get("xml_index", "")
    rel_warn = result.get("warnings_log", "")
    artifact_index = (output_root / rel_index).resolve() if rel_index else None
    artifact_warn = (output_root / rel_warn).resolve() if rel_warn else None

    lines: list[str] = []
    lines.append(f"# {module.upper()} DOXYGEN")
    lines.append("")
    lines.append("Author: ThemisDB Contributors")
    lines.append(f"Created: {generated_at[:10]}")
    lines.append(f"Last Updated: {generated_at[:10]}")
    lines.append("Status: active")
    lines.append("")
    lines.append("## Source")
    lines.append("- This file is generated from module-scoped Doxygen XML.")
    if artifact_index is not None:
        lines.append(f"- XML index: `{artifact_index}`")
    if artifact_warn is not None:
        lines.append(f"- Warnings log: `{artifact_warn}`")
    lines.append("")
    lines.append("## Structure Metrics")
    lines.append(f"- C/C++ Files (scanned): {result.get('cpp_file_count', 0)}")
    lines.append(f"- Compounds: {result.get('compound_total', 0)}")
    lines.append(f"- Classes/Structs: {result.get('class_struct_total', 0)}")
    lines.append(f"- Namespaces: {result.get('namespace_total', 0)}")
    lines.append(f"- File Compounds: {result.get('file_total', 0)}")
    lines.append("")

    lines.append("## Namespaces")
    if details["namespaces"]:
        for name in details["namespaces"]:
            lines.append(f"- {name}")
    else:
        lines.append("- none")
    lines.append("")

    lines.append("## Types")
    lines.append("### Classes")
    if details["classes"]:
        for name in details["classes"]:
            lines.append(f"- {name}")
    else:
        lines.append("- none")
    lines.append("")

    lines.append("### Structs")
    if details["structs"]:
        for name in details["structs"]:
            lines.append(f"- {name}")
    else:
        lines.append("- none")
    lines.append("")

    lines.append("## Notes")
    lines.append("- This is a derived artifact. Do not manually edit semantic content.")
    lines.append("- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.")
    lines.append("")

    lines.append("## Detailed Function Documentation (Soll-Ist)")
    lines.append(f"- Functions extracted: {len(function_docs)}")
    lines.append("")

    if function_docs:
        current_owner = ""
        for fn in function_docs:
            owner = str(fn.get("owner", ""))
            if owner != current_owner:
                current_owner = owner
                lines.append(f"### {owner}")
                lines.append("")

            sig = f"{fn.get('return_type', '').strip()} {fn.get('name', '').strip()}{fn.get('args', '').strip()}".strip()
            lines.append(f"#### `{_escape_md(sig)}`")
            file_ref = str(fn.get("file", "")).strip()
            line_ref = str(fn.get("line", "")).strip()
            if file_ref:
                lines.append(f"- Source: `{_escape_md(file_ref)}`{':' + line_ref if line_ref else ''}")
            brief = str(fn.get("brief", "")).strip()
            lines.append(f"- Brief: {brief if brief else 'n/a'}")

            params = fn.get("params", [])
            if params:
                lines.append("- Parameters:")
                for p in params:
                    pname = str(p.get("name", "")).strip() or "<unnamed>"
                    ptype = str(p.get("type", "")).strip() or "n/a"
                    pdesc = str(p.get("desc", "")).strip() or "n/a"
                    lines.append(f"  - `{_escape_md(pname)}` ({_escape_md(ptype)}): {_escape_md(pdesc)}")
            else:
                lines.append("- Parameters: none")

            ret_doc = str(fn.get("return_doc", "")).strip()
            if ret_doc:
                lines.append(f"- Return: {_escape_md(ret_doc)}")

            throws = fn.get("throws", [])
            if throws:
                lines.append("- Throws:")
                for t in throws:
                    lines.append(f"  - {_escape_md(str(t))}")

            detailed = str(fn.get("detailed", "")).strip()
            if detailed:
                lines.append(f"- Details: {_escape_md(detailed)}")
            lines.append("")
    else:
        lines.append("No function-level Doxygen descriptions found in XML.")

    module_doc.write_text("\n".join(lines) + "\n", encoding="utf-8")


def collect_cpp_files(root: Path) -> list[Path]:
    files: list[Path] = []
    if not root.exists():
        return files
    for path in root.rglob("*"):
        if path.is_file() and path.suffix.lower() in CPP_SUFFIXES:
            files.append(path)
    return files


def resolve_module_inputs(repo_root: Path, module: str, include_root_fallback: bool) -> list[Path]:
    """Resolve focused per-module Doxygen file inputs.

    Preference order:
    1) files from include/<module>
    2) files from src/<module>
    3) files from tests/<module>
    4) files from benchmarks/<module>
    5) files from include/ (fallback only when include/<module> missing)
    """
    inputs: list[Path] = []
    include_module = repo_root / "include" / module
    src_module = repo_root / "src" / module
    tests_module = repo_root / "tests" / module
    benchmarks_module = repo_root / "benchmarks" / module

    if include_module.exists():
        inputs.extend(collect_cpp_files(include_module))

    if src_module.exists():
        inputs.extend(collect_cpp_files(src_module))

    if tests_module.exists():
        inputs.extend(collect_cpp_files(tests_module))

    if benchmarks_module.exists():
        inputs.extend(collect_cpp_files(benchmarks_module))

    if include_root_fallback and not include_module.exists():
        include_root = repo_root / "include"
        if include_root.exists():
            inputs.extend(collect_cpp_files(include_root))

    # De-duplicate while preserving order.
    unique: list[Path] = []
    seen = set()
    for p in inputs:
        key = str(p.resolve())
        if key not in seen:
            unique.append(p)
            seen.add(key)
    return unique


def write_temp_doxyfile(base_doxyfile: Path, module: str, output_dir: Path, module_inputs: list[Path]) -> Path:
    text = base_doxyfile.read_text(encoding="utf-8")
    if not module_inputs:
        input_value = '""'
    else:
        input_value = " ".join(f'"{p.as_posix()}"' for p in module_inputs)
    overrides = [
        f'PROJECT_NAME = "ThemisDB::{module}"',
        f'OUTPUT_DIRECTORY = "{output_dir.as_posix()}"',
        f'INPUT = {input_value}',
        'RECURSIVE = NO',
        'EXCLUDE =',
        'EXCLUDE_PATTERNS =',
        'GENERATE_HTML = NO',
        'GENERATE_LATEX = NO',
        'GENERATE_XML = YES',
        'XML_OUTPUT = xml',
        'WARN_AS_ERROR = NO',
        f'WARN_LOGFILE = "{(output_dir / "doxygen-warnings.log").as_posix()}"',
    ]

    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", suffix=".doxyfile", delete=False) as fh:
        fh.write(text)
        fh.write("\n\n# --- Module XML overrides (generated) ---\n")
        for line in overrides:
            fh.write(line + "\n")
        return Path(fh.name)


def run_module_doxygen(
    repo_root: Path,
    module: str,
    base_doxyfile: Path,
    output_root: Path,
    doxygen_bin: str,
    include_root_fallback: bool,
    module_timeout_seconds: int,
) -> dict[str, Any]:
    module_dir = repo_root / "src" / module
    module_output_dir = output_root / module
    module_output_dir.mkdir(parents=True, exist_ok=True)

    result: dict[str, Any] = {
        "module": module,
        "status": "FAIL",
        "reason": "",
        "cpp_file_count": 0,
        "xml_index": "",
        "warnings_log": "",
        "duration_seconds": 0.0,
        "doxygen_exit_code": -1,
    }

    if not module_dir.exists():
        result["reason"] = f"missing module dir: {module_dir}"
        return result

    module_inputs = resolve_module_inputs(repo_root, module, include_root_fallback)
    cpp_count = len(module_inputs)
    result["cpp_file_count"] = cpp_count
    if cpp_count == 0:
        result["status"] = "SKIP"
        result["reason"] = "no C/C++ files in include/src/tests/benchmarks module scope"
        return result

    temp_doxyfile = write_temp_doxyfile(base_doxyfile, module, module_output_dir, module_inputs)
    start = time.time()
    try:
        try:
            kwargs: dict[str, Any] = {
                "cwd": str(repo_root),
                "stdout": subprocess.PIPE,
                "stderr": subprocess.STDOUT,
                "text": False,
                "check": False,
            }
            if module_timeout_seconds > 0:
                kwargs["timeout"] = module_timeout_seconds
            proc = subprocess.run([doxygen_bin, str(temp_doxyfile)], **kwargs)
        except subprocess.TimeoutExpired as exc:
            duration = round(time.time() - start, 3)
            result["duration_seconds"] = duration
            result["doxygen_exit_code"] = -2
            timeout_log = module_output_dir / "doxygen.stdout.log"
            out = exc.output or b""
            timeout_log.write_text(out.decode("utf-8", errors="replace"), encoding="utf-8", errors="replace")
            result["reason"] = f"doxygen timeout after {module_timeout_seconds}s"
            return result
        duration = round(time.time() - start, 3)
        result["duration_seconds"] = duration
        result["doxygen_exit_code"] = proc.returncode

        log_path = module_output_dir / "doxygen.stdout.log"
        stdout_text = proc.stdout.decode("utf-8", errors="replace") if proc.stdout else ""
        log_path.write_text(stdout_text, encoding="utf-8", errors="replace")

        index_xml = module_output_dir / "xml" / "index.xml"
        warnings_log = module_output_dir / "doxygen-warnings.log"
        result["xml_index"] = str(index_xml.relative_to(output_root)) if index_xml.exists() else ""
        result["warnings_log"] = str(warnings_log.relative_to(output_root)) if warnings_log.exists() else ""

        if proc.returncode != 0:
            result["reason"] = "doxygen non-zero exit"
            return result

        if not index_xml.exists():
            result["reason"] = "missing xml/index.xml"
            return result

        result.update(parse_index_counts(index_xml))
        result["status"] = "PASS"
        return result
    finally:
        try:
            temp_doxyfile.unlink(missing_ok=True)
        except OSError:
            pass


def write_summary(summary_path: Path, manifest: dict[str, Any]) -> None:
    lines: list[str] = []
    lines.append("# Module Doxygen XML Artifact Summary")
    lines.append("")
    lines.append(f"- Generated At: {manifest['generated_at']}")
    lines.append(f"- Doxyfile: {manifest['doxyfile']}")
    lines.append(f"- Modules Requested: {manifest['totals']['requested']}")
    lines.append(f"- Workers: {manifest['workers']}")
    lines.append(f"- Batch: {manifest['batch_index'] + 1}/{manifest['batch_count']}")
    lines.append(f"- PASS: {manifest['totals']['pass']}")
    lines.append(f"- SKIP: {manifest['totals']['skip']}")
    lines.append(f"- FAIL: {manifest['totals']['fail']}")
    lines.append("")
    lines.append("| Module | Status | C/C++ Files | Compounds | Classes/Structs | Namespaces | XML Index |")
    lines.append("|---|---|---:|---:|---:|---:|---|")
    for item in manifest["modules"]:
        lines.append(
            "| {module} | {status} | {cpp_file_count} | {compound_total} | {class_struct_total} | {namespace_total} | {xml_index} |".format(
                module=item.get("module", ""),
                status=item.get("status", ""),
                cpp_file_count=item.get("cpp_file_count", 0),
                compound_total=item.get("compound_total", 0),
                class_struct_total=item.get("class_struct_total", 0),
                namespace_total=item.get("namespace_total", 0),
                xml_index=item.get("xml_index", ""),
            )
        )
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    output_root = Path(args.output_root).resolve()
    manifest_path = Path(args.manifest).resolve()
    summary_path = Path(args.summary_md).resolve()

    if args.workers < 1:
        print("ERROR: --workers must be >= 1")
        return 2
    if args.batch_count < 1:
        print("ERROR: --batch-count must be >= 1")
        return 2
    if args.batch_index < 0 or args.batch_index >= args.batch_count:
        print("ERROR: --batch-index must be in [0, batch-count)")
        return 2

    if args.all_modules:
        modules = discover_all_modules(repo_root)
    else:
        modules = normalize_modules(args.modules or "")

    if not args.all_modules and not (args.modules and args.modules.strip()):
        print("ERROR: Provide --modules or use --all-modules")
        return 2

    if not modules:
        print("No modules requested; nothing to do.")
        return 0

    modules = slice_modules_for_batch(modules, args.batch_count, args.batch_index)
    if not modules:
        print("Selected batch contains no modules; nothing to do.")
        return 0

    base_doxyfile = (repo_root / args.doxyfile).resolve()
    if not base_doxyfile.exists():
        print(f"ERROR: Doxyfile not found: {base_doxyfile}")
        return 2

    output_root.mkdir(parents=True, exist_ok=True)

    module_results: list[dict[str, Any]] = []
    generated_at = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    total = len(modules)
    module_order = {name: idx for idx, name in enumerate(modules)}

    def process_module(module: str) -> dict[str, Any]:
        result = run_module_doxygen(
            repo_root,
            module,
            base_doxyfile,
            output_root,
            args.doxygen_bin,
            args.include_root_fallback,
            args.module_timeout_seconds,
        )
        if args.write_module_markdown and result.get("status") == "PASS":
            rel_index = result.get("xml_index", "")
            index_xml = (output_root / rel_index) if rel_index else None
            if index_xml is not None and index_xml.exists():
                details = parse_index_details(index_xml)
                function_docs = parse_function_docs(output_root / module)
                write_module_markdown(
                    repo_root=repo_root,
                    module=module,
                    result=result,
                    details=details,
                    function_docs=function_docs,
                    generated_at=generated_at,
                    output_root=output_root,
                )
        return result

    if args.workers == 1:
        for idx, module in enumerate(modules, start=1):
            print(f"[{idx}/{total}] module={module} start", flush=True)
            result = process_module(module)
            module_results.append(result)
            print(
                f"[{idx}/{total}] module={module} status={result.get('status')} compounds={result.get('compound_total', 0)} classes={result.get('class_struct_total', 0)}",
                flush=True,
            )
    else:
        print(f"Running with workers={args.workers}, batch={args.batch_index + 1}/{args.batch_count}, modules={total}", flush=True)
        completed = 0
        with ThreadPoolExecutor(max_workers=args.workers) as ex:
            futures = {ex.submit(process_module, module): module for module in modules}
            for future in as_completed(futures):
                module = futures[future]
                result = future.result()
                module_results.append(result)
                completed += 1
                print(
                    f"[{completed}/{total}] module={module} status={result.get('status')} compounds={result.get('compound_total', 0)} classes={result.get('class_struct_total', 0)}",
                    flush=True,
                )

    module_results.sort(key=lambda r: module_order.get(str(r.get("module", "")), 10**9))

    totals = {
        "requested": len(modules),
        "pass": sum(1 for r in module_results if r.get("status") == "PASS"),
        "skip": sum(1 for r in module_results if r.get("status") == "SKIP"),
        "fail": sum(1 for r in module_results if r.get("status") == "FAIL"),
    }

    manifest = {
        "tool": "generate_module_doxygen_xml.py",
        "generated_at": generated_at,
        "selection_mode": "all_modules" if args.all_modules else "explicit_modules",
        "workers": args.workers,
        "batch_count": args.batch_count,
        "batch_index": args.batch_index,
        "include_root_fallback": args.include_root_fallback,
        "write_module_markdown": args.write_module_markdown,
        "repo_root": str(repo_root),
        "doxyfile": str(base_doxyfile.relative_to(repo_root)),
        "output_root": str(output_root),
        "modules": module_results,
        "totals": totals,
    }

    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    write_summary(summary_path, manifest)

    # Fail if any requested module with C/C++ files has no XML artifact.
    if totals["fail"] > 0:
        print(f"Module Doxygen XML generation failed for {totals['fail']} module(s).")
        return 1

    print(f"Module Doxygen XML generation completed: pass={totals['pass']}, skip={totals['skip']}, fail={totals['fail']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
