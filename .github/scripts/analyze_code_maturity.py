#!/usr/bin/env python3
"""
analyze_code_maturity.py – Code-Maturity-Analyse und Auto-Versionierung für ThemisDB

Analysiert Source-Dateien auf Code-Qualität und bewertet den Reifegrad:
  DRAFT → ALPHA → BETA → RC → PRODUCTION-READY

Schreibt automatisch formatierte Header in jede Source-Datei und generiert
einen Markdown-Report sowie eine JSON-Versions-Tracking-Datei.

Verwendung:
    python .github/scripts/analyze_code_maturity.py [--root <repo-root>] [--no-headers]

    --root        Repository-Root (Standard: aktuelles Verzeichnis)
    --no-headers  Header-Aktualisierung deaktivieren (nur Report generieren)
"""

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from urllib.parse import urlparse

# ---------------------------------------------------------------------------
# Konfiguration
# ---------------------------------------------------------------------------

# Unterstützte Dateierweiterungen und ihre Kommentar-Stile
SUPPORTED_EXTENSIONS: Dict[str, str] = {
    '.cpp': 'c',
    '.c':   'c',
    '.h':   'c',
    '.hpp': 'c',
    '.cs':  'c',
    '.py':  'python',
    '.php': 'c',
}

# Maximale Anzahl von Commits in der Revisionshistorie (konfigurierbar via Umgebungsvariable)
MAX_HISTORY_ENTRIES: int = int(os.getenv('MATURITY_MAX_HISTORY', '5'))

# Bot-Commit-Filterung: Commits von automatisierten Workflow-Runs ausschließen
EXCLUDE_BOT_COMMITS: bool = os.getenv('EXCLUDE_BOT_COMMITS', 'true').lower() in ('1', 'true', 'yes')

# Muster und Autoren, die als Bot-Commits gelten
BOT_COMMIT_TITLE_PREFIXES: List[str] = ['🤖 Auto-update:']
BOT_COMMIT_AUTHORS: List[str] = ['ThemisDB Version Bot']

# Verzeichnisse, die von der Analyse ausgeschlossen werden
EXCLUDE_DIRS: set = {
    '.git',
    'node_modules',
    'build',
    'dist',
    'vendor',
    '.github',
    'third_party',
    'external',
}

# ---------------------------------------------------------------------------
# Code-Pattern-Erkennung
# ---------------------------------------------------------------------------

# Negative Muster (Abzüge)

# Catches explicit stubs, placeholders, no-ops and "not yet implemented" patterns.
# Includes the canonical STUB/SIMULATION NOTE: comment format.
# Deliberately broad to surface undocumented shortcomings; the
# PATTERN_DOCUMENTED_STUB pattern below is used to credit proper documentation.
PATTERN_STUBS = re.compile(
    r'\b(?:stub|placeholder|noop|NotImplemented)\b'
    r'|no[-_\s]op\b'
    r'|not\s+yet\s+implement\w*'
    r'|throw\s+std::runtime_error\s*\(\s*["\']not\s+implemented["\']\s*\)',
    re.IGNORECASE,
)

# Catches simulation/mock code including camelCase variants such as
# simulateQualityTest(), "simulated gradients", mockInference(), etc.
# Uses the stem 'simulat' so it matches simulate/simulated/simulating/simulation.
PATTERN_SIMULATIONS = re.compile(
    r'\bsimulat\w*\b'
    r'|\bmock(?:ed|ing|s)?\b'
    r'|\bfake[ds]?\b'
    r'|\bdummy\b',
    re.IGNORECASE,
)

PATTERN_TODOS = re.compile(
    r'\b(TODO|FIXME|HACK|XXX|BUG)\b',
)

PATTERN_DEBUG = re.compile(
    r'\b(DEBUG)\b|cout\s*<<\s*["\']?debug|printf\s*\(["\']debug',
    re.IGNORECASE,
)

PATTERN_HARDCODED = re.compile(
    r'\b(hardcoded|temporary|temp\s+fix)\b',
    re.IGNORECASE,
)

# Catches deliberately disabled code blocks such as `if (false && condition)`
# or `// DISABLED` comments.  These indicate known-broken paths left in source.
PATTERN_DEAD_CODE = re.compile(
    r'if\s*\(\s*false\s*&&'
    r'|//\s*DISABLED\b'
    r'|//\s*disabled:',
    re.IGNORECASE,
)

# Detects the canonical STUB/SIMULATION NOTE: documentation format.
# Lines carrying this marker are already counted as stubs/simulations above,
# but their presence is credited in the score as properly-documented debt.
PATTERN_DOCUMENTED_STUB = re.compile(
    r'STUB/SIMULATION NOTE\s*:',
)

