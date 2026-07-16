#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


REPO = "makr-code/ThemisDB"
ROOT = Path(__file__).resolve().parents[1]
AI_WORKING = ROOT / "ai_working"
SUMMARY_PATH = AI_WORKING / "gap_scan_pipeline_v3_summary.json"
WAVE1_TICKETS_PATH = AI_WORKING / "WAVE1_SERVER_LLM_TICKETS_2026-05-25.json"

CATEGORY_KEY_MAP = {
    "SECURITY": "security",
    "MEMORY": "memory",
    "RELIABILITY": "reliability",
    "CONCURRENCY": "concurrency",
    "RAII": "raii",
    "CONTAINER": "container",
    "PLATFORM": "platform",
    "PERFORMANCE": "performance",
    "TYPE CONVERSION": "type_conversion",
    "INPUT VALIDATION": "input_validation",
    "EXCEPTION SAFETY": "exception_safety",
    "UNINITIALIZED": "uninitialized",
    "OOP DESIGN": "oop_design",
}

CATEGORY_PRETTY = {
    "security": "Security",
    "memory": "Memory",
    "reliability": "Reliability",
    "concurrency": "Concurrency",
    "raii": "RAII",
    "container": "Container",
    "platform": "Platform",
    "performance": "Performance",
    "type_conversion": "Type Conversion",
    "input_validation": "Input Validation",
    "exception_safety": "Exception Safety",
    "uninitialized": "Uninitialized State",
    "oop_design": "OOP Design",
    "audit_logging": "Audit Logging",
    "performance_patterns": "Performance Patterns",
    "distributed_consistency": "Distributed Consistency",
    "llm_ai_safety": "LLM/AI Safety",
    "observability": "Observability",
    "determinism": "Determinism",
    "legacy_duplication": "Legacy Duplication",
}


def gh_json(args: list[str]) -> Any:
    result = subprocess.run(
        ["gh", *args],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=True,
    )
    return json.loads(result.stdout)


