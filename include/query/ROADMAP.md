---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `.`, `../../src/query/ROADMAP.md`, `../../ROADMAP.md`, `../../CMakePresets.json`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Historical planning snapshot: last status refresh 2026-06-01; use src/query/ROADMAP.md and root ROADMAP.md for current delivery posture. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/query/ROADMAP.md -->

# Query Module — Public Header Roadmap

**Module Path:** `include/query/`
**Canonical implementation roadmap:** [`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md)

---

## Overview

This document tracks public query API contract stability, planned header additions, and breaking changes for `include/query/`. Feature items affecting both implementation and headers are tracked in:

→ [`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md)

---

## Current Status

The inventory and planning items below were last status-refreshed on 2026-06-01. Header presence can be checked in this directory; this document does not establish current production stability or feature delivery. Use `src/query/ROADMAP.md` and the root `ROADMAP.md` for current implementation posture.

---

## Completed ✅

- [x] All AQL headers: `aql_parser.h`, `aql_runner.h`, `aql_translator.h`, `aql_safety_validator.h`
- [x] Multi-language parsers: `sql_parser.h`, `cypher_parser.h`, `sparql_parser.h`, `gremlin_parser.h`, `graphql_dialect.h`
- [x] Core execution: `query_engine.h`, `query_compiler.h`, `parallel_executor.h`, `vectorized_execution.h`
- [x] Optimization: `query_optimizer.h`, `adaptive_optimizer.h`, `runtime_reoptimizer.h`, `plan_cache.h`
- [x] Caching: `query_cache.h`, `semantic_cache.h`, `workload_cache_strategy.h`, `cte_cache.h`
- [x] Aggregation: `approximate_aggregator.h`, `statistical_aggregator.h`, `window_evaluator.h`
- [x] Continuous queries: `continuous_query_engine.h`, `continuous_query_registry.h`, `cq_watermark.h`
- [x] Federation: `query_federation.h`, `cross_cluster_federation.h`
- [x] Tensor-aware: `tensor_aware_query_optimizer.h`, `tensor_contraction_engine.h`, `tensor_rag_cost_model.h`

---

## In Progress

- [ ] Align `tensor_rag_cost_model.h` interface with production benchmark results (Target: 2026-Q3)
- [ ] Link `semantic_cache.h` to the LLM/LoRA embedding pipeline (Target: 2026-Q3)

---

## Planned

- [ ] `query_explain_api.h` — structured EXPLAIN output (JSON/DOT/text) for tooling integration (Target: 2026-Q3)
- [ ] `query_admission_controller.h` — per-priority query admission against resource limits (Target: 2026-Q4)
- [ ] Deprecate `continuous_query_engine_impl.h` direct include — expose only via `ContinuousQueryEngine` factory (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Any breaking change requires a MAJOR version bump; see `VERSIONING.md`.
