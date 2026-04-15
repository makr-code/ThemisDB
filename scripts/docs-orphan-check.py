"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs-orphan-check.py                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-15 04:15:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     299                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dbc9bfed9f  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set

TOOL_NAME = "docs-orphan-check"
TOOL_VERSION = "1.0.0"
CODE_ROOTS = ("src", "include")
DEFAULT_DOC_ROOTS = ("docs/de", "docs/en")
PRIMARY_SOURCES_FILE = "PRIMARY_SOURCES.md"
PRIMARY_PATH_PREFIX = r"(?:src|include|examples|external/chimera/src|external/chimera/include)"
PRIMARY_REF_RE = re.compile(rf"`(({PRIMARY_PATH_PREFIX})/[^`]+?\.md)`")
PRIMARY_LINK_RE = re.compile(rf"\((?:\.\./)+(({PRIMARY_PATH_PREFIX})/[^)#]+?\.md)\)")


def find_repo_root() -> Path:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True,
        )
        return Path(result.stdout.strip())
    except Exception:
        return Path(__file__).resolve().parent.parent


@dataclass
class ModuleDocsDir:
    docs_root: str
    module_name: str
    primary_sources_path: str
    referenced_paths: List[str] = field(default_factory=list)
    missing_references: List[str] = field(default_factory=list)


@dataclass
class OrphanReport:
    code_modules: List[str]
    docs_roots: List[str]
    module_docs_dirs: List[ModuleDocsDir]
    orphan_module_dirs: List[ModuleDocsDir]

    @property
    def broken_reference_count(self) -> int:
        return sum(len(item.missing_references) for item in self.module_docs_dirs)


def collect_code_modules(repo_root: Path) -> Set[str]:
    modules: Set[str] = set()
    for root_name in CODE_ROOTS:
        root_path = repo_root / root_name
        if not root_path.is_dir():
            continue
        for entry in root_path.iterdir():
            if entry.is_dir():
                modules.add(entry.name)
    return modules


def _extract_primary_references(content: str) -> List[str]:
    refs: Set[str] = set()
    for match in PRIMARY_REF_RE.finditer(content):
        refs.add(match.group(1).strip())
    for match in PRIMARY_LINK_RE.finditer(content):
        refs.add(match.group(1).strip())
    return sorted(refs)


def collect_module_docs_dirs(repo_root: Path, docs_roots: List[str]) -> List[ModuleDocsDir]:
    module_dirs: List[ModuleDocsDir] = []

    for docs_root in docs_roots:
        root_path = repo_root / docs_root
        if not root_path.is_dir():
            continue

        for entry in sorted(root_path.iterdir()):
            if not entry.is_dir():
                continue

            primary_sources = entry / PRIMARY_SOURCES_FILE
            if not primary_sources.is_file():
                continue

            content = primary_sources.read_text(encoding="utf-8", errors="replace")
            refs = _extract_primary_references(content)
            missing = [ref for ref in refs if not (repo_root / ref).exists()]

            module_dirs.append(
                ModuleDocsDir(
                    docs_root=docs_root,
                    module_name=entry.name,
                    primary_sources_path=str(primary_sources.relative_to(repo_root)).replace("\\", "/"),
                    referenced_paths=refs,
                    missing_references=missing,
                )
            )

    return module_dirs


def run_check(repo_root: Path, docs_roots: List[str]) -> OrphanReport:
    code_modules = sorted(collect_code_modules(repo_root))
    code_module_set = set(code_modules)
    module_docs_dirs = collect_module_docs_dirs(repo_root, docs_roots)
    orphan_dirs = sorted(
        [item for item in module_docs_dirs if item.module_name not in code_module_set],
        key=lambda item: (item.docs_root, item.module_name),
    )
    return OrphanReport(
        code_modules=code_modules,
        docs_roots=docs_roots,
        module_docs_dirs=module_docs_dirs,
        orphan_module_dirs=orphan_dirs,
    )


