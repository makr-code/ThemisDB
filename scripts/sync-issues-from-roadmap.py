"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sync-issues-from-roadmap.py                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:07:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1174                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8452353dc5  2026-03-12  Add unit tests for sync-issues-from-roadmap.py ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Synchronise GitHub issues from the consolidated src/ROADMAP.md backlog.

Modes:
  - preview: parse roadmap items, resolve detail sections, generate issue bodies
    and preview reports without touching GitHub
  - apply: create missing issues via `gh issue create`, detect duplicates, and
    optionally backfill issue references into src/ROADMAP.md
  - backfill: update src/ROADMAP.md from a prior apply manifest
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_DEFAULT = "makr-code/ThemisDB"
REQUIRED_COLUMNS = ["#", "Module", "Title", "Target", "Labels", "Detail"]
ISSUE_COLUMN = "Issue"
SOURCE_KEY_PREFIX = "roadmap-source-key:"
ROADMAP_REF_PREFIX = "roadmap-ref:"
DETAIL_REF_PREFIX = "roadmap-detail:"
GOVERNANCE_AREAS = {
    "acceleration",
    "analytics",
    "api",
    "aql",
    "auth",
    "cache",
    "cdc",
    "chimera",
    "config",
    "content",
    "core",
    "docs",
    "exporters",
    "geo",
    "governance",
    "gpu",
    "graph",
    "importers",
    "index",
    "ingestion",
    "llm",
    "metadata",
    "network",
    "observability",
    "performance",
    "plugins",
    "prompt_engineering",
    "query",
    "rag",
    "replication",
    "scheduler",
    "search",
    "security",
    "server",
    "sharding",
    "storage",
    "temporal",
    "timeseries",
    "transaction",
    "training",
    "updates",
    "utils",
    "voice",
    "ci",
    "infra",
}
GOVERNANCE_TYPES = {"feature", "bug", "test", "documentation", "refactor", "chore", "security", "performance"}
GOVERNANCE_PRIORITIES = {"critical", "high", "medium", "low"}
GOVERNANCE_STATUSES = {"open", "in_progress", "blocked", "ready", "review"}
AREA_ALIASES = {
    "base": "core",
    "maintenance": "governance",
    "themis": "core",
}
LABEL_TO_AREA = {
    "build": "ci",
    "ci": "ci",
    "compliance": "governance",
    "doc": "docs",
    "docs": "docs",
    "documentation": "docs",
    "infra": "infra",
    "maintenance": "governance",
    "observability": "observability",
    "perf": "performance",
    "performance": "performance",
    "query": "query",
    "search": "search",
    "security": "security",
    "storage": "storage",
    "training": "training",
}

CHECKBOX_RE = re.compile(r"^\s*[-*]\s+\[[ xX~!?pIP]\]\s+(.*)$")
LIST_ITEM_RE = re.compile(r"^\s*(?:[-*]|\d+\.)\s+(.*)$")
HEADING_RE = re.compile(r"^(#{2,6})\s+(.*\S)\s*$")
BODY_SOURCE_KEY_RE = re.compile(r"roadmap-source-key:\s*(\S+)")
ISSUE_REF_RE = re.compile(r"#(\d+)")
URL_ISSUE_REF_RE = re.compile(r"/issues/(\d+)")


@dataclass
class TableRow:
    line_index: int
    cells: dict[str, str]


@dataclass
class TableRegion:
    start_line: int
    end_line: int
    header_line: int
    separator_line: int
    priority_heading: str
    headers: list[str]
    rows: list[TableRow]


@dataclass
class RoadmapItem:
    row_number: str
    module: str
    title: str
    target: str
    labels: list[str]
    detail_link: str
    detail_path: str
    detail_anchor: str
    priority: str
    priority_heading: str
    issue_ref: int | None
    source_key: str
    line_index: int
    table_index: int


def split_markdown_row(line: str) -> list[str]:
    stripped = line.strip()
    if not stripped.startswith("|"):
        return []
    parts = stripped.strip("|").split("|")
    return [part.strip() for part in parts]


def is_separator_row(cells: list[str]) -> bool:
    if not cells:
        return False
    return all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells)


def parse_markdown_link(cell: str) -> str:
    match = re.search(r"\[[^\]]+\]\(([^)]+)\)", cell)
    return match.group(1).strip() if match else cell.strip()


