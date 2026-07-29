#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

TOOL_NAME = "ai-context-lint"
TOOL_VERSION = "1.0.0"
AUTO_START = "<!-- AUTO-CONFLICTS-START -->"
AUTO_END = "<!-- AUTO-CONFLICTS-END -->"

STALE_EXCLUDE_PREFIXES = (
    "ai_context/research/",
)

CONTRADICTION_EXCLUDE_PREFIXES = (
    "ai_context/research/",
)

CONTRADICTION_EXCLUDE_FILES = {
    "ai_context/COPILOT_INSTRUCTIONS.md",
    "ai_context/API_MODULE_STATUS_2026_07_18.md",
    "ai_context/KNOWLEDGE_LINT_REPORT.md",
}

SEVERITY_ORDER = {"critical": 4, "high": 3, "medium": 2, "low": 1}


@dataclass
class Finding:
    check: str
    severity: str
    file: str
    message: str


@dataclass
class Report:
    generated_at: str
    findings: List[Finding]
    check_counts: Dict[str, int]


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


def rel(path: Path, root: Path) -> str:
    return str(path.relative_to(root)).replace("\\", "/")


def collect_ai_markdown_files(repo_root: Path) -> List[Path]:
    ai_context = repo_root / "ai_context"
    files = sorted(p for p in ai_context.rglob("*.md") if p.is_file())
    return files


def _extract_md_links(content: str) -> List[str]:
    links: List[str] = []
    for match in re.finditer(r"\[[^\]]+\]\(([^)]+)\)", content):
        target = match.group(1).strip()
        if not target:
            continue
        if target.startswith("#"):
            continue
        if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*://", target):
            continue
        if target.startswith("mailto:"):
            continue
        links.append(target)
    return links


def check_link_integrity(repo_root: Path, files: List[Path]) -> List[Finding]:
    findings: List[Finding] = []
    for file_path in files:
        content = file_path.read_text(encoding="utf-8", errors="replace")
        for target in _extract_md_links(content):
            base = file_path.parent
            target_path = (base / target).resolve()
            if not target_path.exists():
                findings.append(
                    Finding(
                        check="Link Integrity",
                        severity="high",
                        file=rel(file_path, repo_root),
                        message=f"Broken link target: {target}",
                    )
                )
    return findings


def _extract_header_date(content: str) -> Optional[datetime]:
    match = re.search(r"^Datum:\s*(\d{4}-\d{2}-\d{2})\s*$", content, re.MULTILINE)
    if not match:
        return None
    try:
        return datetime.strptime(match.group(1), "%Y-%m-%d").replace(tzinfo=UTC)
    except ValueError:
        return None


def check_stale_claims(repo_root: Path, files: List[Path], stale_days: int) -> List[Finding]:
    findings: List[Finding] = []
    now = datetime.now(UTC)
    for file_path in files:
        file_rel = rel(file_path, repo_root)
        if file_rel.startswith(STALE_EXCLUDE_PREFIXES):
            continue

        content = file_path.read_text(encoding="utf-8", errors="replace")
        date_value = _extract_header_date(content)
        if date_value is None:
            findings.append(
                Finding(
                    check="Stale Claims",
                    severity="medium",
                    file=file_rel,
                    message="Missing or invalid 'Datum: YYYY-MM-DD' header.",
                )
            )
            continue
        age_days = (now - date_value).days
        if age_days > stale_days:
            severity = "high" if age_days > stale_days * 2 else "medium"
            findings.append(
                Finding(
                    check="Stale Claims",
                    severity=severity,
                    file=file_rel,
                    message=f"Document date is {age_days} days old (threshold: {stale_days}).",
                )
            )
    return findings


def check_orphans(repo_root: Path, files: List[Path]) -> List[Finding]:
    findings: List[Finding] = []
    docs_of_interest = [
        repo_root / "ai_context" / "KNOWLEDGE_LINT_REPORT.md",
        repo_root / "ai_context" / "KNOWLEDGE_CONFLICTS.md",
        repo_root / "ai_context" / "API_MODULE_STATUS_2026_07_18.md",
        repo_root / "ai_context" / "COPILOT_INSTRUCTIONS.md",
    ]
    anchors = [
        repo_root / "INDEX.md",
        repo_root / "AI_WIKI_INTEGRATION_PLAYBOOK.md",
        repo_root / "ai_context" / "README.md",
    ] + files

    backlink_count: Dict[str, int] = {str(p.resolve()): 0 for p in docs_of_interest if p.exists()}

    for source in anchors:
        if not source.exists() or not source.is_file():
            continue
        content = source.read_text(encoding="utf-8", errors="replace")
        for target in _extract_md_links(content):
            target_path = (source.parent / target).resolve()
            key = str(target_path)
            if key in backlink_count:
                backlink_count[key] += 1

    for doc in docs_of_interest:
        if not doc.exists():
            continue
        key = str(doc.resolve())
        if backlink_count.get(key, 0) == 0:
            findings.append(
                Finding(
                    check="Orphan Detection",
                    severity="medium",
                    file=rel(doc, repo_root),
                    message="No inbound references from INDEX/PLAYBOOK/ai_context docs.",
                )
            )
    return findings


