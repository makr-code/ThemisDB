#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

TOOL_NAME = "ai-dev-llm-wiki-sync"
TOOL_VERSION = "1.0.0"

WIKI_ROOT = Path("ai_context/developer_llm_wiki")
STATUS_JSON = WIKI_ROOT / "WIKI_STATUS.json"
DELTA_MD = WIKI_ROOT / "WIKI_DELTA_REPORT.md"
INDEX_MD = WIKI_ROOT / "INDEX.md"
MODULES_MD = WIKI_ROOT / "MODULES_AND_APIS.md"
OPS_MD = WIKI_ROOT / "BUILD_TEST_CI_AND_OPERATIONS.md"
GOV_MD = WIKI_ROOT / "GOVERNANCE_AND_ROADMAP.md"
SOURCES_JSON = WIKI_ROOT / "SOURCE_MANIFEST.json"

ROOT_DOCS = [
    "ROADMAP.md",
    "FUTURE_ENHANCEMENTS.md",
    "RELEASE_STRATEGY.md",
    "DOCUMENTATION_GOVERNANCE.md",
    "BRANCHING_STRATEGY.md",
    "VERSIONING.md",
    "CHANGELOG.md",
    "AI_WIKI_INTEGRATION_PLAYBOOK.md",
    "INDEX.md",
]

STANDARD_SOURCE_GLOBS = [
    "docs/**/*.md",
    "ai_context/**/*.md",
    ".github/workflows/*.yml",
    "scripts/*.py",
    "src/**/README.md",
    "src/**/ROADMAP.md",
    "src/**/FUTURE_ENHANCEMENTS.md",
    "include/**/*.h",
    "include/**/*.hpp",
]

FULL_SYNC_EXTRA_GLOBS = [
    "src/**/*.md",
]

EXCLUDE_PREFIXES = (
    "plugins/private/",
    "ai_context/research/",
    "research/",
    "artifacts/",
    "audit/",
    "build/",
    "third_party/",
    "external/",
)

SECRET_PATTERNS = [
    re.compile(r"ghp_[A-Za-z0-9]{20,}"),
    re.compile(r"github_pat_[A-Za-z0-9_]{20,}"),
    re.compile(r"AKIA[0-9A-Z]{16}"),
    re.compile(r"(?i)api[_-]?key\s*[:=]\s*['\"][^'\"]{8,}['\"]"),
    re.compile(r"(?i)secret\s*[:=]\s*['\"][^'\"]{8,}['\"]"),
]

MARKDOWN_EXTENSIONS = {".md", ".markdown"}


@dataclass(frozen=True)
class SourceEntry:
    path: str
    category: str
    size: int
    sha256: str
    modified_utc: str


@dataclass
class ValidationFinding:
    severity: str
    check: str
    message: str


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


