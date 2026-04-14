"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            acceleration_roadmap_audit.py                      ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:31:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     739                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b0dd675ac0  2026-03-09  fix: address code review comments (configurable repo, nam... ║
    • 45aaeb9216  2026-03-09  feat(audit): add acceleration ROADMAP audit tool, reports... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Acceleration Module ROADMAP Audit Tool
=======================================
Parses `src/acceleration/ROADMAP.md`, extracts all referenced GitHub Issue
numbers, queries the GitHub REST API for each issue's status and linked
pull-requests / timeline events, cross-checks whether the expected source
files exist on disk, and writes two reports:

  docs/audits/acceleration-roadmap-audit.json   (machine-readable)
  docs/audits/acceleration-roadmap-audit.md     (human-readable)

Usage
-----
  # Read-only dry-run (no token needed for public repos, but rate-limited)
  python3 scripts/acceleration_roadmap_audit.py

  # Authenticated run (recommended: avoids GitHub rate limits)
  GITHUB_TOKEN=ghp_xxx python3 scripts/acceleration_roadmap_audit.py

  # Or via gh CLI credentials
  python3 scripts/acceleration_roadmap_audit.py --gh-cli

  # Write reports to a different directory
  python3 scripts/acceleration_roadmap_audit.py --output-dir /tmp/audit

Requirements
------------
  Python >= 3.9; only stdlib is used (urllib, json, re, pathlib, os, datetime).

Token / Scopes
--------------
  Set the environment variable GITHUB_TOKEN (or GH_TOKEN) to a personal
  access token with at least the `public_repo` scope.  Without a token the
  tool falls back to unauthenticated requests which are rate-limited to
  60 req/hour per IP.

  Alternatively pass --gh-cli to pull the token from `gh auth token`.

Exit codes
----------
  0  Clean audit – no ROADMAP/GitHub status discrepancies found.
  1  Discrepancies detected (see report for details).
  2  Fatal error (e.g. ROADMAP file not found, API failure).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

REPO_OWNER = "makr-code"
REPO_NAME = "ThemisDB"
REPO_ROOT = Path(__file__).parent.parent
ROADMAP_PATH = REPO_ROOT / "src" / "acceleration" / "ROADMAP.md"
DEFAULT_OUTPUT_DIR = REPO_ROOT / "docs" / "audits"

GITHUB_API_BASE = "https://api.github.com"

# Polite delay between GitHub API calls to avoid triggering secondary rate limits.
API_RATE_LIMIT_DELAY_SECONDS = 0.25

# Source paths to check for evidence of implementation (relative to REPO_ROOT)
EVIDENCE_PATHS: list[str] = [
    "src/acceleration",
    "include/acceleration",
    "tests",
    "benchmarks",
    "docs/acceleration",
    ".github/workflows",
]