# Positive Muster (Bonuspunkte)

# NOTE: The @production_ready / production_ready bonus is intentionally small
# (3 pts, down from 10) to prevent gaming the metric by adding a single comment.
PATTERN_PRODUCTION = re.compile(
    r'@production\b|production_ready',
    re.IGNORECASE,
)

PATTERN_TESTS = re.compile(
    r'\b(TEST|EXPECT_|ASSERT_|unittest|pytest|test_)\w*\s*[\(\{]',
)

PATTERN_DOCS = re.compile(
    r'(/\*\*|"""|\'\'\')|(///\s+\w)|\* @(param|returns?|throws?|brief)',
)

# ---------------------------------------------------------------------------
# Score-Gewichtung
# ---------------------------------------------------------------------------

SCORE_PENALTIES: Dict[str, int] = {
    'stub':            7,   # up from 5 – undocumented stubs are high-priority debt
    'simulation':      5,   # up from 3 – untracked simulations need surfacing
    'todo':            3,   # up from 2 – pending work items
    'debug':           1,
    'hardcoded':       4,
    'dead_code':       6,   # new – deliberately disabled code (`if (false &&`)
    'documented_stub': 2,   # lighter penalty for properly-documented stubs
}

SCORE_BONUSES: Dict[str, Tuple[int, int]] = {
    # (Punkte pro Vorkommen, Maximum)
    # NOTE: 'production' bonus reduced from 10→3 to prevent score gaming.
    'production':       (3,  9),
    'tests':            (2,  20),
    'docs':             (1,  15),
    'documented_stub':  (1,  10),  # credit for proper STUB/SIMULATION NOTE: usage
}

# ---------------------------------------------------------------------------
# Maturity-Level-Definition
# ---------------------------------------------------------------------------

def get_maturity_level(score: float) -> str:
    """Gibt das Maturity-Level für einen Score zurück."""
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


def get_status_line(score: float) -> str:
    """Gibt eine Status-Zeile für den Header zurück."""
    if score >= 80:
        return '✅ Production Ready'
    elif score >= 60:
        return '⚠️  Needs Work'
    elif score >= 40:
        return '🔧 In Progress'
    elif score >= 20:
        return '🚧 Early Development'
    else:
        return '📝 Draft / Stub'


# ---------------------------------------------------------------------------
# Datei-Analyse
# ---------------------------------------------------------------------------

def analyze_file(file_path: Path) -> Dict[str, Any]:
    """
    Analysiert eine Source-Datei und gibt Qualitäts-Metriken zurück.
    """
    try:
        content = file_path.read_text(encoding='utf-8', errors='replace')
    except OSError as exc:
        return {'error': str(exc)}

    ext = file_path.suffix.lower()
    comment_style = SUPPORTED_EXTENSIONS.get(ext, 'c')
    # Strip the auto-generated header before analysis to avoid circular
    # detection of quality-metric keywords (e.g. "Stubs: 0") written by
    # a previous run of this script.
    body = _strip_existing_header(content, comment_style)

    lines = body.splitlines()
    # total_lines counts the full file (including the header block) so that
    # the reported line count reflects the actual size of the file on disk.
    total_lines = len(content.splitlines())

    # Vorkommen pro Pattern mit Zeilennummern
    findings: Dict[str, List[Tuple[int, str]]] = {
        'stub':            [],
        'simulation':      [],
        'todo':            [],
        'debug':           [],
        'hardcoded':       [],
        'dead_code':       [],
        'documented_stub': [],
        'production':      [],
        'tests':           [],
        'docs':            [],
    }

    pattern_map = {
        'stub':            PATTERN_STUBS,
        'simulation':      PATTERN_SIMULATIONS,
        'todo':            PATTERN_TODOS,
        'debug':           PATTERN_DEBUG,
        'hardcoded':       PATTERN_HARDCODED,
        'dead_code':       PATTERN_DEAD_CODE,
        'documented_stub': PATTERN_DOCUMENTED_STUB,
        'production':      PATTERN_PRODUCTION,
        'tests':           PATTERN_TESTS,
        'docs':            PATTERN_DOCS,
    }

    for lineno, line in enumerate(lines, start=1):
        for key, pattern in pattern_map.items():
            if pattern.search(line):
                findings[key].append((lineno, line.strip()))

    # ---------------------------------------------------------------------------
    # Score berechnen
    # ---------------------------------------------------------------------------
    # Documented stubs (STUB/SIMULATION NOTE:) are already matched by the
    # stub and simulation patterns.  To avoid triple-penalising them (stub +
    # simulation + documented_stub), subtract the documented count from the
    # effective stub and simulation counts before applying their heavier penalty,
    # then apply the lighter documented_stub penalty once.
    doc_count      = len(findings['documented_stub'])
    eff_stubs      = max(0, len(findings['stub'])       - doc_count)
    eff_simulations = max(0, len(findings['simulation']) - doc_count)

    score = 100.0
    score -= eff_stubs                    * SCORE_PENALTIES['stub']
    score -= eff_simulations              * SCORE_PENALTIES['simulation']
    score -= doc_count                    * SCORE_PENALTIES['documented_stub']
    score -= len(findings['todo'])        * SCORE_PENALTIES['todo']
    score -= len(findings['debug'])       * SCORE_PENALTIES['debug']
    score -= len(findings['hardcoded'])   * SCORE_PENALTIES['hardcoded']
    score -= len(findings['dead_code'])   * SCORE_PENALTIES['dead_code']

    for key, (bonus_per, max_bonus) in SCORE_BONUSES.items():
        earned = min(len(findings[key]) * bonus_per, max_bonus)
        score += earned

    score = max(0.0, min(100.0, score))

    counts = {k: len(v) for k, v in findings.items()}

    return {
        'path':          str(file_path),
        'total_lines':   total_lines,
        'score':         round(score, 1),
        'maturity':      get_maturity_level(score),
        'status':        get_status_line(score),
        'findings':      findings,
        'counts':        counts,
    }


