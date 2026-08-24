> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-08-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/analytics/ROADMAP.md -->

# Analytics Module — Public Header Roadmap

**Module Path:** `include/analytics/`
**Canonical implementation roadmap:** [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)

---

## Overview

Tracks public analytics API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)

---

## Current Status

All 24 analytics headers are present. Public entry points exist for OLAP, columnar/JIT execution, CEP, streaming joins/windows, forecasting, anomaly detection, AutoML, ML serving, NLP, process mining, Arrow export, and distributed analytics coordination.

---

## Completed ✅

- [x] `olap.h`, `columnar_execution.h`, `jit_aggregation.h` — OLAP and vectorised-execution contract
- [x] `cep_engine.h`, `streaming_window.h`, `streaming_join.h` — CEP and streaming surfaces
- [x] `forecasting.h`, `anomaly_detection.h`, `diff_engine.h` — predictive and diff surfaces
- [x] `automl.h`, `ml_serving.h`, `model_serving.h` — AutoML and model-serving lifecycle
- [x] `nlp_text_analyzer.h`, `knowledge_base.h`, `expert_system_engine.h` — NLP and knowledge surfaces
- [x] `process_mining.h`, `process_pattern_matcher.h`, `llm_process_analyzer.h`, `lora_pattern_classifier.h` — process-mining and LLM-analytics surfaces
- [x] `arrow_export.h`, `arrow_flight.h`, `analytics_export.h`, `distributed_analytics.h` — export and distribution

---

## In Progress

- [ ] Align streaming-window and CEP header docs around shared event-schema expectations (Target: 2026-Q3)
- [ ] Document Arrow Flight server-capability matrix for batch-vs-streaming consumers (Target: 2026-Q3)
- [x] Extend serving-header security contract with model-integrity and transport-policy controls (`model_serving.h`, `ml_serving.h`) (Completed 2026-08-19)

---

## Planned

- [ ] `analytics_policy.h` — per-query resource and access-policy contract (Target: 2026-Q4)
- [ ] Mark experimental JIT/automl headers and add stability annotations (Target: 2026-Q4)
- [ ] Document benchmark-backed throughput and latency targets for columnar hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Analytics headers maintain backward compatibility within the active major line; contract changes require migration notes and changelog updates.
