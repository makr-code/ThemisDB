> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/timeseries/FUTURE_ENHANCEMENTS.md -->

# TIMESERIES Module — Public Header Future Enhancements

**Module Path:** `include/timeseries/`
**Canonical implementation enhancements:** [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Scope

This document covers planned enhancements to the **public header contract** in `include/timeseries/` — new types, interface additions, deprecation removals, and header-level API improvements. Enhancements that touch both headers and implementation are tracked primarily in the canonical source-level document:

→ [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Headers must remain backward-compatible within a major version; new capabilities are added via new methods or versioned types.
- `[x]` `#pragma once` guard required on every header; no include-guard macros.
- `[x]` No implementation code in headers (exception: `constexpr` helpers, template bodies, and header-only utilities explicitly documented as such).
- `[x]` All factory functions and error-returning methods must be `[[nodiscard]]`.
- `[x]` Build-conditional headers must not be included unconditionally by other headers.

---

## Execution Plane Surface

- **Ingest and storage plane:** `timeseries.h`, `tsstore.h`, `hypertable.h`, `ts_auto_buffer.h`, `ts_auto_buffer_adaptive.h`, `ts_stream_cursor.h`, `adaptive_flush_controller.h`
- **Compression and query plane:** `gorilla.h`, `gorilla_simd.h`, `compression_selector.h`, `query_optimizer.h`, `downsampling.h`, `gap_fill.h`, `aggregates.h`
- **Lifecycle and integration plane:** `continuous_agg.h`, `aggregate_scheduler.h`, `retention.h`, `encrypted_chunk_store.h`, `ts_encrypted_key_rotation.h`, `prometheus_remote_write.h`, `timeseries_metrics.h`, `anomaly_detection.h`

For the authoritative interface inventory and stability classification see [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md).

---

## Planned Header Enhancements

### 1. `[[nodiscard]]` Audit

**Priority:** Medium
**Target Version:** v2.1.0

Audit all public headers for factory functions and error-returning methods that are missing `[[nodiscard]]`. Apply missing annotations and add a CI compile-time check to prevent regressions.

---

### 2. Deprecated Symbol Cleanup

**Priority:** Low
**Target Version:** v2.1.0

Identify symbols that have been superseded in `v1.x` and annotate them with `[[deprecated("use X instead")]]`. Track removal in a subsequent major version.

---

### 3. Header Isolation Verification

**Priority:** Low
**Target Version:** v2.1.0

Verify that every header in `include/timeseries/` compiles in isolation (without implicit transitive includes). Add a CMake `check_headers` target for automated CI enforcement.

---

## Test Strategy

| Test Type | Target | Notes |
|---|---|---|
| Compile-time | All headers compile in isolation | CMake `check_headers` target (planned) |
| Unit | Key interface implementations | Tracked in module test suite |
| ABI | No unexpected virtual table changes between patch releases | ABI checker in CI |

---

## Security / Reliability

- `[x]` `[[nodiscard]]` applied to factory and error-returning methods.
- `[x]` No implementation code in public headers.
- `[x]` Build-conditional guards documented in `ARCHITECTURE.md`.

---

## References

- Canonical implementation enhancements: [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Module overview: [`README.md`](README.md)