# ---------------------------------------------------------------------------
# Auto-Versionierung
# ---------------------------------------------------------------------------

def load_version_tracking(tracking_path: Path) -> Dict[str, Any]:
    """Lädt die Versions-Tracking-Datei oder gibt ein leeres Dict zurück."""
    if tracking_path.exists():
        try:
            return json.loads(tracking_path.read_text(encoding='utf-8'))
        except (json.JSONDecodeError, OSError):
            pass
    return {}


def save_version_tracking(tracking_path: Path, data: Dict[str, Any]) -> None:
    """Speichert die Versions-Tracking-Datei."""
    tracking_path.parent.mkdir(parents=True, exist_ok=True)
    tracking_path.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + '\n',
        encoding='utf-8',
    )


def bump_patch_version(version: str) -> str:
    """Erhöht die Patch-Version um 1 (z.B. '1.2.3' → '1.2.4')."""
    parts = version.split('.')
    if len(parts) == 3 and all(p.isdigit() for p in parts):
        parts[2] = str(int(parts[2]) + 1)
        return '.'.join(parts)
    return version


def get_git_author() -> str:
    """Gibt den aktuellen Git-Autor zurück."""
    try:
        result = subprocess.run(
            ['git', 'config', 'user.name'],
            capture_output=True, text=True, timeout=5,
        )
        name = result.stdout.strip()
        return name if name else 'unknown'
    except (subprocess.SubprocessError, OSError):
        return 'unknown'


def _is_bot_commit(title: str, author: str) -> bool:
    """Gibt True zurück, wenn der Commit von einem automatisierten Bot stammt."""
    for prefix in BOT_COMMIT_TITLE_PREFIXES:
        if title.startswith(prefix):
            return True
    return author in BOT_COMMIT_AUTHORS


def get_file_commit_history(
    filepath: Path,
    repo_root: Path,
    max_entries: int = MAX_HISTORY_ENTRIES,
) -> List[Dict[str, str]]:
    """
    Extrahiert die letzten Commits für eine Datei aus der Git-Historie.
    Bot-Commits (z.B. von automatisierten Workflow-Runs) werden bei aktiviertem
    EXCLUDE_BOT_COMMITS ausgefiltert.

    Returns:
        Liste von Dicts mit: {'sha': str, 'date': str, 'title': str, 'author': str}
    """
    try:
        # Prüfe ob Datei in Git-Historie ist
        check = subprocess.run(
            ['git', 'ls-files', '--error-unmatch', str(filepath)],
            capture_output=True,
            cwd=repo_root,
        )
        if check.returncode != 0:
            return []

        # Wenn Bot-Commits gefiltert werden, mehr Commits anfordern damit
        # nach dem Filtern genug echte Commits übrig bleiben.
        # Faktor 3 liefert einen ausreichenden Puffer (z.B. 5 Bot + 5 echte = 10 < 15).
        fetch_count = max_entries * 3 if EXCLUDE_BOT_COMMITS else max_entries

        result = subprocess.run(
            [
                'git', 'log',
                f'-{fetch_count}',
                '--pretty=format:%h|%ad|%s|%an',
                '--date=short',
                '--follow',
                str(filepath),
            ],
            capture_output=True,
            text=True,
            cwd=repo_root,
            timeout=10,
        )

        if result.returncode != 0 or not result.stdout.strip():
            return []

        history: List[Dict[str, str]] = []
        for line in result.stdout.strip().split('\n'):
            if not line:
                continue
            parts = line.split('|', maxsplit=3)
            if len(parts) >= 4:
                title = parts[2]
                author = parts[3]
                # Bot-Commits überspringen wenn Filterung aktiv
                if EXCLUDE_BOT_COMMITS and _is_bot_commit(title, author):
                    continue
                # Titel nach Filterprüfung kürzen (Anzeige im Header)
                if len(title) > 60:
                    title = title[:57] + '...'
                history.append({
                    'sha':    parts[0],
                    'date':   parts[1],
                    'title':  title,
                    'author': author,
                })
                if len(history) >= max_entries:
                    break

        return history
    except subprocess.TimeoutExpired:
        print(f'Timeout getting commit history for {filepath}', file=sys.stderr)
        return []
    except Exception as exc:
        print(f'Error getting commit history for {filepath}: {exc}', file=sys.stderr)
        return []


