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

# Utils Implementation Guide

## Scope and ownership

`src/utils` implements shared cross-cutting helpers; public declarations are
in [`include/utils`](../../include/utils/README.md). Utility code should not
take ownership of domain workflows belonging to query, storage, server, or
other feature modules. Changes to a shared helper have broad downstream
impact, so preserve the public contract and failure behavior documented in
[`API.md`](API.md).

## Implementation map

| Concern | Implementation entry points | Invariant to preserve |
|---|---|---|
| Logging, audit, saga, tracing | `logger.cpp`, `audit_logger.cpp`, `saga_logger.cpp`, `tracing.cpp` | Keep diagnostic and audit responsibilities explicit; surface errors through the helper contracts |
| Privacy detection and transformation | `pii_detector.cpp`, `regex_detection_engine.cpp`, `ner_detection_engine.cpp`, `pii_pseudonymizer.cpp`, `pii_stream_scanner.cpp` | Validate input/configuration, retain bounded scanning, and distinguish detection failure from a clean scan |
| Key and PKI helpers | `hkdf_helper.cpp`, `hkdf_cache.cpp`, `lek_manager.cpp`, `pki_client.cpp` | Keep key ownership/lifecycle bounded and fail closed on invalid key or required trust material |
| Codec and serialization | `zstd_codec.cpp`, `lz4_codec.cpp`, `serialization.cpp` | Validate sizes and malformed input; prefer `Result` APIs where available |
| Runtime services | `thread_pool_manager.cpp`, `rate_limiter.cpp`, `http_client_pool.cpp`, `grpc_channel_pool.cpp` | Bound queues/waits and make rejection, timeout, and shutdown observable |
| General algorithms | `bloom_filter.cpp`, `consistent_hash.cpp`, `simd_distance.cpp`, `cron_parser.cpp`, `timestamp_utils.cpp` | Keep input assumptions and edge behavior deterministic and tested |

The canonical interface is the matching header under
[`include/utils`](../../include/utils/README.md); implementation files do not
create a separate public contract.

## Change workflow

1. Read [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), and
   [`ROADMAP.md`](ROADMAP.md), then inspect the relevant public header and
   implementation.
2. Identify direct callers and the focused test in
   [`tests/utils`](../../tests/utils/README.md). Shared-helper changes need
   tests for both normal operation and the affected failure boundary.
3. Keep locks scoped and short, use RAII for resources, and preserve explicit
   ownership, timeout, and cancellation behavior.
4. Validate malformed input, resource exhaustion, unavailable optional
   dependencies, and concurrency where relevant. Check benchmarks for
   performance-sensitive paths using
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
5. Update the API and operations docs when public behavior or recovery actions
   change.

## High-impact implementation contracts

- PII regex input and pattern size are bounded; matching can return partial
  findings on timeout. A detection exception is not equivalent to a clean
  result.
- NER rules rely on loaded gazetteer/context data; failed configuration reload
  should retain the prior valid state.
- Safe codec APIs return an explicit result, while legacy vector-returning
  wrappers can encode failure as an empty vector. Do not lose that distinction
  at call sites.
- Thread-pool submission can reject on shutdown or queue saturation; callers
  must handle `false`. Blocking rate-limit acquisition must use a bounded
  timeout in request paths.
- Key rotation and PKI errors must not trigger plaintext or unsigned
  substitution in production paths.

## Focused validation

The module's component suites are registered in
[`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt). The
[`test execution guide`](../../tests/utils/PHASE_4_TEST_EXECUTION_GUIDE.md)
documents the repository's existing configure/build/test workflow. See
[`RUNBOOK.md`](RUNBOOK.md) for failure-oriented triage.