def parse_issue_reference(cell: str) -> int | None:
    if not cell.strip():
        return None
    match = ISSUE_REF_RE.search(cell)
    if match:
        return int(match.group(1))
    match = URL_ISSUE_REF_RE.search(cell)
    if match:
        return int(match.group(1))
    return None


def normalise_text(text: str) -> str:
    lowered = text.lower()
    lowered = re.sub(r"`+", "", lowered)
    lowered = re.sub(r"[^a-z0-9]+", "-", lowered)
    lowered = re.sub(r"-+", "-", lowered).strip("-")
    return lowered


def canonical_anchor(anchor: str) -> str:
    return normalise_text(anchor.lstrip("#"))


def slugify_heading(text: str) -> str:
    slug = text.strip().lower()
    slug = slug.replace("&", " and ")
    slug = slug.replace("/", " ")
    slug = slug.replace("·", " ")
    slug = slug.replace("—", " ")
    slug = slug.replace("–", " ")
    slug = re.sub(r"[`'\".:;!?()\[\]{}<>]", "", slug)
    slug = re.sub(r"\s+", "-", slug)
    return slug.strip("-")


def extract_labels(raw: str) -> list[str]:
    labels = re.findall(r"`([^`]+)`", raw)
    if not labels:
        labels = [part.strip().strip("`") for part in raw.split(",") if part.strip()]
    return dedupe(labels)


