#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import argparse, importlib, json
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any, Dict, List

SCOPE_PREFIXES = ("src/", "include/", "docs/", "examples/", "tests/", "benchmarks/")
EXCLUDE_PARTS = ("third_party/", "vcpkg", "build", "releases/")

@dataclass
class Finding:
    file: str
    line: int
    type: str
    severity: str
    message: str

@dataclass
class FixResult:
    status: str     # fixed|skipped|unsafe
    reason: str
    file: str
    line: int
    type: str
    before: str = ""
    after: str = ""

FIXER_MAP = {
    "docs_broken_markdown_link": "tools.fixers.markdown_link_fixer",
    "string_concat_loop": "tools.fixers.string_concat_loop_fixer",
    "hardcoded_output": "tools.fixers.logging_fixer",
    "unstructured_log": "tools.fixers.logging_fixer",
    "missing_doxygen_comment": "tools.fixers.doxygen_fixer_adapter",
    "missing_doxygen_brief": "tools.fixers.doxygen_fixer_adapter",
    "missing_doxygen_param": "tools.fixers.doxygen_fixer_adapter",
    "missing_doxygen_return": "tools.fixers.doxygen_fixer_adapter",
}

def in_scope(path: str) -> bool:
    if any(x in path for x in EXCLUDE_PARTS):
        return False
    return path.startswith(SCOPE_PREFIXES)

def load_findings(p: Path) -> List[Finding]:
    data = json.loads(p.read_text(encoding="utf-8"))
    out: List[Finding] = []
    for x in data:
        out.append(Finding(
            file=x.get("file", ""),
            line=int(x.get("line", 1)),
            type=x.get("type", ""),
            severity=x.get("severity", "MEDIUM"),
            message=x.get("message", "")
        ))
    return out

def run():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=".")
    ap.add_argument("--findings", default="ai_working/gap_scan_results.json")
    ap.add_argument("--types", default="", help="comma-separated whitelist")
    ap.add_argument("--check-only", action="store_true")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--report", default="ai_working/autofix_report.json")
    ap.add_argument("--max-risk", default="medium", choices=["low", "medium", "high"])
    args = ap.parse_args()

    if not args.check_only and not args.apply:
        raise SystemExit("Use --check-only or --apply")

    root = Path(args.root).resolve()
    findings_path = (root / args.findings).resolve()
    findings = load_findings(findings_path)

    allow = set(x.strip() for x in args.types.split(",") if x.strip())
    results: List[FixResult] = []
    doxygen_batch: List[Finding] = []

    for f in findings:
        if allow and f.type not in allow:
            continue
        if not in_scope(f.file):
            continue

        mod_name = FIXER_MAP.get(f.type)
        if not mod_name:
            results.append(FixResult("skipped", "no_fixer_registered", f.file, f.line, f.type))
            continue

        if mod_name.endswith("doxygen_fixer_adapter"):
            doxygen_batch.append(f)
            continue

        mod = importlib.import_module(mod_name)
        r = mod.fix(root=root, finding=asdict(f), check_only=args.check_only)
        results.append(FixResult(**r))

    # one-shot doxygen adapter call
    if doxygen_batch:
        mod = importlib.import_module("tools.fixers.doxygen_fixer_adapter")
        batch_res = mod.fix_batch(root=root, findings=[asdict(x) for x in doxygen_batch], check_only=args.check_only)
        for r in batch_res:
            results.append(FixResult(**r))

    summary = {"fixed": 0, "skipped": 0, "unsafe": 0}
    by_type: Dict[str, Dict[str, int]] = {}
    for r in results:
        summary[r.status] = summary.get(r.status, 0) + 1
        by_type.setdefault(r.type, {"fixed": 0, "skipped": 0, "unsafe": 0})
        by_type[r.type][r.status] += 1

    out = {
        "summary": summary,
        "by_type": by_type,
        "results": [asdict(x) for x in results],
    }
    rp = (root / args.report).resolve()
    rp.parent.mkdir(parents=True, exist_ok=True)
    rp.write_text(json.dumps(out, indent=2, ensure_ascii=False), encoding="utf-8")

    print(json.dumps(summary, indent=2))
    if args.check_only and summary["fixed"] > 0:
        raise SystemExit(1)

if __name__ == "__main__":
    run()
