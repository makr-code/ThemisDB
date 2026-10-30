---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "include/cdc/cdc_admin.h; include/cdc/cdc_metrics.h; include/cdc/changefeed.h; include/cdc/changefeed_buffer.h; src/cdc/ROADMAP.md"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC operational guide

This guide covers operational signals and controls exposed by CDC's C++
interfaces. The existence of a counter or JSON representation does not by
itself guarantee that a deployment exports it to a monitoring system.

## Health and diagnostics

[`CDCAdmin`](../../include/cdc/cdc_admin.h) exposes `healthCheck`,
`getDiagnostics`, and `getRetentionStatus`. The structures include component
health, buffer utilization/error fields, feed watermarks, event totals, buffer
size, uptime, retention policy/status, and metrics. Use the deployed server's
authorized admin interface to access these methods.

[`CDCMetrics`](../../include/cdc/cdc_metrics.h) provides:

- latency histograms for event recording, flush, compression and decompression,
  with percentile accessors;
- throughput counters for event/byte rates and window totals;
- counters for recorded/flushed events, flushes, compression, errors and
  retries; and
- WebSocket and Kafka delivery/overflow/error counters.

These are library-level structures and JSON fields. Confirm the actual
deployment exporter, metric names, scrape endpoint, and alert thresholds in
the service configuration before building dashboards.

## Retention and capacity

`Changefeed::RetentionPolicy` exposes enabled state, maximum age, event count,
byte size, cleanup interval and optional key-based compaction. The header
defaults are configuration defaults, not a capacity recommendation. Check
`getRetentionStatus()` to confirm the effective policy and cleanup state.

`ChangefeedBufferConfig` also declares per-buffer/global event limits,
memory limits, flush behavior, compression controls, retries and optional
rate-limiting configuration. Review
[`changefeed_buffer.h`](../../include/cdc/changefeed_buffer.h) and validate
the values used by the running deployment.

## Routine review

- Review health, feed watermarks, errors and consumer lag/redelivery together;
  a healthy process does not prove a downstream consumer is current.
- Review retention status alongside replay requirements before reducing
  retention or purging data.
- For Kafka-enabled deployments, check adapter build availability and Kafka
  delivery/error counters separately from WebSocket counters.
- Use CDC-specific benchmarks for performance changes. The module roadmap
  calls for refreshed CDC baselines; do not treat a historical benchmark
  threshold as a current production SLO without checking its evidence.

For fault handling, see [`RUNBOOK.md`](./RUNBOOK.md). Production requirements
and access-control assumptions are maintained in
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) and
[`SECURITY.md`](./SECURITY.md).

## Source references

- [Admin health, diagnostics and retention status](../../include/cdc/cdc_admin.h)
- [Metrics structures and serialization](../../include/cdc/cdc_metrics.h)
- [Feed retention policy](../../include/cdc/changefeed.h)
- [Buffer configuration and statistics](../../include/cdc/changefeed_buffer.h)
- [CDC roadmap and pending operability work](./ROADMAP.md)

**Exact model backend not verifiable from final document text alone.**
