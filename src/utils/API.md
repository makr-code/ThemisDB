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

Exact model backend not verifiable from final document text alone.

# Utils API Contracts

## Audience and source of truth

This page is an integration map for consumers of the public
[`include/utils`](../../include/utils/README.md) headers. The header declaration
and implementation define the exact signature and behavior; this summary does
not supersede them. Internal `.cpp` files under `src/utils` are not public API.

## Main interface groups

| Group | Public entry points | Contract and caller responsibility |
|---|---|---|
| Privacy | `PIIDetector`, `IPIIDetectionEngine`, `PIIStreamScanner`, `PIIPseudonymizer` | Findings include type, value, offsets, confidence, and engine identity. Validate results and distinguish errors from no findings; configure streaming lookahead for cross-chunk spans. |
| Audit and logs | `AuditLogger`, `Logger`, `SagaLogger` | Use the dedicated helpers for structured/audit events. Handle enqueue, flush, persistence, and signature errors according to the concrete class contract. |
| Crypto and keys | `HkdfHelper`, `HkdfCache`, `LEKManager`, `VCCPKIClient` | Keep key-provider and storage lifetimes valid. Validate all result flags; key retrieval and configured signature verification can fail. |
| Compression | `zstd_*_safe`, `lz4_*_safe`, streaming Zstd classes | Prefer `Result`-based APIs. Validate encoded sizes, decompression bounds, and codec availability. LZ4 block decoding also requires the original size. |
| Runtime | `ThreadPool`, `ThreadPoolManager`, `RateLimiter`, connection pools | Handle rejected submissions, exhausted capacity, and timeout returns. Stop/join owned workers during shutdown. |
| Data and time helpers | `BloomFilter`, `ConsistentHash`, SIMD/geospatial helpers, `CronParser`, timestamp utilities | Follow each header's input constraints and synchronization guarantees; do not infer thread safety across unrelated helpers. |

## Important edge behavior

- `PIIDetectionEngineFactory::createSigned` requires signature metadata and
  successful PKI verification. The current factory recognizes `regex` and
  `ner`; see [`EMBEDDING.md`](EMBEDDING.md) for the advertised-but-not-
  instantiable embedding name.
- `PIIDetector` catches individual engine exceptions and logs them before
  merging remaining findings. A direct detector result may therefore be
  incomplete after an engine failure. `PIIStreamScanner` has a separate
  fail-closed exception contract; select the API based on the caller's
  fail-open/fail-closed requirement.
- The safe Zstd/LZ4 APIs return explicit success/error values. Legacy
  vector-returning wrappers can return an empty vector on failure or when the
  optional codec is unavailable.
- A `RateLimiter` request for non-positive tokens is a no-op according to its
  header contract. Requests larger than the burst capacity cannot succeed;
  bounded acquisition can return false on timeout.
- `ThreadPool::submit` returns false when stopped or when queue capacity is
  not available within the requested wait. Do not silently discard that
  result.

## Error and compatibility policy

Utility failures are represented per API as an error result, exception,
boolean/optional return, diagnostic context, or legacy empty value. Consumers
must not assume one uniform error mechanism. The common taxonomy and
diagnostic helpers are declared in
[`error_contracts.h`](../../include/utils/error_contracts.h); check the
individual header for exact categories and recoverability.

Shared public contracts are consumed broadly. A behavior change requires
focused regression coverage and documentation of compatibility impact; do not
add an undocumented fallback that hides configuration, storage, key, or
resource errors. See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) and
[`MODULE_GUIDE.md`](MODULE_GUIDE.md).
