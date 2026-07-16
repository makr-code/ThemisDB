"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_grafana_dashboards.py                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     339                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8452353dc5  2026-03-12  Add unit tests for sync-issues-from-roadmap.py ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
scripts/validate_grafana_dashboards.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Static validation for Grafana dashboard JSON files.

This script is designed to be run in CI (see .github/workflows/validate-grafana-dashboards.yml)
and locally before committing dashboard changes.

It validates:
1. JSON syntax — every *.json under grafana/ must parse.
2. Required dashboard fields — 'title' and 'panels' must be present.
3. Panel uniqueness — panel IDs must be unique within a dashboard.
4. Panel structure — every panel needs 'id', 'type', 'title', 'gridPos'.
5. GridPos completeness — every gridPos needs h, w, x, y.
6. Grid overlap detection — no two panels in the same dashboard overlap.
7. Non-empty queries — every graph/stat panel must have at least one target
   with a non-empty 'expr'.
8. LLM dashboard-specific checks — all expected panels are present and all
   PromQL expressions reference canonical 'llm_*' metric names.

Exit codes:
  0 — all checks passed
  1 — one or more checks failed (error messages printed to stdout)

Usage:
  python3 scripts/validate_grafana_dashboards.py [grafana_root]

  If grafana_root is omitted the script searches relative to its own location
  (i.e. the repository root is inferred).
