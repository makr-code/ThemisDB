> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Observability Module Roadmap

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · docs/de/observability/README.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Enterprise-grade observability stack. Prometheus metrics, query profiling, storage profiling, automated performance analysis, Alertmanager integration, distributed tracing, structured log aggregation, and rule-based alerting engine are all implemented.

## Completed ✅
- [x] MetricsCollector singleton with Prometheus text-format export (`/metrics`)
- [x] QueryProfiler – per-phase and per-operator timing with index usage tracking
- [x] StorageProfiler – RocksDB stats, write/read amplification, cache hit rates
- [x] PerformanceAnalyzer – automated issue detection with optimization recommendations
- [x] Alertmanager integration (alert routing and notifications)
- [x] Distributed tracing with span context propagation (OpenTelemetry-compatible)
- [x] Structured logging via Core ILogger interface
- [x] Kubernetes readiness and liveness health probes
- [x] Telemetry aggregation across shards
- [x] Grafana dashboard integration
- [x] PagerDuty/Slack notification routing
- [x] Continuous profiling integration (pprof / async-profiler compatible) (Issue: #2418)
- [x] Adaptive sampling rate for high-frequency spans (Issue: #1963)
- [x] Standalone `tracer.cpp` (ObservabilityTracer) — W3C Trace Context propagation, span ring buffer, MetricsCollector integration (OBS-MISSING-001)
- [x] Standalone `log_aggregator.cpp` (LogAggregator) — structured JSON log collection, trace-context correlation, ring buffer, file sink (OBS-MISSING-001)
- [x] Rule-based alerting engine with configurable notification channels
  - Files: `observability/alerting_engine.h`, `observability/alerting_engine.cpp`
  - Implementation: `INotificationChannel`, `LogNotificationChannel`, `WebhookNotificationChannel`, `SlackNotificationChannel`, `AlertingEngine` (owns `AlertRuleManager`, dispatches to channels, optional Prometheus Alertmanager backend)
  - Predefined default rules: CPU, memory, query latency P99, error rate, disk space, query queue depth, cache miss rate, write amplification
  - Tests: `tests/test_alerting_engine.cpp`
- [x] Custom Metric Types — extended metric primitives beyond counters, gauges, histograms (Issue: #80)
  - Files: `observability/advanced_metrics.h`, `observability/advanced_metrics.cpp`
  - Implementation: `AdvancedMetrics` — Summary (sliding-window quantiles), ExponentialHistogram (scale-locked, zero_count), Cardinality (exact hash-set), TimeWeightedAverage (∫value×dt), Rate (double samples, window-pruned); all methods thread-safe
  - Tests: `tests/test_custom_metric_types.cpp` (AdvancedMetricsTest, 32 tests)

## In Progress 🚧
- [x] OpenTelemetry SDK direct export (OTLP gRPC/HTTP) (Target: Q2 2026)
- [x] OpenTelemetry Full Integration (v1.6.0) — `OpenTelemetryTracer` with `OTelConfig`, multi-exporter dispatch (OTLP/Jaeger/Zipkin), W3C Baggage for tenant/user context, `recordException()`, `recordMetrics()`
  - Files: `observability/opentelemetry_tracer.h`, `observability/opentelemetry_tracer.cpp`
  - OTLP: async `OtlpExporter` started when endpoint is set; `SpanRecord` → `SpanData` conversion on span end
  - Jaeger/Zipkin: `JaegerTracerAdapter`/`ZipkinTracerAdapter` delegate sub-tracers registered per configured exporter
  - Tests: `tests/test_opentelemetry_full_integration.cpp` (44 focused tests covering AC-1 through AC-38)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Exemplars on Prometheus histograms (link traces to metrics) (Issue: #1995)
  - Files: `observability/metrics_collector.h`, `observability/metrics_collector.cpp`
  - Implementation: `Exemplar` struct + `observeHistogramWithExemplar()` + inline p99 exemplar export
  - Tests: `tests/test_metrics_exemplar.cpp`
- [x] Custom user-defined alert rules via API (Issue: #2025)
  - Files: `observability/alertmanager.h`, `observability/alertmanager.cpp`
  - Implementation: `AlertRule`, `AlertRuleOperator`, `AlertRuleManager` (CRUD + `evaluateRules()`)
  - Tests: `tests/test_alert_rules.cpp`
- [x] Rule-based alerting engine with configurable notification channels
  - Files: `observability/alerting_engine.h`, `observability/alerting_engine.cpp`
  - Implementation: `INotificationChannel` (abstract), `LogNotificationChannel`, `WebhookNotificationChannel`, `SlackNotificationChannel`, `AlertingEngine` (extends `Alertmanager`, owns `AlertRuleManager`, pluggable channel dispatch)
  - Default rules: CPU (>80%), memory (>90%), query P99 latency (>1000ms), error rate (>5%), disk free (<10%), queue depth (>100), cache miss (>50%), write amplification (>20×)
  - Tests: `tests/test_alerting_engine.cpp`
- [x] Per-tenant metric namespacing (v1.8.0)
  - Files: `observability/tenant_metrics_namespace.h`, `observability/tenant_metrics_namespace.cpp`
  - Implementation: `TenantMetricsNamespace` — per-tenant counters/gauges/histograms with independent cardinality budgets; `strict_tenant_registration` mode; Prometheus export prefixes `themis_<tenant_id>_` and auto-injects `tenant_id` label; thread-safe via `shared_mutex`
  - Tests: `tests/test_tenant_metrics_namespace.cpp` (TenantMetricsNamespaceFocusedTests)
- [x] Prometheus advanced features — rate calculation, histogram aggregation, cardinality management
  - Files: `observability/metric_aggregator.h`, `observability/metric_aggregator.cpp`
  - Implementation: `MetricAggregator` (rate calculation, histogram aggregation SUM/AVG/MAX/MIN/P50/P95/P99, rule-based aggregation with drop_labels/group_by_labels, per-metric cardinality limits)
  - Tests: `tests/test_metrics_aggregation.cpp` (MetricsAggregationFocusedTests)
- [x] Metric Aggregation Pipeline — cross-shard pre-aggregation and cardinality rollup (Issue: #81)
  - Files: `observability/metric_aggregator.h`, `observability/metric_aggregator.cpp`
  - Implementation: `ShardMetrics` (shard_id, per-metric value vectors, shard-level labels), `MetricSnapshot` (aggregated results + timestamp), `MetricAggregator::aggregateShardMetrics()` (stateless cross-shard aggregation applying registered rules), `MetricAggregator::rollupMetrics()` (time-window based snapshot pruning for cardinality reduction)
  - Tests: `tests/test_metrics_aggregation.cpp` (AggregateShardMetrics_* and RollupMetrics_* test cases)
- [x] Real-time metric streaming via WebSocket / SSE (Issue: #82)
  - Files: `observability/metrics_stream_server.h`, `observability/metrics_stream_server.cpp`
  - Implementation: `MetricsStreamServer` with `StreamSubscription` (client_id, metric_names, MetricFilter[], update_interval), `MetricUpdate` (name, value, labels, timestamp), `SendFn` callback-based delivery decoupled from transport; `pushMetrics()` fans out to matching subscribers with AND-semantics label filtering, per-subscription rate limiting, and `formatWebSocketMessage()` / `formatSseMessage()` serialisers
  - Tests: `tests/test_metrics_stream_server.cpp` (MetricsStreamServerFocusedTests) — 30+ tests covering lifecycle, subscription management, name/label filtering, throttling, serialisation, and concurrent push
- [x] Structured log search API — query logs like data (v1.8.0)
  - Files: `observability/log_search_engine.h`, `observability/log_search_engine.cpp`
  - Implementation: `LogSearchEngine` (stateless) — `LogSearchQuery` with level/time-range/message-contains/field-filter predicates (AND semantics), `FieldMatchOp` (EQUALS/NOT_EQUALS/CONTAINS/STARTS_WITH), pagination (limit/offset), sort order; `count()`, `distinctFieldValues()`
  - Tests: `tests/test_log_search_engine.cpp` (LogSearchEngineFocusedTests)
- [?] Real-time query cost estimator dashboard

### Long-term (6-12 months)
- [x] eBPF-based low-overhead kernel-level tracing (Issue: #2055) — ✅ implemented; `ebpf_tracer.cpp` (0 stubs, unit tests in `tests/test_ebpf_tracer.cpp`)
  - Files: `observability/ebpf_tracer.h`, `observability/ebpf_tracer.cpp`
  - Subsystems: MetricsCollector (gauge export), background sampling thread
  - Behaviour: polls `perf_event_open(2)` software counters at configurable interval (default 1 s); publishes `themis_ebpf_{context_switches,page_faults,cpu_migrations,task_clock_ns,collection_cycles}_total` gauges; optional libbpf BPF program attach via `THEMIS_ENABLE_EBPF`
  - Error handling: graceful no-op on non-Linux; fails-open when perf fd open fails (continues without that probe)
  - Tests: unit tests in `tests/test_ebpf_tracer.cpp` (lifecycle, config, stats, ring-buffer, callback, metrics)
  - Perf target: < 0.1 % CPU overhead per probe type at 1-second interval
- [x] Anomaly detection on metrics time-series (ML-based) (Issue: #2097)
  - Files: `observability/metric_anomaly_detector.h`, `observability/metric_anomaly_detector.cpp`
  - Implementation: `MetricAnomalyDetector` (monitor/observe/getAnomalies/publishMetrics/generateReport), bridges `analytics::StreamingAnomalyDetector` (Z_SCORE, MODIFIED_Z_SCORE, IQR, ISOLATION_FOREST, LOF, ENSEMBLE) with `MetricsCollector` gauges; `AnomalyCallback` hook; JSON & text reports
  - Tests: `tests/test_metric_anomaly_detector.cpp`
- [x] Distributed flame graph generation across nodes (Issue: #2108)
  - Files: `observability/distributed_flame_graph.h`, `observability/distributed_flame_graph.cpp`
  - Implementation: `DistributedFlameGraph` (add/merge/diff node profiles, normalise-per-node, JSON/folded export)
  - Tests: `tests/test_distributed_flame_graph.cpp`
- [?] Metrics federation across multiple ThemisDB clusters
- [x] SLO/SLA compliance reporting with burn-rate alerts (Issue: #2148)
  - Files: `observability/slo_reporter.h`, `observability/slo_reporter.cpp`
  - Implementation: `SloReporter` with `SloDefinition`, multi-window burn-rate detection (FAST 14.4×/MEDIUM 6×/SLOW 3×), `publishMetrics()`, `generateReport()`, `generateReportJson()`
  - Tests: `tests/test_slo_reporter.cpp`

## Implementation Phases

### Phase 1: Enterprise Observability Stack (Status: Completed ✅)
- [x] MetricsCollector singleton with Prometheus text-format export at `/metrics` (`observability/metrics_collector.cpp`)
- [x] QueryProfiler: per-phase and per-operator timing with index usage tracking (`observability/query_profiler.cpp`)
- [x] StorageProfiler: RocksDB stats, write/read amplification, cache hit rates
- [x] PerformanceAnalyzer: automated issue detection with optimization recommendations
- [x] Alertmanager integration (alert routing and notifications)
- [x] Distributed tracing with span context propagation (OpenTelemetry-compatible, `src/observability/continuous_profiler.cpp`)
- [x] Structured logging via Core ILogger interface
- [x] Kubernetes readiness and liveness health probes
- [x] Telemetry aggregation across shards
- [x] Grafana dashboard integration and PagerDuty/Slack notification routing

### Phase 2: Native OTLP Export & Continuous Profiling (Status: Complete ✅)
- [x] OpenTelemetry SDK direct export via OTLP gRPC/HTTP (`observability/otlp_exporter.cpp`, Target: Q2 2026)
  — OtlpExporter: async background flush thread, JSON OTLP payload, libcurl HTTP POST
  — TracingMiddleware: X-Correlation-ID propagation + finishSpan() enqueues SpanData to OtlpExporter
  — Tests: `tests/test_otlp_exporter.cpp`, `tests/test_otel_api_tracing.cpp`
- [x] Distributed tracing spans for all major API paths (Target: Q2 2026)
  — all 64 API handler files fully instrumented (admin, transaction, schema, export, graphql, maintenance, llm, voice, lora, monitoring, cache_admin, distributed_txn, task_scheduler, pii, audit, session, branch, pitr, diff, merge, mvcc, snapshot, import, pki, profiling, geo_topology, policy_*, async_job, hot_reload, wal, serverless_function, service_mesh, update, bpmn, compliance_reporting, prompt, prompt_engineering, replication_topology, review_scheduling, udf, retention, keys, classification, error, saga, feedback, reports)
  — Tests: `tests/test_otel_api_tracing.cpp` (162 tests covering every handler group; 120+ new tests added March 2026)
- [x] Continuous profiling integration (pprof / async-profiler compatible) (Target: Q2 2026)
- [x] Adaptive sampling rate for high-frequency spans (Target: Q3 2026)

### Phase 3: ML-Augmented & Distributed Observability (Status: In Progress 🚧)
- [x] Exemplars on Prometheus histograms (link traces to metrics)
  — `observability/metrics_collector.h/cpp`, tests: `tests/test_metrics_exemplar.cpp`
- [x] Custom user-defined alert rules via API
  — `observability/alertmanager.h/cpp`, tests: `tests/test_alert_rules.cpp`
- [x] eBPF-based low-overhead kernel-level tracing
  — `observability/ebpf_tracer.h/cpp`, tests: `tests/test_ebpf_tracer.cpp`
- [x] Anomaly detection on metrics time-series (ML-based)
  — `observability/metric_anomaly_detector.h/cpp`, tests: `tests/test_metric_anomaly_detector.cpp`
- [x] Distributed flame graph generation across nodes
  — `observability/distributed_flame_graph.h/cpp`, tests: `tests/test_distributed_flame_graph.cpp`
- [x] SLO/SLA compliance reporting with burn-rate alerts
  — `observability/slo_reporter.h/cpp`, tests: `tests/test_slo_reporter.cpp`
- [x] Prometheus advanced features — rate calculation, histogram aggregation, cardinality management
  — `observability/metric_aggregator.h/cpp`, tests: `tests/test_metrics_aggregation.cpp` (MetricsAggregationFocusedTests)
- [x] Real-time metric streaming via WebSocket / SSE (v1.6.0, Issue #82)
  — `observability/metrics_stream_server.h/cpp`, tests: `tests/test_metrics_stream_server.cpp` (MetricsStreamServerFocusedTests)
- [x] Custom Metric Types — Summary, ExponentialHistogram, Cardinality, TimeWeightedAverage, Rate (v1.6.0, Issue #80)
  — `observability/advanced_metrics.h/cpp`, tests: `tests/test_custom_metric_types.cpp` (AdvancedMetricsTest, 32 tests)
- [x] Root Cause Analysis — automated root cause identification for performance issues (v1.7.0, Issue #84)
  — `observability/root_cause_analyzer.h/cpp`, tests: `tests/test_root_cause_analyzer.cpp` (RootCauseAnalyzerFocusedTests, 34 tests)
  — Types: `TimeSeries`, `SystemSnapshot`, `CorrelatedMetric`, `CausalEdge`, `CausalGraph`, `RootCauseReport`, `RootCauseAnalyzerConfig`
  — API: `analyzeIssue()` (before/after `SystemSnapshot` delta analysis + heuristic rules), `findCorrelations()` (Pearson correlation against registered time series), `buildCausalGraph()` (Granger-inspired lag-1 causal inference)
  — CI: `.github/workflows/root-cause-analyzer-ci.yml`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — `test_observability_profilers.cpp` (280 LOC), `test_observability_hardening.cpp`; focused targets: `ObservabilityProfilersFocusedTests`, `ObservabilityHardeningFocusedTests`
- [?] Integration tests (Prometheus scrape, Grafana dashboard rendering)
- [x] Performance benchmarks (metrics overhead < 1% CPU) — `benchmarks/bench_metrics_collector.cpp` registered (2026-03-10)
- [?] Security audit (metrics endpoint authentication, trace data PII)
- [x] Documentation complete — README.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, docs/de/observability/ all present
- [?] API stability guaranteed

## Known Issues & Limitations
- Telemetry aggregation across shards is eventually consistent.
- `query_profiler.cpp`, `storage_profiler.cpp`, and `performance_analyzer.cpp` were missing from `cmake/CMakeLists.txt` — fixed 2026-03-09; `test_observability_profilers.cpp` would fail to link without this fix.
- `tracer.cpp` and `log_aggregator.cpp` were absent — implemented 2026-03-11 (OBS-MISSING-001); see `include/observability/tracer.h`, `include/observability/log_aggregator.h`.

## Breaking Changes
- Prometheus metric names follow `themis_*` namespace; stable from v1.x.
- Span context format may change to full W3C Trace Context standard in v2.0.
