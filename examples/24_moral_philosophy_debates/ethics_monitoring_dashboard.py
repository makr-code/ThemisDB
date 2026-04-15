"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_monitoring_dashboard.py                     ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1421                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Ethics Monitoring Dashboard for Real-Time AI Ethics Tracking

This module provides comprehensive real-time monitoring and visualization
capabilities for the ethical AI system, including:

1. MetricsAggregator - Collect and aggregate metrics over time
2. DashboardRenderer - Visualize metrics in multiple formats
3. AnomalyDetector - Detect statistical outliers and degradation
4. AlertSystem - Generate alerts for critical issues

Features:
- Time-series data management with rolling windows
- Multiple visualization formats (terminal, JSON, Prometheus)
- Anomaly detection and threshold alerts
- Historical tracking and trend analysis
- Grafana integration support

Author: ThemisDB Ethics AI Framework
License: MIT
"""

import json
import statistics
from dataclasses import dataclass, field, asdict
from datetime import datetime, timedelta
from typing import List, Dict, Any, Optional, Tuple, Set
from enum import Enum
from collections import defaultdict, deque
import time
import math


class TimeWindow(Enum):
    """Time window periods for metric aggregation."""
    REAL_TIME = "real_time"
    HOURLY = "hourly"
    DAILY = "daily"
    WEEKLY = "weekly"
    MONTHLY = "monthly"


class AlertSeverity(Enum):
    """Alert severity levels."""
    INFO = "info"
    WARNING = "warning"
    CRITICAL = "critical"


class AnomalyType(Enum):
    """Types of anomalies detected."""
    STATISTICAL_OUTLIER = "statistical_outlier"
    THRESHOLD_BREACH = "threshold_breach"
    QUALITY_DEGRADATION = "quality_degradation"
    BIAS_DETECTION = "bias_detection"
    CONSISTENCY_DROP = "consistency_drop"


@dataclass
class MetricDataPoint:
    """
    Single data point in time-series metrics.
    
    Attributes:
        timestamp: When the metric was recorded
        metric_name: Name of the metric
        value: Metric value
        dimension: Which ethics dimension (quality, fairness, etc.)
        metadata: Additional context about the measurement
    """
    timestamp: datetime
    metric_name: str
    value: float
    dimension: str
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'timestamp': self.timestamp.isoformat(),
            'metric_name': self.metric_name,
            'value': self.value,
            'dimension': self.dimension,
            'metadata': self.metadata
        }


@dataclass
class TimeSeriesData:
    """
    Time-series data for a specific metric.
    
    Attributes:
        metric_name: Name of the metric
        data_points: Deque of data points (fixed size for memory efficiency)
        max_size: Maximum number of data points to retain
    """
    metric_name: str
    data_points: deque = field(default_factory=lambda: deque(maxlen=10000))
    max_size: int = 10000
    
    def add_point(self, point: MetricDataPoint) -> None:
        """Add a data point to the time series."""
        self.data_points.append(point)
    
    def get_window(
        self,
        start_time: datetime,
        end_time: Optional[datetime] = None
    ) -> List[MetricDataPoint]:
        """Get data points within a time window."""
        if end_time is None:
            end_time = datetime.now()
        
        return [
            point for point in self.data_points
            if start_time <= point.timestamp <= end_time
        ]
    
    def get_recent(self, count: int = 100) -> List[MetricDataPoint]:
        """Get the most recent N data points."""
        return list(self.data_points)[-count:]
    
    def calculate_statistics(
        self,
        window: Optional[List[MetricDataPoint]] = None
    ) -> Dict[str, float]:
        """Calculate statistics for a time window."""
        if window is None:
            window = list(self.data_points)
        
        if not window:
            return {
                'mean': 0.0,
                'median': 0.0,
                'std': 0.0,
                'min': 0.0,
                'max': 0.0,
                'count': 0
            }
        
        values = [point.value for point in window]
        
        return {
            'mean': statistics.mean(values),
            'median': statistics.median(values),
            'std': statistics.stdev(values) if len(values) > 1 else 0.0,
            'min': min(values),
            'max': max(values),
            'count': len(values)
        }


@dataclass
class AggregatedMetrics:
    """
    Aggregated metrics for a time window.
    
    Attributes:
        window: Time window type
        start_time: Window start
        end_time: Window end
        dimension_scores: Average scores by dimension
        overall_score: Overall average score
        data_points: Number of evaluations in window
        trends: Trend information (increasing/decreasing)
    """
    window: TimeWindow
    start_time: datetime
    end_time: datetime
    dimension_scores: Dict[str, float] = field(default_factory=dict)
    overall_score: float = 0.0
    data_points: int = 0
    trends: Dict[str, str] = field(default_factory=dict)
    statistics: Dict[str, Dict[str, float]] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'window': self.window.value,
            'start_time': self.start_time.isoformat(),
            'end_time': self.end_time.isoformat(),
            'dimension_scores': self.dimension_scores,
            'overall_score': self.overall_score,
            'data_points': self.data_points,
            'trends': self.trends,
            'statistics': self.statistics
        }


@dataclass
class Alert:
    """
    Alert for anomalous behavior or threshold breaches.
    
    Attributes:
        alert_id: Unique alert identifier
        timestamp: When alert was generated
        severity: Alert severity level
        anomaly_type: Type of anomaly detected
        metric_name: Affected metric
        value: Current value
        threshold: Threshold that was breached (if applicable)
        message: Human-readable description
        context: Additional context information
    """
    alert_id: str
    timestamp: datetime
    severity: AlertSeverity
    anomaly_type: AnomalyType
    metric_name: str
    value: float
    threshold: Optional[float] = None
    message: str = ""
    context: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'alert_id': self.alert_id,
            'timestamp': self.timestamp.isoformat(),
            'severity': self.severity.value,
            'anomaly_type': self.anomaly_type.value,
            'metric_name': self.metric_name,
            'value': self.value,
            'threshold': self.threshold,
            'message': self.message,
            'context': self.context
        }


class MetricsAggregator:
    """
    Aggregate and manage time-series metrics data.
    
    This class collects metrics from the ethics evaluation system and
    provides rolling window analysis, trend detection, and statistical
    aggregation capabilities.
    
    Example:
        >>> aggregator = MetricsAggregator()
        >>> from ethics_evaluation_metrics import quick_evaluate
        >>> result = quick_evaluate(decision)
        >>> aggregator.ingest_evaluation(result)
        >>> hourly = aggregator.get_aggregated_metrics(TimeWindow.HOURLY)
        >>> print(f"Hourly average: {hourly.overall_score:.3f}")
    """
    
    def __init__(self, max_data_points: int = 10000):
        """
        Initialize metrics aggregator.
        
        Args:
            max_data_points: Maximum data points to retain per metric
        """
        self.time_series: Dict[str, TimeSeriesData] = {}
        self.max_data_points = max_data_points
        self.start_time = datetime.now()
        
        # Track dimension names
        self.dimensions = [
            'decision_quality',
            'consistency',
            'fairness',
            'alignment',
            'transparency',
            'overall'
        ]
    
    def ingest_evaluation(self, evaluation_result: Any) -> None:
        """
        Ingest an ethics evaluation result.
        
        Args:
            evaluation_result: EthicsEvaluationResult from ethics_evaluation_metrics
        """
        timestamp = datetime.now()
        
        # Extract scores from evaluation result
        scores = {
            'decision_quality': evaluation_result.decision_quality.overall_score,
            'consistency': evaluation_result.consistency.overall_score,
            'fairness': evaluation_result.fairness.overall_score,
            'alignment': evaluation_result.alignment.overall_score,
            'transparency': evaluation_result.transparency.overall_score,
            'overall': evaluation_result.overall_score
        }
        
        # Add data points for each dimension
        for dimension, score in scores.items():
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in self.time_series:
                self.time_series[metric_name] = TimeSeriesData(
                    metric_name=metric_name,
                    max_size=self.max_data_points
                )
            
            point = MetricDataPoint(
                timestamp=timestamp,
                metric_name=metric_name,
                value=score,
                dimension=dimension,
                metadata={
                    'evaluation_id': evaluation_result.evaluation_id
                }
            )
            
            self.time_series[metric_name].add_point(point)
        
        # Also track individual sub-metrics for detailed analysis
        self._ingest_detailed_metrics(timestamp, evaluation_result)
    
    def _ingest_detailed_metrics(
        self,
        timestamp: datetime,
        evaluation_result: Any
    ) -> None:
        """Ingest detailed sub-metrics."""
        detailed_metrics = {
            'outcome_satisfaction': evaluation_result.decision_quality.outcome_satisfaction,
            'ethical_alignment': evaluation_result.decision_quality.ethical_alignment,
            'feasibility': evaluation_result.decision_quality.feasibility,
            'long_term_impact': evaluation_result.decision_quality.long_term_impact,
            'intra_case_consistency': evaluation_result.consistency.intra_case_consistency,
            'inter_case_consistency': evaluation_result.consistency.inter_case_consistency,
            'philosophy_consistency': evaluation_result.consistency.philosophy_consistency,
            'temporal_consistency': evaluation_result.consistency.temporal_consistency,
            'demographic_parity': evaluation_result.fairness.demographic_parity,
            'equalized_odds': evaluation_result.fairness.equalized_odds,
            'individual_fairness': evaluation_result.fairness.individual_fairness,
            'principle_adherence': evaluation_result.alignment.principle_adherence,
            'constitutional_compliance': evaluation_result.alignment.constitutional_compliance,
            'value_alignment': evaluation_result.alignment.value_alignment,
            'constraint_satisfaction': evaluation_result.alignment.constraint_satisfaction,
            'explanation_completeness': evaluation_result.transparency.explanation_completeness,
            'reasoning_clarity': evaluation_result.transparency.reasoning_clarity,
            'justification_robustness': evaluation_result.transparency.justification_robustness
        }
        
        for metric_name, value in detailed_metrics.items():
            full_name = f"ethics_{metric_name}"
            
            if full_name not in self.time_series:
                self.time_series[full_name] = TimeSeriesData(
                    metric_name=full_name,
                    max_size=self.max_data_points
                )
            
            # Determine dimension for this metric
            dimension = self._determine_dimension(metric_name)
            
            point = MetricDataPoint(
                timestamp=timestamp,
                metric_name=full_name,
                value=value,
                dimension=dimension,
                metadata={'evaluation_id': evaluation_result.evaluation_id}
            )
            
            self.time_series[full_name].add_point(point)
    
    def _determine_dimension(self, metric_name: str) -> str:
        """Determine which dimension a metric belongs to."""
        dimension_keywords = {
            'decision_quality': ['satisfaction', 'feasibility', 'impact'],
            'consistency': ['consistency'],
            'fairness': ['parity', 'odds', 'fairness'],
            'alignment': ['adherence', 'alignment', 'philosophy'],
            'transparency': ['explanation', 'reasoning', 'clarity', 'completeness']
        }
        
        for dimension, keywords in dimension_keywords.items():
            if any(keyword in metric_name for keyword in keywords):
                return dimension
        
        return 'other'
    
    def get_aggregated_metrics(
        self,
        window: TimeWindow,
        end_time: Optional[datetime] = None
    ) -> AggregatedMetrics:
        """
        Get aggregated metrics for a time window.
        
        Args:
            window: Time window to aggregate over
            end_time: End of window (defaults to now)
        
        Returns:
            AggregatedMetrics with aggregated data
        """
        if end_time is None:
            end_time = datetime.now()
        
        # Calculate start time based on window
        start_time = self._calculate_start_time(window, end_time)
        
        # Aggregate dimension scores
        dimension_scores = {}
        statistics_dict = {}
        total_points = 0
        
        for dimension in self.dimensions:
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in self.time_series:
                continue
            
            window_data = self.time_series[metric_name].get_window(start_time, end_time)
            
            if window_data:
                values = [point.value for point in window_data]
                dimension_scores[dimension] = statistics.mean(values)
                statistics_dict[dimension] = self.time_series[metric_name].calculate_statistics(window_data)
                total_points = max(total_points, len(window_data))
        
        # Calculate overall score
        overall_score = 0.0
        if 'overall' in dimension_scores:
            overall_score = dimension_scores['overall']
        elif dimension_scores:
            # Exclude 'overall' if it exists
            dimension_values = [
                v for k, v in dimension_scores.items()
                if k != 'overall'
            ]
            if dimension_values:
                overall_score = statistics.mean(dimension_values)
        
        # Calculate trends
        trends = self._calculate_trends(window, end_time)
        
        return AggregatedMetrics(
            window=window,
            start_time=start_time,
            end_time=end_time,
            dimension_scores=dimension_scores,
            overall_score=overall_score,
            data_points=total_points,
            trends=trends,
            statistics=statistics_dict
        )
    
    def _calculate_start_time(
        self,
        window: TimeWindow,
        end_time: datetime
    ) -> datetime:
        """Calculate start time for a time window."""
        if window == TimeWindow.REAL_TIME:
            return end_time - timedelta(minutes=5)
        elif window == TimeWindow.HOURLY:
            return end_time - timedelta(hours=1)
        elif window == TimeWindow.DAILY:
            return end_time - timedelta(days=1)
        elif window == TimeWindow.WEEKLY:
            return end_time - timedelta(weeks=1)
        elif window == TimeWindow.MONTHLY:
            return end_time - timedelta(days=30)
        else:
            return end_time - timedelta(hours=1)
    
    def _calculate_trends(
        self,
        window: TimeWindow,
        end_time: datetime
    ) -> Dict[str, str]:
        """
        Calculate trend direction for each dimension.
        
        Returns:
            Dictionary mapping dimension to trend ('increasing', 'decreasing', 'stable')
        """
        trends = {}
        
        # Calculate window boundaries directly (avoid recursion)
        current_start = self._calculate_start_time(window, end_time)
        window_duration = end_time - current_start
        previous_end = current_start
        previous_start = previous_end - window_duration
        
        for dimension in self.dimensions:
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in self.time_series:
                trends[dimension] = 'unknown'
                continue
            
            current_data = self.time_series[metric_name].get_window(
                current_start,
                end_time
            )
            previous_data = self.time_series[metric_name].get_window(
                previous_start,
                previous_end
            )
            
            if not current_data or not previous_data:
                trends[dimension] = 'unknown'
                continue
            
            current_avg = statistics.mean([p.value for p in current_data])
            previous_avg = statistics.mean([p.value for p in previous_data])
            
            difference = current_avg - previous_avg
            
            if abs(difference) < 0.02:  # 2% threshold for "stable"
                trends[dimension] = 'stable'
            elif difference > 0:
                trends[dimension] = 'increasing'
            else:
                trends[dimension] = 'decreasing'
        
        return trends
    
    def get_metric_history(
        self,
        metric_name: str,
        start_time: Optional[datetime] = None,
        end_time: Optional[datetime] = None
    ) -> List[MetricDataPoint]:
        """
        Get historical data for a specific metric.
        
        Args:
            metric_name: Name of the metric
            start_time: Start of time range
            end_time: End of time range
        
        Returns:
            List of data points
        """
        if metric_name not in self.time_series:
            return []
        
        if start_time is None:
            start_time = self.start_time
        
        if end_time is None:
            end_time = datetime.now()
        
        return self.time_series[metric_name].get_window(start_time, end_time)


class AnomalyDetector:
    """
    Detect anomalies in ethics metrics.
    
    Uses statistical methods to identify outliers, threshold breaches,
    quality degradation, and bias patterns.
    
    Example:
        >>> detector = AnomalyDetector()
        >>> aggregator = MetricsAggregator()
        >>> # ... collect some data ...
        >>> anomalies = detector.detect_anomalies(aggregator)
        >>> for alert in anomalies:
        ...     print(f"Alert: {alert.message}")
    """
    
    def __init__(
        self,
        outlier_threshold: float = 3.0,
        quality_threshold: float = 0.6,
        bias_threshold: float = 0.15
    ):
        """
        Initialize anomaly detector.
        
        Args:
            outlier_threshold: Standard deviations for outlier detection
            quality_threshold: Minimum acceptable quality score
            bias_threshold: Maximum acceptable bias (fairness disparity)
        """
        self.outlier_threshold = outlier_threshold
        self.quality_threshold = quality_threshold
        self.bias_threshold = bias_threshold
        self.alert_history: List[Alert] = []
    
    def detect_anomalies(
        self,
        aggregator: MetricsAggregator,
        window: TimeWindow = TimeWindow.HOURLY
    ) -> List[Alert]:
        """
        Detect anomalies in recent metrics.
        
        Args:
            aggregator: MetricsAggregator instance with data
            window: Time window to analyze
        
        Returns:
            List of Alert objects
        """
        alerts = []
        
        # Get recent metrics
        metrics = aggregator.get_aggregated_metrics(window)
        
        # Check for statistical outliers
        alerts.extend(self._detect_statistical_outliers(aggregator, window))
        
        # Check for threshold breaches
        alerts.extend(self._detect_threshold_breaches(metrics))
        
        # Check for quality degradation
        alerts.extend(self._detect_quality_degradation(aggregator))
        
        # Check for bias patterns
        alerts.extend(self._detect_bias(metrics))
        
        # Store alerts in history
        self.alert_history.extend(alerts)
        
        return alerts
    
    def _detect_statistical_outliers(
        self,
        aggregator: MetricsAggregator,
        window: TimeWindow
    ) -> List[Alert]:
        """Detect statistical outliers using z-scores."""
        alerts = []
        
        for dimension in aggregator.dimensions:
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in aggregator.time_series:
                continue
            
            # Get recent data
            recent = aggregator.time_series[metric_name].get_recent(100)
            
            if len(recent) < 10:
                continue
            
            values = [point.value for point in recent]
            mean = statistics.mean(values)
            std = statistics.stdev(values) if len(values) > 1 else 0
            
            if std == 0:
                continue
            
            # Check most recent value
            latest = recent[-1]
            z_score = abs((latest.value - mean) / std)
            
            if z_score > self.outlier_threshold:
                alert = Alert(
                    alert_id=f"outlier_{dimension}_{int(time.time())}",
                    timestamp=datetime.now(),
                    severity=AlertSeverity.WARNING if z_score < 4.0 else AlertSeverity.CRITICAL,
                    anomaly_type=AnomalyType.STATISTICAL_OUTLIER,
                    metric_name=metric_name,
                    value=latest.value,
                    threshold=mean,
                    message=f"Statistical outlier detected in {dimension}: "
                            f"value={latest.value:.3f}, z-score={z_score:.2f}",
                    context={
                        'z_score': z_score,
                        'mean': mean,
                        'std': std,
                        'dimension': dimension
                    }
                )
                alerts.append(alert)
        
        return alerts
    
    def _detect_threshold_breaches(
        self,
        metrics: AggregatedMetrics
    ) -> List[Alert]:
        """Detect threshold breaches in metrics."""
        alerts = []
        
        for dimension, score in metrics.dimension_scores.items():
            if score < self.quality_threshold:
                severity = AlertSeverity.WARNING if score > 0.5 else AlertSeverity.CRITICAL
                
                alert = Alert(
                    alert_id=f"threshold_{dimension}_{int(time.time())}",
                    timestamp=datetime.now(),
                    severity=severity,
                    anomaly_type=AnomalyType.THRESHOLD_BREACH,
                    metric_name=f"ethics_{dimension}_score",
                    value=score,
                    threshold=self.quality_threshold,
                    message=f"Quality threshold breached for {dimension}: "
                            f"{score:.3f} < {self.quality_threshold:.3f}",
                    context={
                        'dimension': dimension,
                        'threshold': self.quality_threshold
                    }
                )
                alerts.append(alert)
        
        return alerts
    
    def _detect_quality_degradation(
        self,
        aggregator: MetricsAggregator
    ) -> List[Alert]:
        """Detect quality degradation over time."""
        alerts = []
        
        # Compare last hour vs previous hour
        current = aggregator.get_aggregated_metrics(TimeWindow.HOURLY)
        
        if current.data_points < 5:
            return alerts
        
        # Get previous hour
        previous_end = current.start_time
        previous_start = previous_end - timedelta(hours=1)
        
        for dimension in aggregator.dimensions:
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in aggregator.time_series:
                continue
            
            current_data = aggregator.time_series[metric_name].get_window(
                current.start_time,
                current.end_time
            )
            previous_data = aggregator.time_series[metric_name].get_window(
                previous_start,
                previous_end
            )
            
            if not current_data or not previous_data:
                continue
            
            current_avg = statistics.mean([p.value for p in current_data])
            previous_avg = statistics.mean([p.value for p in previous_data])
            
            degradation = previous_avg - current_avg
            
            # Alert if degradation > 10%
            if degradation > 0.10:
                alert = Alert(
                    alert_id=f"degradation_{dimension}_{int(time.time())}",
                    timestamp=datetime.now(),
                    severity=AlertSeverity.WARNING if degradation < 0.20 else AlertSeverity.CRITICAL,
                    anomaly_type=AnomalyType.QUALITY_DEGRADATION,
                    metric_name=metric_name,
                    value=current_avg,
                    threshold=previous_avg,
                    message=f"Quality degradation detected in {dimension}: "
                            f"{previous_avg:.3f} → {current_avg:.3f} "
                            f"({degradation*100:.1f}% decrease)",
                    context={
                        'dimension': dimension,
                        'current': current_avg,
                        'previous': previous_avg,
                        'degradation_pct': degradation * 100
                    }
                )
                alerts.append(alert)
        
        return alerts
    
    def _detect_bias(self, metrics: AggregatedMetrics) -> List[Alert]:
        """Detect bias in fairness metrics."""
        alerts = []
        
        # Check fairness dimension
        if 'fairness' not in metrics.dimension_scores:
            return alerts
        
        fairness_score = metrics.dimension_scores['fairness']
        
        # Low fairness score indicates potential bias
        if fairness_score < (1.0 - self.bias_threshold):
            bias_level = 1.0 - fairness_score
            
            alert = Alert(
                alert_id=f"bias_{int(time.time())}",
                timestamp=datetime.now(),
                severity=AlertSeverity.WARNING if bias_level < 0.25 else AlertSeverity.CRITICAL,
                anomaly_type=AnomalyType.BIAS_DETECTION,
                metric_name="ethics_fairness_score",
                value=fairness_score,
                threshold=1.0 - self.bias_threshold,
                message=f"Potential bias detected: fairness score = {fairness_score:.3f}, "
                        f"bias level = {bias_level:.3f}",
                context={
                    'fairness_score': fairness_score,
                    'bias_level': bias_level,
                    'threshold': self.bias_threshold
                }
            )
            alerts.append(alert)
        
        return alerts
    
    def get_recent_alerts(
        self,
        severity: Optional[AlertSeverity] = None,
        hours: int = 24
    ) -> List[Alert]:
        """
        Get recent alerts.
        
        Args:
            severity: Filter by severity level
            hours: Number of hours to look back
        
        Returns:
            List of recent alerts
        """
        cutoff = datetime.now() - timedelta(hours=hours)
        
        recent = [
            alert for alert in self.alert_history
            if alert.timestamp >= cutoff
        ]
        
        if severity:
            recent = [
                alert for alert in recent
                if alert.severity == severity
            ]
        
        return recent


class DashboardRenderer:
    """
    Render ethics monitoring dashboard in multiple formats.
    
    Supports:
    - Terminal/ASCII visualization
    - JSON export for web dashboards
    - Prometheus metrics format
    - Grafana integration
    
    Example:
        >>> renderer = DashboardRenderer()
        >>> aggregator = MetricsAggregator()
        >>> # ... collect data ...
        >>> renderer.render_terminal(aggregator)
        >>> json_data = renderer.export_json(aggregator)
        >>> prometheus = renderer.export_prometheus(aggregator)
    """
    
    def __init__(self, width: int = 80):
        """
        Initialize dashboard renderer.
        
        Args:
            width: Terminal width for ASCII rendering
        """
        self.width = width
    
    def render_terminal(
        self,
        aggregator: MetricsAggregator,
        detector: Optional[AnomalyDetector] = None,
        window: TimeWindow = TimeWindow.HOURLY
    ) -> str:
        """
        Render terminal dashboard.
        
        Args:
            aggregator: MetricsAggregator with data
            detector: Optional AnomalyDetector for alerts
            window: Time window to display
        
        Returns:
            ASCII dashboard as string
        """
        lines = []
        
        # Header
        lines.append("=" * self.width)
        lines.append("ETHICS MONITORING DASHBOARD".center(self.width))
        lines.append("=" * self.width)
        lines.append("")
        
        # Get metrics
        metrics = aggregator.get_aggregated_metrics(window)
        
        # Timestamp
        lines.append(f"Time: {metrics.end_time.strftime('%Y-%m-%d %H:%M:%S')}")
        lines.append(f"Window: {window.value} ({metrics.data_points} evaluations)")
        lines.append("")
        
        # Overall score
        lines.append("─" * self.width)
        lines.append("OVERALL ETHICS SCORE")
        lines.append("─" * self.width)
        score_bar = self._render_bar(metrics.overall_score, self.width - 20)
        lines.append(f"  {score_bar} {metrics.overall_score:.3f}")
        lines.append("")
        
        # Dimension breakdown
        lines.append("─" * self.width)
        lines.append("DIMENSION BREAKDOWN")
        lines.append("─" * self.width)
        
        dimension_labels = {
            'decision_quality': 'Decision Quality',
            'consistency': 'Consistency',
            'fairness': 'Fairness',
            'alignment': 'Alignment',
            'transparency': 'Transparency'
        }
        
        for dim_key, dim_label in dimension_labels.items():
            if dim_key in metrics.dimension_scores:
                score = metrics.dimension_scores[dim_key]
                trend = metrics.trends.get(dim_key, 'unknown')
                trend_symbol = self._get_trend_symbol(trend)
                
                bar = self._render_bar(score, self.width - 35)
                lines.append(f"  {dim_label:20} {bar} {score:.3f} {trend_symbol}")
        
        lines.append("")
        
        # Statistics
        if metrics.statistics:
            lines.append("─" * self.width)
            lines.append("STATISTICS (Current Window)")
            lines.append("─" * self.width)
            
            for dim_key, dim_label in dimension_labels.items():
                if dim_key in metrics.statistics:
                    stats = metrics.statistics[dim_key]
                    lines.append(f"  {dim_label}:")
                    lines.append(f"    Mean: {stats['mean']:.3f}  "
                               f"Median: {stats['median']:.3f}  "
                               f"Std: {stats['std']:.3f}")
                    lines.append(f"    Min: {stats['min']:.3f}  "
                               f"Max: {stats['max']:.3f}  "
                               f"Count: {stats['count']}")
            
            lines.append("")
        
        # Alerts
        if detector:
            recent_alerts = detector.get_recent_alerts(hours=1)
            
            if recent_alerts:
                lines.append("─" * self.width)
                lines.append(f"RECENT ALERTS ({len(recent_alerts)})")
                lines.append("─" * self.width)
                
                for alert in recent_alerts[-5:]:  # Show last 5 alerts
                    severity_symbol = {
                        AlertSeverity.INFO: 'ℹ',
                        AlertSeverity.WARNING: '⚠',
                        AlertSeverity.CRITICAL: '✗'
                    }.get(alert.severity, '•')
                    
                    time_str = alert.timestamp.strftime('%H:%M:%S')
                    lines.append(f"  {severity_symbol} [{time_str}] {alert.message}")
                
                lines.append("")
        
        lines.append("=" * self.width)
        
        return "\n".join(lines)
    
    def _render_bar(self, value: float, width: int = 40) -> str:
        """Render a progress bar."""
        filled = int(value * width)
        empty = width - filled
        
        # Color coding (simulated with characters)
        if value >= 0.8:
            char = '█'
        elif value >= 0.6:
            char = '▓'
        else:
            char = '▒'
        
        return '[' + char * filled + '·' * empty + ']'
    
    def _get_trend_symbol(self, trend: str) -> str:
        """Get symbol for trend."""
        symbols = {
            'increasing': '↗',
            'decreasing': '↘',
            'stable': '→',
            'unknown': '?'
        }
        return symbols.get(trend, '?')
    
    def export_json(
        self,
        aggregator: MetricsAggregator,
        detector: Optional[AnomalyDetector] = None,
        windows: Optional[List[TimeWindow]] = None
    ) -> Dict[str, Any]:
        """
        Export dashboard data as JSON.
        
        Args:
            aggregator: MetricsAggregator with data
            detector: Optional AnomalyDetector for alerts
            windows: Time windows to include
        
        Returns:
            Dictionary suitable for JSON serialization
        """
        if windows is None:
            windows = [TimeWindow.HOURLY, TimeWindow.DAILY, TimeWindow.WEEKLY]
        
        data = {
            'timestamp': datetime.now().isoformat(),
            'windows': {}
        }
        
        # Add metrics for each window
        for window in windows:
            metrics = aggregator.get_aggregated_metrics(window)
            data['windows'][window.value] = metrics.to_dict()
        
        # Add alerts
        if detector:
            data['alerts'] = {
                'recent_1h': [
                    alert.to_dict()
                    for alert in detector.get_recent_alerts(hours=1)
                ],
                'recent_24h': [
                    alert.to_dict()
                    for alert in detector.get_recent_alerts(hours=24)
                ]
            }
        
        # Add metadata
        data['metadata'] = {
            'total_evaluations': sum(
                len(ts.data_points)
                for ts in aggregator.time_series.values()
            ) // len(aggregator.dimensions),
            'monitoring_since': aggregator.start_time.isoformat(),
            'dimensions_tracked': aggregator.dimensions
        }
        
        return data
    
    def export_prometheus(
        self,
        aggregator: MetricsAggregator,
        window: TimeWindow = TimeWindow.HOURLY
    ) -> str:
        """
        Export metrics in Prometheus format.
        
        Args:
            aggregator: MetricsAggregator with data
            window: Time window to export
        
        Returns:
            Prometheus metrics text format
        """
        lines = []
        
        metrics = aggregator.get_aggregated_metrics(window)
        timestamp = int(metrics.end_time.timestamp() * 1000)
        
        # Overall score
        lines.append("# HELP ethics_overall_score Overall ethics evaluation score")
        lines.append("# TYPE ethics_overall_score gauge")
        lines.append(f'ethics_overall_score{{window="{window.value}"}} '
                    f'{metrics.overall_score} {timestamp}')
        lines.append("")
        
        # Dimension scores
        lines.append("# HELP ethics_dimension_score Ethics score by dimension")
        lines.append("# TYPE ethics_dimension_score gauge")
        
        for dimension, score in metrics.dimension_scores.items():
            if dimension != 'overall':
                lines.append(
                    f'ethics_dimension_score{{dimension="{dimension}",window="{window.value}"}} '
                    f'{score} {timestamp}'
                )
        
        lines.append("")
        
        # Data points count
        lines.append("# HELP ethics_evaluations_total Total number of evaluations")
        lines.append("# TYPE ethics_evaluations_total counter")
        lines.append(f'ethics_evaluations_total{{window="{window.value}"}} '
                    f'{metrics.data_points} {timestamp}')
        lines.append("")
        
        # Statistics
        for dimension, stats in metrics.statistics.items():
            if dimension != 'overall':
                # Mean
                lines.append(
                    f'ethics_dimension_mean{{dimension="{dimension}",window="{window.value}"}} '
                    f'{stats["mean"]} {timestamp}'
                )
                
                # Std dev
                lines.append(
                    f'ethics_dimension_stddev{{dimension="{dimension}",window="{window.value}"}} '
                    f'{stats["std"]} {timestamp}'
                )
        
        return "\n".join(lines)
    
    def export_grafana_json(
        self,
        aggregator: MetricsAggregator,
        detector: Optional[AnomalyDetector] = None
    ) -> Dict[str, Any]:
        """
        Export data in Grafana-compatible JSON format.
        
        Args:
            aggregator: MetricsAggregator with data
            detector: Optional AnomalyDetector for alerts
        
        Returns:
            Grafana JSON API compatible dictionary
        """
        # Get time-series data for graphing
        time_series_data = []
        
        for dimension in aggregator.dimensions:
            metric_name = f"ethics_{dimension}_score"
            
            if metric_name not in aggregator.time_series:
                continue
            
            recent = aggregator.time_series[metric_name].get_recent(100)
            
            if not recent:
                continue
            
            datapoints = [
                [point.value, int(point.timestamp.timestamp() * 1000)]
                for point in recent
            ]
            
            time_series_data.append({
                'target': dimension,
                'datapoints': datapoints
            })
        
        # Prepare annotations (alerts)
        annotations = []
        
        if detector:
            for alert in detector.get_recent_alerts(hours=24):
                annotations.append({
                    'time': int(alert.timestamp.timestamp() * 1000),
                    'title': alert.anomaly_type.value,
                    'text': alert.message,
                    'tags': [alert.severity.value, alert.metric_name]
                })
        
        return {
            'series': time_series_data,
            'annotations': annotations
        }


def create_monitoring_system(
    outlier_threshold: float = 3.0,
    quality_threshold: float = 0.6,
    bias_threshold: float = 0.15
) -> Tuple[MetricsAggregator, AnomalyDetector, DashboardRenderer]:
    """
    Create a complete monitoring system.
    
    Args:
        outlier_threshold: Z-score threshold for outlier detection
        quality_threshold: Minimum acceptable quality score
        bias_threshold: Maximum acceptable bias level
    
    Returns:
        Tuple of (aggregator, detector, renderer)
    
    Example:
        >>> aggregator, detector, renderer = create_monitoring_system()
        >>> 
        >>> # In your evaluation loop:
        >>> from ethics_evaluation_metrics import quick_evaluate
        >>> result = quick_evaluate(decision)
        >>> aggregator.ingest_evaluation(result)
        >>> 
        >>> # Display dashboard
        >>> dashboard = renderer.render_terminal(aggregator, detector)
        >>> print(dashboard)
        >>> 
        >>> # Check for anomalies
        >>> alerts = detector.detect_anomalies(aggregator)
        >>> for alert in alerts:
        ...     print(f"[{alert.severity.value}] {alert.message}")
    """
    aggregator = MetricsAggregator()
    detector = AnomalyDetector(
        outlier_threshold=outlier_threshold,
        quality_threshold=quality_threshold,
        bias_threshold=bias_threshold
    )
    renderer = DashboardRenderer()
    
    return aggregator, detector, renderer


# Example usage and demonstration
if __name__ == "__main__":
    print("Ethics Monitoring Dashboard - Example Usage\n")
    print("=" * 80)
    
    # Create monitoring system
    aggregator, detector, renderer = create_monitoring_system()
    
    print("\n1. Creating monitoring system...")
    print("   ✓ MetricsAggregator initialized")
    print("   ✓ AnomalyDetector configured")
    print("   ✓ DashboardRenderer ready")
    
    # Simulate some evaluations
    print("\n2. Simulating evaluation data...")
    
    # We'll create mock evaluation results
    from dataclasses import dataclass
    
    @dataclass
    class MockQualityMetrics:
        overall_score: float
        outcome_satisfaction: float
        ethical_alignment: float
        feasibility: float
        long_term_impact: float
    
    @dataclass
    class MockConsistencyMetrics:
        overall_score: float
        intra_case_consistency: float
        inter_case_consistency: float
        philosophy_consistency: float
        temporal_consistency: float
    
    @dataclass
    class MockFairnessMetrics:
        overall_score: float
        demographic_parity: float
        equalized_odds: float
        individual_fairness: float
    
    @dataclass
    class MockAlignmentMetrics:
        overall_score: float
        principle_adherence: float
        constitutional_compliance: float
        value_alignment: float
        constraint_satisfaction: float
    
    @dataclass
    class MockTransparencyMetrics:
        overall_score: float
        explanation_completeness: float
        reasoning_clarity: float
        justification_robustness: float
    
    @dataclass
    class MockEvaluation:
        evaluation_id: str
        overall_score: float
        decision_quality: MockQualityMetrics
        consistency: MockConsistencyMetrics
        fairness: MockFairnessMetrics
        alignment: MockAlignmentMetrics
        transparency: MockTransparencyMetrics
    
    import random
    
    # Simulate 50 evaluations
    for i in range(50):
        # Generate random but realistic scores
        base_score = 0.75 + random.uniform(-0.15, 0.15)
        
        evaluation = MockEvaluation(
            evaluation_id=f"eval_{i}",
            overall_score=base_score,
            decision_quality=MockQualityMetrics(
                overall_score=base_score + random.uniform(-0.1, 0.1),
                outcome_satisfaction=base_score + random.uniform(-0.1, 0.1),
                ethical_alignment=base_score + random.uniform(-0.1, 0.1),
                feasibility=base_score + random.uniform(-0.1, 0.1),
                long_term_impact=base_score + random.uniform(-0.1, 0.1)
            ),
            consistency=MockConsistencyMetrics(
                overall_score=base_score + random.uniform(-0.1, 0.1),
                intra_case_consistency=base_score + random.uniform(-0.1, 0.1),
                inter_case_consistency=base_score + random.uniform(-0.1, 0.1),
                philosophy_consistency=base_score + random.uniform(-0.1, 0.1),
                temporal_consistency=base_score + random.uniform(-0.1, 0.1)
            ),
            fairness=MockFairnessMetrics(
                overall_score=base_score + random.uniform(-0.1, 0.1),
                demographic_parity=base_score + random.uniform(-0.1, 0.1),
                equalized_odds=base_score + random.uniform(-0.1, 0.1),
                individual_fairness=base_score + random.uniform(-0.1, 0.1)
            ),
            alignment=MockAlignmentMetrics(
                overall_score=base_score + random.uniform(-0.1, 0.1),
                principle_adherence=base_score + random.uniform(-0.1, 0.1),
                constitutional_compliance=base_score + random.uniform(-0.1, 0.1),
                value_alignment=base_score + random.uniform(-0.1, 0.1),
                constraint_satisfaction=base_score + random.uniform(-0.1, 0.1)
            ),
            transparency=MockTransparencyMetrics(
                overall_score=base_score + random.uniform(-0.1, 0.1),
                explanation_completeness=base_score + random.uniform(-0.1, 0.1),
                reasoning_clarity=base_score + random.uniform(-0.1, 0.1),
                justification_robustness=base_score + random.uniform(-0.1, 0.1)
            )
        )
        
        aggregator.ingest_evaluation(evaluation)
    
    print(f"   ✓ Ingested {50} mock evaluations")
    
    # Render dashboard
    print("\n3. Rendering Terminal Dashboard:\n")
    dashboard = renderer.render_terminal(aggregator, detector, TimeWindow.HOURLY)
    print(dashboard)
    
    # Export formats
    print("\n4. Exporting data in multiple formats...\n")
    
    # JSON export
    json_data = renderer.export_json(aggregator, detector)
    print(f"   ✓ JSON export: {len(json.dumps(json_data))} bytes")
    print(f"     - Windows: {list(json_data['windows'].keys())}")
    print(f"     - Total evaluations: {json_data['metadata']['total_evaluations']}")
    
    # Prometheus export
    prometheus = renderer.export_prometheus(aggregator)
    print(f"   ✓ Prometheus export: {len(prometheus)} bytes")
    print(f"     - Metrics lines: {len(prometheus.split(chr(10)))}")
    
    # Grafana export
    grafana = renderer.export_grafana_json(aggregator, detector)
    print(f"   ✓ Grafana JSON: {len(grafana['series'])} time series")
    
    # Show aggregated metrics
    print("\n5. Aggregated Metrics:\n")
    
    for window in [TimeWindow.HOURLY, TimeWindow.DAILY]:
        metrics = aggregator.get_aggregated_metrics(window)
        print(f"   {window.value.upper()}")
        print(f"     Overall Score: {metrics.overall_score:.3f}")
        print(f"     Data Points: {metrics.data_points}")
        print(f"     Trends: {', '.join([f'{k}:{v}' for k,v in list(metrics.trends.items())[:3]])}")
        print()
    
    # Check for anomalies
    print("6. Anomaly Detection:\n")
    alerts = detector.detect_anomalies(aggregator, TimeWindow.HOURLY)
    
    if alerts:
        print(f"   Found {len(alerts)} anomalies:")
        for alert in alerts:
            print(f"     [{alert.severity.value}] {alert.message}")
    else:
        print("   ✓ No anomalies detected")
    
    print("\n" + "=" * 80)
    print("\nMonitoring system demonstration complete!")
    print("\nIntegration example:")
    print("```python")
    print("from ethics_evaluation_metrics import quick_evaluate")
    print("from ethics_monitoring_dashboard import create_monitoring_system")
    print("")
    print("# Initialize monitoring")
    print("aggregator, detector, renderer = create_monitoring_system()")
    print("")
    print("# In your evaluation loop:")
    print("result = quick_evaluate(decision)")
    print("aggregator.ingest_evaluation(result)")
    print("")
    print("# Display dashboard periodically")
    print("print(renderer.render_terminal(aggregator, detector))")
    print("")
    print("# Check for critical alerts")
    print("alerts = detector.detect_anomalies(aggregator)")
    print("critical = [a for a in alerts if a.severity == AlertSeverity.CRITICAL]")
    print("if critical:")
    print("    # Take action on critical alerts")
    print("    pass")
    print("```")
