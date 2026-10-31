---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: api-contract
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; include/process/process_api_contract.h; include/process/process_diagnostics.h; include/process/process_orchestration_registry.h
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module API Guide

**Purpose and audience:** Contract-oriented map for developers integrating process-model storage, retrieval, descriptors, and orchestration profiles. Public declarations in `include/process/` remain authoritative.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: api-contract; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Main interfaces

| Interface | Inputs and outputs | Contract notes |
|---|---|---|
| `ProcessModelManager` | Import/save/load/list/search/remove/export model records; validate and deploy/undeploy models. | `ProcessModelResult` carries `ok`, message, and model ID. `findSimilar()` receives a query vector; embedder and vector index integrations are optional. See [`process_model_manager.h`](../../include/process/process_model_manager.h). |
| `ProcessGraphRag` | `retrieve(instance_id, query, config)` or `retrieveForNode(...)` returns `ProcessRagContext`. | Context may include instance/model state, subgraph, attachments, similar cases, missing documents, compliance tags, and prompt. See [`process_graph_rag.h`](../../include/process/process_graph_rag.h). |
| `ProcessLightRetriever` | `retrieve(query, instance_id, mode, config)` returns `LightRetrievalResult`. | `AUTO` uses case-insensitive keyword routing. GLOBAL uses persisted reports when available and otherwise falls back to LOCAL. Check the `used_mode` and `degraded` fields; declared resource budget fields are not currently enforced in the implementation path. See [`process_light_retriever.h`](../../include/process/process_light_retriever.h). |
| `ProcessAgenticRag` | `iterativeQuery(...)` or `iterativeQueryForNode(...)` returns `ProcessAgenticResult`. | Result includes final context/prompt, quality flag, iteration count/history, and elapsed time. The configured maximum iterations/documents and thresholds bound the generic agentic loop; create a per-request instance rather than sharing one between threads. See [`process_agentic_rag.h`](../../include/process/process_agentic_rag.h). |
| `LlmProcessDescriptor` | Static conversion from a `ProcessModelRecord` to JSON, prompt text, list summaries, or conformance prompt. | `include_embedding` only exposes an existing vector when non-empty. It does not invoke an embedder. See [`llm_process_descriptor.h`](../../include/process/llm_process_descriptor.h). |
| `ProcessOrchestrationRegistry` | Upsert file/object, optional catalog sync, activate/archive/rollback, read/list profiles, or predict outcome. | External profile data requires an ID/domain and rejects an explicitly false internal-RAG flag. Activation and rollback involve multiple writes and have no transactional guarantee in this API. See [`process_orchestration_registry.h`](../../include/process/process_orchestration_registry.h). |

## Inputs, outputs and edge cases

- Missing model/instance records can result in failure results, empty retrieval material, or explicit retrieval diagnostics depending on the operation. Do not interpret an empty context as a verified successful answer.
- `AUTO` mode is a fixed keyword heuristic, not semantic intent classification. An explicit LOCAL or GLOBAL mode bypasses that routing choice.
- A GLOBAL request with no persisted communities falls back to LOCAL; inspect `used_mode` if the caller needs to report the effective path.
- Agentic retrieval can finish with `quality_satisfied == false`; callers must handle that state and decide whether to answer, request more input, or escalate.
- Profile `rollbackDomain(domain, profile_id)` invokes activation for that stored profile. It does not restore a database snapshot or undo already observed external effects.
- API enums, error codes, diagnostics and threading declarations are in the headers. Consult those definitions rather than duplicating enum values in application code.

## Validation and integration

Use [`tests/process/test_process_orchestration_registry.cpp`](../../tests/process/test_process_orchestration_registry.cpp) for profile lifecycle and invariant cases, [`tests/process/test_process_light_retriever.cpp`](../../tests/process/test_process_light_retriever.cpp) for routing/fallback, and the model manager tests under [`tests/process/`](../../tests/process/). The module's [implementation notes](IMPLEMENTATION.md) record known header/implementation mismatches.

## Canonical references

- [`process_api_contract.h`](../../include/process/process_api_contract.h) · [`process_diagnostics.h`](../../include/process/process_diagnostics.h)
- [`process_model_manager.h`](../../include/process/process_model_manager.h) · [`process_orchestration_registry.h`](../../include/process/process_orchestration_registry.h)
- [`process_graph_rag.h`](../../include/process/process_graph_rag.h) · [`process_agentic_rag.h`](../../include/process/process_agentic_rag.h) · [`process_light_retriever.h`](../../include/process/process_light_retriever.h)
- [Architecture](ARCHITECTURE.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Module overview](README.md)