def dedupe(values: list[str]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []
    for value in values:
        if value not in seen:
            seen.add(value)
            ordered.append(value)
    return ordered


def normalise_label_value(label: str) -> str:
    return label.strip().strip("`").lower()


def infer_priority(heading: str) -> str:
    lowered = heading.lower()
    if "critical" in lowered:
        return "critical"
    if "high" in lowered:
        return "high"
    if "medium" in lowered:
        return "medium"
    if "low" in lowered:
        return "low"
    return "unspecified"


def build_source_key(row_number: str, module: str, target: str, detail_anchor: str) -> str:
    return f"roadmap:{row_number}:{module}:{target}:{canonical_anchor(detail_anchor)}"


def parse_roadmap_document(roadmap_path: Path) -> tuple[list[RoadmapItem], list[TableRegion]]:
    lines = roadmap_path.read_text(encoding="utf-8").splitlines()
    items: list[RoadmapItem] = []
    tables: list[TableRegion] = []
    current_heading = "Unknown Priority"
    index = 0

    while index < len(lines):
        line = lines[index]
        heading_match = re.match(r"^##\s+(.*\S)\s*$", line)
        if heading_match:
            current_heading = heading_match.group(1).strip()

        if index + 1 < len(lines) and line.lstrip().startswith("|"):
            header_cells = split_markdown_row(line)
            separator_cells = split_markdown_row(lines[index + 1])
            if (
                header_cells
                and separator_cells
                and is_separator_row(separator_cells)
                and all(column in header_cells for column in REQUIRED_COLUMNS)
            ):
                rows: list[TableRow] = []
                row_index = index + 2
                while row_index < len(lines) and lines[row_index].lstrip().startswith("|"):
                    row_cells = split_markdown_row(lines[row_index])
                    if row_cells and not is_separator_row(row_cells):
                        cell_map = {
                            header_cells[position]: row_cells[position] if position < len(row_cells) else ""
                            for position in range(len(header_cells))
                        }
                        rows.append(TableRow(line_index=row_index, cells=cell_map))
                    row_index += 1

                table_index = len(tables)
                tables.append(
                    TableRegion(
                        start_line=index,
                        end_line=row_index - 1,
                        header_line=index,
                        separator_line=index + 1,
                        priority_heading=current_heading,
                        headers=header_cells,
                        rows=rows,
                    )
                )

                for row in rows:
                    detail_link = parse_markdown_link(row.cells["Detail"])
                    detail_path, _, detail_anchor = detail_link.partition("#")
                    module = row.cells["Module"].strip()
                    row_number = row.cells["#"].strip()
                    target = row.cells["Target"].strip()
                    item = RoadmapItem(
                        row_number=row_number,
                        module=module,
                        title=row.cells["Title"].strip(),
                        target=target,
                        labels=extract_labels(row.cells["Labels"]),
                        detail_link=detail_link,
                        detail_path=detail_path,
                        detail_anchor=detail_anchor,
                        priority=infer_priority(current_heading),
                        priority_heading=current_heading,
                        issue_ref=parse_issue_reference(row.cells.get(ISSUE_COLUMN, "")),
                        source_key=build_source_key(row_number, module, target, detail_anchor),
                        line_index=row.line_index,
                        table_index=table_index,
                    )
                    items.append(item)

                index = row_index
                continue
        index += 1

    return items, tables


def find_detail_section(detail_file: Path, requested_anchor: str, title: str) -> tuple[str, str]:
    lines = detail_file.read_text(encoding="utf-8").splitlines()
    headings: list[tuple[int, int, str, str]] = []
    for line_index, line in enumerate(lines):
        match = HEADING_RE.match(line)
        if not match:
            continue
        level = len(match.group(1))
        heading_text = match.group(2).strip()
        headings.append((line_index, level, heading_text, canonical_anchor(slugify_heading(heading_text))))

    target_anchor = canonical_anchor(requested_anchor)
    selected: tuple[int, int, str, str] | None = None

    for heading in headings:
        if heading[3] == target_anchor:
            selected = heading
            break

    if selected is None:
        title_norm = normalise_text(title)
        for heading in headings:
            heading_norm = normalise_text(heading[2])
            if heading_norm == title_norm or title_norm in heading_norm or heading_norm in title_norm:
                selected = heading
                break

    if selected is None:
        raise ValueError(f"No section found for anchor '{requested_anchor}' in {detail_file}")

    start_line, start_level, heading_text, _ = selected
    end_line = len(lines)
    for candidate in headings:
        if candidate[0] <= start_line:
            continue
        if candidate[1] <= start_level:
            end_line = candidate[0]
            break

    section_text = "\n".join(lines[start_line:end_line]).strip()
    return heading_text, section_text


def clean_list_item(text: str) -> str:
    cleaned = text.strip()
    cleaned = re.sub(r"^(?:`?\[[ xX~!?pIP]\]`?\s*)+", "", cleaned)
    cleaned = re.sub(r"\s+", " ", cleaned)
    return cleaned


def extract_acceptance_criteria(section_text: str) -> list[str]:
    lines = section_text.splitlines()
    checkbox_items: list[str] = []
    in_fence = False

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        checkbox_match = CHECKBOX_RE.match(line)
        if checkbox_match:
            checkbox_items.append(f"- [ ] {clean_list_item(checkbox_match.group(1))}")

    if checkbox_items:
        return dedupe(checkbox_items)

    bullet_items: list[str] = []
    in_fence = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence or not stripped:
            continue
        if stripped.startswith("|"):
            continue
        if stripped.startswith("**Priority:**") or stripped.startswith("**Target Version:**"):
            continue
        if stripped.startswith("**Files:**") or stripped.startswith("**Problem"):
            continue
        list_match = LIST_ITEM_RE.match(line)
        if list_match:
            bullet_items.append(f"- [ ] {clean_list_item(list_match.group(1))}")

    if bullet_items:
        return dedupe(bullet_items)

    return [
        "- [ ] Implement the scoped changes described in the linked detail section.",
        "- [ ] Add or update tests that verify the intended behaviour.",
    ]


def classify_issue_kind(item: RoadmapItem) -> str:
    labels = {normalise_label_value(label) for label in item.labels}
    for label in labels:
        if label.startswith("type:"):
            issue_type = label.split(":", 1)[1]
            if issue_type in GOVERNANCE_TYPES:
                return issue_type
    if "security" in labels or item.module == "security":
        return "security"
    if "performance" in labels or "perf" in labels:
        return "performance"
    if "bug" in labels or "fix" in labels or "defect" in labels:
        return "bug"
    if "test" in labels or "tests" in labels or "testing" in labels:
        return "test"
    if "documentation" in labels or "docs" in labels or "doc" in labels:
        return "documentation"
    if "refactor" in labels:
        return "refactor"
    if "maintenance" in labels or "tooling" in labels or "build" in labels or "chore" in labels:
        return "chore"
    return "feature"


def infer_area_label(item: RoadmapItem) -> str:
    labels = [normalise_label_value(label) for label in item.labels]
    for label in labels:
        if label.startswith("area:"):
            area = label.split(":", 1)[1]
            if area in GOVERNANCE_AREAS:
                return f"area:{area}"

    module = AREA_ALIASES.get(item.module.lower(), item.module.lower())
    if module in GOVERNANCE_AREAS:
        return f"area:{module}"

    for label in labels:
        candidate = label.split(":", 1)[1] if ":" in label else label
        mapped = LABEL_TO_AREA.get(candidate)
        if mapped in GOVERNANCE_AREAS:
            return f"area:{mapped}"

    return "area:core"


def infer_priority_label(item: RoadmapItem) -> str:
    labels = [normalise_label_value(label) for label in item.labels]
    for label in labels:
        if label.startswith("priority:"):
            priority = label.split(":", 1)[1]
            if priority in GOVERNANCE_PRIORITIES:
                return f"priority:{priority}"
    if item.priority in GOVERNANCE_PRIORITIES:
        return f"priority:{item.priority}"
    return "priority:medium"


def infer_status_label(item: RoadmapItem) -> str:
    labels = [normalise_label_value(label) for label in item.labels]
    for label in labels:
        if label.startswith("status:"):
            status = label.split(":", 1)[1]
            if status in GOVERNANCE_STATUSES:
                return f"status:{status}"
    return "status:open"


def build_issue_labels(item: RoadmapItem, issue_kind: str) -> list[str]:
    passthrough: list[str] = []
    for label in item.labels:
        normalised = normalise_label_value(label)
        if normalised.startswith(("area:", "module:", "priority:", "type:", "status:")):
            continue
        passthrough.append(label)

    labels = [
        infer_area_label(item),
        infer_priority_label(item),
        f"type:{issue_kind}",
        infer_status_label(item),
        *passthrough,
    ]
    return dedupe(labels)


def build_issue_body(
    item: RoadmapItem,
    section_heading: str,
    section_text: str,
    acceptance_criteria: list[str],
    issue_labels: list[str],
) -> str:
    detail_reference = f"src/{item.detail_link}"
    body_lines = [
        "### Context",
        "",
        (
            f"This issue implements the roadmap item '{item.title}' for the {item.module} domain. "
            f"It is sourced from the consolidated roadmap under {item.priority_heading} "
            f"and targets milestone {item.target}."
        ),
        "",
        f"Primary detail section: {section_heading}",
        "",
        "### Goal",
        "",
        (
            f"Deliver the scoped changes for {item.title} in src/{item.module}/ and complete the linked "
            f"detail section in a release-ready state for {item.target}."
        ),
        "",
        "### Detailed Scope",
        "",
        section_text,
        "",
        "### Acceptance Criteria",
        "",
        *acceptance_criteria,
        "",
        "### Relationships",
        "",
        f"- Roadmap row: #{item.row_number} ({item.priority_heading})",
        "- Depends on: none identified during generation.",
        "- Part of: consolidated roadmap delivery tracking.",
        "",
        "### References",
        "",
        "- src/ROADMAP.md",
        f"- {detail_reference}",
        f"- Source key: {item.source_key}",
        "",
        "Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.",
        "",
        f"<!-- {SOURCE_KEY_PREFIX} {item.source_key} -->",
        f"<!-- {ROADMAP_REF_PREFIX} row={item.row_number};module={item.module};target={item.target} -->",
        f"<!-- {DETAIL_REF_PREFIX} {detail_reference} -->",
    ]
    return "\n".join(body_lines).strip() + "\n"


def filter_items(
    items: list[RoadmapItem],
    *,
    module_filter: str | None,
    priority_filter: str | None,
    offset: int,
    limit: int | None,
) -> list[RoadmapItem]:
    filtered = items
    if module_filter:
        filtered = [item for item in filtered if item.module == module_filter]
    if priority_filter:
        filtered = [item for item in filtered if item.priority == priority_filter]
    if offset:
        filtered = filtered[offset:]
    if limit is not None:
        filtered = filtered[:limit]
    return filtered


def ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def write_preview_artifacts(
    preview_dir: Path,
    records: list[dict],
    mode: str,
) -> tuple[Path, Path]:
    ensure_directory(preview_dir)
    json_path = preview_dir / f"roadmap-issues-{mode}.json"
    md_path = preview_dir / f"roadmap-issues-{mode}.md"
    json_path.write_text(json.dumps(records, indent=2), encoding="utf-8")

    lines = [
        f"# Roadmap Issue {mode.title()} Report",
        "",
        f"Generated items: {len(records)}",
        "",
        "| Row | Module | Title | Target | Issue | Status |",
        "|-----|--------|-------|--------|-------|--------|",
    ]
    for record in records:
        issue_display = f"#{record['issue_number']}" if record.get("issue_number") else ""
        title = str(record["title"]).replace("|", "\\|")
        lines.append(
            f"| {record['row_number']} | {record['module']} | {title} | {record['target']} | {issue_display} | {record['status']} |"
        )
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return json_path, md_path


def compute_roadmap_summary(
    roadmap_path: Path,
    issue_mapping: dict[str, int] | None = None,
) -> dict:
    items, _ = parse_roadmap_document(roadmap_path)
    summary_by_priority: dict[str, dict[str, int]] = {}
    missing_rows: list[dict[str, str]] = []
    total = 0
    linked = 0

    for item in items:
        priority_heading = item.priority_heading
        bucket = summary_by_priority.setdefault(priority_heading, {"total": 0, "linked": 0, "missing": 0})
        bucket["total"] += 1
        total += 1

        issue_number = issue_mapping.get(item.source_key) if issue_mapping else None
        if issue_number is None:
            issue_number = item.issue_ref

        if issue_number is not None:
            bucket["linked"] += 1
            linked += 1
        else:
            bucket["missing"] += 1
            missing_rows.append(
                {
                    "row_number": item.row_number,
                    "module": item.module,
                    "title": item.title,
                    "target": item.target,
                    "priority_heading": item.priority_heading,
                    "detail_link": f"src/{item.detail_link}",
                    "source_key": item.source_key,
                }
            )

    return {
        "total": total,
        "linked": linked,
        "missing": total - linked,
        "by_priority": [
            {
                "priority_heading": priority_heading,
                "total": values["total"],
                "linked": values["linked"],
                "missing": values["missing"],
            }
            for priority_heading, values in summary_by_priority.items()
        ],
        "missing_rows": missing_rows,
    }


def write_summary_artifacts(preview_dir: Path, summary: dict) -> tuple[Path, Path]:
    ensure_directory(preview_dir)
    json_path = preview_dir / "roadmap-issues-summary.json"
    md_path = preview_dir / "roadmap-issues-summary.md"
    json_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    lines = [
        "# Roadmap Issue Summary",
        "",
        f"Total roadmap items: {summary['total']}",
        f"Linked roadmap items: {summary['linked']}",
        f"Missing roadmap items: {summary['missing']}",
        "",
        "## By Priority",
        "",
        "| Priority | Total | Linked | Missing |",
        "|----------|-------|--------|---------|",
    ]
    for bucket in summary["by_priority"]:
        lines.append(
            f"| {bucket['priority_heading'].replace('|', '\\|')} | {bucket['total']} | {bucket['linked']} | {bucket['missing']} |"
        )

    lines.extend(
        [
            "",
            "## Missing Rows",
            "",
        ]
    )

    if not summary["missing_rows"]:
        lines.append("None")
    else:
        lines.extend(
            [
                "| Row | Module | Title | Target | Priority | Detail |",
                "|-----|--------|-------|--------|----------|--------|",
            ]
        )
        for row in summary["missing_rows"]:
            lines.append(
                "| {row_number} | {module} | {title} | {target} | {priority_heading} | {detail_link} |".format(
                    row_number=row["row_number"],
                    module=row["module"],
                    title=row["title"].replace("|", "\\|"),
                    target=row["target"],
                    priority_heading=row["priority_heading"].replace("|", "\\|"),
                    detail_link=row["detail_link"].replace("|", "\\|"),
                )
            )

    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return json_path, md_path


def is_governance_label(label: str) -> bool:
    normalised = normalise_label_value(label)
    return normalised.startswith(("area:", "priority:", "type:", "status:"))


def label_metadata(label: str) -> tuple[str, str]:
    normalised = normalise_label_value(label)
    if normalised.startswith("area:"):
        return "c5def5", f"Governance area label for {label.split(':', 1)[1]}"
    if normalised.startswith("priority:"):
        return "b60205", f"Governance priority label for {label.split(':', 1)[1]}"
    if normalised.startswith("type:"):
        return "84b6eb", f"Governance type label for {label.split(':', 1)[1]}"
    if normalised.startswith("status:"):
        return "0e8a16", f"Governance workflow status label for {label.split(':', 1)[1]}"
    return "ededed", "Automation-managed label"


def run_gh_command(args: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=False,
    )


def list_existing_labels(repo: str, repo_root: Path, limit: int = 1000) -> set[str]:
    args = [
        "gh",
        "label",
        "list",
        "--repo",
        repo,
        "--limit",
        str(limit),
        "--json",
        "name",
    ]
    result = run_gh_command(args, repo_root)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "gh label list failed")
    return {entry["name"] for entry in json.loads(result.stdout)}


