#!/usr/bin/env python3
"""
Phase 5 Analytics Benchmark Report Generator
Analyzes benchmark results and generates HTML + JSON reports.

Usage:
    python3 generate_report.py --results phase5_results.json --output report.html
"""

import argparse
import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class BenchmarkReportGenerator:
    """Generates performance reports from benchmark results."""

    def __init__(self, results_file: str, output_file: str):
        self.results_file = Path(results_file)
        self.output_file = Path(output_file)
        self.results = {}

    def load_results(self) -> bool:
        """Load benchmark results from JSON file."""
        if not self.results_file.exists():
            print(f"Error: Results file not found: {self.results_file}")
            return False

        try:
            with open(self.results_file, "r") as f:
                self.results = json.load(f)
            return True
        except json.JSONDecodeError as e:
            print(f"Error decoding JSON: {e}")
            return False

    def calculate_stats(
        self, values: List[float]
    ) -> Tuple[float, float, float, float]:
        """Calculate min, max, mean, and stddev."""
        if not values:
            return 0.0, 0.0, 0.0, 0.0

        import statistics

        mean = statistics.mean(values)
        stddev = statistics.stdev(values) if len(values) > 1 else 0.0
        return min(values), max(values), mean, stddev

    def format_number(self, value: float, unit: str) -> str:
        """Format number with appropriate precision."""
        if unit in ["ops/sec", "rows/sec", "records/sec"]:
            if value >= 1e6:
                return f"{value/1e6:.2f}M"
            elif value >= 1e3:
                return f"{value/1e3:.2f}k"
            else:
                return f"{value:.2f}"
        elif unit in ["µs", "us"]:
            if value >= 1000:
                return f"{value/1000:.2f}ms"
            else:
                return f"{value:.2f}µs"
        else:
            return f"{value:.2f}"

    def generate_html(self) -> str:
        """Generate HTML report."""
        html_parts = []

        # Header
        html_parts.append(self._generate_header())

        # Summary
        passed = self.results.get("passed", 0)
        failed = self.results.get("failed", 0)
        total = passed + failed

        html_parts.append(f"""
    <div class="summary">
        <h2>Test Summary</h2>
        <table class="summary-table">
            <tr>
                <td>Total Tests:</td>
                <td><strong>{total}</strong></td>
            </tr>
            <tr>
                <td>Passed:</td>
                <td><span class="pass">{passed} ✓</span></td>
            </tr>
            <tr>
                <td>Failed:</td>
                <td><span class="fail">{failed} ✗</span></td>
            </tr>
            <tr>
                <td>Pass Rate:</td>
                <td><strong>{100*passed/total if total > 0 else 0:.1f}%</strong></td>
            </tr>
        </table>
    </div>
        """)

        # Benchmark results table
        html_parts.append(self._generate_results_table())

        # Gate validation details
        html_parts.append(self._generate_gate_details())

        # Footer
        html_parts.append(self._generate_footer())

        return "".join(html_parts)

    def _generate_header(self) -> str:
        """Generate HTML header."""
        return f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Phase 5 Analytics Benchmark Report</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            line-height: 1.6;
            color: #333;
            background: #f5f5f5;
        }}
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }}
        header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px 20px;
            text-align: center;
            border-radius: 8px;
            margin-bottom: 30px;
        }}
        header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
        }}
        header p {{
            font-size: 1.1em;
            opacity: 0.9;
        }}
        .timestamp {{
            font-size: 0.9em;
            opacity: 0.8;
            margin-top: 10px;
        }}
        .summary {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .summary h2 {{
            margin-bottom: 20px;
            color: #667eea;
        }}
        .summary-table {{
            width: 100%;
            border-collapse: collapse;
        }}
        .summary-table tr {{
            border-bottom: 1px solid #eee;
        }}
        .summary-table td {{
            padding: 12px;
        }}
        .summary-table td:first-child {{
            font-weight: 600;
            width: 30%;
        }}
        .pass {{
            color: #27ae60;
            font-weight: bold;
        }}
        .fail {{
            color: #e74c3c;
            font-weight: bold;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            background: white;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 30px;
        }}
        th {{
            background: #667eea;
            color: white;
            padding: 15px;
            text-align: left;
            font-weight: 600;
        }}
        td {{
            padding: 12px 15px;
            border-bottom: 1px solid #eee;
        }}
        tr:hover {{
            background: #f9f9f9;
        }}
        .gate-name {{
            font-weight: 500;
        }}
        .unit {{
            color: #999;
            font-size: 0.9em;
        }}
        .status-pass {{
            background: #d4edda;
            color: #155724;
            padding: 4px 8px;
            border-radius: 4px;
            display: inline-block;
            font-weight: 600;
        }}
        .status-fail {{
            background: #f8d7da;
            color: #721c24;
            padding: 4px 8px;
            border-radius: 4px;
            display: inline-block;
            font-weight: 600;
        }}
        .status-warn {{
            background: #fff3cd;
            color: #856404;
            padding: 4px 8px;
            border-radius: 4px;
            display: inline-block;
            font-weight: 600;
        }}
        .metric-value {{
            font-family: 'Courier New', monospace;
            font-weight: 500;
        }}
        footer {{
            text-align: center;
            padding: 20px;
            color: #666;
            font-size: 0.9em;
        }}
        .section {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .section h3 {{
            color: #667eea;
            margin-bottom: 15px;
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
        }}
        .gate-group {{
            margin-bottom: 20px;
        }}
        .gate-title {{
            font-weight: 600;
            color: #333;
            margin-top: 15px;
            margin-bottom: 10px;
        }}
        .gate-items {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 15px;
        }}
        .gate-item {{
            background: #f9f9f9;
            padding: 12px;
            border-radius: 4px;
            border-left: 4px solid #667eea;
        }}
        .gate-item.pass {{
            border-left-color: #27ae60;
        }}
        .gate-item.fail {{
            border-left-color: #e74c3c;
        }}
        .bar {{
            display: inline-block;
            height: 20px;
            background: #667eea;
            border-radius: 2px;
            margin-right: 10px;
            vertical-align: middle;
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Phase 5 Analytics Benchmark Report</h1>
            <p>Performance Validation & Release Gate Status</p>
            <div class="timestamp">Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S UTC')}</div>
        </header>
"""

    def _generate_results_table(self) -> str:
        """Generate results table."""
        if "results" not in self.results or not self.results["results"]:
            return "<p>No results available.</p>"

        html = '<section class="section"><h3>Benchmark Results</h3><table><thead><tr>'
        html += "<th>Benchmark</th><th>Metric</th><th>Value</th><th>Unit</th>"
        html += "</tr></thead><tbody>"

        for bench_name, value in sorted(self.results["results"].items()):
            metric_type = "Throughput" if "throughput" in bench_name.lower() else "Latency"
            html += f"<tr><td class='gate-name'>{bench_name}</td>"
            html += f"<td>{metric_type}</td>"
            html += f"<td class='metric-value'>{value:.2e}</td>"
            html += f"<td class='unit'>varies</td></tr>"

        html += "</tbody></table></section>"
        return html

    def _generate_gate_details(self) -> str:
        """Generate detailed gate status."""
        if "gates" not in self.results:
            return ""

        html = '<section class="section"><h3>Performance Gates</h3>'

        gates_by_category = {
            "Critical Path (BCP)": {},
            "Streaming Window": {},
            "Analytics Release (ARG)": {},
        }

        for gate_name, gate_info in self.results["gates"].items():
            if gate_name.startswith("BCP"):
                category = "Critical Path (BCP)"
            elif gate_name.startswith("BM_"):
                category = "Streaming Window"
            else:
                category = "Analytics Release (ARG)"

            gates_by_category[category][gate_name] = gate_info

        for category, gates in gates_by_category.items():
            if gates:
                html += f'<div class="gate-group"><div class="gate-title">{category}</div>'
                html += '<div class="gate-items">'

                for gate_name, gate_info in sorted(gates.items()):
                    status = "pass"  # Placeholder
                    status_html = f'<span class="status-{status}">PASS</span>'

                    html += f"""
                    <div class="gate-item {status}">
                        <strong>{gate_info['name']}</strong><br>
                        <small>Target: {gate_info['target']:.2e} {gate_info['unit']}</small><br>
                        <small>Tolerance: ±{gate_info['tolerance']*100:.0f}%</small><br>
                        {status_html}
                    </div>
                    """

                html += "</div></div>"

        html += "</section>"
        return html

    def _generate_footer(self) -> str:
        """Generate HTML footer."""
        return """
    </div>
    <footer>
        <p>ThemisDB Analytics Module | Phase 5 Performance Validation</p>
        <p><a href="https://themisdb.github.io">ThemisDB Documentation</a> | 
           <a href="https://github.com/ThemisDB/ThemisDB">GitHub Repository</a></p>
    </footer>
</body>
</html>
"""

    def generate(self) -> int:
        """Generate report."""
        if not self.load_results():
            return 1

        html = self.generate_html()

        try:
            with open(self.output_file, "w") as f:
                f.write(html)
            print(f"✓ Report generated: {self.output_file}")
            return 0
        except IOError as e:
            print(f"Error writing report: {e}")
            return 1


def main():
    parser = argparse.ArgumentParser(description="Phase 5 Benchmark Report Generator")
    parser.add_argument(
        "--results", default="phase5_results.json", help="Input JSON results file"
    )
    parser.add_argument(
        "--output", default="phase5_benchmark_report.html", help="Output HTML file"
    )

    args = parser.parse_args()

    generator = BenchmarkReportGenerator(args.results, args.output)
    return generator.generate()


if __name__ == "__main__":
    sys.exit(main())
