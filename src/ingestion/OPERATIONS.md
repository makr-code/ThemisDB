---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Operations

## Purpose and boundary

This page describes operational signals and safe lifecycle practices exposed by
the ingestion module. It is for deployment owners wiring health checks,
Prometheus exposition, and restart/recovery behavior. Service discovery,
authentication, alert rules, and deployment-specific SLOs belong to the
deploying service; this module does not install an HTTP endpoint by itself.

## Health and metrics

`IngestionAdminApi::healthJson()` returns a compact status plus registered,
enabled, and quarantined counts. `listSources()` adds availability and last-run
status. An ingestion report contains aggregate totals and per-source
`IngestionStats`; pass it to `IngestionMetricsExporter` to obtain Prometheus
text exposition.

The default metric prefix is `themis_ingestion`. Exported families include:

- `docs_processed_total`, `docs_failed_total`, and `bytes_processed_total`;
- `elapsed_seconds`, `retry_total`, `errors_total`, and
  `throughput_docs_per_sec`;
- `schema_violations_total` and per-error-code breakdowns;
- report-level `total_documents`, `total_failures`, `total_time_seconds`,
  `quarantine_size`, `quarantine_permanently_failed_total`, and
  `quarantine_retry_success_total`.

Single-source series include `source_id`; the caller may supply `source_type`.
Metric names are prefixed by the configured exporter prefix. These metrics are
generated from a report snapshot; they are not a continuously updated scrape
registry. The deployment must retain/export snapshots at the frequency it
needs.

For incident correlation, retain `IngestionStats::correlation_id` alongside
structured errors. `DiagnosticEmitter` can notify registered listeners with
incident category/severity, error context, remediation hint, and optional
metrics. Listener delivery and external alerting are integration work.

## Capacity and configuration

- Bound source-side pagination/message/object limits where connectors support
  them. `0` is unlimited for several limits.
- Configure finite retry timeouts and rate limits appropriate for the external
  source. The manager's rate limiter is opt-in; its default config is disabled.
- For large runs, size parallelism and downstream capacity together. Raising
  source concurrency without checking sink/storage capacity can shift pressure
  downstream.
- Use a persistent checkpoint implementation for restart-safe progress; the
  coordinator's default in-memory checkpoint and leader-election implementations
  do not survive restart or coordinate multiple processes.
- Dry-run and `previewSource()` are useful for configuration checks, but they do
  not prove successful writes or production durability.

## Restart and recovery

Before restart, preserve checkpoint state and the latest run report where
operationally important. On restart, confirm source config, target collection,
connector availability, checkpoint directory/store, and workflow profile. Run a
bounded source verification and compare report counts before restoring normal
rates.

Do not treat a successful `healthJson()` response as proof that all sources or
sinks are healthy: the source-level `available` field and recent run errors
provide additional context. Empty lineage data may mean tracking was disabled
or the in-memory store was lost, not that no ingestion occurred.

## Capacity evidence and limits

[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) maps benchmark
cases to regression gates. The release-gate benchmark includes in-memory mock
operations; use end-to-end tests and representative deployment measurements
before setting capacity or availability objectives. No alert threshold or
ingest-to-searchable freshness SLA is defined by this module document set.

## Operational references

- [`RUNBOOK.md`](./RUNBOOK.md) — source isolation, quarantine, and recovery.
- [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) — production
  configuration and security requirements.
- [`ROADMAP.md`](./ROADMAP.md) — remaining hardening and baseline work.

> Exact model backend not verifiable from final document text alone.
