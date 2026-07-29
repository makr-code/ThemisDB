"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            statistics.py                                      ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     402                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • b081c5508d  2026-02-28  feat(chimera): integrate StatisticalAnalyzer with Benchma... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Statistical Analysis Module for Vendor-Neutral Benchmarking

Implements IEEE-standard statistical tests and analysis methods for
rigorous benchmark comparison.

References:
- IEEE Std 7-4.3.2-2016: IEEE Standard Criteria for Digital Computers in Safety Systems
- Cohen, J. (1988). Statistical Power Analysis for the Behavioral Sciences
- Mann, H. B., & Whitney, D. R. (1947). On a test of whether one of two random variables is stochastically larger than the other
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass
from enum import Enum
import warnings


class SignificanceLevel(Enum):
    """Standard significance levels for hypothesis testing"""
    VERY_SIGNIFICANT = 0.001  # p < 0.001
    HIGHLY_SIGNIFICANT = 0.01  # p < 0.01
    SIGNIFICANT = 0.05  # p < 0.05
    MARGINALLY_SIGNIFICANT = 0.10  # p < 0.10


@dataclass
class StatisticalResult:
    """Container for statistical test results"""
    test_name: str
    statistic: float
    p_value: float
    is_significant: bool
    significance_level: float
    effect_size: Optional[float] = None
    confidence_interval: Optional[Tuple[float, float]] = None
    interpretation: str = ""


@dataclass
class DescriptiveStats:
    """Container for descriptive statistics"""
    mean: float
    median: float
    std_dev: float
    min_value: float
    max_value: float
    p25: float  # 25th percentile
    p75: float  # 75th percentile
    p95: float  # 95th percentile
    p99: float  # 99th percentile
    n_samples: int
    outliers_removed: int = 0


