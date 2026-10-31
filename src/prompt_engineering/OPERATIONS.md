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

# Prompt Engineering Operations

## Operational boundary

`prompt_engineering` provides library APIs and instrumentation; it does not deploy a daemon, expose a metrics HTTP endpoint, or own LLM connectivity. The service integrating the module must own lifecycle, transport timeouts/retries, authorization, metrics scraping, and alert routing.

## Lifecycle and configuration

`PromptEngineeringIntegration` exposes `start()`, `stop()`, `getStatus()`, execution hooks, background-worker controls, and configuration access. Review `IntegrationConfig` explicitly before enabling it in a deployment. Current defaults enable auto-versioning, auto-optimization, feedback collection, performance tracking, injection detection, and the background worker; reflection tuning defaults off. The background interval defaults to one hour, with optimization thresholds of 100 executions and 0.7 minimum success rate.

The worker exposes running state, completed cycles, optimized-prompt count, last run, and next scheduled run. During shutdown, call the integration/worker stop method and allow its thread to join before destroying dependencies.

## Observability

- Export `PromptEngineeringMetrics::exportMetrics()` through the host application's metrics endpoint/adapter. The metrics object has counters for optimization, A/B tests, executions, feedback, version operations, integration/worker cycles, and reflection tuning.
- Configure alert thresholds and callback deliberately. A callback runs on the thread recording the triggering metric, so it must be thread-safe and must not block a prompt hot path.
- In the current implementation, callbacks fire for execution failure rate and hallucination count; `AlertConfig::min_success_rate` is declared but is not checked by the metric-recording methods. Do not rely on that threshold for paging without adding and testing the corresponding behavior.
- Use integration and worker status snapshots to distinguish no traffic from a stopped worker. Use logs and structured result/error details for per-operation causes.
- RewriteEngine `get_stats_json()` includes rewrite counts, rules evaluated/applied, maximum-step failures, total latency, and average latency. `RewriteTrace` is bounded by context settings; preserve trace data only when its retention is approved.
- Metrics are in memory until the host persists a JSON snapshot. Snapshot restore overwrites current counters and ignores unknown keys; arrange snapshot cadence and retention in the hosting service.

Metric identifiers and exported text are defined in [`prompt_engineering_metrics.h`](../../include/prompt_engineering/prompt_engineering_metrics.h) and [`prompt_engineering_metrics.cpp`](./prompt_engineering_metrics.cpp). The rewrite statistics contract is in [`rewrite_engine.h`](../../include/prompt_engineering/rewrite_engine.h).

## Data retention and storage

The manager, version controller, and feedback collector accept non-owning RocksDB pointers. Ensure the wrapper and column-family handles outlive those objects. Feedback includes raw query and response strings; apply storage access controls, retention, backup, and deletion policy in the owning service. `FeedbackCollector::pruneOldFeedback()` and `clearFeedback()` are module APIs, but run them only under an approved retention procedure.

Cross-shard feedback publishing occurs only when both the sync component and embedding model are supplied. A publish exception is logged and does not fail local feedback recording. Validate local and peer-shard outcomes independently.

## Change control and release readiness

Before rollout, run focused prompt-engineering tests, exercise a representative fixture set, verify metrics export and alert callback behavior, and compare relevant benchmark cases with the release baseline. Deploy changed prompts and rewrite rules with a known rollback version/configuration. Follow [`RUNBOOK.md`](./RUNBOOK.md) for incidents and [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for deployment checks.

Exact model backend not verifiable from final document text alone.
