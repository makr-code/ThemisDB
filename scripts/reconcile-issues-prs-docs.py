"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reconcile-issues-prs-docs.py                       ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:15:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     626                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import subprocess
from collections import Counter, defaultdict
from datetime import date, datetime, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = "makr-code/ThemisDB"

LEGACY_VERSIONS = [
    "v0.9.0",
    "v1.0.0",
    "v1.0.1",
    "v1.0.2",
    "v1.1.0",
    "v1.1.1",
    "v1.1.2",
    "v1.2.0",
    "v1.2.1",
    "v1.2.2",
    "v1.3.0",
    "v1.3.1",
    "v1.3.2",
    "v1.4.0",
    "v1.4.1",
]

ISSUE_RE = re.compile(r"Issue:\s*#(\d+)")
TARGET_RE = re.compile(r"Target:\s*([^)]+)")
VERSION_RE = re.compile(r"v\d+\.\d+(?:\.\d+)?", re.IGNORECASE)
TITLE_MOD_RE = re.compile(r"^\[([^\]]+)\]")

# Known git-tag release dates (from: git log --format="%aI" -1 <tag>)
KNOWN_TAG_DATES: dict[str, str] = {
    "v1.0.0": "2025-11-30T20:10:04+01:00",
    "v1.0.1": "2025-12-09T21:09:10+01:00",
    "v1.0.2": "2025-12-14T15:42:45+01:00",
    "v1.3.0": "2025-12-21T12:55:06+01:00",
    "v1.3.4": "2025-12-28T16:47:19+01:00",
    "v1.4.0": "2026-01-12T15:22:12+01:00",
    "v1.4.1": "2026-01-29T10:13:44+01:00",  # from v1.4.1-dev-alpha tag
}


def parse_dt(s: str | None) -> datetime | None:
    if not s:
        return None
    try:
        return datetime.fromisoformat(s.replace("Z", "+00:00"))
    except ValueError:
        return None


def build_time_windows() -> list[tuple[str, datetime]]:
    """Return sorted list of (version, release_datetime) for all LEGACY_VERSIONS.

    Known tag dates are used as anchors; missing versions are linearly interpolated
    between their nearest known neighbours.
    """
    known: dict[str, datetime] = {}
    for v, ds in KNOWN_TAG_DATES.items():
        dt = parse_dt(ds)
        if dt and v in LEGACY_VERSIONS:
            known[v] = dt

    # Estimate v0.9.0 as 30 days before v1.0.0
    if "v1.0.0" in known:
        known.setdefault("v0.9.0", known["v1.0.0"] - timedelta(days=30))

    windows: list[tuple[str, datetime]] = []
    for i, v in enumerate(LEGACY_VERSIONS):
        if v in known:
            windows.append((v, known[v]))
        else:
            prev_v = next((vv for vv in reversed(LEGACY_VERSIONS[:i]) if vv in known), None)
            next_v = next((vv for vv in LEGACY_VERSIONS[i + 1:] if vv in known), None)
            if prev_v and next_v:
                prev_i = LEGACY_VERSIONS.index(prev_v)
                next_i = LEGACY_VERSIONS.index(next_v)
                frac = (i - prev_i) / (next_i - prev_i)
                prev_dt = known[prev_v]
                next_dt = known[next_v]
                windows.append((v, prev_dt + (next_dt - prev_dt) * frac))
            elif next_v:
                windows.append((v, known[next_v]))
            elif prev_v:
                windows.append((v, known[prev_v]))

    return sorted(windows, key=lambda x: x[1])


def suggest_by_time(
    dt: datetime, time_windows: list[tuple[str, datetime]]
) -> str | None:
    """Return the LEGACY_VERSIONS entry whose release window contains *dt*.

    Each version "owns" work done after the previous release up to and including
    its own release date.
    """
    if not time_windows:
        return None
    # Assign to the first version whose release_date >= dt
    for version, rel_dt in time_windows:
        if dt <= rel_dt:
            return version
    return time_windows[-1][0]


def run_gh(args: list[str]) -> str:
    proc = subprocess.run(["gh", *args], cwd=ROOT, text=True, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or "gh command failed")
    return proc.stdout


def normalize_version(v: str | None) -> str | None:
    if not v:
        return None
    m = VERSION_RE.search(v)
    if not m:
        return None
    out = m.group(0).lower()
    if out.count(".") == 1:
        return out + ".0"
    return out


