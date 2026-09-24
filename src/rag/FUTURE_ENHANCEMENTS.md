# RAG Module - Future Enhancements

<!-- Status: current | validated: 2026-09-24 | Test Coverage Expanded Q3 2026 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

## Scope

Forward-looking enhancements for retrieval quality, context reliability, evaluation trustworthiness, and operational hardening in the RAG module.

## Design Constraints

- Preserve stable retrieval and evaluation interfaces for existing consumers.
- Keep context assembly deterministic under equivalent inputs and budgets.
- Ensure safety checks execute before potentially unsafe generation paths.
- Keep optional backend features degradable with explicit fallback signaling.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| retrieval and fusion APIs | handlers and orchestration paths | stable document ranking and scoring semantics |
| context assembly interfaces | generation and quality-control paths | deterministic budget and truncation behavior |
| quality and judge interfaces | policy and response governance | explicit pass/fail and diagnostic surfaces |
| ingestion bridge interfaces | ingestion and retrieval runtime | canonical document hydration and enrichment |
| safety detector/sanitizer interfaces | request and retrieval processing | prompt/context safety enforcement |

## Implementation Notes

### Retrieval and Context Hardening
**Priority:** High
**Target:** Q3-Q4 2026

- consolidate deterministic ranking and tie-break behavior in retrieval flows
- harden context assembly for malformed inputs and metadata gaps
- standardize truncation and budget-reservation semantics across orchestrators

### Quality and Safety Hardening
**Priority:** High
**Target:** Q4 2026

- strengthen quality-gate explainability without leaking sensitive internals
- expand prompt-injection and adversarial-context regression scenarios
- align evaluator failure envelopes for degraded backend/runtime conditions

### Distributed and Operational Hardening
**Priority:** Medium
**Target:** Q4 2026

- broaden distributed retrieval/evaluation coverage under partial failures
- improve observability for deny/fallback paths and budget decisions
- increase reliability of operator-facing diagnostics and audit evidence

### Performance and Capacity Hardening
**Priority:** Medium
**Target:** Q1 2027

- re-baseline retrieval and end-to-end latency envelopes by release profile
- keep cache and context assembly overhead bounded under sustained load
- lock benchmark-backed release thresholds for critical RAG paths

### Wave B B1: Self-RAG (Self-Retrieving, Auto-Critique)
**Priority:** High
**Target:** Q1–Q2 2027

- add retrieval controller to gate when retrieval is re-invoked
- add three-class critic output (Relevant/Partial/Irrelevant) for context quality scoring
- implement bounded iterative refinement loop (max three rounds)
- wire callback integration with `InferenceEngineEnhanced`
- benchmark ALCE quality/latency deltas against vanilla RAG baseline

## Test Strategy

### Implemented (Q3 2026)
- ✅ Focused regression suites for ingestion bridge and context assembly behavior
  - Test file: `tests/rag/test_rag_ingestion_bridge_hardening_focused.cpp`
  - Coverage: Groups A-E (19 tests) for fail-closed behavior, metadata handling, deterministic hydration
- ✅ Budget/selection determinism tests across adaptive and multi-step paths
  - Test file: `tests/rag/test_rag_budget_consistency_focused.cpp`
  - Coverage: Groups A-E (20 tests) for budget determinism, propagation, multi-step consistency
- ✅ Error handling and edge-case regression matrix
  - Test file: `tests/rag/test_rag_error_handling_edge_cases_focused.cpp`
  - Coverage: Groups A-E (23 tests) for malformed context, invalid budgets, partial failures, resource exhaustion
- ✅ Safety/adversarial regression matrix for prompt and retrieved context payloads
  - Existing coverage: `tests/rag/test_rag_prompt_injection.cpp`
  - Extended with edge-case adversarial scenarios

### In Progress / Planned
- [ ] Benchmark regression gates for retrieval, judge, and end-to-end workflows
  - Infrastructure: `tests/performance/test_rag_ttft_benchmark.cpp`
  - Gap: Release-profile threshold mapping and validation

## Performance Targets

- stable p95 and p99 envelopes for representative RAG production workloads
- bounded throughput regressions against release baselines
- bounded memory and queue growth under sustained retrieval/evaluation load

