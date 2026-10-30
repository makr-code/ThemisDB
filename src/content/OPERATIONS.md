**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [ContentManager API](../../include/content/content_manager.h), [manager implementation](content_manager.cpp), [metrics API](../../include/content/content_metrics.h), [metrics implementation](content_metrics.cpp), [async worker API](../../include/content/async_ingestion_worker.h), [production requirements](PRODUCTION_REQUIREMENTS.md), [performance expectations](PERFORMANCE_EXPECTATIONS.md)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module operations

## Purpose and boundary

This page supports operators configuring and monitoring the content module. The content
module exposes counters, job state, and processing results; dashboards, alert routing,
producer throttling, process restarts, and storage recovery are deployment responsibilities.
See [production requirements](PRODUCTION_REQUIREMENTS.md) before release.

## Before accepting production traffic

- Set explicit deployment-appropriate content size, archive, queue, and timeout limits.
- Confirm enabled format processors and their external dependencies match the intended
  workload. Compile-time feature flags can change available processing paths.
- Verify validation, policy, malware, abuse, and PII controls required by the deployment
  are enabled and have their required detector/scanner dependencies attached. Some
  controls are optional in the API defaults; defaults are not a production assurance.
- Configure bounded async queue capacity and worker count. Review `max_queue_size` and
  `max_queue_depth` together with producer concurrency.
- Confirm storage/index dependencies, encryption integration where required, and
  audit/log export are working before enabling bulk ingestion.
- Record a baseline for expected input volume, failure rates, and stage latency on the
  deployment's representative hardware.

## Signals available in module APIs

### Per-ingest diagnostics

`ContentManager::IngestResult` provides `success`, `error_message`,
`primary_content_id`, `extracted_content_ids`, `metadata`, and ordered `stage_outcomes`.
Each stage outcome includes `stage_name`, `succeeded`, `skipped`, `attempts`, and
`error_message`. Integrators should preserve these diagnostics (and safe content IDs)
when translating module results into application responses or logs. Avoid logging raw
payloads or unreviewed metadata.

### Separate metrics surfaces

`ContentMetrics::toJson()` includes ingestion/byte counts, validation and processing
success/failure counts, extraction/chunk/embedding/dedup counters, errors/timeouts,
format counters, MIME counts, validation violations, and latency percentiles for
operations recorded by the caller. `toPrometheusFormat()` serializes a Prometheus
text-format string with counters including `content_ingestions_total`, `content_validations_total`,
`content_processing_total`, `content_errors_total`, `content_timeouts_total`, and
`content_embedding_failures_total`. A caller must wire any publication or endpoint;
these methods do not establish automatic server registration. Verify instrumentation and
exporter wiring for the particular deployment; a declared metric is not proof that every
path records it.

Separately, `ContentManager::getMetrics()` returns manager-owned atomic counters:
compressed/uncompressed bytes, skipped compression counts, compression-ratio buckets
and sum/count, and dedup checks/hits. The manager API does not provide a corresponding
Prometheus serializer. Dedup checks are incremented only when deduplication is enabled,
a checker is attached, and the content category follows the implementation's image/text
paths; counter values are not a complete measure of all dedup behavior.

`AsyncIngestionWorker::getStatistics()` returns running state, worker count, queue size
and limits, job-status counts, processed/failed totals, back-pressure events, and queue
depth high-watermark. Job status/history methods can provide diagnosis context, but job
records may contain sensitive input/configuration; restrict access and do not export
payload-bearing fields as metric labels.

No queue-depth Prometheus metric or universal alert threshold is established by these
interfaces. Use deployment-specific telemetry only after verifying its source and
cardinality behavior.

## Routine review

1. Compare recent throughput, validation/processing failures, timeouts, embedding
   failures, and latency with the reviewed baseline.
2. Check async queue size and back-pressure trends alongside completion/failure counts.
3. Break down security and validation rejections by safe categories, not raw content.
4. Verify optional processors are healthy when their output is a required business
   outcome; absence may otherwise be a configured degraded mode.
5. Re-run the focused tests and mapped benchmarks after implementation/configuration
   changes; the [performance expectations](PERFORMANCE_EXPECTATIONS.md) define comparison
   gates, not a deployment-independent SLO.

## Recovery boundaries

Use [`RUNBOOK.md`](RUNBOOK.md) for incident triage. Graceful worker shutdown with
`stop(true)` drains queued work; `stop(false)` cancels queued jobs. Decide explicitly
whether cancellation is acceptable before selecting it. Do not disable safety checks,
increase resource limits without review, or assume a restart replays failed jobs.
Reprocessing must use an approved source and a deployment-owned replay procedure.

## Sources

- [Content metrics API](../../include/content/content_metrics.h) and
  [implementation](content_metrics.cpp).
- [Async worker API](../../include/content/async_ingestion_worker.h) and
  [implementation](async_ingestion_worker.cpp).
- [Content metrics tests](../../tests/content/test_content_metrics.cpp),
  [security tests](../../tests/content/test_content_security.cpp), and
  [soak test](../../tests/content/test_content_soak.cpp).
- [Production requirements](PRODUCTION_REQUIREMENTS.md),
  [performance expectations](PERFORMANCE_EXPECTATIONS.md),
  [security overview](SECURITY.md), and [incident runbook](RUNBOOK.md).
