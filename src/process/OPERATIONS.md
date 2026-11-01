---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; src/process/PRODUCTION_REQUIREMENTS.md; include/process/process_orchestration_registry.h; benchmarks/process/CMakeLists.txt; benchmarks/process/*.cpp
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Operations

**Purpose and audience:** Deployment and maintenance checklist for teams operating process-model data and orchestration profiles. This page describes operational ownership and verification, not a deployment command reference.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7368. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Before enabling process workloads

- Identify the owner of model imports, normalized model correctness, supported notation, domain, revision, and compliance metadata.
- Confirm process storage is available and the relevant model is readable through the model-manager interface.
- Validate the process definition before deployment; review parser and model constraints in [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).
- For retrieval, verify the instance references the intended model and the required linker/community data is present. Select LOCAL/GLOBAL/AUTO intentionally and confirm consumers inspect the effective mode.
- For external orchestration profiles, verify unique ID/domain, source path/format, revision expectations, and `internal_rag_stage=true`. Stage and inspect a profile before activation where the application workflow allows it.
- If embedding search is needed, explicitly configure the caller-supplied embedder and optional vector index; validate dimensions and index health. Do not assume a default embedding backend.
- Review the deployment's actual logging/metrics export. Process diagnostic and result structures are interfaces, not proof that a metric or alert is installed.

## Profile change control

1. Record the current domain and active profile ID before a change.
2. Upsert the intended JSON/YAML profile and inspect the persisted record and source metadata.
3. Confirm profile state and internal-RAG invariant before requesting activation.
4. After activation, read back the active profile and compare it with the intended ID/domain/revision.
5. If returning to an earlier profile, call the documented registry rollback operation with the known profile ID, then verify the active pointer and stored states.

Activation is implemented as separate writes to profile records and an active-domain pointer. It does not provide a transaction guarantee. Use a change window and recovery plan appropriate to the application; if partial state is observed, stop and inspect before attempting another transition.

## Routine verification

- Confirm model loads, expected revision, and model consistency through the module API.
- Confirm instance retrieval returns the expected instance/model identity and non-empty evidence where the use case requires it.
- For GLOBAL retrieval, check that reports exist for the associated process model; otherwise, the implementation falls back to LOCAL.
- For agentic retrieval, review `quality_satisfied`, `total_iterations`, iteration history, and elapsed time at the application boundary.
- Periodically test a controlled malformed input, missing-target scenario, and concurrent update in non-production test environments, using the relevant existing tests.

## Operational limits and known gaps

- The process module has explicit diagnostic types and result fields, but monitoring thresholds and platform-level observability must be verified in the consuming deployment.
- The light-retriever header describes configurable resource budgets and graceful degradation; its current `retrieve()` implementation does not invoke those checks. Treat resource-budget enforcement through that API as an unresolved implementation gap.
- `benchmarks/process/CMakeLists.txt` registers 12 targets with 88 source-level Google Benchmark registration macro declarations; these do not define a performance-gate total. The performance comparator currently finds zero Process expectation rows, and four PM-BM numeric targets are comments above in-process stubs. Separately, the release source-to-CMake-to-CTest coverage audit reports Process at 14/14 mappings; this checks registration, not benchmark execution. The latest weekly performance run failed during configure, and the tracked April 2026 mining JSON is not a current performance baseline or pass. See [Performance expectations](PERFORMANCE_EXPECTATIONS.md).
- Stale process links require explicit remediation; no cascading delete is performed.

## Ownership and escalation

Route model content/notation problems to the process-model owner; profile state problems to the registry/operator owner; relevance or incomplete context to the retrieval owner; suspected authorization, privacy, or exposure issues to the security response owner. For incident steps, see [RUNBOOK.md](RUNBOOK.md). Vulnerabilities should be reported through the project [`SECURITY.md`](../../SECURITY.md).

## References

- [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
- [Orchestration registry API](../../include/process/process_orchestration_registry.h) · [Model manager API](../../include/process/process_model_manager.h)
- [Light retrieval API](../../include/process/process_light_retriever.h) · [Diagnostics API](../../include/process/process_diagnostics.h)
- [Module roadmap](ROADMAP.md) · [Runbook](RUNBOOK.md) · [Documentation governance](../../DOCUMENTATION_GOVERNANCE.md)
