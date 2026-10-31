---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; include/process/process_diagnostics.h; src/process/process_orchestration_registry.cpp
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Modeling Runtime Runbook

**Purpose and audience:** Triage guidance for process-model storage, process retrieval, linking, and orchestration-profile issues. Scope is the process modeling runtime; it is not a runbook for OS subprocess spawning or signal handling.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

For OS process-manager incidents, use [`docs/operability/RUNBOOK_PROCESS_MANAGER.md`](../../docs/operability/RUNBOOK_PROCESS_MANAGER.md); that guide addresses subprocess lifecycle/resource incidents, not all incidents covered here.

## Triage principles

1. Preserve the diagnostic record and request/instance/model/profile identifiers. `DiagnosticRecord` exposes incident type, error code, operation, input identifier, timestamp, and actionable message; `DiagnosticContext` can carry resource/limit details.
2. Classify the symptom using the process diagnostic type; do not invent a metric, alert, or log pattern when the deployment has not configured one.
3. Check source state through the owning interface before changing persisted state. Prefer read-only inspection first.
4. Preserve the original input and relevant revision/profile identifiers before retry or manual repair.
5. Escalate when the source of truth is inconsistent or the required recovery would rely on unsupported transactional/automatic rollback behavior.

## Incident paths

### Import, malformed input, or validation failure

- **Evidence:** `IMPORT_INCIDENT`, `MALFORMED_INPUT_INCIDENT`, or `VALIDATION_INCIDENT`; inspect operation, input identifier, error code and actionable message.
- **Diagnosis:** Confirm the source format and model identity; inspect parser/validation failure details and the referenced model constraints. Determine whether the input is malformed, unsupported, or exceeds a documented parser/model bound.
- **Recovery:** Correct or reject the source input, then retry import through the normal validated path. Do not mark an invalid model active to bypass validation.
- **Escalate:** If a valid, unchanged fixture reproduces the failure or persisted state is inconsistent.

### Resource or retrieval failure

- **Evidence:** `RESOURCE_INCIDENT` or `RETRIEVAL_INCIDENT`, with any recorded limit name/value and actual value. Empty context alone does not distinguish missing data from a successful empty result.
- **Diagnosis:** Check that the instance and process definition exist, then inspect active-node/model/link inputs and the requested retrieval mode. A GLOBAL request may fall back to LOCAL when no community reports are available; check the returned effective mode.
- **Recovery:** Reduce or correct an input only when the diagnostic identifies the exceeded limit or malformed data. Retry after confirming the source record. If the operation returned a partial/degraded result, do not represent it as complete.
- **Caution:** Although the light-retriever header declares time/size/depth/result budgets and degraded results, its current `retrieve()` implementation does not call those enforcement helpers. Do not rely on that façade to stop or truncate work until this discrepancy is resolved.

### Missing target, stale link, or linking inconsistency

- **Evidence:** `MISSING_TARGET_INCIDENT`, `RETRIEVAL_INCIDENT`, `LINKING_INCIDENT`, or `CYCLE_INCIDENT`.
- **Diagnosis:** Identify the link and referenced source/target records; confirm whether the target was deleted, moved, or never persisted. The module documents read-time stale-link detection and does not cascade-delete dependent links.
- **Recovery:** Decide with the data owner whether to restore the missing target or explicitly remove/repair the stale link. Validate affected references after the manual change; do not assume automatic cleanup.
- **Escalate:** If the reference graph is ambiguous or repair could discard business data.

### Concurrent model update

- **Evidence:** `CONCURRENCY_INCIDENT`; capture model ID, operation, current revision and conflicting operation context if supplied.
- **Diagnosis:** Reload the latest model revision and compare it with the attempted update.
- **Recovery:** Merge changes deliberately and retry against the current revision, or abort. The module does not promise automatic rollback or nested transactions; do not blindly replay a write.

### Orchestration profile activation/rollback

- **Evidence:** Failed `ProcessModelResult`, missing/mismatched profile/domain, or unexpected active-profile lookup.
- **Diagnosis:** Use registry reads (`getProfile`, `getActiveProfile`, `listProfiles`) to compare profile ID, domain, state, revision, and `internal_rag_stage`. An explicitly false RAG-stage flag is rejected.
- **Recovery:** For an approved prior profile, request `rollbackDomain(domain, profile_id)` and then read back the active profile and relevant profile states. This method delegates to activation; it is not transactional rollback.
- **Safety:** Activation persists the previous profile's demotion, the requested profile's activation, and the active pointer separately. If a failure occurs mid-transition, inspect all three pieces of state before attempting another activation. Do not claim atomicity or automatic recovery.

## Observability and escalation

The process diagnostics API defines incident categories and structured context, and the light retrieval result exposes effective mode/degraded/reason fields. These interfaces do not by themselves establish that a deployment exports them as metrics, alert rules, or dashboards. Confirm actual deployment instrumentation before setting alert thresholds.

Escalate code defects with the smallest reproducible model/instance fixture, the diagnostic record, relevant revision/state reads, and the focused test that reproduces the behavior. Consult [`SECURITY.md`](SECURITY.md) for security incidents and the project-level [`SECURITY.md`](../../SECURITY.md) for vulnerability reporting.

## References

- [`process_diagnostics.h`](../../include/process/process_diagnostics.h) · [`process_api_contract.h`](../../include/process/process_api_contract.h)
- [`process_light_retriever.cpp`](process_light_retriever.cpp) · [`process_orchestration_registry.cpp`](process_orchestration_registry.cpp)
- [`process_model_manager.cpp`](process_model_manager.cpp) · [`process_linker.h`](../../include/process/process_linker.h)
- [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Operations overview](OPERATIONS.md) · [OS subprocess runbook](../../docs/operability/RUNBOOK_PROCESS_MANAGER.md)