# Mapping of issue numbers to canonical evidence files mentioned in ROADMAP
ISSUE_EVIDENCE_FILES: dict[int, list[str]] = {
    1366: ["src/acceleration/cuda/vector_kernels.cu", "src/acceleration/cuda/ann_kernels.cu"],
    1367: ["src/acceleration/vulkan/shaders", "src/acceleration/graphics_backends.cpp"],
    1368: ["src/acceleration/geo_acceleration_bridge.cpp", "include/acceleration/geo_acceleration_bridge.h"],
    1369: ["src/acceleration/cuda/ann_kernels.cu"],
    1370: ["src/acceleration/hip/ann_kernels.hip", "src/acceleration/hip/geo_kernels.hip"],
    1372: ["src/acceleration/cuda/geo_kernels.cu", "tests/test_geo_gpu_backend.cpp"],
    1373: ["src/acceleration/vulkan/shaders", "src/acceleration/graphics_backends.cpp"],
    1374: ["src/acceleration/device_manager.cpp", "include/acceleration/device_manager.h"],
    1375: ["benchmarks/bench_cuda_vs_cpu.cpp", "benchmarks/baselines/acceleration/baseline.json"],
    1376: ["src/acceleration/multi_gpu_backend.cpp", "tests/test_multi_gpu_backend.cpp"],
    1377: ["src/acceleration/cuda/tensor_core_matmul.cu", "src/acceleration/tensor_core_matmul.cpp"],
    1378: ["src/acceleration/cuda_backend.cpp", "tests/test_cuda_graph_capture.cpp"],
    1379: ["src/acceleration/opencl_backend.cpp"],
    1380: ["include/acceleration/compute_backend.h"],
    1381: ["include/acceleration/kernel_invocation.h"],
    1382: ["include/acceleration/error_codes.h"],
    1383: ["src/acceleration/cuda/ann_kernels.cu", "src/acceleration/cuda/geo_kernels.cu"],
    1384: ["src/acceleration/vulkan/shaders", "src/acceleration/graphics_backends.cpp"],
    1385: ["src/acceleration/backend_registry.cpp"],
    1386: ["include/acceleration/batch_validator.h"],
    1387: ["include/acceleration/kernel_fallback_dispatcher.h"],
    1388: ["include/acceleration/batch_validator.h"],
    1389: ["tests/test_backend_selection_matrix.cpp"],
    1390: ["tests/test_cuda_ann_search.cpp"],
    1391: ["tests/test_cuda_ann_search.cpp"],
    1392: ["benchmarks/bench_cuda_vs_cpu.cpp"],
    1393: [".github/workflows/acceleration-benchmark-ci.yml"],
    1394: ["src/acceleration/plugin_security.cpp", "tests/test_plugin_security_audit.cpp"],
    1395: ["docs/acceleration/capability_negotiation.md"],
    1396: ["docs/acceleration/capability_negotiation.md"],
    1397: [],
    1398: [],
    1399: ["tests/test_cpu_gpu_parity.cpp"],
    1400: [".github/workflows/acceleration-benchmark-ci.yml", "benchmarks/baselines/acceleration/baseline.json"],
    1401: ["src/acceleration/plugin_security.cpp", "tests/test_plugin_security_audit.cpp"],
    1402: ["docs/acceleration/capability_negotiation.md"],
    1403: ["include/acceleration/compute_backend.h", "tests/test_backend_api_stability.cpp"],
}

