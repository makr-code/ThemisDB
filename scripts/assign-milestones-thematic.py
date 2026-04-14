"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            assign-milestones-thematic.py                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:29:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     257                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""Assign closed issues without milestone using module-aware thematic mapping.

Strategy:
- Build module -> default version map from src/*/ROADMAP.md Current Status line.
- Detect module from issue labels and title prefixes.
- Assign only when confidence >= threshold.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parents[1]
REPO = "makr-code/ThemisDB"

VERSION_RE = re.compile(r"v\d+\.\d+\.\d+", re.IGNORECASE)
TITLE_PREFIX_RE = re.compile(r"^(?:feat|fix|docs|refactor|perf|test|build|chore)\(([^)]+)\):", re.IGNORECASE)
BRACKET_PREFIX_RE = re.compile(r"^\[([^\]]+)\]")


@dataclass
class Suggestion:
    issue: int
    title: str
    module: str
    confidence: str
    source: str
    suggested_milestone: str


def run_gh(args: list[str]) -> str:
    result = subprocess.run(["gh", *args], cwd=WORKSPACE, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "gh command failed")
    return result.stdout


def parse_module_versions() -> dict[str, str]:
    mapping: dict[str, str] = {}
    for roadmap in sorted(WORKSPACE.glob("src/**/ROADMAP.md")):
        module = roadmap.parent.name.lower()
        text = roadmap.read_text(encoding="utf-8", errors="replace")
        # Prefer Current Status section; fallback to first version in file.
        current_idx = text.lower().find("## current status")
        search_text = text[current_idx: current_idx + 1200] if current_idx >= 0 else text[:1200]
        m = VERSION_RE.search(search_text)
        if not m:
            m = VERSION_RE.search(text)
        if m:
            mapping[module] = m.group(0).lower()
    return mapping


def extract_module(title: str, labels: list[str], known_modules: set[str]) -> tuple[str | None, str, str]:
    # Highest confidence: explicit module:<name> label
    for label in labels:
        low = label.lower().strip()
        if low.startswith("module:"):
            mod = low.split(":", 1)[1].strip().split("/")[0]
            return mod, "high", "label:module:*"

    # Medium confidence: label equals module name
    for label in labels:
        low = label.lower().strip()
        if low in known_modules:
            return low, "medium", "label:module-name"

    # Medium confidence: feat(module): ...
    m = TITLE_PREFIX_RE.match(title)
    if m:
        mod = m.group(1).strip().lower().split("/")[0]
        return mod, "medium", "title:kind(module)"

    # Medium confidence: [module] ...
    b = BRACKET_PREFIX_RE.match(title)
    if b:
        mod = b.group(1).strip().lower().split("/")[0]
        return mod, "medium", "title:[module]"

    return None, "low", "none"


def load_closed_no_milestone() -> list[dict]:
    out = run_gh([
        "issue",
        "list",
        "--repo",
        REPO,
        "--state",
        "closed",
        "--search",
        "no:milestone",
        "--limit",
        "5000",
        "--json",
        "number,title,labels,milestone",
    ])
    return json.loads(out)


def load_closed_with_milestone() -> list[dict]:
    out = run_gh([
        "issue",
        "list",
        "--repo",
        REPO,
        "--state",
        "closed",
        "--search",
        "milestone:*",
        "--limit",
        "5000",
        "--json",
        "number,title,labels,milestone",
    ])
    return json.loads(out)


def build_empirical_module_versions(issues: list[dict]) -> dict[str, str]:
    per_module: dict[str, Counter] = defaultdict(Counter)
    for issue in issues:
        milestone = (issue.get("milestone") or {}).get("title")
        if not milestone:
            continue
        labels = [str(x.get("name", "")).lower() for x in issue.get("labels", [])]
        for label in labels:
            mod = None
            if label.startswith("module:"):
                mod = label.split(":", 1)[1].strip().split("/")[0]
            elif re.fullmatch(r"[a-z0-9_\-]+", label):
                mod = label.strip().split("/")[0]
            if mod:
                per_module[mod][milestone.lower()] += 1

    selected: dict[str, str] = {}
    for module, counter in per_module.items():
        top, top_count = counter.most_common(1)[0]
        total = sum(counter.values())
        # Conservative acceptance rule.
        if top_count >= 3 and (top_count / total) >= 0.55:
            selected[module] = top
    return selected


def confidence_rank(level: str) -> int:
    return {"low": 0, "medium": 1, "high": 2}.get(level, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description="Thematic milestone assignment")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--min-confidence", choices=["low", "medium", "high"], default="medium")
    parser.add_argument("--output", default="artifacts/thematic-assignment-run.json")
    args = parser.parse_args()

    module_versions = parse_module_versions()
    historical_versions = build_empirical_module_versions(load_closed_with_milestone())

    for module, version in historical_versions.items():
        module_versions.setdefault(module, version)

    known_modules = set(module_versions.keys())
    issues = load_closed_no_milestone()

    threshold = confidence_rank(args.min_confidence)
    suggestions: list[Suggestion] = []

    for issue in issues:
        number = int(issue["number"])
        title = str(issue["title"])
        labels = [str(x.get("name", "")) for x in issue.get("labels", [])]
        module, confidence, source = extract_module(title, labels, known_modules)
        if not module:
            continue
        if module not in module_versions:
            continue
        if confidence_rank(confidence) < threshold:
            continue
        suggestions.append(
            Suggestion(
                issue=number,
                title=title,
                module=module,
                confidence=confidence,
                source=source,
                suggested_milestone=module_versions[module],
            )
        )

    applied = 0
    failed: list[dict] = []
    if args.apply:
        for s in suggestions:
            result = subprocess.run(
                ["gh", "issue", "edit", str(s.issue), "--repo", REPO, "--milestone", s.suggested_milestone],
                cwd=WORKSPACE,
                capture_output=True,
                text=True,
            )
            if result.returncode == 0:
                applied += 1
            else:
                failed.append({"issue": s.issue, "stderr": result.stderr.strip()[:400]})

    payload = {
        "issues_closed_without_milestone": len(issues),
        "module_versions": module_versions,
        "suggestions_total": len(suggestions),
        "apply": bool(args.apply),
        "applied": applied,
        "failed": failed,
        "suggestions": [s.__dict__ for s in suggestions],
    }
    out_path = WORKSPACE / args.output
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    print(f"closed_no_milestone={len(issues)}")
    print(f"suggestions={len(suggestions)}")
    print(f"apply={args.apply}")
    print(f"applied={applied}")
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
