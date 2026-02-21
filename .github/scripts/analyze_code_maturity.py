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
PATTERN_STUBS = re.compile(
    r'\b(stub|STUB|NotImplemented|'
    r'throw\s+std::runtime_error\s*\(\s*["\']not implemented["\'])',
    re.IGNORECASE,
)

PATTERN_SIMULATIONS = re.compile(
    r'\b(simulate|mock|fake|dummy|SIMULATION)\b',
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

# Positive Muster (Bonuspunkte)
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
    'stub':       5,
    'simulation': 3,
    'todo':       2,
    'debug':      1,
    'hardcoded':  4,
}

SCORE_BONUSES: Dict[str, Tuple[int, int]] = {
    # (Punkte pro Vorkommen, Maximum)
    'production': (10, 9999),
    'tests':      (2,  20),
    'docs':       (1,  15),
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

    lines = content.splitlines()
    total_lines = len(lines)

    # Vorkommen pro Pattern mit Zeilennummern
    findings: Dict[str, List[Tuple[int, str]]] = {
        'stub':       [],
        'simulation': [],
        'todo':       [],
        'debug':      [],
        'hardcoded':  [],
        'production': [],
        'tests':      [],
        'docs':       [],
    }

    pattern_map = {
        'stub':       PATTERN_STUBS,
        'simulation': PATTERN_SIMULATIONS,
        'todo':       PATTERN_TODOS,
        'debug':      PATTERN_DEBUG,
        'hardcoded':  PATTERN_HARDCODED,
        'production': PATTERN_PRODUCTION,
        'tests':      PATTERN_TESTS,
        'docs':       PATTERN_DOCS,
    }

    for lineno, line in enumerate(lines, start=1):
        for key, pattern in pattern_map.items():
            if pattern.search(line):
                findings[key].append((lineno, line.strip()))

    # Score berechnen
    score = 100.0
    for key, penalty in SCORE_PENALTIES.items():
        score -= len(findings[key]) * penalty

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


def update_version(
    rel_path: str,
    tracking: Dict[str, Any],
    author: str,
    maturity: str,
    score: float,
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
    status: str,
) -> List[str]:
    """
    Erstellt die Zeilen des formatierten Datei-Headers.
    Gibt eine Liste von Strings zurück (ohne führende Kommentar-Marker).
    """
    inner = _BOX_WIDTH - 4  # Breite des Inhalts zwischen ║ … ║

    def row(text: str) -> str:
        return f'  {_pad(text, inner)} ║'

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
        row(f'  • Open Issues:     TODOs: {todos}, Stubs: {stubs}'),
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
        status=result['status'],
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
) -> None:
    """Generiert den Markdown-Report."""
    now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')
    total = len(results)

    # Gesamtstatistiken berechnen
    total_stubs   = sum(r['counts']['stub']       for r in results)
    total_todos   = sum(r['counts']['todo']        for r in results)
    total_sims    = sum(r['counts']['simulation']  for r in results)
    avg_score     = (sum(r['score'] for r in results) / total) if total else 0

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

    lines.append('## 📊 Overall Statistics\n')
    lines.append('| Metric | Count |')
    lines.append('|--------|-------|')
    lines.append(f'| 🔴 Stubs Found | {total_stubs} |')
    lines.append(f'| 📝 TODOs/FIXMEs | {total_todos} |')
    lines.append(f'| 🎭 Simulations/Mocks | {total_sims} |')
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
            for k in ('stub', 'simulation', 'todo', 'debug', 'hardcoded')
        )
        if has_issues:
            lines.append('**Issues Found:**\n')

        issue_map = [
            ('stub',       '🔴 STUB'),
            ('simulation', '🎭 SIMULATION'),
            ('todo',       '📝 TODO'),
            ('debug',      '🐛 DEBUG'),
            ('hardcoded',  '🔒 HARDCODED'),
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
        actions.append(f'1. **Implement {total_stubs} stub(s)** - Replace placeholder code with real implementations')
    if total_todos:
        actions.append(f'{len(actions)+1}. **Resolve {total_todos} TODO(s)** - Complete pending work items')
    if total_sims:
        actions.append(f'{len(actions)+1}. **Replace {total_sims} simulation(s)** - Integrate real services/data')
    if not actions:
        actions.append('✅ No critical issues found!')
    lines.extend(actions)
    lines.append('')

    output_path.write_text('\n'.join(lines), encoding='utf-8')


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
    args = parser.parse_args()

    root = Path(args.root).resolve()
    write_headers = not args.no_headers

    tracking_path = root / '.github' / 'version_tracking.json'
    report_path   = root / 'feature_enhancement.md'

    print('🔍 Starting code maturity analysis...')
    if write_headers:
        print('📝 Header updates ENABLED')
    else:
        print('📝 Header updates DISABLED')

    author   = get_git_author()
    tracking = load_version_tracking(tracking_path)
    files    = find_source_files(root)

    results: List[Dict[str, Any]] = []
    for file_path in files:
        rel = str(file_path.relative_to(root))
        ext = file_path.suffix.lower()
        comment_style = SUPPORTED_EXTENSIONS.get(ext, 'c')

        result = analyze_file(file_path)
        if 'error' in result:
            print(f'⚠️  Fehler bei {rel}: {result["error"]}', file=sys.stderr)
            continue

        result['path'] = rel

        # Versions-Tracking aktualisieren
        version = update_version(
            rel, tracking, author,
            result['maturity'], result['score'],
        )

        # Header schreiben
        if write_headers:
            write_header(file_path, comment_style, version, author, result)
            print(f'✅ Updated header: {rel}')

        results.append(result)

    print(f'\n📊 Analyzed {len(results)} files')

    # Report generieren
    generate_report(results, tracking, report_path)
    print(f'✅ Report written to {report_path.relative_to(root)}')

    # Versions-Tracking speichern
    save_version_tracking(tracking_path, tracking)
    print('✅ Version data saved')

    return 0


if __name__ == '__main__':
    sys.exit(main())