def format_text(report: OrphanReport) -> str:
    lines: List[str] = []
    lines.append("-" * 72)
    lines.append(f"  ThemisDB docs orphan check  ·  {TOOL_NAME} v{TOOL_VERSION}")
    lines.append("-" * 72)
    lines.append("")
    lines.append(f"  Code modules scanned      : {len(report.code_modules):>4}")
    lines.append(f"  Docs roots scanned        : {', '.join(report.docs_roots)}")
    lines.append(f"  PRIMARY_SOURCES files     : {len(report.module_docs_dirs):>4}")
    lines.append(f"  Orphan module doc dirs    : {len(report.orphan_module_dirs):>4}")
    lines.append(f"  Broken source references  : {report.broken_reference_count:>4}")
    lines.append("")

    if report.orphan_module_dirs:
        lines.append("Orphan module documentation directories")
        lines.append("-" * 72)
        for item in report.orphan_module_dirs:
            lines.append(
                f"  - {item.docs_root}/{item.module_name}/  -> no matching top-level module in src/ or include/"
            )
        lines.append("")

    broken_items = [item for item in report.module_docs_dirs if item.missing_references]
    if broken_items:
        lines.append("Broken PRIMARY_SOURCES references")
        lines.append("-" * 72)
        for item in broken_items:
            lines.append(f"  - {item.primary_sources_path}")
            for ref in item.missing_references:
                lines.append(f"      missing: {ref}")
        lines.append("")

    if not report.orphan_module_dirs and not broken_items:
        lines.append("  No orphan module-doc directories and no broken PRIMARY_SOURCES references found.")
        lines.append("")

    return "\n".join(lines)


def format_json(report: OrphanReport) -> str:
    payload = {
        "tool": TOOL_NAME,
        "version": TOOL_VERSION,
        "docs_roots": report.docs_roots,
        "summary": {
            "code_modules": len(report.code_modules),
            "module_docs_dirs": len(report.module_docs_dirs),
            "orphan_module_dirs": len(report.orphan_module_dirs),
            "broken_primary_references": report.broken_reference_count,
        },
        "orphan_module_dirs": [
            {
                "docs_root": item.docs_root,
                "module_name": item.module_name,
                "primary_sources_path": item.primary_sources_path,
            }
            for item in report.orphan_module_dirs
        ],
        "primary_sources": [
            {
                "docs_root": item.docs_root,
                "module_name": item.module_name,
                "primary_sources_path": item.primary_sources_path,
                "referenced_paths": item.referenced_paths,
                "missing_references": item.missing_references,
            }
            for item in sorted(report.module_docs_dirs, key=lambda item: (item.docs_root, item.module_name))
        ],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description=(
            "Detect orphaned module documentation directories in docs/<lang>/ that still "
            "contain PRIMARY_SOURCES.md, and report broken references to supported "
            "source documentation roots such as src/include/examples and "
            "external/chimera from those generated indexes."
        ),
    )
    parser.add_argument("--repo-root", metavar="DIR", help="Repository root (auto-detected via git if omitted).")
    parser.add_argument(
        "--docs-dirs",
        metavar="DIRS",
        default=",".join(DEFAULT_DOC_ROOTS),
        help=f"Comma-separated docs roots relative to repo root (default: {','.join(DEFAULT_DOC_ROOTS)}).",
    )
    parser.add_argument("--format", choices=["text", "json"], default="text", help="Output format.")
    parser.add_argument("--output", metavar="PATH", help="Write report to this file instead of stdout.")
    parser.add_argument(
        "--fail-on-findings",
        action="store_true",
        help="Exit 1 if orphan docs dirs or broken PRIMARY_SOURCES references are found.",
    )
    parser.add_argument("--quiet", action="store_true", help="Suppress informational stderr output.")
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    repo_root = Path(args.repo_root).resolve() if args.repo_root else find_repo_root()
    docs_roots = [item.strip() for item in args.docs_dirs.split(",") if item.strip()]

    if not repo_root.is_dir():
        print(f"ERROR: repo root not found: {repo_root}", file=sys.stderr)
        return 2

    if not args.quiet:
        print(f"{TOOL_NAME} v{TOOL_VERSION}: scanning {repo_root} ...", file=sys.stderr)

    try:
        report = run_check(repo_root, docs_roots)
    except Exception as exc:
        print(f"ERROR: docs orphan check failed: {exc}", file=sys.stderr)
        return 2

    rendered = format_json(report) if args.format == "json" else format_text(report)

    if args.output:
        try:
            Path(args.output).write_text(rendered, encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: cannot write report to {args.output}: {exc}", file=sys.stderr)
            return 2
    else:
        print(rendered)

    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        try:
            with open(github_output, "a", encoding="utf-8") as handle:
                handle.write(f"code_modules={len(report.code_modules)}\n")
                handle.write(f"module_docs_dirs={len(report.module_docs_dirs)}\n")
                handle.write(f"orphan_module_dirs={len(report.orphan_module_dirs)}\n")
                handle.write(f"broken_primary_references={report.broken_reference_count}\n")
        except OSError:
            pass

    has_findings = bool(report.orphan_module_dirs or report.broken_reference_count)
    if args.fail_on_findings and has_findings:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())