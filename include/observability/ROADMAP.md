> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/observability/ROADMAP.md -->

# Observability Module — Public Header Roadmap

**Module Path:** `include/observability/`
**Canonical implementation roadmap:** [`../../src/observability/ROADMAP.md`](../../src/observability/ROADMAP.md)

---

## Overview

Tracks public observability API contract stability, telemetry header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/observability/ROADMAP.md`](../../src/observability/ROADMAP.md)

---

## Current Status

All 22 observability headers are present and cover metrics, tracing, profiling, alerting, anomaly detection, logging, flame graphs, root-cause analysis, and SLO reporting.

---

## Completed ✅

- [x] metrics and export headers — `metrics_collector.h`, `metric_aggregator.h`, `metrics_stream_server.h`, `tenant_metrics_namespace.h`
- [x] tracing and profiling headers — `tracer.h`, `opentelemetry_tracer.h`, `continuous_profiler.h`, `query_profiler.h`, `storage_profiler.h`, `ebpf_tracer.h`
- [x] alerting and anomaly headers — `alerting_engine.h`, `alertmanager.h`, `metric_anomaly_detector.h`, `ml_anomaly_detector.h`, `slo_reporter.h`
- [x] diagnostics headers — `log_aggregator.h`, `log_search_engine.h`, `distributed_flame_graph.h`, `root_cause_analyzer.h`, `performance_analyzer.h`

---

## In Progress

- [ ] Clarify exporter/back-end degradation guidance across metrics and alerting headers (Target: 2026-Q3)
- [ ] Add stronger compatibility notes for high-cardinality telemetry and profiler-result DTOs (Target: 2026-Q3)

---

## Planned

- [ ] `telemetry_incident.h` — shared incident/diagnostic DTO for exporter and collector failures (Target: 2026-Q4)
- [ ] `observability_capability_profile.h` — runtime feature/capability summary for API and admin layers (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for collector and export hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public observability headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
