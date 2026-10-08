> **Status:** current | validated: 2026-10-08

<!-- Status: current | validated: 2026-10-08 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB ONNX CLIP Plugin - Production Requirements

## Purpose and Scope

This document is the canonical operational contract for the `onnx_clip` module.
It describes the minimum production requirements for the deterministic image-text
embedding backend implemented in `onnx_clip_plugin.cpp` and its public interface in
`onnx_clip_plugin.h`.

## Document Boundary (Canonical Split)

- **`src/onnx_clip/PRODUCTION_REQUIREMENTS.md` (this file):** mandatory production requirements, security assumptions, and operational bounds.
- **`src/onnx_clip/README.md`:** functional overview, runtime contract, and usage examples.
- **`src/onnx_clip/ARCHITECTURE.md`:** component design, interfaces, and data-flow description.
- **`src/onnx_clip/ROADMAP.md`:** delivery phases, open work, and readiness status.
- **`src/onnx_clip/SECURITY.md`:** threat model, security controls, and known limitations.
- **`src/onnx_clip/FUTURE_ENHANCEMENTS.md`:** medium and long-term implementation follow-up items.

## Mandatory Production Requirements

### Initialization and configuration

- **MUST:** `initialize(config, backend)` validates the incoming `PluginConfig` and returns `false` for invalid configuration values.
- **MUST:** `model.name` and `model.embedding_dim` are accepted as runtime configuration values; invalid dimensions are normalized to `512` instead of being used blindly.
- **MUST:** `max_batch_size` is applied as a hard upper bound for `generateEmbeddingBatch()` sub-batches.
- **MUST NOT:** rely on implicit defaults in production without explicit configuration and validation.

### Backend selection and availability

- **MUST:** the selected `BackendType` drives runtime behavior; when `AUTO` is used, the current implementation resolves deterministically to `CPU`.
- **MUST:** unsupported or unavailable backends must not silently mutate the active backend in a way that hides a configuration problem.
- **MUST:** the module remains safe when native backend providers are absent because the deterministic reference path does not assume a live ONNX session is present.

### Model integrity and verification

- **MUST:** when both `model.path` and `model.expected_sha256` are set, the file hash is checked during initialization.
- **MUST:** a hash mismatch causes initialization to fail and prevents the plugin from continuing in a production-unsafe state.
- **MUST:** built-in SHA-256 validation uses OpenSSL when `THEMIS_HAS_OPENSSL` is defined; otherwise an injected `ModelHashFn` may be used as the verification bridge.
- **MUST NOT:** skip the integrity check in hardened environments simply because an external backend or provider is unavailable.

### Request processing and thread safety

- **MUST:** all request entry points (`generateEmbedding()`, `generateEmbeddingBatch()`, `generateTextEmbedding()`) serialize access through the plugin instance state and must not race on the active implementation snapshot.
- **MUST:** concurrent threads are serialized by the implementation mutex and request guard pattern; partial results are not emitted for a single request.
- **MUST:** all failures return an `EmbeddingResult` with `success=false` and an error message rather than silently fabricating a successful embedding.

### Batch processing

- **MUST:** `generateEmbeddingBatch()` processes data in bounded sub-batches of `max_batch_size` while preserving request order.
- **MUST:** empty inputs or invalid payloads are rejected per item rather than being silently replaced.
- **MUST:** batch lengths and total inference counters must remain consistent with the number of processed images.

### Dynamic reload and hot-swap

- **MUST:** `reloadModel(new_config)` only swaps to a new implementation state when the current instance is initialized and the new configuration validates.
- **MUST:** in-flight requests continue until completion or the 30-second drain window expires; the old implementation remains valid until the new state is published.
- **MUST:** if the new configuration fails validation or hash verification, the old controller remains active and the function returns `false`.
- **MUST NOT:** call `reloadModel()` when the plugin is not ready unless the caller explicitly handles the failure result.

### Memory-mapping support

