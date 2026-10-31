---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: architecture-governance
Canonical_Sources: include/process/*.h; src/process/*.cpp; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; src/process/API.md; src/process/IMPLEMENTATION.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Architecture

**Purpose:** Source-checked orientation to the module's current components. This is a summary, not an exhaustive component inventory or a deployment-readiness statement.

**AI provenance:** Exact model backend not verifiable from final document text alone.

## Boundaries and components

The process module includes process-model storage/import/export, runtime graph-context retrieval, process-object linking, format-specific serializers/importers, analytics helpers, and integration contracts. `ProcessModelManager::deployToEngine()` bridges stored models to `ProcessGraphManager`; this is an integration surface, not a claim that this module owns a general workflow orchestration service.

The public declarations in [`include/process/`](../../include/process/) and their implementations under this directory define the supported surfaces. The files below are examples of current components, not a finite inventory:

| Concern | Source evidence |
|---|---|
| Model lifecycle and optional search integrations | [`process_model_manager.h`](../../include/process/process_model_manager.h), `process_model_manager.cpp` |
| Graph and iterative retrieval | [`process_graph_rag.h`](../../include/process/process_graph_rag.h), [`process_agentic_rag.h`](../../include/process/process_agentic_rag.h), [`process_light_retriever.h`](../../include/process/process_light_retriever.h) |
| External orchestration profiles | [`process_orchestration_registry.h`](../../include/process/process_orchestration_registry.h), `process_orchestration_registry.cpp` |
| Diagnostics and audit/telemetry contracts | [`process_diagnostics.h`](../../include/process/process_diagnostics.h), [`process_diagnostics_api.h`](../../include/process/process_diagnostics_api.h), [`process_audit_logger.h`](../../include/process/process_audit_logger.h), [`process_telemetry_contract.h`](../../include/process/process_telemetry_contract.h) |
| Federation and conflict-resolution surfaces | `federation_consensus_manager.cpp`, `federation_replica_manager.cpp`, `process_conflict_resolver.cpp`; corresponding declarations under `include/process/` |

## Data flow and limits

1. Import or caller-provided records are normalized/validated and stored by `ProcessModelManager`; optional embedder and index callbacks are caller-wired.
2. Runtime graph retrieval combines instance/model data and, where requested and available, link and similarity evidence.
3. `ProcessLightRetriever` selects LOCAL/GLOBAL/AUTO; GLOBAL can fall back to LOCAL when community reports are unavailable. Its declared resource limits are not enforced by the current `retrieve()` path; see [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
4. `ProcessOrchestrationRegistry` stores profile revisions and an active-domain pointer separately. Activation/rollback comprise multiple writes, with no transactional guarantee stated by the API.

The audit and federation components have source implementations, but source presence does not establish production readiness. In particular, audit storage/patch operations and telemetry export contain simplified behavior, and federation sources identify themselves as alpha/core implementation. Consult the current code and tests before relying on advertised design targets.

## Failure paths

- Invalid or malformed process data is handled through operation results and diagnostics; exact failure behavior is operation-specific.
- GLOBAL light retrieval may fall back to LOCAL when community reports are unavailable. Callers should inspect the effective mode.
- Profile activation and rollback span separate persistence writes. A failed operation may require inspection of the stored profile and active-domain pointer; there is no transaction guarantee.
- Resource limits declared by `ProcessLightRetriever` are not enforced by the current `retrieve()` path.

## Non-goals

- The process module does not own a general workflow execution service.
- Source or contract presence does not establish production maturity, external service configuration, or successful release evidence.

## Build and validation ownership

- Tests are discovered/registered by [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt), which globs `test_*.cpp`; there is no `src/process/CMakeLists.txt`.
- Process benchmarks are conditionally registered by [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt) when `THEMIS_BUILD_BENCHMARKS` is enabled. The registered set is not identical to every `bench_*.cpp` file in the directory.
- Neither registration is evidence that a target was built or passed. See the [API guide](API.md), [implementation notes](IMPLEMENTATION.md), and current registration files for validation scope.

## Related documentation

[Module overview](README.md) · [API guide](API.md) · [Implementation notes](IMPLEMENTATION.md) · [Governance](../../DOCUMENTATION_GOVERNANCE.md)
