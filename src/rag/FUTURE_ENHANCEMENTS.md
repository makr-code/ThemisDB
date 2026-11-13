---
Author: "ThemisDB Team"
Created: "2026-02-10"
Last Updated: "2026-11-13"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

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

## Small-Model Context and Framework Interoperability (Proposal)

### Scope

Prioritize evidence selection for smaller local models and decentralized
native inference. Then expose the same verified database contracts through
optional Python and .NET framework adapters. This proposal changes no existing
API and claims neither tokenizer-exact current packing nor current framework
compatibility. See [the context design](ARCHITECTURE.md).

### Design Constraints

- Reuse existing QueryEngine/search/RAG components and supported client
  transports; reconcile server/SDK semantics before wrapping them.
- Keep framework-specific packages optional and separately versioned from the
  server and core Python client.
- Preserve document IDs, metadata, filters, tenant boundaries, and explicit
  error semantics through adapter round trips.
- Keep bounded native re-retrieval/tool planning available in the hybrid path;
  require cumulative step/token/time/candidate limits. Optional durable
  workflow state requires a separate process/storage contract.
- Budget the fully rendered input with the generation model's tokenizer,
  reserving output and a safety margin within its effective runtime window.
  Current shared preparation uses character estimates, not exact counts.
- Prefer attributable, diverse passages over redundant top-k chunks. Any
  derived compression retains source links and cannot replace original quotes.
- Apply trusted access/locality policy at data sources and caches, not merely
  a caller-supplied metadata filter; no automatic cloud escalation.

### Required Interfaces

- A model/tokenizer-aware final context-admission contract covering system,
  templates, tools/history, query, evidence framing and reserved output.
- An evidence envelope with stable namespace/source/chunk identity, source
  revision, provenance, score metric/direction, and declared freshness/coverage.
  New metadata is proposed, not already present in `RAGCandidate`.
- Optional LangChain VectorStore/Retriever and Microsoft .NET
  `Microsoft.Extensions.VectorData` adapters with documented CRUD, filters,
  score direction, cancellation and paging semantics.
- A document conversion contract that preserves stable IDs and JSON metadata.
- A conversation-history adapter with explicit transaction and retention
  behavior where supported by the client API.
- A compatibility matrix linking package versions to supported ThemisDB API
  versions.

### Implementation Notes

- **P0 source observations (2026-11-13; static inspection, not runtime tests):**
  [`clients/python/themis/__init__.py`](../../clients/python/themis/__init__.py)
  sends `filter` in `vector_search()`, but the inspected
  [`VectorApiHandler::handleSearch`](../server/vector_api_handler.cpp) calls
  unfiltered `searchKnn()` without interpreting it. The Python multi-endpoint
  merge also sorts distance-only hits descending and skips non-200 responses.
  Reconcile these paths before claiming filtered, metric-correct or
  failure-transparent adapter support; do not infer a database-wide absence
  of filtering from this endpoint.
- Verify synchronous/asynchronous Python and .NET request/response parity,
  namespace/ID mapping and cursor semantics; API exposure is not evidence that
  predicates or distributed pagination work end to end.
- **P1 native context:** integrate backend tokenization with final prompt
  packing, remove redundant passages, retain complementary evidence and source
  revisions, and bound any refinement across the entire session.
- **P1 locality/lifecycle:** coordinate aggregate shard budgets and explicit
  partial results with search; coordinate idempotent updates, model/index
  manifests, deletion and revocation visibility with ingestion/CDC owners.
- **P1 specialization/tensor integration:** coordinate request-pinned
  Multi-LoRA selection with trained-artifact promotion; use tensor summaries
  for bounded candidate/adapter routing only under declared approximation and
  compatibility contracts. Retain original evidence for attribution.
- Start adapters with LangChain and .NET VectorData only after the above
  contracts are qualified; evaluate LlamaIndex/Haystack according to demand
  and maintainer capacity. Do not copy their orchestration engines.
- Keep adapter packaging and dependencies outside core builds; document
  installation, authentication, TLS, and supported server versions.
- Target: Q2 2027; provisional planning target, not a release commitment.

### Test Strategy

- Run integration tests against a supported ThemisDB endpoint for upsert,
  delete, filtered retrieval, score conversion, and metadata/ID round trips.
- Verify error propagation for unavailable endpoints, authorization denial,
  malformed metadata, timeout, and unsupported filters.
- Cover tokenizer/model/template changes, multilingual/code input, output
  reservation, no-fitting evidence, duplicates, multi-part questions, and
  explicit stop outcomes for no new evidence/budget exhaustion.
- Compare filtered ANN results with an exact oracle at 100%, 10%, 1% and 0.1%
  selectivity; separately check hard authorization restrictions and denied
  documents in caches. Numeric quality gates require corpus/owner approval.
- Exercise source updates/deletes/revocations, missing shards, local-model
  outages and denied egress. Preserve explicit partial coverage/freshness.
- Pin the framework/client version matrix and require all declared supported
  combinations to pass applicable upstream standard tests plus ThemisDB
  integration cases before release.

### Performance Targets

- Evaluate task quality at fixed supported context sizes with pinned local
  models, quantizations and corpora: evidence precision/support coverage,
  attribution and abstention, recall/nDCG, duplicate/input-token use, prefill,
  end-to-end p95/p99, RAM/VRAM and cross-node bytes.
- Compare native direct vs adapted operations and concurrent database load on
  the same hardware/topology. The earlier 5% conversion-overhead figure was a
  proposal, not measured evidence; approve gates only after baselining.
- Ablate base/RAG/adapter/RAG+adapter/tensor-enabled execution with identical
  pinned evaluation sets and documented budgets; separate learned task gains
  from evidence quality and actual prompt-token reductions.

### Security / Reliability

- Enforce tenant and authorization boundaries through the existing API; an
  adapter must not broaden access or bypass server policy.
- Bound batch sizes and client retries; preserve idempotency and report partial
  writes rather than returning success-shaped fallbacks.
- Do not log prompts, retrieved content, credentials, or personal data.
- Retrieval quality is not an authorization decision, and model self-evaluation
  is not ground truth. Report empty evidence, partial backend failure and
  exhausted budget as distinct outcomes; do not silently generate an
  ungrounded answer after internal retrieval fails.

### Official Research Basis (reviewed 2026-11-13)

- [LangChain vector stores](https://docs.langchain.com/oss/python/integrations/vectorstores)
  and [standard tests](https://docs.langchain.com/oss/python/contributing/standard-tests-langchain):
  declare actual integration capabilities and pin the conformance version.
- [Microsoft vector integrations](https://learn.microsoft.com/en-us/agent-framework/integrations/by-component/vector-stores/):
  .NET uses standalone VectorData abstractions; Python interfaces are experimental.
- [LlamaIndex ingestion](https://developers.llamaindex.ai/python/framework/module_guides/loading/ingestion_pipeline/)
  and [Haystack DocumentStore](https://docs.haystack.deepset.ai/docs/document-store):
  stable document identity, update/delete and duplicate-policy semantics.
- [Azure vector filters](https://learn.microsoft.com/en-us/azure/search/vector-search-filters):
  pre/post-filter strategy affects recall and resource cost.
  None of these sources proves current ThemisDB runtime behavior.
