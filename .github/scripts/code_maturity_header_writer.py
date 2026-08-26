#!/usr/bin/env python3
"""
Canonical entrypoint für Code-Maturity-Header/Report-Updates (OOP/SoC-Refactor-Basis).

Verzeichnis-/Schichtenstruktur:
  - .github/scripts/: CI/CD-Entrypoints, orchestriert Dispatcher
  - scripts/: Lokale Tools, können Scanner/Dispatcher nutzen
  - ai_working/: Arbeitsartefakte (JSON, Reports)
  - src/: Ziel für Header-Updates

Schichten:
  1. Scanner: Sammelt Rohdaten (z. B. aus gap_scanner_v2, analyze_code_maturity)
  2. Dispatcher: Entscheidet, welche Dateien/Module aktualisiert werden
  3. Updater: Schreibt Header/Reports inkl. Header-Vollständigkeitsvalidierung

Aktuell: Enthält die kanonische Header-Generierung für Workflow und lokale Läufe.
"""

import sys

# --- Scanner ---
import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any


_RE_AUTOGEN_HEADER_BLOCK = re.compile(
    r'^\s*(//\s*THEMIS_GAP_STATS:.*\n)?/\*[\s\S]*?@file\s+[\w./\\-]+[\s\S]*?\*/\s*',
    re.MULTILINE,
)
# Matches the legacy compact C-style block written by analyze_code_maturity.py:
#   /* ThemisDB | File: ... */ or /* ... (Automatisch generiert ... */
_RE_COMPACT_LEGACY_BLOCK = re.compile(
    r'^\s*/\*[\s\S]*?(?:ThemisDB\s*\|\s*File:|Automatisch generiert)[\s\S]*?\*/\s*',
    re.MULTILINE,
)
_RE_LEGACY_GAP_LINE_GLOBAL = re.compile(r'^\s*//\s*THEMIS_GAP_STATS:.*\n?', re.MULTILINE)


def _debug_log(stage: str, message: str) -> None:
    timestamp = datetime.now().strftime('%H:%M:%S')
    print(f"[DEBUG {timestamp}] {stage}: {message}", flush=True)


def strip_generated_header(content: str) -> str:
    """Entfernt den automatisch erzeugten ThemisDB-Header vor der Analyse."""
    stripped = _RE_AUTOGEN_HEADER_BLOCK.sub('', content, count=1)
    stripped = _RE_COMPACT_LEGACY_BLOCK.sub('', stripped, count=1)
    stripped = _RE_LEGACY_GAP_LINE_GLOBAL.sub('', stripped, count=1)
    return stripped.lstrip('\n')