def check_cross_references(repo_root: Path) -> List[Finding]:
    findings: List[Finding] = []
    index_path = repo_root / "INDEX.md"
    if not index_path.exists():
        return [
            Finding(
                check="Required Cross-References",
                severity="critical",
                file="INDEX.md",
                message="Missing INDEX.md.",
            )
        ]

    content = index_path.read_text(encoding="utf-8", errors="replace")
    required_links = [
        "ai_context/KNOWLEDGE_LINT_REPORT.md",
        "ai_context/KNOWLEDGE_CONFLICTS.md",
        "AI_WIKI_INTEGRATION_PLAYBOOK.md",
        "LOG.md",
    ]

    for link in required_links:
        if f"({link})" not in content:
            findings.append(
                Finding(
                    check="Required Cross-References",
                    severity="high",
                    file="INDEX.md",
                    message=f"Missing required link: {link}",
                )
            )
    return findings


def check_contradictions(repo_root: Path, files: List[Path]) -> List[Finding]:
    findings: List[Finding] = []

    conflicts_file = repo_root / "ai_context" / "KNOWLEDGE_CONFLICTS.md"
    if conflicts_file.exists():
        content = conflicts_file.read_text(encoding="utf-8", errors="replace")
        ids = re.findall(r"KCON-[A-Z0-9-]+|KCON-\d{4}", content)
        duplicates = sorted({item for item in ids if ids.count(item) > 1})
        for dup in duplicates:
            findings.append(
                Finding(
                    check="Contradiction Signals",
                    severity="medium",
                    file=rel(conflicts_file, repo_root),
                    message=f"Duplicate conflict ID detected: {dup}",
                )
            )

    placeholder_re = re.compile(r"<[A-Za-z0-9_\- /|:.]+>")
    for file_path in files:
        file_rel = rel(file_path, repo_root)
        if file_rel.startswith(CONTRADICTION_EXCLUDE_PREFIXES):
            continue
        if file_rel in CONTRADICTION_EXCLUDE_FILES:
            continue

        content = file_path.read_text(encoding="utf-8", errors="replace")
        if placeholder_re.search(content):
            findings.append(
                Finding(
                    check="Contradiction Signals",
                    severity="low",
                    file=file_rel,
                    message="Template placeholders detected; document may be incomplete.",
                )
            )
    return findings


def summarize_by_check(findings: List[Finding]) -> Dict[str, int]:
    counts: Dict[str, int] = {
        "Link Integrity": 0,
        "Orphan Detection": 0,
        "Stale Claims": 0,
        "Contradiction Signals": 0,
        "Required Cross-References": 0,
    }
    for item in findings:
        counts[item.check] = counts.get(item.check, 0) + 1
    return counts


def summarize_by_severity(findings: List[Finding]) -> Dict[str, int]:
    counts = {"critical": 0, "high": 0, "medium": 0, "low": 0}
    for item in findings:
        counts[item.severity] = counts.get(item.severity, 0) + 1
    return counts


def _status_for_count(count: int) -> str:
    if count == 0:
        return "PASS"
    if count <= 2:
        return "WARN"
    return "FAIL"


def _top_findings(findings: List[Finding], limit: int = 3) -> List[Finding]:
    return sorted(
        findings,
        key=lambda f: (SEVERITY_ORDER.get(f.severity, 0), f.check, f.file),
        reverse=True,
    )[:limit]


