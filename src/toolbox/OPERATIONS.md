---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox operations

**Purpose and audience:** Startup, monitoring, validation, and recovery guidance for operators of services that embed `src/toolbox`. This module supplies library APIs and metric text; endpoint publication and alert routing belong to the host service.

## Startup and lifecycle

- Construct the toolbox after its workflow dependencies and backends are available.
- Load profiles with `ToolboxBuilder`, then verify profile-load warnings; failures are non-fatal to `build()`.
- If using global access, call `ToolboxRegistry::initialize()` once during serialized startup before serving requests. `isInitialized()` is suitable for a readiness check.
- Do not use `ToolboxRegistry::reset()` as a production recovery mechanism while requests may be active. A replacement instance can be initialized through the host's controlled reconfiguration path.
- Configure and keep each `ToolboxBuilder` on one thread; it is single-use.

## Observability surfaces

| Surface | Signal | Interpretation |
|---|---|---|
| Extraction | `toolbox_extract_calls_total`, `toolbox_extract_errors_total`, `toolbox_extract_empty_results_total`, `toolbox_extraction_failures_total`, `toolbox_extraction_latency_us_*` | Counters are process-local to the toolbox instance. An empty result can be normal; compare errors and calls. |
| Content bridge | `toolbox_bridge_failures_total`, `toolbox_bridge_graph_write_failures_total`, `toolbox_bridge_vector_write_failures_total`, `toolbox_bridge_latency_us_*` | Writer errors are separately counted and can coexist with `BridgeResult::ok == true`. |
| Registry | `toolbox_registry_misuse_total` in the global metrics export after the registry is initialized | A non-zero value indicates access before initialization or repeated initialization. The counter is process-global. |
| Logs | Builder profile warnings, registry initialization warning, content-manager errors, graph/vector writer failures, and explicit bridge errors | Correlate with request/content IDs and downstream subsystem logs. |

The helper metric-export functions (`getTextChunkerMetrics`, `getTextNormalizerMetrics`, and `getTextQualityScorerMetrics`) currently expose counters, but the corresponding helper implementations do not increment those counters on ordinary input paths. Do not use their zero values as proof that all text inputs were valid.

Metric exposition is conditional: extraction output is empty before the first recorded call; bridge exposition is empty until a measured operation completes. Early-return bridge failures increment the bridge failure counter but may not increment the operation count or appear in the returned metric text. Export the object accessors or add host-level error telemetry if those cases must be monitored.

Latency observations are coarse. The bridge records elapsed whole milliseconds and converts that value to microseconds before selecting buckets, so the histogram should not be read as sub-millisecond precision. The module does not itself serve an HTTP `/metrics` endpoint.

## Validation and release checks

- Run the primitive and contract test targets using the commands in [`tests/toolbox/README.md`](../../tests/toolbox/README.md).
- For performance-sensitive releases, build `bench_toolbox_native_workloads` and compare repeatable measurements against a recorded baseline using [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
- Record hardware, build profile, sample sizes, run count, and raw benchmark output. Do not treat historical gate-completion statements or the bridge placeholder benchmark as current release evidence.
- The toolbox has no module-owned profile reload command, queue-flush operation, or rollback CLI; use the host application's supported control plane.

## Recovery and rollback

1. Identify whether the failing stage is workflow execution, content storage, or an optional graph/vector sink.
2. Correct the responsible dependency or profile through the owning service configuration.
3. Rebuild or replace the toolbox instance using normal host lifecycle management and verify readiness plus the relevant counters.
4. If writes may have partially succeeded, reconcile downstream state before retrying. `ContentToolboxBridge::ingest()` stores content before enrichment, and `enrichExisting()` does not rescan or deduplicate and can repeat sink writes.
5. Roll back the host configuration or deployment if the failure persists; preserve the failed profile, logs, metrics, and affected IDs for the owning subsystem.

See [`RUNBOOK.md`](./RUNBOOK.md) for symptom-based triage and [`TRUST.md`](./TRUST.md) for security-sensitive boundaries.

> Exact model backend not verifiable from final document text alone.
