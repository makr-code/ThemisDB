"""Metrics collection for RESPO."""

import time
from collections import defaultdict
from dataclasses import dataclass, field
from threading import Lock
from typing import Optional


@dataclass
class Counter:
    """A simple counter metric."""
    name: str
    help: str
    labels: tuple = ()
    _values: dict = field(default_factory=lambda: defaultdict(int))
    _lock: Lock = field(default_factory=Lock)
    
    def inc(self, labels: Optional[dict] = None, value: int = 1) -> None:
        """Increment counter."""
        label_key = tuple(sorted((labels or {}).items()))
        with self._lock:
            self._values[label_key] += value
    
    def get(self, labels: Optional[dict] = None) -> int:
        """Get counter value."""
        label_key = tuple(sorted((labels or {}).items()))
        return self._values.get(label_key, 0)
    
    def values(self) -> dict:
        """Get all values."""
        return dict(self._values)


@dataclass
class Gauge:
    """A simple gauge metric."""
    name: str
    help: str
    labels: tuple = ()
    _values: dict = field(default_factory=lambda: defaultdict(float))
    _lock: Lock = field(default_factory=Lock)
    
    def set(self, value: float, labels: Optional[dict] = None) -> None:
        """Set gauge value."""
        label_key = tuple(sorted((labels or {}).items()))
        with self._lock:
            self._values[label_key] = value
    
    def inc(self, labels: Optional[dict] = None, value: float = 1.0) -> None:
        """Increment gauge."""
        label_key = tuple(sorted((labels or {}).items()))
        with self._lock:
            self._values[label_key] += value
    
    def dec(self, labels: Optional[dict] = None, value: float = 1.0) -> None:
        """Decrement gauge."""
        label_key = tuple(sorted((labels or {}).items()))
        with self._lock:
            self._values[label_key] -= value
    
    def get(self, labels: Optional[dict] = None) -> float:
        """Get gauge value."""
        label_key = tuple(sorted((labels or {}).items()))
        return self._values.get(label_key, 0.0)
    
    def values(self) -> dict:
        """Get all values."""
        return dict(self._values)


@dataclass
class Histogram:
    """A simple histogram metric."""
    name: str
    help: str
    labels: tuple = ()
    buckets: tuple = (0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0)
    _observations: dict = field(default_factory=lambda: defaultdict(list))
    _lock: Lock = field(default_factory=Lock)
    
    def observe(self, value: float, labels: Optional[dict] = None) -> None:
        """Observe a value."""
        label_key = tuple(sorted((labels or {}).items()))
        with self._lock:
            self._observations[label_key].append(value)
    
    def get_buckets(self, labels: Optional[dict] = None) -> dict:
        """Get bucket counts."""
        label_key = tuple(sorted((labels or {}).items()))
        observations = self._observations.get(label_key, [])
        
        buckets = {}
        for bucket in self.buckets:
            buckets[bucket] = sum(1 for o in observations if o <= bucket)
        buckets[float("inf")] = len(observations)
        
        return buckets
    
    def get_sum(self, labels: Optional[dict] = None) -> float:
        """Get sum of observations."""
        label_key = tuple(sorted((labels or {}).items()))
        return sum(self._observations.get(label_key, []))
    
    def get_count(self, labels: Optional[dict] = None) -> int:
        """Get count of observations."""
        label_key = tuple(sorted((labels or {}).items()))
        return len(self._observations.get(label_key, []))
    
    def values(self) -> dict:
        """Get all observations."""
        return dict(self._observations)


class Timer:
    """Context manager for timing operations."""
    
    def __init__(self, histogram: Histogram, labels: Optional[dict] = None):
        self.histogram = histogram
        self.labels = labels
        self.start_time = None
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        return self
    
    def __exit__(self, *args):
        duration = time.perf_counter() - self.start_time
        self.histogram.observe(duration, self.labels)