def update_version(
    rel_path: str,
    tracking: Dict[str, Any],
    author: str,
    maturity: str,
    score: float,
    recent_commits: int = 0,
) -> str:
    """
    Ermittelt die neue Version für eine Datei und aktualisiert das Tracking-Dict.
    Gibt die neue Versionszeichenkette zurück.
    """
    now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')
    if rel_path in tracking:
        old_version = tracking[rel_path].get('version', '0.0.1')
        new_version = bump_patch_version(old_version)
    else:
        new_version = '0.0.1'

    tracking[rel_path] = {
        'version':        new_version,
        'last_update':    now,
        'author':         author,
        'maturity_level': maturity,
        'score':          score,
        'recent_commits': recent_commits,
    }
    return new_version


# ---------------------------------------------------------------------------
# Header-Generierung
# ---------------------------------------------------------------------------

_BOX_WIDTH = 71  # Gesamtbreite der Box inklusive Rahmen-Zeichen

def _pad(text: str, width: int) -> str:
    """Füllt Text auf width Zeichen auf (ohne Rahmen-Zeichen)."""
    return text.ljust(width)


def _build_header_lines(
    filename: str,
    version: str,
    last_modified: str,
    author: str,
    maturity: str,
    score: float,
    total_lines: int,
    todos: int,
    stubs: int,
    simulations: int,
    dead_code: int,
    documented_stubs: int,
    status: str,
    history: Optional[List[Dict[str, str]]] = None,
) -> List[str]:
    """
    Erstellt die Zeilen des formatierten Datei-Headers.
    Gibt eine Liste von Strings zurück (ohne führende Kommentar-Marker).
    """
    inner = _BOX_WIDTH - 4  # Breite des Inhalts zwischen ║ … ║

    def row(text: str) -> str:
        return f'  {_pad(text, inner)} ║'

    # Build the Open Issues detail string
    issue_parts = [f'TODOs: {todos}', f'Stubs: {stubs}', f'Simulations: {simulations}']
    if dead_code:
        issue_parts.append(f'DeadCode: {dead_code}')
    if documented_stubs:
        issue_parts.append(f'DocumentedStubs: {documented_stubs}')
    open_issues_str = ', '.join(issue_parts)

    lines = [
        '╔' + '═' * (_BOX_WIDTH - 2) + '╗',
        f'║ {_pad("ThemisDB - Hybrid Database System", _BOX_WIDTH - 4)} ║',
        '╠' + '═' * (_BOX_WIDTH - 2) + '╣',
        row(f'File:            {filename}'),
        row(f'Version:         {version}'),
        row(f'Last Modified:   {last_modified}'),
        row(f'Author:          {author}'),
        '╠' + '═' * (_BOX_WIDTH - 2) + '╣',
        row('Quality Metrics:'),
        row(f'  • Maturity Level:  {maturity}'),
        row(f'  • Quality Score:   {score}/100'),
        row(f'  • Total Lines:     {total_lines}'),
        row(f'  • Open Issues:     {open_issues_str}'),
    ]

    # Revisionshistorie hinzufügen
    if history:
        lines.append('╠' + '═' * (_BOX_WIDTH - 2) + '╣')
        lines.append(row('Revision History:'))
        for commit in history:
            commit_text = f'  • {commit["sha"]}  {commit["date"]}  {commit["title"]}'
            lines.append(row(commit_text))

    lines += [
        '╠' + '═' * (_BOX_WIDTH - 2) + '╣',
        row(f'Status: {status}'),
        '╚' + '═' * (_BOX_WIDTH - 2) + '╝',
    ]
    return lines


