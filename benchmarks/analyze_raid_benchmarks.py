"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analyze_raid_benchmarks.py                         ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     316                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Docker RAID Benchmark Results Analyzer

Analyzes JSON output from bench_docker_raid_comprehensive and generates:
- Summary statistics
- Performance comparison tables
- RAID-level recommendations
- Visualization-ready data

Usage:
    python analyze_raid_benchmarks.py results_dir/
    python analyze_raid_benchmarks.py results.json --output report.html
"""

import json
import sys
import argparse
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Any
import statistics

def parse_args():
    parser = argparse.ArgumentParser(
        description="Analyze Docker RAID benchmark results"
    )
    parser.add_argument(
        "input",
        help="JSON results file or directory containing results.json"
    )
    parser.add_argument(
        "--output", "-o",
        help="Output file (markdown/html/csv)",
        default="raid_benchmark_report.md"
    )
    parser.add_argument(
        "--format", "-f",
        choices=["markdown", "html", "csv"],
        default="markdown",
        help="Output format"
    )
    return parser.parse_args()

def load_results(path: Path) -> Dict:
    """Load benchmark results from JSON file"""
    if path.is_dir():
        json_file = path / "results.json"
        if not json_file.exists():
            print(f"Error: No results.json found in {path}")
            sys.exit(1)
        path = json_file
    
    with open(path, 'r') as f:
        return json.load(f)

def extract_raid_level(benchmark_name: str) -> str:
    """Extract RAID level from benchmark name"""
    # Format: BenchmarkName/containers/raid_level/param
    parts = benchmark_name.split('/')
    if len(parts) >= 3:
        raid_num = int(parts[2])
        return f"RAID{raid_num}"
    return "Unknown"

def extract_suite_name(benchmark_name: str) -> str:
    """Extract suite name from benchmark name"""
    return benchmark_name.split('/')[0]

def calculate_throughput_mbps(benchmark: Dict) -> float:
    """Calculate throughput in MB/s from benchmark data"""
    if "bytes_processed" in benchmark:
        bytes_proc = benchmark["bytes_processed"]
        time_sec = benchmark["real_time"]
        return (bytes_proc / (1024 * 1024)) / time_sec
    return 0.0

def analyze_by_raid_level(benchmarks: List[Dict]) -> Dict:
    """Group and analyze benchmarks by RAID level"""
    by_raid = defaultdict(list)
    
    for bench in benchmarks:
        if bench["run_type"] != "iteration":
            continue
        
        raid_level = extract_raid_level(bench["name"])
        by_raid[raid_level].append(bench)
    
    summary = {}
    for raid_level, benches in by_raid.items():
        throughputs = [calculate_throughput_mbps(b) for b in benches if calculate_throughput_mbps(b) > 0]
        latencies = [b["real_time"] for b in benches]
        
        summary[raid_level] = {
            "num_tests": len(benches),
            "avg_throughput_mbps": statistics.mean(throughputs) if throughputs else 0,
            "max_throughput_mbps": max(throughputs) if throughputs else 0,
            "avg_latency_ms": statistics.mean(latencies),
            "p95_latency_ms": statistics.quantiles(latencies, n=20)[18] if len(latencies) > 20 else max(latencies),
            "p99_latency_ms": statistics.quantiles(latencies, n=100)[98] if len(latencies) > 100 else max(latencies),
        }
    
    return summary

def analyze_by_suite(benchmarks: List[Dict]) -> Dict:
    """Group and analyze benchmarks by suite"""
    by_suite = defaultdict(list)
    
    for bench in benchmarks:
        if bench["run_type"] != "iteration":
            continue
        
        suite_name = extract_suite_name(bench["name"])
        by_suite[suite_name].append(bench)
    
    summary = {}
    for suite_name, benches in by_suite.items():
        latencies = [b["real_time"] for b in benches]
        
        summary[suite_name] = {
            "num_tests": len(benches),
            "total_time_sec": sum(latencies),
            "avg_latency_ms": statistics.mean(latencies),
            "min_latency_ms": min(latencies),
            "max_latency_ms": max(latencies),
        }
    
    return summary

def generate_markdown_report(results: Dict, raid_summary: Dict, suite_summary: Dict) -> str:
    """Generate markdown report"""
    context = results.get("context", {})
    benchmarks = results.get("benchmarks", [])
    
    md = []
    md.append("# ThemisDB Docker RAID Benchmark Report\n")
    md.append(f"**Generated:** {context.get('date', 'N/A')}\n")
    md.append(f"**Total Tests:** {len(benchmarks)}\n")
    md.append("\n---\n")
    
    # System Info
    md.append("## System Information\n")
    md.append(f"- **CPUs:** {context.get('num_cpus', 'N/A')}\n")
    md.append(f"- **CPU MHz:** {context.get('mhz_per_cpu', 'N/A')}\n")
    md.append(f"- **Hostname:** {context.get('host_name', 'N/A')}\n")
    md.append(f"- **CPU Scaling:** {context.get('cpu_scaling_enabled', 'N/A')}\n")
    md.append("\n")
    
    # RAID Level Comparison
    md.append("## RAID Level Performance Comparison\n")
    md.append("| RAID Level | Tests | Avg Throughput (MB/s) | Max Throughput (MB/s) | Avg Latency (ms) | P95 Latency (ms) |\n")
    md.append("|------------|-------|----------------------|----------------------|------------------|------------------|\n")
    
    for raid_level in sorted(raid_summary.keys()):
        data = raid_summary[raid_level]
        md.append(f"| **{raid_level}** | {data['num_tests']} | "
                 f"{data['avg_throughput_mbps']:.2f} | "
                 f"{data['max_throughput_mbps']:.2f} | "
                 f"{data['avg_latency_ms']:.3f} | "
                 f"{data['p95_latency_ms']:.3f} |\n")
    
    md.append("\n")
    
    # Suite Performance
    md.append("## Benchmark Suite Performance\n")
    md.append("| Suite | Tests | Total Time (s) | Avg Latency (ms) | Min (ms) | Max (ms) |\n")
    md.append("|-------|-------|----------------|------------------|----------|----------|\n")
    
    for suite_name in sorted(suite_summary.keys()):
        data = suite_summary[suite_name]
        md.append(f"| {suite_name} | {data['num_tests']} | "
                 f"{data['total_time_sec']:.2f} | "
                 f"{data['avg_latency_ms']:.3f} | "
                 f"{data['min_latency_ms']:.3f} | "
                 f"{data['max_latency_ms']:.3f} |\n")
    
    md.append("\n")
    
    # Recommendations
    md.append("## Performance Recommendations\n")
    md.append(generate_recommendations(raid_summary))
    
    md.append("\n---\n")
    md.append("*Report generated by analyze_raid_benchmarks.py*\n")
    
    return "".join(md)

def generate_recommendations(raid_summary: Dict) -> str:
    """Generate RAID level recommendations based on results"""
    rec = []
    
    # Find best performers
    best_throughput = max(raid_summary.values(), key=lambda x: x['avg_throughput_mbps'])
    best_latency = min(raid_summary.values(), key=lambda x: x['avg_latency_ms'])
    
    best_throughput_raid = [k for k, v in raid_summary.items() if v == best_throughput][0]
    best_latency_raid = [k for k, v in raid_summary.items() if v == best_latency][0]
    
    rec.append(f"### 🏆 Best Throughput: **{best_throughput_raid}**\n")
    rec.append(f"- Average: {best_throughput['avg_throughput_mbps']:.2f} MB/s\n")
    rec.append(f"- Peak: {best_throughput['max_throughput_mbps']:.2f} MB/s\n")
    rec.append(f"- **Use Case:** High-volume data ingestion, bulk operations\n\n")
    
    rec.append(f"### ⚡ Best Latency: **{best_latency_raid}**\n")
    rec.append(f"- Average: {best_latency['avg_latency_ms']:.3f} ms\n")
    rec.append(f"- P95: {best_latency['p95_latency_ms']:.3f} ms\n")
    rec.append(f"- **Use Case:** Real-time queries, low-latency applications\n\n")
    
    rec.append("### 📊 General Recommendations\n")
    rec.append("- **RAID0:** Maximum performance, no redundancy - use for temporary/cacheable data\n")
    rec.append("- **RAID1:** Best read performance with full redundancy - ideal for read-heavy workloads\n")
    rec.append("- **RAID5:** Balanced performance/capacity - good for general-purpose storage\n")
    rec.append("- **RAID6:** Dual parity protection - best for mission-critical data\n")
    rec.append("- **RAID10:** Best overall balance - recommended for production databases\n")
    
    return "".join(rec)

def main():
    args = parse_args()
    
    # Load results
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: Input path '{input_path}' does not exist")
        sys.exit(1)
    
    print(f"Loading results from: {input_path}")
    results = load_results(input_path)
    
    # Analyze
    print("Analyzing benchmarks...")
    benchmarks = results.get("benchmarks", [])
    print(f"Found {len(benchmarks)} benchmark results")
    
    raid_summary = analyze_by_raid_level(benchmarks)
    suite_summary = analyze_by_suite(benchmarks)
    
    # Generate report
    print(f"Generating {args.format} report...")
    
    if args.format == "markdown":
        report = generate_markdown_report(results, raid_summary, suite_summary)
    elif args.format == "html":
        md_report = generate_markdown_report(results, raid_summary, suite_summary)
        # Simple HTML wrapper
        report = f"""<!DOCTYPE html>
