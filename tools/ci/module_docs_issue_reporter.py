"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_docs_issue_reporter.py                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:54:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     496                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • afcb89febb  2026-03-12  fix: robustness/performance/efficiency improvements for d... ║
    • 212c6d4a65  2026-03-12  feat: add changelog_updater, module_docs_issue_reporter, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Module-Docs Issue Reporter
=====================================

Creates GitHub Issues for documentation findings that require human expertise.

Two report modes, auto-detected from the JSON structure:

  module  — underdocumented/new modules (produced by module_docs_builder.py
            ``--issues-json``).  An issue is opened per module that has no
            human-authored secondary documentation beyond the auto-generated
            ``PRIMARY_SOURCES.md``.

  drift   — drifting/stale secondary docs (produced by
            ``scripts/drift-detector.py --format json``).  An issue is opened
            per secondary-doc file that has drifted past the configured
            threshold.

Deduplication
-------------
Before creating an issue the script calls ``gh issue list --search`` to check
whether an open issue with the same title already exists.  If one is found the
creation is skipped.

Usage
-----
    python3 tools/ci/module_docs_issue_reporter.py \\
        --report   <path-to-json>    \\
        --repo     makr-code/ThemisDB \\
        [--dry-run] [--quiet]

Exit codes
----------
    0   Success
    1   Unrecoverable error (gh CLI missing, repo not specified, …)
"""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DEFAULT_REPO = "makr-code/ThemisDB"

# Label sets (must already exist in the target repository).
LABELS_MODULE = ["type:documentation", "area:docs-audit", "priority:medium", "status:open"]
LABELS_DRIFT_DRIFTING = ["type:documentation", "area:docs-audit", "priority:low", "status:open"]
LABELS_DRIFT_STALE = ["type:documentation", "area:docs-audit", "priority:medium", "status:open"]

TITLE_PREFIX_MODULE = "[docs-sync]"
TITLE_PREFIX_DRIFT = "[docs-drift]"

# [R2] Default timeout for all gh CLI subprocess calls (seconds).
_GH_TIMEOUT = 30


# ---------------------------------------------------------------------------
# gh CLI helpers
# ---------------------------------------------------------------------------


def _run(args: List[str], timeout: int = _GH_TIMEOUT) -> Tuple[int, str, str]:
    """Run a subprocess and return (returncode, stdout, stderr).

    [R2] ``timeout`` prevents the gh CLI from hanging CI indefinitely.
    ``FileNotFoundError`` is caught so callers get a clean (127, "", …) tuple
    instead of an unhandled exception when gh is not installed.
    """
    try:
        result = subprocess.run(
            args, capture_output=True, text=True, timeout=timeout
        )
        return result.returncode, result.stdout.strip(), result.stderr.strip()
    except subprocess.TimeoutExpired:
        return 1, "", f"timed out after {timeout}s: {' '.join(args)}"
    except FileNotFoundError:
        return 127, "", f"command not found: {args[0]}"


def _gh_available() -> bool:
    rc, _, _ = _run(["gh", "--version"])
    return rc == 0


def _list_open_issue_titles(repo: str, prefix: str) -> frozenset:
    """Return all open issue titles that start with *prefix* (single API call).

    [P1] Replaces the per-issue ``gh issue list --search`` pattern so the
    deduplication check requires only **one** network round-trip regardless of
    how many issues are in the report.

    [P2] ``--limit 500`` ensures we don't silently miss existing issues when
    more than the default 30 are open.
    """
    rc, stdout, _ = _run([
        "gh", "issue", "list",
        "--repo", repo,
        "--state", "open",
        "--search", prefix,
        "--json", "title",
        "--jq", ".[].title",
        "--limit", "500",
    ])
    if rc != 0:
        return frozenset()
    return frozenset(line for line in stdout.splitlines() if line.strip())


def _create_issue(
    repo: str,
    title: str,
    labels: List[str],
    body: str,
    dry_run: bool,
) -> bool:
    """Create a GitHub issue.  Returns True on success."""
    if dry_run:
        return True
    rc, stdout, stderr = _run([
        "gh", "issue", "create",
        "--repo", repo,
        "--title", title,
        "--label", ",".join(labels),
        "--body", body,
    ])
    if rc != 0:
        print(f"  ❌ gh error: {stderr or 'unknown'}", file=sys.stderr)
        return False
    return True


# ---------------------------------------------------------------------------
# Report format detection
# ---------------------------------------------------------------------------


def _detect_format(report: Dict[str, Any]) -> str:
    """Return 'module' or 'drift' based on JSON structure."""
    if "entries" in report and "drift_count" in report:
        return "drift"
    if "underdocumented_modules" in report or "modules" in report:
        return "module"
    return "unknown"


# ---------------------------------------------------------------------------
# Module findings → issues
# ---------------------------------------------------------------------------


def _module_issue_title(module: str) -> str:
    return f"{TITLE_PREFIX_MODULE} Modul `{module}`: Sekundärdokumentation fehlt"


def _module_issue_body(module: str, info: Dict[str, Any]) -> str:
    files = info.get("files", [])
    de_human = info.get("de_human_authored", 0)
    file_list = "\n".join(f"- `{f}`" for f in files[:20])
    if len(files) > 20:
        file_list += f"\n- … und {len(files) - 20} weitere"

    return f"""\