class MetricsCollector:
    """Central metrics collector for RESPO."""
    
    def __init__(self):
        # Request metrics
        self.requests_total = Counter(
            name="respo_requests_total",
            help="Total number of requests",
            labels=("endpoint", "method", "status"),
        )
        self.request_duration = Histogram(
            name="respo_request_duration_seconds",
            help="Request duration in seconds",
            labels=("endpoint", "method"),
        )
        
        # LLM metrics
        self.llm_requests_total = Counter(
            name="respo_llm_requests_total",
            help="Total LLM requests",
            labels=("model", "task"),
        )
        self.llm_tokens_total = Counter(
            name="respo_llm_tokens_total",
            help="Total tokens processed",
            labels=("model", "type"),  # type: input, output
        )
        self.llm_latency = Histogram(
            name="respo_llm_latency_seconds",
            help="LLM request latency",
            labels=("model",),
        )
        
        # Embedding metrics
        self.embedding_requests_total = Counter(
            name="respo_embedding_requests_total",
            help="Total embedding requests",
            labels=("model",),
        )
        self.embedding_batch_size = Histogram(
            name="respo_embedding_batch_size",
            help="Embedding batch sizes",
            labels=(),
            buckets=(1, 5, 10, 25, 50, 100, 250, 500, 1000),
        )
        
        # Cache metrics
        self.cache_hits_total = Counter(
            name="respo_cache_hits_total",
            help="Cache hits",
            labels=("namespace",),
        )
        self.cache_misses_total = Counter(
            name="respo_cache_misses_total",
            help="Cache misses",
            labels=("namespace",),
        )
        
        # Vector store metrics
        self.vector_search_total = Counter(
            name="respo_vector_search_total",
            help="Vector search requests",
            labels=("backend",),
        )
        self.vector_search_latency = Histogram(
            name="respo_vector_search_latency_seconds",
            help="Vector search latency",
            labels=("backend",),
        )
        self.vector_store_size = Gauge(
            name="respo_vector_store_size",
            help="Number of vectors in store",
            labels=("backend", "collection"),
        )
        
        # Task metrics
        self.tasks_total = Counter(
            name="respo_tasks_total",
            help="Total tasks created",
            labels=("type", "status"),
        )
        self.active_tasks = Gauge(
            name="respo_active_tasks",
            help="Currently active tasks",
            labels=("type",),
        )
        
        # Agent metrics
        self.agent_plans_total = Counter(
            name="respo_agent_plans_total",
            help="Agent plans created",
            labels=(),
        )
        self.agent_steps_total = Counter(
            name="respo_agent_steps_total",
            help="Agent steps executed",
            labels=("status",),
        )
    
    def time_request(self, endpoint: str, method: str = "POST") -> Timer:
        """Create a timer for request duration."""
        return Timer(self.request_duration, {"endpoint": endpoint, "method": method})
    
    def time_llm(self, model: str) -> Timer:
        """Create a timer for LLM latency."""
        return Timer(self.llm_latency, {"model": model})
    
    def time_vector_search(self, backend: str) -> Timer:
        """Create a timer for vector search."""
        return Timer(self.vector_search_latency, {"backend": backend})
    
    def get_all_metrics(self) -> dict:
        """Get all metrics as a dictionary."""
        return {
            "requests": {
                "total": self.requests_total.values(),
                "duration": {
                    "count": self.request_duration.values(),
                },
            },
            "llm": {
                "requests": self.llm_requests_total.values(),
                "tokens": self.llm_tokens_total.values(),
            },
            "cache": {
                "hits": self.cache_hits_total.values(),
                "misses": self.cache_misses_total.values(),
            },
            "vector_store": {
                "searches": self.vector_search_total.values(),
                "size": self.vector_store_size.values(),
            },
            "tasks": {
                "total": self.tasks_total.values(),
                "active": self.active_tasks.values(),
            },
            "agents": {
                "plans": self.agent_plans_total.values(),
                "steps": self.agent_steps_total.values(),
            },
        }


# Global metrics collector
_metrics: Optional[MetricsCollector] = None


def get_metrics() -> MetricsCollector:
    """Get the global metrics collector."""
    global _metrics
    if _metrics is None:
        _metrics = MetricsCollector()
    return _metrics