def read_text_safe(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def iter_globbed_files(repo_root: Path, patterns: Sequence[str]) -> Iterable[Path]:
    seen: set[Path] = set()
    for pattern in patterns:
        for candidate in sorted(repo_root.glob(pattern)):
            if not candidate.is_file():
                continue
            if candidate in seen:
                continue
            seen.add(candidate)
            yield candidate


def classify_source(path_rel: str) -> str:
    if path_rel.startswith("src/") or path_rel.startswith("include/"):
        return "modules_and_api"
    if path_rel.startswith(".github/workflows/") or path_rel.startswith("scripts/"):
        return "build_test_ci"
    if path_rel in ROOT_DOCS or path_rel.startswith("ai_context/") or path_rel.startswith("docs/"):
        return "governance_and_docs"
    return "misc"


def collect_sources(repo_root: Path, full_sync: bool) -> List[SourceEntry]:
    patterns = list(STANDARD_SOURCE_GLOBS)
    if full_sync:
        patterns.extend(FULL_SYNC_EXTRA_GLOBS)

    files: List[Path] = []
    for root_doc in ROOT_DOCS:
        path = repo_root / root_doc
        if path.is_file():
            files.append(path)
    files.extend(iter_globbed_files(repo_root, patterns))

    unique_files: Dict[str, Path] = {}
    for file_path in files:
        r = rel(file_path, repo_root)
        if r.startswith(EXCLUDE_PREFIXES):
            continue
        if "/.git/" in r or r.startswith(".git/"):
            continue
        unique_files[r] = file_path

    entries: List[SourceEntry] = []
    for path_rel in sorted(unique_files.keys()):
        abs_path = unique_files[path_rel]
        digest = hashlib.sha256(abs_path.read_bytes()).hexdigest()
        stat = abs_path.stat()
        entries.append(
            SourceEntry(
                path=path_rel,
                category=classify_source(path_rel),
                size=stat.st_size,
                sha256=digest,
                modified_utc=datetime.fromtimestamp(stat.st_mtime, UTC).isoformat(timespec="seconds"),
            )
        )
    return entries


def aggregate_hash(entries: Sequence[SourceEntry]) -> str:
    h = hashlib.sha256()
    for entry in entries:
        h.update(entry.path.encode("utf-8"))
        h.update(entry.sha256.encode("utf-8"))
    return h.hexdigest()


def load_previous_status(path: Path) -> Optional[dict]:
    if not path.exists():
        return None
    try:
        return json.loads(read_text_safe(path))
    except Exception:
        return None


def build_delta(prev: Optional[dict], current: Sequence[SourceEntry]) -> Dict[str, List[str]]:
    prev_map: Dict[str, str] = {}
    if prev and isinstance(prev.get("sources"), list):
        for item in prev["sources"]:
            if isinstance(item, dict) and isinstance(item.get("path"), str) and isinstance(item.get("sha256"), str):
                prev_map[item["path"]] = item["sha256"]

    curr_map = {entry.path: entry.sha256 for entry in current}
    prev_paths = set(prev_map.keys())
    curr_paths = set(curr_map.keys())

    added = sorted(curr_paths - prev_paths)
    removed = sorted(prev_paths - curr_paths)
    changed = sorted(p for p in (curr_paths & prev_paths) if curr_map[p] != prev_map[p])
    return {"added": added, "removed": removed, "changed": changed}


def first_heading(content: str) -> str:
    for line in content.splitlines():
        if line.startswith("#"):
            return line.lstrip("#").strip()
    return "(no heading)"


def content_excerpt(content: str, max_lines: int = 3) -> List[str]:
    lines: List[str] = []
    for line in content.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            continue
        normalized = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"\1 (\2)", stripped)
        lines.append(normalized)
        if len(lines) >= max_lines:
            break
    return lines


def render_index(entries: Sequence[SourceEntry], generated_at: str, full_sync: bool, aggregate_digest: str) -> str:
    by_cat: Dict[str, int] = {}
    for entry in entries:
        by_cat[entry.category] = by_cat.get(entry.category, 0) + 1

    mode = "FULL_SYNC" if full_sync else "INCREMENTAL"
    return "\n".join(
        [
            "# Developer LLM Wiki — Index",
            "",
            f"Datum: {generated_at[:10]}",
            "Status: Active",
            "Bezug: CI-verwaltete Entwickler-Wissensbasis fuer Coder-LLMs",
            "Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, ai_context/COPILOT_INSTRUCTIONS.md",
            "",
            "## Scope",
            "",
            "- Ziel: Onboarding- und Coding-relevantes Wissen fuer Entwickler-LLMs",
            f"- Laufmodus: {mode}",
            f"- Quellen gesamt: {len(entries)}",
            f"- Quellen-Hash: `{aggregate_digest}`",
            "",
            "## Artefakte",
            "",
            "- [MODULES_AND_APIS.md](MODULES_AND_APIS.md)",
            "- [BUILD_TEST_CI_AND_OPERATIONS.md](BUILD_TEST_CI_AND_OPERATIONS.md)",
            "- [GOVERNANCE_AND_ROADMAP.md](GOVERNANCE_AND_ROADMAP.md)",
            "- [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json)",
            "- [WIKI_STATUS.json](WIKI_STATUS.json)",
            "- [WIKI_DELTA_REPORT.md](WIKI_DELTA_REPORT.md)",
            "",
            "## Source Distribution",
            "",
            f"- modules_and_api: {by_cat.get('modules_and_api', 0)}",
            f"- build_test_ci: {by_cat.get('build_test_ci', 0)}",
            f"- governance_and_docs: {by_cat.get('governance_and_docs', 0)}",
            f"- misc: {by_cat.get('misc', 0)}",
            "",
            "## Source-Priority / Konfliktregel",
            "",
            "1. Root Governance/SOT-Dokumente",
            "2. Modul-ROADMAP/FUTURE_ENHANCEMENTS/README",
            "3. Sonstige docs/ und ai_context/ Wissensseiten",
            "4. CI-/Workflow- und Script-Metadaten",
            "",
            "Bei widerspruechlichen Aussagen wird markiert statt still ueberschrieben.",
            "",
        ]
    )


