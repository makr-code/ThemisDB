#!/usr/bin/env python3
"""
Module Gap Documentation Generator

Generates module-local developer notes from the v3 aggregate gap scan.

Outputs:
- ai_working/module_gaps/<module>_GAPS.md
- src/<module>/MODULE_GAPS.md

The generated documentation is complete per module and includes the full
scanner findings grouped by file. Only modules under src/ are emitted, which
implicitly excludes external git submodules outside the source tree.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


SEVERITY_ORDER = {
    "CRITICAL": 0,
    "HIGH": 1,
    "MEDIUM": 2,
    "LOW": 3,
    "INFO": 4,
    "INTENTIONAL": 5,
    "N/A": 6,
}


@dataclass(frozen=True)
class ModuleSummary:
    name: str
    total: int
    critical: int
    high: int
    medium: int
    low: int
    by_category: Dict[str, int]
    by_file: Dict[str, List[Dict]]

    @property
    def actionable(self) -> int:
        return self.critical + self.high

    @property
    def affected_files(self) -> int:
        return len(self.by_file)


class ModuleDocumentationGenerator:
    """Generate complete v3-based developer notes for src modules."""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root).resolve()
        self.src_dir = self.repo_root / 'src'
        self.scan_results: Dict[str, Dict] = {}
        self.src_modules = self._discover_src_modules()

    def _discover_src_modules(self) -> List[str]:
        if not self.src_dir.exists():
            return []
        modules: List[str] = []
        for entry in sorted(self.src_dir.iterdir()):
            if not entry.is_dir():
                continue
            if entry.name.startswith('_'):
                continue
            modules.append(entry.name)
        return modules

    def load_scan_results(self, scan_dir: str | Path) -> bool:
        """Load v3 aggregate scan results and keep only src modules."""
        scan_path = Path(scan_dir)
        aggregate_path = scan_path / 'gap_scan_v3_aggregate.json'

        if not aggregate_path.exists():
            print(f"[FAIL] Aggregate scan file not found: {aggregate_path}")
            return False

        try:
            data = json.loads(aggregate_path.read_text(encoding='utf-8'))
        except Exception as exc:
            print(f"[FAIL] Could not parse {aggregate_path}: {exc}")
            return False

        if not isinstance(data, dict):
            print(f"[FAIL] Unexpected aggregate format in: {aggregate_path}")
            return False

        src_modules = set(self.src_modules)
        self.scan_results = {
            module: payload
            for module, payload in data.items()
            if module in src_modules and isinstance(payload, dict)
        }
        return bool(self.scan_results)

    def _summarize_module(self, module_name: str) -> ModuleSummary:
        payload = self.scan_results.get(module_name, {})
        return ModuleSummary(
            name=module_name,
            total=int(payload.get('total', 0) or 0),
            critical=int(payload.get('severity_critical', 0) or 0),
            high=int(payload.get('severity_high', 0) or 0),
            medium=int(payload.get('severity_medium', 0) or 0),
            low=int(payload.get('severity_low', 0) or 0),
            by_category=dict(payload.get('by_category', {}) or {}),
            by_file=dict(payload.get('by_file', {}) or {}),
        )

    def _health_status(self, summary: ModuleSummary) -> str:
        if summary.total == 0:
            return 'No Findings'
        if summary.critical > 0:
            return 'Critical Findings Present'
        if summary.high > 0:
            return 'High-Priority Findings Present'
        return 'Findings Present'

    def _normalize_path(self, file_path: str) -> str:
        return file_path.replace('\\', '/')

    def _sort_findings(self, findings: Iterable[Dict]) -> List[Dict]:
        return sorted(
            findings,
            key=lambda item: (
                SEVERITY_ORDER.get(str(item.get('severity', 'INFO')).upper(), 99),
                int(item.get('line', 0) or 0),
                str(item.get('category') or item.get('type') or item.get('pattern') or ''),
            ),
        )

    def _sort_files(self, by_file: Dict[str, List[Dict]]) -> List[Tuple[str, List[Dict]]]:
        return sorted(
            by_file.items(),
            key=lambda entry: (-len(entry[1]), self._normalize_path(entry[0])),
        )

    def _format_category_table(self, by_category: Dict[str, int]) -> str:
        if not by_category:
            return 'No category findings recorded.\n'

        lines = [
            '| Category | Count |',
            '|---|---:|',
        ]
        for category, count in sorted(by_category.items(), key=lambda item: (-item[1], item[0])):
            lines.append(f'| {category} | {count} |')
        return '\n'.join(lines) + '\n'

    def _format_file_overview(self, summary: ModuleSummary) -> str:
        if not summary.by_file:
            return 'No file-level findings recorded.\n'

        lines = [
            '| File | Findings | Critical | High | Medium | Low |',
            '|---|---:|---:|---:|---:|---:|',
        ]
        for file_path, findings in self._sort_files(summary.by_file):
            critical = sum(1 for item in findings if str(item.get('severity', '')).upper() == 'CRITICAL')
            high = sum(1 for item in findings if str(item.get('severity', '')).upper() == 'HIGH')
            medium = sum(1 for item in findings if str(item.get('severity', '')).upper() == 'MEDIUM')
            low = sum(1 for item in findings if str(item.get('severity', '')).upper() == 'LOW')
            lines.append(
                f'| {self._normalize_path(file_path)} | {len(findings)} | {critical} | {high} | {medium} | {low} |'
            )
        return '\n'.join(lines) + '\n'

    def _format_finding(self, finding: Dict) -> str:
        severity = str(finding.get('severity', 'INFO')).upper()
        line = int(finding.get('line', 0) or 0)
        category = str(finding.get('category') or finding.get('type') or 'uncategorized')
        pattern = str(finding.get('pattern') or '')
        description = str(finding.get('description') or '').strip()
        remediation = str(finding.get('remediation') or '').strip()
        snippet = str(finding.get('context') or finding.get('snippet') or '').strip()
        confidence = finding.get('confidence_band')
        confidence_score = finding.get('confidence_score')

        header = f'- Line {line}: severity={severity}; category={category}'
        if pattern:
            header += f'; pattern={pattern}'

        lines = [header]
        if description:
            lines.append(f'  Description: {description}')
        if remediation:
            lines.append(f'  Remediation: {remediation}')
        if snippet:
            lines.append(f'  Context: {snippet}')
        if confidence is not None or confidence_score is not None:
            band_text = str(confidence) if confidence is not None else 'n/a'
            score_text = f'{confidence_score}' if confidence_score is not None else 'n/a'
            lines.append(f'  Confidence: band={band_text}; score={score_text}')
        return '\n'.join(lines)

    def _format_full_findings(self, summary: ModuleSummary) -> str:
        if not summary.by_file:
            return '## Full Scanner Findings\n\nNo findings recorded for this module.\n'

        sections = ['## Full Scanner Findings', '']
        for file_path, findings in self._sort_files(summary.by_file):
            sections.append(f'### {self._normalize_path(file_path)}')
            sections.append(f'Total findings: {len(findings)}')
            sections.append('')
            for finding in self._sort_findings(findings):
                sections.append(self._format_finding(finding))
            sections.append('')
        return '\n'.join(sections).rstrip() + '\n'

    def _generate_doc_content(self, summary: ModuleSummary) -> str:
        generated = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        parts = [
            f'# {summary.name} Module - Developer Gap Note',
            '',
            '> Auto-generated from ai_working/gap_scan_v3_aggregate.json.',
            '> This file is overwritten on each regeneration.',
            '',
            '## Scan Snapshot',
            '',
            f'- Module: {summary.name}',
            f'- Generated: {generated}',
            f'- Status: {self._health_status(summary)}',
            f'- Total Findings: {summary.total}',
            f'- Actionable Findings (Critical + High): {summary.actionable}',
            f'- Affected Files: {summary.affected_files}',
            '',
            '## Severity Summary',
            '',
            '| Severity | Count |',
            '|---|---:|',
            f'| Critical | {summary.critical} |',
            f'| High | {summary.high} |',
            f'| Medium | {summary.medium} |',
            f'| Low | {summary.low} |',
            '',
            '## Category Summary',
            '',
            self._format_category_table(summary.by_category).rstrip(),
            '',
            '## File Overview',
            '',
            self._format_file_overview(summary).rstrip(),
            '',
            self._format_full_findings(summary).rstrip(),
            '',
            '## Update Workflow',
            '',
            '- Refresh scan artifacts with: python tools/gap_scanner_v3.py',
            '- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps',
            '- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.',
            '',
            'Format: THEMIS_MODULE_GAPS_V3',
        ]
        return '\n'.join(parts).rstrip() + '\n'

    def _write_doc_pair(self, module_name: str, content: str, output_dir: str | Path | None) -> bool:
        archive_dir = Path(output_dir) if output_dir else self.repo_root / 'ai_working' / 'module_gaps'
        archive_dir.mkdir(parents=True, exist_ok=True)

        archive_path = archive_dir / f'{module_name}_GAPS.md'
        module_path = self.src_dir / module_name / 'MODULE_GAPS.md'
        module_path.parent.mkdir(parents=True, exist_ok=True)

        archive_path.write_text(content, encoding='utf-8')
        module_path.write_text(content, encoding='utf-8')
        return True

    def generate_module_docs(self, module_name: str, output_dir: str | Path | None = None) -> bool:
        if module_name not in self.src_modules:
            return False
        summary = self._summarize_module(module_name)
        content = self._generate_doc_content(summary)
        return self._write_doc_pair(module_name, content, output_dir)

    def generate_all_module_docs(self, output_dir: str | Path | None = None) -> Dict[str, bool]:
        results: Dict[str, bool] = {}
        for module_name in self.src_modules:
            try:
                success = self.generate_module_docs(module_name, output_dir)
            except Exception:
                success = False
            results[module_name] = success
            print(f"  {'[OK]' if success else '[FAIL]'} {module_name}")
        return results

    def generate_module_index(self, output_dir: str | Path) -> bool:
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        modules_sorted = sorted(
            (self._summarize_module(module_name) for module_name in self.src_modules),
            key=lambda item: (-item.total, item.name),
        )

        lines = [
            '# Module Gap Documentation Index',
            '',
            f'Updated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}',
            '',
            'The archive docs in this directory are mirrored into src/<module>/MODULE_GAPS.md.',
            '',
            '| Module | Total | Critical | High | Medium | Low | Affected Files |',
            '|---|---:|---:|---:|---:|---:|---:|',
        ]

        for summary in modules_sorted:
            lines.append(
                f'| [{summary.name}]({summary.name}_GAPS.md) | {summary.total} | {summary.critical} | {summary.high} | '
                f'{summary.medium} | {summary.low} | {summary.affected_files} |'
            )

        (output_path / 'MODULE_GAPS_INDEX.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
        return True


if __name__ == '__main__':
    import sys

    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    scan_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    output_dir = sys.argv[3] if len(sys.argv) > 3 else 'ai_working/module_gaps'

    print('[INFO] Module Gap Documentation Generator')
    print('=' * 60)

    gen = ModuleDocumentationGenerator(repo_root)

    if not gen.load_scan_results(scan_dir):
        print('[FAIL] No v3 scan results found')
        raise SystemExit(1)

    print(f"\n[OK] Loaded {len(gen.scan_results)} modules from v3 aggregate\n")

    print('[...] Generating complete module documentation...')
    results = gen.generate_all_module_docs(output_dir)
    success_count = sum(1 for value in results.values() if value)
    print(f"\n[OK] Generated {success_count}/{len(results)} module docs")

    print('\n[...] Generating module index...')
    if gen.generate_module_index(output_dir):
        print(f"[OK] Index created: {Path(output_dir) / 'MODULE_GAPS_INDEX.md'}")

    print('\n' + '=' * 60)
    print(f'[INFO] Output directory: {output_dir}/')
