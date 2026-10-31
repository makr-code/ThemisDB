---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; src/process/README.md; include/process/process_model_manager.h; include/process/process_orchestration_registry.h
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Modeling

**Purpose and audience:** Describe the process representations and lifecycle boundaries used by process-module developers and integrators. This page documents implementation concepts, not a new process notation.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Representations

### Process model records

`ProcessModelRecord` carries identity, version/revision, notation, domain, lifecycle state, descriptions, compliance tags, ownership metadata, original payload, normalized JSON, and an optional float embedding. The normalized graph is represented by nodes and edges. The manager serializes records as ThemisDB documents and maintains previous revisions as versioned snapshots when saving an existing record.

The model's lifecycle state (`DRAFT`, `ACTIVE`, `DEPRECATED`, `ARCHIVED`) belongs to the model record. Do not confuse it with the separate orchestration-profile state below.

### Runtime process graph and instance

`ProcessGraphRag` uses the execution graph manager for instance and token state, the model manager for process definitions, and the linker for attachments and missing-document checks. Its context contains the selected instance/query, process state, active/visited nodes, relevant subgraph, attached objects, similar cases, missing documents, compliance tags, scores, and assembled prompt.

The model graph is also convertible to knowledge-graph nodes and edges for retrieval. This is a retrieval representation; it does not replace the stored process definition or establish that a workflow engine is owned by this module.

### External orchestration profiles

`ProcessOrchestrationRegistry` persists profile records in RocksDB. A profile has an identifier, normalized domain, version/source metadata, revision, model payload, timestamps, `internal_rag_stage`, and one of three states:

- `STAGED`: stored but not selected as active for the domain.
- `ACTIVE`: selected by the domain's active-profile pointer.
- `ARCHIVED`: retired; an archived record cannot be activated until it is upserted back into a non-archived state.

The registry requires a usable ID and domain, rejects an explicitly false `internal_rag_stage`, and writes the persisted profile with that invariant set true. An omitted flag is accepted as true by the implementation. Activation demotes the prior active profile to staged, persists the new state, and updates a separate active-domain key. These are multiple writes; do not assume transactional or crash-atomic deployment semantics. `rollbackDomain` delegates to activation of the requested stored profile; it is not an undo transaction.

## Data flow and ownership

1. Import or save produces a validated process model record with a normalized graph.
2. The model manager stores the record and, when optional integrations are wired, updates text/vector search indexes.
3. Runtime instance state and model definition are read by graph retrieval; the linker supplies attachment/missing-document context.
4. Retrieval returns prompt/context data. Agentic retrieval may request follow-up contexts and merge newly returned documents.
5. Orchestration profiles use their own persisted state and active pointer; `predictOutcome` reads process summary/compliance/similar-case evidence and may recommend the current active profile for a domain.

## Constraints and non-goals

- Model lifecycle and orchestration profile lifecycle are distinct contracts with different states and persistence keys.
- External orchestration profiles do not move RAG into external control logic: the `internal_rag_stage=true` requirement is retained.
- Prediction fields are bounded arithmetic outputs with evidence recorded by the registry; the code does not establish model training, calibration, or decision authority.
- A model embedding is optional data. Its presence does not imply a built-in model, a particular vector dimension, or an automatically available vector index.
- Process context is assembled from persisted data. Missing instance/model/community data can yield empty, partial, or fallback results; consumers should inspect the returned status/context rather than infer completeness.

## Source references

- [`include/process/process_model_manager.h`](../../include/process/process_model_manager.h) · [`src/process/process_model_manager.cpp`](process_model_manager.cpp)
- [`include/process/process_graph_rag.h`](../../include/process/process_graph_rag.h) · [`src/process/process_graph_rag.cpp`](process_graph_rag.cpp)
- [`include/process/process_orchestration_registry.h`](../../include/process/process_orchestration_registry.h) · [`src/process/process_orchestration_registry.cpp`](process_orchestration_registry.cpp)
- [`include/process/process_agentic_rag.h`](../../include/process/process_agentic_rag.h) · [`src/process/process_agentic_rag.cpp`](process_agentic_rag.cpp)
- [Architecture](ARCHITECTURE.md) · [Module overview](README.md) · [Documentation governance](../../DOCUMENTATION_GOVERNANCE.md)
