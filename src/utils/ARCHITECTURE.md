# Architecture - Utils Module

<!-- Status: current | validated: 2026-09-09 -->
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
- host-library or external-service dependencies degrade predictably rather than expanding failure domains.
- hot-path helpers remain bounded and measurable through benchmark-backed verification.

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
| (none — Layer 0) | stdlib, nlohmann/json, spdlog | No ThemisDB module dependencies; relies only on third-party and standard libraries |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| ALL modules | `include/utils/` | Every ThemisDB module consumes at least one utils surface (logging, compression, thread-pool, tracing, etc.) |

## Integration Points

### Critical Integration: Observability Pipeline
**Files:** `src/utils/audit_logger.cpp`, `src/utils/tracing.cpp`
**Contract:** All audit-log and trace calls across ThemisDB route through utils helpers; output format is fixed (structured JSON / W3C TraceContext); callers must not build log payloads outside these helpers.
**Thread Safety:** Logger and tracer helpers are thread-safe; each call is independently atomic; no cross-call ordering guarantees.

### Critical Integration: Thread-Pool Services
**Files:** `src/utils/thread_pool_manager.cpp`
**Contract:** Modules obtain worker threads from the shared pool; tasks must not capture pool references beyond their own lifetime.
**Thread Safety:** Thread pool is fully thread-safe; submit() may be called from any thread concurrently.

### Critical Integration: Compression Codec
**Files:** `src/utils/zstd_codec.cpp`
**Contract:** Codec is stateless and re-entrant; compressor/decompressor instances are not shared across threads without external synchronisation.
**Thread Safety:** Stateless encode/decode calls are safe; streaming context objects must not be shared.