# Marker, der bestehende Auto-Header erkennt (für den Update-Schutz /
# genauer: das Überschreiben alter Header)
_HEADER_MARKER = 'ThemisDB - Hybrid Database System'


def _strip_existing_header(content: str, comment_style: str) -> str:
    """
    Entfernt einen bestehenden Auto-Header aus dem Datei-Inhalt.
    """
    if comment_style == 'c':
        # Sucht /*  … */ Block am Dateianfang
        pattern = re.compile(
            r'^/\*\s*\n'                       # öffnender /*
            r'(?:.*\n)*?'                       # beliebige Zeilen
            r'.*' + re.escape(_HEADER_MARKER) + r'.*\n'   # Marker-Zeile
            r'(?:.*\n)*?'                       # restliche Header-Zeilen
            r'\s*\*/\s*\n?',                   # schließendes */
            re.MULTILINE,
        )
    else:  # python / php docstring-style
        pattern = re.compile(
            r'^"""\s*\n'
            r'(?:.*\n)*?'
            r'.*' + re.escape(_HEADER_MARKER) + r'.*\n'
            r'(?:.*\n)*?'
            r'"""\s*\n?',
            re.MULTILINE,
        )

    return pattern.sub('', content, count=1).lstrip('\n')


def write_header(
    file_path: Path,
    comment_style: str,
    version: str,
    author: str,
    result: Dict[str, Any],
    history: Optional[List[Dict[str, str]]] = None,
) -> None:
    """Schreibt den formatierten Header in die Datei."""
    try:
        original = file_path.read_text(encoding='utf-8', errors='replace')
    except OSError as exc:
        print(f'⚠️  Konnte {file_path} nicht lesen: {exc}', file=sys.stderr)
        return

    now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')
    body = _strip_existing_header(original, comment_style)

    header_lines = _build_header_lines(
        filename=file_path.name,
        version=version,
        last_modified=now,
        author=author,
        maturity=result['maturity'],
        score=result['score'],
        total_lines=result['total_lines'],
        todos=result['counts']['todo'],
        stubs=result['counts']['stub'],
        simulations=result['counts']['simulation'],
        dead_code=result['counts']['dead_code'],
        documented_stubs=result['counts']['documented_stub'],
        status=result['status'],
        history=history,
    )

    if comment_style == 'c':
        header_block = '/*\n' + '\n'.join(header_lines) + '\n */\n\n'
    else:
        header_block = '"""\n' + '\n'.join(header_lines) + '\n"""\n\n'

    try:
        file_path.write_text(header_block + body, encoding='utf-8')
    except OSError as exc:
        print(f'⚠️  Konnte {file_path} nicht schreiben: {exc}', file=sys.stderr)


# ---------------------------------------------------------------------------
# Report-Generierung
# ---------------------------------------------------------------------------

