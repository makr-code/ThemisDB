"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cleanup_milestones.py                              ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-15 18:48:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Milestone Cleanup - Remove module prefixes from GitHub milestone titles.

Unwanted pattern : "<module> - <version>"  e.g. "query - Q2 2026"
Desired pattern  : "<version>"             e.g. "Q2 2026"
"""

import re
import subprocess
import json
import sys
from collections import defaultdict

REPO = "makr-code/ThemisDB"

# Matches titles like "query - Q2 2026" or "acceleration - v1.4.0"
MODULE_PREFIX_RE = re.compile(r"^[\w]+(?:\s[\w]+)* - (.+)$")


def run_gh(args: list[str]) -> tuple[int, str, str]:
    """Run a gh CLI command and return (returncode, stdout, stderr)."""
    cmd = ["gh"] + args
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def get_open_milestones() -> list[dict]:
    """Fetch all open milestones from the repository."""
    print("Lade offene Milestones...")
    all_milestones: list[dict] = []
    page = 1
    while True:
        rc, out, err = run_gh([
            "api",
            f"repos/{REPO}/milestones?state=open&per_page=100&page={page}",
        ])
        if rc != 0:
            print(f"  ❌ Fehler beim Laden der Milestones: {err}", file=sys.stderr)
            sys.exit(1)
        batch = json.loads(out)
        if not batch:
            break
        all_milestones.extend(batch)
        if len(batch) < 100:
            break
        page += 1

    print(f"  {len(all_milestones)} offene Milestones gefunden.\n")
    return all_milestones


def get_issues_for_milestone(milestone_number: int) -> list[dict]:
    """Return all open + closed issues assigned to a milestone."""
    issues: list[dict] = []
    for state in ("open", "closed"):
        page = 1
        while True:
            rc, out, err = run_gh([
                "api",
                (
                    f"repos/{REPO}/issues"
                    f"?milestone={milestone_number}&state={state}"
                    f"&per_page=100&page={page}"
                ),
            ])
            if rc != 0:
                break
            batch = json.loads(out)
            if not batch:
                break
            issues.extend(batch)
            if len(batch) < 100:
                break
            page += 1
    return issues


def reassign_issue_milestone(issue_number: int, milestone_number: int) -> bool:
    """Move an issue to a different milestone."""
    rc, _, err = run_gh([
        "api",
        f"repos/{REPO}/issues/{issue_number}",
        "-X", "PATCH",
        "-f", f"milestone={milestone_number}",
    ])
    if rc != 0:
        print(f"    ❌ Issue #{issue_number} konnte nicht verschoben werden: {err}")
    return rc == 0


def rename_milestone(milestone_number: int, new_title: str) -> bool:
    """Rename a milestone."""
    rc, _, err = run_gh([
        "api",
        f"repos/{REPO}/milestones/{milestone_number}",
        "-X", "PATCH",
        "-f", f"title={new_title}",
    ])
    if rc != 0:
        print(f"  ❌ Umbenennen fehlgeschlagen: {err}")
    return rc == 0


def delete_milestone(milestone_number: int, title: str) -> bool:
    """Delete a milestone (it must have no open issues attached)."""
    rc, _, err = run_gh([
        "api",
        f"repos/{REPO}/milestones/{milestone_number}",
        "-X", "DELETE",
    ])
    if rc != 0:
        print(f"  ❌ Löschen von '{title}' fehlgeschlagen: {err}")
    return rc == 0


def create_milestone(title: str) -> dict | None:
    """Create a new open milestone and return the API response object."""
    rc, out, err = run_gh([
        "api",
        f"repos/{REPO}/milestones",
        "-X", "POST",
        "-f", f"title={title}",
        "-f", "state=open",
    ])
    if rc != 0:
        print(f"  ❌ Erstellen von '{title}' fehlgeschlagen: {err}")
        return None
    return json.loads(out)


def main() -> int:
    print("=" * 60)
    print("  ThemisDB Milestone-Bereinigung")
    print("=" * 60)
    print()

    milestones = get_open_milestones()

    # Separate module-prefixed milestones from clean ones
    clean: dict[str, dict] = {}     # version_title -> milestone object
    prefixed: list[dict] = []        # milestones with module prefixes

    for ms in milestones:
        title: str = ms["title"]
        m = MODULE_PREFIX_RE.match(title)
        if m:
            prefixed.append(ms)
        else:
            clean[title] = ms

    if not prefixed:
        print("✅ Keine modulabhängigen Milestones gefunden. Nichts zu tun.")
        return 0

    print(f"🔍 Modulabhängige Milestones gefunden: {len(prefixed)}")
    for ms in prefixed:
        print(f"   • \"{ms['title']}\"  (#{ms['number']})")
    print()

    # Group prefixed milestones by their target version title
    by_version: dict[str, list[dict]] = defaultdict(list)
    for ms in prefixed:
        version_title = MODULE_PREFIX_RE.match(ms["title"]).group(1)
        by_version[version_title].append(ms)

    renamed = 0
    deleted = 0
    moved_issues = 0

    for version_title, group in by_version.items():
        print(f"--- Ziel-Milestone: \"{version_title}\" ---")

        # Determine or create the canonical clean milestone
        if version_title in clean:
            target = clean[version_title]
            print(f"  Bestehender Milestone gefunden: #{target['number']}")
        else:
            if len(group) == 1:
                # Simply rename the single prefixed milestone
                ms = group[0]
                print(
                    f"  Benenne um: \"{ms['title']}\" -> \"{version_title}\""
                )
                if rename_milestone(ms["number"], version_title):
                    print(f"  ✅ Umbenannt.")
                    renamed += 1
                continue  # Nothing more to do for this version
            else:
                # Multiple prefixed milestones – create a fresh canonical one
                print(f"  Erstelle neuen Milestone: \"{version_title}\"")
                new_ms = create_milestone(version_title)
                if new_ms is None:
                    print(f"  ❌ Abbruch für Version \"{version_title}\".")
                    continue
                target = new_ms
                clean[version_title] = target
                print(f"  ✅ Erstellt: #{target['number']}")

        # Move all issues from prefixed milestones to the target
        for ms in group:
            issues = get_issues_for_milestone(ms["number"])
            print(
                f"  Verschiebe {len(issues)} Issues von "
                f"\"{ms['title']}\" (#{ms['number']}) "
                f"-> \"{version_title}\" (#{target['number']})"
            )
            for issue in issues:
                if reassign_issue_milestone(issue["number"], target["number"]):
                    moved_issues += 1

            # Delete the now-empty prefixed milestone
            print(f"  Lösche \"{ms['title']}\" (#{ms['number']})...")
            if delete_milestone(ms["number"], ms["title"]):
                print(f"  ✅ Gelöscht.")
                deleted += 1

        print()

    print("=" * 60)
    print("  Zusammenfassung")
    print("=" * 60)
    print(f"  Umbenannte Milestones : {renamed}")
    print(f"  Gelöschte Milestones  : {deleted}")
    print(f"  Verschobene Issues    : {moved_issues}")
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
