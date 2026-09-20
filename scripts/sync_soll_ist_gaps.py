#!/usr/bin/env python3
"""Build Soll-Ist gap reports and upsert GitHub issues without duplicates.

Inputs
- Documentation compliance markdown table
- Verified source gap JSON files (module-level)

Outputs
- JSON report
- Markdown summary
- Optional GitHub issue upsert (create/update/close) keyed by stable marker

Key design
- One implementation issue per module: key "soll-ist-key:impl:<module>"
- One documentation issue per module: key "soll-ist-key:docs:<module>"
- Existing open issue with same key is updated, not recreated.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


DOC_STATUS_OK = "OK"
REQUIRED_MODULE_DOCS = [
    "README.md",
    "ROADMAP.md",
    "ARCHITECTURE.md",
    "CHANGELOG.md",
    "FUTURE_ENHANCEMENTS.md",
]
SEVERITIES = ("CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO")


@dataclass
class ModuleDocsStatus:
    module: str
    status: str
    score_percent: int
    missing_doc_types: list[str]


@dataclass
class ModuleImplStatus:
    module: str
    actionable_total: int
    severity_counts: dict[str, int]
    findings: list[dict[str, Any]]


@dataclass
class ModuleQualityStatus:
    module: str
    test_files: list[str]
    benchmark_files: list[str]
    failing_tests: list[str]
    failing_benchmarks: list[str]


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Compare source/doc artifacts and upsert issues")
    p.add_argument("--repo-root", default=".", help="Repository root")
    p.add_argument(
        "--docs-compliance-md",
        default="docs/governance/SRC_MODULE_DOCUMENTATION_COMPLIANCE_2026-09-20.md",
        help="Markdown file with module compliance table",
    )
    p.add_argument(
        "--source-gap-glob",
        default="ai_working/gap_scanner_verified_*.json",
        help="Glob for verified source gap JSON files",
    )
    p.add_argument(
        "--report-json",
        default="ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json",
        help="Output JSON report",
    )
    p.add_argument(
        "--summary-md",
        default="ai_context/developer_llm_wiki/SOLL_IST_GAP_SUMMARY.md",
        help="Output markdown summary",
    )
    p.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB"), help="owner/repo")
    p.add_argument("--sync-issues", action="store_true", help="Upsert GitHub issues via gh CLI")
    p.add_argument("--apply", action="store_true", help="Apply write operations (default preview)")
    p.add_argument("--close-resolved", action="store_true", help="Close previously tracked issues that are now resolved")
    p.add_argument("--max-findings-per-issue", type=int, default=12)
    p.add_argument(
        "--ctest-log",
        default="ctest_last_run.txt",
        help="Optional CTest log used to map failed tests to modules",
    )
    p.add_argument(
        "--benchmark-log",
        default="",
        help="Optional benchmark log used to map failed benchmarks to modules",
    )
    return p.parse_args()


def parse_markdown_table(md_path: Path) -> dict[str, ModuleDocsStatus]:
    text = md_path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    table_header_idx = -1
    for idx, line in enumerate(lines):
        if line.strip().startswith("| Modul |") and "| Status |" in line:
            table_header_idx = idx
            break

    if table_header_idx < 0:
        raise RuntimeError(f"Compliance table header not found in {md_path}")

    headers = [c.strip() for c in lines[table_header_idx].strip().strip("|").split("|")]
    col_idx = {name: i for i, name in enumerate(headers)}

    required = ["Modul", "Score", "Status"]
    for col in required:
        if col not in col_idx:
            raise RuntimeError(f"Required column '{col}' not found in compliance table")

    doc_columns = [c for c in headers if c not in ("Modul", "Score", "Status")]

    by_module: dict[str, ModuleDocsStatus] = {}

    for line in lines[table_header_idx + 2 :]:
        if not line.strip().startswith("|"):
            break
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) != len(headers):
            continue

        module = cells[col_idx["Modul"]].strip()
        if not module:
            continue

        status = cells[col_idx["Status"]].strip()
        score_raw = cells[col_idx["Score"]].strip().replace("%", "")
        try:
            score = int(float(score_raw))
        except ValueError:
            score = 0

        missing: list[str] = []
        for c in doc_columns:
            val = cells[col_idx[c]].strip().upper()
            if val == "N":
                missing.append(c)

        by_module[module] = ModuleDocsStatus(
            module=module,
            status=status,
            score_percent=score,
            missing_doc_types=missing,
        )

    return by_module


def classify_actionable(finding: dict[str, Any]) -> bool:
    classification = str(finding.get("classification", "")).lower()
    if "false-positive" in classification or "false positive" in classification:
        return False
    if "real gap" in classification:
        return True
    if "stub" in classification:
        return True
    return False


def parse_source_gaps(repo_root: Path, source_gap_glob: str) -> dict[str, ModuleImplStatus]:
    by_module: dict[str, ModuleImplStatus] = {}

    for path in sorted(repo_root.glob(source_gap_glob)):
        try:
            obj = json.loads(path.read_text(encoding="utf-8", errors="replace"))
        except json.JSONDecodeError:
            continue

        module = str(obj.get("module", "")).strip()
        if not module:
            continue

        severity_counts = {s: 0 for s in SEVERITIES}
        actionable: list[dict[str, Any]] = []
        for f in obj.get("findings", []):
            sev = str(f.get("verified_severity") or f.get("original_severity") or "").upper()
            if sev not in severity_counts:
                sev = "INFO"
            if classify_actionable(f):
                severity_counts[sev] += 1
                actionable.append(f)

        by_module[module] = ModuleImplStatus(
            module=module,
            actionable_total=sum(severity_counts.values()),
            severity_counts=severity_counts,
            findings=actionable,
        )

    return by_module


def _related_artifact_files(repo_root: Path, base_dir: str, module: str, suffixes: tuple[str, ...]) -> list[str]:
    root = repo_root / base_dir
    if not root.exists():
        return []
    m = module.lower()
    out: list[str] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in suffixes:
            continue
        rel = path.relative_to(repo_root).as_posix()
        text = rel.lower()
        stem = path.stem.lower()
        if m in text or m in stem:
            out.append(rel)
    return sorted(set(out))


def _parse_failed_items_from_log(log_path: Path, module_names: set[str], kind: str) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {m: [] for m in module_names}
    if not log_path.exists():
        return result

    lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    failed_names: list[str] = []

    if kind == "test":
        rx = re.compile(r"\[\s*FAILED\s*\]\s+(.+)$")
        for line in lines:
            m = rx.search(line)
            if m:
                failed_names.append(m.group(1).strip())
    else:
        for line in lines:
            lowered = line.lower()
            if "benchmark" in lowered and ("fail" in lowered or "error" in lowered):
                failed_names.append(line.strip())

    for name in failed_names:
        lowered = name.lower()
        for module in module_names:
            if module in lowered:
                result[module].append(name)

    for module in result:
        result[module] = sorted(set(result[module]))
    return result


def scan_module_doc_files(repo_root: Path, module: str) -> dict[str, list[str]]:
    module_root = repo_root / "src" / module
    files_by_scope: dict[str, list[str]] = {
        "src": [],
        "include": [],
        "docs": [],
    }

    for scope, base in [("src", module_root), ("include", repo_root / "include" / module), ("docs", repo_root / "docs" / module)]:
        if not base.exists():
            continue
        for path in base.rglob("*.md"):
            rel = path.relative_to(repo_root).as_posix()
            files_by_scope.setdefault(scope, []).append(rel)

    for scope in files_by_scope:
        files_by_scope[scope] = sorted(set(files_by_scope[scope]))

    return files_by_scope


def build_module_doc_alignment(repo_root: Path, module: str, docs_status: ModuleDocsStatus | None) -> dict[str, Any]:
    found_files: dict[str, list[str]] = scan_module_doc_files(repo_root, module)
    found_names: set[str] = set()
    for scope_files in found_files.values():
        for rel in scope_files:
            found_names.add(Path(rel).name)

    missing = [doc for doc in REQUIRED_MODULE_DOCS if doc not in found_names]
    status = "ok"
    if docs_status is not None and docs_status.status != DOC_STATUS_OK:
        status = "stale"
    elif missing:
        status = "missing"

    return {
        "status": status,
        "required_core_docs": REQUIRED_MODULE_DOCS,
        "found_core_docs": sorted(found_names.intersection(REQUIRED_MODULE_DOCS)),
        "missing_core_docs": missing,
        "doc_files": {scope: files for scope, files in found_files.items() if files},
        "gap_open": status != "ok" or bool(missing),
    }


def parse_quality_status(
    repo_root: Path,
    module_names: set[str],
    ctest_log: Path,
    benchmark_log: Path | None,
) -> dict[str, ModuleQualityStatus]:
    failed_tests = _parse_failed_items_from_log(ctest_log, module_names, kind="test")
    failed_bench = _parse_failed_items_from_log(benchmark_log, module_names, kind="bench") if benchmark_log else {m: [] for m in module_names}

    quality: dict[str, ModuleQualityStatus] = {}
    for module in module_names:
        quality[module] = ModuleQualityStatus(
            module=module,
            test_files=_related_artifact_files(repo_root, "tests", module, (".cpp", ".cc", ".cxx", ".h", ".hpp")),
            benchmark_files=_related_artifact_files(repo_root, "benchmarks", module, (".cpp", ".cc", ".cxx", ".h", ".hpp", ".md")),
            failing_tests=failed_tests.get(module, []),
            failing_benchmarks=failed_bench.get(module, []),
        )
    return quality


def build_report(
    repo_root: Path,
    docs: dict[str, ModuleDocsStatus],
    impl: dict[str, ModuleImplStatus],
    quality: dict[str, ModuleQualityStatus],
) -> dict[str, Any]:
    modules = sorted(set(docs.keys()) | set(impl.keys()))
    rows: list[dict[str, Any]] = []

    for m in modules:
        d = docs.get(m)
        i = impl.get(m)
        q = quality.get(m)

        docs_missing = [] if d is None else d.missing_doc_types
        docs_gap_by_status = d is not None and d.status != DOC_STATUS_OK

        impl_actionable_total = 0 if i is None else i.actionable_total
        test_count = 0 if q is None else len(q.test_files)
        bench_count = 0 if q is None else len(q.benchmark_files)
        failing_test_count = 0 if q is None else len(q.failing_tests)
        failing_bench_count = 0 if q is None else len(q.failing_benchmarks)

        tests_missing = impl_actionable_total > 0 and test_count == 0
        benchmarks_missing = impl_actionable_total > 0 and bench_count == 0
        release_gate_gap_open = tests_missing or benchmarks_missing or failing_test_count > 0 or failing_bench_count > 0

        impl_gap_open = impl_actionable_total > 0 or release_gate_gap_open

        developer_docs_alignment = build_module_doc_alignment(repo_root, m, d)
        docs_gap_open = docs_gap_by_status or bool(developer_docs_alignment.get("gap_open"))
        rows.append(
            {
                "module": m,
                "docs": {
                    "status": d.status if d else "UNKNOWN",
                    "score_percent": d.score_percent if d else 0,
                    "missing_doc_types": docs_missing,
                    "gap_open": docs_gap_open,
                    "gap_open_by_status": docs_gap_by_status,
                    "gap_open_by_alignment": bool(developer_docs_alignment.get("gap_open")),
                },
                "developer_docs_alignment": developer_docs_alignment,
                "implementation": {
                    "actionable_total": impl_actionable_total,
                    "severity_counts": i.severity_counts if i else {s: 0 for s in SEVERITIES},
                    "gap_open": impl_gap_open,
                },
                "release_gates": {
                    "related_tests": test_count,
                    "related_benchmarks": bench_count,
                    "failing_tests": failing_test_count,
                    "failing_benchmarks": failing_bench_count,
                    "tests_missing": tests_missing,
                    "benchmarks_missing": benchmarks_missing,
                    "gap_open": release_gate_gap_open,
                },
            }
        )

    return {
        "tool": "sync_soll_ist_gaps.py",
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "totals": {
            "modules": len(rows),
            "docs_gap_modules": sum(1 for r in rows if r["docs"]["gap_open"]),
            "docs_gap_modules_by_status": sum(1 for r in rows if r["docs"].get("gap_open_by_status")),
            "docs_gap_modules_by_alignment": sum(1 for r in rows if r["docs"].get("gap_open_by_alignment")),
            "impl_gap_modules": sum(1 for r in rows if r["implementation"]["gap_open"]),
            "release_gate_gap_modules": sum(1 for r in rows if r["release_gates"]["gap_open"]),
        },
        "rows": rows,
    }


def write_summary(path: Path, report: dict[str, Any]) -> None:
    lines: list[str] = []
    lines.append("# Soll-Ist Gap Summary")
    lines.append("")
    lines.append(f"- Generated At: {report.get('generated_at')}")
    lines.append(f"- Modules: {report['totals']['modules']}")
    lines.append(f"- Modules with docs gaps: {report['totals']['docs_gap_modules']}")
    lines.append(f"- Modules with implementation gaps: {report['totals']['impl_gap_modules']}")
    lines.append(f"- Modules with release-gate gaps (tests/benchmarks): {report['totals']['release_gate_gap_modules']}")
    lines.append("")
    lines.append("| Module | Docs Status | Docs Score | Missing Docs | Impl Actionable | Critical | High | Medium | Tests | Bench | Failing Tests | Failing Bench |")
    lines.append("|---|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|")

    for row in report.get("rows", []):
        d = row["docs"]
        i = row["implementation"]
        g = row["release_gates"]
        missing = ", ".join(d.get("missing_doc_types", [])) if d.get("missing_doc_types") else "-"
        sev = i.get("severity_counts", {})
        lines.append(
            f"| {row['module']} | {d.get('status','UNKNOWN')} | {d.get('score_percent',0)}% | {missing} | {i.get('actionable_total',0)} | {sev.get('CRITICAL',0)} | {sev.get('HIGH',0)} | {sev.get('MEDIUM',0)} | {g.get('related_tests',0)} | {g.get('related_benchmarks',0)} | {g.get('failing_tests',0)} | {g.get('failing_benchmarks',0)} |"
        )

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_gh(args: list[str], repo_root: Path) -> str:
    proc = subprocess.run(["gh", *args], cwd=repo_root, capture_output=True, text=False)
    stdout = proc.stdout.decode("utf-8", errors="replace") if proc.stdout else ""
    stderr = proc.stderr.decode("utf-8", errors="replace") if proc.stderr else ""
    if proc.returncode != 0:
        raise RuntimeError(stderr.strip() or "gh command failed")
    return stdout


def load_open_issues(repo_root: Path, repo: str) -> list[dict[str, Any]]:
    out = run_gh(
        [
            "issue",
            "list",
            "--repo",
            repo,
            "--state",
            "open",
            "--limit",
            "1000",
            "--json",
            "number,title,body,labels",
        ],
        repo_root,
    )
    return json.loads(out)


def find_issue_by_key(open_issues: list[dict[str, Any]], key: str) -> dict[str, Any] | None:
    marker = f"soll-ist-key:{key}"
    for issue in open_issues:
        body = str(issue.get("body", ""))
        if marker in body:
            return issue
    return None


def issue_body_impl(module: str, row: dict[str, Any], impl_status: ModuleImplStatus | None, max_findings: int) -> str:
    docs = row["docs"]
    impl = row["implementation"]
    gates = row.get("release_gates", {})
    alignment = row.get("developer_docs_alignment", {})
    sev = impl.get("severity_counts", {})

    lines: list[str] = []
    lines.append(f"# Soll-Ist Gap: Implementierung ({module})")
    lines.append("")
    lines.append(f"soll-ist-key:impl:{module}")
    lines.append("")
    lines.append("## Ist-Zustand")
    lines.append(f"- Actionable Findings: {impl.get('actionable_total', 0)}")
    lines.append(f"- Critical: {sev.get('CRITICAL', 0)}")
    lines.append(f"- High: {sev.get('HIGH', 0)}")
    lines.append(f"- Medium: {sev.get('MEDIUM', 0)}")
    lines.append(f"- Developer docs alignment: {alignment.get('status', 'unknown')}")
    lines.append("")
    lines.append("## Soll-Zustand")
    lines.append("- Keine offenen, verifizierten Implementierungs-Gaps (Real Gap / Stub)")
    lines.append("- Kritische und hohe Findings zuerst schließen")
    lines.append("- Modul-Dokumentation muss mit Code-Scope und Betriebsstatus konsistent sein")
    lines.append("")
    lines.append("## Entwickler-Doku-Abgleich")
    lines.append(f"- Doku-Status: {docs.get('status', 'UNKNOWN')} ({docs.get('score_percent', 0)}%)")
    if alignment.get("missing_core_docs"):
        lines.append(f"- Fehlende Core-Dokumente: {', '.join(alignment.get('missing_core_docs', []))}")
    else:
        lines.append("- Fehlende Core-Dokumente: keine")
    if alignment.get("doc_files"):
        lines.append("- Dokumente gefunden: " + ", ".join(sorted({Path(p).name for files in alignment.get("doc_files", {}).values() for p in files})))
    lines.append("")

    lines.append("## Kontext")
    lines.append(f"- Doku-Status parallel: {docs.get('status', 'UNKNOWN')} ({docs.get('score_percent', 0)}%)")
    lines.append("")

    lines.append("## Release-Gate: Tests und Benchmarks")
    lines.append(f"- Related tests: {gates.get('related_tests', 0)}")
    lines.append(f"- Related benchmarks: {gates.get('related_benchmarks', 0)}")
    lines.append(f"- Failing tests: {gates.get('failing_tests', 0)}")
    lines.append(f"- Failing benchmarks: {gates.get('failing_benchmarks', 0)}")
    if gates.get("tests_missing", False):
        lines.append("- Gap: keine modulbezogenen Tests gefunden")
    if gates.get("benchmarks_missing", False):
        lines.append("- Gap: keine modulbezogenen Benchmarks gefunden")
    lines.append("")

    if gates.get("failing_tests", 0) > 0 and impl_status is not None:
        lines.append("### Failing Tests (aus CTest-Log)")
        quality_details = row.get("release_gate_details", {})
        for item in quality_details.get("failing_tests", [])[:10]:
            lines.append(f"- {item}")
        lines.append("")

    if gates.get("failing_benchmarks", 0) > 0 and impl_status is not None:
        lines.append("### Failing Benchmarks (aus Benchmark-Log)")
        quality_details = row.get("release_gate_details", {})
        for item in quality_details.get("failing_benchmarks", [])[:10]:
            lines.append(f"- {item}")
        lines.append("")

    lines.append("## Top Findings")

    if impl_status is None or not impl_status.findings:
        lines.append("- Keine einzelnen Findings im Input vorhanden.")
    else:
        for finding in impl_status.findings[:max_findings]:
            file_path = str(finding.get("file", "?")).strip()
            line = str(finding.get("line", "?")).strip()
            pattern = str(finding.get("pattern", "")).strip()
            cls = str(finding.get("classification", "")).strip()
            sev_item = str(finding.get("verified_severity") or finding.get("original_severity") or "").upper()
            lines.append(f"- {file_path}:{line} [{sev_item}] {cls} - {pattern}")

    lines.append("")
    lines.append("## Akzeptanzkriterien")
    lines.append("- [ ] Alle Critical/High Findings mit Repro und Fix verifiziert")
    lines.append("- [ ] Fokus-Tests oder relevante Integrations-Tests gruen")
    lines.append("- [ ] Release-Gate relevant: fehlende Tests/Benchmarks ergänzt oder bewusst begründet")
    lines.append("- [ ] Release-Gate relevant: fehlerhafte Tests/Benchmarks wieder grün")
    lines.append("- [ ] Betroffene Doku aktualisiert (ARCHITECTURE/README/SECURITY je nach Scope)")

    return "\n".join(lines) + "\n"


def issue_body_docs(module: str, row: dict[str, Any]) -> str:
    docs = row["docs"]
    impl = row["implementation"]
    missing = docs.get("missing_doc_types", [])
    alignment = row.get("developer_docs_alignment", {})

    lines: list[str] = []
    lines.append(f"# Soll-Ist Gap: Dokumentation ({module})")
    lines.append("")
    lines.append(f"soll-ist-key:docs:{module}")
    lines.append("")
    lines.append("## Ist-Zustand")
    lines.append(f"- Compliance Status: {docs.get('status', 'UNKNOWN')}")
    lines.append(f"- Compliance Score: {docs.get('score_percent', 0)}%")
    lines.append(f"- Drift via Status: {bool(docs.get('gap_open_by_status', False))}")
    lines.append(f"- Drift via Modul-Alignment: {bool(docs.get('gap_open_by_alignment', False))}")
    if missing:
        lines.append("- Fehlende Core-Dokumente: " + ", ".join(missing))
    else:
        lines.append("- Fehlende Core-Dokumente: keine")
    if alignment.get("missing_core_docs"):
        lines.append("- Modul-Alignment fehlende Core-Dokumente: " + ", ".join(alignment.get("missing_core_docs", [])))
    else:
        lines.append("- Modul-Alignment fehlende Core-Dokumente: keine")
    lines.append("")
    lines.append("## Soll-Zustand")
    lines.append("- Modul erfuellt das Core-Set gemaess Governance")
    lines.append("- Pflichtabschnitte je Dokumenttyp vorhanden")
    lines.append("- Aktualisierungsrhythmus gemaess Governance dokumentiert")
    lines.append("")
    lines.append("## Kontext")
    lines.append(f"- Offene Implementierungs-Findings parallel: {impl.get('actionable_total', 0)}")
    lines.append("")
    lines.append("## Akzeptanzkriterien")
    lines.append("- [ ] Alle fehlenden Core-Dokumente erstellt")
    lines.append("- [ ] Pflichtabschnitte validiert")
    lines.append("- [ ] Referenzen zwischen README/ARCHITECTURE/ROADMAP konsistent")
    lines.append("- [ ] Doxygen-Artefakte und Modul-DOXYGEN.md bei C++-Aenderungen aktualisiert")

    return "\n".join(lines) + "\n"


def upsert_issue(
    repo_root: Path,
    repo: str,
    open_issues: list[dict[str, Any]],
    title: str,
    body: str,
    key: str,
    apply: bool,
) -> tuple[str, int | None]:
    existing = find_issue_by_key(open_issues, key)
    if existing is not None:
        number = int(existing["number"])
        if apply:
            body_file = repo_root / "ai_context" / "developer_llm_wiki" / f".tmp_issue_{key.replace(':', '_')}.md"
            body_file.parent.mkdir(parents=True, exist_ok=True)
            body_file.write_text(body, encoding="utf-8")
            run_gh(["issue", "edit", str(number), "--repo", repo, "--title", title, "--body-file", str(body_file)], repo_root)
        return ("updated", number)

    if apply:
        body_file = repo_root / "ai_context" / "developer_llm_wiki" / f".tmp_issue_{key.replace(':', '_')}.md"
        body_file.parent.mkdir(parents=True, exist_ok=True)
        body_file.write_text(body, encoding="utf-8")
        out = run_gh(["issue", "create", "--repo", repo, "--title", title, "--body-file", str(body_file)], repo_root)
        match = re.search(r"/issues/(\d+)", out)
        num = int(match.group(1)) if match else None
        return ("created", num)

    return ("would-create", None)


def close_issue_if_open(repo_root: Path, repo: str, issue_number: int, apply: bool) -> str:
    if apply:
        run_gh(
            [
                "issue",
                "comment",
                str(issue_number),
                "--repo",
                repo,
                "--body",
                "Soll-Ist-Abgleich: Das Modul ist wieder konsistent; der Generator dokumentiert den Zustand, aber das Issue bleibt bewusst offen, damit der Betreiber die finale Freigabe manuell bestätigt.",
            ],
            repo_root,
        )
        return "commented-no-close"
    return "would-comment-no-close"


def sync_issues(
    repo_root: Path,
    repo: str,
    report: dict[str, Any],
    impl_map: dict[str, ModuleImplStatus],
    apply: bool,
    close_resolved: bool,
    max_findings: int,
) -> dict[str, Any]:
    open_issues = load_open_issues(repo_root, repo)
    actions: list[dict[str, Any]] = []

    for row in report.get("rows", []):
        module = row["module"]
        docs_open = bool(row["docs"].get("gap_open"))
        impl_open = bool(row["implementation"].get("gap_open"))

        impl_key = f"impl:{module}"
        docs_key = f"docs:{module}"

        impl_existing = find_issue_by_key(open_issues, impl_key)
        docs_existing = find_issue_by_key(open_issues, docs_key)

        if impl_open:
            title = f"[soll-ist][impl][{module}] Implementierungs-Gaps schliessen"
            body = issue_body_impl(module, row, impl_map.get(module), max_findings)
            action, num = upsert_issue(repo_root, repo, open_issues, title, body, impl_key, apply)
            actions.append({"module": module, "kind": "impl", "action": action, "issue": num})
        elif close_resolved and impl_existing is not None:
            num = int(impl_existing["number"])
            action = close_issue_if_open(repo_root, repo, num, apply)
            actions.append({"module": module, "kind": "impl", "action": action, "issue": num})

        if docs_open:
            title = f"[soll-ist][docs][{module}] Dokumentations-Gaps schliessen"
            body = issue_body_docs(module, row)
            action, num = upsert_issue(repo_root, repo, open_issues, title, body, docs_key, apply)
            actions.append({"module": module, "kind": "docs", "action": action, "issue": num})
        elif close_resolved and docs_existing is not None:
            num = int(docs_existing["number"])
            action = close_issue_if_open(repo_root, repo, num, apply)
            actions.append({"module": module, "kind": "docs", "action": action, "issue": num})

    return {
        "actions": actions,
        "totals": {
            "created": sum(1 for a in actions if a["action"] == "created"),
            "updated": sum(1 for a in actions if a["action"] == "updated"),
            "closed": sum(1 for a in actions if a["action"] == "closed"),
            "would_create": sum(1 for a in actions if a["action"] == "would-create"),
            "would_close": sum(1 for a in actions if a["action"] == "would-close"),
        },
    }


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()

    docs_map = parse_markdown_table(repo_root / args.docs_compliance_md)
    impl_map = parse_source_gaps(repo_root, args.source_gap_glob)
    all_modules = set(docs_map.keys()) | set(impl_map.keys())
    benchmark_log = (repo_root / args.benchmark_log).resolve() if args.benchmark_log else None
    quality_map = parse_quality_status(
        repo_root=repo_root,
        module_names=all_modules,
        ctest_log=(repo_root / args.ctest_log).resolve(),
        benchmark_log=benchmark_log,
    )

    report = build_report(repo_root, docs_map, impl_map, quality_map)

    # Attach detailed gate evidence per row for issue body rendering.
    for row in report.get("rows", []):
        q = quality_map.get(row["module"])
        row["release_gate_details"] = {
            "test_files": [] if q is None else q.test_files,
            "benchmark_files": [] if q is None else q.benchmark_files,
            "failing_tests": [] if q is None else q.failing_tests,
            "failing_benchmarks": [] if q is None else q.failing_benchmarks,
        }

    report_path = repo_root / args.report_json
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    write_summary(repo_root / args.summary_md, report)

    print(
        "Soll-Ist report generated: "
        f"modules={report['totals']['modules']} "
        f"docs_gap_modules={report['totals']['docs_gap_modules']} "
        f"impl_gap_modules={report['totals']['impl_gap_modules']}"
    )

    if args.sync_issues:
        sync_result = sync_issues(
            repo_root=repo_root,
            repo=args.repo,
            report=report,
            impl_map=impl_map,
            apply=args.apply,
            close_resolved=args.close_resolved,
            max_findings=args.max_findings_per_issue,
        )
        sync_path = repo_root / "ai_context" / "developer_llm_wiki" / "SOLL_IST_ISSUE_SYNC_RESULT.json"
        sync_path.parent.mkdir(parents=True, exist_ok=True)
        sync_path.write_text(json.dumps(sync_result, indent=2), encoding="utf-8")
        totals = sync_result["totals"]
        print(
            "Issue sync: "
            f"created={totals['created']} updated={totals['updated']} closed={totals['closed']} "
            f"would_create={totals['would_create']} would_close={totals['would_close']}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
