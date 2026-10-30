**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [ContentManager API](../../include/content/content_manager.h), [async worker API](../../include/content/async_ingestion_worker.h), [worker implementation](async_ingestion_worker.cpp), [metrics API](../../include/content/content_metrics.h), [security implementation](content_security.cpp), [existing incident runbook](../../docs/operability/content_runbook.md)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module incident runbook

## Purpose and safety

Use this runbook to diagnose ingestion failures, queue pressure, optional processor
degradation, and security rejections. It describes observable interfaces present in the
module source. Deployment dashboards, alert thresholds, service-control commands, and
external integrations are deployment-owned and must be verified before an incident.

Preserve correlation IDs, content/job IDs, typed errors, timestamps, and sanitized
diagnostics. Do not copy raw payloads or credentials into tickets. Do not disable
validation/security gates or loosen archive/size limits as a first response.

## Triage order

1. Establish impact and time window; determine whether the issue affects all formats or a
   processor/dependency subset.
2. Read caller-visible `IngestResult`/`Status` and structured content errors. For an
   `IngestResult`, retain `success`, `error_message`, `primary_content_id`,
   `extracted_content_ids`, and safe `metadata` fields. Inspect the ordered
   `stage_outcomes` (`stage_name`, `succeeded`, `skipped`, `attempts`,
   `error_message`) to locate the failed or skipped stage. Separate validation,
   processor, embedding, storage, and queue symptoms.
3. For async paths, inspect `AsyncIngestionWorker::isRunning()`,
   `getStatistics()`, `getAllJobs(status)`, and `getJobStatus(job_id)`.
4. Distinguish the two content metrics APIs. `ContentManager::getMetrics()` exposes
   manager-owned compression and deduplication counters; `ContentMetrics` is a separate
   object with its own JSON and Prometheus text serialization. Confirm which instances
   are updated and whether caller/server wiring publishes them before relying on
   dashboards. Neither API alone proves automatic endpoint registration.
5. Preserve evidence and escalate if a safety control is unavailable or the failure
   scope is unclear.

## Scenario procedures

### No ingestion progress or growing async backlog

- Check `running`, `queue_size`, `max_queue_size`, `max_queue_depth`,
  job counts, failure counts, and back-pressure totals in `getStatistics()`.
- Inspect representative queued/failed jobs and their error messages; avoid dumping job
  objects because job records may include payload/configuration.
- For a failed synchronous ingest, compare `success` and `error_message` with each
  `stage_outcomes` entry. Preserve content IDs for correlation, but avoid logging raw
  payloads or unreviewed metadata values.
- Confirm producers are still submitting and storage/processors are reachable.
- If stopping is necessary, prefer `stop(true)` when draining is safe and time permits.
  `stop(false)` cancels queued work. Do not use it without an explicit decision about
  those jobs. A submission after shutdown can throw.
- Restart only through the deployment's approved lifecycle procedure; verify that the
  queue drains and new jobs complete before resuming full producer volume.

### Validation or security rejection increase

- Group failures by typed error, MIME/category, producer, and time window using existing
  sanitized logs/metrics.
- Confirm whether input format, declared MIME, size, or active policy changed.
- Preserve rejected IDs and diagnostic context. Correct the producer or approved policy
  configuration, then retry only approved source inputs.
- Do not infer a malware/PII/abuse scan occurred merely from a successful ingestion:
  feature flags and detector registration are configurable. Escalate if a required
  control is missing or unexpectedly disabled.

### Optional processor or embedding degradation

- Check build feature flags, processor registration, dependency health, and the
  corresponding stage configuration.
- For embeddings, compare empty results and `EmbeddingPipeline::getFailureCount()` with
  the optional `content_embedding_failures_total` metric. Disabled/empty input does not
  increment the pipeline failure count.
- Keep ingestion/enrichment outcomes distinct. Do not report enrichment as complete
  solely because a content item was stored. Reprocessing/re-enrichment is not automatic
  unless the deployment provides that workflow.

### Archive rejection or resource pressure

- Capture archive size, configured ratio/file-count limits, MIME, and rejection code
  without retaining unsafe payloads.
- Keep protective limits in place while confirming whether the input is legitimate.
  Any temporary limit change requires the deployment's security and service-owner
  approval, plus a bounded duration and rollback plan.
- Rate-limit or quarantine at the upstream boundary where those controls exist; the
  content module does not define a universal producer-control command.

## Recovery and closure

Recovery is complete only after the actual failure cause is addressed, required gates
remain enabled, the affected path succeeds with an approved test input, and error/backlog
trends return to the deployment's reviewed baseline. Record the triggering evidence,
actions, outstanding work, and any replayed/cancelled IDs. Roll back a configuration
change if it increases rejection risk, causes unbounded resource use, or worsens the
original symptom.

The existing [operability runbook](../../docs/operability/content_runbook.md) covers
additional incident scenarios, but its command names, metric names, and thresholds are
not defined by the content C++ API. Validate those integrations in the target deployment
before executing its examples.

## Sources

- [Async worker API](../../include/content/async_ingestion_worker.h) and
  [worker implementation](async_ingestion_worker.cpp).
- [ContentManager API](../../include/content/content_manager.h) and
  [manager implementation](content_manager.cpp), including `IngestResult` and
  manager-owned compression/deduplication counters.
- [Content metrics API](../../include/content/content_metrics.h),
  [metrics implementation](content_metrics.cpp), and
  [typed errors](../../include/content/content_errors.h).
- [Security API](../../include/content/content_security.h),
  [security implementation](content_security.cpp), and
  [security tests](../../tests/content/test_content_security.cpp).
- [Streaming tests](../../tests/content/test_content_streaming_ingestion.cpp),
  [production requirements](PRODUCTION_REQUIREMENTS.md), and
  [security overview](SECURITY.md).
- [Existing incident runbook](../../docs/operability/content_runbook.md) and
  [Wave D evidence](WAVE_D_CLOSURE_EVIDENCE_BUNDLE.md).
