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

# ONNX CLIP Production Requirements

Exact model backend not verifiable from final document text alone.

## Purpose and current support boundary

This page defines deployment constraints for `src/onnx_clip`. It is grounded in
[`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp) and
[`onnx_clip_plugin.h`](./onnx_clip_plugin.h).

The current implementation is a deterministic contract/reference backend. It
does not decode image formats, load CLIP weights, execute ONNX Runtime, or
produce learned semantic vectors. A successful `initialize()` means plugin
state was configured, not that an ONNX model/provider was loaded. Do not expose
the current output as semantic CLIP search without replacing the reference
algorithm and completing model-backed evaluation.

## Deployment requirements for current reference use

### Initialization and configuration

- Check the return value of `initialize()` and keep the plugin out of traffic on
  failure; separately check `isReady()` after initialization.
- Set `model.embedding_dim` to the consuming index dimension. Non-positive
  values silently revert to 512; positive values have no upper clamp.
- Configure `max_batch_size` to a resource-appropriate positive value. The
  source clamps it to at least 1, but does not impose an upper limit.
- Enforce maximum input bytes and total batch size at the host/API layer.
  `max_batch_size` bounds sequential processing chunks, not total request or
  result allocation.
- Treat backend enum/string values and `model.name` as labels/configuration.
  `BackendType::AUTO` selects CPU; other initialization enums are not runtime
  provider probes.

### Artifact integrity

- If digest checking is required, configure both `model.path` and
  `model.expected_sha256`.
- In builds with `THEMIS_HAS_OPENSSL`, the source computes SHA-256 and fails
  initialization/reload on read error or mismatch.
- In builds without OpenSSL, register `ModelHashFn`. If no callback is set, the
  source skips the digest check. Hardened deployments must fail configuration
  review rather than assume the expected digest was enforced.
- Keep artifact paths access-controlled. A digest match proves equality to the
  configured digest only; it does not prove provenance, ONNX validity, or that
  this implementation uses those bytes for inference.

### Mapping and reload

- `enable_mmap_loading` is a top-level boolean defaulting to false. It attempts
  read-only file mapping on supported code paths but the pointer is not used by
  an inference session. It is not a model loading or acceleration guarantee.
- `reloadModel()` rejects a non-ready current plugin and validates configured
  digest before publication. It publishes the replacement before waiting up to
  30 seconds for old-snapshot requests to drain.
- A `false` reload result can mean pre-publication failure or post-publication
  drain timeout. On timeout, the new state remains active. Save the prior
  config and inspect `getStatistics()` before attempting a rollback reload.

## Data, availability, and observability

- Reject failed results: empty image bytes and empty text return
  `success=false`; a call when not ready returns a structured failed result.
- Do not assume input decoding, MIME validation, semantic quality, or
  image/text-space alignment. These are absent from the source implementation.
- Store derived vectors only under the host's data access, retention, and
  privacy policy. The plugin does not authenticate callers, enforce quotas,
  persist data, or anonymize outputs.
- Use `getStatistics()` for a local JSON snapshot. It includes readiness,
  labels, dimension, chunk size, totals, errors, and average integer-ms
  latency. It does not create a Prometheus endpoint or provide latency
  percentiles.
- For host-level alerting and request attribution, combine plugin counters
  with application telemetry. The plugin does not emit its own log/trace
  events.

## Production acceptance gates for learned inference

Before deploying this module as an actual CLIP semantic backend, all of the
following are required:

1. A verified model artifact, digest/provenance record, preprocessing, and
   tokenizer contract.
2. Provider selection that probes availability and reports unsupported or
   failed providers without silently treating labels as active execution.
3. Model-backed tests for image and text embeddings, cross-modal retrieval
   quality, repeatability, and failure handling on each supported provider.
4. Measured latency, throughput, peak memory, request bounds, and a stated
   baseline on representative hardware.
5. Updated trust analysis, operational recovery, and a tested reload/rollback
   contract that distinguishes failed publication from a drain timeout.

## Current evidence

The existing focused tests in [`tests/onnx_clip/`](../../tests/onnx_clip/) and
[`tests/test_onnx_clip_plugin.cpp`](../../tests/test_onnx_clip_plugin.cpp)
exercise deterministic outputs, dimensions, normalization, batching,
statistics, hash callback, mmap resources, and reload behavior. Generated test
bytes and mock files do not establish production inference quality.

For detailed requirements by contract area, see
[`API.md`](./API.md), [`IMPLEMENTATION.md`](./IMPLEMENTATION.md),
[`OPERATIONS.md`](./OPERATIONS.md), and [`TRUST.md`](./TRUST.md).