def render_markdown_report(repo_root: Path, report: Report) -> str:
    severity = summarize_by_severity(report.findings)

    checks_order = [
        "Link Integrity",
        "Orphan Detection",
        "Stale Claims",
        "Contradiction Signals",
        "Required Cross-References",
    ]

    detail_map: Dict[str, List[Finding]] = {key: [] for key in checks_order}
    for item in report.findings:
        detail_map.setdefault(item.check, []).append(item)

    def section(check_name: str, idx: int) -> str:
        items = detail_map.get(check_name, [])
        lines = [f"### {idx}. {check_name}", "", f"- Status: {_status_for_count(len(items))}", f"- Findings: {len(items)}", "- Details:"]
        if not items:
            lines.append("  - none")
        else:
            for finding in sorted(items, key=lambda f: (SEVERITY_ORDER.get(f.severity, 0), f.file), reverse=True)[:10]:
                lines.append(f"  - [{finding.severity}] {finding.file}: {finding.message}")
        lines.append("")
        return "\n".join(lines)

    actions: List[str] = []
    for finding in _top_findings(report.findings, limit=3):
        actions.append(f"Fix {finding.check.lower()} in {finding.file}: {finding.message}")
    while len(actions) < 3:
        actions.append("No additional mandatory action.")

    result = "READY"
    reason = "No blocking findings."
    if severity["critical"] > 0:
        result = "BLOCKED"
        reason = "Critical findings present."
    elif severity["high"] > 0:
        result = "REVIEW_REQUIRED"
        reason = "High-severity findings require review."

    date_value = datetime.now(UTC).strftime("%Y-%m-%d")

    body = [
        "# KNOWLEDGE Lint Report",
        "",
        f"Datum: {date_value}",
        "Status: Active",
        "Bezug: Automatisierter AI-Context-Lintlauf",
        "Primary (Quelle der Wahrheit): AI_WIKI_INTEGRATION_PLAYBOOK.md, DOCUMENTATION_GOVERNANCE.md, ai_context/COPILOT_INSTRUCTIONS.md",
        "",
        "---",
        "",
        "## Scope",
        "",
        "- Geltungsbereich: ai_context/, INDEX.md, AI_WIKI_INTEGRATION_PLAYBOOK.md",
        "- Laufart: scheduled oder ad-hoc",
        f"- Datenstand: {date_value}",
        "",
        "---",
        "",
        "## Execution Metadata",
        "",
        "- Run-ID: auto",
        f"- Ausfuehrungszeit: {report.generated_at}",
        "- Ausfuehrender Agent/Reviewer: automation",
        "- Branch: auto",
        "- Vergleichsbasis: repository working tree",
        "",
        "---",
        "",
        "## Checks",
        "",
    ]

    for i, check_name in enumerate(checks_order, start=1):
        body.append(section(check_name, i))

    body.extend(
        [
            "---",
            "",
            "## Severity Summary",
            "",
            f"- Critical: {severity['critical']}",
            f"- High: {severity['high']}",
            f"- Medium: {severity['medium']}",
            f"- Low: {severity['low']}",
            "",
            "---",
            "",
            "## Required Actions",
            "",
            f"1. {actions[0]}",
            f"2. {actions[1]}",
            f"3. {actions[2]}",
            "",
            "---",
            "",
            "## Closure Decision",
            "",
            f"- Resultat: {result}",
            f"- Begruendung: {reason}",
            f"- Sign-off: automation/{date_value}",
            "",
        ]
    )

    return "\n".join(body)


def render_conflicts_block(report: Report) -> str:
    severe = [f for f in report.findings if f.severity in {"critical", "high"}]
    date_value = datetime.now(UTC).strftime("%Y-%m-%d")

    lines = [
        "## Auto-Detected Conflicts (Managed)",
        "",
        f"Generated: {date_value}",
        "",
    ]

    if not severe:
        lines.append("- Keine automatisch detektierten High/Critical-Konflikte im letzten Lauf.")
        lines.append("")
        return "\n".join(lines)

    for idx, finding in enumerate(sorted(severe, key=lambda f: (SEVERITY_ORDER[f.severity], f.file), reverse=True), start=1):
        conflict_id = f"KCON-AUTO-{date_value.replace('-', '')}-{idx:02d}"
        lines.extend(
            [
                f"### {conflict_id}",
                "",
                f"- Datum: {date_value}",
                "- Status: OPEN",
                "- SOT-Domain: ai-context",
                f"- Betroffene Dateien: {finding.file}",
                f"- Claim A: Lint policy expects consistency for {finding.check}.",
                f"- Claim B: {finding.message}",
                "- Evidenz: automated lint report",
                f"- Risiko: {finding.severity}",
                "- Naechster Schritt: Review und Korrektur im naechsten AI-context Updatezyklus.",
                "",
            ]
        )

    return "\n".join(lines)


