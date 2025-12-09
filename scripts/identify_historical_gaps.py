#!/usr/bin/env python3
"""
Historical Gap Identifier - Analyzes v1.0.0 Benchmark Results
==============================================================

Extrahiert und kategorisiert historische Performance-Gaps aus v1.0.0:
- Identifiziert Konkurrenten-Überlegenheit
- Priorisiert Verbesserungen nach Impact
- Generiert Gap-Closure Targets für v1.0.1

Usage:
    python3 identify_historical_gaps.py --input benchmarks/enterprise_benchmarks_20251204_*/
"""

import json
import os
import sys
import argparse
from datetime import datetime
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
import statistics
from pathlib import Path


@dataclass
class PerformanceGap:
    """Einzelner Performance Gap"""
    workload: str
    test: str
    protocol: str
    themis_latency_ms: float
    competitor: str
    competitor_latency_ms: float
    latency_delta_ms: float
    percentage_worse: float  # Negative = schlechter, Positive = besser
    throughput_delta: float
    severity: str  # 'critical', 'high', 'medium', 'low'
    category: str  # 'latency', 'throughput', 'consistency'
    
    def to_dict(self):
        return asdict(self)


class HistoricalGapAnalyzer:
    """Analyzes historical performance gaps from v1.0.0"""
    
    def __init__(self, baseline_dir: str):
        self.baseline_dir = baseline_dir
        self.results = {}
        self.gaps: List[PerformanceGap] = []
        self.summary = {
            "total_gaps": 0,
            "critical_gaps": 0,
            "high_gaps": 0,
            "medium_gaps": 0,
            "low_gaps": 0,
            "by_workload": {},
            "by_competitor": {},
            "by_category": {}
        }
    
    def load_results(self) -> bool:
        """Lädt Baseline-Ergebnisse aus JSON"""
        print(f"Loading baseline results from: {self.baseline_dir}")
        
        results_file = os.path.join(self.baseline_dir, "benchmark_results.json")
        
        if not os.path.exists(results_file):
            print(f"ERROR: Baseline file not found: {results_file}")
            return False
        
        try:
            with open(results_file, 'r') as f:
                self.results = json.load(f)
            print(f"✓ Loaded baseline results")
            return True
        except Exception as e:
            print(f"ERROR: Failed to load results: {e}")
            return False
    
    def analyze_gaps(self) -> None:
        """Analysiert Gaps zwischen ThemisDB und Konkurrenten"""
        print("\nAnalyzing performance gaps...")
        
        if not self.results:
            print("No results to analyze")
            return
        
        # Iteriere über Workloads
        for workload, competitors_data in self.results.items():
            print(f"\n  Workload: {workload}")
            
            if not isinstance(competitors_data, dict) or "competitors" not in competitors_data:
                continue
            
            competitors = competitors_data.get("competitors", {})
            themis_data = competitors.get("ThemisDB", {})
            
            if not themis_data:
                print(f"    No ThemisDB data found")
                continue
            
            themis_protocols = themis_data.get("protocols", {})
            
            # Für jeden Konkurrenten
            for competitor_name, competitor_data in competitors.items():
                if competitor_name == "ThemisDB":
                    continue
                
                competitor_protocols = competitor_data.get("protocols", {})
                
                # Für jedes Protokoll
                for protocol, themis_metrics in themis_protocols.items():
                    if protocol not in competitor_protocols:
                        continue
                    
                    competitor_metrics = competitor_protocols[protocol]
                    
                    themis_latency = themis_metrics.get("latency_mean_ms", 0)
                    competitor_latency = competitor_metrics.get("latency_mean_ms", 0)
                    
                    themis_throughput = themis_metrics.get("throughput_ops_sec", 0)
                    competitor_throughput = competitor_metrics.get("throughput_ops_sec", 0)
                    
                    if themis_latency == 0 or competitor_latency == 0:
                        continue
                    
                    # Berechne Deltas
                    latency_delta = competitor_latency - themis_latency
                    percentage_worse = ((themis_latency - competitor_latency) / competitor_latency) * 100
                    
                    throughput_delta = competitor_throughput - themis_throughput
                    
                    # Kategorisiere Schweregrad
                    if percentage_worse < -30:  # ThemisDB > 30% schlechter
                        severity = "critical"
                    elif percentage_worse < -15:  # ThemisDB > 15% schlechter
                        severity = "high"
                    elif percentage_worse < -5:   # ThemisDB > 5% schlechter
                        severity = "medium"
                    else:
                        severity = "low"
                    
                    # Nur Gaps (ThemisDB schlechter)
                    if percentage_worse < 0:
                        gap = PerformanceGap(
                            workload=workload,
                            test="unknown",  # Nicht in Results enthalten
                            protocol=protocol,
                            themis_latency_ms=themis_latency,
                            competitor=competitor_name,
                            competitor_latency_ms=competitor_latency,
                            latency_delta_ms=abs(latency_delta),
                            percentage_worse=abs(percentage_worse),
                            throughput_delta=throughput_delta,
                            severity=severity,
                            category="latency"
                        )
                        
                        self.gaps.append(gap)
                        
                        print(f"    ✗ {competitor_name} ({protocol}): " +
                              f"ThemisDB {abs(percentage_worse):.1f}% slower " +
                              f"({themis_latency:.2f}ms vs {competitor_latency:.2f}ms) " +
                              f"[{severity.upper()}]")
    
    def categorize_gaps(self) -> None:
        """Kategorisiert Gaps nach Workload, Konkurrent und Kategorie"""
        print("\nCategorizing gaps...")
        
        # By Workload
        for gap in self.gaps:
            if gap.workload not in self.summary["by_workload"]:
                self.summary["by_workload"][gap.workload] = {
                    "total_gaps": 0,
                    "critical": 0,
                    "high": 0,
                    "medium": 0,
                    "low": 0,
                    "avg_percentage_worse": 0
                }
            
            wl_summary = self.summary["by_workload"][gap.workload]
            wl_summary["total_gaps"] += 1
            wl_summary[gap.severity] += 1
        
        # By Competitor
        for gap in self.gaps:
            if gap.competitor not in self.summary["by_competitor"]:
                self.summary["by_competitor"][gap.competitor] = {
                    "total_gaps": 0,
                    "critical": 0,
                    "high": 0,
                    "medium": 0,
                    "low": 0,
                    "avg_percentage_worse": 0
                }
            
            comp_summary = self.summary["by_competitor"][gap.competitor]
            comp_summary["total_gaps"] += 1
            comp_summary[gap.severity] += 1
        
        # Summary Stats
        self.summary["total_gaps"] = len(self.gaps)
        self.summary["critical_gaps"] = len([g for g in self.gaps if g.severity == "critical"])
        self.summary["high_gaps"] = len([g for g in self.gaps if g.severity == "high"])
        self.summary["medium_gaps"] = len([g for g in self.gaps if g.severity == "medium"])
        self.summary["low_gaps"] = len([g for g in self.gaps if g.severity == "low"])
    
    def print_summary(self) -> None:
        """Gibt Gap-Zusammenfassung aus"""
        print("\n" + "=" * 80)
        print("HISTORICAL PERFORMANCE GAPS SUMMARY (v1.0.0)")
        print("=" * 80)
        
        print(f"\nTotal Gaps: {self.summary['total_gaps']}")
        print(f"  Critical: {self.summary['critical_gaps']}")
        print(f"  High:     {self.summary['high_gaps']}")
        print(f"  Medium:   {self.summary['medium_gaps']}")
        print(f"  Low:      {self.summary['low_gaps']}")
        
        print("\nBy Workload:")
        for workload, wl_stats in sorted(self.summary["by_workload"].items()):
            print(f"  {workload:20} {wl_stats['total_gaps']:3} gaps " +
                  f"(C:{wl_stats['critical']} H:{wl_stats['high']} M:{wl_stats['medium']})")
        
        print("\nBy Competitor (Top Offenders):")
        sorted_comps = sorted(
            self.summary["by_competitor"].items(),
            key=lambda x: x[1]["critical"] + x[1]["high"],
            reverse=True
        )
        for comp, comp_stats in sorted_comps[:10]:
            print(f"  {comp:20} {comp_stats['total_gaps']:3} gaps " +
                  f"(C:{comp_stats['critical']} H:{comp_stats['high']})")
        
        print("\n" + "=" * 80)
        print("TOP 10 CRITICAL GAPS (Priorität für v1.0.1)")
        print("=" * 80)
        
        sorted_gaps = sorted(
            [g for g in self.gaps if g.severity == "critical"],
            key=lambda g: g.percentage_worse,
            reverse=True
        )
        
        for i, gap in enumerate(sorted_gaps[:10], 1):
            print(f"\n{i}. {gap.workload} vs {gap.competitor} ({gap.protocol})")
            print(f"   ThemisDB: {gap.themis_latency_ms:.3f}ms")
            print(f"   {gap.competitor}: {gap.competitor_latency_ms:.3f}ms")
            print(f"   Delta: +{gap.percentage_worse:.1f}% slower")
            print(f"   Latency Diff: {gap.latency_delta_ms:.3f}ms")
    
    def generate_report(self, output_file: str) -> None:
        """Generiert detaillierten Gap-Report"""
        print(f"\nGenerating gap report: {output_file}")
        
        report = {
            "timestamp": datetime.now().isoformat(),
            "version": "1.0.0",
            "analysis_type": "Historical Gap Identification",
            "summary": self.summary,
            "critical_gaps": [g.to_dict() for g in self.gaps if g.severity == "critical"],
            "high_gaps": [g.to_dict() for g in self.gaps if g.severity == "high"],
            "medium_gaps": [g.to_dict() for g in self.gaps if g.severity == "medium"],
            "low_gaps": [g.to_dict() for g in self.gaps if g.severity == "low"],
            "all_gaps": [g.to_dict() for g in self.gaps],
        }
        
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"✓ Report saved to {output_file}")
    
    def generate_markdown_report(self, output_file: str) -> None:
        """Generiert Markdown Gap-Report"""
        print(f"\nGenerating markdown report: {output_file}")
        
        md = "# Historical Performance Gaps Analysis (v1.0.0)\n\n"
        md += f"**Date:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        md += f"**Version Analyzed:** 1.0.0\n"
        md += f"**Total Gaps:** {self.summary['total_gaps']}\n\n"
        
        # Summary Section
        md += "## Summary Statistics\n\n"
        md += f"- **Critical:** {self.summary['critical_gaps']}\n"
        md += f"- **High:** {self.summary['high_gaps']}\n"
        md += f"- **Medium:** {self.summary['medium_gaps']}\n"
        md += f"- **Low:** {self.summary['low_gaps']}\n\n"
        
        # By Workload
        md += "## Gaps by Workload\n\n"
        md += "| Workload | Total | Critical | High | Medium | Low |\n"
        md += "|----------|-------|----------|------|--------|-----|\n"
        for workload, stats in sorted(self.summary["by_workload"].items()):
            md += f"| {workload} | {stats['total_gaps']} | "
            md += f"{stats['critical']} | {stats['high']} | "
            md += f"{stats['medium']} | {stats['low']} |\n"
        
        md += "\n"
        
        # Top Competitors
        md += "## Performance Gaps by Competitor\n\n"
        sorted_comps = sorted(
            self.summary["by_competitor"].items(),
            key=lambda x: x[1]["critical"] + x[1]["high"],
            reverse=True
        )
        
        md += "| Competitor | Total | Critical | High | Medium | Low |\n"
        md += "|------------|-------|----------|------|--------|-----|\n"
        for comp, stats in sorted_comps:
            md += f"| {comp} | {stats['total_gaps']} | "
            md += f"{stats['critical']} | {stats['high']} | "
            md += f"{stats['medium']} | {stats['low']} |\n"
        
        md += "\n"
        
        # Critical Gaps Detail
        md += "## Critical Performance Gaps\n\n"
        critical = sorted(
            [g for g in self.gaps if g.severity == "critical"],
            key=lambda g: g.percentage_worse,
            reverse=True
        )
        
        for i, gap in enumerate(critical, 1):
            md += f"### {i}. {gap.workload.title()} vs {gap.competitor} ({gap.protocol})\n\n"
            md += f"- **ThemisDB Latency:** {gap.themis_latency_ms:.3f}ms\n"
            md += f"- **{gap.competitor} Latency:** {gap.competitor_latency_ms:.3f}ms\n"
            md += f"- **Performance Gap:** +{gap.percentage_worse:.1f}% slower\n"
            md += f"- **Latency Delta:** {gap.latency_delta_ms:.3f}ms\n"
            md += f"- **Category:** {gap.category}\n\n"
        
        with open(output_file, 'w') as f:
            f.write(md)
        
        print(f"✓ Markdown report saved to {output_file}")
    
    def generate_closure_targets(self, output_file: str) -> None:
        """Generiert v1.0.1 Gap-Closure Targets"""
        print(f"\nGenerating closure targets: {output_file}")
        
        targets = {
            "timestamp": datetime.now().isoformat(),
            "version": "1.0.1",
            "gap_closure_strategy": "Priorisiere kritische Gaps mit höchstem Impact",
            "targets": []
        }
        
        # Sortiere nach Impact (Severity × Percentage Worse)
        impact_sorted = sorted(
            self.gaps,
            key=lambda g: {
                "critical": 100,
                "high": 50,
                "medium": 20,
                "low": 5
            }[g.severity] * g.percentage_worse,
            reverse=True
        )
        
        for gap in impact_sorted[:20]:  # Top 20 Prioritäten
            target = {
                "workload": gap.workload,
                "competitor": gap.competitor,
                "protocol": gap.protocol,
                "current_gap_pct": round(gap.percentage_worse, 1),
                "target_closure_pct": min(100, round(gap.percentage_worse * 0.7, 1)),  # Ziel: 70% Closure
                "target_latency_ms": round(
                    gap.themis_latency_ms * 0.7, 3  # Ziel: 30% Latenz-Reduktion
                ),
                "severity": gap.severity,
                "impact_score": round(
                    ({
                        "critical": 100,
                        "high": 50,
                        "medium": 20,
                        "low": 5
                    }[gap.severity] * gap.percentage_worse), 1
                )
            }
            targets["targets"].append(target)
        
        # Add Summary
        targets["summary"] = {
            "total_critical_targets": len([t for t in targets["targets"] if t["severity"] == "critical"]),
            "total_high_targets": len([t for t in targets["targets"] if t["severity"] == "high"]),
            "total_targets": len(targets["targets"]),
            "expected_overall_improvement": "~45-50% average latency reduction"
        }
        
        with open(output_file, 'w') as f:
            json.dump(targets, f, indent=2)
        
        print(f"✓ Closure targets saved to {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="Identify Historical Performance Gaps from v1.0.0"
    )
    parser.add_argument(
        "--input",
        default="benchmarks/enterprise_benchmarks_20251204_213836",
        help="Directory with v1.0.0 benchmark results"
    )
    parser.add_argument(
        "--output-dir",
        default="benchmarks/gap_analysis",
        help="Output directory for reports"
    )
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Initialize analyzer
    analyzer = HistoricalGapAnalyzer(args.input)
    
    # Load and analyze
    if not analyzer.load_results():
        sys.exit(1)
    
    analyzer.analyze_gaps()
    analyzer.categorize_gaps()
    
    # Print summary
    analyzer.print_summary()
    
    # Generate reports
    analyzer.generate_report(os.path.join(args.output_dir, "historical_gaps.json"))
    analyzer.generate_markdown_report(os.path.join(args.output_dir, "historical_gaps.md"))
    analyzer.generate_closure_targets(os.path.join(args.output_dir, "v1.0.1_closure_targets.json"))
    
    print("\n✓ Gap analysis completed successfully!")


if __name__ == "__main__":
    main()
