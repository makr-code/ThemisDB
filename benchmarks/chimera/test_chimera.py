"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_chimera.py                                    ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1449                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • edfcc1a70a  2026-02-28  feat(chimera): implement BenchmarkDashboard for result ag... ║
    • b081c5508d  2026-02-28  feat(chimera): integrate StatisticalAnalyzer with Benchma... ║
    • 717272ce84  2026-02-27  fix(chimera): remove unused StatisticalAnalyzer import; c... ║
    • c39051d13c  2026-02-27  feat(chimera): implement unified benchmark harness ║
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
from chimera import BenchmarkScorer, MetricConfig, MetricDirection, NormalizationMethod, NormalizedScore, CompositeScore
from chimera import BenchmarkHarness, WorkloadDefinition, HarnessConfig, WorkloadResult
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


class TestBenchmarkScorer:
    """Test benchmark result normalization and scoring framework."""

    # ------------------------------------------------------------------
    # MetricConfig validation
    # ------------------------------------------------------------------

    def test_metric_config_positive_weight(self):
        """MetricConfig raises ValueError for non-positive weight."""
        with pytest.raises(ValueError, match="weight"):
            MetricConfig("latency", MetricDirection.LOWER_IS_BETTER, weight=0.0)

    def test_metric_config_defaults(self):
        """MetricConfig default weight and unit are correct."""
        cfg = MetricConfig("throughput", MetricDirection.HIGHER_IS_BETTER)
        assert cfg.weight == 1.0
        assert cfg.unit == ""

    # ------------------------------------------------------------------
    # BenchmarkScorer construction guards
    # ------------------------------------------------------------------

    def test_scorer_requires_metrics(self):
        """BenchmarkScorer raises ValueError when no metrics are given."""
        with pytest.raises(ValueError, match="MetricConfig"):
            BenchmarkScorer(metrics=[])

    def test_scorer_baseline_without_name_raises(self):
        """BASELINE normalization without baseline_system raises ValueError."""
        cfg = MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)
        with pytest.raises(ValueError, match="baseline_system"):
            BenchmarkScorer(
                metrics=[cfg],
                normalization_method=NormalizationMethod.BASELINE,
            )

    # ------------------------------------------------------------------
    # add_results validation
    # ------------------------------------------------------------------

    def test_add_results_unknown_metric_raises(self):
        """add_results raises KeyError for unregistered metric names."""
        scorer = BenchmarkScorer(
            [MetricConfig("latency", MetricDirection.LOWER_IS_BETTER)]
        )
        with pytest.raises(KeyError, match="throughput"):
            scorer.add_results("SystemA", "throughput", [100.0])

    def test_add_results_empty_data_raises(self):
        """add_results raises ValueError when data list is empty."""
        scorer = BenchmarkScorer(
            [MetricConfig("latency", MetricDirection.LOWER_IS_BETTER)]
        )
        with pytest.raises(ValueError, match="empty"):
            scorer.add_results("SystemA", "latency", [])

    # ------------------------------------------------------------------
    # MIN_MAX normalization – HIGHER_IS_BETTER
    # ------------------------------------------------------------------

    def test_min_max_higher_is_better_best_system_scores_100(self):
        """Highest throughput system receives score 100 with min-max normalization."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Fast", "thr", [1000.0, 1000.0])
        scorer.add_results("Slow", "thr", [500.0, 500.0])

        scores = scorer.normalize()
        assert scores["thr"]["Fast"].normalized_score == pytest.approx(100.0)
        assert scores["thr"]["Slow"].normalized_score == pytest.approx(0.0)

    def test_min_max_higher_is_better_intermediate(self):
        """Intermediate throughput yields an intermediate normalized score."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("A", "thr", [0.0])
        scorer.add_results("B", "thr", [50.0])
        scorer.add_results("C", "thr", [100.0])

        scores = scorer.normalize()
        assert scores["thr"]["B"].normalized_score == pytest.approx(50.0)

    # ------------------------------------------------------------------
    # MIN_MAX normalization – LOWER_IS_BETTER
    # ------------------------------------------------------------------

    def test_min_max_lower_is_better_lowest_latency_scores_100(self):
        """Lowest latency system receives score 100 with min-max normalization."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER)]
        )
        scorer.add_results("Fast", "lat", [5.0])
        scorer.add_results("Slow", "lat", [20.0])

        scores = scorer.normalize()
        assert scores["lat"]["Fast"].normalized_score == pytest.approx(100.0)
        assert scores["lat"]["Slow"].normalized_score == pytest.approx(0.0)

    # ------------------------------------------------------------------
    # Degenerate case: all values identical
    # ------------------------------------------------------------------

    def test_min_max_identical_values_all_score_100(self):
        """When all systems have the same mean, every system scores 100."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER)]
        )
        scorer.add_results("A", "lat", [10.0, 10.0])
        scorer.add_results("B", "lat", [10.0, 10.0])

        scores = scorer.normalize()
        for ns in scores["lat"].values():
            assert ns.normalized_score == pytest.approx(100.0)

    # ------------------------------------------------------------------
    # BASELINE normalization
    # ------------------------------------------------------------------

    def test_baseline_normalization_baseline_system_scores_100(self):
        """Baseline system always scores exactly 100 with BASELINE normalization."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)],
            normalization_method=NormalizationMethod.BASELINE,
            baseline_system="Reference",
        )
        scorer.add_results("Reference", "thr", [1000.0])
        scorer.add_results("Candidate", "thr", [1200.0])

        scores = scorer.normalize()
        assert scores["thr"]["Reference"].normalized_score == pytest.approx(100.0)
        assert scores["thr"]["Candidate"].normalized_score == pytest.approx(120.0)

    def test_baseline_normalization_lower_is_better(self):
        """BASELINE normalization inverts for LOWER_IS_BETTER metrics."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER)],
            normalization_method=NormalizationMethod.BASELINE,
            baseline_system="Reference",
        )
        scorer.add_results("Reference", "lat", [10.0])
        scorer.add_results("Better",    "lat", [5.0])   # half the latency → 200

        scores = scorer.normalize()
        assert scores["lat"]["Reference"].normalized_score == pytest.approx(100.0)
        assert scores["lat"]["Better"].normalized_score == pytest.approx(200.0)

    def test_baseline_missing_data_raises(self):
        """BASELINE normalization raises when baseline system has no data for metric."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)],
            normalization_method=NormalizationMethod.BASELINE,
            baseline_system="Reference",
        )
        scorer.add_results("Other", "thr", [100.0])
        with pytest.raises(ValueError, match="no data"):
            scorer.normalize()

    # ------------------------------------------------------------------
    # Z-SCORE normalization
    # ------------------------------------------------------------------

    def test_z_score_normalization_range(self):
        """Z-score normalization maps the best system to 100 and worst to 0."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)],
            normalization_method=NormalizationMethod.Z_SCORE,
        )
        scorer.add_results("A", "thr", [100.0])
        scorer.add_results("B", "thr", [200.0])
        scorer.add_results("C", "thr", [150.0])

        scores = scorer.normalize()
        assert scores["thr"]["B"].normalized_score == pytest.approx(100.0)
        assert scores["thr"]["A"].normalized_score == pytest.approx(0.0)
        assert 0.0 < scores["thr"]["C"].normalized_score < 100.0

    # ------------------------------------------------------------------
    # Composite scoring
    # ------------------------------------------------------------------

    def test_composite_score_single_metric(self):
        """Composite score equals the per-metric score when only one metric exists."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Best",  "thr", [100.0])
        scorer.add_results("Worst", "thr", [0.0])

        composites = scorer.compute_composite_scores()
        by_name = {cs.system_name: cs for cs in composites}
        assert by_name["Best"].composite_score == pytest.approx(100.0)
        assert by_name["Worst"].composite_score == pytest.approx(0.0)

    def test_composite_score_weighted_average(self):
        """Composite score is the weighted mean of per-metric scores."""
        scorer = BenchmarkScorer(
            [
                MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER, weight=2.0),
                MetricConfig("lat", MetricDirection.LOWER_IS_BETTER,  weight=1.0),
            ]
        )
        # System A: throughput best (100), latency worst (0)  → (2*100 + 1*0) / 3 ≈ 66.7
        # System B: throughput worst (0),  latency best (100) → (2*0 + 1*100) / 3 ≈ 33.3
        scorer.add_results("A", "thr", [100.0])
        scorer.add_results("A", "lat", [20.0])
        scorer.add_results("B", "thr", [0.0])
        scorer.add_results("B", "lat", [5.0])

        composites = {cs.system_name: cs for cs in scorer.compute_composite_scores()}
        assert composites["A"].composite_score == pytest.approx(100.0 * 2 / 3, abs=1e-6)
        assert composites["B"].composite_score == pytest.approx(100.0 / 3, abs=1e-6)

    # ------------------------------------------------------------------
    # rank_systems
    # ------------------------------------------------------------------

    def test_rank_systems_ordering(self):
        """rank_systems returns systems sorted best-first with correct rank field."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Mid",  "thr", [50.0])
        scorer.add_results("High", "thr", [100.0])
        scorer.add_results("Low",  "thr", [10.0])

        ranked = scorer.rank_systems()

        assert ranked[0].system_name == "High"
        assert ranked[0].rank == 1
        assert ranked[1].system_name == "Mid"
        assert ranked[1].rank == 2
        assert ranked[2].system_name == "Low"
        assert ranked[2].rank == 3

    # ------------------------------------------------------------------
    # Per-metric ranks
    # ------------------------------------------------------------------

    def test_per_metric_rank_assigned(self):
        """normalize() assigns rank 1 to the best system on each metric."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER)]
        )
        scorer.add_results("Fast",   "lat", [5.0])
        scorer.add_results("Medium", "lat", [10.0])
        scorer.add_results("Slow",   "lat", [20.0])

        scores = scorer.normalize()
        assert scores["lat"]["Fast"].rank == 1
        assert scores["lat"]["Medium"].rank == 2
        assert scores["lat"]["Slow"].rank == 3

    # ------------------------------------------------------------------
    # generate_summary
    # ------------------------------------------------------------------

    def test_generate_summary_structure(self):
        """generate_summary returns the expected top-level keys."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER, unit="ops/s")]
        )
        scorer.add_results("A", "thr", [100.0])
        scorer.add_results("B", "thr", [80.0])

        summary = scorer.generate_summary()

        assert "normalization_method" in summary
        assert "metrics" in summary
        assert "rankings" in summary
        assert len(summary["rankings"]) == 2
        assert summary["rankings"][0]["rank"] == 1

    def test_generate_summary_metric_metadata(self):
        """generate_summary includes direction and unit for each metric."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER, unit="ms")]
        )
        scorer.add_results("X", "lat", [10.0])

        summary = scorer.generate_summary()
        metric_info = summary["metrics"][0]

        assert metric_info["name"] == "lat"
        assert metric_info["direction"] == "lower_is_better"
        assert metric_info["unit"] == "ms"

    # ------------------------------------------------------------------
    # raw_mean / raw_std in NormalizedScore
    # ------------------------------------------------------------------

    def test_normalized_score_raw_statistics(self):
        """NormalizedScore contains correct raw_mean and raw_std."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        data = [100.0, 110.0, 90.0]
        scorer.add_results("System", "thr", data)
        scorer.add_results("Other", "thr", [50.0])

        scores = scorer.normalize()
        ns = scores["thr"]["System"]
        assert ns.raw_mean == pytest.approx(np.mean(data))
        assert ns.raw_std == pytest.approx(np.std(data, ddof=1))

    # ------------------------------------------------------------------
    # Confidence interval integration (StatisticalAnalyzer)
    # ------------------------------------------------------------------

    def test_confidence_interval_populated_for_multi_sample(self):
        """NormalizedScore.confidence_interval_95 is set when sample size >= 2."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("A", "thr", [100.0, 110.0, 90.0])
        scorer.add_results("B", "thr", [50.0, 55.0, 45.0])

        scores = scorer.normalize()
        ns_a = scores["thr"]["A"]

        assert ns_a.confidence_interval_95 is not None
        lower, upper = ns_a.confidence_interval_95
        assert lower < ns_a.raw_mean < upper

    def test_confidence_interval_none_for_single_sample(self):
        """NormalizedScore.confidence_interval_95 is None when sample size < 2."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Solo", "thr", [100.0])
        scorer.add_results("Other", "thr", [50.0])

        scores = scorer.normalize()
        assert scores["thr"]["Solo"].confidence_interval_95 is None

    def test_generate_summary_includes_confidence_interval(self):
        """generate_summary includes confidence_interval_95 in per_metric output."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("A", "thr", [100.0, 110.0, 90.0])
        scorer.add_results("B", "thr", [50.0, 55.0, 45.0])

        summary = scorer.generate_summary()
        per_metric = summary["rankings"][0]["per_metric"]["thr"]

        assert "confidence_interval_95" in per_metric
        assert per_metric["confidence_interval_95"] is not None
        assert len(per_metric["confidence_interval_95"]) == 2

    def test_generate_summary_ci_null_for_single_sample(self):
        """generate_summary sets confidence_interval_95 to null for single-sample systems."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Solo", "thr", [100.0])
        scorer.add_results("Other", "thr", [80.0])

        summary = scorer.generate_summary()
        by_system = {r["system"]: r for r in summary["rankings"]}
        assert by_system["Solo"]["per_metric"]["thr"]["confidence_interval_95"] is None

    def test_summary_json_serializable_with_ci(self):
        """generate_summary with CI is still JSON-serializable."""
        import json

        scorer = BenchmarkScorer(
            [MetricConfig("ops", MetricDirection.HIGHER_IS_BETTER)]
        )
        scorer.add_results("Alpha", "ops", [500.0, 510.0, 490.0])
        scorer.add_results("Beta", "ops", [300.0, 310.0, 290.0])

        summary = scorer.generate_summary()
        json_str = json.dumps(summary)
        assert "confidence_interval_95" in json_str

    # ------------------------------------------------------------------
    # Edge cases for branch coverage
    # ------------------------------------------------------------------

    def test_normalize_skips_metric_with_no_data(self):
        """normalize() silently skips metrics for which no data was added."""
        scorer = BenchmarkScorer(
            [
                MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER),
                MetricConfig("lat", MetricDirection.LOWER_IS_BETTER),
            ]
        )
        # Only add data for "thr"; "lat" has no data at all
        scorer.add_results("A", "thr", [100.0])
        scorer.add_results("B", "thr", [80.0])

        scores = scorer.normalize()
        assert "thr" in scores
        assert "lat" not in scores

    def test_composite_skips_metric_with_no_data(self):
        """compute_composite_scores skips metrics for which no data was added."""
        scorer = BenchmarkScorer(
            [
                MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER),
                MetricConfig("lat", MetricDirection.LOWER_IS_BETTER),
            ]
        )
        # Only "thr" has data; "lat" has no data
        scorer.add_results("A", "thr", [100.0])
        scorer.add_results("B", "thr", [80.0])

        composites = {cs.system_name: cs for cs in scorer.compute_composite_scores()}

        # Both systems should appear, with only "thr" in their metric_scores
        assert "A" in composites
        assert "B" in composites
        assert "thr" in composites["A"].metric_scores
        assert "lat" not in composites["A"].metric_scores

    def test_composite_partial_system_data(self):
        """Composite score only includes metrics for which a system has data."""
        scorer = BenchmarkScorer(
            [
                MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER),
                MetricConfig("lat", MetricDirection.LOWER_IS_BETTER),
            ]
        )
        # Both systems have "thr"; only "Alpha" has "lat"
        scorer.add_results("Alpha", "thr", [100.0])
        scorer.add_results("Alpha", "lat", [5.0])
        scorer.add_results("Beta",  "thr", [50.0])
        # "Beta" has no "lat" data

        composites = {cs.system_name: cs for cs in scorer.compute_composite_scores()}

        # Alpha composite uses both metrics; Beta uses only thr
        assert "thr" in composites["Alpha"].metric_scores
        assert "lat" in composites["Alpha"].metric_scores
        assert "thr" in composites["Beta"].metric_scores
        assert "lat" not in composites["Beta"].metric_scores

    def test_baseline_zero_mean_raises(self):
        """BASELINE normalization raises when the baseline system's mean is zero."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)],
            normalization_method=NormalizationMethod.BASELINE,
            baseline_system="Reference",
        )
        scorer.add_results("Reference", "thr", [0.0, 0.0])
        scorer.add_results("Candidate", "thr", [100.0])

        with pytest.raises(ValueError, match="mean of 0"):
            scorer.normalize()

    def test_z_score_single_system_returns_100(self):
        """Z-score normalization with a single system (sigma=0) returns 100."""
        scorer = BenchmarkScorer(
            [MetricConfig("thr", MetricDirection.HIGHER_IS_BETTER)],
            normalization_method=NormalizationMethod.Z_SCORE,
        )
        scorer.add_results("OnlySystem", "thr", [42.0, 42.0, 42.0])

        scores = scorer.normalize()
        assert scores["thr"]["OnlySystem"].normalized_score == pytest.approx(100.0)

    def test_z_score_lower_is_better_direction(self):
        """Z-score LOWER_IS_BETTER: smallest latency gets the highest score."""
        scorer = BenchmarkScorer(
            [MetricConfig("lat", MetricDirection.LOWER_IS_BETTER)],
            normalization_method=NormalizationMethod.Z_SCORE,
        )
        scorer.add_results("Fast",   "lat", [5.0])
        scorer.add_results("Slow",   "lat", [50.0])
        scorer.add_results("Medium", "lat", [25.0])

        scores = scorer.normalize()
        assert scores["lat"]["Fast"].normalized_score == pytest.approx(100.0)
        assert scores["lat"]["Slow"].normalized_score == pytest.approx(0.0)


class TestBenchmarkScorerIntegration:
    """Integration tests covering multi-metric, multi-system scenarios."""

    def test_three_systems_two_metrics(self):
        """Verify composite ranking with three systems and two metrics."""
        scorer = BenchmarkScorer(
            [
                MetricConfig("throughput", MetricDirection.HIGHER_IS_BETTER, weight=1.0),
                MetricConfig("latency",    MetricDirection.LOWER_IS_BETTER,  weight=1.0),
            ]
        )
        # Alpha: best throughput, worst latency
        scorer.add_results("Alpha", "throughput", [1000.0])
        scorer.add_results("Alpha", "latency",    [50.0])
        # Beta: worst throughput, best latency
        scorer.add_results("Beta", "throughput", [200.0])
        scorer.add_results("Beta", "latency",    [5.0])
        # Gamma: middle on both
        scorer.add_results("Gamma", "throughput", [600.0])
        scorer.add_results("Gamma", "latency",    [25.0])

        ranked = scorer.rank_systems()

        # All ranks are assigned
        assert {cs.rank for cs in ranked} == {1, 2, 3}
        # Composite scores are non-negative
        assert all(cs.composite_score >= 0 for cs in ranked)
        # Best rank has highest score
        assert ranked[0].composite_score >= ranked[1].composite_score >= ranked[2].composite_score

    def test_summary_json_serializable(self):
        """generate_summary output is serializable to JSON (all values are primitives)."""
        import json

        scorer = BenchmarkScorer(
            [
                MetricConfig("ops", MetricDirection.HIGHER_IS_BETTER),
                MetricConfig("p99", MetricDirection.LOWER_IS_BETTER),
            ]
        )
        scorer.add_results("Alpha", "ops", [500.0, 510.0, 490.0])
        scorer.add_results("Alpha", "p99", [12.0, 13.0, 11.0])
        scorer.add_results("Beta",  "ops", [300.0, 310.0, 290.0])
        scorer.add_results("Beta",  "p99", [8.0,   9.0,   7.0])

        summary = scorer.generate_summary()
        # Should not raise
        json_str = json.dumps(summary)
        assert "Alpha" in json_str
        assert "Beta" in json_str


# ---------------------------------------------------------------------------
# BenchmarkHarness tests
# ---------------------------------------------------------------------------

class TestWorkloadDefinition:
    """Tests for WorkloadDefinition construction."""

    def test_minimal_creation(self):
        wd = WorkloadDefinition(workload_id="wl_1", operation=lambda: None)
        assert wd.workload_id == "wl_1"
        assert callable(wd.operation)
        assert wd.workload_family == "custom"
        assert wd.parameters == {}

    def test_full_creation(self):
        wd = WorkloadDefinition(
            workload_id="ycsb_a",
            operation=lambda: None,
            description="Update heavy",
            workload_family="YCSB",
            standard_reference="Cooper et al., 2010",
            parameters={"record_count": 1_000_000},
        )
        assert wd.workload_family == "YCSB"
        assert wd.parameters["record_count"] == 1_000_000


class TestHarnessConfig:
    """Tests for HarnessConfig validation."""

    def test_defaults(self):
        cfg = HarnessConfig()
        assert cfg.warmup_iterations == 100
        assert cfg.run_iterations == 1000
        assert 50.0 in cfg.percentiles
        assert 95.0 in cfg.percentiles
        assert 99.0 in cfg.percentiles

    def test_custom_values(self):
        cfg = HarnessConfig(warmup_iterations=5, run_iterations=50, percentiles=[90.0])
        assert cfg.warmup_iterations == 5
        assert cfg.run_iterations == 50
        assert cfg.percentiles == [90.0]

    def test_negative_warmup_raises(self):
        with pytest.raises(ValueError, match="warmup_iterations"):
            HarnessConfig(warmup_iterations=-1)

    def test_zero_run_iterations_raises(self):
        with pytest.raises(ValueError, match="run_iterations"):
            HarnessConfig(run_iterations=0)

    def test_invalid_percentile_raises(self):
        with pytest.raises(ValueError, match="percentile"):
            HarnessConfig(percentiles=[101.0])


class TestBenchmarkHarness:
    """Unit tests for BenchmarkHarness."""

    _cfg = HarnessConfig(warmup_iterations=5, run_iterations=20)

    def _make_harness(self) -> BenchmarkHarness:
        return BenchmarkHarness("TestSystem", self._cfg)

    def test_add_workload_registers_id(self):
        harness = self._make_harness()
        wd = WorkloadDefinition("op1", lambda: None)
        harness.add_workload(wd)
        assert "op1" in harness.get_workload_ids()

    def test_add_workload_replaces_existing(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("op1", lambda: 1))
        harness.add_workload(WorkloadDefinition("op1", lambda: 2))
        assert len(harness.get_workload_ids()) == 1

    def test_warm_up_returns_success_count(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        successful = harness.warm_up("noop")
        assert successful == self._cfg.warmup_iterations

    def test_warm_up_with_errors_does_not_raise(self):
        harness = self._make_harness()

        def boom():
            raise RuntimeError("simulated failure")

        harness.add_workload(WorkloadDefinition("failing", boom))
        # Must not raise; errors are silently swallowed during warm-up
        result = harness.warm_up("failing")
        assert result == 0

    def test_warm_up_unknown_workload_raises(self):
        harness = self._make_harness()
        with pytest.raises(KeyError):
            harness.warm_up("nonexistent")

    def test_run_returns_workload_result(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert isinstance(result, WorkloadResult)
        assert result.workload_id == "noop"
        assert result.system_name == "TestSystem"

    def test_run_latency_count_matches_iterations(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert len(result.latencies_ms) == self._cfg.run_iterations

    def test_run_latencies_are_non_negative(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert all(lat >= 0.0 for lat in result.latencies_ms)

    def test_run_throughput_positive(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert result.throughput_ops_per_sec > 0.0

    def test_run_error_count(self):
        harness = self._make_harness()
        call_count = [0]

        def sometimes_fails():
            call_count[0] += 1
            if call_count[0] % 2 == 0:
                raise ValueError("even call")

        harness.add_workload(WorkloadDefinition("flaky", sometimes_fails))
        result = harness.run("flaky")
        assert result.error_count == self._cfg.run_iterations // 2
        assert result.error_count + (self._cfg.run_iterations - result.error_count) == self._cfg.run_iterations

    def test_failed_operations_still_contribute_latency_samples(self):
        """Every iteration — including those that raise — records a latency sample."""
        harness = self._make_harness()

        def always_fails():
            raise RuntimeError("always fails")

        harness.add_workload(WorkloadDefinition("fail_all", always_fails))
        result = harness.run("fail_all")

        # All iterations recorded an error
        assert result.error_count == self._cfg.run_iterations
        # Each iteration still contributed a latency sample
        assert len(result.latencies_ms) == self._cfg.run_iterations
        assert all(lat >= 0.0 for lat in result.latencies_ms)

    def test_run_statistics_computed(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert result.mean_latency_ms >= 0.0
        assert result.p50_latency_ms >= 0.0
        assert result.p95_latency_ms >= result.p50_latency_ms
        assert result.p99_latency_ms >= result.p95_latency_ms

    def test_run_percentile_latencies_populated(self):
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=10, percentiles=[90.0])
        harness = BenchmarkHarness("S", cfg)
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        result = harness.run("noop")
        assert 90.0 in result.percentile_latencies_ms

    def test_get_result_none_before_run(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        assert harness.get_result("noop") is None

    def test_get_result_after_run(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("noop", lambda: None))
        harness.run("noop")
        assert harness.get_result("noop") is not None

    def test_run_all_runs_every_workload(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("a", lambda: None))
        harness.add_workload(WorkloadDefinition("b", lambda: None))
        results = harness.run_all(warmup=False)
        assert set(results.keys()) == {"a", "b"}

    def test_run_all_with_warmup(self):
        harness = self._make_harness()
        harness.add_workload(WorkloadDefinition("a", lambda: None))
        results = harness.run_all(warmup=True)
        assert "a" in results


class TestBenchmarkHarnessReport:
    """Tests for BenchmarkHarness.report()."""

    def test_report_structure(self):
        import json

        cfg = HarnessConfig(warmup_iterations=0, run_iterations=10)
        harness = BenchmarkHarness("ReportSystem", cfg)
        harness.add_workload(WorkloadDefinition("load", lambda: None))
        harness.run("load")

        summary = harness.report()
        assert summary["system_name"] == "ReportSystem"
        assert "load" in summary["workloads"]

        workload_data = summary["workloads"]["load"]
        assert "throughput_ops_per_sec" in workload_data
        assert "mean_latency_ms" in workload_data
        assert "p50_latency_ms" in workload_data
        assert "p95_latency_ms" in workload_data
        assert "p99_latency_ms" in workload_data
        assert "error_count" in workload_data

        # Must be JSON-serialisable
        json.dumps(summary)

    def test_report_no_workloads_run(self):
        harness = BenchmarkHarness("Empty", HarnessConfig(run_iterations=5))
        summary = harness.report()
        assert summary["workloads"] == {}

    def test_report_html_output(self, tmp_path):
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=5)
        harness = BenchmarkHarness("SysHTML", cfg)
        harness.add_workload(WorkloadDefinition("q", lambda: None))
        harness.run("q")

        html_path = str(tmp_path / "report.html")
        harness.report(output_path=html_path)

        import os
        assert os.path.exists(html_path)
        content = open(html_path).read()
        assert "<html" in content.lower()


# ---------------------------------------------------------------------------
# BenchmarkDashboard tests
# ---------------------------------------------------------------------------

from chimera import BenchmarkDashboard
from chimera.scoring import NormalizationMethod


def _make_harness(system_name: str, workload_id: str = "noop") -> BenchmarkHarness:
    cfg = HarnessConfig(warmup_iterations=0, run_iterations=10)
    h = BenchmarkHarness(system_name, cfg)
    h.add_workload(WorkloadDefinition(workload_id, lambda: None))
    h.run(workload_id)
    return h


class TestBenchmarkDashboardConstruction:
    """Tests for BenchmarkDashboard construction and accessors."""

    def test_default_title(self):
        d = BenchmarkDashboard()
        assert "CHIMERA" in d.title

    def test_custom_title(self):
        d = BenchmarkDashboard(title="My Suite")
        assert d.title == "My Suite"

    def test_get_system_names_empty(self):
        d = BenchmarkDashboard()
        assert d.get_system_names() == []

    def test_get_workload_ids_empty(self):
        d = BenchmarkDashboard()
        assert d.get_workload_ids() == []


class TestBenchmarkDashboardIngestion:
    """Tests for add_harness_results and add_workload_result."""

    def test_add_harness_results_registers_system(self):
        d = BenchmarkDashboard()
        h = _make_harness("Alpha")
        d.add_harness_results(h)
        assert "Alpha" in d.get_system_names()

    def test_add_harness_results_registers_workload(self):
        d = BenchmarkDashboard()
        h = _make_harness("Alpha", workload_id="ycsb_a")
        d.add_harness_results(h)
        assert "ycsb_a" in d.get_workload_ids()

    def test_add_harness_results_empty_harness_raises(self):
        d = BenchmarkDashboard()
        h = BenchmarkHarness("Empty", HarnessConfig(run_iterations=5))
        # No workloads added → no results → should raise
        with pytest.raises(ValueError, match="no completed workload results"):
            d.add_harness_results(h)

    def test_add_workload_result_direct(self):
        d = BenchmarkDashboard()
        result = WorkloadResult(
            workload_id="load",
            system_name="SysX",
            latencies_ms=[1.0, 2.0, 3.0],
            throughput_ops_per_sec=5000.0,
            error_count=0,
            warmup_iterations=0,
            run_iterations=3,
            elapsed_seconds=0.001,
            mean_latency_ms=2.0,
            p50_latency_ms=2.0,
            p95_latency_ms=3.0,
            p99_latency_ms=3.0,
        )
        d.add_workload_result(result)
        assert "SysX" in d.get_system_names()
        assert "load" in d.get_workload_ids()

    def test_multiple_systems_registered(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        assert set(d.get_system_names()) == {"Alpha", "Beta"}

    def test_multiple_workloads_registered(self):
        d = BenchmarkDashboard()
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=5)
        h = BenchmarkHarness("Sys", cfg)
        h.add_workload(WorkloadDefinition("wl_a", lambda: None))
        h.add_workload(WorkloadDefinition("wl_b", lambda: None))
        h.run_all(warmup=False)
        d.add_harness_results(h)
        assert "wl_a" in d.get_workload_ids()
        assert "wl_b" in d.get_workload_ids()

    def test_later_harness_results_replace_earlier(self):
        """Re-registering same workload_id for same system replaces the result."""
        d = BenchmarkDashboard()
        h1 = _make_harness("Alpha", "noop")
        d.add_harness_results(h1)
        h2 = _make_harness("Alpha", "noop")
        d.add_harness_results(h2)
        # System still registered once
        assert d.get_system_names().count("Alpha") == 1


class TestBenchmarkDashboardAggregate:
    """Tests for the aggregate() method."""

    def test_aggregate_raises_when_empty(self):
        d = BenchmarkDashboard()
        with pytest.raises(ValueError, match="No results"):
            d.aggregate()

    def test_aggregate_structure(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        summary = d.aggregate()

        assert "title" in summary
        assert "generated_at" in summary
        assert "normalization_method" in summary
        assert "systems" in summary
        assert "workloads" in summary
        assert "results" in summary
        assert "composite_rankings" in summary

    def test_aggregate_systems_sorted(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Zebra"))
        d.add_harness_results(_make_harness("Alpha"))
        summary = d.aggregate()
        assert summary["systems"] == sorted(summary["systems"])

    def test_aggregate_workloads_sorted(self):
        d = BenchmarkDashboard()
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=5)
        h = BenchmarkHarness("S", cfg)
        h.add_workload(WorkloadDefinition("z_wl", lambda: None))
        h.add_workload(WorkloadDefinition("a_wl", lambda: None))
        h.run_all(warmup=False)
        d.add_harness_results(h)
        summary = d.aggregate()
        assert summary["workloads"] == sorted(summary["workloads"])

    def test_aggregate_results_contain_metrics(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha", "wl1"))
        summary = d.aggregate()
        wl1 = summary["results"]["wl1"]["Alpha"]
        assert "throughput_ops_per_sec" in wl1
        assert "mean_latency_ms" in wl1
        assert "p50_latency_ms" in wl1
        assert "p95_latency_ms" in wl1
        assert "p99_latency_ms" in wl1
        assert "error_count" in wl1

    def test_aggregate_composite_rankings_present(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        summary = d.aggregate()
        assert len(summary["composite_rankings"]) == 2

    def test_aggregate_composite_ranking_fields(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        summary = d.aggregate()
        entry = summary["composite_rankings"][0]
        assert "rank" in entry
        assert "system" in entry
        assert "composite_score" in entry

    def test_aggregate_composite_score_in_range(self):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        summary = d.aggregate()
        for entry in summary["composite_rankings"]:
            assert 0.0 <= entry["composite_score"] <= 100.0

    def test_aggregate_json_serializable(self):
        import json as _json
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("A"))
        d.add_harness_results(_make_harness("B"))
        summary = d.aggregate()
        # Must not raise
        serialised = _json.dumps(summary)
        assert "A" in serialised
        assert "B" in serialised

    def test_aggregate_normalization_method_recorded(self):
        d = BenchmarkDashboard(normalization_method=NormalizationMethod.Z_SCORE)
        d.add_harness_results(_make_harness("X"))
        summary = d.aggregate()
        assert summary["normalization_method"] == "z_score"

    def test_aggregate_missing_system_workload_combination(self):
        """A system that lacks data for a workload should appear as absent in results."""
        d = BenchmarkDashboard()
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=5)

        h_a = BenchmarkHarness("Alpha", cfg)
        h_a.add_workload(WorkloadDefinition("wl_shared", lambda: None))
        h_a.add_workload(WorkloadDefinition("wl_alpha_only", lambda: None))
        h_a.run_all(warmup=False)
        d.add_harness_results(h_a)

        h_b = BenchmarkHarness("Beta", cfg)
        h_b.add_workload(WorkloadDefinition("wl_shared", lambda: None))
        h_b.run_all(warmup=False)
        d.add_harness_results(h_b)

        summary = d.aggregate()
        # Beta has no result for wl_alpha_only
        assert "Beta" not in summary["results"].get("wl_alpha_only", {})
        # Both have results for wl_shared
        assert "Alpha" in summary["results"]["wl_shared"]
        assert "Beta" in summary["results"]["wl_shared"]


class TestBenchmarkDashboardExport:
    """Tests for generate_json_report and generate_html_dashboard."""

    def test_generate_json_report_creates_file(self, tmp_path):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        out = str(tmp_path / "report.json")
        d.generate_json_report(out)
        import os
        assert os.path.exists(out)

    def test_generate_json_report_valid_json(self, tmp_path):
        import json as _json
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        out = str(tmp_path / "report.json")
        d.generate_json_report(out)
        with open(out) as fh:
            data = _json.load(fh)
        assert "systems" in data
        assert "Alpha" in data["systems"]

    def test_generate_json_report_creates_parent_dirs(self, tmp_path):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        out = str(tmp_path / "deep" / "nested" / "report.json")
        d.generate_json_report(out)
        import os
        assert os.path.exists(out)

    def test_generate_html_dashboard_creates_file(self, tmp_path):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        out = str(tmp_path / "dashboard.html")
        d.generate_html_dashboard(out)
        import os
        assert os.path.exists(out)

    def test_generate_html_dashboard_valid_html(self, tmp_path):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("Alpha"))
        d.add_harness_results(_make_harness("Beta"))
        out = str(tmp_path / "dashboard.html")
        d.generate_html_dashboard(out)
        content = open(out, encoding="utf-8").read()
        assert "<!DOCTYPE html>" in content
        assert "<html" in content
        assert "</html>" in content

    def test_generate_html_dashboard_contains_system_names(self, tmp_path):
        d = BenchmarkDashboard(title="Test Dashboard")
        d.add_harness_results(_make_harness("SystemAlpha"))
        d.add_harness_results(_make_harness("SystemBeta"))
        out = str(tmp_path / "dashboard.html")
        d.generate_html_dashboard(out)
        content = open(out, encoding="utf-8").read()
        assert "SystemAlpha" in content
        assert "SystemBeta" in content

    def test_generate_html_dashboard_contains_title(self, tmp_path):
        d = BenchmarkDashboard(title="My Benchmark Suite 2026")
        d.add_harness_results(_make_harness("X"))
        out = str(tmp_path / "dashboard.html")
        d.generate_html_dashboard(out)
        content = open(out, encoding="utf-8").read()
        assert "My Benchmark Suite 2026" in content

    def test_generate_html_dashboard_contains_workload_ids(self, tmp_path):
        cfg = HarnessConfig(warmup_iterations=0, run_iterations=5)
        h = BenchmarkHarness("Sys", cfg)
        h.add_workload(WorkloadDefinition("ycsb_read", lambda: None))
        h.run("ycsb_read")
        d = BenchmarkDashboard()
        d.add_harness_results(h)
        out = str(tmp_path / "dashboard.html")
        d.generate_html_dashboard(out)
        content = open(out, encoding="utf-8").read()
        assert "ycsb_read" in content

    def test_generate_html_dashboard_creates_parent_dirs(self, tmp_path):
        d = BenchmarkDashboard()
        d.add_harness_results(_make_harness("X"))
        out = str(tmp_path / "sub" / "dashboard.html")
        d.generate_html_dashboard(out)
        import os
        assert os.path.exists(out)


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