def generate_report(
    results: List[Dict[str, Any]],
    tracking: Dict[str, Any],
    output_path: Path,
    repo_meta: Optional[Dict[str, str]] = None,
) -> None:
    """Generiert den Markdown-Report."""
    now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')
    total = len(results)

    # Gesamtstatistiken berechnen
    total_stubs       = sum(r['counts']['stub']            for r in results)
    total_todos       = sum(r['counts']['todo']            for r in results)
    total_sims        = sum(r['counts']['simulation']      for r in results)
    total_dead        = sum(r['counts']['dead_code']       for r in results)
    total_doc_stubs   = sum(r['counts']['documented_stub'] for r in results)
    avg_score         = (sum(r['score'] for r in results) / total) if total else 0

    dist: Dict[str, int] = {
        '🟢 PRODUCTION-READY':  0,
        '🟡 RELEASE-CANDIDATE': 0,
        '🟠 BETA':              0,
        '🔴 ALPHA':             0,
        '⚫ DRAFT':             0,
    }
    for r in results:
        lvl = r['maturity']
        if lvl in dist:
            dist[lvl] += 1

    lines: List[str] = []
    lines.append('# ThemisDB - Code Maturity Analysis\n')
    lines.append(f'**Last Updated:** {now} UTC  ')
    lines.append(f'**Analyzed Files:** {total}  ')
    lines.append(f'**Average Maturity Score:** {avg_score:.1f}/100\n')

    if repo_meta:
        lines.append('## 🧭 Repository Context\n')
        if repo_meta.get('name_with_owner'):
            lines.append(f'**Repository:** {repo_meta["name_with_owner"]}  ')
        if repo_meta.get('default_branch'):
            lines.append(f'**Default Branch:** {repo_meta["default_branch"]}  ')
        if repo_meta.get('url'):
            lines.append(f'**Remote URL:** {repo_meta["url"]}  ')
        lines.append('')

    lines.append('## 📊 Overall Statistics\n')
    lines.append('| Metric | Count |')
    lines.append('|--------|-------|')
    lines.append(f'| 🔴 Stubs / Placeholders Found | {total_stubs} |')
    lines.append(f'| 📝 TODOs/FIXMEs | {total_todos} |')
    lines.append(f'| 🎭 Simulations/Mocks | {total_sims} |')
    lines.append(f'| 💀 Dead Code Blocks (`if false &&`) | {total_dead} |')
    lines.append(f'| ✅ Documented Stubs (STUB/SIMULATION NOTE:) | {total_doc_stubs} |')
    lines.append('')

    lines.append('## 📈 Maturity Distribution\n')
    for level, count in dist.items():
        lines.append(f'- **{level}**: {count} file(s)')
    lines.append('')

    lines.append('## 📁 Detailed File Analysis\n')
    for r in results:
        rel = r['path']
        info = tracking.get(rel, {})
        version = info.get('version', 'N/A')

        lines.append(f'### `{rel}` (v{version})\n')
        lines.append(f'**Maturity Level:** {r["maturity"]} ({r["score"]}/100)\n')

        has_issues = any(
            r['counts'].get(k, 0) > 0
            for k in ('stub', 'simulation', 'todo', 'debug', 'hardcoded', 'dead_code', 'documented_stub')
        )
        if has_issues:
            lines.append('**Issues Found:**\n')

        issue_map = [
            ('stub',            '🔴 STUB/PLACEHOLDER'),
            ('simulation',      '🎭 SIMULATION'),
            ('todo',            '📝 TODO'),
            ('debug',           '🐛 DEBUG'),
            ('hardcoded',       '🔒 HARDCODED'),
            ('dead_code',       '💀 DEAD CODE'),
            ('documented_stub', '✅ DOCUMENTED STUB'),
        ]
        for key, label in issue_map:
            occurrences = r['findings'].get(key, [])
            if not occurrences:
                continue
            lines.append(f'**{label}** ({len(occurrences)} occurrences):')
            for lineno, text in occurrences[:5]:  # Maximal 5 Beispiele
                lines.append(f'  - Line {lineno}: `{text[:100]}`')
            lines.append('')

        lines.append('---\n')

    # Empfehlungen
    lines.append('## 🎯 Recommended Actions\n')
    actions = []
    if total_stubs:
        actions.append(f'1. **Implement {total_stubs} stub(s)/placeholder(s)** – Replace placeholder code with real implementations')
    if total_todos:
        actions.append(f'{len(actions)+1}. **Resolve {total_todos} TODO(s)** – Complete pending work items')
    if total_sims:
        actions.append(f'{len(actions)+1}. **Replace {total_sims} simulation(s)** – Integrate real services/data')
    if total_dead:
        actions.append(f'{len(actions)+1}. **Fix {total_dead} dead code block(s)** – Remove or re-enable `if (false &&` guarded paths')
    if total_doc_stubs:
        actions.append(
            f'{len(actions)+1}. **Track {total_doc_stubs} documented stub(s)** – '
            'Properly documented via STUB/SIMULATION NOTE: – schedule removal per Removal Plan'
        )
    if not actions:
        actions.append('✅ No critical issues found!')
    lines.extend(actions)
    lines.append('')

    output_path.write_text('\n'.join(lines), encoding='utf-8')


# ---------------------------------------------------------------------------
# Badge-Generierung
# ---------------------------------------------------------------------------

def count_category(root: Path, dirs: List[str], extensions: List[str]) -> Tuple[int, int]:
    """
    Zählt Dateien und Zeilen für eine Kategorie.

    Scannt die angegebenen Verzeichnisse unter root nach Dateien mit den
    gegebenen Erweiterungen und gibt (file_count, total_lines) zurück.
    Nicht vorhandene Verzeichnisse werden übersprungen.
    """
    file_count  = 0
    total_lines = 0
    ext_set     = set(extensions)
    for dir_name in dirs:
        dir_path = root / dir_name
        if not dir_path.is_dir():
            continue
        for dirpath, _dirnames, filenames in os.walk(dir_path):
            for fname in filenames:
                if Path(fname).suffix.lower() in ext_set:
                    file_count += 1
                    try:
                        total_lines += sum(
                            1 for _ in (Path(dirpath) / fname).open(
                                encoding='utf-8', errors='replace'
                            )
                        )
                    except OSError:
                        pass
    return file_count, total_lines