def create_label(repo: str, repo_root: Path, label: str) -> None:
    color, description = label_metadata(label)
    args = [
        "gh",
        "label",
        "create",
        label,
        "--repo",
        repo,
        "--color",
        color,
        "--description",
        description,
    ]
    result = run_gh_command(args, repo_root)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or f"gh label create failed for {label}")


def resolve_labels_for_apply(
    repo: str,
    repo_root: Path,
    labels: list[str],
    existing_labels: set[str],
) -> tuple[list[str], list[str]]:
    applied: list[str] = []
    skipped: list[str] = []

    for label in labels:
        if label in existing_labels:
            applied.append(label)
            continue
        if is_governance_label(label):
            create_label(repo, repo_root, label)
            existing_labels.add(label)
            applied.append(label)
            continue
        skipped.append(label)

    return dedupe(applied), dedupe(skipped)


def list_existing_issues(repo: str, repo_root: Path, limit: int) -> list[dict]:
    args = [
        "gh",
        "issue",
        "list",
        "--repo",
        repo,
        "--state",
        "all",
        "--limit",
        str(limit),
        "--json",
        "number,title,body,url,milestone,labels,state",
    ]
    result = run_gh_command(args, repo_root)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "gh issue list failed")
    return json.loads(result.stdout)


def find_existing_issue(item: RoadmapItem, issues: list[dict]) -> dict | None:
    for issue in issues:
        body = issue.get("body") or ""
        match = BODY_SOURCE_KEY_RE.search(body)
        if match and match.group(1) == item.source_key:
            return issue

    for issue in issues:
        labels = {label.get("name") for label in issue.get("labels") or []}
        issue_milestone = ((issue.get("milestone") or {}) or {}).get("title")
        area_label = infer_area_label(item)
        if (
            issue.get("title") == item.title
            and (area_label in labels or f"module:{item.module}" in labels)
            and issue_milestone == item.target
        ):
            return issue
    return None


