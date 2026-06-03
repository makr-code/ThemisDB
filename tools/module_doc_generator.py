#!/usr/bin/env python3
"""
Module Gap Documentation Generator (New Style)

Primary input format:
- ai_working/gap_scan_results.json (uniform scanner output with top-level "gaps")

Fallback input format:
- ai_working/gap_scan_v3_aggregate.json (legacy module aggregate)

Outputs:
- ai_working/module_gaps/<module>_GAPS.md
- src/<module>/MODULE_GAPS.md (optional mirror)
"""

from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Iterable, List


SEVERITY_ORDER = {
    "CRITICAL": 0,
    "HIGH": 1,
    "MEDIUM": 2,
    "LOW": 3,
    "INFO": 4,
    "INTENTIONAL": 5,
    "N/A": 6,
}

EXTRA_CODE_SCOPES = {"include", "tests", "benchmarks", "internal"}
CORE_RELATIVE_HINTS = {"concerns", "adapters"}
UNSCOPED_BUCKET = "_unscoped"
IGNORED_SRC_SCOPE_NAMES = {"ai_working"}


@dataclass(frozen=True)
class ModuleSummary:
    name: str
    total: int
    critical: int
    high: int
    medium: int
    low: int
    by_category: Dict[str, int]
    by_file: Dict[str, List[Dict[str, Any]]]

    @property
    def actionable(self) -> int:
        return self.critical + self.high

    @property
    def affected_files(self) -> int:
        return len(self.by_file)