def apply_conflicts_update(repo_root: Path, block: str) -> None:
    path = repo_root / "ai_context" / "KNOWLEDGE_CONFLICTS.md"
    content = path.read_text(encoding="utf-8", errors="replace")

    managed = f"{AUTO_START}\n{block}\n{AUTO_END}"
    if AUTO_START in content and AUTO_END in content:
        content = re.sub(
            rf"{re.escape(AUTO_START)}.*?{re.escape(AUTO_END)}",
            managed,
            content,
            flags=re.DOTALL,
        )
    else:
        content = content.rstrip() + "\n\n---\n\n" + managed + "\n"

    path.write_text(content, encoding="utf-8")


def run_lint(repo_root: Path, stale_days: int) -> Report:
    files = collect_ai_markdown_files(repo_root)
    findings: List[Finding] = []

    findings.extend(check_link_integrity(repo_root, files))
    findings.extend(check_orphans(repo_root, files))
    findings.extend(check_stale_claims(repo_root, files, stale_days=stale_days))
    findings.extend(check_contradictions(repo_root, files))
    findings.extend(check_cross_references(repo_root))

    return Report(
        generated_at=datetime.now(UTC).isoformat(timespec="seconds"),
        findings=findings,
        check_counts=summarize_by_check(findings),
    )


def format_text(report: Report) -> str:
    severity = summarize_by_severity(report.findings)
    lines = [
        "-" * 72,
        f"  ThemisDB AI context lint  ·  {TOOL_NAME} v{TOOL_VERSION}",
        "-" * 72,
        "",
        f"  Generated at              : {report.generated_at}",
        f"  Total findings            : {len(report.findings):>4}",
        f"  Critical / High / Medium  : {severity['critical']:>2} / {severity['high']:>2} / {severity['medium']:>2}",
        f"  Low                       : {severity['low']:>4}",
        "",
    ]

    for check_name, count in report.check_counts.items():
        lines.append(f"  - {check_name:<26} {count:>4}")

    lines.append("")
    if report.findings:
        lines.append("Top findings")
        lines.append("-" * 72)
        for item in _top_findings(report.findings, limit=10):
            lines.append(f"  - [{item.severity}] {item.file}: {item.message}")
    else:
        lines.append("No findings.")

    lines.append("")
    return "\n".join(lines)


def format_json(report: Report) -> str:
    payload = {
        "tool": TOOL_NAME,
        "version": TOOL_VERSION,
        "generated_at": report.generated_at,
        "summary": {
            "findings": len(report.findings),
            "checks": report.check_counts,
            "severity": summarize_by_severity(report.findings),
        },
        "findings": [
            {
                "check": item.check,
                "severity": item.severity,
                "file": item.file,
                "message": item.message,
            }
            for item in report.findings
        ],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description="Run AI context lint checks and optionally update AI context knowledge artifacts.",
    )
    parser.add_argument("--repo-root", metavar="DIR", help="Repository root (auto-detected if omitted).")
    parser.add_argument("--format", choices=["text", "json"], default="text", help="Output format.")
    parser.add_argument("--output", metavar="PATH", help="Write report to this file instead of stdout.")
    parser.add_argument("--stale-days", type=int, default=30, help="Stale threshold in days for Datum header checks.")
    parser.add_argument("--apply-updates", action="store_true", help="Update ai_context/KNOWLEDGE_LINT_REPORT.md and ai_context/KNOWLEDGE_CONFLICTS.md.")
    parser.add_argument("--fail-on-findings", action="store_true", help="Exit 1 if findings are present.")
    return parser


def _write_outputs(repo_root: Path, report: Report) -> None:
    lint_path = repo_root / "ai_context" / "KNOWLEDGE_LINT_REPORT.md"
    lint_path.write_text(render_markdown_report(repo_root, report), encoding="utf-8")
    apply_conflicts_update(repo_root, render_conflicts_block(report))


def main(argv: Optional[List[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    repo_root = Path(args.repo_root).resolve() if args.repo_root else find_repo_root()

    if not repo_root.is_dir():
        print(f"ERROR: repo root not found: {repo_root}", file=sys.stderr)
        return 2

    report = run_lint(repo_root, stale_days=max(1, args.stale_days))

    if args.apply_updates:
        _write_outputs(repo_root, report)

    rendered = format_json(report) if args.format == "json" else format_text(report)
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    else:
        print(rendered)

    if args.fail_on_findings and report.findings:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