def create_issue(repo: str, repo_root: Path, item: RoadmapItem, body_path: Path, labels: list[str]) -> tuple[int, str]:
    args = [
        "gh",
        "issue",
        "create",
        "--repo",
        repo,
        "--title",
        item.title,
        "--body-file",
        str(body_path),
        "--milestone",
        item.target,
    ]
    for label in labels:
        args.extend(["--label", label])
    result = run_gh_command(args, repo_root)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "gh issue create failed")

    output = result.stdout.strip()
    match = re.search(r"/issues/(\d+)", output)
    if not match:
        raise RuntimeError(f"Could not parse created issue number from: {output}")
    return int(match.group(1)), output


def backfill_roadmap(roadmap_path: Path, issue_mapping: dict[str, int]) -> bool:
    original_lines = roadmap_path.read_text(encoding="utf-8").splitlines()
    items, tables = parse_roadmap_document(roadmap_path)
    items_by_line = {item.line_index: item for item in items}
    table_by_start = {table.start_line: table for table in tables}

    rebuilt: list[str] = []
    line_index = 0
    changed = False

    while line_index < len(original_lines):
        table = table_by_start.get(line_index)
        if table is None:
            rebuilt.append(original_lines[line_index])
            line_index += 1
            continue

        headers = list(table.headers)
        if ISSUE_COLUMN not in headers:
            detail_index = headers.index("Detail")
            headers.insert(detail_index, ISSUE_COLUMN)
            changed = True

        rebuilt.append("| " + " | ".join(headers) + " |")
        rebuilt.append("| " + " | ".join(["---"] * len(headers)) + " |")

        for row in table.rows:
            item = items_by_line[row.line_index]
            issue_number = issue_mapping.get(item.source_key) or item.issue_ref
            row_cells = dict(row.cells)
            existing_issue = parse_issue_reference(row_cells.get(ISSUE_COLUMN, ""))
            if issue_number and existing_issue != issue_number:
                row_cells[ISSUE_COLUMN] = f"#{issue_number}"
                changed = True
            elif ISSUE_COLUMN not in row_cells:
                row_cells[ISSUE_COLUMN] = f"#{issue_number}" if issue_number else ""

            ordered = [row_cells.get(header, "") for header in headers]
            rebuilt.append("| " + " | ".join(ordered) + " |")

        line_index = table.end_line + 1

    if changed:
        roadmap_path.write_text("\n".join(rebuilt) + "\n", encoding="utf-8")
    return changed