def fetch_issues() -> list[dict]:
    out = run_gh([
        "issue",
        "list",
        "--repo",
        REPO,
        "--state",
        "all",
        "--limit",
        "5000",
        "--json",
        "number,title,createdAt,closedAt,state,milestone,labels",
    ])
    return json.loads(out)


def fetch_milestone_totals() -> dict[str, dict[str, int]]:
    """Return GitHub milestone totals (issues + PRs) by title.

    GitHub milestone counters in UI/API include both issues and pull requests.
    """
    out = run_gh([
        "api",
        f"repos/{REPO}/milestones?state=all&per_page=100",
    ])
    rows = json.loads(out)
    totals: dict[str, dict[str, int]] = {}
    for r in rows:
        title = r.get("title")
        if not isinstance(title, str):
            continue
        totals[title] = {
            "open": int(r.get("open_issues") or 0),
            "closed": int(r.get("closed_issues") or 0),
            "total": int((r.get("open_issues") or 0) + (r.get("closed_issues") or 0)),
        }
    return totals


def fetch_pr_data() -> tuple[dict[int, list[int]], dict[int, datetime | None], list[dict]]:
    """Fetch all PRs and return:
    - issue_to_prs: mapping from closed issue# → list of PR#s
    - pr_merged_at: mapping from PR# → mergedAt datetime (or None)
    """
    out = run_gh([
        "pr",
        "list",
        "--repo",
        REPO,
        "--state",
        "all",
        "--limit",
        "5000",
        "--json",
        "number,title,state,mergedAt,milestone,closingIssuesReferences",
    ])
    prs = json.loads(out)
    issue_to_prs: dict[int, list[int]] = defaultdict(list)
    pr_merged_at: dict[int, datetime | None] = {}
    for pr in prs:
        pr_num = int(pr["number"])
        pr_merged_at[pr_num] = parse_dt(pr.get("mergedAt"))
        for ref in pr.get("closingIssuesReferences") or []:
            num = ref.get("number")
            if isinstance(num, int):
                issue_to_prs[num].append(pr_num)
    return dict(issue_to_prs), pr_merged_at, prs


def parse_roadmap_targets() -> dict[int, dict]:
    out: dict[int, dict] = {}
    for roadmap in sorted(ROOT.glob("src/**/ROADMAP.md")):
        module = roadmap.parent.name.lower()
        phase_version = None
        for line in roadmap.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("### "):
                phase_version = normalize_version(line)
            issue_matches = ISSUE_RE.findall(line)
            if not issue_matches:
                continue
            target_match = TARGET_RE.search(line)
            target_raw = target_match.group(1).strip() if target_match else None
            target_version = normalize_version(target_raw) or phase_version
            for issue_num_str in issue_matches:
                issue_num = int(issue_num_str)
                out[issue_num] = {
                    "module": module,
                    "roadmap": str(roadmap.relative_to(ROOT)),
                    "target_raw": target_raw,
                    "target_version": target_version,
                    "line": line.strip(),
                }
    return out


def infer_module(issue: dict, roadmap_targets: dict[int, dict]) -> str | None:
    num = int(issue["number"])
    if num in roadmap_targets:
        return roadmap_targets[num]["module"]
    for label in issue.get("labels") or []:
        name = str(label.get("name", "")).lower()
        if name.startswith("module:"):
            return name.split(":", 1)[1].split("/")[0]
        if (ROOT / "src" / name).exists():
            return name
    m = TITLE_MOD_RE.match(str(issue.get("title", "")))
    if m:
        return m.group(1).lower().split("/")[0]
    return None


def build_legacy_counts(issues: list[dict]) -> dict[str, int]:
    """Count how many already-assigned issues sit in each legacy version bucket."""
    counts: dict[str, int] = {v: 0 for v in LEGACY_VERSIONS}
    for i in issues:
        ms = ((i.get("milestone") or {}) or {}).get("title")
        if ms in LEGACY_VERSIONS:
            counts[ms] += 1
    return counts