def _render_section(repo_root: Path, entries: Sequence[SourceEntry], title: str, category: str, limit: int = 80) -> List[str]:
    lines = [f"# {title}", "", f"Datum: {datetime.now(UTC).strftime('%Y-%m-%d')}", "Status: Active", ""]
    filtered = [entry for entry in entries if entry.category == category][:limit]
    if not filtered:
        lines.extend(["- Keine Quellen gefunden.", ""])
        return lines
    for entry in filtered:
        source_path = repo_root / entry.path
        heading = "(binary or unreadable)"
        excerpt: List[str] = []
        if source_path.suffix.lower() in MARKDOWN_EXTENSIONS:
            text = read_text_safe(source_path)
            heading = first_heading(text)
            excerpt = content_excerpt(text)
        lines.append(f"## {entry.path}")
        lines.append(f"- Kategorie: {entry.category}")
        lines.append(f"- Hash: `{entry.sha256[:16]}`")
        lines.append(f"- Titel: {heading}")
        if excerpt:
            lines.append("- Auszug:")
            for e in excerpt:
                lines.append(f"  - {e}")
        lines.append("")
    return lines


def render_modules_and_apis(repo_root: Path, entries: Sequence[SourceEntry]) -> str:
    lines = _render_section(repo_root, entries, "Developer LLM Wiki — Modules and APIs", "modules_and_api")
    return "\n".join(lines)


def render_ops(repo_root: Path, entries: Sequence[SourceEntry]) -> str:
    lines = _render_section(repo_root, entries, "Developer LLM Wiki — Build/Test/CI/Operations", "build_test_ci")
    return "\n".join(lines)


def render_governance(repo_root: Path, entries: Sequence[SourceEntry]) -> str:
    lines = _render_section(repo_root, entries, "Developer LLM Wiki — Governance and Roadmap", "governance_and_docs")
    return "\n".join(lines)


def _render_limited_list(items: Sequence[str], limit: int = 200) -> List[str]:
    if not items:
        return ["- none"]
    shown = [f"- {item}" for item in items[:limit]]
    if len(items) > limit:
        shown.append(f"- ... truncated ({len(items) - limit} more)")
    return shown


def render_delta_md(generated_at: str, delta: Dict[str, List[str]]) -> str:
    return "\n".join(
        [
            "# Developer LLM Wiki — Delta Report",
            "",
            f"Datum: {generated_at[:10]}",
            "Status: Active",
            "",
            "## Summary",
            "",
            f"- Added: {len(delta['added'])}",
            f"- Removed: {len(delta['removed'])}",
            f"- Changed: {len(delta['changed'])}",
            "",
            "## Added",
            "",
            *_render_limited_list(delta["added"]),
            "",
            "## Removed",
            "",
            *_render_limited_list(delta["removed"]),
            "",
            "## Changed",
            "",
            *_render_limited_list(delta["changed"]),
            "",
        ]
    )