def _write_badge(
    badges_dir: Path,
    filename: str,
    label: str,
    message: str,
    color: str,
) -> None:
    """Schreibt eine einzelne Shields.io-Endpoint-JSON-Badge-Datei."""
    badge = {
        'schemaVersion': 1,
        'label':         label,
        'message':       message,
        'color':         color,
    }
    (badges_dir / filename).write_text(
        json.dumps(badge, indent=2) + '\n', encoding='utf-8'
    )


def generate_category_badges(root: Path) -> None:
    """Generiert 10 Shields.io-kompatible JSON-Badge-Dateien in .github/badges/."""
    badges_dir = root / '.github' / 'badges'
    badges_dir.mkdir(parents=True, exist_ok=True)

    categories = [
        {
            'dirs':       ['src', 'include'],
            'extensions': ['.cpp', '.h', '.hpp', '.c'],
            'loc_file':   'lines-of-code.json',
            'fc_file':    'file-count.json',
            'loc_label':  'Lines of Code (Core)',
            'fc_label':   'Core Files',
        },
        {
            'dirs':       ['tests'],
            'extensions': ['.cpp', '.py', '.cs'],
            'loc_file':   'lines-of-code-tests.json',
            'fc_file':    'file-count-tests.json',
            'loc_label':  'Lines of Code (Tests)',
            'fc_label':   'Test Files',
        },
        {
            'dirs':       ['benchmarks'],
            'extensions': ['.py', '.cpp'],
            'loc_file':   'lines-of-code-benchmarks.json',
            'fc_file':    'file-count-benchmarks.json',
            'loc_label':  'Lines of Code (Benchmarks)',
            'fc_label':   'Benchmark Files',
        },
        {
            'dirs':       ['docs', 'compendium'],
            'extensions': ['.md'],
            'loc_file':   'lines-of-code-docs.json',
            'fc_file':    'file-count-docs.json',
            'loc_label':  'Lines of Code (Docs)',
            'fc_label':   'Doc Files',
        },
        {
            'dirs':       ['tools', 'scripts', 'projects'],
            'extensions': ['.py', '.cs', '.php'],
            'loc_file':   'lines-of-code-tools.json',
            'fc_file':    'file-count-tools.json',
            'loc_label':  'Lines of Code (Tools)',
            'fc_label':   'Tool Files',
        },
    ]

    for cat in categories:
        file_count, total_lines = count_category(root, cat['dirs'], cat['extensions'])
        _write_badge(badges_dir, cat['loc_file'], cat['loc_label'], f'{total_lines:,}', 'blue')
        _write_badge(badges_dir, cat['fc_file'],  cat['fc_label'],  str(file_count),    'green')


# ---------------------------------------------------------------------------
# Haupt-Routine
# ---------------------------------------------------------------------------