## Security / Reliability

- fail closed on invalid retrieval/context states and unsafe preconditions
- preserve deterministic safety and quality gate behavior
- prevent unbounded growth in cache, queue, and context buffers

## Wave B Acceptance Gates (B1)

- hallucination rate reduction ≥ 20% vs standard RAG
- latency increase ≤ 1.5× vs baseline
- precision@k retrieval ≥ 0.85 on golden-doc tests

## Related Documents

- AI wave tracker: `../ai/ROADMAP.md`
- bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Risk Backlog

### Risk 1: retrieval divergence under mixed backend conditions
**Severity:** High
**Signal:** unstable ranking/output behavior under equivalent inputs.
**Mitigation Status:** 🟢 High confidence
- Deterministic ordering rules implemented in context_assembler
- Cross-path regression packs created: test_rag_budget_consistency_focused.cpp (Group C)
- Same-budget determinism tests: test_rag_budget_consistency_focused.cpp (Group A1-A4)

### Risk 2: context budget drift across orchestration variants
**Severity:** Medium
**Signal:** inconsistent truncation/selection outcomes across paths.
**Mitigation Status:** 🟡 In progress
- Shared budget utilities in place (ContextWindowBudget contract)
- Contract tests created: test_rag_budget_consistency_focused.cpp (20 tests)
- Multi-step consistency tests: test_rag_budget_consistency_focused.cpp (Group C1-C2)
- Gap: Integration testing across all orchestration variants pending

### Risk 3: safety detection quality drift
**Severity:** Medium
**Signal:** increased false-negative or false-positive rates.
**Mitigation Status:** 🟡 Ongoing
- Calibration infrastructure in place
- Adversarial regression scenarios: test_rag_prompt_injection.cpp
- Additional edge-case coverage: test_rag_error_handling_edge_cases_focused.cpp (malformed context tests)

## RAG-Readiness Audit Backlog (2026-09-23)

- [ ] RAG Eval Contract v1 ueber Modulgrenzen verankern (Target: Q4 2026)
  - Rationale: Der Codepfad ist stark, aber Qualitaetsaussagen sind nicht einheitlich release-gebunden.
  - Umsetzungsschritte: (1) goldene Datensaetze pro Domaintyp definieren, (2) Pflichtmetriken `Recall@10`, `nDCG@10`, `MRR@10`, `Faithfulness`, `p95 latency`, `cost/query` zentral sammeln, (3) release_critical-Gate mit Fail-on-regression aktivieren.
  - Abhaengigkeiten: `src/llm`, `src/query`, `src/observability`, `tests/rag`, `tests/integration`.
  - Messbares DoD: 100% der release-kritischen RAG-Laeufe publizieren den kompletten Metrik-Satz; keine Regression >2 Prozentpunkte bei Recall@10 gg. Baseline.

- [ ] Zitier-/Provenance-Stabilitaet fuer Antwortkontext absichern (Target: Q4 2026)
  - Rationale: Produktions-RAG benoetigt reproduzierbare Quellenbindung je Antwort.
  - Umsetzungsschritte: (1) verpflichtende source-hint IDs in jedem Kontextblock, (2) deterministic ordering bei gleichwertigen Scores, (3) Negative-Tests fuer fehlende/duplizierte Quellen.
  - Abhaengigkeiten: `src/rag/rag_context_assembler.cpp`, `src/llm/wiki_index_store.cpp`, `tests/rag`.
  - Messbares DoD: Citation-Stabilitaet >=99.5% auf identischen Inputs ueber 100 Wiederholungen.

- [ ] Budget-aware Re-Ranking Trigger standardisieren (Target: Q1 2027)
  - Rationale: Re-Ranking verbessert Qualitaet, darf aber Latenz-/Kostenbudgets nicht verletzen.
  - Umsetzungsschritte: (1) Trigger nur bei Uncertainty/Intent-Features, (2) Hard-Caps fuer TTFT und total latency, (3) Fallback auf baseline fusion bei Budgetbruch.
  - Abhaengigkeiten: `src/query`, `src/llm`, `src/observability`.
  - Messbares DoD: +5% nDCG@10 bei <=10% p95-Latenzanstieg auf definierter Eval-Suite.
