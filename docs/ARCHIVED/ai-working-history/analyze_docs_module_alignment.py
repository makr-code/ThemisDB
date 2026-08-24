#!/usr/bin/env python3
"""Analyze alignment of docs/ markdown against module planning docs.

Compares per-module src/<module>/FUTURE_ENHANCEMENTS.md and MODULE_GAPS.md
with docs/ markdown files. Newer docs are weighted higher.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, asdict
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
DOCS_DIR = ROOT / "docs"
OUT_JSON = ROOT / "ai_working" / "docs_module_alignment_report_2026-05-31.json"
OUT_MD = ROOT / "ai_working" / "docs_module_alignment_report_2026-05-31.md"

PLAN_FILES = ("FUTURE_ENHANCEMENTS.md", "MODULE_GAPS.md")
ARCHIVE_HINTS = {
    "archived",
    "archive",
    "implementation-history",
    "audit-reports",
    "tmp-notes",
}
LOW_SIGNAL_NAME_HINTS = {
    "report",
    "summary",
    "status",
    "final",
    "completion",
}

OPEN_TASK_RE = re.compile(r"^\s*[-*]\s*\[\s\]\s+")
CHECKBOX_RE = re.compile(r"^\s*[-*]\s*\[[ x~!IP?]\]\s+", re.IGNORECASE)


@dataclass
class ModuleAlignment:
    module: str
    plan_files: list[str]
    plan_open_tasks: int
    plan_checkbox_total: int
    plan_newest: str | None
    related_docs_count: int
    related_docs_high_relevance_count: int
    related_docs_open_tasks_high_relevance: int
    docs_newest: str | None
    docs_minus_plan_days: int | None
    status: str
    risk_score: int
    high_relevance_docs: list[str]


@dataclass
class GlobalSummary:
    generated_at: str
    modules_analyzed: int
    modules_with_risks: int
    modules_without_docs_links: int
    modules_with_stale_docs: int
    modules_with_workload_underestimate_risk: int


def to_dt(path: Path) -> datetime:
    return datetime.fromtimestamp(path.stat().st_mtime, tz=timezone.utc)


def iso(dt: datetime | None) -> str | None:
    if dt is None:
        return None
    return dt.date().isoformat()


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ""


def count_open_tasks(path: Path) -> tuple[int, int]:
    open_tasks = 0
    total_checkboxes = 0
    for line in read_text(path).splitlines():
        if OPEN_TASK_RE.search(line):
            open_tasks += 1
        if CHECKBOX_RE.search(line):
            total_checkboxes += 1
    return open_tasks, total_checkboxes


def age_score(dt: datetime, now: datetime) -> int:
    days = (now - dt).days
    if days <= 14:
        return 5
    if days <= 30:
        return 4
    if days <= 90:
        return 3
    if days <= 180:
        return 2
    if days <= 365:
        return 1
    return 0


def relevance_score(module: str, path: Path, now: datetime) -> int:
    rel = path.relative_to(ROOT)
    parts = [p.lower() for p in rel.parts]
    name = path.name.lower()

    score = age_score(to_dt(path), now)

    # Strong positive signal: module-focused docs paths
    if f"en/{module}" in str(rel).replace('\\', '/').lower():
        score += 2
    if f"de/{module}" in str(rel).replace('\\', '/').lower():
        score += 2
    if f"docs/{module}" in str(rel).replace('\\', '/').lower():
        score += 1

    if name == "primary_sources.md":
        score += 2
    elif name == "readme.md":
        score += 1

    # Negative signal for archived/historical-heavy areas
    if any(h in parts for h in ARCHIVE_HINTS):
        score -= 3

    if any(h in name for h in LOW_SIGNAL_NAME_HINTS):
        score -= 1

    if score < 0:
        score = 0
    return score


def find_related_docs(module: str, docs_files: list[Path]) -> list[Path]:
    related: list[Path] = []
    module_lower = module.lower()
    for p in docs_files:
        rel = str(p.relative_to(ROOT)).replace('\\', '/').lower()
        stem = p.stem.lower()
        name = p.name.lower()

        by_segment = f"/{module_lower}/" in f"/{rel}/"
        by_lang_segment = f"/en/{module_lower}/" in f"/{rel}/" or f"/de/{module_lower}/" in f"/{rel}/"
        by_filename = (
            stem == module_lower
            or stem.startswith(module_lower + "_")
            or stem.endswith("_" + module_lower)
            or f"{module_lower}_" in stem
            or f"_{module_lower}" in stem
            or module_lower in name
        )

        if by_segment or by_lang_segment or by_filename:
            related.append(p)
    return related


def derive_status(
    related_count: int,
    stale_days: int | None,
    plan_open: int,
    docs_open_high_rel: int,
) -> tuple[str, int]:
    risk = 0

    if related_count == 0:
        return "NO_DOC_LINK", 100

    if stale_days is not None and stale_days < -30:
        risk += 50

    if plan_open >= 20 and docs_open_high_rel < max(3, int(plan_open * 0.10)):
        risk += 30

    if plan_open >= 50 and docs_open_high_rel < max(5, int(plan_open * 0.08)):
        risk += 20

    if risk >= 70:
        return "HIGH_RISK_MISALIGNMENT", risk
    if risk >= 40:
        return "STALE_OR_PARTIAL", risk
    return "OK", risk


def build_report() -> dict:
    now = datetime.now(timezone.utc)

    modules = sorted(
        [p.name for p in SRC_DIR.iterdir() if p.is_dir() and not p.name.startswith('.')]
    )
    docs_files = sorted([p for p in DOCS_DIR.rglob("*.md") if p.is_file()])

    results: list[ModuleAlignment] = []

    for module in modules:
        module_dir = SRC_DIR / module
        plan_paths = [module_dir / f for f in PLAN_FILES if (module_dir / f).exists()]

        # Keep only modules with at least one planning document in scope
        if not plan_paths:
            continue

        plan_open_tasks = 0
        plan_checkbox_total = 0
        plan_dates: list[datetime] = []
        for pp in plan_paths:
            open_count, cb_total = count_open_tasks(pp)
            plan_open_tasks += open_count
            plan_checkbox_total += cb_total
            plan_dates.append(to_dt(pp))

        plan_newest = max(plan_dates) if plan_dates else None

        related_docs = find_related_docs(module, docs_files)
        docs_dates = [to_dt(p) for p in related_docs]
        docs_newest = max(docs_dates) if docs_dates else None

        docs_minus_plan_days = None
        if docs_newest is not None and plan_newest is not None:
            docs_minus_plan_days = (docs_newest - plan_newest).days

        scored = sorted(
            [(relevance_score(module, p, now), p) for p in related_docs],
            key=lambda x: (x[0], to_dt(x[1])),
            reverse=True,
        )

        high_rel = [p for s, p in scored if s >= 4]
        docs_open_high_rel = 0
        for p in high_rel:
            o, _ = count_open_tasks(p)
            docs_open_high_rel += o

        status, risk_score = derive_status(
            related_count=len(related_docs),
            stale_days=docs_minus_plan_days,
            plan_open=plan_open_tasks,
            docs_open_high_rel=docs_open_high_rel,
        )

        results.append(
            ModuleAlignment(
                module=module,
                plan_files=[str(p.relative_to(ROOT)).replace('\\', '/') for p in plan_paths],
                plan_open_tasks=plan_open_tasks,
                plan_checkbox_total=plan_checkbox_total,
                plan_newest=iso(plan_newest),
                related_docs_count=len(related_docs),
                related_docs_high_relevance_count=len(high_rel),
                related_docs_open_tasks_high_relevance=docs_open_high_rel,
                docs_newest=iso(docs_newest),
                docs_minus_plan_days=docs_minus_plan_days,
                status=status,
                risk_score=risk_score,
                high_relevance_docs=[
                    str(p.relative_to(ROOT)).replace('\\', '/') for p in high_rel[:8]
                ],
            )
        )

    results_sorted = sorted(
        results,
        key=lambda r: (r.risk_score, r.plan_open_tasks, -(r.docs_minus_plan_days or 0)),
        reverse=True,
    )

    modules_with_risks = [r for r in results_sorted if r.status != "OK"]
    summary = GlobalSummary(
        generated_at=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        modules_analyzed=len(results_sorted),
        modules_with_risks=len(modules_with_risks),
        modules_without_docs_links=sum(1 for r in results_sorted if r.status == "NO_DOC_LINK"),
        modules_with_stale_docs=sum(
            1 for r in results_sorted if r.docs_minus_plan_days is not None and r.docs_minus_plan_days < -30
        ),
        modules_with_workload_underestimate_risk=sum(
            1
            for r in results_sorted
            if r.plan_open_tasks >= 20
            and r.related_docs_open_tasks_high_relevance < max(3, int(r.plan_open_tasks * 0.10))
        ),
    )

    payload = {
        "summary": asdict(summary),
        "modules": [asdict(r) for r in results_sorted],
    }
    return payload


def write_outputs(payload: dict) -> None:
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    summary = payload["summary"]
    rows = payload["modules"]

    top_risks = [r for r in rows if r["status"] != "OK"][:25]

    lines: list[str] = []
    lines.append("# Docs-zu-Planungs-Abgleich (Module)")
    lines.append("")
    lines.append("## Methode")
    lines.append("")
    lines.append("- Quelle pro Modul: src/<module>/FUTURE_ENHANCEMENTS.md und src/<module>/MODULE_GAPS.md")
    lines.append("- Vergleichsziel: docs/**/*.md")
    lines.append("- Relevanzprinzip: neuere Dokumente werden hoeher gewichtet als alte")
    lines.append("- Zusatzgewicht: docs/en/<module>/..., docs/de/<module>/..., PRIMARY_SOURCES.md")
    lines.append("- Abwertung: archive/ARCHIVED/implementation-history/audit-reports")
    lines.append("")
    lines.append("## Zusammenfassung")
    lines.append("")
    lines.append(f"- Generiert: {summary['generated_at']}")
    lines.append(f"- Module analysiert: {summary['modules_analyzed']}")
    lines.append(f"- Module mit Risiko: {summary['modules_with_risks']}")
    lines.append(f"- Ohne Docs-Link: {summary['modules_without_docs_links']}")
    lines.append(f"- Mit veralteten Docs (>30 Tage hinter Plan): {summary['modules_with_stale_docs']}")
    lines.append(
        f"- Risiko Workload-Unterschaetzung: {summary['modules_with_workload_underestimate_risk']}"
    )
    lines.append("")

    lines.append("## Top-Risiko-Module")
    lines.append("")
    lines.append(
        "| Modul | Status | Risiko-Score | Plan offene Tasks | High-Rel Docs | High-Rel offene Tasks | Docs minus Plan (Tage) |"
    )
    lines.append("|---|---:|---:|---:|---:|---:|---:|")
    for r in top_risks:
        dmp = "—" if r["docs_minus_plan_days"] is None else str(r["docs_minus_plan_days"])
        lines.append(
            f"| {r['module']} | {r['status']} | {r['risk_score']} | {r['plan_open_tasks']} | "
            f"{r['related_docs_high_relevance_count']} | {r['related_docs_open_tasks_high_relevance']} | {dmp} |"
        )

    lines.append("")
    lines.append("## Hinweise zur Nutzung")
    lines.append("")
    lines.append("- Prioritaet 1: Status NO_DOC_LINK und HIGH_RISK_MISALIGNMENT")
    lines.append("- Prioritaet 2: Module mit stark negativem Docs-minus-Plan-Wert")
    lines.append("- Prioritaet 3: Module mit vielen offenen Plan-Tasks, aber kaum offene High-Rel-Doku-Tasks")
    lines.append("")
    lines.append("## Artefakte")
    lines.append("")
    lines.append(f"- JSON: {OUT_JSON.relative_to(ROOT).as_posix()}")
    lines.append(f"- Report: {OUT_MD.relative_to(ROOT).as_posix()}")

    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    payload = build_report()
    write_outputs(payload)
    print(f"Wrote {OUT_JSON}")
    print(f"Wrote {OUT_MD}")


if __name__ == "__main__":
    main()
