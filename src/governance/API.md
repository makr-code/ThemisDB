---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Governance Public API

## Purpose and contract

This is the module-level guide to the C++ interfaces under `include/governance/`. The public headers are the API source of truth; this page summarizes caller-visible behavior and important limits. Include `governance/<header>.h`; do not include implementation files from `src/governance/`.

## Main interfaces

| Interface | Header | Inputs and outputs | Caller responsibility / edge case |
|---|---|---|---|
| `PolicyEngine` | [`policy_engine.h`](../../include/governance/policy_engine.h) | Loads YAML; evaluates headers plus route into `PolicyDecision`; offers query/inference checks and dry-run simulation. | Apply returned export/cache/ANN/redaction restrictions. Unknown/missing classification is strict in native evaluation. Simulation is dry-run and does not write an audit entry. |
| `PolicyManager` | [`policy_manager.h`](../../include/governance/policy_manager.h) | Loads/saves `PolicyRule` sets and manages rule state/version operations. | Check `PolicyResult` and lifecycle transitions; invalid transition, missing rule, conflict, or audit failure is not success. |
| `OpaAdapter` | [`opa_adapter.h`](../../include/governance/opa_adapter.h) | Evaluates request headers and route against configured OPA REST endpoint/policy path; returns `optional<PolicyDecision>`. | `nullopt` represents unavailable, non-2xx, or invalid response. Engine fallback is native evaluation. Built-in WASM stub is not policy evaluation. |
| `DataMasker` | [`data_masker.h`](../../include/governance/data_masker.h) | Applies a `FieldMaskingPolicy` to a JSON document or array. | Disabled/empty policy is a no-op; only matching string fields are transformed. |
| `DataLineageTracker` | [`data_lineage.h`](../../include/governance/data_lineage.h) | Records `LineageEvent`; queries dataset/upstream/downstream lineage and exports JSON. | Check `LineageRecordResult`; audit circuit-open can mean locally recorded but not forwarded. Limits can evict oldest events. |
| `ModelGovernancePolicy` | [`model_governance.h`](../../include/governance/model_governance.h) | Checks `ModelTrainingExportRequest` and returns permit/deny plus optional lineage event ID. | Restricted collections and strict classifications are denied; configure the audit and lineage sinks required by the deployment. |
| Subject rights | [`gdpr_subject_rights.h`](../../include/governance/gdpr_subject_rights.h) | Calls registered erase/export targets and returns per-store erasure reports or portability payload. | Empty subject ID throws `std::invalid_argument`; erasure can partially fail and is not atomic across stores. |
| Cross-border transfer | [`cross_border_transfer.h`](../../include/governance/cross_border_transfer.h) | Checks destination region and optional classification; returns allowed flag, mechanism, reason, and header value. | Unknown destinations resolve to `PROHIBITED`; callers must enforce the result. |
| Compliance errors | [`governance_api_contract.h`](../../include/governance/governance_api_contract.h) | `GovError`, `ComplianceCheckResult`, and supported contract-level regulations. | A default result is denied. These types do not replace the more specific rule-set APIs. |

## Result handling

- Treat a returned decision/result as structured data: check `allowed`/`is_permitted`, error code, explanatory reason, and related reports as applicable.
- Do not infer success from a non-throwing call or a non-empty report alone.
- For policy evaluation, distinguish native, OPA REST, injected WASM, and built-in WASM stub paths. Native fallback is not equivalent to the unavailable external policy.
- For subject erasure, report completion only when `fully_erased` is true and all `store_results` are successful.
- For masking and transfer controls, the caller owns enforcement at the operation/output boundary.

`governance_api_contract.h` documents the frozen v2.x contract-level error taxonomy (`kRegulationUnknown`, `kConsentMissing`, `kJurisdictionBlocked`, `kExportLimitExceeded`, `kAuditWriteFailed`, `kRuleConflict`). Other APIs have their own result/error types; do not treat these values as a global error enum.

## Threading and ownership

Thread-safety is interface-specific. The frozen API contract states internal synchronization for its covered compliance surfaces; individual managers expose their own mutex-protected behavior. `PolicyEngine::setOpaEvaluator()` stores a non-owning evaluator pointer, so the evaluator must outlive calls that use it. Shared logger, model-policy, and lineage dependencies use `shared_ptr` where declared; caller-owned callbacks and targets must remain valid for the duration of their use.

## Source references

- [Public header index](../../include/governance/README.md)
- [Implementation guide](IMPLEMENTATION.md)
- [Architecture](ARCHITECTURE.md)
- [Trust boundaries](TRUST.md)