def find_source_files(root: Path) -> List[Path]:
    """Findet alle unterstützten Source-Dateien unter root."""
    files: List[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        # Ausgeschlossene Verzeichnisse überspringen
        dirnames[:] = [
            d for d in dirnames
            if d not in EXCLUDE_DIRS
        ]
        for fname in filenames:
            ext = Path(fname).suffix.lower()
            if ext in SUPPORTED_EXTENSIONS:
                files.append(Path(dirpath) / fname)
    return sorted(files)


def _parse_owner_repo_from_remote(remote_url: str) -> Tuple[Optional[str], Optional[str]]:
    """Best-effort parsing of owner/repo from common git remote URL formats."""
    if not remote_url:
        return None, None

    cleaned = remote_url.strip()
    if cleaned.endswith('.git'):
        cleaned = cleaned[:-4]

    # SSH format: git@github.com:owner/repo
    m = re.match(r'^[^@]+@[^:]+:(?P<owner>[^/]+)/(?P<repo>[^/]+)$', cleaned)
    if m:
        return m.group('owner'), m.group('repo')

    # HTTPS format: https://github.com/owner/repo
    try:
        parsed = urlparse(cleaned)
        if parsed.path:
            parts = [p for p in parsed.path.split('/') if p]
            if len(parts) >= 2:
                return parts[0], parts[1]
    except Exception:
        pass

    return None, None


def get_repo_metadata(root: Path) -> Dict[str, str]:
    """
    Resolve repository metadata for report context.

    Priority:
      1) gh repo view --json ... (local-friendly when gh auth is configured)
      2) git remote + git rev-parse fallback
    """
    meta: Dict[str, str] = {}

    # 1) Preferred: GitHub CLI (works locally with auth)
    try:
        gh = subprocess.run(
            [
                'gh', 'repo', 'view',
                '--json', 'nameWithOwner,defaultBranchRef,url',
            ],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=8,
        )
        if gh.returncode == 0 and gh.stdout.strip():
            data = json.loads(gh.stdout)
            meta['name_with_owner'] = data.get('nameWithOwner', '')
            meta['default_branch'] = (data.get('defaultBranchRef') or {}).get('name', '')
            meta['url'] = data.get('url', '')
            # If gh worked we already have the richest source.
            return {k: v for k, v in meta.items() if v}
    except Exception:
        pass

    # 2) Fallback: git origin + branch parsing
    try:
        remote = subprocess.run(
            ['git', 'remote', 'get-url', 'origin'],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=5,
        )
        if remote.returncode == 0:
            remote_url = remote.stdout.strip()
            meta['url'] = remote_url
            owner, repo = _parse_owner_repo_from_remote(remote_url)
            if owner and repo:
                meta['name_with_owner'] = f'{owner}/{repo}'
    except Exception:
        pass

    try:
        branch = subprocess.run(
            ['git', 'rev-parse', '--abbrev-ref', 'origin/HEAD'],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=5,
        )
        if branch.returncode == 0 and branch.stdout.strip().startswith('origin/'):
            meta['default_branch'] = branch.stdout.strip().split('/', 1)[1]
    except Exception:
        pass

    return {k: v for k, v in meta.items() if v}


def main() -> int:
    parser = argparse.ArgumentParser(
        description='ThemisDB Code-Maturity-Analyse und Auto-Versionierung',
    )
    parser.add_argument(
        '--root',
        default='.',
        help='Repository-Root (Standard: aktuelles Verzeichnis)',
    )
    parser.add_argument(
        '--no-headers',
        action='store_true',
        help='Header-Aktualisierung deaktivieren',
    )
    parser.add_argument(
        '--report-path',
        default=None,
        help='Ausgabepfad für den Markdown-Report (Standard: docs/code_maturity_report.md)',
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    # Header writing is permanently disabled in this script.
    # The canonical header writer is code_maturity_header_writer.py.
    # Keeping write_headers=False prevents the legacy compact C-style block
    # (/* ThemisDB | File: ... */) from being regenerated alongside the
    # Doxygen block produced by code_maturity_header_writer.py.
    write_headers = False
    repo_meta = get_repo_metadata(root)

    tracking_path = root / '.github' / 'version_tracking.json'
    # Use the provided --report-path, falling back to the canonical docs location.
    # The old default (FEATURE_ENHANCEMENT.md) was semantically incorrect as that
    # file is the product roadmap/feature wishlist, not a code quality report.
    if args.report_path:
        report_path = Path(args.report_path)
        if not report_path.is_absolute():
            report_path = root / report_path
    else:
        report_path = root / 'docs' / 'code_maturity_report.md'
    report_path.parent.mkdir(parents=True, exist_ok=True)

    print('🔍 Starting code maturity analysis...')
    if write_headers:
        print('📝 Header updates ENABLED')
    else:
        print('📝 Header updates DISABLED')

    author   = get_git_author()
    tracking = load_version_tracking(tracking_path)
    files    = find_source_files(root)

    results: List[Dict[str, Any]] = []
    total_commits = 0
    for file_path in files:
        rel = str(file_path.relative_to(root))
        ext = file_path.suffix.lower()
        comment_style = SUPPORTED_EXTENSIONS.get(ext, 'c')

        result = analyze_file(file_path)
        if 'error' in result:
            print(f'⚠️  Fehler bei {rel}: {result["error"]}', file=sys.stderr)
            continue

        result['path'] = rel

        # Commit-Historie ermitteln (einmal für Header + Tracking)
        history = get_file_commit_history(file_path, root) if write_headers else []
        num_commits = len(history)

        # Versions-Tracking aktualisieren
        version = update_version(
            rel, tracking, author,
            result['maturity'], result['score'],
            recent_commits=num_commits,
        )

        # Header schreiben
        if write_headers:
            write_header(file_path, comment_style, version, author, result, history)
            total_commits += num_commits
            print(f'✅ Updated header: {rel} ({num_commits} commits in history)')

        results.append(result)

    print(f'\n📊 Analyzed {len(results)} files')
    if write_headers:
        print(f'📈 Total commits in history: {total_commits:,}')

    # Report generieren
    generate_report(results, tracking, report_path, repo_meta=repo_meta)
    try:
        display_path = report_path.relative_to(root)
    except ValueError:
        display_path = report_path
    print(f'✅ Report written to {display_path}')

    # Versions-Tracking speichern
    save_version_tracking(tracking_path, tracking)
    print('✅ Version data saved')

    # Shields.io Badge-Dateien generieren
    generate_category_badges(root)
    print('✅ Badges updated')

    return 0


if __name__ == '__main__':
    sys.exit(main())
