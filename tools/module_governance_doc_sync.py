#!/usr/bin/env python3
"""
Content-aware synchronization for module governance docs based on v3 gap data.

Updates per module (if file exists):
- src/<module>/ROADMAP.md
- src/<module>/FUTURE_ENHANCEMENTS.md
- src/<module>/AUDIT.md
- src/<module>/SECURITY.md

Unlike a plain snapshot injector, this script continues existing documents:
- it reads current content,
- locates semantic anchor sections,
- inserts/replaces a focused "Fortschreibung" block near those sections.
"""

from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple

BEGIN_MARKER = "<!-- BEGIN AUTO_GAP_SYNC_CONTINUATION -->"
END_MARKER = "<!-- END AUTO_GAP_SYNC_CONTINUATION -->"

TARGET_DOCS = [
    "ROADMAP.md",
    "FUTURE_ENHANCEMENTS.md",
    "AUDIT.md",
    "SECURITY.md",
]


def _load_json(path: Path) -> Dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _top_categories(by_category: Dict[str, int], limit: int = 5) -> List[Tuple[str, int]]:
    return sorted(by_category.items(), key=lambda item: (-int(item[1]), item[0]))[:limit]


def _top_files(by_file: Dict[str, List[Dict]], limit: int = 5) -> List[Tuple[str, int]]:
    items = [(path.replace('\\\\', '/'), len(findings)) for path, findings in by_file.items()]
    return sorted(items, key=lambda item: (-item[1], item[0]))[:limit]


def _severity(payload: Dict) -> Tuple[int, int, int, int]:
    return (
        int(payload.get("severity_critical", 0) or 0),
        int(payload.get("severity_high", 0) or 0),
        int(payload.get("severity_medium", 0) or 0),
        int(payload.get("severity_low", 0) or 0),
    )


def _build_block(module: str, doc_name: str, payload: Dict, generated_at: str) -> str:
    total = int(payload.get("total", 0) or 0)
    critical, high, medium, low = _severity(payload)
    actionable = critical + high
    categories = _top_categories(dict(payload.get("by_category", {}) or {}), limit=5)
    files = _top_files(dict(payload.get("by_file", {}) or {}), limit=5)

    lines: List[str] = [
        BEGIN_MARKER,
        f"### Gap-Driven Fortschreibung (v3) - {generated_at}",
        "",
        f"- Modul: {module}",
        f"- Gesamtbefunde: {total}",
        f"- Prioritaet (Critical + High): {actionable}",
        f"- Severity: Critical={critical}, High={high}, Medium={medium}, Low={low}",
        "",
        "#### Leitkategorien fuer die naechste Iteration",
        "",
    ]

    if categories:
        for name, count in categories:
            lines.append(f"- {name}: {count}")
    else:
        lines.append("- Keine Kategorien im aktuellen Snapshot")

    lines.extend([
        "",
        "#### Fokusdateien",
        "",
    ])

    if files:
        for path, count in files:
            lines.append(f"- {path}: {count}")
    else:
        lines.append("- Keine Datei-Funde im aktuellen Snapshot")

    lines.extend([
        "",
        "#### Quellen",
        "",
        "- Scanner-Artefakt: ai_working/gap_scan_v3_aggregate.json",
        "- Moduldetails: src/<module>/MODULE_GAPS.md",
        "",
        "#### Fortzuschreibende Inhalte",
        "",
    ])

    if doc_name == "ROADMAP.md":
        lines.extend([
            "- [~] In-Progress-Abschnitt auf Critical/High-Fokus nachziehen.",
            "- [ ] Geplante Features um konkrete Gap-Reduktionsziele erweitern.",
            "- [ ] Produktionsreife-Checklist mit messbaren Kriterien gegen Top-Dateien ergaenzen.",
        ])
    elif doc_name == "FUTURE_ENHANCEMENTS.md":
        lines.extend([
            "- [ ] Scope/Constraints auf Top-Kategorien konkretisieren.",
            "- [ ] Required Interfaces und Test Strategy an Fokusdateien ausrichten.",
            "- [ ] Messbare Performance-/Security-Targets pro Enhancement nachziehen.",
        ])
    elif doc_name == "AUDIT.md":
        lines.extend([
            "- [ ] Findings-/Evidence-Teil mit Top-Dateien und Severity priorisieren.",
            "- [ ] Nachweislinks fuer behobene Critical-Befunde einpflegen.",
            "- [ ] Offene High-Risk-Pfade mit Owner, Termin und Testnachweis markieren.",
        ])
    elif doc_name == "SECURITY.md":
        lines.extend([
            "- [ ] Threat Model und Mitigations auf Top-Kategorien konkretisieren.",
            "- [ ] Top-Dateien auf Input-Validation/Secrets/Fail-Closed explizit referenzieren.",
            "- [ ] Security-Backlog und Hardening-Checklist mit aktuellen Befunden synchronisieren.",
        ])
    else:
        lines.append("- [ ] Befunde in Modul-Backlog übernehmen.")

    lines.extend([
        "",
        END_MARKER,
        "",
    ])

    return "\n".join(lines)


