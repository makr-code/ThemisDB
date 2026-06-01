> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/analytics/ROADMAP.md -->

# ANALYTICS Module — Public Header Roadmap

**Module Path:** `include/analytics/`
**Canonical implementation roadmap:** [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/analytics/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)

---

## Current Status

production-grade analytics runtime with OLAP, streaming, CEP, forecasting, and ML-serving paths. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `olap.h` — olap and aggregation contract
- [x] `columnar_execution.h` — olap and aggregation contract
- [x] `jit_aggregation.h` — olap and aggregation contract
- [x] `incremental_view.h` — olap and aggregation contract
- [x] `diff_engine.h` — olap and aggregation contract
- [x] `arrow_export.h` — olap and aggregation contract
- [x] `streaming_window.h` — streaming and cep contract
- [x] `streaming_join.h` — streaming and cep contract
- [x] `cep_engine.h` — streaming and cep contract
- [x] `forecasting.h` — predictive and ml integration contract
- [x] `anomaly_detection.h` — predictive and ml integration contract
- [x] `automl.h` — predictive and ml integration contract
- [x] `ml_serving.h` — predictive and ml integration contract
- [x] `model_serving.h` — predictive and ml integration contract
- [x] `lora_pattern_classifier.h` — predictive and ml integration contract
- [x] `llm_process_analyzer.h` — predictive and ml integration contract
- [x] `nlp_text_analyzer.h` — predictive and ml integration contract
- [x] `distributed_analytics.h` — distributed and knowledge contract
- [x] `expert_system_engine.h` — distributed and knowledge contract
- [x] `knowledge_base.h` — distributed and knowledge contract
- [x] `process_mining.h` — distributed and knowledge contract
- [x] `process_pattern_matcher.h` — distributed and knowledge contract
- [x] `arrow_flight.h` — distributed and knowledge contract

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

- Canonical implementation roadmap: [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
