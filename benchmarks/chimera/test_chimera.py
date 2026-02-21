"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_chimera.py                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Test suite for CHIMERA vendor-neutral reporting framework.

Tests cover:
- Statistical analysis correctness
- Color-blind palette compliance
- Vendor neutrality guarantees
- Report generation
- IEEE citation formatting
"""

import pytest
import numpy as np
from pathlib import Path
import sys

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from chimera import ChimeraReporter, StatisticalAnalyzer, ColorBlindPalette, CitationManager
from chimera.statistics import DescriptiveStats, StatisticalResult


class TestStatisticalAnalyzer:
    """Test statistical analysis functions"""
    
    def test_descriptive_statistics(self):
        """Test descriptive statistics calculation"""
        analyzer = StatisticalAnalyzer()
        data = [10, 12, 11, 13, 10, 12, 11, 14, 10, 13]
        
        stats = analyzer.descriptive_statistics(data, remove_outliers=False)
        
        assert stats.n_samples == 10
        assert abs(stats.mean - 11.6) < 0.1
        assert abs(stats.median - 11.5) < 0.5
        assert stats.min_value == 10
        assert stats.max_value == 14
    
    def test_outlier_removal(self):
        """Test IQR outlier removal"""
        analyzer = StatisticalAnalyzer()
        
        # Data with clear outliers
        data = [10, 11, 12, 11, 10, 12, 11, 100, 10, 11]  # 100 is outlier
        
        stats = analyzer.descriptive_statistics(data, remove_outliers=True)
        
        assert stats.outliers_removed > 0
        assert stats.n_samples < len(data)
        assert stats.mean < 20  # Outlier removed
    
    def test_t_test(self):
        """Test Welch's t-test"""
        analyzer = StatisticalAnalyzer()
        
        # Two clearly different samples
        data1 = [10, 11, 12, 11, 10] * 6  # Mean ~10.8
        data2 = [20, 21, 22, 21, 20] * 6  # Mean ~20.8
        
        result = analyzer.t_test(data1, data2)
        
        assert result.test_name == "Welch's t-test"
        assert result.p_value < 0.05  # Should be significant
        assert result.is_significant == True  # Convert np.bool_ to bool
        assert result.effect_size is not None
    
    def test_mann_whitney_u(self):
        """Test Mann-Whitney U test"""
        analyzer = StatisticalAnalyzer()
        
        # Two different distributions
        data1 = [5, 6, 7, 6, 5, 7, 6] * 5
        data2 = [15, 16, 17, 16, 15, 17, 16] * 5
        
        result = analyzer.mann_whitney_u(data1, data2)
        
        assert result.test_name == "Mann-Whitney U test"
        assert result.p_value < 0.05
        assert result.is_significant == True  # Convert np.bool_ to bool
    
    def test_cohens_d(self):
        """Test Cohen's d effect size"""
        analyzer = StatisticalAnalyzer()
        
        # Large effect size
        data1 = [10, 11, 12, 11, 10]
        data2 = [20, 21, 22, 21, 20]
        
        d = analyzer.cohens_d(data1, data2)
        
        assert abs(d) > 0.8  # Should be large effect
    
    def test_confidence_interval(self):
        """Test confidence interval calculation"""
        analyzer = StatisticalAnalyzer()
        
        data = [10, 12, 11, 13, 10, 12, 11, 14, 10, 13]
        
        ci_lower, ci_upper = analyzer.confidence_interval(data, confidence=0.95)
        
        mean = np.mean(data)
        assert ci_lower < mean < ci_upper
        assert ci_upper - ci_lower > 0  # Non-zero width


class TestColorBlindPalette:
    """Test color-blind friendly palettes"""
    
    def test_okabe_ito_palette(self):
        """Test Okabe-Ito palette"""
        palette = ColorBlindPalette.get_palette('okabe_ito')
        
        assert len(palette) > 0
        assert all(color.startswith('#') for color in palette)
        assert len(palette[0]) == 7  # Hex color format
    
    def test_tol_bright_palette(self):
        """Test Tol bright palette"""
        palette = ColorBlindPalette.get_palette('tol_bright')
        
        assert len(palette) > 0
        assert all(color.startswith('#') for color in palette)
    
    def test_sequential_palette(self):
        """Test sequential palette generation"""
        colors = ColorBlindPalette.get_sequential_palette(10, 'tol_muted')
        
        assert len(colors) == 10
        assert all(color.startswith('#') for color in colors)
    
    def test_diverging_palette(self):
        """Test diverging palette"""
        palette = ColorBlindPalette.get_diverging_palette()
        
        assert 'negative' in palette
        assert 'neutral' in palette
        assert 'positive' in palette
        assert all(color.startswith('#') for color in palette.values())
    
    def test_matplotlib_colors(self):
        """Test matplotlib RGB conversion"""
        colors = ColorBlindPalette.get_matplotlib_colors(5, 'tol_muted')
        
        assert len(colors) == 5
        assert all(isinstance(c, tuple) for c in colors)
        assert all(len(c) == 3 for c in colors)
        assert all(0 <= v <= 1 for c in colors for v in c)