class CodeMaturityScanner:
    """Sammelt Code-Maturity-Daten (Pattern-Analyse und Score-Berechnung)"""
    SUPPORTED_EXTENSIONS: Dict[str, str] = {
        '.cpp': 'c', '.c': 'c', '.h': 'c', '.hpp': 'c', '.cs': 'c', '.py': 'python', '.php': 'c',
    }
    EXCLUDE_DIRS: set = {
        '.git', 'node_modules', 'build', 'dist', 'vendor', '.github', 'third_party', 'external',
    }

    # Pattern-Definitionen (aus analyze_code_maturity.py)
    PATTERN_STUBS = re.compile(r'\b(?:stub|placeholder|noop|NotImplemented)\b|no[-_\s]op\b|not\s+yet\s+implement\w*|throw\s+std::runtime_error\s*\(\s*["\]not\s+implemented["\]\s*\)', re.IGNORECASE)
    PATTERN_SIMULATIONS = re.compile(r'\bsimulat\w*\b|\bmock(?:ed|ing|s)?\b|\bfake[ds]?\b|\bdummy\b', re.IGNORECASE)
    PATTERN_TODOS = re.compile(r'\b(TODO|FIXME|HACK|XXX|BUG)\b')
    PATTERN_DEBUG = re.compile(r'\b(DEBUG)\b|cout\s*<<\s*["\]?debug|printf\s*\(["\]debug', re.IGNORECASE)
    PATTERN_HARDCODED = re.compile(r'\b(hardcoded|temporary|temp\s+fix)\b', re.IGNORECASE)
    PATTERN_DEAD_CODE = re.compile(r'if\s*\(\s*false\s*&&|//\s*DISABLED\b|//\s*disabled:', re.IGNORECASE)
    PATTERN_DOCUMENTED_STUB = re.compile(r'STUB/SIMULATION NOTE\s*:')
    PATTERN_PRODUCTION = re.compile(r'@production\b|production_ready', re.IGNORECASE)
    PATTERN_TESTS = re.compile(r'\b(TEST|EXPECT_|ASSERT_|unittest|pytest|test_)\w*\s*[\(\{]')
    PATTERN_DOCS = re.compile(r'(/\*\*|"""|\'\'\')|(///\s+\w)|\* @(param|returns?|throws?|brief)')

    SCORE_PENALTIES: Dict[str, int] = {
        'stub': 7, 'simulation': 5, 'todo': 3, 'debug': 1, 'hardcoded': 4, 'dead_code': 6, 'documented_stub': 2,
    }
    SCORE_BONUSES: Dict[str, Tuple[int, int]] = {
        'production': (3, 9), 'tests': (2, 20), 'docs': (1, 15), 'documented_stub': (1, 10),
    }

    def scan(self, repo_root: str = ".", target_paths: Optional[List[str]] = None):
        repo_root = Path(repo_root).resolve()
        files = list(self._find_files(repo_root, target_paths=target_paths))
        _debug_log('scanner', f'Found {len(files)} candidate files under {repo_root}')
        results = []
        for index, file in enumerate(files, start=1):
            maturity = self._analyze_file(file)
            if maturity:
                results.append(maturity)
            if index == 1 or index % 250 == 0 or index == len(files):
                _debug_log('scanner', f'Analyzed {index}/{len(files)} files')
        return results

    def _resolve_scan_roots(self, repo_root: Path, target_paths: Optional[List[str]]) -> List[Path]:
        if not target_paths:
            return [repo_root / d for d in ("src", "include", "tests", "benchmarks") if (repo_root / d).exists()]
        roots: List[Path] = []
        for raw in target_paths:
            candidate = Path(raw)
            resolved = candidate.resolve() if candidate.is_absolute() else (repo_root / candidate).resolve()
            if resolved.exists():
                roots.append(resolved)
            else:
                _debug_log('scanner', f'Skip missing target path: {raw}')
        return roots

    def _find_files(self, repo_root: Path, target_paths: Optional[List[str]] = None):
        roots = self._resolve_scan_roots(repo_root, target_paths)
        seen: set = set()
        for root_path in roots:
            if root_path.is_file():
                ext = root_path.suffix.lower()
                if ext in self.SUPPORTED_EXTENSIONS:
                    file_key = root_path.as_posix()
                    if file_key not in seen:
                        seen.add(file_key)
                        yield root_path
                continue
            for root, dirs, files in os.walk(root_path):
                dirs[:] = [d for d in dirs if d not in self.EXCLUDE_DIRS]
                for file in files:
                    ext = os.path.splitext(file)[1]
                    if ext in self.SUPPORTED_EXTENSIONS:
                        candidate = Path(root) / file
                        file_key = candidate.as_posix()
                        if file_key not in seen:
                            seen.add(file_key)
                            yield candidate

    def _analyze_file(self, file_path: Path):
        try:
            with open(file_path, encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception:
            return None
        content = strip_generated_header(content)
        penalties = 0
        bonuses = 0
        # Penalties
        if self.PATTERN_STUBS.search(content):
            penalties += self.SCORE_PENALTIES['stub']
        if self.PATTERN_SIMULATIONS.search(content):
            penalties += self.SCORE_PENALTIES['simulation']
        if self.PATTERN_TODOS.search(content):
            penalties += self.SCORE_PENALTIES['todo']
        if self.PATTERN_DEBUG.search(content):
            penalties += self.SCORE_PENALTIES['debug']
        if self.PATTERN_HARDCODED.search(content):
            penalties += self.SCORE_PENALTIES['hardcoded']
        if self.PATTERN_DEAD_CODE.search(content):
            penalties += self.SCORE_PENALTIES['dead_code']
        if self.PATTERN_DOCUMENTED_STUB.search(content):
            penalties += self.SCORE_PENALTIES['documented_stub']
        # Bonuses
        if self.PATTERN_PRODUCTION.search(content):
            bonuses += self.SCORE_BONUSES['production'][0]
        if self.PATTERN_TESTS.search(content):
            bonuses += self.SCORE_BONUSES['tests'][0]
        if self.PATTERN_DOCS.search(content):
            bonuses += self.SCORE_BONUSES['docs'][0]
        if self.PATTERN_DOCUMENTED_STUB.search(content):
            bonuses += self.SCORE_BONUSES['documented_stub'][0]
        # Score-Berechnung
        score = max(0, min(100, 100 - penalties + bonuses))
        level = self._get_maturity_level(score)
        return {
            'file': str(file_path),
            'score': score,
            'level': level,
        }

    def _get_maturity_level(self, score: float) -> str:
        if score >= 80:
            return '🟢 PRODUCTION-READY'
        elif score >= 60:
            return '🟡 RELEASE-CANDIDATE'
        elif score >= 40:
            return '🟠 BETA'
        elif score >= 20:
            return '🔴 ALPHA'
        else:
            return '⚫ DRAFT'

# --- Dispatcher ---
class CodeMaturityDispatcher:
    """Entscheidet, welche Dateien/Module aktualisiert werden sollen (Policy-gesteuert)"""
    def __init__(
        self,
        scanner: CodeMaturityScanner,
        min_score: int = 80,
        include_all_files: bool = False,
        target_paths: Optional[List[str]] = None,
    ):
        self.scanner = scanner
        self.min_score = min_score
        self.include_all_files = include_all_files
        self.target_paths = target_paths or []

    def dispatch(self, repo_root: str = "."):
        _debug_log('dispatcher', f'Start dispatch with min_score={self.min_score}, include_all_files={self.include_all_files}')
        scan_results = self.scanner.scan(repo_root, target_paths=self.target_paths)
        if self.include_all_files:
            to_update = scan_results
        else:
            # Policy: Nur Dateien mit Score < min_score (nicht production-ready)
            to_update = [r for r in scan_results if r['score'] < self.min_score]
        _debug_log('dispatcher', f'Selected {len(to_update)} files for header updates out of {len(scan_results)} scanned files')
        return to_update

# --- Updater ---


class CodeMaturityUpdater:
    """Schreibt/aktualisiert kompakten Maturity-Header mit maschinenlesbaren GAP_STATS."""
    LEAN_HEADER_TEMPLATE = (
        """/**
 * @file {file}
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version {version}
 * @note Maturity: {level}
 * @note Score: {score}/100
 * @note Module Context: {module_context}
 * @note Ownership Scope: {ownership_scope}
 * @note Primary Symbols: {primary_symbols}
 * @note Gap Summary: total={gaps}; TODO={todo}, Stub={stub}, Unimpl={unimpl}, Mock={mock}, Sim={sim}, Debt={debt}, C={ext_critical}, H={ext_high}, M={ext_medium}, L={ext_low}
 * @note Governance: {governance_context}
 * @note Release Context: {release_context}
 * @note Status: {status}{maturity_gate}
 * @note Generator: .github/scripts/code_maturity_header_writer.py
 * @note This block is auto-generated and will be overwritten.
 */"""
    )

    EXTENDED_HEADER_TEMPLATE = (
        """/**
 * @file {file}
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author {author}
 * @version {version}
 * @date {last_modified}
 * @note Maturity: {level}
 * @note Score: {score}/100
 * @note Lines: {total_lines}
 * @note Module Context: {module_context}
 * @note Ownership Scope: {ownership_scope}
 * @note Primary Symbols: {primary_symbols}
 * @note Gap Summary: total={gaps}; TODO={todo}, Stub={stub}, Unimpl={unimpl}, Mock={mock}, Sim={sim}, Debt={debt}, C={ext_critical}, H={ext_high}, M={ext_medium}, L={ext_low}
 * @note PR History (last 5): {pr_info}
 * @note Governance: {governance_context}
 * @note Release Context: {release_context}
 * @note Status: {status}{maturity_gate}
 * @note Generator: .github/scripts/code_maturity_header_writer.py
 * @note This block is auto-generated and will be overwritten.
 */"""
    )

    _RE_EXISTING_HEADER = _RE_AUTOGEN_HEADER_BLOCK
    _RE_COMPACT_LEGACY_BLOCK = _RE_COMPACT_LEGACY_BLOCK
    _RE_LEGACY_GAP_LINE = _RE_LEGACY_GAP_LINE_GLOBAL
    _RE_VERSION = re.compile(r'Version:\s*([0-9]+\.[0-9]+\.[0-9]+)')

    _PATTERN_UNIMPL = re.compile(r'\bnot\s+implemented\b|\bunimplemented\b', re.IGNORECASE)
    _PATTERN_STUB = re.compile(r'\bstub\b|\bplaceholder\b|\bnoop\b', re.IGNORECASE)
    _PATTERN_MOCK = re.compile(r'\bmock(?:ed|ing|s)?\b', re.IGNORECASE)
    _PATTERN_SIM = re.compile(r'\bsimulat\w*\b|\bfake[ds]?\b|\bdummy\b', re.IGNORECASE)
    _PATTERN_TODO = re.compile(r'\b(TODO|FIXME|HACK|XXX|BUG)\b')
    _PATTERN_DEBT = re.compile(r'\btechnical debt\b|\btemp(?:orary)?\s+fix\b|\bhardcoded\b', re.IGNORECASE)

    def __init__(self, dispatcher: CodeMaturityDispatcher, header_mode: str = 'auto'):
        self.dispatcher = dispatcher
        self.header_mode = header_mode if header_mode in ('auto', 'lean', 'extended') else 'auto'
        self.external_gap_details: Dict[str, Dict[str, int]] = {}

    def _detect_maturity_gates(self, file_path: Path, repo_root: Path) -> str:
        """Detect which automation gates this file contributes to.
         
        Args:
          file_path: Absolute path to file
          repo_root: Repository root
         
        Returns:
          Formatted gate string or empty string if no gates detected.
          Example: "\n  * @maturity_gate: GATE-W8-02 (code coverage verification)"
        """
        try:
            rel_path = file_path.relative_to(repo_root)
            parts = rel_path.parts
             
            gates = []
             
            # Detect module from path
            if len(parts) >= 2:
                context = parts[0]  # src, tests, benchmarks, include
                module = parts[1] if len(parts) > 1 else None
                 
                # Tests contribute to phase gates
                if context == 'tests' and module:
                    # Phase gate for test files: GATE-PHASE-<N>-<MODULE>
                    if 'phase1_focused' in file_path.name:
                        gates.append(f'GATE-PHASE-1-{module}')
                    elif 'phase2_focused' in file_path.name or 'phase2_phase3_focused' in file_path.name:
                        gates.append(f'GATE-PHASE-2-{module}')
                    elif 'phase3_focused' in file_path.name:
                        gates.append(f'GATE-PHASE-3-{module}')
                 
                # Benchmarks contribute to wave gates
                elif context == 'benchmarks' and module:
                    # Benchmark gate: GATE-W<wave>-<benchmark_type>
                    if 'bench_' in file_path.name and '_gates' in file_path.name:
                        # Extract benchmark identifier from filename
                        stem = file_path.stem
                        if 'phase2_phase3' in stem or 'fp23' in stem.lower():
                            gates.append(f'GATE-W7-02')  # Wave 7 phase gate
                        elif 'failover' in module or 'replication' in module:
                            gates.append(f'GATE-W7-01')  # Wave 7 failover
                        elif 'security' in module or 'auth' in module:
                            gates.append(f'GATE-W8-02')  # Wave 8 security
             
            # Format gate string for header
            if gates:
                unique_gates = list(set(gates))
                gate_str = ', '.join(sorted(unique_gates))
                return f'\n  * @maturity_gate: {gate_str}'
             
            return ''
         
        except Exception as e:
            _debug_log('maturity_gate_detection', f'Warning: {e}')
            return ''

    def update(
        self,
        repo_root: str = ".",
        no_headers: bool = False,
        report_path: Optional[str] = None,
        validate_headers: bool = True,
        fail_on_validation: bool = False,
    ):
        repo_root_path = Path(repo_root).resolve()
        _debug_log('updater', f'Load external gap details from {repo_root_path / "ai_working"}')
        self.external_gap_details = self._load_external_gap_details(repo_root_path)
        _debug_log('updater', f'Loaded external gap details for {len(self.external_gap_details)} files')
        to_update = self.dispatcher.dispatch(str(repo_root_path))
        _debug_log('updater', f'Start header updates for {len(to_update)} files (no_headers={no_headers})')
        updated = []
        total = len(to_update)
        if not no_headers:
            for index, entry in enumerate(to_update, start=1):
                file_path = entry['file']
                score = entry['score']
                level = entry['level']
                try:
                    self._write_header(repo_root_path, Path(file_path), score, level)
                    updated.append(file_path)
                    if index == 1 or index % 100 == 0 or index == total:
                        _debug_log('updater', f'Updated {index}/{total}: {Path(file_path).name}')
                except Exception as e:
                    print(f"[FAIL] Header-Update für {file_path}: {e}")
        else:
            _debug_log('updater', 'Header write disabled via --no-headers')
        print(f"[OK] {len(updated)} Header aktualisiert.")
        validation_summary = {'checked': 0, 'failed': 0, 'failures': []}
        if validate_headers:
            validation_summary = self._validate_headers(repo_root_path, to_update)
            print(f"[OK] Header validation checked={validation_summary['checked']} failed={validation_summary['failed']}")

        summary = {
            'scanned_candidates': total,
            'updated': len(updated),
            'no_headers': no_headers,
            'validation': validation_summary,
            'header_mode': self._resolve_header_mode(''),
        }
        if report_path:
            self._write_report(Path(report_path), summary)
        if fail_on_validation and validation_summary['failed'] > 0:
            summary['exit_code'] = 2
        else:
            summary['exit_code'] = 0
        return summary

    def _write_header(self, repo_root: Path, file_path: Path, score: int, level: str):
        # Lese Originalinhalt
        with open(file_path, encoding='utf-8', errors='ignore') as f:
            content = f.read()

        metrics = self._collect_quality_metrics(content)
        # Konsistenz: Score/Level immer aus dem tatsächlichen Dateiinhalt ableiten.
        # Damit kann kein Header "100/100 + Stubs/Gaps" mehr entstehen.
        score, level = self._derive_score_and_level(content)
        version = self._extract_or_default_version(content)
        status = self._status_from_level(level)

        # Entferne alten Header, falls vorhanden
        content_ohne_header = self._RE_EXISTING_HEADER.sub('', content, count=1)
        content_ohne_header = self._RE_COMPACT_LEGACY_BLOCK.sub('', content_ohne_header, count=1)
        content_ohne_header = self._RE_LEGACY_GAP_LINE.sub('', content_ohne_header, count=1).lstrip('\n')

        template_data = {
            'file': file_path.name,
            'version': version,
            'level': level,
            'score': score,
            'todo': metrics['todo'],
            'stub': metrics['stub'],
            'gaps': metrics['gaps'],
            'unimpl': metrics['unimpl'],
            'mock': metrics['mock'],
            'sim': metrics['sim'],
            'debt': metrics['debt'],
            'status': status,
        }
        module_context, ownership_scope = self._derive_module_context(repo_root, file_path)
        template_data.update({
            'module_context': module_context,
            'ownership_scope': ownership_scope,
            'primary_symbols': self._extract_primary_symbols(content_ohne_header),
            'governance_context': 'BranchModel=develop-first; CanonicalBranches=develop,community,military',
            'release_context': 'GateModel=WaveA→B→C→D on develop',
        })

        gap_corr = self._build_gap_correlation(repo_root, file_path, metrics['gaps'])
        template_data.update(gap_corr)
         
        # Detect maturity gates that this file contributes to
        maturity_gate = self._detect_maturity_gates(file_path, repo_root)
        template_data['maturity_gate'] = maturity_gate

        # Füge neuen Header ein
        effective_mode = self._resolve_header_mode(level)
        if effective_mode == 'extended':
            template_data.update({
                'last_modified': self._get_last_modified(repo_root, file_path),
                'author': self._get_last_author(repo_root, file_path),
                'total_lines': content.count('\n') + (1 if content and not content.endswith('\n') else 0),
                'pr_info': self._get_file_pr_info(repo_root, file_path),
            })
            header = self.EXTENDED_HEADER_TEMPLATE.format(**template_data)
        else:
            header = self.LEAN_HEADER_TEMPLATE.format(**template_data)

        header_out = header.rstrip() + '\n\n'
        new_content = header_out + content_ohne_header
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)

    def _derive_module_context(self, repo_root: Path, file_path: Path) -> Tuple[str, str]:
        rel = file_path.relative_to(repo_root)
        parts = rel.parts
        if len(parts) < 2:
            return (parts[0] if parts else 'root', 'global')
        context = parts[0]
        module = parts[1]
        ownership = {
            'src': 'production-code',
            'include': 'public-api',
            'tests': 'test-suite',
            'benchmarks': 'benchmark-suite',
        }.get(context, 'global')
        return (f'{context}/{module}', ownership)

    def _extract_primary_symbols(self, content: str, max_symbols: int = 6) -> str:
        class_pattern = re.compile(r'^\s*(?:template\s*<[^>]+>\s*)?(?:class|struct|enum(?:\s+class)?)\s+([A-Za-z_]\w*)', re.MULTILINE)
        function_pattern = re.compile(
            r'^\s*(?:inline\s+|static\s+|virtual\s+|constexpr\s+|friend\s+)*'
            r'(?:[\w:\<\>\,\s\*&~]+)\s+([A-Za-z_~]\w*)\s*\([^;{}]*\)\s*(?:const)?\s*(?:noexcept)?\s*(?:\{|$)',
            re.MULTILINE,
        )
        blocked = {'if', 'for', 'while', 'switch', 'return', 'catch'}
        symbols: List[str] = []
        for pattern in (class_pattern, function_pattern):
            for match in pattern.finditer(content):
                name = match.group(1)
                if not name or name in blocked:
                    continue
                if name not in symbols:
                    symbols.append(name)
                if len(symbols) >= max_symbols:
                    return ', '.join(symbols)
        return ', '.join(symbols) if symbols else 'none-detected'

    def _derive_score_and_level(self, content: str) -> Tuple[int, str]:
        content = strip_generated_header(content)
        scanner = self.dispatcher.scanner
        penalties = 0
        bonuses = 0

        if scanner.PATTERN_STUBS.search(content):
            penalties += scanner.SCORE_PENALTIES['stub']
        if scanner.PATTERN_SIMULATIONS.search(content):
            penalties += scanner.SCORE_PENALTIES['simulation']
        if scanner.PATTERN_TODOS.search(content):
            penalties += scanner.SCORE_PENALTIES['todo']
        if scanner.PATTERN_DEBUG.search(content):
            penalties += scanner.SCORE_PENALTIES['debug']
        if scanner.PATTERN_HARDCODED.search(content):
            penalties += scanner.SCORE_PENALTIES['hardcoded']
        if scanner.PATTERN_DEAD_CODE.search(content):
            penalties += scanner.SCORE_PENALTIES['dead_code']
        if scanner.PATTERN_DOCUMENTED_STUB.search(content):
            penalties += scanner.SCORE_PENALTIES['documented_stub']

        if scanner.PATTERN_PRODUCTION.search(content):
            bonuses += scanner.SCORE_BONUSES['production'][0]
        if scanner.PATTERN_TESTS.search(content):
            bonuses += scanner.SCORE_BONUSES['tests'][0]
        if scanner.PATTERN_DOCS.search(content):
            bonuses += scanner.SCORE_BONUSES['docs'][0]
        if scanner.PATTERN_DOCUMENTED_STUB.search(content):
            bonuses += scanner.SCORE_BONUSES['documented_stub'][0]

        score = max(0, min(100, 100 - penalties + bonuses))
        return score, scanner._get_maturity_level(score)

    def _collect_quality_metrics(self, content: str) -> Dict[str, int]:
        unimpl = len(self._PATTERN_UNIMPL.findall(content))
        stub = len(self._PATTERN_STUB.findall(content))
        mock = len(self._PATTERN_MOCK.findall(content))
        sim = len(self._PATTERN_SIM.findall(content))
        todo = len(self._PATTERN_TODO.findall(content))
        debt = len(self._PATTERN_DEBT.findall(content))
        gaps = unimpl + stub + mock + sim + todo + debt
        return {
            'gaps': gaps,
            'unimpl': unimpl,
            'stub': stub,
            'mock': mock,
            'sim': sim,
            'todo': todo,
            'debt': debt,
        }

    def _extract_or_default_version(self, content: str) -> str:
        m = self._RE_VERSION.search(content)
        if m:
            return m.group(1)
        return '0.0.1'

    def _status_from_level(self, level: str) -> str:
        if 'PRODUCTION-READY' in level:
            return 'Production Ready'
        if 'RELEASE-CANDIDATE' in level:
            return 'Release Candidate'
        if 'BETA' in level:
            return 'Beta'
        if 'ALPHA' in level:
            return 'Alpha'
        return 'Draft'

    def _run_cmd(self, args: List[str], cwd: Path, timeout: int = 8) -> str:
        try:
            result = subprocess.run(
                args,
                cwd=cwd,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            if result.returncode == 0:
                return result.stdout.strip()
        except Exception:
            pass
        return ''

    def _get_last_modified(self, repo_root: Path, file_path: Path) -> str:
        rel = file_path.relative_to(repo_root)
        out = self._run_cmd(
            ['git', 'log', '-1', '--format=%ad', '--date=format:%Y-%m-%d %H:%M:%S', '--', str(rel)],
            cwd=repo_root,
            timeout=5,
        )
        if out:
            return out
        return datetime.fromtimestamp(file_path.stat().st_mtime).strftime('%Y-%m-%d %H:%M:%S')

    def _get_last_author(self, repo_root: Path, file_path: Path) -> str:
        rel = file_path.relative_to(repo_root)
        out = self._run_cmd(
            ['git', 'log', '-1', '--format=%an', '--', str(rel)],
            cwd=repo_root,
            timeout=5,
        )
        return out if out else 'unknown'

    def _get_file_pr_info(self, repo_root: Path, file_path: Path) -> str:
        rel = file_path.relative_to(repo_root).as_posix()
        out = self._run_cmd(
            [
                'gh', 'pr', 'list',
                '--state', 'all',
                '--search', f'{rel} in:files',
                '--limit', '25',
                '--json', 'number,title,updatedAt',
            ],
            cwd=repo_root,
            timeout=10,
        )
        if not out:
            return 'none'
        try:
            data = json.loads(out)
            if not data:
                return 'none'
            prs = [pr for pr in data if isinstance(pr, dict)]
            prs.sort(key=lambda pr: (pr.get('updatedAt') or ''), reverse=True)
            top = prs[:5]
            formatted: List[str] = []
            for pr in top:
                number = pr.get('number', '?')
                title = (pr.get('title') or '').strip()
                if len(title) > 30:
                    title = title[:27] + '...'
                updated = (pr.get('updatedAt') or '')[:10]
                if updated:
                    formatted.append(f'#{number} {title} ({updated})')
                else:
                    formatted.append(f'#{number} {title}')
            return ' | '.join(formatted) if formatted else 'none'
        except Exception:
            return 'none'

    def _normalize_rel_path(self, repo_root: Path, file_path: Path) -> str:
        rel = file_path.relative_to(repo_root).as_posix().lower()
        return rel

    def _get_external_gap_detail(self, repo_root: Path, file_path: Path) -> Optional[Dict[str, int]]:
        rel = self._normalize_rel_path(repo_root, file_path)
        return self.external_gap_details.get(rel)

    def _build_gap_correlation(self, repo_root: Path, file_path: Path, internal_gaps: int) -> Dict[str, str]:
        detail = self._get_external_gap_detail(repo_root, file_path)
        if not detail:
            return {
                'external_total': 'n/a',
                'ext_critical': 'n/a',
                'ext_high': 'n/a',
                'ext_medium': 'n/a',
                'ext_low': 'n/a',
                'gap_delta': 'n/a',
                'gap_alignment': 'no_external_data',
            }

        external_total = detail['total']
        delta = abs(external_total - internal_gaps)
        if delta == 0:
            alignment = 'aligned'
        else:
            tolerance = max(2, int(0.1 * max(external_total, internal_gaps, 1)))
            alignment = 'near' if delta <= tolerance else 'divergent'

        return {
            'external_total': str(external_total),
            'ext_critical': str(detail['critical']),
            'ext_high': str(detail['high']),
            'ext_medium': str(detail['medium']),
            'ext_low': str(detail['low']),
            'gap_delta': str(delta),
            'gap_alignment': alignment,
        }

    def _load_external_gap_details(self, repo_root: Path) -> Dict[str, Dict[str, int]]:
        """Lädt per-Datei-Gap-Details aus ai_working/gap_scan_v3_*.json."""
        details: Dict[str, Dict[str, int]] = {}
        ai_working = repo_root / 'ai_working'
        if not ai_working.exists():
            return details

        candidates = sorted(ai_working.glob('gap_scan_v3_*.json'))
        for path in candidates:
            name = path.name
            if 'aggregate' in name or 'summary' in name:
                continue
            try:
                data = json.loads(path.read_text(encoding='utf-8', errors='ignore'))
            except Exception:
                continue

            if not isinstance(data, dict):
                continue

            for module_payload in data.values():
                if not isinstance(module_payload, dict):
                    continue
                by_file = module_payload.get('by_file')
                if not isinstance(by_file, dict):
                    continue
                for rel_path_raw, items in by_file.items():
                    if not isinstance(rel_path_raw, str):
                        continue
                    rel_path = rel_path_raw.replace('\\', '/').lower()
                    if isinstance(items, list):
                        if rel_path not in details:
                            details[rel_path] = {
                                'total': 0,
                                'critical': 0,
                                'high': 0,
                                'medium': 0,
                                'low': 0,
                            }
                        details[rel_path]['total'] += len(items)
                        for item in items:
                            if not isinstance(item, dict):
                                continue
                            sev = str(item.get('severity', '')).strip().lower()
                            if sev == 'critical':
                                details[rel_path]['critical'] += 1
                            elif sev == 'high':
                                details[rel_path]['high'] += 1
                            elif sev == 'medium':
                                details[rel_path]['medium'] += 1
                            elif sev == 'low':
                                details[rel_path]['low'] += 1

        return details

    def _resolve_header_mode(self, level: str) -> str:
        if self.header_mode == 'auto':
            # Always emit extended headers to preserve all metadata fields
            # (Lines, Author, Last Modified, PR History) that were previously
            # only written by the legacy compact block in analyze_code_maturity.py.
            return 'extended'
        return self.header_mode

    def _required_header_fragments(self, mode: str) -> List[str]:
        base = [
            '@file',
            '@brief',
            '@version',
            '@note Maturity:',
            '@note Score:',
            '@note Module Context:',
            '@note Ownership Scope:',
            '@note Primary Symbols:',
            '@note Gap Summary:',
            '@note Governance:',
            '@note Release Context:',
            '@note Status:',
            '@note Generator:',
        ]
        if mode == 'extended':
            return ['@author', '@date', '@note Lines:', '@note PR History (last 5):'] + base
        return base

    def _validate_headers(self, repo_root: Path, entries: List[Dict[str, Any]]) -> Dict[str, Any]:
        mode = self._resolve_header_mode('')
        required = self._required_header_fragments(mode)
        failures: List[Dict[str, Any]] = []
        checked = 0
        for entry in entries:
            file_path = Path(entry['file'])
            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')
            except Exception:
                failures.append({'file': file_path.as_posix(), 'missing': ['<read_failed>']})
                continue
            match = self._RE_EXISTING_HEADER.match(content)
            checked += 1
            if not match:
                failures.append({'file': file_path.relative_to(repo_root).as_posix(), 'missing': ['<header_block_missing>']})
                continue
            header = match.group(0)
            missing = [fragment for fragment in required if fragment not in header]
            if missing:
                failures.append({'file': file_path.relative_to(repo_root).as_posix(), 'missing': missing})
        return {'checked': checked, 'failed': len(failures), 'failures': failures}

    def _write_report(self, report_path: Path, summary: Dict[str, Any]) -> None:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')
        validation = summary.get('validation', {})
        lines = [
            '# Code Maturity Header Report',
            '',
            f'- Generated: {now}',
            f'- Header mode: {summary.get("header_mode", "unknown")}',
            f'- Scan candidates: {summary.get("scanned_candidates", 0)}',
            f'- Headers updated: {summary.get("updated", 0)}',
            f'- Header write mode: {"check-only" if summary.get("no_headers") else "rewrite"}',
            f'- Validation checked: {validation.get("checked", 0)}',
            f'- Validation failed: {validation.get("failed", 0)}',
            '',
        ]
        failures = validation.get('failures', [])
        if failures:
            lines.extend(['## Missing required header fragments', ''])
            for item in failures[:200]:
                missing = ', '.join(item.get('missing', []))
                lines.append(f"- `{item.get('file', '<unknown>')}` → {missing}")
        else:
            lines.append('All validated files contain required header fragments.')
        report_path.write_text('\n'.join(lines).rstrip() + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description='Write/update canonical code maturity Doxygen headers.')
    parser.add_argument('--root', default='.', help='Repository root path')
    parser.add_argument('--min-score', type=int, default=80, help='Only files with score < min-score are updated')
    parser.add_argument(
        '--header-mode',
        choices=['auto', 'lean', 'extended'],
        default='auto',
        help='Header detail level (auto=extended until production-ready, then lean)',
    )
    parser.add_argument('--target-paths', default='', help='Comma-separated list of paths to scan (files or directories)')
    parser.add_argument('--include-all-files', action='store_true', help='Update all scanned files regardless of score')
    parser.add_argument('--no-headers', action='store_true', help='Do not rewrite headers; run scan/validation/report only')
    parser.add_argument('--report-path', default='', help='Optional markdown report output path')
    parser.add_argument('--no-validate-headers', action='store_true', help='Disable required-header validation')
    parser.add_argument('--fail-on-validation', action='store_true', help='Return non-zero exit code if validation fails')
    args = parser.parse_args()

    target_paths = [p.strip() for p in args.target_paths.split(',') if p.strip()]
    scanner = CodeMaturityScanner()
    dispatcher = CodeMaturityDispatcher(
        scanner,
        min_score=args.min_score,
        include_all_files=args.include_all_files,
        target_paths=target_paths,
    )
    updater = CodeMaturityUpdater(dispatcher, header_mode=args.header_mode)
    summary = updater.update(
        args.root,
        no_headers=args.no_headers,
        report_path=args.report_path or None,
        validate_headers=not args.no_validate_headers,
        fail_on_validation=args.fail_on_validation,
    )
    return int(summary.get('exit_code', 0))


if __name__ == '__main__':
    raise SystemExit(main())
