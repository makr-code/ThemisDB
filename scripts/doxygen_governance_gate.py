#!/usr/bin/env python3
"""Scoped Doxygen governance gate for changed C/C++ PR files."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, List

try:
    import yaml  # type: ignore
except Exception:  # pragma: no cover - optional import fallback
    yaml = None

from tools.scanners.gs3_step04_quality_cpp_doxygen import (
    ThemisCppDoxygenPolicyRulesScan,
)


CPP_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
DOXYGEN_CONFIG_NAME = "Doxyfile.audit"
RELEASE_BRANCHES = {"community", "military", "hyperscaler"}
WAIVER_LABEL = "governance/doxygen-waiver"
BLOCKING_PATTERNS = {
    "missing_doxygen_class",
    "missing_doxygen_comment",
    "missing_doxygen_brief",
    "missing_doxygen_param",
    "missing_doxygen_return",
}


@dataclass
class GateReport:
    verdict: str
    base_ref: str
    release_lane: bool
    changed_code_files: List[str]
    changed_non_code_files: List[str]
    scoped_modules: List[str]
    phase6_modules: List[str]
    coverage_threshold: float
    coverage_enforced: bool
    coverage_waived: bool
    coverage_percent: float | None
    structural_findings: List[dict]
    doxygen_warnings: List[str]
    doxygen_exit_code: int | None
    xml_index_exists: bool
    generated_config: str | None
    warning_log: str | None
    coverage_summary: str | None
    summary_markdown: str


def _run(cmd: list[str], cwd: Path, check: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=str(cwd),
        text=True,
        capture_output=True,
        check=check,
    )


def _normalize_paths(lines: Iterable[str]) -> List[str]:
    paths: List[str] = []
    for line in lines:
        value = line.strip().replace("\\", "/")
        if value:
            paths.append(value)
    return paths


def get_changed_files(repo_root: Path, base_ref: str) -> List[str]:
    candidates = [
        ["git", "diff", "--name-only", "--diff-filter=ACMR", f"origin/{base_ref}...HEAD"],
        ["git", "diff", "--name-only", "--diff-filter=ACMR", f"{base_ref}...HEAD"],
        ["git", "diff", "--name-only", "--diff-filter=ACMR", base_ref],
    ]
    for cmd in candidates:
        result = _run(cmd, repo_root)
        if result.returncode == 0:
            return _normalize_paths(result.stdout.splitlines())
    return []


def _is_cpp_file(rel_path: str) -> bool:
    return Path(rel_path).suffix.lower() in CPP_EXTENSIONS


def _module_name(rel_path: str) -> str | None:
    parts = Path(rel_path).parts
    if len(parts) >= 2 and parts[0] in {"src", "include"}:
        return parts[1]
    return None


def _phase6_complete(roadmap_path: Path) -> bool:
    if not roadmap_path.exists():
        return False
    text = roadmap_path.read_text(encoding="utf-8", errors="ignore")
    if re.search(r"Milestone:\s*All Phase 1-6 deliverables complete", text, re.IGNORECASE):
        return True
    phase6_match = re.search(r"### Phase 6.*?(?=\n### |\Z)", text, re.IGNORECASE | re.DOTALL)
    if not phase6_match:
        return False
    phase6_section = phase6_match.group(0)
    return bool(
        re.search(r"Status:\s*[✓✅]?\s*COMPLETE", phase6_section, re.IGNORECASE)
        or re.search(r"COMPLETE\s*[✓✅]", phase6_section, re.IGNORECASE)
    )


def load_coverage_threshold(repo_root: Path) -> float:
    cfg = repo_root / ".github" / "ci-scope-config.yaml"
    if cfg.exists():
        text = cfg.read_text(encoding="utf-8", errors="ignore")
        if yaml is not None:
            try:
                payload = yaml.safe_load(text) or {}
                return float(payload.get("quality_gates", {}).get("docs_coverage_threshold", 95))
            except Exception:
                pass
        match = re.search(r"docs_coverage_threshold:\s*([0-9]+(?:\.[0-9]+)?)", text)
        if match:
            return float(match.group(1))
    return 95.0


def _selected_scope_paths(repo_root: Path, changed_code_files: List[str]) -> List[Path]:
    modules = sorted({name for path in changed_code_files if (name := _module_name(path))})
    scope: List[Path] = []
    for module in modules:
        include_dir = repo_root / "include" / module
        src_dir = repo_root / "src" / module
        if include_dir.is_dir():
            scope.append(include_dir)
        if src_dir.is_dir():
            scope.append(src_dir)
    if scope:
        return scope

    parents = sorted({(repo_root / rel).parent for rel in changed_code_files if (repo_root / rel).exists()})
    return parents


def _render_input_paths(paths: List[Path]) -> str:
    return " ".join(f'"{path.as_posix()}"' for path in paths)


def write_scoped_doxyfile(repo_root: Path, artifact_dir: Path, scope_paths: List[Path]) -> tuple[Path, Path, Path]:
    artifact_dir.mkdir(parents=True, exist_ok=True)
    output_dir = artifact_dir / "doxygen"
    output_dir.mkdir(parents=True, exist_ok=True)
    warn_log = output_dir / "doxygen-warnings.log"
    xml_index = output_dir / "xml" / "index.xml"
    base_cfg = repo_root / DOXYGEN_CONFIG_NAME
    content = base_cfg.read_text(encoding="utf-8", errors="ignore")
    content += (
        "\n# Appended by scripts/doxygen_governance_gate.py\n"
        f"OUTPUT_DIRECTORY = {output_dir.as_posix()}\n"
        f"WARN_LOGFILE = {warn_log.as_posix()}\n"
        "GENERATE_HTML = NO\n"
        "GENERATE_LATEX = NO\n"
        "GENERATE_XML = YES\n"
        "XML_OUTPUT = xml\n"
        f"INPUT = {_render_input_paths(scope_paths)}\n"
    )
    generated_cfg = artifact_dir / "Doxyfile.gate"
    generated_cfg.write_text(content, encoding="utf-8")
    return generated_cfg, warn_log, xml_index


def run_structural_scan(repo_root: Path, changed_code_files: List[str]) -> List[dict]:
    scanner = ThemisCppDoxygenPolicyRulesScan(str(repo_root))
    findings = scanner.scan_files([Path(path) for path in changed_code_files])
    return [finding for finding in findings if finding.get("pattern") in BLOCKING_PATTERNS]


def parse_warning_lines(raw_lines: Iterable[str]) -> List[str]:
    warnings: List[str] = []
    for line in raw_lines:
        stripped = line.strip()
        if stripped:
            warnings.append(stripped)
    return warnings


def extract_coverage_percent(summary_text: str) -> float | None:
    match = re.search(r"(\d+(?:\.\d+)?)\s*%", summary_text)
    return float(match.group(1)) if match else None


def build_summary(report: GateReport) -> str:
    lines = [
        "## Doxygen Governance Gate",
        "",
        f"- Verdict: **{report.verdict}**",
        f"- Base branch: `{report.base_ref}`",
        f"- Changed C/C++ files: `{len(report.changed_code_files)}`",
        f"- Scoped modules: `{', '.join(report.scoped_modules) if report.scoped_modules else 'none'}`",
        f"- Release lane: `{'yes' if report.release_lane else 'no'}`",
        f"- Phase 6 modules in scope: `{', '.join(report.phase6_modules) if report.phase6_modules else 'none'}`",
        f"- Coverage enforced: `{'yes' if report.coverage_enforced else 'no'}`",
        f"- Coverage threshold: `{report.coverage_threshold:.1f}%`",
        f"- Coverage result: `{f'{report.coverage_percent:.2f}%' if report.coverage_percent is not None else 'skipped'}`",
        f"- Coverage waiver active: `{'yes' if report.coverage_waived else 'no'}`",
        f"- Structural findings: `{len(report.structural_findings)}`",
        f"- Doxygen warnings: `{len(report.doxygen_warnings)}`",
        f"- XML generated: `{'yes' if report.xml_index_exists else 'no'}`",
        "",
    ]

    if report.changed_code_files:
        lines.append("### Changed source files")
        lines.extend([f"- `{path}`" for path in report.changed_code_files[:20]])
        if len(report.changed_code_files) > 20:
            lines.append(f"- `... {len(report.changed_code_files) - 20} more`")
        lines.append("")

    if report.structural_findings:
        lines.append("### Blocking structural findings")
        for finding in report.structural_findings[:20]:
            lines.append(
                f"- `{finding['file']}:{finding['line']}` — {finding['description']}"
            )
        if len(report.structural_findings) > 20:
            lines.append(f"- `... {len(report.structural_findings) - 20} more`")
        lines.append("")

    if report.doxygen_warnings:
        lines.append("### Doxygen warnings")
        for warning in report.doxygen_warnings[:20]:
            lines.append(f"- `{warning}`")
        if len(report.doxygen_warnings) > 20:
            lines.append(f"- `... {len(report.doxygen_warnings) - 20} more`")
        lines.append("")

    if report.coverage_enforced and report.verdict != "PASS":
        lines.append("### Escalation")
        lines.append(
            "- Coverage on release-lane or Phase-6 scope failed the Tier-1 threshold."
        )
        lines.append(
            f"- Remediation: add public API Doxygen coverage or use `/approve-with-waiver T1-DOXYGEN-COVERAGE \"justification\"` and re-run after authorized waiver handling."
        )
        if not report.coverage_waived:
            lines.append(f"- Temporary label for approved override: `{WAIVER_LABEL}`")
        lines.append("")

    return "\n".join(lines).strip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the ThemisDB Doxygen governance gate.")
    parser.add_argument("--repo-root", default=".", help="Repository root")
    parser.add_argument("--base-ref", required=True, help="Base branch/ref for diffing")
    parser.add_argument("--artifact-dir", required=True, help="Directory for gate artifacts")
    parser.add_argument("--report-json", required=True, help="Path to JSON report")
    parser.add_argument("--summary-md", required=True, help="Path to markdown summary")
    parser.add_argument("--pr-labels", default="", help="Comma-separated PR labels")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    artifact_dir = Path(args.artifact_dir).resolve()
    artifact_dir.mkdir(parents=True, exist_ok=True)

    changed_files = get_changed_files(repo_root, args.base_ref)
    changed_code_files = [path for path in changed_files if _is_cpp_file(path)]
    changed_non_code_files = [path for path in changed_files if path not in changed_code_files]
    scoped_modules = sorted({name for path in changed_code_files if (name := _module_name(path))})
    phase6_modules = [
        module
        for module in scoped_modules
        if _phase6_complete(repo_root / "src" / module / "ROADMAP.md")
    ]

    threshold = load_coverage_threshold(repo_root)
    base_ref = args.base_ref
    release_lane = base_ref in RELEASE_BRANCHES
    waiver_active = WAIVER_LABEL in {label.strip() for label in args.pr_labels.split(",") if label.strip()}
    coverage_enforced = release_lane or bool(phase6_modules)

    generated_config: Path | None = None
    warning_log: Path | None = None
    coverage_summary_path: Path | None = None
    xml_index: Path | None = None
    doxygen_exit_code: int | None = None
    xml_index_exists = False
    coverage_percent: float | None = None
    doxygen_warnings: List[str] = []
    structural_findings: List[dict] = []
    verdict = "PASS"

    if changed_code_files:
        structural_findings = run_structural_scan(repo_root, changed_code_files)
        scope_paths = _selected_scope_paths(repo_root, changed_code_files)
        generated_config, warning_log, xml_index = write_scoped_doxyfile(
            repo_root, artifact_dir, scope_paths
        )

        doxygen_proc = _run(["doxygen", str(generated_config)], repo_root)
        doxygen_exit_code = doxygen_proc.returncode

        if warning_log.exists():
            doxygen_warnings = parse_warning_lines(warning_log.read_text(encoding="utf-8", errors="ignore").splitlines())
        else:
            combined = (doxygen_proc.stdout or "") + "\n" + (doxygen_proc.stderr or "")
            doxygen_warnings = parse_warning_lines(
                line for line in combined.splitlines() if "warning:" in line.lower()
            )

        xml_index_exists = bool(xml_index and xml_index.exists())
        coverage_summary_path = artifact_dir / "doxygen-coverage-summary.txt"

        if doxygen_exit_code == 0 and xml_index_exists:
            coverage_proc = _run(
                [
                    "python3",
                    "-m",
                    "coverxygen",
                    "--xml-dir",
                    str(xml_index.parent),
                    "--src-dir",
                    str(repo_root),
                    "--format",
                    "summary",
                ],
                repo_root,
            )
            coverage_summary_path.write_text(
                (coverage_proc.stdout or "") + (coverage_proc.stderr or ""),
                encoding="utf-8",
            )
            if coverage_proc.returncode == 0:
                coverage_percent = extract_coverage_percent(coverage_summary_path.read_text(encoding="utf-8", errors="ignore"))
            else:
                doxygen_warnings.append(
                    f"coverxygen failed with exit code {coverage_proc.returncode}"
                )

        if structural_findings or doxygen_exit_code != 0 or not xml_index_exists or doxygen_warnings:
            verdict = "FAIL"
        elif coverage_enforced and coverage_percent is not None and coverage_percent + 1e-9 < threshold:
            verdict = "WARN" if waiver_active else "FAIL"
        elif coverage_percent is None and coverage_enforced:
            verdict = "FAIL"
        elif not coverage_enforced and coverage_percent is not None and coverage_percent + 1e-9 < threshold:
            verdict = "WARN"
    else:
        doxygen_warnings = ["No changed C/C++ files in scope; Doxygen gate skipped."]

    report = GateReport(
        verdict=verdict,
        base_ref=base_ref,
        release_lane=release_lane,
        changed_code_files=changed_code_files,
        changed_non_code_files=changed_non_code_files,
        scoped_modules=scoped_modules,
        phase6_modules=phase6_modules,
        coverage_threshold=threshold,
        coverage_enforced=coverage_enforced,
        coverage_waived=waiver_active and verdict == "WARN",
        coverage_percent=coverage_percent,
        structural_findings=structural_findings,
        doxygen_warnings=doxygen_warnings,
        doxygen_exit_code=doxygen_exit_code,
        xml_index_exists=xml_index_exists,
        generated_config=str(generated_config) if generated_config else None,
        warning_log=str(warning_log) if warning_log else None,
        coverage_summary=str(coverage_summary_path) if coverage_summary_path else None,
        summary_markdown="",
    )
    report.summary_markdown = build_summary(report)

    Path(args.report_json).write_text(
        json.dumps(asdict(report), indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    Path(args.summary_md).write_text(report.summary_markdown, encoding="utf-8")

    if verdict == "FAIL":
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