def gh_edit_issue(number: int, body: str) -> None:
    temp_path: Path | None = None
    try:
        payload = {"body": body}
        with tempfile.NamedTemporaryFile("w", delete=False, encoding="utf-8", newline="\n", suffix=".json") as handle:
            json.dump(payload, handle, ensure_ascii=False)
            temp_path = Path(handle.name)

        subprocess.run(
            ["gh", "api", f"repos/{REPO}/issues/{number}", "-X", "PATCH", "--input", str(temp_path)],
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    finally:
        if temp_path and temp_path.exists():
            temp_path.unlink()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def load_wave1_tickets() -> dict[str, dict[str, Any]]:
    if not WAVE1_TICKETS_PATH.exists():
        return {}
    data = load_json(WAVE1_TICKETS_PATH)
    tickets = data.get("tickets", []) if isinstance(data, dict) else []
    return {
        str(ticket.get("id", "")).strip(): ticket
        for ticket in tickets
        if isinstance(ticket, dict) and ticket.get("id")
    }


def normalize_module_name(raw: str) -> str:
    value = raw.strip()
    value = re.sub(r"\s+", "_", value)
    return value.lower()


def extract_legacy_block(body: str, title: str) -> str:
    lines = body.splitlines()
    for index, line in enumerate(lines):
        if line.strip() != "## Legacy Baseline Note (2026-05-31)":
            continue

        block = [lines[index].strip()]
        for next_line in lines[index + 1 :]:
            stripped = next_line.strip()
            if not stripped:
                continue
            if stripped.startswith("## "):
                break
            if stripped.startswith("- "):
                block.append(stripped)
                continue
            break

        if len(block) > 1:
            return "\n".join(block)

    return "\n".join(
        [
            "## Legacy Baseline Note (2026-05-31)",
            f"- Legacy issue title/body baseline: {title}",
            "- Earlier scanner-wave counts and descriptions are historical snapshots and not the current canonical v3 metric.",
            "- The sections below replace generic text with a concrete, checkable v3 remediation plan.",
        ]
    )


def classify_issue(issue: dict[str, Any]) -> tuple[str, str | None]:
    title = issue["title"]
    if issue["number"] == 5231:
        return ("skip", None)
    if title.startswith("[Wave1]"):
        return ("wave1", None)
    if title.startswith("[PHASE 1-5] Gap Scanner Analysis"):
        return ("phase_meta", None)
    module_match = re.match(r"^\[P0-CRITICAL\]\s+(.+?)\s+Module\b", title)
    if module_match:
        return ("module", normalize_module_name(module_match.group(1)))
    module_match = re.match(r"^Gap Remediation(?: Phase 1-10:|:)\s+(.+?)(?:\s+\(|$)", title)
    if module_match:
        return ("module", normalize_module_name(module_match.group(1)))
    category_match = re.match(
        r"^\[(SECURITY|MEMORY|RELIABILITY|CONCURRENCY|RAII|CONTAINER|PLATFORM|PERFORMANCE|TYPE CONVERSION|INPUT VALIDATION|EXCEPTION SAFETY|UNINITIALIZED|OOP DESIGN)\]",
        title,
    )
    if category_match:
        return ("category", CATEGORY_KEY_MAP[category_match.group(1)])
    return ("skip", None)


def load_module_report(module: str) -> dict[str, Any] | None:
    path = AI_WORKING / f"gap_scan_v3_{module}.json"
    if not path.exists():
        return None
    data = load_json(path)
    return data.get(module)


def top_categories(report: dict[str, Any], limit: int = 5) -> list[tuple[str, int]]:
    categories = report.get("by_category") or report.get("by_type") or {}
    return sorted(categories.items(), key=lambda item: (-item[1], item[0]))[:limit]


def summarize_file_findings(entries: list[dict[str, Any]]) -> tuple[int, int, int, list[str]]:
    critical = sum(1 for item in entries if item.get("severity") == "CRITICAL")
    high = sum(1 for item in entries if item.get("severity") == "HIGH")
    medium = sum(1 for item in entries if item.get("severity") == "MEDIUM")
    lines: list[str] = []
    for item in entries:
        if len(lines) >= 3:
            break
        line = item.get("line")
        category = item.get("category") or item.get("type") or "unknown"
        desc = item.get("description") or item.get("pattern") or "finding"
        lines.append(f"L{line}: {category} - {desc}")
    return critical, high, medium, lines


def top_files(report: dict[str, Any], limit: int = 5) -> list[dict[str, Any]]:
    file_map = report.get("by_file") or report.get("gaps_by_file") or {}
    ranked: list[dict[str, Any]] = []
    for file_path, entries in file_map.items():
        critical, high, medium, lines = summarize_file_findings(entries)
        ranked.append(
            {
                "file": str(file_path).replace("\\", "/"),
                "total": len(entries),
                "critical": critical,
                "high": high,
                "medium": medium,
                "examples": lines,
            }
        )
    ranked.sort(key=lambda item: (-item["total"], -item["critical"], item["file"]))
    return ranked[:limit]


def build_module_body(issue: dict[str, Any], module: str, report: dict[str, Any]) -> str:
    legacy = extract_legacy_block(issue.get("body") or "", issue["title"])
    categories = top_categories(report)
    files = top_files(report)
    report_path = f"ai_working/gap_scan_v3_{module}.json"
    module_gap_path = f"ai_working/module_gaps/{module}_GAPS.md"
    top_slice_lines = []
    for item in files[:5]:
        examples = "; ".join(item["examples"][:2]) if item["examples"] else "no example lines extracted"
        top_slice_lines.append(
            f"- [ ] {item['file']} ({item['total']} findings; C:{item['critical']} H:{item['high']} M:{item['medium']}) - {examples}"
        )
    category_lines = []
    for key, count in categories:
        category_lines.append(f"- {CATEGORY_PRETTY.get(key, key)}: {count}")
    phase1_checks = []
    for key, count in categories[:3]:
        phase1_checks.append(f"- [ ] Validate {CATEGORY_PRETTY.get(key, key)} findings ({count}) against source context and false-positive risk")
    phase2_checks = []
    for item in files[:3]:
        phase2_checks.append(f"- [ ] Create one implementation slice for {item['file']}")
    doc_candidates = [
        f"src/{module}/ROADMAP.md",
        f"src/{module}/FUTURE_ENHANCEMENTS.md",
        f"src/{module}/AUDIT.md",
        f"src/{module}/SECURITY.md",
    ]
    return f"""{legacy}

## Current Status
- Module: {module}
- Current v3 findings: total={report['total']}, critical={report['severity_critical']}, high={report['severity_high']}, medium={report['severity_medium']}
- Canonical scanner source: {report_path}
- Human-readable module note: {module_gap_path}

## Scope Snapshot

### Top Categories
{chr(10).join(category_lines) if category_lines else '- No category breakdown available'}

### Abhakbare Remediation-Slices
{chr(10).join(top_slice_lines) if top_slice_lines else '- [ ] No concrete file slices available; refresh scanner artifacts first'}

## Phase 0: Pre-Start Validation & Planning
- [ ] Confirm {report_path} exists and matches this issue scope
- [ ] Confirm build preset `windows-release` is usable for touched files
- [ ] Confirm focused test target(s) for {module} are identifiable before code changes
- [ ] Record blockers in the issue before implementation if source/test ownership is unclear

## Phase 1: Code Audit & Gap Discovery
{chr(10).join(phase1_checks) if phase1_checks else '- [ ] Validate scanner output against source files'}
- [ ] Verify top-file hotspots still reproduce in current `develop`
- [ ] Mark obviously stale findings as deferred/false-positive candidates instead of silently ignoring them

## Phase 2: Implementation Planning
{chr(10).join(phase2_checks) if phase2_checks else '- [ ] Break remediation into atomic file/category slices'}
- [ ] Order slices by `critical` first, then `high`, then medium cleanup
- [ ] Document shared refactorings before touching multiple files with the same pattern

## Phase 3: Code Implementation
- [ ] Fix all agreed `critical` slices first
- [ ] For each touched file, add or adjust targeted regression tests
- [ ] Re-run focused build/tests after each slice instead of batching the whole module blindly
- [ ] Keep issue checkboxes aligned with file-level progress and defer explicitly when a finding is out of scope

## Phase 4: Automated Review & Testing
- [ ] Build touched targets with `cmake --build --preset windows-release --parallel 16`
- [ ] Run the narrowest available tests for {module}
- [ ] Verify no new compiler warnings or static-analysis regressions were introduced by the remediation slice

## Phase 5: Human Code Review & Sign-Off
- [ ] Review whether the remediation changed API/ABI or concurrency semantics
- [ ] Review whether remaining `high` findings are real deferrals, not hidden leftovers
- [ ] Confirm code-review notes are reflected back into this issue checklist

## Phase 6: Documentation & Governance Sync
- [ ] Update affected module governance docs if remediation changed status or priorities
- [ ] Sync `MODULE_GAPS.md` / audit notes if findings were materially reduced or reclassified
- [ ] Add changelog or release notes only if user-visible behavior changed

Suggested docs to inspect:
{chr(10).join(f'- {path}' for path in doc_candidates)}

## Phase 7: Merge & Close-Out
- [ ] Rebase onto current `develop`
- [ ] Re-run final focused validation
- [ ] Post a short delta summary in this issue: fixed files, deferred files, test evidence
- [ ] Close only when critical scope is either fixed or explicitly split into follow-up issues

## Production Readiness Checklist
- [ ] Critical findings for this issue scope are resolved or split into explicit follow-ups
- [ ] High findings are triaged with rationale
- [ ] Focused tests pass for each touched slice
- [ ] Documentation/governance notes updated where required
- [ ] The issue body remains a truthful checklist of remaining work
"""


def build_phase_meta_body(issue: dict[str, Any], summary: dict[str, Any]) -> str:
    legacy = extract_legacy_block(issue.get("body") or "", issue["title"])
    scanner = summary["scanner_summary"]
    top_modules = "\n".join(
        f"- [ ] {item['module']}: {item['gaps']} findings; keep module issue in sync with current remediation state"
        for item in scanner.get("top_modules", [])[:10]
    )
    top_categories = sorted(
        scanner.get("by_category", {}).items(), key=lambda item: (-item[1], item[0])
    )[:8]
    top_category_lines = "\n".join(
        f"- {CATEGORY_PRETTY.get(key, key)}: {count}" for key, count in top_categories
    )
    return f"""{legacy}

## Current Status
- Canonical v3 baseline: total={summary['total_gaps']}, critical={summary['by_severity']['critical']}, high={summary['by_severity']['high']}, medium={summary['by_severity']['medium']}
- Scanner scope: {scanner['phase']}
- Modules scanned: {scanner['modules_scanned']}
- Canonical source: ai_working/gap_scan_pipeline_v3_summary.json

## Scope Snapshot

### Top Modules
{top_modules}

### Top Categories
{top_category_lines}

## Phase 0: Pre-Start Validation & Planning
- [ ] Confirm v3 baseline artifacts are current before using this issue for planning
- [ ] Confirm duplicate strategic trackers stay closed and all new updates land here
- [ ] Confirm module issue set is complete for the current scan scope

## Phase 1: Code Audit & Gap Discovery
- [ ] Reconfirm top-module ordering from the current v3 summary
- [ ] Spot-check representative modules for scanner drift or false-positive clusters
- [ ] Reconcile category totals vs. module totals before reprioritization

## Phase 2: Implementation Planning
- [ ] Convert the top-module list into a sequenced remediation queue
- [ ] Split cross-module categories into dedicated execution waves where needed
- [ ] Track which module issues are active, blocked, deferred, or ready to close

## Phase 3: Code Implementation
- [ ] Drive remediation through the linked module issues, not in this meta issue directly
- [ ] Update this tracker whenever a module issue materially changes the global backlog
- [ ] Keep closure criteria tied to actual v3 deltas, not legacy scanner counts

## Phase 4: Automated Review & Testing
- [ ] Re-run the v3 pipeline at agreed checkpoints
- [ ] Publish delta vs. previous baseline in this issue
- [ ] Call out regressions where total critical/high findings increased

## Phase 5: Human Code Review & Sign-Off
- [ ] Review whether module prioritization still matches engineering risk
- [ ] Review deferred findings that remain open across multiple waves
- [ ] Confirm closed issues have enough evidence to stay closed

## Phase 6: Documentation & Governance Sync
- [ ] Keep roadmap/audit/security docs aligned with the current v3 backlog
- [ ] Keep issue labels/milestones aligned with actual remediation phase
- [ ] Record notable baseline changes in tracker comments or linked docs

## Phase 7: Merge & Close-Out
- [ ] Close only when a newer canonical strategic tracker replaces this one
- [ ] Otherwise keep this issue open as the single phase-level coordination issue

## Production Readiness Checklist
- [ ] Module issue coverage matches the current v3 scan universe
- [ ] Top-module prioritization reviewed against current baseline
- [ ] Strategic tracker comments reflect actual scan deltas
- [ ] Duplicate strategic trackers remain closed
"""


def build_category_index() -> tuple[dict[str, list[tuple[str, int]]], dict[str, list[dict[str, Any]]]]:
    modules_by_category: dict[str, list[tuple[str, int]]] = defaultdict(list)
    files_by_category: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for path in AI_WORKING.glob("gap_scan_v3_*.json"):
        if path.name.endswith("_aggregate.json") or path.name in {
            "gap_scan_v3_summary.json",
            "gap_scan_v3_summary_before_fp_tuning.json",
            "gap_scan_v3_baseline_before_rescan_2026-05-25.json",
            "gap_scan_v3_aggregate.json",
            "gap_scan_v3_aggregate_before_fp_tuning.json",
            "gap_scan_pipeline_v3_summary.json",
        }:
            continue
        data = load_json(path)
        if isinstance(data, dict) and all(isinstance(value, dict) for value in data.values()):
            items = data.items()
        elif isinstance(data, dict):
            items = [(path.stem.replace("gap_scan_v3_", ""), data)]
        else:
            continue

        for module, report in items:
            if not isinstance(report, dict):
                continue
            categories = report.get("by_category") or report.get("by_type") or {}
            for key, count in categories.items():
                if count:
                    modules_by_category[key].append((module, count))

            file_map = report.get("by_file") or report.get("gaps_by_file") or {}
            for file_path, entries in file_map.items():
                counter = Counter((entry.get("category") or entry.get("type") or "unknown") for entry in entries)
                for key, count in counter.items():
                    examples = []
                    for entry in entries:
                        entry_key = entry.get("category") or entry.get("type") or "unknown"
                        if entry_key != key:
                            continue
                        examples.append(
                            f"L{entry.get('line')}: {entry.get('description') or entry.get('pattern') or 'finding'}"
                        )
                        if len(examples) >= 2:
                            break
                    files_by_category[key].append(
                        {
                            "module": module,
                            "file": str(file_path).replace("\\", "/"),
                            "count": count,
                            "examples": examples,
                        }
                    )
    for key in modules_by_category:
        modules_by_category[key].sort(key=lambda item: (-item[1], item[0]))
    for key in files_by_category:
        files_by_category[key].sort(key=lambda item: (-item["count"], item["file"]))
    return modules_by_category, files_by_category


def build_category_body(
    issue: dict[str, Any],
    category_key: str,
    summary: dict[str, Any],
    modules_by_category: dict[str, list[tuple[str, int]]],
    files_by_category: dict[str, list[dict[str, Any]]],
) -> str:
    legacy = extract_legacy_block(issue.get("body") or "", issue["title"])
    scanner = summary["scanner_summary"]
    total = scanner.get("by_category", {}).get(category_key, 0)
    top_modules = modules_by_category.get(category_key, [])[:8]
    top_file_entries = files_by_category.get(category_key, [])[:8]
    if not top_file_entries:
        for module, _count in top_modules[:4]:
            report = load_module_report(module)
            if not report:
                continue
            for item in top_files(report, limit=2):
                top_file_entries.append(
                    {
                        "module": module,
                        "file": item["file"],
                        "count": item["total"],
                        "examples": item["examples"],
                    }
                )
    module_lines = "\n".join(f"- [ ] {module}: {count} findings" for module, count in top_modules)
    file_lines = "\n".join(
        f"- [ ] {item['file']} ({item['module']}, {item['count']} findings) - {'; '.join(item['examples']) if item['examples'] else 'review source lines'}"
        for item in top_file_entries
    )
    pretty = CATEGORY_PRETTY.get(category_key, category_key)
    return f"""{legacy}

## Current Status
- Category: {pretty}
- Current v3 total: {total}
- Canonical source: ai_working/gap_scan_pipeline_v3_summary.json
- Scope: cross-module remediation and triage, not a single-module implementation slice

## Scope Snapshot

### Top Affected Modules
{module_lines if module_lines else '- [ ] No module ranking available; refresh v3 scanner outputs'}

### Abhakbare Remediation-Slices
{file_lines if file_lines else '- [ ] No file-level slices extracted; refresh category data first'}

## Phase 0: Pre-Start Validation & Planning
- [ ] Confirm this category is still present in the current v3 summary
- [ ] Confirm linked module issues exist for the top affected modules
- [ ] Confirm remediation ownership is clear when a finding spans multiple modules

## Phase 1: Code Audit & Gap Discovery
- [ ] Validate top affected modules against the current per-module reports
- [ ] Spot-check representative file findings to separate true positives from noise
- [ ] Record recurring sub-patterns that should be fixed with one shared refactor

## Phase 2: Implementation Planning
- [ ] Split work by module and shared pattern families
- [ ] Create a first remediation wave for the highest-density modules
- [ ] Record which modules will be fixed here vs. tracked in their own module issue

## Phase 3: Code Implementation
- [ ] Remediate highest-risk file slices first
- [ ] Use one implementation slice per module/pattern cluster and check it off here
- [ ] Keep module issues and this cross-module issue synchronized when slices are moved or split

## Phase 4: Automated Review & Testing
- [ ] Rebuild only the touched targets first, then widen if needed
- [ ] Run focused tests for each affected module
- [ ] Confirm the category count trends downward on the next v3 scan

## Phase 5: Human Code Review & Sign-Off
- [ ] Review whether shared fixes introduced regressions in adjacent modules
- [ ] Review deferrals for noisy or low-confidence findings
- [ ] Confirm issue checkboxes still reflect actual remaining slices

## Phase 6: Documentation & Governance Sync
- [ ] Update affected module docs when category-driven fixes changed module status
- [ ] Record scan delta and any notable false-positive triage in comments or docs

## Phase 7: Merge & Close-Out
- [ ] Close only when the category backlog is either resolved or explicitly decomposed into active module issues
- [ ] Leave a closing summary with modules touched and delta from the previous scan

## Production Readiness Checklist
- [ ] Top modules for {pretty} have an active remediation path
- [ ] High-density files are split into checkable implementation slices
- [ ] Focused tests exist for touched modules
- [ ] v3 rescan shows a real reduction or a documented reclassification
"""


def parse_wave1_body(title: str, body: str) -> dict[str, Any]:
    result: dict[str, Any] = {"scope_files": [], "focus_types": [], "ticket_id": None, "module": None}
    ticket_match = re.search(r"Wave 1 Ticket ID:\s*(.+)", body)
    module_match = re.search(r"Modul:\s*(.+)", body)
    if ticket_match:
        result["ticket_id"] = ticket_match.group(1).strip()
    if module_match:
        result["module"] = normalize_module_name(module_match.group(1))

    if not result["ticket_id"]:
        current_ticket_match = re.search(r"- Wave1 ticket:\s*(.+)", body)
        if current_ticket_match:
            value = current_ticket_match.group(1).strip()
            if value and value.lower() != "unknown":
                result["ticket_id"] = value

    if not result["module"]:
        current_module_match = re.search(r"- Module:\s*(.+)", body)
        if current_module_match:
            value = current_module_match.group(1).strip()
            if value and value.lower() != "unknown":
                result["module"] = normalize_module_name(value)

    if not result["ticket_id"]:
        title_ticket_match = re.search(r"\[Wave1\]\[(.*?)\]", title)
        if title_ticket_match:
            result["ticket_id"] = title_ticket_match.group(1).strip()

    if not result["module"]:
        title_module_match = re.match(r"^\[Wave1\]\[[^\]]+\]\s+([^\s]+)", title)
        if title_module_match:
            result["module"] = normalize_module_name(title_module_match.group(1))

    section = None
    for raw_line in body.splitlines():
        line = raw_line.strip()
        if line.startswith("Scope-Dateien:"):
            section = "scope_files"
            continue
        if line.startswith("Fokus-Typen:"):
            section = "focus_types"
            continue
        if line == "### Scope Files":
            section = "scope_files"
            continue
        if line == "### Focus Types":
            section = "focus_types"
            continue
        if line.startswith("Akzeptanzkriterien:"):
            section = None
            continue
        if line.startswith("## "):
            section = None
            continue
        if section and line.startswith("-"):
            value = line[1:].strip()
            if value.startswith("[ ] "):
                value = value[4:].strip()
            result[section].append(value)
        elif line:
            section = None
    return result


def build_wave1_body(issue: dict[str, Any], wave1_tickets: dict[str, dict[str, Any]]) -> str:
    parsed = parse_wave1_body(issue["title"], issue.get("body") or "")
    ticket = wave1_tickets.get(parsed.get("ticket_id") or "")
    if ticket:
        if ticket.get("module"):
            parsed["module"] = normalize_module_name(ticket["module"])
        if ticket.get("files"):
            parsed["scope_files"] = list(ticket["files"])
        if ticket.get("focus_types"):
            parsed["focus_types"] = list(ticket["focus_types"])

    module = parsed.get("module") or "unknown"
    scope_files = parsed.get("scope_files") or []
    focus_types = parsed.get("focus_types") or []
    legacy = extract_legacy_block(issue.get("body") or "", issue["title"])
    report = load_module_report(module)
    top_files_for_module = top_files(report, limit=5) if report else []
    slice_lines = []
    for path in scope_files:
        matching = next((item for item in top_files_for_module if item["file"].endswith(path.replace("\\", "/"))), None)
        if matching:
            examples = "; ".join(matching["examples"][:2]) if matching["examples"] else "review focused findings"
            slice_lines.append(
                f"- [ ] {matching['file']} ({matching['total']} findings; C:{matching['critical']} H:{matching['high']} M:{matching['medium']}) - {examples}"
            )
        else:
            slice_lines.append(f"- [ ] {path} - validate the focused Wave1 findings against current source")
    if not slice_lines:
        slice_lines.append("- [ ] Recreate a focused file list before implementation")
    focus_lines = [f"- [ ] {item}" for item in focus_types] or ["- [ ] Confirm focused finding types before implementation"]
    report_path = f"ai_working/gap_scan_v3_{module}.json" if module != "unknown" else "ai_working/gap_scan_pipeline_v3_summary.json"
    return f"""{legacy}

## Current Status
- Wave1 ticket: {parsed.get('ticket_id') or 'unknown'}
- Module: {module}
- Canonical scanner source for current validation: {report_path}
- Purpose: focused remediation of one narrow server/llm slice instead of whole-module cleanup

## Scope Snapshot

### Scope Files
{chr(10).join(slice_lines)}

### Focus Types
{chr(10).join(focus_lines)}

## Phase 0: Pre-Start Validation & Planning
- [ ] Confirm the Wave1 scope file(s) still exist and match the current codebase
- [ ] Confirm focused tests or build targets for the scope file(s)
- [ ] Confirm this issue is still narrow enough to finish without reopening a whole-module backlog

## Phase 1: Code Audit & Gap Discovery
- [ ] Validate the listed focus types against the current scanner output for the scope file(s)
- [ ] Capture exact line-level findings before editing
- [ ] Split unrelated findings out instead of widening this issue silently

## Phase 2: Implementation Planning
- [ ] Turn each scope file into one atomic implementation slice
- [ ] Decide which fixes need tests, guards, or doc updates before touching code
- [ ] Record explicit deferrals for findings outside the Wave1 scope

## Phase 3: Code Implementation
- [ ] Implement the focused guard/fix set for each scope file
- [ ] Add or update targeted tests for the changed path
- [ ] Re-run the narrowest build/test after each file-level slice

## Phase 4: Automated Review & Testing
- [ ] Build touched targets successfully
- [ ] Run focused tests for the affected endpoint/runtime path
- [ ] Confirm the focused scanner delta is reduced or justified

## Phase 5: Human Code Review & Sign-Off
- [ ] Review that the fix stayed within the declared Wave1 scope
- [ ] Review whether any remaining file-local findings need a follow-up issue

## Phase 6: Documentation & Governance Sync
- [ ] Update issue comments with focused delta and evidence
- [ ] Update module docs only if this narrow fix changes status or behavior materially

## Phase 7: Merge & Close-Out
- [ ] Leave a closing note with touched files, tests, and remaining deferrals
- [ ] Close when the focused slice is complete, not when the whole module is clean

## Production Readiness Checklist
- [ ] Scope files remediated or split into follow-up tickets
- [ ] Focused tests pass
- [ ] Delta documented in the issue
- [ ] Remaining non-scope findings are explicitly deferred
"""


def load_issues() -> list[dict[str, Any]]:
    return gh_json(["issue", "list", "--repo", REPO, "--state", "all", "--limit", "1000", "--json", "number,title,body,state"])


def select_issues(issues: list[dict[str, Any]], only: set[int] | None) -> list[dict[str, Any]]:
    selected = []
    for issue in issues:
        if only and issue["number"] not in only:
            continue
        kind, _ = classify_issue(issue)
        if kind != "skip":
            selected.append(issue)
    return sorted(selected, key=lambda item: item["number"])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", help="comma-separated issue numbers")
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()

    only = {int(part.strip()) for part in args.only.split(",")} if args.only else None
    summary = load_json(SUMMARY_PATH)
    wave1_tickets = load_wave1_tickets()
    modules_by_category, files_by_category = build_category_index()
    issues = select_issues(load_issues(), only)

    for issue in issues:
        kind, scope = classify_issue(issue)
        if kind == "phase_meta":
            body = build_phase_meta_body(issue, summary)
        elif kind == "module":
            report = load_module_report(scope or "")
            if not report:
                print(f"SKIP #{issue['number']}: missing module report for {scope}")
                continue
            body = build_module_body(issue, scope or "unknown", report)
        elif kind == "category":
            body = build_category_body(issue, scope or "", summary, modules_by_category, files_by_category)
        elif kind == "wave1":
            body = build_wave1_body(issue, wave1_tickets)
        else:
            continue

        if args.apply:
            gh_edit_issue(issue["number"], body)
            print(f"UPDATED #{issue['number']} {issue['title']}")
        else:
            print(f"===== ISSUE #{issue['number']} =====")
            print(body[:2500])
            print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())