---
Author: ThemisDB Maintainers
Created: 2026-10-01
Last Updated: 2026-10-01
Status: active
---
# ThemisDB — Actionable TODO Reality Check (2026-10-01)

## Executive Summary

- Marker reality check was rerun against current `src/`.
- After targeted false-positive cleanup, remaining TODO markers are **30**.
- **All 30 remaining TODO markers are in `src/rag/`** and represent real implementation follow-ups (not resolved-comment/string noise).

## Current Marker Baseline

| Marker | Count |
|---|---:|
| TODO | 30 |
| STUB | 139 |
| MOCK | 3 |
| FIXME | 0 |
| **Total** | **172** |

Source snapshot: `audit/MARKER_LOCATIONS_2026-10-01.md`

## False-Positive Cleanup Performed

1. `src/analytics/streaming_window.cpp`
   - Historical resolved comment block changed from `TODO(v1.8.0)` labels to `RESOLVED(v1.8.0)` labels.
   - Effect: resolved items no longer inflate TODO scans.
2. `src/graph/graph_error_taxonomy.cpp`
   - Error text changed from `stub/TODO code path` to `stub code path`.
   - Effect: taxonomy string no longer creates TODO false-positive.
3. Multi-module tracking comments/strings
   - Non-actionable labels of the form `STUB #<id>` were renamed to `BRIDGE_REF #<id>`.
   - Effect: scanner noise reduced while preserving bridge-tracking semantics.

## Remaining Actionable TODOs (source-verified)

| File | Line | TODO |
|---|---:|---|
| `src/rag/reranker_cost_analyzer.cpp` | 300 | `// TODO: Store forecast for later use` |
| `src/rag/reranker_budget_gate.cpp` | 137 | `// TODO: This requires Document struct with score field` |
| `src/rag/freshness_sla_enforcer.cpp` | 67 | `// TODO: Call IndexRefreshScheduler::ScheduleEmergencyRefresh()` |
| `src/rag/freshness_sla_enforcer.cpp` | 93 | `// TODO: Filter events by time window (last N hours)` |
| `src/rag/otel_span_emitter.cpp` | 161 | `// TODO: Implement actual OTLP export logic` |
| `src/rag/ingestion_latency_monitor.cpp` | 86 | `status.is_primary = true;  // TODO: Track primary vs secondary` |
| `src/rag/ingestion_latency_monitor.cpp` | 128 | `// TODO: Implement persistence to RocksDB` |
| `src/rag/ingestion_latency_monitor.cpp` | 135 | `// TODO: Retrieve historical aggregates for last N hours` |
| `src/rag/adaptive_hybrid_router.cpp` | 63 | `// TODO: Call actual retrieval backends` |
| `src/rag/adaptive_hybrid_router.cpp` | 119 | `// TODO: Integrate with OpenTelemetry SDK` |
| `src/rag/model_registry.cpp` | 227 | `// TODO: Implement JSON serialization to persistence_path` |
| `src/rag/model_registry.cpp` | 237 | `// TODO: Implement JSON deserialization from persistence_path` |
| `src/rag/model_registry.cpp` | 247 | `// TODO: Implement comprehensive JSON export` |
| `src/rag/evaluation_result_store.cpp` | 193 | `return "{}";  // TODO: Serialize to JSON` |
| `src/rag/evaluation_result_store.cpp` | 211 | `// TODO: Export run to file` |
| `src/rag/cost_model_builder.cpp` | 78 | `// TODO: Serialize model to JSON file` |
| `src/rag/cost_model_builder.cpp` | 83 | `// TODO: Deserialize model from JSON file` |
| `src/rag/recommendation_engine.cpp` | 123 | `// TODO: Export recommendations to file (JSON or markdown)` |
| `src/rag/benchmark_suite.cpp` | 19 | `// TODO: Parse TREC/MARCO format TSV or JSON` |
| `src/rag/benchmark_suite.cpp` | 53 | `// TODO: Execute RAG system with scenario config, retrieve and rerank` |
| `src/rag/benchmark_suite.cpp` | 104 | `// TODO: Export to JSON or CSV` |
| `src/rag/cross_encoder_orchestrator.cpp` | 56 | `// TODO: Actual cross-encoder inference` |
| `src/rag/cross_encoder_orchestrator.cpp` | 126 | `// TODO: Load model from disk or download from registry` |
| `src/rag/cost_attribution_tracker.cpp` | 30 | `// TODO: Replace with actual RocksDB initialization:` |
| `src/rag/cost_attribution_tracker.cpp` | 243 | `// TODO: Compare actual cost vs forecast from cost model` |
| `src/rag/router_policy_store.cpp` | 16 | `// TODO: In production, initialize RocksDB connection here` |
| `src/rag/router_policy_store.cpp` | 20 | `// TODO: Close RocksDB connection` |
| `src/rag/router_policy_store.cpp` | 94 | `// TODO: Implement metrics recording in database` |
| `src/rag/router_policy_store.cpp` | 103 | `// TODO: Retrieve and aggregate metrics` |
| `src/rag/metrics_reporter.cpp` | 218 | `// TODO: Implement with access to model-specific metrics` |

## Interpretation

- The TODO situation is no longer broad and diffuse across modules.
- Remaining TODOs are concentrated in RAG follow-up implementation areas (serialization/persistence, telemetry export, benchmark execution/export, retrieval backend integration).
- This is a **real implementation backlog**, not primarily scanner-noise.

## Recommended Next Batch (high signal)

1. `src/rag/router_policy_store.cpp` — replace in-memory placeholders with real RocksDB metric persistence path.
2. `src/rag/model_registry.cpp` + `src/rag/cost_model_builder.cpp` + `src/rag/evaluation_result_store.cpp` — close JSON/protobuf serialization/export gaps.
3. `src/rag/benchmark_suite.cpp` + `src/rag/cross_encoder_orchestrator.cpp` + `src/rag/adaptive_hybrid_router.cpp` — replace placeholder execution paths with real retrieval/inference wiring.
4. `src/rag/otel_span_emitter.cpp` + `src/rag/metrics_reporter.cpp` + `src/rag/freshness_sla_enforcer.cpp` — close operational telemetry and refresh-trigger gaps.

## Governance Note

This report classifies TODO markers only. Remaining STUB/MOCK markers require a separate pass with activation-condition and roadmap/issue linkage checks before deciding close vs accepted non-production path.