<html>
<head>
    <title>ThemisDB RAID Benchmark Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; max-width: 1200px; margin: 0 auto; padding: 20px; }}
        table {{ border-collapse: collapse; width: 100%; margin: 20px 0; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #4CAF50; color: white; }}
        tr:nth-child(even) {{ background-color: #f2f2f2; }}
    </style>
</head>
<body>
    <pre>{md_report}</pre>
</body>
</html>"""
    else:  # CSV
        # Simple CSV export of RAID summary
        lines = ["RAID Level,Tests,Avg Throughput (MB/s),Max Throughput (MB/s),Avg Latency (ms),P95 Latency (ms)"]
        for raid_level in sorted(raid_summary.keys()):
            data = raid_summary[raid_level]
            lines.append(f"{raid_level},{data['num_tests']},{data['avg_throughput_mbps']:.2f},"
                        f"{data['max_throughput_mbps']:.2f},{data['avg_latency_ms']:.3f},"
                        f"{data['p95_latency_ms']:.3f}")
        report = "\n".join(lines)
    
    # Write output
    output_path = Path(args.output)
    print(f"Writing report to: {output_path}")
    with open(output_path, 'w') as f:
        f.write(report)
    
    print(f"✓ Report generated successfully!")
    print(f"  Format: {args.format}")
    print(f"  Output: {output_path}")
    print(f"  Size: {len(report)} bytes")
    
    # Print summary to console
    print("\n" + "="*70)
    print("QUICK SUMMARY")
    print("="*70)
    for raid_level in sorted(raid_summary.keys()):
        data = raid_summary[raid_level]
        print(f"{raid_level:8} | Throughput: {data['avg_throughput_mbps']:7.2f} MB/s | "
              f"Latency: {data['avg_latency_ms']:7.3f} ms | Tests: {data['num_tests']:3}")
    print("="*70)

if __name__ == "__main__":
    main()
