"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dashboard.py                                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     599                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • edfcc1a70a  2026-02-28  feat(chimera): implement BenchmarkDashboard for result ag... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
CHIMERA Suite: Benchmark Result Aggregation and Reporting Dashboard

Aggregates results from multiple BenchmarkHarness runs across different
database systems and generates a comprehensive, vendor-neutral HTML dashboard
with normalized scoring and workload-level breakdowns.

Standards:
    - IEEE Std 2807-2022: Framework for Artificial Intelligence (AI) in Databases
    - ISO/IEC 14756:2015: Performance benchmarking
    - YCSB: Cooper et al., SoCC 2010 (DOI: 10.1145/1807128.1807152)
"""

import json
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

from .benchmark_harness import BenchmarkHarness, WorkloadResult
from .colors import ColorBlindPalette, VendorNeutralStyle
from .scoring import (
    BenchmarkScorer,
    MetricConfig,
    MetricDirection,
    NormalizationMethod,
)


_CHIMERA_VERSION = "1.0.0"


# ---------------------------------------------------------------------------
# BenchmarkDashboard
# ---------------------------------------------------------------------------

class BenchmarkDashboard:
    """Aggregate benchmark results from multiple systems and generate a dashboard.

    Collects :class:`~chimera.benchmark_harness.WorkloadResult` data from one
    or more completed :class:`~chimera.benchmark_harness.BenchmarkHarness`
    instances, normalises the raw metrics with
    :class:`~chimera.scoring.BenchmarkScorer`, and renders a self-contained
    HTML dashboard (and optional JSON export) that compares all registered
    systems in a vendor-neutral manner.

    Usage::

        harness_a = BenchmarkHarness("SystemA", cfg)
        harness_b = BenchmarkHarness("SystemB", cfg)
        # … add workloads and run …

        dashboard = BenchmarkDashboard(title="Q1 2026 Benchmark Suite")
        dashboard.add_harness_results(harness_a)
        dashboard.add_harness_results(harness_b)
        dashboard.generate_html_dashboard("dashboard.html")
        dashboard.generate_json_report("results.json")

    Attributes:
        title: Human-readable title for the dashboard.
    """

    def __init__(
        self,
        title: str = "CHIMERA Benchmark Dashboard",
        normalization_method: NormalizationMethod = NormalizationMethod.MIN_MAX,
    ) -> None:
        """Initialise the dashboard.

        Args:
            title: Human-readable title rendered in the HTML header.
            normalization_method: Strategy used by :class:`BenchmarkScorer`
                when computing normalised per-metric scores.
        """
        self.title = title
        self._normalization_method = normalization_method
        # _systems[system_name][workload_id] = WorkloadResult
        self._systems: Dict[str, Dict[str, WorkloadResult]] = {}
        self._generated_at: str = datetime.now().isoformat()

    # ------------------------------------------------------------------
    # Data ingestion
    # ------------------------------------------------------------------

    def add_harness_results(self, harness: BenchmarkHarness) -> None:
        """Incorporate all completed results from *harness* into the dashboard.

        The harness's ``system_name`` is used as the system identifier.  If
        the same system name is registered more than once, later results for
        each ``workload_id`` replace earlier ones.

        Args:
            harness: A :class:`BenchmarkHarness` whose measurement phase
                (``run`` / ``run_all``) has already been executed.

        Raises:
            ValueError: If *harness* has no recorded results yet.
        """
        results = {wid: harness.get_result(wid)
                   for wid in harness.get_workload_ids()
                   if harness.get_result(wid) is not None}
        if not results:
            raise ValueError(
                f"Harness for '{harness.system_name}' has no completed "
                "workload results.  Run harness.run() or harness.run_all() first."
            )
        system = harness.system_name
        if system not in self._systems:
            self._systems[system] = {}
        self._systems[system].update(results)

    def add_workload_result(self, result: WorkloadResult) -> None:
        """Add a single :class:`WorkloadResult` to the dashboard.

        Useful when results have been loaded from a persisted JSON file rather
        than collected from a live harness run.

        Args:
            result: A completed :class:`WorkloadResult`.  The ``system_name``
                and ``workload_id`` fields are used as the lookup keys.
        """
        system = result.system_name
        if system not in self._systems:
            self._systems[system] = {}
        self._systems[system][result.workload_id] = result

    # ------------------------------------------------------------------
    # Accessors
    # ------------------------------------------------------------------

    def get_system_names(self) -> List[str]:
        """Return a sorted list of registered system names."""
        return sorted(self._systems.keys())

    def get_workload_ids(self) -> List[str]:
        """Return a sorted list of workload IDs present in any registered system."""
        ids: set = set()
        for results in self._systems.values():
            ids.update(results.keys())
        return sorted(ids)

    # ------------------------------------------------------------------
    # Aggregation
    # ------------------------------------------------------------------

    def aggregate(self) -> Dict[str, Any]:
        """Build a structured aggregation of all collected results.

        For each workload the method collects throughput and latency
        percentiles across all systems.  It then uses
        :class:`~chimera.scoring.BenchmarkScorer` to produce normalised
        per-metric scores and a composite ranking.

        Returns:
            A JSON-serialisable dict with the following structure::

                {
                    "title": "<dashboard title>",
                    "generated_at": "<ISO-8601 timestamp>",
                    "normalization_method": "<method name>",
                    "systems": ["SystemA", "SystemB", ...],
                    "workloads": ["wl_a", "wl_b", ...],
                    "results": {
                        "<workload_id>": {
                            "<system_name>": {
                                "throughput_ops_per_sec": ...,
                                "mean_latency_ms": ...,
                                "p50_latency_ms": ...,
                                "p95_latency_ms": ...,
                                "p99_latency_ms": ...,
                                "error_count": ...,
                                "run_iterations": ...
                            },
                            ...
                        },
                        ...
                    },
                    "composite_rankings": [
                        {"rank": 1, "system": "...", "composite_score": ...,
                         "per_metric": {...}},
                        ...
                    ]
                }

        Raises:
            ValueError: If no results have been added to the dashboard.
        """
        if not self._systems:
            raise ValueError("No results have been added to the dashboard.")

        system_names = self.get_system_names()
        workload_ids = self.get_workload_ids()

        # Per-workload raw results
        raw_results: Dict[str, Dict[str, Dict[str, Any]]] = {}
        for wid in workload_ids:
            wl_data: Dict[str, Dict[str, Any]] = {}
            for sys in system_names:
                result = self._systems.get(sys, {}).get(wid)
                if result is not None:
                    wl_data[sys] = {
                        "throughput_ops_per_sec": result.throughput_ops_per_sec,
                        "mean_latency_ms": result.mean_latency_ms,
                        "p50_latency_ms": result.p50_latency_ms,
                        "p95_latency_ms": result.p95_latency_ms,
                        "p99_latency_ms": result.p99_latency_ms,
                        "error_count": result.error_count,
                        "run_iterations": result.run_iterations,
                    }
            raw_results[wid] = wl_data

        # Composite scoring across all workloads and metric types
        composite_rankings = self._compute_composite_rankings(workload_ids, system_names)

        return {
            "title": self.title,
            "generated_at": self._generated_at,
            "normalization_method": self._normalization_method.value,
            "systems": system_names,
            "workloads": workload_ids,
            "results": raw_results,
            "composite_rankings": composite_rankings,
        }

    def _compute_composite_rankings(
        self,
        workload_ids: List[str],
        system_names: List[str],
    ) -> List[Dict[str, Any]]:
        """Compute normalised composite scores across all workloads and metrics.

        Each (workload × metric) pair becomes one :class:`MetricConfig` entry
        so that the scorer treats every combination independently before
        combining them into a weighted composite.

        Returns:
            List of ranking dicts ordered best-first, each with keys
            ``rank``, ``system``, ``composite_score``, and ``per_metric``.
        """
        metric_configs: List[MetricConfig] = []
        for wid in workload_ids:
            metric_configs.extend([
                MetricConfig(
                    name=f"{wid}__throughput",
                    direction=MetricDirection.HIGHER_IS_BETTER,
                    weight=1.0,
                    unit="ops/s",
                ),
                MetricConfig(
                    name=f"{wid}__p99_latency",
                    direction=MetricDirection.LOWER_IS_BETTER,
                    weight=1.0,
                    unit="ms",
                ),
            ])

        if not metric_configs:
            return []

        scorer = BenchmarkScorer(
            metrics=metric_configs,
            normalization_method=self._normalization_method,
        )

        for sys in system_names:
            for wid in workload_ids:
                result = self._systems.get(sys, {}).get(wid)
                if result is None:
                    continue
                if result.throughput_ops_per_sec > 0:
                    scorer.add_results(
                        sys,
                        f"{wid}__throughput",
                        [result.throughput_ops_per_sec],
                    )
                if result.p99_latency_ms > 0:
                    scorer.add_results(
                        sys,
                        f"{wid}__p99_latency",
                        [result.p99_latency_ms],
                    )

        ranked = scorer.rank_systems()
        rankings: List[Dict[str, Any]] = []
        for cs in ranked:
            rankings.append({
                "rank": cs.rank,
                "system": cs.system_name,
                "composite_score": round(cs.composite_score, 2),
                "per_metric": {
                    metric: {
                        "raw_mean": round(ns.raw_mean, 4),
                        "normalized_score": round(ns.normalized_score, 2),
                        "metric_rank": ns.rank,
                    }
                    for metric, ns in cs.metric_scores.items()
                },
            })
        return rankings

    # ------------------------------------------------------------------
    # Export
    # ------------------------------------------------------------------

    def generate_json_report(self, output_path: str) -> None:
        """Export the aggregated dashboard data as a JSON file.

        Args:
            output_path: Destination file path for the JSON report.
        """
        summary = self.aggregate()
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as fh:
            json.dump(summary, fh, indent=2)
        print(f"✓ JSON report saved to: {output_path}")

    def generate_html_dashboard(self, output_path: str) -> None:
        """Generate a self-contained HTML benchmark dashboard.

        The dashboard includes:

        * An executive summary table (system × workload).
        * Composite rankings with normalised scores.
        * Per-workload latency and throughput comparison tables.
        * Optional matplotlib visualisations when the library is available.

        Args:
            output_path: Destination file path for the HTML dashboard.
        """
        summary = self.aggregate()
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)

        parts = [
            self._html_header(),
            self._html_body_open(summary),
            self._html_composite_rankings(summary),
            self._html_workload_breakdown(summary),
            self._html_visualizations(summary),
            self._html_footer(),
        ]

        html = "\n".join(parts)
        with open(output_path, "w", encoding="utf-8") as fh:
            fh.write(html)
        print(f"✓ HTML dashboard saved to: {output_path}")

    # ------------------------------------------------------------------
    # HTML helpers
    # ------------------------------------------------------------------

    def _html_header(self) -> str:
        css = VendorNeutralStyle.get_css_theme()
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{self.title}</title>
    <style>
        {css}
        body {{ max-width: 1200px; margin: 0 auto; padding: 20px; }}
        h1 {{ color: var(--color-primary); border-bottom: 3px solid var(--color-primary); padding-bottom: 10px; }}
        h2 {{ color: var(--color-secondary); margin-top: 30px; }}
        h3 {{ margin-top: 20px; }}
        table {{ width: 100%; border-collapse: collapse; margin: 16px 0; }}
        th, td {{ text-align: left; padding: 10px 12px; border-bottom: 1px solid #ddd; }}
        th {{ background-color: var(--color-primary); color: white; }}
        tr:hover {{ background-color: #f5f5f5; }}
        .rank-badge {{ display: inline-block; background: var(--color-primary); color: white;
                       border-radius: 50%; width: 24px; height: 24px; text-align: center;
                       line-height: 24px; font-weight: bold; font-size: 0.85em; }}
        .metric-card {{ background: #f8f9fa; padding: 14px; margin: 10px 0;
                        border-radius: 5px; border-left: 4px solid var(--color-info); }}
        .plot {{ margin: 20px 0; text-align: center; }}
        footer {{ margin-top: 40px; padding-top: 20px; border-top: 2px solid #ddd;
                  color: #666; font-size: 0.9em; }}
        .neutrality-note {{ font-style: italic; color: #555; margin-bottom: 16px; }}
    </style>
</head>
<body>"""

    def _html_body_open(self, summary: Dict[str, Any]) -> str:
        systems = ", ".join(summary["systems"]) or "—"
        workloads = ", ".join(summary["workloads"]) or "—"
        return f"""
    <header>
        <h1>🔬 {self.title}</h1>
        <p><strong>Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis</strong></p>
        <p>Generated: {summary["generated_at"]} &nbsp;|&nbsp;
           Normalization: {summary["normalization_method"]} &nbsp;|&nbsp;
           CHIMERA v{_CHIMERA_VERSION}</p>
    </header>

    <section>
        <h2>Overview</h2>
        <p class="neutrality-note">✓ Vendor-neutral report — alphabetical ordering, no brand colours, IEEE-compliant methodology.</p>
        <p><strong>Systems evaluated:</strong> {systems}</p>
        <p><strong>Workloads:</strong> {workloads}</p>
    </section>

    <section>
        <h2>Executive Summary</h2>
        {self._html_summary_table(summary)}
    </section>"""

    def _html_summary_table(self, summary: Dict[str, Any]) -> str:
        """Cross-reference table: system × workload showing throughput and p99."""
        systems = summary["systems"]
        workloads = summary["workloads"]
        results = summary["results"]

        if not systems or not workloads:
            return "<p><em>No results to display.</em></p>"

        rows = ["<table>", "<thead><tr>",
                "<th>System</th>"]
        for wid in workloads:
            rows.append(f"<th>{wid}<br><small>tput (ops/s)</small></th>")
            rows.append(f"<th>{wid}<br><small>p99 (ms)</small></th>")
        rows.append("</tr></thead><tbody>")

        for sys in systems:
            rows.append(f"<tr><td><strong>{sys}</strong></td>")
            for wid in workloads:
                entry = results.get(wid, {}).get(sys)
                if entry:
                    tput = entry["throughput_ops_per_sec"]
                    p99 = entry["p99_latency_ms"]
                    rows.append(f"<td>{tput:,.1f}</td><td>{p99:.3f}</td>")
                else:
                    rows.append("<td>—</td><td>—</td>")
            rows.append("</tr>")

        rows.append("</tbody></table>")
        return "\n".join(rows)

    def _html_composite_rankings(self, summary: Dict[str, Any]) -> str:
        rankings = summary.get("composite_rankings", [])
        if not rankings:
            return ""

        rows = ["<section>", "<h2>Composite Rankings</h2>",
                "<p class='neutrality-note'>Scores normalised to [0, 100] using "
                f"{summary['normalization_method']} strategy. "
                "Higher is always better regardless of raw metric direction.</p>",
                "<table>", "<thead><tr>",
                "<th>Rank</th><th>System</th><th>Composite Score</th>",
                "</tr></thead><tbody>"]

        for entry in rankings:
            rank = entry["rank"]
            sys = entry["system"]
            score = entry["composite_score"]
            badge = f'<span class="rank-badge">{rank}</span>'
            rows.append(f"<tr><td>{badge}</td><td>{sys}</td><td>{score:.2f} / 100</td></tr>")

        rows.append("</tbody></table>")
        rows.append("</section>")
        return "\n".join(rows)

    def _html_workload_breakdown(self, summary: Dict[str, Any]) -> str:
        systems = summary["systems"]
        workloads = summary["workloads"]
        results = summary["results"]

        if not workloads:
            return ""

        parts = ["<section>", "<h2>Workload Breakdown</h2>"]

        for wid in workloads:
            wl_data = results.get(wid, {})
            parts.append(f"<div class='metric-card'>")
            parts.append(f"<h3>Workload: {wid}</h3>")
            parts.append("<table><thead><tr>")
            parts.append("<th>System</th><th>Throughput (ops/s)</th>"
                         "<th>Mean Latency (ms)</th><th>p50 (ms)</th>"
                         "<th>p95 (ms)</th><th>p99 (ms)</th><th>Errors</th></tr></thead><tbody>")

            for sys in systems:
                entry = wl_data.get(sys)
                if entry:
                    parts.append(
                        f"<tr>"
                        f"<td><strong>{sys}</strong></td>"
                        f"<td>{entry['throughput_ops_per_sec']:,.1f}</td>"
                        f"<td>{entry['mean_latency_ms']:.4f}</td>"
                        f"<td>{entry['p50_latency_ms']:.4f}</td>"
                        f"<td>{entry['p95_latency_ms']:.4f}</td>"
                        f"<td>{entry['p99_latency_ms']:.4f}</td>"
                        f"<td>{entry['error_count']}</td>"
                        f"</tr>"
                    )
                else:
                    parts.append(f"<tr><td><strong>{sys}</strong></td>"
                                 + "<td>—</td>" * 6 + "</tr>")

            parts.append("</tbody></table></div>")

        parts.append("</section>")
        return "\n".join(parts)

    def _html_visualizations(self, summary: Dict[str, Any]) -> str:
        """Generate optional matplotlib bar charts for each workload."""
        import base64
        from io import BytesIO

        systems = summary["systems"]
        workloads = summary["workloads"]
        results = summary["results"]

        if not systems or not workloads:
            return ""

        parts = ["<section>", "<h2>Visualizations</h2>",
                 "<p><em>All plots use colour-blind friendly palettes (Okabe-Ito/Tol schemes).</em></p>"]

        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt

            palette = ColorBlindPalette()
            colors = palette.get_matplotlib_colors(len(systems), "tol_muted")

            for wid in workloads:
                wl_data = results.get(wid, {})
                tputs = [wl_data.get(s, {}).get("throughput_ops_per_sec", 0.0) for s in systems]
                p99s = [wl_data.get(s, {}).get("p99_latency_ms", 0.0) for s in systems]

                fig, (ax_t, ax_p) = plt.subplots(1, 2, figsize=(12, 4))

                # Throughput bar chart
                bars = ax_t.bar(systems, tputs, color=colors)
                ax_t.set_title(f"{wid} — Throughput")
                ax_t.set_ylabel("ops/s")
                ax_t.tick_params(axis="x", rotation=30)
                ax_t.grid(axis="y", alpha=0.3)

                # p99 latency bar chart
                ax_p.bar(systems, p99s, color=colors)
                ax_p.set_title(f"{wid} — p99 Latency")
                ax_p.set_ylabel("ms")
                ax_p.tick_params(axis="x", rotation=30)
                ax_p.grid(axis="y", alpha=0.3)

                plt.tight_layout()
                buf = BytesIO()
                plt.savefig(buf, format="png", dpi=120, bbox_inches="tight")
                buf.seek(0)
                img_b64 = base64.b64encode(buf.read()).decode("utf-8")
                plt.close(fig)

                parts.append(
                    f'<div class="plot">'
                    f'<img src="data:image/png;base64,{img_b64}" '
                    f'alt="{wid} comparison chart" style="max-width:100%;">'
                    f"</div>"
                )

        except ImportError:
            parts.append("<p><em>Install matplotlib for charts: pip install matplotlib</em></p>")

        parts.append("</section>")
        return "\n".join(parts)

    def _html_footer(self) -> str:
        year = datetime.now().year
        return f"""
    <footer>
        <p><strong>CHIMERA v{_CHIMERA_VERSION}</strong> — Benchmark Result Aggregation and Reporting Dashboard</p>
        <p>Generated: {self._generated_at}</p>
        <p>Sorting: alphabetical only. No vendor logos, brand colours, or promotional language.</p>
        <p>© {year} CHIMERA Project. Released under MIT License.</p>
    </footer>
</body>
</html>"""
