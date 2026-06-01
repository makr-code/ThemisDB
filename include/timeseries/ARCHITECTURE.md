> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/timeseries/ARCHITECTURE.md -->

# TIMESERIES Module — Public Header Architecture

**Module Path:** `include/timeseries/`
**Implementation:** `../../src/timeseries/`
**Canonical architecture doc:** [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md)

---

## 1. Overview

The `include/timeseries/` directory contains the **public C++ header contract** for ThemisDB's high-frequency ingest and storage, compression and adaptive flush, aggregation and query optimization, and retention/encryption/remote-write lifecycle. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::timeseries`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Ingest and storage` | `timeseries.h`, `tsstore.h`, `hypertable.h`... |
| `Compression and query` | `gorilla.h`, `gorilla_simd.h`, `compression_selector.h`... |
| `Lifecycle and integration` | `continuous_agg.h`, `aggregate_scheduler.h`, `retention.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_SIMD` | gorilla_simd.h | SIMD-accelerated Gorilla compression |
| `THEMIS_ENABLE_PROMETHEUS` | prometheus_remote_write.h, timeseries_metrics.h | Prometheus remote-write endpoint |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md)
- Module overview: [`../../src/timeseries/README.md`](../../src/timeseries/README.md)
- Roadmap: [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)
- Future enhancements: [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