"""

import json
import os
import re
import sys
from typing import Any, Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Panel types that are expected to have PromQL 'expr' targets
# ---------------------------------------------------------------------------
QUERY_PANEL_TYPES = {
    "graph",
    "timeseries",
    "stat",
    "gauge",
    "bargauge",
    "heatmap",
    "table",
}

# ---------------------------------------------------------------------------
# Expected panels in the LLM monitoring dashboard (panel title substrings)
# ---------------------------------------------------------------------------
LLM_DASHBOARD_FILE = "dashboards/themisdb-llm-dashboard.json"
LLM_REQUIRED_PANEL_TITLES = [
    "Inference Requests",
    "First Token Latency",
    "Throughput",
    "GPU Memory",
    "GPU Utilization",
    "Cache Hit",
    "Queue",
    "Error Rate",
    "Heatmap",
]

# All PromQL expressions in the LLM dashboard must start with one of these
# metric name prefixes (or use histogram_quantile / rate wrapping them).
LLM_METRIC_PREFIXES = ("llm_",)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_dashboard(fpath: str) -> Tuple[Optional[Dict[str, Any]], Optional[str]]:
    """Load and normalise a dashboard JSON file.

    Returns (dashboard_dict, None) on success, (None, error_message) on failure.
    The 'dashboard' top-level wrapper (Grafana provisioning format) is unwrapped
    transparently.
    """
    try:
        with open(fpath, encoding="utf-8") as fh:
            raw = json.load(fh)
    except json.JSONDecodeError as exc:
        return None, f"JSON parse error: {exc}"
    except OSError as exc:
        return None, f"File read error: {exc}"

    # Grafana export/provisioning wraps the payload in a 'dashboard' key.
    return raw.get("dashboard", raw), None


def _rect_overlaps(a: Dict, b: Dict) -> bool:
    """Return True if gridPos rectangles a and b overlap."""
    ax1, ay1 = a["x"], a["y"]
    ax2, ay2 = ax1 + a["w"], ay1 + a["h"]
    bx1, by1 = b["x"], b["y"]
    bx2, by2 = bx1 + b["w"], by1 + b["h"]
    return ax1 < bx2 and ax2 > bx1 and ay1 < by2 and ay2 > by1


# ---------------------------------------------------------------------------
# Validation functions — each returns a list of error strings (empty = pass)
# ---------------------------------------------------------------------------

def check_required_fields(d: Dict, relpath: str) -> List[str]:
    errors = []
    for field in ("title", "panels"):
        if field not in d:
            errors.append(f"{relpath}: missing required dashboard field '{field}'")
    return errors


def check_panel_ids(d: Dict, relpath: str) -> List[str]:
    errors = []
    panels = d.get("panels", [])
    ids = [p.get("id") for p in panels if "id" in p]
    seen: Dict[Any, int] = {}
    for pid in ids:
        seen[pid] = seen.get(pid, 0) + 1
    for pid, count in seen.items():
        if count > 1:
            errors.append(f"{relpath}: duplicate panel id {pid} (appears {count} times)")
    # Check panels that have no id at all
    missing_id = [i for i, p in enumerate(panels) if "id" not in p]
    for idx in missing_id:
        errors.append(f"{relpath}: panel at index {idx} is missing 'id'")
    return errors


def check_panel_structure(d: Dict, relpath: str) -> List[str]:
    errors = []
    for panel in d.get("panels", []):
        pid = panel.get("id", "?")
        for field in ("type", "title", "gridPos"):
            if field not in panel:
                errors.append(f"{relpath}: panel {pid} missing '{field}'")
        gp = panel.get("gridPos", {})
        for dim in ("h", "w", "x", "y"):
            if dim not in gp:
                errors.append(f"{relpath}: panel {pid} gridPos missing '{dim}'")
    return errors


def check_grid_overlaps(d: Dict, relpath: str) -> List[str]:
    """Detect panels that physically overlap on the dashboard grid."""
    errors = []
    panels = d.get("panels", [])
    valid = [p for p in panels if all(k in p.get("gridPos", {}) for k in ("h", "w", "x", "y"))]
    for i, pa in enumerate(valid):
        for pb in valid[i + 1 :]:
            if _rect_overlaps(pa["gridPos"], pb["gridPos"]):
                errors.append(
                    f"{relpath}: panels '{pa.get('title', pa.get('id'))}' and "
                    f"'{pb.get('title', pb.get('id'))}' overlap on the grid"
                )
    return errors


def check_query_targets(d: Dict, relpath: str) -> List[str]:
    """Ensure query panels have at least one non-empty 'expr'."""
    errors = []
    for panel in d.get("panels", []):
        ptype = panel.get("type", "")
        if ptype not in QUERY_PANEL_TYPES:
            continue
        pid = panel.get("id", "?")
        ptitle = panel.get("title", "")
        targets = panel.get("targets", [])
        if not targets:
            errors.append(f"{relpath}: panel {pid} ('{ptitle}') has no targets")
            continue
        has_expr = any(bool(t.get("expr", "").strip()) for t in targets)
        if not has_expr:
            errors.append(
                f"{relpath}: panel {pid} ('{ptitle}') has targets but all 'expr' are empty"
            )
    return errors


def check_llm_dashboard(d: Dict, relpath: str) -> List[str]:
    """LLM-specific checks: required panels and metric naming conventions."""
    errors = []
    panels = d.get("panels", [])
    panel_titles = " ".join(p.get("title", "") for p in panels)

    # 1. Required panels
    for required in LLM_REQUIRED_PANEL_TITLES:
        if required.lower() not in panel_titles.lower():
            errors.append(
                f"{relpath}: LLM dashboard missing expected panel containing '{required}'"
            )

    # 2. All PromQL expressions should reference llm_* metrics
    for panel in panels:
        pid = panel.get("id", "?")
        ptitle = panel.get("title", "")
        for target in panel.get("targets", []):
            expr = target.get("expr", "").strip()
            if not expr:
                continue
            # Extract bare metric names (sequences of word chars that look
            # like metric names: all lowercase, may contain _ but no /).
            metric_names = re.findall(r"\b([a-z][a-z0-9_]{4,})\b", expr)
            # Filter out PromQL keywords and functions
            pql_keywords = {
                "rate", "sum", "by", "le", "model_id", "without",
                "histogram_quantile", "increase", "irate", "avg", "max",
                "min", "count", "label_replace", "on", "or", "and",
                "unless", "offset", "bool", "ignoring", "group_left",
                "group_right", "topk", "bottomk", "quantile",
            }
            # Minimum identifier length to consider as a metric name.
            # Shorter identifiers (e.g. "le", "by") are PromQL keywords or
            # label names, not metric names.  5 chars avoids false positives
            # on common 2-4-char PromQL tokens while still catching all llm_*
            # metrics (shortest: "llm_x" = 5 chars).
            MIN_METRIC_NAME_LENGTH = 5
            bare_metrics = [
                m for m in metric_names
                if m not in pql_keywords
                and not m.isdigit()
                and len(m) >= MIN_METRIC_NAME_LENGTH
            ]
            for metric in bare_metrics:
                if not any(metric.startswith(pfx) for pfx in LLM_METRIC_PREFIXES):
                    errors.append(
                        f"{relpath}: panel {pid} ('{ptitle}') references metric "
                        f"'{metric}' which does not start with 'llm_' — "
                        f"ensure it is an LLM metric or update LLM_METRIC_PREFIXES"
                    )
    return errors


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def find_dashboard_files(grafana_root: str) -> List[str]:
    found = []
    for root, _dirs, files in os.walk(grafana_root):
        for fname in files:
            if fname.endswith(".json"):
                found.append(os.path.join(root, fname))
    return sorted(found)


def validate_all(grafana_root: str) -> int:
    """Validate all dashboards under grafana_root.  Returns exit code."""
    files = find_dashboard_files(grafana_root)
    if not files:
        print(f"WARNING: no .json files found under {grafana_root}")
        return 0

    all_errors: List[str] = []
    skipped_files: List[str] = []

    for fpath in files:
        relpath = os.path.relpath(fpath, grafana_root)

        dashboard, load_error = load_dashboard(fpath)
        if load_error:
            all_errors.append(f"{relpath}: {load_error}")
            continue

        # Skip Grafana alerting provisioning files (apiVersion/groups) since
        # they are not dashboard definitions and do not contain panel layouts.
        if (
            isinstance(dashboard, dict)
            and "groups" in dashboard
            and "panels" not in dashboard
            and "title" not in dashboard
        ):
            skipped_files.append(relpath)
            continue

        # Generic checks (every dashboard)
        all_errors += check_required_fields(dashboard, relpath)
        all_errors += check_panel_ids(dashboard, relpath)
        all_errors += check_panel_structure(dashboard, relpath)
        all_errors += check_grid_overlaps(dashboard, relpath)
        all_errors += check_query_targets(dashboard, relpath)

        # LLM-specific checks
        if relpath == LLM_DASHBOARD_FILE:
            all_errors += check_llm_dashboard(dashboard, relpath)

    if all_errors:
        print(f"FAILED — {len(all_errors)} issue(s) found:\n")
        for err in all_errors:
            print(f"  ✗  {err}")
        print()
        return 1

    if skipped_files:
        print(f"INFO: skipped {len(skipped_files)} non-dashboard provisioning file(s):")
        for relpath in skipped_files:
            print(f"  - {relpath}")
        print()

    print(f"PASSED — all {len(files)} Grafana dashboard files are valid.")
    return 0


if __name__ == "__main__":
    if len(sys.argv) > 1:
        root = sys.argv[1]
    else:
        # Default: grafana/ directory relative to repo root (script is in scripts/)
        script_dir = os.path.dirname(os.path.abspath(__file__))
        root = os.path.join(os.path.dirname(script_dir), "grafana")

    sys.exit(validate_all(root))
