> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/timeseries/ROADMAP.md -->

# TIMESERIES Module — Public Header Roadmap

**Module Path:** `include/timeseries/`
**Canonical implementation roadmap:** [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/timeseries/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)

---

## Current Status

production timeseries runtime with hypertable ingest, Gorilla compression, adaptive flush, continuous aggregation, and retention. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `timeseries.h` — ingest and storage contract
- [x] `tsstore.h` — ingest and storage contract
- [x] `hypertable.h` — ingest and storage contract
- [x] `ts_auto_buffer.h` — ingest and storage contract
- [x] `ts_auto_buffer_adaptive.h` — ingest and storage contract
- [x] `ts_stream_cursor.h` — ingest and storage contract
- [x] `adaptive_flush_controller.h` — ingest and storage contract
- [x] `gorilla.h` — compression and query contract
- [x] `gorilla_simd.h` — compression and query contract
- [x] `compression_selector.h` — compression and query contract
- [x] `query_optimizer.h` — compression and query contract
- [x] `downsampling.h` — compression and query contract
- [x] `gap_fill.h` — compression and query contract
- [x] `aggregates.h` — compression and query contract
- [x] `continuous_agg.h` — lifecycle and integration contract
- [x] `aggregate_scheduler.h` — lifecycle and integration contract
- [x] `retention.h` — lifecycle and integration contract
- [x] `encrypted_chunk_store.h` — lifecycle and integration contract
- [x] `ts_encrypted_key_rotation.h` — lifecycle and integration contract
- [x] `prometheus_remote_write.h` — lifecycle and integration contract
- [x] `timeseries_metrics.h` — lifecycle and integration contract
- [x] `anomaly_detection.h` — lifecycle and integration contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
