"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_report.py                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     708                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Comparative Benchmark - Report Generator

Generates visual reports from benchmark results in various formats:
- HTML: Interactive charts and tables
- Markdown: Documentation-friendly format
- CSV: Raw data export

Usage:
    python generate_report.py --format html --input results/ --output reports/
    python generate_report.py --format markdown --input results/benchmark_results_*.json
"""

import json
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional
import click
from rich.console import Console

console = Console()


class BenchmarkReportGenerator:
    """Generates reports from benchmark results."""
    
    def __init__(self, results_data: Dict[str, Any]):
        self.data = results_data
        self.run_id = results_data.get("run_id", "unknown")
        self.timestamp = results_data.get("timestamp", datetime.now().isoformat())
        self.config = results_data.get("config", {})
        self.databases = results_data.get("databases", [])
        self.results = results_data.get("results", {})
    
    def generate_html(self, output_path: Path) -> None:
        """Generate an interactive HTML report."""
        html_content = self._build_html()
        
        output_file = output_path / f"benchmark_report_{self.run_id}.html"
        with open(output_file, "w") as f:
            f.write(html_content)
        
        console.print(f"[green]✓ HTML report saved to {output_file}[/green]")
    
    def generate_markdown(self, output_path: Path) -> None:
        """Generate a Markdown report for documentation."""
        md_content = self._build_markdown()
        
        output_file = output_path / f"benchmark_report_{self.run_id}.md"
        with open(output_file, "w") as f:
            f.write(md_content)
        
        console.print(f"[green]✓ Markdown report saved to {output_file}[/green]")
    
    def generate_csv(self, output_path: Path) -> None:
        """Export results as CSV for further analysis."""
        import csv
        
        output_file = output_path / f"benchmark_results_{self.run_id}.csv"
        
        # Flatten results
        rows = []
        for db_name, benchmarks in self.results.items():
            for bench in benchmarks:
                row = {
                    "database": db_name,
                    "benchmark": bench["name"],
                    "category": bench["category"],
                    "iterations": bench["iterations"],
                    "mean_ms": bench["mean_ms"],
                    "median_ms": bench["median_ms"],
                    "p50_ms": bench["p50_ms"],
                    "p95_ms": bench["p95_ms"],
                    "p99_ms": bench["p99_ms"],
                    "min_ms": bench["min_ms"],
                    "max_ms": bench["max_ms"],
                    "std_dev_ms": bench["std_dev_ms"],
                    "ops_per_second": bench["ops_per_second"],
                }
                rows.append(row)
        
        if rows:
            with open(output_file, "w", newline="") as f:
                writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                writer.writeheader()
                writer.writerows(rows)
        
        console.print(f"[green]✓ CSV export saved to {output_file}[/green]")
    
    def _build_html(self) -> str:
        """Build HTML report content."""
        # Group benchmarks by category
        categories: Dict[str, Dict[str, List[Dict]]] = {}
        for db_name, benchmarks in self.results.items():
            for bench in benchmarks:
                cat = bench["category"]
                if cat not in categories:
                    categories[cat] = {}
                if db_name not in categories[cat]:
                    categories[cat][db_name] = []
                categories[cat][db_name].append(bench)
        
        # Build comparison tables
        tables_html = ""
        for category, db_results in categories.items():
            tables_html += self._build_category_table(category, db_results)
        
        # Build charts data (JSON for JavaScript)
        charts_data = self._build_charts_data(categories)
        
        return f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Benchmark Report - {self.run_id}</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {{
            --primary-color: #2563eb;
            --secondary-color: #64748b;
            --success-color: #22c55e;
            --warning-color: #f59e0b;
            --danger-color: #ef4444;
            --bg-color: #f8fafc;
            --card-bg: #ffffff;
            --text-color: #1e293b;
        }}
        
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            line-height: 1.6;
        }}
        
        .container {{
            max-width: 1400px;
            margin: 0 auto;
            padding: 2rem;
        }}
        
        header {{
            background: linear-gradient(135deg, var(--primary-color), #1d4ed8);
            color: white;
            padding: 3rem 2rem;
            margin-bottom: 2rem;
            border-radius: 0 0 1rem 1rem;
        }}
        
        header h1 {{
            font-size: 2.5rem;
            margin-bottom: 0.5rem;
        }}
        
        header .subtitle {{
            opacity: 0.9;
            font-size: 1.1rem;
        }}
        
        .config-summary {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 1rem;
            margin-top: 1.5rem;
        }}
        
        .config-item {{
            background: rgba(255,255,255,0.1);
            padding: 0.75rem 1rem;
            border-radius: 0.5rem;
        }}
        
        .config-item .label {{
            font-size: 0.85rem;
            opacity: 0.8;
        }}
        
        .config-item .value {{
            font-size: 1.25rem;
            font-weight: 600;
        }}
        
        .card {{
            background: var(--card-bg);
            border-radius: 0.75rem;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            padding: 1.5rem;
            margin-bottom: 1.5rem;
        }}
        
        .card h2 {{
            color: var(--primary-color);
            margin-bottom: 1rem;
            padding-bottom: 0.5rem;
            border-bottom: 2px solid var(--bg-color);
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 1rem;
        }}
        
        th, td {{
            padding: 0.75rem 1rem;
            text-align: left;
            border-bottom: 1px solid #e2e8f0;
        }}
        
        th {{
            background-color: var(--bg-color);
            font-weight: 600;
            color: var(--secondary-color);
        }}
        
        tr:hover {{
            background-color: #f1f5f9;
        }}
        
        .metric {{
            display: flex;
            flex-direction: column;
        }}
        
        .metric .primary {{
            font-weight: 600;
            font-size: 1rem;
        }}
        
        .metric .secondary {{
            font-size: 0.8rem;
            color: var(--secondary-color);
        }}
        
        .best {{
            background-color: #dcfce7 !important;
        }}
        
        .chart-container {{
            position: relative;
            height: 400px;
            margin-top: 1rem;
        }}
        
        .grid-2 {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(600px, 1fr));
            gap: 1.5rem;
        }}
        
        footer {{
            text-align: center;
            padding: 2rem;
            color: var(--secondary-color);
            font-size: 0.9rem;
        }}
        
        @media (max-width: 768px) {{
            .grid-2 {{
                grid-template-columns: 1fr;
            }}
            
            header h1 {{
                font-size: 1.75rem;
            }}
            
            .config-summary {{
                grid-template-columns: repeat(2, 1fr);
            }}
        }}
    </style>
</head>
<body>
    <header>
        <div class="container">
            <h1>ThemisDB Benchmark Report</h1>
            <p class="subtitle">Comparative Performance Analysis</p>
            <div class="config-summary">
                <div class="config-item">
                    <div class="label">Run ID</div>
                    <div class="value">{self.run_id}</div>
                </div>
                <div class="config-item">
                    <div class="label">Dataset Size</div>
                    <div class="value">{self.config.get('dataset_size', 'N/A'):,}</div>
                </div>
                <div class="config-item">
                    <div class="label">Iterations</div>
                    <div class="value">{self.config.get('iterations', 'N/A'):,}</div>
                </div>
                <div class="config-item">
                    <div class="label">Databases</div>
                    <div class="value">{len(self.databases)}</div>
                </div>
            </div>
        </div>
    </header>
    
    <div class="container">
        {tables_html}
        
        <div class="card">
            <h2>Performance Comparison Charts</h2>
            <div class="grid-2">
                <div class="chart-container">
                    <canvas id="latencyChart"></canvas>
                </div>
                <div class="chart-container">
                    <canvas id="throughputChart"></canvas>
                </div>
            </div>
        </div>
    </div>
    
    <footer>
        <p>Generated by ThemisDB Comparative Benchmark Suite</p>
        <p>Timestamp: {self.timestamp}</p>
    </footer>
    
    <script>
        const chartsData = {json.dumps(charts_data)};
        
        // Latency comparison chart
        new Chart(document.getElementById('latencyChart'), {{
            type: 'bar',
            data: {{
                labels: chartsData.benchmarks,
                datasets: chartsData.latency_datasets
            }},
            options: {{
                responsive: true,
                maintainAspectRatio: false,
                plugins: {{
                    title: {{
                        display: true,
                        text: 'Mean Latency (ms) - Lower is Better'
                    }},
                    legend: {{
                        position: 'bottom'
                    }}
                }},
                scales: {{
                    y: {{
                        beginAtZero: true,
                        title: {{
                            display: true,
                            text: 'Latency (ms)'
                        }}
                    }}
                }}
            }}
        }});
        
        // Throughput comparison chart
        new Chart(document.getElementById('throughputChart'), {{
            type: 'bar',
            data: {{
                labels: chartsData.benchmarks,
                datasets: chartsData.throughput_datasets
            }},
            options: {{
                responsive: true,
                maintainAspectRatio: false,
                plugins: {{
                    title: {{
                        display: true,
                        text: 'Throughput (ops/s) - Higher is Better'
                    }},
                    legend: {{
                        position: 'bottom'
                    }}
                }},
                scales: {{
                    y: {{
                        beginAtZero: true,
                        title: {{
                            display: true,
                            text: 'Operations/second'
                        }}
                    }}
                }}
            }}
        }});
    </script>
</body>
</html>
"""
    
    def _build_category_table(self, category: str, db_results: Dict[str, List[Dict]]) -> str:
        """Build HTML table for a benchmark category."""
        # Get all benchmark names in this category
        all_benchmarks = set()
        for benchmarks in db_results.values():
            for bench in benchmarks:
                all_benchmarks.add(bench["name"])
        
        # Build header row
        header_cells = "<th>Benchmark</th>"
        for db_name in self.databases:
            if db_name in db_results:
                header_cells += f"<th>{db_name}</th>"
        
        # Build data rows
        rows_html = ""
        for bench_name in sorted(all_benchmarks):
            row_data = {}
            best_latency = float("inf")
            
            # Collect data for each database
            for db_name in self.databases:
                if db_name in db_results:
                    for bench in db_results[db_name]:
                        if bench["name"] == bench_name:
                            row_data[db_name] = bench
                            if bench["mean_ms"] < best_latency:
                                best_latency = bench["mean_ms"]
            
            # Build row
            row_cells = f"<td><strong>{bench_name}</strong></td>"
            for db_name in self.databases:
                if db_name in row_data:
                    bench = row_data[db_name]
                    is_best = bench["mean_ms"] == best_latency
                    cell_class = "best" if is_best else ""
                    row_cells += f"""
                    <td class="{cell_class}">
                        <div class="metric">
                            <span class="primary">{bench['mean_ms']:.3f} ms</span>
                            <span class="secondary">{bench['ops_per_second']:.0f} ops/s</span>
                        </div>
                    </td>
                    """
                elif db_name in db_results:
                    row_cells += "<td>N/A</td>"
            
            rows_html += f"<tr>{row_cells}</tr>"
        
        return f"""
        <div class="card">
            <h2>{category.upper()} Benchmarks</h2>
            <table>
                <thead>
                    <tr>{header_cells}</tr>
                </thead>
                <tbody>
                    {rows_html}
                </tbody>
            </table>
        </div>
        """
    
    def _build_charts_data(self, categories: Dict) -> Dict:
        """Build data structure for Chart.js charts."""
        colors = [
            "rgba(37, 99, 235, 0.8)",   # Blue
            "rgba(34, 197, 94, 0.8)",   # Green
            "rgba(245, 158, 11, 0.8)",  # Yellow
            "rgba(239, 68, 68, 0.8)",   # Red
            "rgba(168, 85, 247, 0.8)",  # Purple
            "rgba(6, 182, 212, 0.8)",   # Cyan
        ]
        
        # Collect all unique benchmark names
        all_benchmarks = []
        benchmark_data: Dict[str, Dict[str, Dict]] = {}
        
        for category, db_results in categories.items():
            for db_name, benchmarks in db_results.items():
                for bench in benchmarks:
                    name = bench["name"]
                    if name not in benchmark_data:
                        benchmark_data[name] = {}
                        all_benchmarks.append(name)
                    benchmark_data[name][db_name] = bench
        
        # Build datasets for latency chart
        latency_datasets = []
        throughput_datasets = []
        
        for i, db_name in enumerate(self.databases):
            color = colors[i % len(colors)]
            
            latency_data = []
            throughput_data = []
            
            for bench_name in all_benchmarks:
                if db_name in benchmark_data.get(bench_name, {}):
                    latency_data.append(benchmark_data[bench_name][db_name]["mean_ms"])
                    throughput_data.append(benchmark_data[bench_name][db_name]["ops_per_second"])
                else:
                    latency_data.append(None)
                    throughput_data.append(None)
            
            latency_datasets.append({
                "label": db_name,
                "data": latency_data,
                "backgroundColor": color,
            })
            
            throughput_datasets.append({
                "label": db_name,
                "data": throughput_data,
                "backgroundColor": color,
            })
        
        return {
            "benchmarks": all_benchmarks,
            "latency_datasets": latency_datasets,
            "throughput_datasets": throughput_datasets,
        }
    
    def _build_markdown(self) -> str:
        """Build Markdown report content."""
        md = f"""# ThemisDB Comparative Benchmark Report

**Run ID:** {self.run_id}  
**Timestamp:** {self.timestamp}

## Configuration

| Parameter | Value |
|-----------|-------|
| Dataset Size | {self.config.get('dataset_size', 'N/A'):,} |
| Iterations | {self.config.get('iterations', 'N/A'):,} |
| Warmup Iterations | {self.config.get('warmup_iterations', 'N/A'):,} |
| Vector Dimensions | {self.config.get('vector_dimensions', 'N/A')} |
| K-Nearest | {self.config.get('k_nearest', 'N/A')} |
| Graph Depth | {self.config.get('graph_depth', 'N/A')} |

## Databases Tested

{', '.join(f'**{db}**' for db in self.databases)}

## Results

"""
        
        # Group by category
        categories: Dict[str, Dict[str, List[Dict]]] = {}
        for db_name, benchmarks in self.results.items():
            for bench in benchmarks:
                cat = bench["category"]
                if cat not in categories:
                    categories[cat] = {}
                if db_name not in categories[cat]:
                    categories[cat][db_name] = []
                categories[cat][db_name].append(bench)
        
        for category, db_results in categories.items():
            md += f"### {category.upper()} Benchmarks\n\n"
            
            # Get all benchmark names
            all_benchmarks = set()
            for benchmarks in db_results.values():
                for bench in benchmarks:
                    all_benchmarks.add(bench["name"])
            
            # Build table header
            header = "| Benchmark |"
            separator = "|-----------|"
            for db_name in self.databases:
                if db_name in db_results:
                    header += f" {db_name} |"
                    separator += "-----------|"
            
            md += header + "\n" + separator + "\n"
            
            # Build rows
            for bench_name in sorted(all_benchmarks):
                row = f"| {bench_name} |"
                for db_name in self.databases:
                    if db_name in db_results:
                        found = False
                        for bench in db_results[db_name]:
                            if bench["name"] == bench_name:
                                row += f" {bench['mean_ms']:.3f}ms ({bench['ops_per_second']:.0f} ops/s) |"
                                found = True
                                break
                        if not found:
                            row += " N/A |"
                md += row + "\n"
            
            md += "\n"
        
        md += """
## Interpretation Guide

- **Lower latency** indicates better performance for response time
- **Higher throughput** indicates better performance for concurrent workloads
- Results marked with ✓ indicate the best performer in each category

## Notes

- All tests use standardized Hugging Face datasets for reproducibility
- Databases configured with equivalent resource limits (CPU, memory)
- Results measured after warmup phase to ensure consistent cache state

---
*Generated by ThemisDB Comparative Benchmark Suite*
"""
        
        return md