def _replace_or_insert_block(content: str, block: str) -> str:
    if BEGIN_MARKER in content and END_MARKER in content:
        start = content.index(BEGIN_MARKER)
        end = content.index(END_MARKER, start) + len(END_MARKER)
        before = content[:start].rstrip()
        after = content[end:].lstrip("\n")
        new_content = f"{before}\n\n{block}"
        if after:
            new_content += after if after.startswith("\n") else f"\n{after}"
        return new_content

    lines = content.splitlines()

    def find_anchor(candidates: List[str]) -> int | None:
        lowered = [line.lower().strip() for line in lines]
        for cand in candidates:
            c = cand.lower()
            for idx, line in enumerate(lowered):
                if line.startswith("#") and c in line:
                    return idx + 1
        return None

    # Choose context-sensitive insertion point so docs are continued, not overwritten.
    if "# core module roadmap" in content.lower() or "roadmap" in content.lower():
        insert_at = find_anchor(["current status", "in progress", "planned features", "implementation phases"])
    elif "future enhancements" in content.lower():
        insert_at = find_anchor(["design constraints", "required interfaces", "implementation notes", "planned features"])
    elif "audit" in content.lower():
        insert_at = find_anchor(["summary", "findings", "test coverage", "compliance"])
    elif "security" in content.lower():
        insert_at = find_anchor(["threat model", "security controls", "known limitations", "hardening checklist"])
    else:
        insert_at = None

    if insert_at is None:
        h1_idx = next((idx for idx, line in enumerate(lines) if line.startswith("# ")), None)
        if h1_idx is None:
            return f"{block}{content}"
        insert_at = h1_idx + 1

    # Skip empty lines after anchor.
    while insert_at < len(lines) and lines[insert_at].strip() == "":
        insert_at += 1

    prefix = "\n".join(lines[:insert_at]).rstrip()
    suffix = "\n".join(lines[insert_at:]).lstrip("\n")
    if suffix:
        return f"{prefix}\n\n{block}{suffix}\n"
    return f"{prefix}\n\n{block}"


def sync(repo_root: Path) -> Dict[str, int]:
    aggregate_path = repo_root / "ai_working" / "gap_scan_v3_aggregate.json"
    if not aggregate_path.exists():
        raise FileNotFoundError(f"Missing artifact: {aggregate_path}")

    aggregate = _load_json(aggregate_path)
    generated_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    stats = {
        "modules_seen": 0,
        "files_updated": 0,
        "files_skipped": 0,
    }

    src_dir = repo_root / "src"

    for module, payload in sorted(aggregate.items()):
        module_dir = src_dir / module
        if not module_dir.exists() or not module_dir.is_dir():
            continue
        if not isinstance(payload, dict):
            continue

        stats["modules_seen"] += 1

        for doc_name in TARGET_DOCS:
            target = module_dir / doc_name
            if not target.exists():
                stats["files_skipped"] += 1
                continue

            old = target.read_text(encoding="utf-8", errors="ignore")
            block = _build_block(module, doc_name, payload, generated_at)
            new = _replace_or_insert_block(old, block)

            if new != old:
                target.write_text(new, encoding="utf-8")
                stats["files_updated"] += 1
            else:
                stats["files_skipped"] += 1

    return stats


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Sync module governance docs from v3 gap aggregate")
    parser.add_argument("--repo", default=".", help="Repository root")
    args = parser.parse_args()

    repo_root = Path(args.repo).resolve()
    result = sync(repo_root)
    print("[OK] Governance doc sync completed")
    print(f"Modules processed: {result['modules_seen']}")
    print(f"Files updated: {result['files_updated']}")
    print(f"Files skipped: {result['files_skipped']}")
