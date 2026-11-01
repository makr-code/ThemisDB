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

> Exact model backend not verifiable from final document text alone.

# ThemisDB Utils Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The utils module provides shared cross-cutting runtime building blocks for ThemisDB, including audit and structured logging, privacy utilities, key and checksum helpers, compression and serialization, concurrency and rate limiting, tracing, timestamp and cron helpers, and generic support code used by many other modules.

## Representative Implementations

| Implementation | Role |
|---|---|
| audit_logger.cpp | audit trail behavior |
| logger.cpp | structured logging behavior |
| saga_logger.cpp | saga logging behavior |
| pii_detection_engine.cpp | privacy scan orchestration behavior |
| pii_detector.cpp | PII detection behavior |
| pii_pseudonymizer.cpp | pseudonymization behavior |
| pii_stream_scanner.cpp | streaming privacy scan behavior |
| regex_detection_engine.cpp | regex-based detection behavior |
| hkdf_helper.cpp | key derivation behavior |
| hkdf_cache.cpp | derived-key cache behavior |
| lek_manager.cpp | local encryption key management behavior |
| zstd_codec.cpp | zstd compression behavior |
| lz4_codec.cpp | lz4 compression behavior |
| serialization.cpp | serialization behavior |
| simd_distance.cpp | SIMD distance behavior |
| rate_limiter.cpp | rate limiting behavior |
| thread_pool_manager.cpp | thread pool lifecycle behavior |
| tracing.cpp | tracing behavior |
| cron_parser.cpp | cron parsing behavior |
| timestamp_utils.cpp | timestamp conversion behavior |
| consistent_hash.cpp | consistent hash behavior |
| bloom_filter.cpp | probabilistic membership behavior |

## Scope

In scope:
- shared runtime helpers and infrastructure reused across ThemisDB modules
- utility-layer privacy, crypto, compression, tracing, concurrency, and support behavior

Out of scope:
- business-domain orchestration and feature logic owned by higher-level modules
- module-specific storage, query, network, or indexing behavior outside utility boundaries

## Runtime Behavior and Limits

- utils surfaces are consumed by many modules and therefore must remain bounded, explicit, and reusable.
- utility hot paths include privacy scan, compression, thread-pool, and SIMD helper behavior.
- selected components depend on host libraries or external services, but failures must degrade predictably.

## Source and Build Verification (Module: utils/readme)

- Representative source files checked (not an exhaustive inventory):
  - src/utils/audit_logger.cpp
  - src/utils/logger.cpp
  - src/utils/saga_logger.cpp
  - src/utils/pii_detection_engine.cpp
  - src/utils/pii_detector.cpp
  - src/utils/pii_pseudonymizer.cpp
  - src/utils/pii_stream_scanner.cpp
  - src/utils/regex_detection_engine.cpp
  - src/utils/hkdf_helper.cpp
  - src/utils/hkdf_cache.cpp
  - src/utils/lek_manager.cpp
  - src/utils/zstd_codec.cpp
  - src/utils/lz4_codec.cpp
  - src/utils/serialization.cpp
  - src/utils/simd_distance.cpp
  - src/utils/rate_limiter.cpp
  - src/utils/thread_pool_manager.cpp
  - src/utils/tracing.cpp
  - src/utils/cron_parser.cpp
  - src/utils/timestamp_utils.cpp
  - src/utils/consistent_hash.cpp
  - src/utils/bloom_filter.cpp
- Benchmark sources under `benchmarks/utils/`:
  - `bench_utils_release_gates.cpp` is the only benchmark registered by
    `benchmarks/utils/CMakeLists.txt`.
  - `bench_pii_stream_scanner.cpp`, `benchmark_runner.cpp`, and
    `phase_a3_benchmark_runner.cpp` exist but are not registered by that
    CMake file. The Phase A.3 runner uses simulated work; its historic
    measurements are not production-path performance evidence.
- `tests/utils/CMakeLists.txt` registers focused tests and discovers
  `test_utils_*.cpp` files. The root `tests/CMakeLists.txt` includes module
  test directories that contain a `CMakeLists.txt`.
- See [`tests/utils/README.md`](../../tests/utils/README.md) for build and test
  commands and [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
  for the current benchmark boundary.