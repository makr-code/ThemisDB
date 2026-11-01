---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; src/process/README.md; include/process/process_graph_rag.h; include/process/process_light_retriever.h
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Research Notes

**Purpose and audience:** Evidence notes for developers evaluating retrieval and process-model techniques in `src/process`. This is not a claim that every cited technique has been validated for every workload.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Research questions and current implementation evidence

| Question | Implemented evidence | Evaluation boundary |
|---|---|---|
| How should a process instance supply context for a query? | `ProcessGraphRag` builds context from instance state, active nodes, model subgraph, attachments, missing documents, compliance information and similar cases. | Validate relevance and completeness against a labeled set of representative process queries; the code and module benchmark notes do not supply a labeled quality study. |
| How should retrieval address local and process-wide questions? | `ProcessLightRetriever` exposes `LOCAL`, `GLOBAL` and keyword-routed `AUTO`. `GLOBAL` selects up to three persisted community reports, ordered by modularity score; if it cannot use communities it falls back to local retrieval. | Measure answer-context relevance and fallback frequency separately by mode. The AUTO route is a keyword heuristic, not a learned classifier. |
| When is iterative retrieval useful? | `ProcessAgenticRag` starts with process graph retrieval, then delegates iterative evaluation and follow-up retrieval to generic `AgenticRAG`, subject to configured iteration/document/quality settings. | Compare with a one-pass baseline on the same queries and record quality, faithfulness, iteration count and elapsed time. The adapter does not establish that the generated answer is factually correct. |
| What similarity evidence is available? | Process-model records can carry float vectors; an injected embedder and optional vector index are supported. Similar-instance lookup reads stored instance vectors when available and falls back to variable-key Jaccard matching if it finds no embedding-based candidates. | Keep model-embedding search and instance-case matching as separate experiments. Their inputs and fallback behavior differ. |

The headers cite HippoRAG for PPR and LightRAG for the LOCAL/GLOBAL retrieval pattern. These references describe design context, not independent proof of ThemisDB process-module quality. In the process graph API, PPR is configurable and bounded by iteration/convergence parameters; the light-retrieval API's LOCAL/GLOBAL choice and AUTO heuristic are separate controls.

## Data basis, assumptions and limits

- Process models are normalized into JSON node/edge structures. Instance state, model metadata, process links, and any persisted vectors are separate inputs to retrieval.
- Similar-case and compliance output depends on available persisted data and the behavior of the supporting process graph, linker, and retrieval components.
- The `ProcessOrchestrationRegistry` prediction scores are deterministic calculations from summary/compliance/similar-case evidence in its implementation. They are not documented as trained or calibrated predictive models.
- LLM output and query interpretation are not deterministic guarantees. Treat any generated description, prompt response, or recommendation as a result requiring application-level review.
- Resource settings documented in retrieval headers are not sufficient evidence that all settings are enforced on every path. In particular, see [implementation notes](IMPLEMENTATION.md) for the discrepancy between `ProcessLightRetriever`'s declared budgets and its current `retrieve()` implementation.

## Reproducible evaluation plan

1. Select representative models and instance states, and preserve the exact model/query/instance fixtures.
2. Compare LOCAL, GLOBAL, and AUTO on the same specific-entity and process-wide questions; include a no-community fixture to exercise fallback.
3. For agentic retrieval, record the configured thresholds, iterations, document counts, quality/faithfulness scores, elapsed time, and whether the quality threshold was met.
4. Evaluate model-vector similarity only with a known embedder, compatible vector dimensions, and an explicitly configured vector index; record which fallback path was used.
5. Report sample size, fixture provenance, build/configuration, percentile method, and raw result summary. Do not equate a configured target or a passing unit test with a user-facing quality result.

The repository's [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) distinguishes benchmark registrations from four numeric PM-BM comments, comparator-enforced expectations, and the separate source-to-CMake-to-CTest coverage audit. The comparator discovers zero Process expectation rows. Coverage reports show 14/14 Process mappings, but that is not runtime performance. The tracked April 2026 mining output is historical, and the latest visible weekly performance run did not reach benchmark execution; neither establishes a current baseline, performance pass, or retrieval-quality result.

## Validation references

- [`include/process/process_graph_rag.h`](../../include/process/process_graph_rag.h) — retrieval inputs, context, PPR and similar-case API.
- [`include/process/process_light_retriever.h`](../../include/process/process_light_retriever.h) — mode selection and declared resource-limit API.
- [`include/process/process_agentic_rag.h`](../../include/process/process_agentic_rag.h) — iterative retrieval configuration and result.
- [`include/process/process_model_manager.h`](../../include/process/process_model_manager.h) — optional embedder/vector-index integration.
- [`tests/process/test_process_light_retriever.cpp`](../../tests/process/test_process_light_retriever.cpp) and [`tests/process/test_process_orchestration_registry.cpp`](../../tests/process/test_process_orchestration_registry.cpp) — behavior fixtures.
- [Module overview](README.md) · [Research/evaluation targets](PERFORMANCE_EXPECTATIONS.md) · [Repository documentation governance](../../DOCUMENTATION_GOVERNANCE.md)
