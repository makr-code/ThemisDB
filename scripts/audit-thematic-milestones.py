"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audit-thematic-milestones.py                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:29:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     361                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""Audit GitHub issues against roadmap/docs sources and suggest thematic milestones.

This script does not change GitHub state. It reads structured planning sources from
the repository and produces high-confidence milestone suggestions for issues based
on module, title similarity, roadmap targets, and docs/de reality-check findings.
"""

from __future__ import annotations

import argparse
import difflib
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parents[1]
ISSUE_RE = re.compile(r"\(Issue:\s*#(\d+)\)")
TARGET_RE = re.compile(r"\(Target:\s*([^)]+)\)")
VERSION_RE = re.compile(r"v\d+\.\d+(?:\.\d+)?")
PHASE_VERSION_RE = re.compile(r"v\d+\.\d+(?:\.\d+)?", re.IGNORECASE)
MODULE_PREFIX_RE = re.compile(r"^(feat|fix|docs|refactor|perf|test|build|chore)\(([^)]+)\):\s*(.+)$", re.IGNORECASE)
BRACKET_MODULE_RE = re.compile(r"^\[([^\]]+)\]\s*(.+)$")


@dataclass
class RoadmapEntry:
    module: str
    issue_number: int | None
    text: str
    target_label: str | None
    target_version: str | None
    source: str


@dataclass
class DocSuggestion:
    module: str
    title: str
    target_version: str | None
    source: str


def normalize_version(value: str | None) -> str | None:
    if not value:
        return None
    match = VERSION_RE.search(value)
    if not match:
        return None
    version = match.group(0)
    if version.count(".") == 1:
        return f"{version}.0"
    return version


def normalize_title(value: str) -> str:
    value = value.lower().strip()
    module_match = MODULE_PREFIX_RE.match(value)
    if module_match:
        value = module_match.group(3)
    bracket_match = BRACKET_MODULE_RE.match(value)
    if bracket_match:
        value = bracket_match.group(2)
    value = re.sub(r"\([^)]*\)", " ", value)
    value = re.sub(r"[^a-z0-9]+", " ", value)
    return re.sub(r"\s+", " ", value).strip()


def extract_module_from_title(title: str) -> str | None:
    module_match = MODULE_PREFIX_RE.match(title)
    if module_match:
        module = module_match.group(2).split("/")[0].strip().lower()
        return module
    bracket_match = BRACKET_MODULE_RE.match(title)
    if bracket_match:
        return bracket_match.group(1).split("/")[0].strip().lower()
    return None


def iter_roadmaps() -> list[Path]:
    return sorted(WORKSPACE.glob("src/**/ROADMAP.md"))


def parse_roadmaps() -> tuple[dict[int, RoadmapEntry], list[RoadmapEntry], dict[str, set[str]]]:
    by_issue: dict[int, RoadmapEntry] = {}
    entries: list[RoadmapEntry] = []
    module_versions: dict[str, set[str]] = {}

    for roadmap in iter_roadmaps():
        module = roadmap.parent.name.lower()
        current_phase_version: str | None = None
        lines = roadmap.read_text(encoding="utf-8", errors="replace").splitlines()

        for line in lines:
            if line.startswith("### "):
                current_phase_version = normalize_version(line)

            issue_match = ISSUE_RE.search(line)
            target_match = TARGET_RE.search(line)
            target_label = target_match.group(1).strip() if target_match else None
            target_version = normalize_version(target_label) or current_phase_version

            if target_version:
                module_versions.setdefault(module, set()).add(target_version)

            if not issue_match:
                continue

            entry = RoadmapEntry(
                module=module,
                issue_number=int(issue_match.group(1)),
                text=line.strip(),
                target_label=target_label,
                target_version=target_version,
                source=str(roadmap.relative_to(WORKSPACE)),
            )
            entries.append(entry)
            by_issue[entry.issue_number] = entry

    return by_issue, entries, module_versions


def iter_missing_implementation_files() -> list[Path]:
    return sorted(WORKSPACE.glob("docs/de/**/missing-implementations.json"))


def parse_doc_suggestions() -> list[DocSuggestion]:
    suggestions: list[DocSuggestion] = []
    for json_file in iter_missing_implementation_files():
        try:
            payload = json.loads(json_file.read_text(encoding="utf-8", errors="replace"))
        except json.JSONDecodeError:
            continue

        module = str(payload.get("module") or json_file.parent.name).lower()
        items = payload.get("entries") or payload.get("findings") or []
        if not isinstance(items, list):
            continue

        for item in items:
            if not isinstance(item, dict):
                continue
            title = item.get("issue_title_suggestion")
            status = str(item.get("status") or "open").lower()
            if not title or status == "resolved":
                continue
            suggestions.append(
                DocSuggestion(
                    module=module,
                    title=title,
                    target_version=normalize_version(title),
                    source=str(json_file.relative_to(WORKSPACE)),
                )
            )
    return suggestions


def gh_issue_list(state: str, limit: int | None) -> list[dict]:
    gh_state = "all" if state == "all" else state
    gh_limit = limit if limit is not None else 10000
    cmd = [
        "gh",
        "issue",
        "list",
        "--repo",
        "makr-code/ThemisDB",
        "--state",
        gh_state,
        "--limit",
        str(gh_limit),
        "--json",
        "number,title,milestone,state",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=WORKSPACE)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "gh issue list failed")

    rows = json.loads(result.stdout or "[]")
    issues: list[dict] = []
    for row in rows:
        milestone = row.get("milestone") or {}
        issues.append(
            {
                "number": row.get("number"),
                "title": row.get("title"),
                "milestone": milestone.get("title"),
                "state": str(row.get("state") or "").lower(),
            }
        )
    return issues


def best_similarity(title: str, candidates: list[str]) -> tuple[float, str | None]:
    normalized = normalize_title(title)
    best_score = 0.0
    best_candidate = None
    for candidate in candidates:
        score = difflib.SequenceMatcher(None, normalized, normalize_title(candidate)).ratio()
        if score > best_score:
            best_score = score
            best_candidate = candidate
    return best_score, best_candidate


def choose_suggestion(
    issue: dict,
    roadmap_by_issue: dict[int, RoadmapEntry],
    roadmap_entries: list[RoadmapEntry],
    doc_suggestions: list[DocSuggestion],
    module_versions: dict[str, set[str]],
) -> dict | None:
    issue_number = int(issue["number"])
    issue_title = str(issue["title"])
    current_milestone = issue.get("milestone")

    direct = roadmap_by_issue.get(issue_number)
    if direct and direct.target_version:
        return {
            "issue": issue_number,
            "title": issue_title,
            "current_milestone": current_milestone,
            "suggested_milestone": direct.target_version,
            "confidence": "high",
            "reason": f"explicit roadmap mapping in {direct.source}",
            "source": direct.source,
        }

    module = extract_module_from_title(issue_title)
    module_doc_candidates = [item for item in doc_suggestions if item.module == module] if module else []
    module_roadmap_candidates = [item for item in roadmap_entries if item.module == module and item.target_version] if module else []

    best_doc_score = 0.0
    best_doc: DocSuggestion | None = None
    for candidate in module_doc_candidates:
        score = difflib.SequenceMatcher(None, normalize_title(issue_title), normalize_title(candidate.title)).ratio()
        if score > best_doc_score:
            best_doc_score = score
            best_doc = candidate

    if best_doc and best_doc_score >= 0.72 and best_doc.target_version:
        return {
            "issue": issue_number,
            "title": issue_title,
            "current_milestone": current_milestone,
            "suggested_milestone": best_doc.target_version,
            "confidence": "high",
            "reason": f"docs/de missing-implementations title match ({best_doc_score:.2f})",
            "source": best_doc.source,
        }

    # A strong documentation match without an explicit version is useful for
    # module classification, but not strong enough for milestone reassignment.
    if best_doc and best_doc_score >= 0.85 and not best_doc.target_version:
        return None

    best_roadmap_score = 0.0
    best_roadmap: RoadmapEntry | None = None
    for candidate in module_roadmap_candidates:
        score = difflib.SequenceMatcher(None, normalize_title(issue_title), normalize_title(candidate.text)).ratio()
        if score > best_roadmap_score:
            best_roadmap_score = score
            best_roadmap = candidate

    if best_roadmap and best_roadmap_score >= 0.68 and best_roadmap.target_version:
        return {
            "issue": issue_number,
            "title": issue_title,
            "current_milestone": current_milestone,
            "suggested_milestone": best_roadmap.target_version,
            "confidence": "medium",
            "reason": f"roadmap text similarity ({best_roadmap_score:.2f}) in module {best_roadmap.module}",
            "source": best_roadmap.source,
        }

    if module and issue.get("milestone") is None and len(module_versions.get(module, set())) == 1:
        only_version = sorted(module_versions[module])[0]
        return {
            "issue": issue_number,
            "title": issue_title,
            "current_milestone": current_milestone,
            "suggested_milestone": only_version,
            "confidence": "low",
            "reason": f"single known version target for module {module}",
            "source": "roadmap module aggregate",
        }

    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state", default="open", choices=["open", "closed", "all"])
    parser.add_argument("--limit", type=int, default=None)
    parser.add_argument("--only-mismatches", action="store_true", help="show only issues where current milestone differs")
    parser.add_argument("--json", action="store_true", help="emit JSON instead of a text table")
    args = parser.parse_args()

    roadmap_by_issue, roadmap_entries, module_versions = parse_roadmaps()
    doc_suggestions = parse_doc_suggestions()
    issues = gh_issue_list(args.state, args.limit)

    suggestions: list[dict] = []
    for issue in issues:
        suggestion = choose_suggestion(issue, roadmap_by_issue, roadmap_entries, doc_suggestions, module_versions)
        if not suggestion:
            continue
        if args.only_mismatches and suggestion["current_milestone"] == suggestion["suggested_milestone"]:
            continue
        suggestions.append(suggestion)

    if args.json:
        print(json.dumps({
            "issues_scanned": len(issues),
            "suggestions": suggestions,
        }, indent=2))
        return 0

    print(f"Issues scanned: {len(issues)}")
    print(f"Suggestions:   {len(suggestions)}")
    print()
    for item in suggestions[:100]:
        print(
            f"#{item['issue']}: {item['current_milestone'] or '-'} -> {item['suggested_milestone']} "
            f"[{item['confidence']}] | {item['reason']}"
        )
        print(f"  {item['title']}")
        print(f"  source: {item['source']}")
    if len(suggestions) > 100:
        print()
        print(f"... {len(suggestions) - 100} more suggestions omitted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())