class StatisticalAnalyzer:
    """
    Statistical analysis engine for benchmark data.
    
    Provides IEEE-compliant statistical tests and analysis methods
    for rigorous, transparent benchmark comparison.
    """
    
    def __init__(self, significance_level: float = 0.05):
        """
        Initialize statistical analyzer.
        
        Args:
            significance_level: Alpha level for hypothesis tests (default: 0.05)
        """
        self.significance_level = significance_level
        self.warnings = []
    
    def descriptive_statistics(self, data: List[float], remove_outliers: bool = True) -> DescriptiveStats:
        """
        Calculate comprehensive descriptive statistics.
        
        Args:
            data: List of numerical measurements
            remove_outliers: Whether to remove outliers using IQR method
            
        Returns:
            DescriptiveStats object with all statistics
        """
        if not data:
            raise ValueError("Data cannot be empty")
        
        original_n = len(data)
        data_array = np.array(data)
        
        # Remove outliers if requested
        outliers_removed = 0
        if remove_outliers and len(data) > 3:
            data_array, outliers_removed = self._remove_outliers_iqr(data_array)
            if outliers_removed > 0:
                self.warnings.append(
                    f"Removed {outliers_removed} outliers using IQR method "
                    f"({outliers_removed/original_n*100:.1f}% of data)"
                )
        
        # Calculate statistics
        return DescriptiveStats(
            mean=float(np.mean(data_array)),
            median=float(np.median(data_array)),
            std_dev=float(np.std(data_array, ddof=1)) if len(data_array) > 1 else 0.0,
            min_value=float(np.min(data_array)),
            max_value=float(np.max(data_array)),
            p25=float(np.percentile(data_array, 25)),
            p75=float(np.percentile(data_array, 75)),
            p95=float(np.percentile(data_array, 95)),
            p99=float(np.percentile(data_array, 99)),
            n_samples=len(data_array),
            outliers_removed=outliers_removed
        )
    
    def _remove_outliers_iqr(self, data: np.ndarray, iqr_multiplier: float = 1.5) -> Tuple[np.ndarray, int]:
        """
        Remove outliers using the Interquartile Range (IQR) method.
        
        Args:
            data: NumPy array of measurements
            iqr_multiplier: IQR multiplier for outlier detection (default: 1.5)
            
        Returns:
            Tuple of (cleaned_data, num_outliers_removed)
        """
        q1 = np.percentile(data, 25)
        q3 = np.percentile(data, 75)
        iqr = q3 - q1
        
        lower_bound = q1 - iqr_multiplier * iqr
        upper_bound = q3 + iqr_multiplier * iqr
        
        mask = (data >= lower_bound) & (data <= upper_bound)
        cleaned_data = data[mask]
        outliers_removed = len(data) - len(cleaned_data)
        
        return cleaned_data, outliers_removed
    
    def t_test(self, data1: List[float], data2: List[float], 
               paired: bool = False) -> StatisticalResult:
        """
        Perform Student's t-test for comparing two samples.
        
        Args:
            data1: First sample
            data2: Second sample
            paired: Whether to perform paired t-test
            
        Returns:
            StatisticalResult object
        """
        from scipy import stats
        
        arr1 = np.array(data1)
        arr2 = np.array(data2)
        
        if paired:
            if len(arr1) != len(arr2):
                raise ValueError("Paired t-test requires equal sample sizes")
            statistic, p_value = stats.ttest_rel(arr1, arr2)
            test_name = "Paired t-test"
        else:
            statistic, p_value = stats.ttest_ind(arr1, arr2, equal_var=False)  # Welch's t-test
            test_name = "Welch's t-test"
        
        is_significant = p_value < self.significance_level
        effect_size = self.cohens_d(data1, data2)
        
        interpretation = self._interpret_t_test(p_value, effect_size, is_significant)
        
        return StatisticalResult(
            test_name=test_name,
            statistic=float(statistic),
            p_value=float(p_value),
            is_significant=is_significant,
            significance_level=self.significance_level,
            effect_size=effect_size,
            interpretation=interpretation
        )
    
    def mann_whitney_u(self, data1: List[float], data2: List[float]) -> StatisticalResult:
        """
        Perform Mann-Whitney U test (non-parametric alternative to t-test).
        
        Args:
            data1: First sample
            data2: Second sample
            
        Returns:
            StatisticalResult object
        """
        from scipy import stats
        
        arr1 = np.array(data1)
        arr2 = np.array(data2)
        
        statistic, p_value = stats.mannwhitneyu(arr1, arr2, alternative='two-sided')
        is_significant = p_value < self.significance_level
        
        # Calculate rank-biserial correlation as effect size
        effect_size = self._rank_biserial_correlation(arr1, arr2, float(statistic))
        
        interpretation = self._interpret_mann_whitney(p_value, effect_size, is_significant)
        
        return StatisticalResult(
            test_name="Mann-Whitney U test",
            statistic=float(statistic),
            p_value=float(p_value),
            is_significant=is_significant,
            significance_level=self.significance_level,
            effect_size=effect_size,
            interpretation=interpretation
        )
    
    def cohens_d(self, data1: List[float], data2: List[float]) -> float:
        """
        Calculate Cohen's d effect size.
        
        Cohen's d interpretation:
        - Small: 0.2
        - Medium: 0.5
        - Large: 0.8
        
        Args:
            data1: First sample
            data2: Second sample
            
        Returns:
            Cohen's d value
        """
        arr1 = np.array(data1)
        arr2 = np.array(data2)
        
        n1, n2 = len(arr1), len(arr2)
        var1, var2 = np.var(arr1, ddof=1), np.var(arr2, ddof=1)
        
        # Pooled standard deviation
        pooled_std = np.sqrt(((n1 - 1) * var1 + (n2 - 1) * var2) / (n1 + n2 - 2))
        
        # Cohen's d
        d = (np.mean(arr1) - np.mean(arr2)) / pooled_std
        
        return float(d)
    
    def _rank_biserial_correlation(self, data1: np.ndarray, data2: np.ndarray, 
                                   u_statistic: float) -> float:
        """Calculate rank-biserial correlation as effect size for Mann-Whitney U"""
        n1, n2 = len(data1), len(data2)
        # r = 1 - (2U) / (n1 * n2)
        r = 1 - (2 * u_statistic) / (n1 * n2)
        return float(r)
    
    def confidence_interval(self, data: List[float], confidence: float = 0.95) -> Tuple[float, float]:
        """
        Calculate confidence interval for the mean.
        
        Args:
            data: Sample data
            confidence: Confidence level (default: 0.95 for 95% CI)
            
        Returns:
            Tuple of (lower_bound, upper_bound)
        """
        from scipy import stats
        
        arr = np.array(data)
        n = len(arr)
        mean = np.mean(arr)
        std_err = stats.sem(arr)
        
        # t-distribution critical value
        df = n - 1
        t_critical = stats.t.ppf((1 + confidence) / 2, df)
        
        margin_of_error = t_critical * std_err
        
        return (float(mean - margin_of_error), float(mean + margin_of_error))
    
    def _interpret_t_test(self, p_value: float, effect_size: float, 
                         is_significant: bool) -> str:
        """Generate interpretation text for t-test results"""
        parts = []
        
        if is_significant:
            parts.append("The difference is statistically significant")
        else:
            parts.append("The difference is not statistically significant")
        
        parts.append(f"(p = {p_value:.4f})")
        
        # Effect size interpretation
        abs_d = abs(effect_size)
        if abs_d < 0.2:
            effect_desc = "negligible"
        elif abs_d < 0.5:
            effect_desc = "small"
        elif abs_d < 0.8:
            effect_desc = "medium"
        else:
            effect_desc = "large"
        
        parts.append(f"with a {effect_desc} effect size (Cohen's d = {effect_size:.3f})")
        
        return " ".join(parts)
    
    def _interpret_mann_whitney(self, p_value: float, effect_size: float,
                               is_significant: bool) -> str:
        """Generate interpretation text for Mann-Whitney U results"""
        parts = []
        
        if is_significant:
            parts.append("The distributions differ significantly")
        else:
            parts.append("The distributions do not differ significantly")
        
        parts.append(f"(p = {p_value:.4f})")
        
        # Effect size interpretation
        abs_r = abs(effect_size)
        if abs_r < 0.1:
            effect_desc = "negligible"
        elif abs_r < 0.3:
            effect_desc = "small"
        elif abs_r < 0.5:
            effect_desc = "medium"
        else:
            effect_desc = "large"
        
        parts.append(f"with a {effect_desc} effect size (r = {effect_size:.3f})")
        
        return " ".join(parts)
    
    def compare_systems(self, system_a_data: List[float], system_b_data: List[float],
                       system_a_name: str = "System A", 
                       system_b_name: str = "System B") -> Dict[str, Any]:
        """
        Comprehensive comparison of two systems with all statistical tests.
        
        Args:
            system_a_data: Performance data for system A
            system_b_data: Performance data for system B
            system_a_name: Name of system A (for neutral reporting)
            system_b_name: Name of system B (for neutral reporting)
            
        Returns:
            Dictionary with all comparison results
        """
        # Calculate descriptive statistics
        stats_a = self.descriptive_statistics(system_a_data)
        stats_b = self.descriptive_statistics(system_b_data)
        
        # Perform tests
        t_test_result = self.t_test(system_a_data, system_b_data)
        mann_whitney_result = self.mann_whitney_u(system_a_data, system_b_data)
        
        # Confidence intervals
        ci_a = self.confidence_interval(system_a_data)
        ci_b = self.confidence_interval(system_b_data)
        
        return {
            'system_a': {
                'name': system_a_name,
                'statistics': stats_a,
                'confidence_interval_95': ci_a
            },
            'system_b': {
                'name': system_b_name,
                'statistics': stats_b,
                'confidence_interval_95': ci_b
            },
            'tests': {
                't_test': t_test_result,
                'mann_whitney_u': mann_whitney_result
            },
            'warnings': self.warnings
        }
