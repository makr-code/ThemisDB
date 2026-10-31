---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Operations Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and deployment boundary

This guide describes the operational contracts visible in the current RAG
module code. The monitor and scheduler classes are components that a service
may wire into its runtime; their existence does not establish that the request
path, metric exporter, alert receiver, or refresh backend is configured.
Configuration is primarily passed through C++ config objects and constructor
dependencies; do not assume environment variables from phase specifications
are read by these classes.

## Signals to collect

| Signal | Current source | Operational interpretation |
|---|---|---|
| Retrieval candidates and fused scores | `HybridFusionResult` | Compare per-source IDs/ranks and fused score; fusion cannot reveal omitted candidates. |
| Context token use and truncation | `AssembledContext` | Record `tokens_used`, `tokens_remaining_for_response`, chunk count, and `was_truncated`. |
| Quality decision and stage duration | `QualityCheckResult` | Retain status, stage timings, dimension scores, confidence, and failure reasons. |
| Index staleness | `IngestionLatencyMonitor::GetPercentiles()` and `GetShardStatuses()` | Treat values as the monitor's in-memory samples; corroborate with the actual index and ingestion pipeline. |
| Layer handoff quality | `RagQualityMonitor` | Retains up to 300 samples; exports latest-sample gauge lines through the logger and checks selected anomalies. |
| Span buffer state | `OTELSpanEmitter::GetStats()` | Distinguish pending, locally counted emitted, and dropped spans; local counts do not prove collector receipt. |

Correlate signals with service-level request IDs and model/index versions.
Do not use raw query or document text as metric labels. Apply retention,
redaction, and access controls in the service's logging and telemetry systems.

## Routine operation

### Before enabling a model or index revision

1. Record model artifact hashes, embedding dimension, tokenizer/runtime version,
   and index/chunking configuration.
2. Verify query and passage encoders are a compatible pair and initialize
   `DPRVectorizer` before use.
3. Compare retrieval on held-out relevance judgments using the same corpus and
   metric cutoffs as the recorded baseline. Keep quality, latency, and cost as
   separate measurements.
4. Use the deployment's canary and rollback controls. The RAG module itself
   does not provide an atomic general model/index cutover.

### Ingestion health

- Check `IndexResult` for each indexed document and separately verify vector
  and graph sink state. A workflow fallback or partial sink write can still
  return `ok == true`.
- Monitor ingestion timestamps and destination collection. A monitor's
  percentile does not prove that all expected source records were indexed.
- If using `RotateHourlyAggregate()`, schedule it explicitly; the API does not
  create a background task by itself. The monitor's historical aggregate map is
  pruned to at most 168 buckets when rotation runs.

### Quality and freshness checks

- Call `RealtimeSLOTracker::RecordQuery()` with validated latency, rerank,
  relevance, and cost values, then call `UpdateCompliance()` as required by the
  integration. This class uses in-memory samples and is not documented as
  thread-safe.
- Use `RagQualityMonitor` for its bounded 300-sample window and mutex-protected
  record/read operations. Its gauge output is log text, not a Prometheus
  exporter endpoint.
- Use `FreshnessSLAEnforcer::UpdateCompliance()` to update its state from the
  configured latency monitor. A scheduled refresh must still be verified
  against the scheduler and actual index backend.

## Implementation limits relevant to operations

The following caveats are source-observed and should be considered before using
these components as an external SLO or alerting authority:

- `RealtimeSLOTracker` retains samples in vectors until its lifetime ends; it
  has no pruning API. Its public methods do not provide synchronization.
- `FreshnessSLAEnforcer::GetComplianceStatus()` currently synthesizes
  `compliance_percentage` and `breaches_in_hour` as fixed values based on the
  current state; they are not computed from a historical compliance window.
- `OTELSpanEmitter::ExportSpans()` builds a local serialized payload and updates
  in-process counters but does not transmit the payload to an OTLP endpoint.
  Do not treat `Flush()` or `emitted_spans` as collector acknowledgement.
- `RagQualityMonitor` sends gauge-formatted lines to the logger. An external
  log-to-metrics integration is required before those lines can be scraped as
  metrics.
- `IngestionLatencyMonitor` computes percentiles from its retained samples.
  Hourly rotation caps historical buckets, but does not prune the underlying
  sample vector. Its public API has no synchronization contract.

These are not substitutes for service-level telemetry, durable history,
external alert delivery, or backend health checks. The module's phase
specifications and `EVALUATION_CONTRACT_V1.md` include targets and designs;
recorded targets are not current measurements.

## Change validation

For code changes, build and run the matching focused test targets from
`tests/rag/CMakeLists.txt`. Monitoring and freshness tests include
`test_ingestion_latency_monitor`, `test_freshness_sla_enforcer`,
`test_realtime_slo_tracker`, and `test_otel_span_emitter`. For ranking and
quality changes, see the [module guide](./MODULE_GUIDE.md) test map.

## References

- [Incident runbook](./RUNBOOK.md)
- [Trust and data handling](./TRUST.md)
- [Freshness specification](./FRESHNESS_SLA_SPECIFICATION.md)
- [Observability specification](./OBSERVABILITY_SLO_SPECIFICATION.md)
- [Evaluation contract](./EVALUATION_CONTRACT_V1.md)
- `include/rag/ingestion_latency_monitor.h`
- `include/rag/freshness_sla_enforcer.h`
- `include/rag/realtime_slo_tracker.h`
- `include/rag/rag_quality_monitor.h`
- `include/rag/otel_span_emitter.h`