def preview_records_for_items(items: list[RoadmapItem], repo_root: Path, preview_dir: Path) -> list[dict]:
    records: list[dict] = []
    body_dir = preview_dir / "bodies"
    ensure_directory(body_dir)

    for item in items:
        detail_file = repo_root / "src" / item.detail_path
        missing_detail = False
        try:
            section_heading, section_text = find_detail_section(detail_file, item.detail_anchor, item.title)
        except ValueError:
            missing_detail = True
            section_heading = "Detail section not found"
            section_text = (
                f"No matching section was found for anchor '{item.detail_anchor}' in `src/{item.detail_path}`.\n\n"
                "Use this issue to restore roadmap/detail consistency and implement the scope described in the roadmap title."
            )
        acceptance = extract_acceptance_criteria(section_text)
        issue_kind = classify_issue_kind(item)
        issue_labels = build_issue_labels(item, issue_kind)
        body = build_issue_body(item, section_heading, section_text, acceptance, issue_labels)
        body_path = body_dir / f"{item.row_number.zfill(4)}-{item.module}-{canonical_anchor(item.detail_anchor)}.md"
        body_path.write_text(body, encoding="utf-8")

        records.append(
            {
                "row_number": item.row_number,
                "module": item.module,
                "title": item.title,
                "target": item.target,
                "priority": item.priority,
                "priority_heading": item.priority_heading,
                "labels": issue_labels,
                "detail_link": f"src/{item.detail_link}",
                "detail_heading": section_heading,
                "issue_number": item.issue_ref,
                "source_key": item.source_key,
                "acceptance_criteria": acceptance,
                "body_file": body_path.relative_to(repo_root).as_posix(),
                "detail_section_missing": missing_detail,
                "status": "planned" if item.issue_ref is None else "already-linked",
            }
        )

    return records