class TestCitationManager:
    """Test IEEE citation management"""
    
    def test_standard_citations_loaded(self):
        """Test that standard citations are pre-loaded"""
        manager = CitationManager()
        
        assert 'cohen1988' in manager.citations
        assert 'mann1947' in manager.citations
        assert 'okabe2008' in manager.citations
        assert 'tol2021' in manager.citations
    
    def test_citation_formatting(self):
        """Test IEEE citation formatting"""
        manager = CitationManager()
        
        citation = manager.get_citation('cohen1988')
        formatted = citation.format_ieee()
        
        assert 'Cohen' in formatted
        assert '1988' in formatted
        assert 'Statistical Power Analysis' in formatted
    
    def test_bibliography_generation(self):
        """Test bibliography generation"""
        manager = CitationManager()
        
        bib = manager.format_bibliography(['cohen1988', 'mann1947'])
        
        assert '## References' in bib
        assert 'Cohen' in bib
        assert 'Mann' in bib
    
    def test_neutrality_statement(self):
        """Test neutrality statement generation"""
        manager = CitationManager()
        
        statement = manager.get_neutrality_statement()
        
        assert 'Neutrality Seal' in statement
        assert 'Vendor Neutrality' in statement
        assert 'CHIMERA' in statement


class TestChimeraReporter:
    """Test main reporter functionality"""
    
    def test_add_system_results(self):
        """Test adding system results"""
        reporter = ChimeraReporter()
        
        data = [10, 11, 12, 11, 10]
        reporter.add_system_results(
            system_name="Test System",
            metric_name="Throughput",
            metric_unit="ops/sec",
            data=data
        )
        
        assert "Test System" in reporter.systems
        assert reporter.systems["Test System"].metric_name == "Throughput"
    
    def test_system_name_normalization(self):
        """Test vendor-neutral name normalization"""
        reporter = ChimeraReporter()
        
        # Test that marketing terms are removed
        data = [10, 11, 12]
        reporter.add_system_results(
            system_name="MyDB Enterprise Edition™",
            metric_name="Throughput",
            metric_unit="ops/sec",
            data=data
        )
        
        # Should be normalized
        assert "MyDB" in list(reporter.systems.keys())[0]
        assert "Enterprise" not in list(reporter.systems.keys())[0]
        assert "™" not in list(reporter.systems.keys())[0]
    
    def test_alphabetical_sorting(self):
        """Test alphabetical system sorting"""
        reporter = ChimeraReporter()
        
        reporter.add_system_results("Zeta", "Metric", "unit", [10, 11])
        reporter.add_system_results("Alpha", "Metric", "unit", [12, 13])
        reporter.add_system_results("Beta", "Metric", "unit", [14, 15])
        
        sorted_systems = reporter._sort_systems('alphabetical')
        
        assert sorted_systems == ['Alpha', 'Beta', 'Zeta']
    
    def test_metric_sorting(self):
        """Test metric-based system sorting"""
        reporter = ChimeraReporter()
        
        reporter.add_system_results("Low", "Metric", "unit", [5, 6, 5])
        reporter.add_system_results("High", "Metric", "unit", [20, 21, 20])
        reporter.add_system_results("Medium", "Metric", "unit", [10, 11, 10])
        
        sorted_systems = reporter._sort_systems('metric')
        
        # Should be sorted by mean (descending)
        assert sorted_systems[0] == "High"
        assert sorted_systems[1] == "Medium"
        assert sorted_systems[2] == "Low"
    
    def test_csv_generation(self, tmp_path):
        """Test CSV report generation"""
        reporter = ChimeraReporter()
        
        reporter.add_system_results("System A", "Throughput", "ops/sec", 
                                   [100, 110, 105, 108, 102])
        reporter.add_system_results("System B", "Throughput", "ops/sec",
                                   [90, 95, 92, 94, 91])
        
        output_path = tmp_path / "test_report.csv"
        reporter.generate_csv_report(str(output_path))
        
        assert output_path.exists()
        content = output_path.read_text()
        assert "System A" in content
        assert "System B" in content
        assert "Mean" in content
        assert "Median" in content
    
    def test_html_generation(self, tmp_path):
        """Test HTML report generation"""
        reporter = ChimeraReporter()
        
        reporter.add_system_results("System A", "Latency", "ms",
                                   [10, 11, 10, 12, 11] * 10)
        reporter.add_system_results("System B", "Latency", "ms",
                                   [15, 16, 15, 17, 16] * 10)
        
        output_path = tmp_path / "test_report.html"
        reporter.generate_html_report(str(output_path), include_plots=False)
        
        assert output_path.exists()
        content = output_path.read_text()
        assert "CHIMERA" in content
        assert "Neutrality" in content
        assert "System A" in content
        assert "System B" in content
        assert "Statistical Analysis" in content


class TestIntegration:
    """Integration tests for complete workflows"""
    
    def test_complete_workflow(self, tmp_path):
        """Test complete workflow from data to reports"""
        np.random.seed(42)
        
        reporter = ChimeraReporter(significance_level=0.05)
        
        # Add multiple systems
        for i, name in enumerate(['Alpha', 'Beta', 'Gamma']):
            data = np.random.normal(100 + i*10, 5, 30).tolist()
            reporter.add_system_results(
                system_name=name,
                metric_name="Query Throughput",
                metric_unit="queries/sec",
                data=data
            )
        
        # Generate all report types
        csv_path = tmp_path / "report.csv"
        html_path = tmp_path / "report.html"
        
        reporter.generate_csv_report(str(csv_path))
        reporter.generate_html_report(str(html_path), include_plots=False)
        
        assert csv_path.exists()
        assert html_path.exists()
        
        # Verify content
        csv_content = csv_path.read_text()
        assert all(name in csv_content for name in ['Alpha', 'Beta', 'Gamma'])
        
        html_content = html_path.read_text()
        assert 'Neutrality' in html_content
        assert 'Statistical Analysis' in html_content


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