class ModuleDocumentationGenerator:
    """Generate module-local gap documentation from scanner artifacts."""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root).resolve()
        self.src_dir = self.repo_root / "src"
        self.src_modules = self._discover_src_modules()
        self.doc_scopes: List[str] = list(self.src_modules)
        self.scan_results: Dict[str, Dict[str, Any]] = {}
        self.input_source: str = "unknown"

    def _discover_src_modules(self) -> List[str]:
        if not self.src_dir.exists():
            return []
        modules: List[str] = []
        for entry in sorted(self.src_dir.iterdir()):
            if (
                entry.is_dir()
                and not entry.name.startswith("_")
                and entry.name not in IGNORED_SRC_SCOPE_NAMES
            ):
                modules.append(entry.name)
        return modules

    @staticmethod
    def _normalize_path(path: str) -> str:
        return str(path).replace("\\", "/")

    def _module_from_path(self, file_path: str) -> str | None:
        normalized = self._normalize_path(file_path)
        normalized = normalized.lstrip("./")
        parts = [part for part in normalized.split("/") if part]
        if not parts:
            return None

        if parts[0] == "src" and len(parts) >= 2:
            module = parts[1]
            if module in self.src_modules:
                return module

        # Some scanners emit paths relative to src/ (e.g. core/..., security/...).
        if parts[0] in self.src_modules:
            return parts[0]

        if parts[0] in EXTRA_CODE_SCOPES:
            return parts[0]

        # Scans started at src/core may emit paths like concerns/... or adapters/...
        # Keep these with core instead of creating artificial pseudo-modules.
        if parts[0] in CORE_RELATIVE_HINTS and "core" in self.src_modules:
            return "core"

        if normalized.startswith("src/"):
            nested = normalized.split("/", 2)
            if len(nested) >= 2:
                module = nested[1]
                if module in self.src_modules:
                    return module

        return UNSCOPED_BUCKET

    def _summarize_from_grouped_findings(
        self,
        grouped: Dict[str, List[Dict[str, Any]]],
    ) -> Dict[str, Dict[str, Any]]:
        module_payload: Dict[str, Dict[str, Any]] = {}
        scopes = sorted(set(self.src_modules) | set(grouped.keys()))
        self.doc_scopes = scopes
        for module in scopes:
            findings = grouped.get(module, [])
            by_file: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
            by_category: Counter[str] = Counter()
            sev_counter: Counter[str] = Counter()

            for finding in findings:
                file_path = self._normalize_path(str(finding.get("file", "")))
                by_file[file_path].append(finding)

                cat = str(
                    finding.get("category")
                    or finding.get("type")
                    or finding.get("pattern")
                    or "uncategorized"
                )
                by_category[cat] += 1

                sev = str(finding.get("severity", "INFO")).upper()
                sev_counter[sev] += 1

            module_payload[module] = {
                "total": len(findings),
                "severity_critical": sev_counter.get("CRITICAL", 0),
                "severity_high": sev_counter.get("HIGH", 0),
                "severity_medium": sev_counter.get("MEDIUM", 0),
                "severity_low": sev_counter.get("LOW", 0),
                "by_category": dict(by_category),
                "by_file": dict(by_file),
            }
        return module_payload

    def _load_uniform_gaps(self, scan_path: Path) -> bool:
        results_path = scan_path / "gap_scan_results.json"
        if not results_path.exists():
            return False

        try:
            data = json.loads(results_path.read_text(encoding="utf-8"))
        except Exception as exc:
            print(f"[FAIL] Could not parse {results_path}: {exc}")
            return False

        gaps = data.get("gaps")
        if not isinstance(gaps, list):
            print(f"[FAIL] Unexpected format in {results_path}: missing top-level gaps[]")
            return False

        grouped: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
        for finding in gaps:
            if not isinstance(finding, dict):
                continue
            module = self._module_from_path(str(finding.get("file", "")))
            if module:
                grouped[module].append(finding)

        self.scan_results = self._summarize_from_grouped_findings(grouped)
        self.input_source = str(results_path.relative_to(self.repo_root))
        return True

    def _load_legacy_aggregate(self, scan_path: Path) -> bool:
        aggregate_path = scan_path / "gap_scan_v3_aggregate.json"
        if not aggregate_path.exists():
            return False

        try:
            data = json.loads(aggregate_path.read_text(encoding="utf-8"))
        except Exception as exc:
            print(f"[FAIL] Could not parse {aggregate_path}: {exc}")
            return False

        if not isinstance(data, dict):
            print(f"[FAIL] Unexpected aggregate format in: {aggregate_path}")
            return False

        src_set = set(self.src_modules)
        self.scan_results = {
            module: payload
            for module, payload in data.items()
            if module in src_set and isinstance(payload, dict)
        }
        self.doc_scopes = sorted(set(self.src_modules) | set(self.scan_results.keys()))
        self.input_source = str(aggregate_path.relative_to(self.repo_root))
        return bool(self.scan_results)

    def load_scan_results(self, scan_dir: str | Path) -> bool:
        scan_path = Path(scan_dir)
        if not scan_path.is_absolute():
            scan_path = self.repo_root / scan_path

        if self._load_uniform_gaps(scan_path):
            return True
        return self._load_legacy_aggregate(scan_path)

    def _summarize_module(self, module_name: str) -> ModuleSummary:
        payload = self.scan_results.get(module_name, {})
        return ModuleSummary(
            name=module_name,
            total=int(payload.get("total", 0) or 0),
            critical=int(payload.get("severity_critical", 0) or 0),
            high=int(payload.get("severity_high", 0) or 0),
            medium=int(payload.get("severity_medium", 0) or 0),
            low=int(payload.get("severity_low", 0) or 0),
            by_category=dict(payload.get("by_category", {}) or {}),
            by_file=dict(payload.get("by_file", {}) or {}),
        )

    @staticmethod
    def _health_status(summary: ModuleSummary) -> str:
        if summary.total == 0:
            return "No Findings"
        if summary.critical > 0:
            return "Critical Findings Present"
        if summary.high > 0:
            return "High-Priority Findings Present"
        return "Findings Present"

    def _sort_findings(self, findings: Iterable[Dict[str, Any]]) -> List[Dict[str, Any]]:
        return sorted(
            findings,
            key=lambda item: (
                SEVERITY_ORDER.get(str(item.get("severity", "INFO")).upper(), 99),
                int(item.get("line", 0) or 0),
                str(item.get("category") or item.get("type") or item.get("pattern") or ""),
            ),
        )

    def _sort_files(self, by_file: Dict[str, List[Dict[str, Any]]]) -> List[tuple[str, List[Dict[str, Any]]]]:
        return sorted(by_file.items(), key=lambda entry: (-len(entry[1]), self._normalize_path(entry[0])))

    def _format_finding(self, finding: Dict[str, Any]) -> str:
        severity = str(finding.get("severity", "INFO")).upper()
        line = int(finding.get("line", 0) or 0)
        category = str(finding.get("category") or finding.get("type") or "uncategorized")
        pattern = str(finding.get("pattern") or "")
        desc = str(finding.get("description") or "").strip()
        remediation = str(finding.get("remediation") or "").strip()
        context = str(finding.get("context") or finding.get("snippet") or "").strip()
        scanner = str(finding.get("scanner") or "").strip()

        head = f"- Line {line}: severity={severity}; category={category}"
        if pattern:
            head += f"; pattern={pattern}"
        lines = [head]
        if desc:
            lines.append(f"  Description: {desc}")
        if remediation:
            lines.append(f"  Remediation: {remediation}")
        if scanner:
            lines.append(f"  Scanner: {scanner}")
        if context:
            lines.append(f"  Context: {context}")
        return "\n".join(lines)

    def _generate_doc_content(self, summary: ModuleSummary) -> str:
        generated = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        out: List[str] = [
            f"# {summary.name} Module - Developer Gap Note",
            "",
            f"> Auto-generated from {self.input_source}.",
            "> This file is overwritten on each regeneration.",
            "",
            "## Scan Snapshot",
            "",
            f"- Module: {summary.name}",
            f"- Generated: {generated}",
            f"- Status: {self._health_status(summary)}",
            f"- Total Findings: {summary.total}",
            f"- Actionable Findings (Critical + High): {summary.actionable}",
            f"- Affected Files: {summary.affected_files}",
            "",
            "## Severity Summary",
            "",
            "| Severity | Count |",
            "|---|---:|",
            f"| Critical | {summary.critical} |",
            f"| High | {summary.high} |",
            f"| Medium | {summary.medium} |",
            f"| Low | {summary.low} |",
            "",
            "## Category Summary",
            "",
            "| Category | Count |",
            "|---|---:|",
        ]

        for category, count in sorted(summary.by_category.items(), key=lambda item: (-item[1], item[0])):
            out.append(f"| {category} | {count} |")

        if not summary.by_category:
            out.append("| none | 0 |")

        out.extend([
            "",
            "## File Overview",
            "",
            "| File | Findings | Critical | High | Medium | Low |",
            "|---|---:|---:|---:|---:|---:|",
        ])

        for file_path, findings in self._sort_files(summary.by_file):
            sev_counter = Counter(str(item.get("severity", "INFO")).upper() for item in findings)
            out.append(
                f"| {self._normalize_path(file_path)} | {len(findings)} | "
                f"{sev_counter.get('CRITICAL', 0)} | {sev_counter.get('HIGH', 0)} | "
                f"{sev_counter.get('MEDIUM', 0)} | {sev_counter.get('LOW', 0)} |"
            )

        if not summary.by_file:
            out.append("| none | 0 | 0 | 0 | 0 | 0 |")

        out.extend(["", "## Full Scanner Findings", ""])

        if not summary.by_file:
            out.append("No findings recorded for this module.")
        else:
            for file_path, findings in self._sort_files(summary.by_file):
                out.append(f"### {self._normalize_path(file_path)}")
                out.append(f"Total findings: {len(findings)}")
                out.append("")
                for finding in self._sort_findings(findings):
                    out.append(self._format_finding(finding))
                out.append("")

        out.extend([
            "## Update Workflow",
            "",
            "- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json",
            "- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps",
            "- Add --no-mirror when you only want archive docs in ai_working/module_gaps.",
            "",
            "Format: THEMIS_MODULE_GAPS_V4",
        ])

        return "\n".join(out).rstrip() + "\n"

    def _write_doc_pair(self, module_name: str, content: str, output_dir: Path, mirror: bool) -> bool:
        output_dir.mkdir(parents=True, exist_ok=True)
        archive_path = output_dir / f"{module_name}_GAPS.md"
        archive_path.write_text(content, encoding="utf-8")

        if mirror:
            module_path = self.src_dir / module_name / "MODULE_GAPS.md"
            module_path.parent.mkdir(parents=True, exist_ok=True)
            module_path.write_text(content, encoding="utf-8")
        return True

    def generate_module_docs(self, module_name: str, output_dir: str | Path | None = None, mirror: bool = True) -> bool:
        if module_name not in self.doc_scopes:
            return False
        summary = self._summarize_module(module_name)
        content = self._generate_doc_content(summary)
        archive_dir = Path(output_dir) if output_dir else (self.repo_root / "ai_working" / "module_gaps")
        if not archive_dir.is_absolute():
            archive_dir = self.repo_root / archive_dir
        return self._write_doc_pair(module_name, content, archive_dir, mirror=mirror and module_name in self.src_modules)

    def generate_all_module_docs(self, output_dir: str | Path | None = None, mirror: bool = True) -> Dict[str, bool]:
        results: Dict[str, bool] = {}
        for module_name in self.doc_scopes:
            try:
                success = self.generate_module_docs(module_name, output_dir=output_dir, mirror=mirror)
            except Exception:
                success = False
            results[module_name] = success
            print(f"  {'[OK]' if success else '[FAIL]'} {module_name}")
        return results

    def generate_module_index(self, output_dir: str | Path) -> bool:
        out_dir = Path(output_dir)
        if not out_dir.is_absolute():
            out_dir = self.repo_root / out_dir
        out_dir.mkdir(parents=True, exist_ok=True)

        summaries = [self._summarize_module(m) for m in self.doc_scopes]
        summaries.sort(key=lambda item: (-item.total, item.name))

        lines = [
            "# Module Gap Documentation Index",
            "",
            f"Updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
            f"Source: {self.input_source}",
            "",
            "| Module | Total | Critical | High | Medium | Low | Affected Files |",
            "|---|---:|---:|---:|---:|---:|---:|",
        ]

        for summary in summaries:
            lines.append(
                f"| [{summary.name}]({summary.name}_GAPS.md) | {summary.total} | {summary.critical} | "
                f"{summary.high} | {summary.medium} | {summary.low} | {summary.affected_files} |"
            )

        (out_dir / "MODULE_GAPS_INDEX.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
        return True


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate gap docs for src modules and other code scopes from scan artifacts")
    parser.add_argument("repo_root", nargs="?", default=".", help="Repository root")
    parser.add_argument("scan_dir", nargs="?", default="ai_working", help="Directory with scan artifacts")
    parser.add_argument("output_dir", nargs="?", default="ai_working/module_gaps", help="Archive output dir")
    parser.add_argument("--module", help="Generate docs for one module only")
    parser.add_argument("--no-mirror", action="store_true", help="Do not mirror files into src/<module>/MODULE_GAPS.md")
    return parser.parse_args()


if __name__ == "__main__":
    args = _parse_args()
    generator = ModuleDocumentationGenerator(args.repo_root)

    print("[INFO] Module Gap Documentation Generator (New Style)")
    print("=" * 70)

    if not generator.load_scan_results(args.scan_dir):
        print("[FAIL] No compatible scan artifacts found")
        raise SystemExit(1)

    print(f"[OK] Loaded scanner data from: {generator.input_source}")
    print(f"[OK] Discovered src modules: {len(generator.src_modules)}")
    print(f"[OK] Discovered documentation scopes: {len(generator.doc_scopes)}")

    mirror = not args.no_mirror
    if args.module:
        success = generator.generate_module_docs(args.module, output_dir=args.output_dir, mirror=mirror)
        print(f"[{'OK' if success else 'FAIL'}] Module: {args.module}")
    else:
        results = generator.generate_all_module_docs(output_dir=args.output_dir, mirror=mirror)
        ok_count = sum(1 for value in results.values() if value)
        print(f"[OK] Generated {ok_count}/{len(results)} module docs")

    if generator.generate_module_index(args.output_dir):
        print(f"[OK] Index: {Path(args.output_dir) / 'MODULE_GAPS_INDEX.md'}")

    print("=" * 70)