- **MUST:** `enable_mmap_loading=true` attempts a memory-mapped model load on Linux and Windows when a readable model file is available.
- **MUST:** the mmap path is best-effort and must not cause a hard failure when the platform does not support it or the model is unreadable.
- **MUST:** mmap usage is optional; the default remains a normal file-based configuration path.

## Mandatory Security Requirements

### Input validation and preprocessing

- **MUST:** image payloads are validated for emptiness before embedding generation is attempted.
- **MUST:** text payloads are validated for emptiness before tokenization and embedding generation.
- **MUST:** input shapes and data must remain bounded to the deterministic reference model contract and must not be expanded beyond the configured embedding dimension.

### Model trust boundary

- **MUST:** model file path and expected digest are treated as a trust boundary; if configured, the verification path is enforced rather than skipped.
- **MUST NOT:** store model files in writable paths in production deployments unless the deployment explicitly accepts the additional risk and documents the compensating controls.

### Resource protection

- **MUST:** batch and inference sizes remain bounded by `max_batch_size` and the embedding dimension so the plugin does not consume unbounded memory in a single request.
- **MUST:** request serialization prevents concurrent inferences from the same instance from mutating the same state without coordination.
- **MUST:** out-of-memory or verification failures are treated as structured failures, not as a crash path.

### Data sensitivity

- **MUST:** embeddings are treated as semantic feature vectors and not as biometric identifiers by default.
- **MUST NOT:** store or reuse face/biometric embeddings in ThemisDB indexes without an explicit policy review and access control configuration.

## Operational Bounds

### Configuration and environment

- Deployment-specific values must be set explicitly; default values are not a substitute for production configuration.
- Model paths must point to readable, access-controlled files that match the intended runtime environment.
- The module is intentionally portable and therefore does not assume a real GPU provider is present in every deployment.

### Performance and latency

- The reference implementation prioritizes deterministic output and bounded resource use over a native GPU provider path.
- The design accepts sequential sub-batching rather than a single native accumulator call, which is intentional for portability and predictable behavior.
- End-to-end latency is still trackable through `getStatistics()` and the per-call inference timings recorded in results.

### Resource limits

- `max_batch_size` is the primary bound for request fan-in and memory footprint.
- Statistics counters and request guards ensure that the plugin remains observable under load and can be safely hot-swapped.
- The default CPU limit is 16; the default non-CPU limit is 64; deployment overrides must be validated to match operational constraints.

## Minimum Production Check (audit-capable)

- [x] module configuration is validated during `initialize()`
- [x] backend selection is deterministic and explicit in the current implementation
- [x] model hash verification is active when both inputs are configured
- [x] image and text inputs are rejected when empty or invalid
- [x] request serialization is enforced with mutex/guard patterns
- [x] `generateEmbeddingBatch()` uses bounded sub-batching via `max_batch_size`
- [x] hot-swap reload path preserves request safety and validates on swap
- [x] errors are reported as structured `EmbeddingResult{success=false}` failures
- [x] statistics are available through `getStatistics()`
- [x] health checks and warmup are exposed via the public API
- [x] mmap support is available on supported platforms and automatically best-effort

## Review / Sourcecode Audit Evidence

### Affected files in scope

- `src/onnx_clip/onnx_clip_plugin.h`
- `src/onnx_clip/onnx_clip_plugin.cpp`
- `src/onnx_clip/CMakeLists.txt`
- `src/onnx_clip/README.md`
- `src/onnx_clip/ARCHITECTURE.md`
- `src/onnx_clip/ROADMAP.md`
- `src/onnx_clip/SECURITY.md`

### Validation

- Unit coverage: `tests/onnx_clip/` focused tests for embedding behavior, batching, and hot-swap behavior
- Integration coverage: `tests/integration/test_onnx_clip_soak.cpp`
- Additional contract checks: focused test targets for hot-swap and mmap behaviors
- Governance validation: `python scripts/check_module_direct_doxygen.py --module onnx_clip`

---

**Version:** 1.0
**Valid from:** 2026-10-08
**Next review:** 2026-12-08 or on any contract-changing revision