## Zusammenfassung

Das Modul **`{module}`** wurde im Module-Docs-Sync-Lauf erkannt, hat aber \
noch keine menschlich erstellte Sekundärdokumentation in `docs/de/{module}/`.

| Feld | Wert |
|------|------|
| **Modul** | `{module}` |
| **Primary-Markdown-Dateien** | {len(files)} |
| **Menschlich erstellte DE-Docs** | {de_human} |
| **Auto-generiert** | `docs/de/{module}/PRIMARY_SOURCES.md` ✅ |
| **Erkannt von** | `tools/module_docs_builder.py` |

## Primary Sources

{file_list}

## Fehlende Sekundärdokumentation

Mindestens eine der folgenden Dateien sollte manuell erstellt werden:

- [ ] `docs/de/{module}/architecture.md` — Architektur & Designentscheidungen
- [ ] `docs/de/{module}/README.md` — Modulübersicht für Nutzer/Entwickler
- [ ] `docs/de/{module}/feature.md` — Feature-Beschreibung

## Akzeptanzkriterien

- [ ] Mindestens eine menschlich erstellte Sekundärdoku-Datei in `docs/de/{module}/` vorhanden
- [ ] Breadcrumb, Datum, Status und Primary-Quellenangabe korrekt gesetzt
- [ ] `docs/en/{module}/` entsprechend ergänzt (oder verlinkt)
- [ ] Kein Drift erkannt (`Status: stable` oder `review`)

## Referenz

- Auto-generierter Index: [`docs/de/{module}/PRIMARY_SOURCES.md`](../docs/de/{module}/PRIMARY_SOURCES.md)
- Tool: `tools/module_docs_builder.py`
- Doku-Standard: `docs/CONTENT_MODEL.md`
"""


def process_module_report(
    report: Dict[str, Any],
    repo: str,
    dry_run: bool,
    quiet: bool,
) -> Tuple[int, int, int]:
    """Process a module-findings report.  Returns (created, skipped, failed)."""
    modules_info: Dict[str, Any] = report.get("modules", {})
    underdoc: List[str] = report.get("underdocumented_modules", [])

    created = skipped = failed = 0

    # [P1] Fetch all existing module-docs issue titles in one API call.
    if not quiet:
        print(f"  Fetching existing '{TITLE_PREFIX_MODULE}' issues …")
    existing_titles = (
        _list_open_issue_titles(repo, TITLE_PREFIX_MODULE)
        if not dry_run
        else frozenset()
    )
    if not quiet:
        print(f"  Found {len(existing_titles)} existing issue(s).\n")

    for module in sorted(underdoc):
        title = _module_issue_title(module)
        info = modules_info.get(module, {})

        if title in existing_titles:
            if not quiet:
                print(f"  [{module}] ⏭  already exists — skipped")
            skipped += 1
            continue

        body = _module_issue_body(module, info)
        ok = _create_issue(repo, title, LABELS_MODULE, body, dry_run)

        if ok:
            if not quiet:
                print(f"  [{module}] ✅ {'(dry-run) would create' if dry_run else 'created'}")
            created += 1
        else:
            failed += 1

    return created, skipped, failed


# ---------------------------------------------------------------------------
# Drift findings → issues
# ---------------------------------------------------------------------------


def _drift_issue_title(file_path: str) -> str:
    return f"{TITLE_PREFIX_DRIFT} `{file_path}`: Sekundärdoku hinter Primärquelle zurück"


def _drift_issue_body(entry: Dict[str, Any]) -> str:
    file_path = entry["file"]
    age_days = entry["age_days"]
    status = entry["status"]
    primary_refs = entry.get("primary_refs", [])
    sec_mtime = entry.get("secondary_mtime", "–")
    pri_mtime = entry.get("max_primary_mtime", "–")

    status_emoji = "🔴" if status == "stale" else "🟡"
    primary_list = "\n".join(f"- `{r}`" for r in primary_refs)

    return f"""\
## Zusammenfassung

Die Sekundärdokumentation **`{file_path}`** ist **{age_days} Tage** hinter \
ihrer Primärquelle zurück und wurde als {status_emoji} **{status.upper()}** markiert.

| Feld | Wert |
|------|------|
| **Secondary Doc** | `{file_path}` |
| **Status** | {status_emoji} `{status}` |
| **Rückstand** | {age_days} Tage |
| **Secondary zuletzt geändert** | `{sec_mtime}` |
| **Primary zuletzt geändert** | `{pri_mtime}` |
| **Erkannt von** | `scripts/drift-detector.py` |

## Primärquellen

{primary_list}

## Erforderliche Maßnahmen