def load_results(path: str) -> Dict[str, Any]:
    """Load benchmark results from JSON file(s)."""
    path_obj = Path(path)
    
    if path_obj.is_file():
        with open(path_obj) as f:
            return json.load(f)
    elif path_obj.is_dir():
        # Find most recent results file
        json_files = list(path_obj.glob("benchmark_results_*.json"))
        if not json_files:
            raise FileNotFoundError(f"No benchmark results found in {path}")
        
        latest = max(json_files, key=lambda p: p.stat().st_mtime)
        console.print(f"Loading results from {latest}")
        
        with open(latest) as f:
            return json.load(f)
    else:
        raise FileNotFoundError(f"Path not found: {path}")


@click.command()
@click.option("--format", "output_format", 
              type=click.Choice(["html", "markdown", "csv", "all"]),
              default="all",
              help="Output format for the report")
@click.option("--input", "input_path", 
              default="results",
              type=click.Path(exists=True),
              help="Input path (directory or JSON file)")
@click.option("--output", "output_path",
              default="reports",
              type=click.Path(),
              help="Output directory for reports")
def main(output_format: str, input_path: str, output_path: str):
    """
    Generate reports from benchmark results.
    
    Supports HTML, Markdown, and CSV output formats.
    """
    console.print("[bold blue]ThemisDB Benchmark Report Generator[/bold blue]")
    
    # Load results
    try:
        results = load_results(input_path)
    except FileNotFoundError as e:
        console.print(f"[red]Error: {e}[/red]")
        sys.exit(1)
    
    # Create output directory
    output_dir = Path(output_path)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Generate reports
    generator = BenchmarkReportGenerator(results)
    
    if output_format in ("html", "all"):
        generator.generate_html(output_dir)
    
    if output_format in ("markdown", "all"):
        generator.generate_markdown(output_dir)
    
    if output_format in ("csv", "all"):
        generator.generate_csv(output_dir)
    
    console.print("\n[bold green]Report generation complete![/bold green]")


if __name__ == "__main__":
    main()