def main() -> int:
    print("[1/4] Fetching issues …")
    issues = fetch_issues()
    print("[2/4] Fetching PR data …")
    issue_to_prs, pr_merged_at, prs = fetch_pr_data()
    print("[3/4] Fetching milestone totals + parsing ROADMAP targets …")
    milestone_totals = fetch_milestone_totals()
    roadmap_targets = parse_roadmap_targets()
    print("[4/4] Building time windows …")

    time_windows = build_time_windows()
    legacy_counts = build_legacy_counts(issues)

    # Determine legacy time range boundaries
    legacy_start = time_windows[0][1]   # earliest version release date
    legacy_end   = time_windows[-1][1]  # latest version release date

    dataset = []
    proposals = []
    overload_rebalance_proposals = []
    issue_milestone_by_number: dict[int, str | None] = {}
    for i in issues:
        num = int(i["number"])
        milestone = ((i.get("milestone") or {}) or {}).get("title")
        issue_milestone_by_number[num] = milestone
        roadmap = roadmap_targets.get(num)
        module = infer_module(i, roadmap_targets)
        linked_prs = sorted(issue_to_prs.get(num, []))
        rec = {
            "number": num,
            "title": i.get("title"),
            "state": str(i.get("state", "")).lower(),
            "createdAt": i.get("createdAt"),
            "closedAt": i.get("closedAt"),
            "milestone": milestone,
            "labels": [x.get("name") for x in (i.get("labels") or [])],
            "module": module,
            "linked_prs": linked_prs,
            "roadmap_target_raw": roadmap.get("target_raw") if roadmap else None,
            "roadmap_target_version": roadmap.get("target_version") if roadmap else None,
            "roadmap_source": roadmap.get("roadmap") if roadmap else None,
        }
        dataset.append(rec)

        # Skip issues already in a legacy version
        if milestone in LEGACY_VERSIONS:
            # Dedicated overload rebalancing for v1.0.2: keep only issues that
            # are actually in/near its release window.
            if milestone == "v1.0.2":
                # High signal: linked PR merged date
                best_pr_dt: datetime | None = None
                best_pr_num: int | None = None
                for pr_num in linked_prs:
                    mdt = pr_merged_at.get(pr_num)
                    if mdt is not None:
                        if best_pr_dt is None or mdt < best_pr_dt:
                            best_pr_dt = mdt
                            best_pr_num = pr_num

                suggested = None
                confidence = None
                reason = None

                if best_pr_dt is not None:
                    suggested = suggest_by_time(best_pr_dt, time_windows)
                    confidence = "high"
                    reason = f"v1.0.2 overload: PR #{best_pr_num} mergedAt {best_pr_dt.date()}"
                else:
                    created_dt = parse_dt(i.get("createdAt"))
                    if created_dt is not None:
                        suggested = suggest_by_time(created_dt, time_windows)
                        confidence = "medium"
                        reason = f"v1.0.2 overload: issue createdAt {created_dt.date()}"

                if suggested and suggested != "v1.0.2":
                    overload_rebalance_proposals.append(
                        {
                            "number": num,
                            "title": i.get("title"),
                            "current_milestone": milestone,
                            "suggested_legacy_milestone": suggested,
                            "confidence": confidence,
                            "reason": reason,
                            "module": module,
                            "linked_prs": linked_prs,
                        }
                    )
            continue

        # ── Highest signal: explicit ROADMAP target for a legacy version ──────
        if roadmap and roadmap.get("target_version") in LEGACY_VERSIONS:
            proposals.append({
                "number": num,
                "title": i.get("title"),
                "current_milestone": milestone,
                "suggested_legacy_milestone": roadmap["target_version"],
                "confidence": "high",
                "reason": f"explicit roadmap legacy target ({roadmap['roadmap']})",
                "module": module,
                "linked_prs": linked_prs,
            })
            continue

        # ── High signal: linked PR was merged within the legacy time range ────
        best_pr_dt: datetime | None = None
        best_pr_num: int | None = None
        for pr_num in linked_prs:
            mdt = pr_merged_at.get(pr_num)
            if mdt and legacy_start <= mdt <= legacy_end:
                if best_pr_dt is None or mdt < best_pr_dt:
                    best_pr_dt = mdt
                    best_pr_num = pr_num

        if best_pr_dt is not None:
            suggested = suggest_by_time(best_pr_dt, time_windows)
            if suggested:
                proposals.append({
                    "number": num,
                    "title": i.get("title"),
                    "current_milestone": milestone,
                    "suggested_legacy_milestone": suggested,
                    "confidence": "high",
                    "reason": f"PR #{best_pr_num} mergedAt {best_pr_dt.date()}",
                    "module": module,
                    "linked_prs": linked_prs,
                })
            continue

        # ── Medium signal: issue createdAt within legacy time range ───────────
        created_dt = parse_dt(i.get("createdAt"))
        if created_dt and legacy_start <= created_dt <= legacy_end:
            suggested = suggest_by_time(created_dt, time_windows)
            if suggested:
                proposals.append({
                    "number": num,
                    "title": i.get("title"),
                    "current_milestone": milestone,
                    "suggested_legacy_milestone": suggested,
                    "confidence": "medium",
                    "reason": f"issue createdAt {created_dt.date()}",
                    "module": module,
                    "linked_prs": linked_prs,
                })

    # PR milestone alignment against linked issue milestones.
    pr_alignment_proposals = []
    for pr in prs:
        pr_num = int(pr.get("number"))
        current_pr_milestone = ((pr.get("milestone") or {}) or {}).get("title")
        linked_issues = [
            int(r["number"]) for r in (pr.get("closingIssuesReferences") or [])
            if isinstance(r.get("number"), int)
        ]
        if not linked_issues:
            continue

        linked_issue_milestones = [
            issue_milestone_by_number.get(n) for n in linked_issues
            if issue_milestone_by_number.get(n)
        ]
        if not linked_issue_milestones:
            continue

        counts = Counter(linked_issue_milestones)
        suggested_pr_milestone, top_count = counts.most_common(1)[0]
        coverage = top_count / float(len(linked_issue_milestones))

        if current_pr_milestone == suggested_pr_milestone:
            continue

        if len(counts) == 1:
            confidence = "high"
            reason = "all linked issues share the same milestone"
        elif coverage >= 0.7:
            confidence = "medium"
            reason = f"majority linked-issue milestone coverage={coverage:.2f}"
        else:
            continue

        pr_alignment_proposals.append({
            "pr_number": pr_num,
            "pr_title": pr.get("title"),
            "pr_state": str(pr.get("state") or "").lower(),
            "current_pr_milestone": current_pr_milestone,
            "suggested_pr_milestone": suggested_pr_milestone,
            "confidence": confidence,
            "reason": reason,
            "linked_issues": sorted(linked_issues),
            "linked_issue_milestone_counts": dict(counts),
        })

    today = str(date.today())

    # ── Summary stats ─────────────────────────────────────────────────────────
    dist: dict[str, int] = {v: 0 for v in LEGACY_VERSIONS}
    high_count = 0
    medium_count = 0
    for p in proposals:
        sv = p["suggested_legacy_milestone"]
        if sv in dist:
            dist[sv] += 1
        if p["confidence"] == "high":
            high_count += 1
        else:
            medium_count += 1

    # Build time-window table for output
    tw_table = [{"version": v, "release_date": str(dt.date())} for v, dt in time_windows]

    pr_align_high = sum(1 for p in pr_alignment_proposals if p["confidence"] == "high")
    pr_align_medium = sum(1 for p in pr_alignment_proposals if p["confidence"] == "medium")

    # Issue-only open/closed by legacy milestone (for direct comparison with
    # GitHub milestone UI/API totals that include PRs).
    issue_only_legacy_state: dict[str, dict[str, int]] = {
        v: {"open": 0, "closed": 0, "total": 0} for v in LEGACY_VERSIONS
    }
    for i in issues:
        ms = ((i.get("milestone") or {}) or {}).get("title")
        if ms not in issue_only_legacy_state:
            continue
        state = str(i.get("state") or "").lower()
        if state == "open":
            issue_only_legacy_state[ms]["open"] += 1
        else:
            issue_only_legacy_state[ms]["closed"] += 1
        issue_only_legacy_state[ms]["total"] += 1

    milestone_totals_legacy: dict[str, dict[str, int]] = {
        v: milestone_totals.get(v, {"open": 0, "closed": 0, "total": 0}) for v in LEGACY_VERSIONS
    }

    out_data = {
        "generated": today,
        "repo": REPO,
        "issues_total": len(dataset),
        "time_windows": tw_table,
        "existing_legacy_counts": legacy_counts,
        "issue_only_legacy_state": issue_only_legacy_state,
        "milestone_totals_legacy_including_prs": milestone_totals_legacy,
        "dataset": sorted(dataset, key=lambda x: x["number"]),
        "legacy_rebalance_proposals": sorted(proposals, key=lambda x: x["number"]),
        "legacy_overload_rebalance_proposals": sorted(overload_rebalance_proposals, key=lambda x: x["number"]),
        "pr_milestone_alignment_proposals": sorted(pr_alignment_proposals, key=lambda x: x["pr_number"]),
    }

    out_json = ROOT / f"artifacts/issues-prs-doc-reconcile-{today}.json"
    out_json.write_text(json.dumps(out_data, indent=2), encoding="utf-8")

    summary_lines = [
        f"# Issue/PR/Doku Reconcile ({today})",
        "",
        f"- Issues gesamt: {len(dataset)}",
        f"- Legacy-Rebalance Vorschlaege: {len(proposals)} (high={high_count}, medium={medium_count})",
        f"- Quelle: `{out_json.name}`",
        "",
        "## Zeitfenster-Modell (Git-Tags + Interpolation)",
        "",
        "| Version | Release-Datum (geschaetzt) |",
        "|---------|---------------------------|",
    ]
    for v, dt in time_windows:
        known_marker = " *(Tag)*" if v in KNOWN_TAG_DATES else " *(interpoliert)*"
        summary_lines.append(f"| {v} | {dt.date()}{known_marker} |")

    summary_lines += [
        "",
        "## Vorschlagsverteilung",
        "",
        "| Version | Vorschlaege (neu) | Bereits zugewiesen |",
        "|---------|-------------------|--------------------|",
    ]
    for v in LEGACY_VERSIONS:
        summary_lines.append(f"| {v} | {dist[v]} | {legacy_counts.get(v, 0)} |")

    # Overload section for already assigned v1.0.2 issues
    ov_dist: dict[str, int] = {v: 0 for v in LEGACY_VERSIONS}
    ov_high = 0
    ov_medium = 0
    for p in overload_rebalance_proposals:
        sv = p["suggested_legacy_milestone"]
        if sv in ov_dist:
            ov_dist[sv] += 1
        if p["confidence"] == "high":
            ov_high += 1
        else:
            ov_medium += 1

    summary_lines += [
        "",
        "## v1.0.2 Overload-Rebalance (bereits Legacy-zugewiesen)",
        "",
        f"- Kandidaten gesamt: {len(overload_rebalance_proposals)} (high={ov_high}, medium={ov_medium})",
        "",
        "| Ziel-Version | Kandidaten |",
        "|--------------|-----------|",
    ]
    for v in LEGACY_VERSIONS:
        if ov_dist[v] > 0:
            summary_lines.append(f"| {v} | {ov_dist[v]} |")

    summary_lines += [
        "",
        "## Zaehldifferenz: Issues-only vs GitHub-Milestone (inkl. PRs)",
        "",
        "| Version | Issues open | Issues closed | Issues total | Milestone total (inkl PRs) |",
        "|---------|-------------|---------------|--------------|-----------------------------|",
    ]
    for v in LEGACY_VERSIONS:
        ios = issue_only_legacy_state[v]
        mts = milestone_totals_legacy[v]
        summary_lines.append(
            f"| {v} | {ios['open']} | {ios['closed']} | {ios['total']} | {mts['total']} |"
        )

    summary_lines += [
        "",
        "## PR-Milestone-Alignment gegen verknuepfte Issues",
        "",
        f"- PR-Vorschlaege: {len(pr_alignment_proposals)} (high={pr_align_high}, medium={pr_align_medium})",
    ]

    out_md = ROOT / f"artifacts/issues-prs-doc-reconcile-{today}.md"
    out_md.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")

    print(f"OUT_JSON={out_json.relative_to(ROOT)}")
    print(f"OUT_MD={out_md.relative_to(ROOT)}")
    print(f"ISSUES_TOTAL={len(dataset)}")
    print(f"PROPOSALS={len(proposals)} (high={high_count}, medium={medium_count})")
    print(
        f"OVERLOAD_V102={len(overload_rebalance_proposals)} "
        f"(high={ov_high}, medium={ov_medium})"
    )
    v102_ios = issue_only_legacy_state.get("v1.0.2", {"open": 0, "closed": 0, "total": 0})
    v102_mts = milestone_totals_legacy.get("v1.0.2", {"open": 0, "closed": 0, "total": 0})
    print(
        "V102_COUNTS "
        f"issues(open={v102_ios['open']},closed={v102_ios['closed']},total={v102_ios['total']}) "
        f"milestone_total_including_prs={v102_mts['total']}"
    )
    print(
        f"PR_ALIGNMENT={len(pr_alignment_proposals)} "
        f"(high={pr_align_high}, medium={pr_align_medium})"
    )
    for v in LEGACY_VERSIONS:
        if dist[v] > 0:
            print(f"  {v}: {dist[v]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
