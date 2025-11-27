"""Prometheus metrics exporter for RESPO."""

from typing import Optional

from .metrics import MetricsCollector, Counter, Gauge, Histogram


class PrometheusExporter:
    """Export metrics in Prometheus format."""
    
    def __init__(self, collector: MetricsCollector):
        self.collector = collector
    
    def _format_labels(self, labels: tuple) -> str:
        """Format labels as Prometheus string."""
        if not labels:
            return ""
        parts = [f'{k}="{v}"' for k, v in labels]
        return "{" + ",".join(parts) + "}"
    
    def _export_counter(self, counter: Counter) -> list[str]:
        """Export a counter metric."""
        lines = [
            f"# HELP {counter.name} {counter.help}",
            f"# TYPE {counter.name} counter",
        ]
        for labels, value in counter.values().items():
            label_str = self._format_labels(labels)
            lines.append(f"{counter.name}{label_str} {value}")
        return lines
    
    def _export_gauge(self, gauge: Gauge) -> list[str]:
        """Export a gauge metric."""
        lines = [
            f"# HELP {gauge.name} {gauge.help}",
            f"# TYPE {gauge.name} gauge",
        ]
        for labels, value in gauge.values().items():
            label_str = self._format_labels(labels)
            lines.append(f"{gauge.name}{label_str} {value}")
        return lines
    
    def _export_histogram(self, histogram: Histogram) -> list[str]:
        """Export a histogram metric."""
        lines = [
            f"# HELP {histogram.name} {histogram.help}",
            f"# TYPE {histogram.name} histogram",
        ]
        
        for labels in histogram.values().keys():
            buckets = histogram.get_buckets(dict(labels) if labels else None)
            sum_val = histogram.get_sum(dict(labels) if labels else None)
            count_val = histogram.get_count(dict(labels) if labels else None)
            
            base_labels = self._format_labels(labels)
            
            for bucket, count in buckets.items():
                le = "+Inf" if bucket == float("inf") else str(bucket)
                if labels:
                    label_str = base_labels[:-1] + f',le="{le}"' + "}"
                else:
                    label_str = f'{{le="{le}"}}'
                lines.append(f"{histogram.name}_bucket{label_str} {count}")
            
            lines.append(f"{histogram.name}_sum{base_labels} {sum_val}")
            lines.append(f"{histogram.name}_count{base_labels} {count_val}")
        
        return lines
    
    def export(self) -> str:
        """Export all metrics in Prometheus format."""
        lines = []
        
        # Counters
        lines.extend(self._export_counter(self.collector.requests_total))
        lines.extend(self._export_counter(self.collector.llm_requests_total))
        lines.extend(self._export_counter(self.collector.llm_tokens_total))
        lines.extend(self._export_counter(self.collector.embedding_requests_total))
        lines.extend(self._export_counter(self.collector.cache_hits_total))
        lines.extend(self._export_counter(self.collector.cache_misses_total))
        lines.extend(self._export_counter(self.collector.vector_search_total))
        lines.extend(self._export_counter(self.collector.tasks_total))
        lines.extend(self._export_counter(self.collector.agent_plans_total))
        lines.extend(self._export_counter(self.collector.agent_steps_total))
        
        # Gauges
        lines.extend(self._export_gauge(self.collector.vector_store_size))
        lines.extend(self._export_gauge(self.collector.active_tasks))
        
        # Histograms
        lines.extend(self._export_histogram(self.collector.request_duration))
        lines.extend(self._export_histogram(self.collector.llm_latency))
        lines.extend(self._export_histogram(self.collector.embedding_batch_size))
        lines.extend(self._export_histogram(self.collector.vector_search_latency))
        
        return "\n".join(lines) + "\n"
