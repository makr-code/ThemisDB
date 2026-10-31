---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; src/process/README.md; src/process/ARCHITECTURE.md
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Developer Guide

**Purpose and audience:** Onboarding route for developers making a focused change to process modeling, retrieval, orchestration, or supporting contracts.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Start with the boundary

The process module owns process-model records, import/export support, process graph context assembly, process linking/compliance support, and its external orchestration-profile registry. It does not claim ownership of a general workflow execution service outside its integration boundary. The overview in [`README.md`](README.md) and component map in [`ARCHITECTURE.md`](ARCHITECTURE.md) summarize these responsibilities.

Public contracts are under [`include/process/`](../../include/process/); implementation is under this `src/process/` directory; module tests are under [`tests/process/`](../../tests/process/). Keep a change within the narrowest owner path and review the public header whenever behavior or return semantics change.

## Find the right subsystem

1. **Import, validation, persistence, versioned snapshots, or export:** start with `ProcessModelManager` and its serializers.
2. **Runtime graph context or similar cases:** start with `ProcessGraphRag`; check which instance/model/link data the query requires.
3. **LOCAL/GLOBAL/AUTO selection:** start with `ProcessLightRetriever`; verify actual behavior in its implementation as well as header comments.
4. **Iterative question answering:** start with `ProcessAgenticRag`; review per-request thread-safety and quality/iteration results.
5. **External YAML/JSON orchestration profiles:** start with `ProcessOrchestrationRegistry`; preserve staged/active/archived state and the internal-RAG invariant.
6. **Descriptor or vector search changes:** review [`EMBEDDING.md`](EMBEDDING.md); embedding generation depends on an injected callback, not a default model backend.

## Work and validation sequence

1. Read the module overview, architecture, and relevant public header before implementation details.
2. Trace the concrete implementation and existing tests for the behavior being changed.
3. Record invariants, missing-data behavior, error/incident classification, and whether state updates are single-step or multi-step.
4. Make a narrow change and add/adjust tests at the closest existing seam.
5. Verify the focused configured test target and review related performance/security requirements when those surfaces are affected.
6. Update the canonical module page for a behavior/contract change. Do not treat generated wiki context or historical snapshots as current implementation evidence.

The root `tests/process/CMakeLists.txt` defines available test targets. Use the repository's configured build/test workflow to discover and run the relevant target; this guide intentionally does not prescribe an unverified executable or runtime command.

## Important implementation caveats

- `ProcessLightRetriever` declares retrieval-budget fields and degradation helpers, but its current `retrieve()` does not invoke them. See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) before relying on those limits.
- Profile activation updates profile records and an active-domain key through multiple writes. There is no transactional activation/rollback guarantee.
- Model vectors and instance embeddings are optional inputs with distinct storage/search paths. Do not infer a shared index or built-in embedder.
- `ProcessAgenticRag` instances are documented as not thread-shareable. Construct one per concurrent request.

## Reading map

- [API guide](API.md) · [Implementation notes](IMPLEMENTATION.md) · [Modeling](MODELING.md)
- [Research notes](RESEARCH.md) · [Embedding integration](EMBEDDING.md)
- [Operations](OPERATIONS.md) · [Runtime runbook](RUNBOOK.md) · [Trust boundaries](TRUST.md)
- [Roadmap](ROADMAP.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
- [Repository module-documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md) · [Quick rules](../../.github/copilot/module-quick-rules.md)
