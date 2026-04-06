"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reporter.py                                        ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     519                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • edfcc1a70a  2026-02-28  feat(chimera): implement BenchmarkDashboard for result ag... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
CHIMERA Reporter - Main reporting engine

Combines statistics, colors, and citations to generate comprehensive,
vendor-neutral benchmark reports.
"""

import json
import csv
from typing import Dict, List, Any, Optional
from pathlib import Path
from datetime import datetime
from dataclasses import dataclass, asdict
import base64
from io import BytesIO

from .statistics import StatisticalAnalyzer, DescriptiveStats
from .colors import ColorBlindPalette, VendorNeutralStyle
from .citations import CitationManager


@dataclass
class SystemResults:
    """Container for system benchmark results"""
    name: str
    metric_name: str
    metric_unit: str
    raw_data: List[float]
    metadata: Dict[str, Any] = None
    
    def __post_init__(self):
        if self.metadata is None:
            self.metadata = {}


class ChimeraReporter:
    """
    Main reporting engine for vendor-neutral benchmark reports.
    
    Generates HTML, PDF, and CSV reports with:
    - Statistical analysis
    - Color-blind friendly visualizations
    - IEEE citations
    - Neutrality guarantees
    """
    
    def __init__(self, significance_level: float = 0.05):
        """
        Initialize CHIMERA reporter.
        
        Args:
            significance_level: Alpha level for statistical tests
        """
        self.systems: Dict[str, SystemResults] = {}
        self.analyzer = StatisticalAnalyzer(significance_level)
        self.citation_manager = CitationManager()
        self.color_palette = ColorBlindPalette()
        self.report_metadata = {
            'generated_at': datetime.now().isoformat(),
            'chimera_version': '1.0.0',
            'neutrality_certified': True
        }
    
    def add_system_results(self, system_name: str, metric_name: str, 
                          metric_unit: str, data: List[float],
                          metadata: Dict[str, Any] = None):
        """
        Add benchmark results for a system.
        
        Args:
            system_name: Name of the system (will be normalized)
            metric_name: Name of the metric being measured
            metric_unit: Unit of measurement
            data: Raw benchmark data
            metadata: Optional metadata
        """
        # Normalize system name (remove vendor indicators)
        normalized_name = self._normalize_system_name(system_name)
        
        self.systems[normalized_name] = SystemResults(
            name=normalized_name,
            metric_name=metric_name,
            metric_unit=metric_unit,
            raw_data=data,
            metadata=metadata or {}
        )
    
    def _normalize_system_name(self, name: str) -> str:
        """
        Normalize system names to be vendor-neutral.
        
        Args:
            name: Original system name
            
        Returns:
            Normalized name
        """
        # Remove common marketing terms
        replacements = {
            ' enterprise': '',
            ' professional': '',
            ' community': '',
            ' edition': '',
            '™': '',
            '®': '',
            '©': ''
        }
        
        normalized = name
        for term, replacement in replacements.items():
            normalized = normalized.replace(term, replacement)
            normalized = normalized.replace(term.title(), replacement)
            normalized = normalized.replace(term.upper(), replacement)
        
        return normalized.strip()
    
    def _sort_systems(self, sort_by: str = 'alphabetical') -> List[str]:
        """
        Sort systems in a vendor-neutral manner.
        
        Args:
            sort_by: 'alphabetical' or 'metric' (mean value)
            
        Returns:
            Sorted list of system names
        """
        if sort_by == 'alphabetical':
            return sorted(self.systems.keys())
        elif sort_by == 'metric':
            # Sort by mean value (descending for higher-is-better metrics)
            stats = {name: self.analyzer.descriptive_statistics(results.raw_data)
                    for name, results in self.systems.items()}
            return sorted(stats.keys(), key=lambda x: stats[x].mean, reverse=True)
        else:
            raise ValueError(f"Invalid sort_by: {sort_by}")
    
    def generate_csv_report(self, output_path: str, sort_by: str = 'alphabetical'):
        """
        Generate CSV report with statistical summary.
        
        Args:
            output_path: Path to save CSV file
            sort_by: How to sort systems ('alphabetical' or 'metric')
        """
        sorted_systems = self._sort_systems(sort_by)
        
        # Prepare data
        rows = []
        for system_name in sorted_systems:
            results = self.systems[system_name]
            stats = self.analyzer.descriptive_statistics(results.raw_data)
            ci = self.analyzer.confidence_interval(results.raw_data)
            
            row = {
                'System': system_name,
                'Metric': results.metric_name,
                'Unit': results.metric_unit,
                'N Samples': stats.n_samples,
                'Mean': f"{stats.mean:.4f}",
                'Median': f"{stats.median:.4f}",
                'Std Dev': f"{stats.std_dev:.4f}",
                'Min': f"{stats.min_value:.4f}",
                'Max': f"{stats.max_value:.4f}",
                'P25': f"{stats.p25:.4f}",
                'P75': f"{stats.p75:.4f}",
                'P95': f"{stats.p95:.4f}",
                'P99': f"{stats.p99:.4f}",
                'CI 95% Lower': f"{ci[0]:.4f}",
                'CI 95% Upper': f"{ci[1]:.4f}",
                'Outliers Removed': stats.outliers_removed
            }
            rows.append(row)
        
        # Write CSV
        with open(output_path, 'w', newline='', encoding='utf-8') as f:
            if rows:
                writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                writer.writeheader()
                writer.writerows(rows)
        
        print(f"✓ CSV report saved to: {output_path}")
    
    def generate_html_report(self, output_path: str, sort_by: str = 'alphabetical',
                           include_plots: bool = True):
        """
        Generate comprehensive HTML report.
        
        Args:
            output_path: Path to save HTML file
            sort_by: How to sort systems ('alphabetical' or 'metric')
            include_plots: Whether to include visualization plots
        """
        sorted_systems = self._sort_systems(sort_by)
        
        # Generate report sections
        html_parts = [
            self._html_header(),
            self._html_neutrality_seal(),
            self._html_executive_summary(sorted_systems),
            self._html_statistical_analysis(sorted_systems),
        ]
        
        if include_plots:
            html_parts.append(self._html_visualizations(sorted_systems))
        
        html_parts.extend([
            self._html_methodology(),
            self._html_citations(),
            self._html_footer()
        ])
        
        # Combine and save
        html_content = "\n".join(html_parts)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html_content)
        
        print(f"✓ HTML report saved to: {output_path}")
    
    def _html_header(self) -> str:
        """Generate HTML header"""
        css = VendorNeutralStyle.get_css_theme()
        
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CHIMERA Benchmark Report</title>
    <style>
        {css}
        
        body {{
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }}
        
        h1 {{
            color: var(--color-primary);
            border-bottom: 3px solid var(--color-primary);
            padding-bottom: 10px;
        }}
        
        h2 {{
            color: var(--color-secondary);
            margin-top: 30px;
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }}
        
        th, td {{
            text-align: left;
            padding: 12px;
            border-bottom: 1px solid #ddd;
        }}
        
        th {{
            background-color: var(--color-primary);
            color: white;
        }}
        
        tr:hover {{
            background-color: #f5f5f5;
        }}
        
        .metric-card {{
            background-color: #f8f9fa;
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
            border-left: 4px solid var(--color-info);
        }}
        
        .significant {{
            background-color: #fff3cd;
            padding: 2px 6px;
            border-radius: 3px;
            font-weight: bold;
        }}
        
        .plot {{
            margin: 20px 0;
            text-align: center;
        }}
        
        footer {{
            margin-top: 40px;
            padding-top: 20px;
            border-top: 2px solid #ddd;
            color: #666;
            font-size: 0.9em;
        }}
    </style>
</head>
<body>
    <header>
        <h1>🔬 CHIMERA Benchmark Report</h1>
        <p><strong>Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis</strong></p>
        <p>Generated: {self.report_metadata['generated_at']}</p>
    </header>
"""
    
    def _html_neutrality_seal(self) -> str:
        """Generate neutrality seal section"""
        return """
    <section class="neutrality-seal">
        <h3>✓ Vendor Neutrality Certified</h3>
        <p>This report adheres to strict vendor-neutrality standards:</p>
        <ul>
            <li>✓ Color-blind friendly visualizations (Okabe-Ito palette)</li>
            <li>✓ Alphabetical or metric-based sorting only</li>
            <li>✓ No vendor logos or brand colors</li>
            <li>✓ Transparent statistical methodology</li>
            <li>✓ IEEE-compliant citations</li>
        </ul>
    </section>
"""
    
    def _html_executive_summary(self, sorted_systems: List[str]) -> str:
        """Generate executive summary section"""
        html = ['<section>', '<h2>Executive Summary</h2>']
        
        # Get metric info from first system
        first_system = list(self.systems.values())[0]
        html.append(f'<p><strong>Metric:</strong> {first_system.metric_name} ({first_system.metric_unit})</p>')
        html.append(f'<p><strong>Systems Evaluated:</strong> {len(self.systems)}</p>')
        
        # Summary table
        html.append('<table>')
        html.append('<thead><tr>')
        html.append('<th>System</th><th>Mean</th><th>Median</th><th>Std Dev</th>')
        html.append('<th>95% CI</th><th>N</th>')
        html.append('</tr></thead>')
        html.append('<tbody>')
        
        for system_name in sorted_systems:
            results = self.systems[system_name]
            stats = self.analyzer.descriptive_statistics(results.raw_data)
            ci = self.analyzer.confidence_interval(results.raw_data)
            
            html.append('<tr>')
            html.append(f'<td><strong>{system_name}</strong></td>')
            html.append(f'<td>{stats.mean:.4f}</td>')
            html.append(f'<td>{stats.median:.4f}</td>')
            html.append(f'<td>{stats.std_dev:.4f}</td>')
            html.append(f'<td>[{ci[0]:.4f}, {ci[1]:.4f}]</td>')
            html.append(f'<td>{stats.n_samples}</td>')
            html.append('</tr>')
        
        html.append('</tbody></table>')
        html.append('</section>')
        
        return '\n'.join(html)
    
    def _html_statistical_analysis(self, sorted_systems: List[str]) -> str:
        """Generate statistical analysis section"""
        html = ['<section>', '<h2>Statistical Analysis</h2>']
        
        # Pairwise comparisons
        if len(sorted_systems) >= 2:
            html.append('<h3>Pairwise Comparisons</h3>')
            html.append('<p class="statistical-note">⚠️ Statistical significance indicates a measurable difference, not superiority. Consider effect sizes and practical significance.</p>')
            
            for i in range(len(sorted_systems)):
                for j in range(i + 1, len(sorted_systems)):
                    sys_a = sorted_systems[i]
                    sys_b = sorted_systems[j]
                    
                    data_a = self.systems[sys_a].raw_data
                    data_b = self.systems[sys_b].raw_data
                    
                    comparison = self.analyzer.compare_systems(data_a, data_b, sys_a, sys_b)
                    
                    html.append('<div class="metric-card">')
                    html.append(f'<h4>{sys_a} vs {sys_b}</h4>')
                    
                    # T-test
                    t_test = comparison['tests']['t_test']
                    sig_marker = '<span class="significant">✓ Significant</span>' if t_test.is_significant else '○ Not Significant'
                    html.append(f'<p><strong>Welch\'s t-test:</strong> {sig_marker}</p>')
                    html.append(f'<p>{t_test.interpretation}</p>')
                    
                    # Mann-Whitney
                    mw_test = comparison['tests']['mann_whitney_u']
                    sig_marker = '<span class="significant">✓ Significant</span>' if mw_test.is_significant else '○ Not Significant'
                    html.append(f'<p><strong>Mann-Whitney U test:</strong> {sig_marker}</p>')
                    html.append(f'<p>{mw_test.interpretation}</p>')
                    
                    html.append('</div>')
        
        html.append('</section>')
        return '\n'.join(html)
    
    def _html_visualizations(self, sorted_systems: List[str]) -> str:
        """Generate visualizations section"""
        html = ['<section>', '<h2>Visualizations</h2>']
        html.append('<p><em>All plots use color-blind friendly palettes (Okabe-Ito/Tol schemes)</em></p>')
        
        try:
            import matplotlib
            matplotlib.use('Agg')  # Non-interactive backend
            import matplotlib.pyplot as plt
            
            # Box plot
            fig, ax = plt.subplots(figsize=(10, 6))
            data = [self.systems[name].raw_data for name in sorted_systems]
            colors = self.color_palette.get_matplotlib_colors(len(sorted_systems), 'tol_muted')
            
            bp = ax.boxplot(data, labels=sorted_systems, patch_artist=True)
            for patch, color in zip(bp['boxes'], colors):
                patch.set_facecolor(color)
            
            ax.set_ylabel(f"{list(self.systems.values())[0].metric_name} ({list(self.systems.values())[0].metric_unit})")
            ax.set_title("Distribution Comparison (Box Plot)")
            ax.grid(True, alpha=0.3)
            plt.xticks(rotation=45, ha='right')
            plt.tight_layout()
            
            # Convert to base64
            buf = BytesIO()
            plt.savefig(buf, format='png', dpi=150, bbox_inches='tight')
            buf.seek(0)
            img_base64 = base64.b64encode(buf.read()).decode('utf-8')
            plt.close()
            
            html.append(f'<div class="plot">')
            html.append(f'<img src="data:image/png;base64,{img_base64}" alt="Box Plot" style="max-width: 100%;">')
            html.append('</div>')
            
        except ImportError:
            html.append('<p><em>Matplotlib not available. Install with: pip install matplotlib</em></p>')
        
        html.append('</section>')
        return '\n'.join(html)
    
    def _html_methodology(self) -> str:
        """Generate methodology section"""
        statement = self.citation_manager.get_neutrality_statement()
        return f'<section>{statement}</section>'
    
    def _html_citations(self) -> str:
        """Generate citations section"""
        methods = self.citation_manager.get_method_citations()
        citation_ids = list(set(methods.values()))
        bibliography = self.citation_manager.format_bibliography(citation_ids)
        
        return f'<section>{bibliography}</section>'
    
    def _html_footer(self) -> str:
        """Generate HTML footer"""
        return f"""
    <footer>
        <p><strong>CHIMERA v{self.report_metadata['chimera_version']}</strong></p>
        <p>Generated: {self.report_metadata['generated_at']}</p>
        <p>© {datetime.now().year} CHIMERA Project. Released under MIT License.</p>
    </footer>
</body>
</html>
"""
    
    def generate_pdf_report(self, output_path: str, sort_by: str = 'alphabetical'):
        """
        Generate PDF report (via HTML rendering).
        
        Note: Requires weasyprint or similar HTML-to-PDF library.
        
        Args:
            output_path: Path to save PDF file
            sort_by: How to sort systems
        """
        try:
            from weasyprint import HTML
            
            # Generate HTML first
            temp_html = output_path.replace('.pdf', '.tmp.html')
            self.generate_html_report(temp_html, sort_by)
            
            # Convert to PDF
            HTML(temp_html).write_pdf(output_path)
            
            # Clean up temp file
            Path(temp_html).unlink()
            
            print(f"✓ PDF report saved to: {output_path}")
            
        except ImportError:
            print("⚠️ PDF generation requires weasyprint: pip install weasyprint")
            print("   Falling back to HTML report...")
            html_path = output_path.replace('.pdf', '.html')
            self.generate_html_report(html_path, sort_by)
