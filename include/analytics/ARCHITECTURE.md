> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/analytics/ARCHITECTURE.md -->

# ANALYTICS Module — Public Header Architecture

**Module Path:** `include/analytics/`
**Implementation:** `../../src/analytics/`
**Canonical architecture doc:** [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)

---

## 1. Overview

The `include/analytics/` directory contains the **public C++ header contract** for ThemisDB's multi-surface runtime for analytical workloads — OLAP execution, streaming/CEP processing, forecasting, anomaly detection, and model-serving integrations. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::analytics`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `OLAP and aggregation` | `olap.h`, `columnar_execution.h`, `jit_aggregation.h`... |
| `Streaming and CEP` | `streaming_window.h`, `streaming_join.h`, `cep_engine.h` |
| `Predictive and ML integration` | `forecasting.h`, `anomaly_detection.h`, `automl.h`... |
| `Distributed and knowledge` | `distributed_analytics.h`, `expert_system_engine.h`, `knowledge_base.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_ML_SERVING` | ml_serving.h, model_serving.h | ML/model-serving runtime |
| `THEMIS_ENABLE_ARROW` | arrow_export.h, arrow_flight.h | Apache Arrow / Flight libraries |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)
- Module overview: [`../../src/analytics/README.md`](../../src/analytics/README.md)
- Roadmap: [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)
- Future enhancements: [`../../src/analytics/FUTURE_ENHANCEMENTS.md`](../../src/analytics/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