- [ ] Sekundärdokumentation `{file_path}` überprüfen und mit Primärquelle abgleichen
- [ ] `**Status:**` auf `stable` setzen sobald aktuell (oder `deprecated` wenn veraltet)
- [ ] Breadcrumb und Primary-Quellenangabe aktuell halten
- [ ] Englische Entsprechung prüfen (falls vorhanden)

## Akzeptanzkriterien

- [ ] Inhalt wurde mit der Primärquelle abgeglichen
- [ ] `**Status:** stable` (oder `deprecated`) gesetzt
- [ ] Kein neuer Drift nach dem Update erkannt

## Referenz

- Drift-Erkennung: `scripts/drift-detector.py`
- Doku-Standard: `docs/CONTENT_MODEL.md`
- Primary-Index: `docs/_generated/primary_index.json`
"""


def process_drift_report(
    report: Dict[str, Any],
    repo: str,
    dry_run: bool,
    quiet: bool,
) -> Tuple[int, int, int]:
    """Process a drift-detector report.  Returns (created, skipped, failed)."""
    entries: List[Dict[str, Any]] = report.get("entries", [])
    created = skipped = failed = 0

    # [P1] Fetch all existing drift-issue titles in one API call.
    if not quiet:
        print(f"  Fetching existing '{TITLE_PREFIX_DRIFT}' issues …")
    existing_titles = (
        _list_open_issue_titles(repo, TITLE_PREFIX_DRIFT)
        if not dry_run
        else frozenset()
    )
    if not quiet:
        print(f"  Found {len(existing_titles)} existing issue(s).\n")

    for entry in entries:
        file_path = entry.get("file", "")
        status = entry.get("status", "drifting")
        title = _drift_issue_title(file_path)
        labels = LABELS_DRIFT_STALE if status == "stale" else LABELS_DRIFT_DRIFTING

        if title in existing_titles:
            if not quiet:
                print(f"  [{status}] ⏭  {file_path} — already exists, skipped")
            skipped += 1
            continue

        body = _drift_issue_body(entry)
        ok = _create_issue(repo, title, labels, body, dry_run)

        if ok:
            if not quiet:
                print(f"  [{status}] ✅ {'(dry-run) would create' if dry_run else 'created'}: {file_path}")
            created += 1
        else:
            failed += 1

    return created, skipped, failed


# ---------------------------------------------------------------------------
# Step summary
# ---------------------------------------------------------------------------


def _write_step_summary(
    report_type: str,
    created: int,
    skipped: int,
    failed: int,
    dry_run: bool,
) -> None:
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    mode_note = " *(dry-run)*" if dry_run else ""
    with open(path, "a", encoding="utf-8") as fh:
        fh.write(f"## 📝 Module-Docs Issue Reporter{mode_note}\n\n")
        fh.write(f"| Parameter | Value |\n")
        fh.write(f"|-----------|-------|\n")
        fh.write(f"| **Report type** | `{report_type}` |\n")
        fh.write(f"| **Issues created** | {created} |\n")
        fh.write(f"| **Already existed (skipped)** | {skipped} |\n")
        fh.write(f"| **Failed** | {failed} |\n")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Create GitHub Issues for documentation findings requiring human review.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--report",
        required=True,
        metavar="PATH",
        help="Path to the JSON report (module or drift format, auto-detected)",
    )
    p.add_argument(
        "--repo",
        default=DEFAULT_REPO,
        metavar="OWNER/REPO",
        help=f"GitHub repository (default: {DEFAULT_REPO})",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Check for duplicates and report, but do not create issues",
    )
    p.add_argument("--quiet", action="store_true", help="Suppress informational output")
    return p


def main(argv=None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    report_path = Path(args.report)
    if not report_path.exists():
        print(f"ERROR: report file not found: {report_path}", file=sys.stderr)
        return 1

    if not args.dry_run and not _gh_available():
        print("ERROR: gh CLI not found. Install it or use --dry-run.", file=sys.stderr)
        return 1

    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"ERROR: cannot parse JSON report: {exc}", file=sys.stderr)
        return 1

    fmt = _detect_format(report)

    if not args.quiet:
        print(f"Module-Docs Issue Reporter")
        print(f"  Report  : {report_path}")
        print(f"  Format  : {fmt}")
        print(f"  Repo    : {args.repo}")
        print(f"  Mode    : {'dry-run' if args.dry_run else 'live'}")
        print()

    if fmt == "module":
        created, skipped, failed = process_module_report(
            report, args.repo, args.dry_run, args.quiet
        )
    elif fmt == "drift":
        created, skipped, failed = process_drift_report(
            report, args.repo, args.dry_run, args.quiet
        )
    else:
        print(f"ERROR: unrecognised report format (keys: {list(report.keys())})", file=sys.stderr)
        return 1

    if not args.quiet:
        print()
        print(f"Issues created : {created}")
        print(f"Skipped (exist): {skipped}")
        print(f"Failed         : {failed}")

    _write_step_summary(fmt, created, skipped, failed, args.dry_run)

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
