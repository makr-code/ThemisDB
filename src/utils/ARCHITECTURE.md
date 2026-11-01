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

# Architecture - Utils Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The utils module groups foundational helper subsystems that other ThemisDB modules consume for logging, privacy, key handling, compression, tracing, concurrency, and generic support behavior.

## Main Execution Planes

1. Observability plane
- structured logging, audit logging, saga logging, and tracing behavior

2. Privacy and key plane
- PII detection, pseudonymization, key derivation, and local key lifecycle behavior

3. Runtime services plane
- compression, serialization, thread-pool, rate-limiter, timestamp, cron, and numeric helper behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| observability contract | explicit logging, audit, and tracing helper behavior |
| privacy/key contract | bounded privacy scan and key helper semantics |
| runtime service contract | reusable support behavior without business-domain ownership |

## Failure Semantics

- utility failures must remain explicit and must not silently change consuming module behavior.
- failure handling is component-specific: callers must inspect each helper's documented result, exception, timeout, or diagnostic rather than assume one shared fallback.
- hot-path helpers remain bounded and measurable through benchmark-backed verification.

## Data Flow and Ownership

1. A consumer includes a public declaration from `include/utils/`.
2. The corresponding implementation in `src/utils/` validates input and
   invokes its component-specific library or service dependencies.
3. The helper returns its documented result or reports failure through its
   documented error channel. The consumer owns policy decisions such as
   retrying, rejecting a request, or treating a partial scan as incomplete.

The public headers define the API and thread-safety contract. Do not infer
thread safety, fallback behavior, or ownership guarantees between different
utility components.

## Build and Test

The utility implementation is built as part of the repository's core target;
there is no separate utils library target. Module tests are registered in
[`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt), which is
included by the root test build. The registered release-gate benchmark is
`bench_utils_release_gates`; see
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for its scope.

## Sourcecode Verification (Module: utils/architecture)

- Verified files:
  - src/utils/audit_logger.cpp
  - src/utils/pii_detection_engine.cpp
  - src/utils/hkdf_helper.cpp
  - src/utils/zstd_codec.cpp
  - src/utils/thread_pool_manager.cpp
  - src/utils/tracing.cpp
  - src/utils/simd_distance.cpp
- Verified architecture claims:
  - observability, privacy/key, and runtime service plane split
  - explicit failure boundaries for shared utility behavior
  - module-local ownership of cross-cutting helper infrastructure

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| Core/security and storage interfaces | For example, `security/encryption.h` and RocksDB-backed helpers | Some utility headers directly depend on shared core/security/storage types; inspect the specific header before treating a helper as standalone |
| External libraries | OpenSSL, spdlog, JSON, compression and optional tracing/network libraries | Availability and failure behavior are component- and build-configuration-specific |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| ThemisDB consumers | `include/utils/` | Utilities are shared broadly; each consuming module depends on its selected helper surfaces |

## Integration Points

### Integration Boundaries

| Surface | Ownership and caller responsibility |
|---|---|
| Logging, audit, and tracing | Use the specific header contract for event shape, initialization, flushing, and errors; no repository-wide routing or ordering guarantee is implied. |
| Thread pools and rate limiting | The caller handles rejected work and timeout outcomes and keeps captured dependencies alive for the task lifetime. |
| Compression and serialization | The caller selects the codec API and respects its documented size bounds, frame state, and error result. |

These are integration reminders, not substitutes for the API contracts in
[`API.md`](API.md) and the corresponding public headers.