def apply_items(
    items: list[RoadmapItem],
    preview_records: list[dict],
    *,
    repo: str,
    repo_root: Path,
    existing_limit: int,
) -> list[dict]:
    existing_issues = list_existing_issues(repo, repo_root, existing_limit)
    existing_labels = list_existing_labels(repo, repo_root)
    record_by_source_key = {record["source_key"]: record for record in preview_records}
    applied_records: list[dict] = []

    for item in items:
        record = dict(record_by_source_key[item.source_key])
        if item.issue_ref is not None:
            record["issue_number"] = item.issue_ref
            record["status"] = "already-linked"
            applied_records.append(record)
            continue

        matched = find_existing_issue(item, existing_issues)
        if matched is not None:
            record["issue_number"] = matched["number"]
            record["issue_url"] = matched.get("url")
            record["status"] = "matched-existing"
            applied_records.append(record)
            continue

        labels, skipped_labels = resolve_labels_for_apply(repo, repo_root, record["labels"], existing_labels)
        body_path = repo_root / record["body_file"]
        issue_number, issue_url = create_issue(repo, repo_root, item, body_path, labels)
        record["issue_number"] = issue_number
        record["issue_url"] = issue_url
        record["status"] = "created"
        record["labels"] = labels
        if skipped_labels:
            record["skipped_labels"] = skipped_labels
        existing_issues.append(
            {
                "number": issue_number,
                "title": item.title,
                "body": body_path.read_text(encoding="utf-8"),
                "url": issue_url,
                "milestone": {"title": item.target},
                "labels": [{"name": label} for label in labels],
            }
        )
        applied_records.append(record)

    return applied_records


def load_manifest(path: Path) -> list[dict]:
    return json.loads(path.read_text(encoding="utf-8"))


def run_preview(args: argparse.Namespace, repo_root: Path) -> int:
    roadmap_path = (repo_root / args.roadmap).resolve()
    preview_dir = (repo_root / args.preview_dir).resolve()
    items, _ = parse_roadmap_document(roadmap_path)
    items = filter_items(
        items,
        module_filter=args.module,
        priority_filter=args.priority,
        offset=args.offset,
        limit=args.limit,
    )
    records = preview_records_for_items(items, repo_root, preview_dir)
    json_path, md_path = write_preview_artifacts(preview_dir, records, "preview")
    summary = compute_roadmap_summary(roadmap_path)
    summary_json_path, summary_md_path = write_summary_artifacts(preview_dir, summary)
    print(f"PREVIEW_JSON={json_path.relative_to(repo_root)}")
    print(f"PREVIEW_MD={md_path.relative_to(repo_root)}")
    print(f"SUMMARY_JSON={summary_json_path.relative_to(repo_root)}")
    print(f"SUMMARY_MD={summary_md_path.relative_to(repo_root)}")
    print(f"ITEMS={len(records)}")
    return 0


