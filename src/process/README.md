---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: include/process/*.h; src/process/*.cpp; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; src/process/API.md; src/process/IMPLEMENTATION.md
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI documentation assistant
Human_Review: false
Human_Author: unknown
---

# ThemisDB Process Module

**Purpose:** Source-based overview of the process-modeling module. Public declarations live under [`include/process/`](../../include/process/); implementations are in this directory.

**AI provenance:** Exact model backend not verifiable from final document text alone.

## Scope and boundaries

The module contains process-model record lifecycle and format import/export, graph-based process context retrieval, process-object/process-process linking, process-related serializers/importers, analytics helpers, and integration surfaces for external orchestration profiles. `ProcessModelManager::deployToEngine()` bridges model records to `ProcessGraphManager`; this is not evidence that the process module owns a general workflow orchestration service.

The source tree also includes federation, audit, diagnostics, and telemetry components. Their existence is not a maturity or deployment guarantee. Some sources explicitly describe alpha/core or simplified behavior; verify each implementation and its tests before relying on design-contract claims.

## Key documented behavior and caveats

- `ProcessLightRetriever` supports LOCAL/GLOBAL/AUTO selection; AUTO is keyword-based and GLOBAL falls back to LOCAL if no persisted communities are usable. The resource budget declarations are not enforced by the current `retrieve()` implementation.
- `ProcessOrchestrationRegistry` requires a profile ID/domain and enforces `internal_rag_stage=true`; activation and rollback use separate writes and do not promise a transaction.
- Model embedding generation/indexing is optional and caller-configured. `LlmProcessDescriptor` can pass through an existing vector but does not generate it.
- Diagnostics define nine incident types. Metrics collector interfaces do not imply that a consuming deployment exports metrics or alerts.
- Do not infer persistence, cryptographic integrity, OpenTelemetry transport, calibrated prediction, or release approval merely from an API or design contract.

## Build and verification map

- Tests are declared from `test_*.cpp` in [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt). There is no `src/process/CMakeLists.txt`.
- Benchmarks are conditionally registered in [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt) when `THEMIS_BUILD_BENCHMARKS` is enabled. Not every benchmark source in that directory is registered there.
- Registration is not evidence of a successful build, test run, benchmark result, or production status.

## Developer documentation

- [API guide](API.md) · [Implementation notes](IMPLEMENTATION.md) · [Developer guide](MODULE_GUIDE.md)
- [Architecture](ARCHITECTURE.md) · [Modeling](MODELING.md) · [Embedding integration](EMBEDDING.md)
- [Operations](OPERATIONS.md) · [Runbook](RUNBOOK.md) · [Trust boundaries](TRUST.md)
- [Roadmap](ROADMAP.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md)
- [Documentation governance](../../DOCUMENTATION_GOVERNANCE.md)
