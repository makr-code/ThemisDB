---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-10
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; src/process/README.md; include/process/process_light_retriever.h; src/process/process_light_retriever.cpp
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Implementation Notes

**Purpose and audience:** Practical implementation map for developers changing process lifecycle, retrieval, descriptors, and external orchestration profiles.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Component map

| Concern | Implementation and contract |
|---|---|
| Process model CRUD/import/export | [`process_model_manager.cpp`](process_model_manager.cpp), public API in [`process_model_manager.h`](../../include/process/process_model_manager.h). Normalized records are validated before save; optional embedder/full-text/vector-index integrations are caller-wired. |
| Graph retrieval/context | [`process_graph_rag.cpp`](process_graph_rag.cpp), API/config in [`process_graph_rag.h`](../../include/process/process_graph_rag.h). Reads instance state, model graph, linker context and optional similar-case evidence. |
| LOCAL/GLOBAL/AUTO façade | [`process_light_retriever.cpp`](process_light_retriever.cpp) and [`process_light_retriever.h`](../../include/process/process_light_retriever.h). GLOBAL uses stored community reports when available; without them it warns and delegates to LOCAL. |
| Iterative retrieval | [`process_agentic_rag.cpp`](process_agentic_rag.cpp) and [`process_agentic_rag.h`](../../include/process/process_agentic_rag.h). Starts with `ProcessGraphRag`, passes context to generic `AgenticRAG`, filters previously seen document IDs, and returns quality/iteration metadata. One `ProcessAgenticRag` object is not to be shared across threads. |
| LLM process descriptor | [`llm_process_descriptor.cpp`](llm_process_descriptor.cpp) and [`llm_process_descriptor.h`](../../include/process/llm_process_descriptor.h). Converts stored model data into JSON and prompt text; optional embedding output is pass-through. |
| External orchestration profile registry | [`process_orchestration_registry.cpp`](process_orchestration_registry.cpp) and [`process_orchestration_registry.h`](../../include/process/process_orchestration_registry.h). Persists profile/revision records and a separate active-domain pointer in RocksDB. |
| Incident contracts | [`process_diagnostics.h`](../../include/process/process_diagnostics.h) and [`process_api_contract.h`](../../include/process/process_api_contract.h). Use explicit incident/error context rather than silently treating missing or malformed data as success. |

## Change invariants

- Keep the normalized model graph and public import/export contract aligned; validate node references and model constraints before persistence.
- Keep orchestration profile state separate from model lifecycle state. Preserve the requirement that persisted external profiles have `internal_rag_stage=true`.
- Do not describe profile activation or rollback as transactional: activation persists profile state and active pointer through separate database writes, while `rollbackDomain()` calls `activateProfile()`.
- Preserve explicit retrieval mode in returned results. GLOBAL can resolve to LOCAL when there is no usable community report.
- `ProcessLightRetriever` measures elapsed time with a monotonic clock and reports the returned context's byte size. It caps context bytes on a UTF-8 boundary, caps GLOBAL reports and LOCAL prompt lines, and clamps LOCAL traversal depth. Timeout is only detected after synchronous dependencies return; it does not cancel them or guarantee a hard deadline.
- Keep optional embedding generation and indexing optional. The model manager uses a caller-supplied embedder/index; do not add backend assumptions to the descriptor contract.

## Validation path

Use the closest existing tests for the changed seam:

- Registry lifecycle and `internal_rag_stage`: [`test_process_orchestration_registry.cpp`](../../tests/process/test_process_orchestration_registry.cpp).
- Retrieval mode/fallback: [`test_process_light_retriever.cpp`](../../tests/process/test_process_light_retriever.cpp).
- Retrieval budgets: the same production-class test covers GLOBAL report/byte caps, UTF-8-safe truncation, LOCAL depth/line caps, metrics, and best-effort timeout reporting.
- Retrieval edge scenarios: [`test_process_retriever_edge_focused.cpp`](../../tests/process/test_process_retriever_edge_focused.cpp). These tests use mock behavior for some resource cases and are not proof that `ProcessLightRetriever::retrieve()` enforces those budgets.
- Model concurrency/determinism/diagnostics: [`test_process_concurrency_churn_focused.cpp`](../../tests/process/test_process_concurrency_churn_focused.cpp), [`test_process_determinism_conflict_focused.cpp`](../../tests/process/test_process_determinism_conflict_focused.cpp), and [`test_process_diagnostics_incident_focused.cpp`](../../tests/process/test_process_diagnostics_incident_focused.cpp).

Repository build/test configuration lives alongside the module in [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt). Select the configured target/test name for the local build rather than assuming a standalone executable name.

## Related developer docs

[Module guide](MODULE_GUIDE.md) · [API reference](API.md) · [Modeling](MODELING.md) · [Runbook](RUNBOOK.md) · [Architecture](ARCHITECTURE.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md)
