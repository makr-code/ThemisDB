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

# ONNX CLIP Operations

Exact model backend not verifiable from final document text alone.

## Operational role

The plugin offers deterministic, normalized reference vectors through the
ThemisDB image-analysis interface. It does not decode images, execute an ONNX
model, or establish semantic compatibility between image and text vectors.
Operators must not enable user-facing similarity/search behavior on the
assumption that a model was loaded merely because initialization succeeded.

## Lifecycle and capacity

- Initialize before routing requests; `healthCheck()` reports only ready state
  plus positive embedding dimension.
- The default output dimension is 512. The default sequential chunk size is
  16 for CPU and 64 for other backend labels. Configured chunk sizes have a
  minimum of 1 but no source-level maximum.
- Bound request count and payload bytes in the API/service layer. The batch
  method retains one result per input; chunking does not bound total request
  allocation.
- Calls on a plugin instance are serialized by its implementation mutex.
  Concurrency at the service layer can therefore queue on a single instance.
- Shutdown marks the current instance not ready and releases mmap resources.

## Signals and interpretation

`getStatistics()` is the only module-local operational snapshot. It includes
readiness, backend and model labels, dimension, configured chunk size, totals,
error count, average latency, and three counters named
`clip_embeddings_total`, `clip_text_embeddings_total`, and
`clip_batch_embeddings_total`. These values are protected by the plugin mutex
and returned as JSON. They are not exported automatically to Prometheus.

The latency fields are integer milliseconds recorded per method call. Very fast
reference calls can report `0 ms`; `avg_latency_ms` divides accumulated call
latency by `total_inferences`. A batch adds one call duration while counting
each image as an inference, so this value is neither a per-call mean nor a
percentile or provider-only measure.
The batch counter named `clip_batch_embeddings_total` increases by image item
count, while `total_batches` increases once per batch API request.

## Routine checks

1. At startup, verify `initialize()` returns true, `isReady()` is true, and
   `healthCheck()` passes.
2. Verify configured dimension and `max_batch_size` match caller/index
   expectations.
3. During operation, sample statistics and compare errors/counts with the
   caller's request telemetry; plugin counters do not identify callers.
4. Before reload, retain the previous configuration and collect a statistics
   snapshot. A false reload result can occur after new-state publication.
5. Before enabling real inference in a future implementation, require a
   model-backed validation set, provider-level health checks, and revised
   memory/latency limits.

## Recovery boundaries

The module cannot restart an external inference service, recover a corrupted
model session, emit its own metrics, or undo vector writes in a database. For
these actions use the host application/database's procedures. If semantic
quality is unexpected, this source implementation has no learned model to
recover; escalate as a capability/rollout mismatch rather than retrying the
same input.

See [`RUNBOOK.md`](./RUNBOOK.md) for symptom-specific triage and reload
rollback conditions.