# ---------------------------------------------------------------------------
# Status legend from ROADMAP header
# ---------------------------------------------------------------------------
STATUS_LEGEND = {
    "[ ]": "open",
    "[~]": "in_progress",
    "[x]": "done",
    "[I]": "open_issue",
    "[P]": "open_pr",
    "[?]": "blocked",
    "[!]": "unclear",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _github_token() -> str | None:
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    return token


def _gh_cli_token() -> str | None:
    try:
        result = subprocess.run(
            ["gh", "auth", "token"],
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def _api_request(
    path: str,
    token: str | None,
    *,
    accept: str = "application/vnd.github+json",
) -> Any:
    """Perform a GET request against the GitHub REST API."""
    url = f"{GITHUB_API_BASE}{path}"
    req = urllib.request.Request(url)
    req.add_header("Accept", accept)
    req.add_header("X-GitHub-Api-Version", "2022-11-28")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            return json.loads(resp.read())
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return None
        if exc.code == 403:
            raise RuntimeError(
                f"GitHub API rate-limited or forbidden for {url}. "
                "Set GITHUB_TOKEN to avoid rate limits."
            ) from exc
        raise


def _fetch_issue(issue_number: int, token: str | None) -> dict | None:
    """Return raw GitHub issue object or None if not found."""
    return _api_request(
        f"/repos/{REPO_OWNER}/{REPO_NAME}/issues/{issue_number}",
        token,
    )


def _fetch_issue_timeline(issue_number: int, token: str | None) -> list:
    """Return timeline events for an issue (first page, up to 100)."""
    data = _api_request(
        f"/repos/{REPO_OWNER}/{REPO_NAME}/issues/{issue_number}/timeline"
        "?per_page=100",
        token,
        accept="application/vnd.github.mockingbird-preview+json",
    )
    return data if isinstance(data, list) else []


def _extract_linked_prs(timeline: list) -> list[dict]:
    """Pull connected PR references from timeline events."""
    prs: list[dict] = []
    for event in timeline:
        etype = event.get("event", "")
        if etype in ("cross-referenced", "referenced"):
            src = event.get("source") or {}
            issue_src = src.get("issue") or {}
            if issue_src.get("pull_request"):
                prs.append(
                    {
                        "number": issue_src.get("number"),
                        "title": issue_src.get("title"),
                        "url": issue_src.get("html_url"),
                        "state": issue_src.get("state"),
                        "merged": issue_src.get("pull_request", {}).get("merged_at") is not None,
                    }
                )
    return prs


def _check_evidence_files(issue_number: int) -> dict:
    """Check whether expected evidence files exist on disk."""
    files = ISSUE_EVIDENCE_FILES.get(issue_number, [])
    results: dict[str, bool] = {}
    for rel_path in files:
        abs_path = REPO_ROOT / rel_path
        results[rel_path] = abs_path.exists()
    all_present = bool(results) and all(results.values())
    any_present = any(results.values())
    return {
        "expected_files": files,
        "file_results": results,
        "all_files_present": all_present,
        "any_file_present": any_present,
    }


# ---------------------------------------------------------------------------
# ROADMAP parser
# ---------------------------------------------------------------------------

_ISSUE_REF_RE = re.compile(r"\(Issue:\s*#(\d+)\)")
_CHECKBOX_RE = re.compile(r"^\s*-\s+(\[[^\]]*\])\s+(.+)$")


def parse_roadmap(path: Path) -> list[dict]:
    """
    Parse ROADMAP.md and return a list of items, each with:
      - raw_line: original line text
      - status_marker: e.g. '[x]', '[P]', '[I]'
      - status_label: human-readable status
      - issue_numbers: list of referenced issue numbers
      - description: item text (stripped)
      - section: nearest parent heading
    """
    items: list[dict] = []
    current_section = "Uncategorized"
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            stripped = line.rstrip()
            # Track headings for section context
            if stripped.startswith("#"):
                current_section = stripped.lstrip("#").strip()
                continue
            m = _CHECKBOX_RE.match(stripped)
            if not m:
                continue
            marker = m.group(1)  # e.g. "[x]"
            description = m.group(2)
            issue_numbers = [int(n) for n in _ISSUE_REF_RE.findall(description)]
            items.append(
                {
                    "raw_line": stripped,
                    "status_marker": marker,
                    "status_label": STATUS_LEGEND.get(marker, "unknown"),
                    "issue_numbers": issue_numbers,
                    "description": description,
                    "section": current_section,
                }
            )
    return items


# ---------------------------------------------------------------------------
# Audit logic
# ---------------------------------------------------------------------------

def audit_item(item: dict, token: str | None) -> dict:
    """
    Given a parsed ROADMAP item, audit each referenced issue and return a
    richer dict with findings.
    """
    issue_audits: list[dict] = []
    for inum in item["issue_numbers"]:
        issue_data = _fetch_issue(inum, token)
        if issue_data is None:
            issue_audits.append(
                {
                    "number": inum,
                    "github_state": "not_found",
                    "state_reason": None,
                    "closed_at": None,
                    "closed_by": None,
                    "linked_prs": [],
                    "evidence": _check_evidence_files(inum),
                    "discrepancies": [
                        f"Issue #{inum} does not exist on GitHub but is referenced in ROADMAP"
                    ],
                }
            )
            continue

        timeline = _fetch_issue_timeline(inum, token)
        linked_prs = _extract_linked_prs(timeline)
        evidence = _check_evidence_files(inum)

        github_state = issue_data.get("state", "unknown")  # open / closed
        state_reason = issue_data.get("state_reason")  # completed / not_planned / None
        closed_at = issue_data.get("closed_at")
        closed_by = (issue_data.get("closed_by") or {}).get("login")

        # Determine discrepancies
        discrepancies: list[str] = []
        roadmap_marker = item["status_marker"]

        if github_state == "closed" and roadmap_marker in ("[P]", "[I]", "[ ]"):
            discrepancies.append(
                f"Issue #{inum} is CLOSED (reason: {state_reason}) on GitHub "
                f"but ROADMAP still marks it as '{roadmap_marker}' "
                f"({STATUS_LEGEND.get(roadmap_marker, '?')}). "
                "Consider updating to [x] (if files exist) or [~]."
            )

        if github_state == "open" and roadmap_marker == "[x]":
            discrepancies.append(
                f"Issue #{inum} is still OPEN on GitHub "
                "but ROADMAP marks it as '[x]' (done)."
            )

        merged_prs = [pr for pr in linked_prs if pr.get("merged")]
        if roadmap_marker == "[x]" and not merged_prs and evidence["expected_files"]:
            if not evidence["any_file_present"]:
                discrepancies.append(
                    f"Issue #{inum} is marked [x] (done) but no expected files "
                    f"found and no merged PR linked. Files checked: "
                    + ", ".join(evidence["expected_files"])
                )
            elif not evidence["all_files_present"]:
                missing = [
                    f for f, ok in evidence["file_results"].items() if not ok
                ]
                discrepancies.append(
                    f"Issue #{inum} is marked [x] (done) but some expected files "
                    f"are missing: {', '.join(missing)}"
                )

        issue_audits.append(
            {
                "number": inum,
                "title": issue_data.get("title"),
                "html_url": issue_data.get("html_url"),
                "github_state": github_state,
                "state_reason": state_reason,
                "closed_at": closed_at,
                "closed_by": closed_by,
                "linked_prs": linked_prs,
                "merged_prs": merged_prs,
                "evidence": evidence,
                "discrepancies": discrepancies,
            }
        )
        # Small delay to be polite to the API
        time.sleep(API_RATE_LIMIT_DELAY_SECONDS)

    has_discrepancy = any(d for ia in issue_audits for d in ia["discrepancies"])
    return {
        "section": item["section"],
        "status_marker": item["status_marker"],
        "status_label": item["status_label"],
        "description": item["description"],
        "issue_audits": issue_audits,
        "has_discrepancy": has_discrepancy,
    }


def run_audit(token: str | None) -> dict:
    """Full audit run. Returns structured result dict."""
    items = parse_roadmap(ROADMAP_PATH)
    results: list[dict] = []
    total_issues = sum(len(it["issue_numbers"]) for it in items)
    print(
        f"Auditing {len(items)} ROADMAP items referencing "
        f"{total_issues} issues …",
        file=sys.stderr,
    )
    for idx, item in enumerate(items, 1):
        if not item["issue_numbers"]:
            continue  # Skip items without issue references
        print(
            f"  [{idx}/{len(items)}] {item['status_marker']} #{item['issue_numbers']} "
            f"— {item['description'][:60]}…",
            file=sys.stderr,
        )
        result = audit_item(item, token)
        results.append(result)

    total_discrepancies = sum(1 for r in results if r["has_discrepancy"])
    total_issues_audited = sum(len(r["issue_audits"]) for r in results)
    not_found = sum(
        1
        for r in results
        for ia in r["issue_audits"]
        if ia["github_state"] == "not_found"
    )
    closed_issues = sum(
        1
        for r in results
        for ia in r["issue_audits"]
        if ia["github_state"] == "closed"
    )
    open_issues = sum(
        1
        for r in results
        for ia in r["issue_audits"]
        if ia["github_state"] == "open"
    )

    return {
        "meta": {
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "roadmap_path": str(ROADMAP_PATH.relative_to(REPO_ROOT)),
            "repo": f"{REPO_OWNER}/{REPO_NAME}",
            "total_roadmap_items_with_issues": len(results),
            "total_issues_audited": total_issues_audited,
            "closed_issues": closed_issues,
            "open_issues": open_issues,
            "not_found_issues": not_found,
            "items_with_discrepancies": total_discrepancies,
        },
        "items": results,
    }


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

_STATUS_EMOJI = {
    "open": "🔴",
    "closed": "✅",
    "not_found": "❓",
}

_ROADMAP_MARKER_LABEL = {
    "[x]": "✅ done",
    "[P]": "🔄 open PR",
    "[I]": "🎫 open issue",
    "[~]": "🚧 in progress",
    "[ ]": "⬜ open",
    "[?]": "❓ blocked",
    "[!]": "⚠️ unclear",
}


def _pr_line(pr: dict) -> str:
    state = "merged" if pr.get("merged") else pr.get("state", "?")
    return f"[#{pr['number']} {pr.get('title', '')}]({pr.get('url', '')}) ({state})"


def generate_markdown_report(audit: dict) -> str:
    meta = audit["meta"]
    items = audit["items"]
    lines: list[str] = []

    lines += [
        "# Acceleration ROADMAP Audit Report",
        "",
        f"> Generated: {meta['generated_at']}  ",
        f"> Repo: `{meta['repo']}`  ",
        f"> Source: `{meta['roadmap_path']}`",
        "",
        "## Summary",
        "",
        f"| Metric | Value |",
        f"|---|---|",
        f"| ROADMAP items audited | {meta['total_roadmap_items_with_issues']} |",
        f"| Issues audited | {meta['total_issues_audited']} |",
        f"| Issues closed on GitHub | {meta['closed_issues']} |",
        f"| Issues open on GitHub | {meta['open_issues']} |",
        f"| Issues not found | {meta['not_found_issues']} |",
        f"| Items with discrepancies | **{meta['items_with_discrepancies']}** |",
        "",
    ]

    # Quick-wins table
    discrepant_items = [r for r in items if r["has_discrepancy"]]
    if discrepant_items:
        lines += [
            "## ⚠️ Discrepancies Found",
            "",
            "The following ROADMAP entries have a mismatch between the "
            "ROADMAP checkbox status and the GitHub issue state or file evidence.",
            "",
        ]
        for r in discrepant_items:
            for ia in r["issue_audits"]:
                for disc in ia["discrepancies"]:
                    marker_label = _ROADMAP_MARKER_LABEL.get(r["status_marker"], r["status_marker"])
                    lines.append(
                        f"- **#{ia['number']}** "
                        f"(ROADMAP: {marker_label}): {disc}"
                    )
        lines.append("")
    else:
        lines += ["## ✅ No Discrepancies Found", ""]

    lines += [
        "## Detailed Results",
        "",
    ]

    current_section = None
    for r in items:
        if r["section"] != current_section:
            current_section = r["section"]
            lines += [f"### {current_section}", ""]

        marker_label = _ROADMAP_MARKER_LABEL.get(r["status_marker"], r["status_marker"])
        disc_tag = " ⚠️" if r["has_discrepancy"] else ""
        lines += [
            f"#### {marker_label}{disc_tag} — {r['description'][:100]}",
            "",
        ]
        for ia in r["issue_audits"]:
            gh_emoji = _STATUS_EMOJI.get(ia["github_state"], "❓")
            lines += [
                f"- **Issue #{ia['number']}** "
                f"[{ia.get('title', '—')}]({ia.get('html_url', '')})",
                f"  - GitHub state: {gh_emoji} `{ia['github_state']}`"
                + (f" (reason: `{ia['state_reason']}`)" if ia.get("state_reason") else ""),
                f"  - Closed at: {ia.get('closed_at') or '—'}"
                + (f", by: `{ia['closed_by']}`" if ia.get("closed_by") else ""),
            ]
            if ia["linked_prs"]:
                lines.append("  - Linked PRs:")
                for pr in ia["linked_prs"]:
                    lines.append(f"    - {_pr_line(pr)}")
            else:
                lines.append("  - Linked PRs: *none found in timeline*")

            ev = ia["evidence"]
            if ev["expected_files"]:
                lines.append("  - Evidence files:")
                for f, ok in ev["file_results"].items():
                    icon = "✅" if ok else "❌"
                    lines.append(f"    - {icon} `{f}`")
            if ia["discrepancies"]:
                lines.append("  - **⚠️ Discrepancies:**")
                for d in ia["discrepancies"]:
                    lines.append(f"    - {d}")
            lines.append("")

    lines += [
        "---",
        "",
        "## Policy Reminder",
        "",
        "Per the ThemisDB ROADMAP policy:",
        "",
        "- **`[x]`** — Only set when a **merged PR or commit** providing the "
        "implementation exists. Files must be present in the repository.",
        "- **`[~]`** — Work is actively in progress (open PR / ongoing commit activity).",
        "- **`[P]`** — A PR exists but is not yet merged.",
        "- **`[I]`** — A GitHub Issue is open and work has not started.",
        "- **`[ ]`** — Planned but no issue yet.",
        "- **`[?]`** — Blocked; human input needed.",
        "",
        "Issues closed on GitHub with `state_reason: completed` **do not** "
        "automatically satisfy the `[x]` criterion unless supporting "
        "implementation files and/or a merged PR can be found.",
        "",
        "## How to Re-run",
        "",
        "```bash",
        "# Unauthenticated (60 req/h rate limit)",
        "python3 scripts/acceleration_roadmap_audit.py",
        "",
        "# Authenticated (5000 req/h)",
        "GITHUB_TOKEN=ghp_xxx python3 scripts/acceleration_roadmap_audit.py",
        "",
        "# Using gh CLI credentials",
        "python3 scripts/acceleration_roadmap_audit.py --gh-cli",
        "",
        "# Custom output directory",
        "python3 scripts/acceleration_roadmap_audit.py --output-dir /tmp/audit",
        "```",
    ]

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    global REPO_OWNER, REPO_NAME  # noqa: PLW0603
    parser = argparse.ArgumentParser(
        description="Audit src/acceleration/ROADMAP.md against GitHub issue status and source files."
    )
    parser.add_argument(
        "--output-dir",
        default=str(DEFAULT_OUTPUT_DIR),
        help="Directory to write audit reports (default: docs/audits/)",
    )
    parser.add_argument(
        "--repo-owner",
        default=os.environ.get("GITHUB_REPOSITORY_OWNER", REPO_OWNER),
        help="GitHub repository owner (default: makr-code, or GITHUB_REPOSITORY_OWNER env var)",
    )
    parser.add_argument(
        "--repo-name",
        default=os.environ.get("GITHUB_REPOSITORY_NAME", REPO_NAME),
        help="GitHub repository name (default: ThemisDB, or GITHUB_REPOSITORY_NAME env var)",
    )
    parser.add_argument(
        "--gh-cli",
        action="store_true",
        help="Pull GitHub token from `gh auth token` instead of env var.",
    )
    parser.add_argument(
        "--json-only",
        action="store_true",
        help="Write only the JSON report (skip Markdown).",
    )
    parser.add_argument(
        "--md-only",
        action="store_true",
        help="Write only the Markdown report (skip JSON).",
    )
    args = parser.parse_args(argv)

    # Apply repo overrides (affects module-level globals used by audit functions)
    REPO_OWNER = args.repo_owner  # type: ignore[assignment]
    REPO_NAME = args.repo_name  # type: ignore[assignment]

    # Resolve token
    token: str | None = None
    if args.gh_cli:
        token = _gh_cli_token()
        if not token:
            print("Warning: gh CLI not available or not authenticated; running unauthenticated.", file=sys.stderr)
    else:
        token = _github_token()
    if token:
        print("GitHub token found – using authenticated requests.", file=sys.stderr)
    else:
        print(
            "No GitHub token found – using unauthenticated requests "
            "(rate-limited to 60 req/h). Set GITHUB_TOKEN to avoid limits.",
            file=sys.stderr,
        )

    if not ROADMAP_PATH.exists():
        print(f"ERROR: ROADMAP not found at {ROADMAP_PATH}", file=sys.stderr)
        return 2

    try:
        audit = run_audit(token)
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    json_path = output_dir / "acceleration-roadmap-audit.json"
    md_path = output_dir / "acceleration-roadmap-audit.md"

    if not args.md_only:
        json_path.write_text(json.dumps(audit, indent=2), encoding="utf-8")
        print(f"JSON report: {json_path}", file=sys.stderr)

    if not args.json_only:
        md_path.write_text(generate_markdown_report(audit), encoding="utf-8")
        print(f"Markdown report: {md_path}", file=sys.stderr)

    discrepancies = audit["meta"]["items_with_discrepancies"]
    not_found = audit["meta"]["not_found_issues"]
    total = audit["meta"]["total_issues_audited"]
    closed = audit["meta"]["closed_issues"]
    print(
        f"\nAudit complete: {total} issues checked, "
        f"{closed} closed, "
        f"{not_found} not found, "
        f"{discrepancies} items with discrepancies.",
        file=sys.stderr,
    )
    return 1 if discrepancies else 0


if __name__ == "__main__":
    sys.exit(main())