def validate_outputs(repo_root: Path, files: Sequence[Path], entries: Sequence[SourceEntry]) -> List[ValidationFinding]:
    findings: List[ValidationFinding] = []

    for output_file in files:
        if not output_file.exists():
            findings.append(
                ValidationFinding(
                    severity="critical",
                    check="missing-output",
                    message=f"Missing output artifact: {rel(output_file, repo_root)}",
                )
            )
            continue
        content = read_text_safe(output_file)
        for pattern in SECRET_PATTERNS:
            if pattern.search(content):
                findings.append(
                    ValidationFinding(
                        severity="critical",
                        check="secret-patterns",
                        message=f"Potential secret pattern detected in {rel(output_file, repo_root)} ({pattern.pattern}).",
                    )
                )
        for match in re.finditer(r"\[[^\]]+\]\(([^)]+)\)", content):
            target = match.group(1).strip()
            if not target or target.startswith("#") or "://" in target:
                continue
            resolved = (output_file.parent / target).resolve()
            if not resolved.exists():
                findings.append(
                    ValidationFinding(
                        severity="medium",
                        check="broken-links",
                        message=f"Broken wiki link in {rel(output_file, repo_root)} -> {target}",
                    )
                )
    return findings


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def render_text_summary(
    generated_at: str,
    full_sync: bool,
    entries: Sequence[SourceEntry],
    delta: Dict[str, List[str]],
    findings: Sequence[ValidationFinding],
    outputs_written: bool,
) -> str:
    mode = "FULL_SYNC" if full_sync else "INCREMENTAL"
    lines = [
        "-" * 72,
        f"  ThemisDB Developer LLM Wiki Sync  ·  {TOOL_NAME} v{TOOL_VERSION}",
        "-" * 72,
        "",
        f"  Generated at       : {generated_at}",
        f"  Mode               : {mode}",
        f"  Sources            : {len(entries)}",
        f"  Added/Removed/Changed : {len(delta['added'])}/{len(delta['removed'])}/{len(delta['changed'])}",
        f"  Findings           : {len(findings)}",
        f"  Outputs updated    : {'yes' if outputs_written else 'no'}",
        "",
    ]
    for finding in findings[:10]:
        lines.append(f"  - [{finding.severity}] {finding.check}: {finding.message}")
    if not findings:
        lines.append("  - No validation findings.")
    lines.append("")
    return "\n".join(lines)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description="Build and validate ThemisDB developer LLM wiki artifacts in ai_context/.",
    )
    parser.add_argument("--repo-root", metavar="DIR", help="Repository root (auto-detected if omitted).")
    parser.add_argument("--full-sync", action="store_true", help="Enable broad source scan for initial global migration runs.")
    parser.add_argument("--apply-updates", action="store_true", help="Write or update wiki artifacts under ai_context/developer_llm_wiki/.")
    parser.add_argument("--output-text", metavar="PATH", help="Write text summary to file.")
    parser.add_argument("--output-json", metavar="PATH", help="Write JSON summary to file.")
    parser.add_argument("--fail-on-findings", action="store_true", help="Exit 1 when validation findings exist.")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    repo_root = Path(args.repo_root).resolve() if args.repo_root else find_repo_root()

    if not repo_root.is_dir():
        print(f"ERROR: repo root not found: {repo_root}", file=sys.stderr)
        return 2

    generated_at = datetime.now(UTC).isoformat(timespec="seconds")
    entries = collect_sources(repo_root, full_sync=args.full_sync)
    digest = aggregate_hash(entries)
    prev_status = load_previous_status(repo_root / STATUS_JSON)
    delta = build_delta(prev_status, entries)

    output_files = [
        repo_root / INDEX_MD,
        repo_root / MODULES_MD,
        repo_root / OPS_MD,
        repo_root / GOV_MD,
        repo_root / SOURCES_JSON,
        repo_root / STATUS_JSON,
        repo_root / DELTA_MD,
    ]

    if args.apply_updates:
        write_file(repo_root / INDEX_MD, render_index(entries, generated_at, args.full_sync, digest))
        write_file(repo_root / MODULES_MD, render_modules_and_apis(repo_root, entries))
        write_file(repo_root / OPS_MD, render_ops(repo_root, entries))
        write_file(repo_root / GOV_MD, render_governance(repo_root, entries))
        write_file(repo_root / DELTA_MD, render_delta_md(generated_at, delta))

        manifest_payload = {
            "tool": TOOL_NAME,
            "version": TOOL_VERSION,
            "generated_at": generated_at,
            "source_count": len(entries),
            "sources": [entry.__dict__ for entry in entries],
        }
        write_file(repo_root / SOURCES_JSON, json.dumps(manifest_payload, indent=2, ensure_ascii=False))

        manifest_path = repo_root / SOURCES_JSON
        manifest_hash = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        status_payload = {
            "tool": TOOL_NAME,
            "version": TOOL_VERSION,
            "generated_at": generated_at,
            "mode": "full_sync" if args.full_sync else "incremental",
            "source_count": len(entries),
            "source_hash": digest,
            "delta": {
                "added_count": len(delta["added"]),
                "removed_count": len(delta["removed"]),
                "changed_count": len(delta["changed"]),
                "added_preview": delta["added"][:50],
                "removed_preview": delta["removed"][:50],
                "changed_preview": delta["changed"][:50],
            },
            "manifest_path": str(SOURCES_JSON).replace("\\", "/"),
            "manifest_sha256": manifest_hash,
        }
        write_file(repo_root / STATUS_JSON, json.dumps(status_payload, indent=2, ensure_ascii=False))

    findings = validate_outputs(repo_root, output_files, entries) if args.apply_updates else []
    summary_text = render_text_summary(
        generated_at=generated_at,
        full_sync=args.full_sync,
        entries=entries,
        delta=delta,
        findings=findings,
        outputs_written=args.apply_updates,
    )

    payload = {
        "tool": TOOL_NAME,
        "version": TOOL_VERSION,
        "generated_at": generated_at,
        "mode": "full_sync" if args.full_sync else "incremental",
        "outputs_written": args.apply_updates,
        "source_count": len(entries),
        "source_hash": digest,
        "delta": delta,
        "findings": [finding.__dict__ for finding in findings],
    }

    if args.output_text:
        Path(args.output_text).write_text(summary_text, encoding="utf-8")
    else:
        print(summary_text)
    if args.output_json:
        Path(args.output_json).write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    if args.fail_on_findings and findings:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