def run_apply(args: argparse.Namespace, repo_root: Path) -> int:
    roadmap_path = (repo_root / args.roadmap).resolve()
    preview_dir = (repo_root / args.preview_dir).resolve()
    items, _ = parse_roadmap_document(roadmap_path)
    items = filter_items(
        items,
        module_filter=args.module,
        priority_filter=args.priority,
        offset=args.offset,
        limit=args.limit,
    )
    preview_records = preview_records_for_items(items, repo_root, preview_dir)
    applied_records = apply_items(
        items,
        preview_records,
        repo=args.repo,
        repo_root=repo_root,
        existing_limit=args.issue_fetch_limit,
    )
    json_path, md_path = write_preview_artifacts(preview_dir, applied_records, "apply")
    issue_mapping = {
        record["source_key"]: int(record["issue_number"])
        for record in applied_records
        if record.get("issue_number")
    }

    if args.backfill:
        changed = backfill_roadmap(roadmap_path, issue_mapping)
        print(f"BACKFILL_CHANGED={'true' if changed else 'false'}")

    summary = compute_roadmap_summary(roadmap_path, issue_mapping)
    summary_json_path, summary_md_path = write_summary_artifacts(preview_dir, summary)

    print(f"APPLY_JSON={json_path.relative_to(repo_root)}")
    print(f"APPLY_MD={md_path.relative_to(repo_root)}")
    print(f"SUMMARY_JSON={summary_json_path.relative_to(repo_root)}")
    print(f"SUMMARY_MD={summary_md_path.relative_to(repo_root)}")
    print(f"ITEMS={len(applied_records)}")
    created = sum(1 for record in applied_records if record["status"] == "created")
    matched = sum(1 for record in applied_records if record["status"] == "matched-existing")
    linked = sum(1 for record in applied_records if record["status"] == "already-linked")
    print(f"CREATED={created}")
    print(f"MATCHED_EXISTING={matched}")
    print(f"ALREADY_LINKED={linked}")
    return 0


def run_backfill(args: argparse.Namespace, repo_root: Path) -> int:
    if not args.manifest:
        print("❌ --manifest is required for backfill mode", file=sys.stderr)
        return 1
    roadmap_path = (repo_root / args.roadmap).resolve()
    manifest_path = (repo_root / args.manifest).resolve()
    manifest = load_manifest(manifest_path)
    issue_mapping = {
        record["source_key"]: int(record["issue_number"])
        for record in manifest
        if record.get("issue_number")
    }
    changed = backfill_roadmap(roadmap_path, issue_mapping)
    preview_dir = (repo_root / args.preview_dir).resolve()
    summary = compute_roadmap_summary(roadmap_path, issue_mapping)
    summary_json_path, summary_md_path = write_summary_artifacts(preview_dir, summary)
    print(f"BACKFILL_CHANGED={'true' if changed else 'false'}")
    print(f"ROADMAP={roadmap_path.relative_to(repo_root)}")
    print(f"SUMMARY_JSON={summary_json_path.relative_to(repo_root)}")
    print(f"SUMMARY_MD={summary_md_path.relative_to(repo_root)}")
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", REPO_DEFAULT))
    parser.add_argument("--roadmap", default="src/ROADMAP.md")
    parser.add_argument("--preview-dir", default="artifacts/roadmap-issues")
    parser.add_argument("--module", default=None)
    parser.add_argument("--priority", choices=["critical", "high", "medium", "low"], default=None)
    parser.add_argument("--offset", type=int, default=0)
    parser.add_argument("--limit", type=int, default=None)
    parser.add_argument("--issue-fetch-limit", type=int, default=5000)
    parser.add_argument("--manifest", default=None)
    parser.add_argument("--backfill", action="store_true", default=False)
    parser.add_argument(
        "--mode",
        choices=["preview", "apply", "backfill"],
        default="preview",
        help="preview: generate artifacts only; apply: create/match issues; backfill: update roadmap from a manifest",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = Path(__file__).resolve().parent.parent

    if args.mode == "preview":
        return run_preview(args, repo_root)
    if args.mode == "apply":
        return run_apply(args, repo_root)
    return run_backfill(args, repo_root)


if __name__ == "__main__":
    sys.exit(main())