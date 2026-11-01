---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Public API

## Purpose and contract

This is the module-level guide to the C++ interfaces under `include/governance/`. The public headers are the API source of truth; this page summarizes caller-visible behavior and important limits. Include `governance/<header>.h`; do not include implementation files from `src/governance/`.

## Complete public-header index

The current `include/governance/` inventory has 36 headers. This index covers
all files in that directory; declarations and exact types remain authoritative
in each header.

| Area | Headers |
|---|---|
| Policy decisions and lifecycle | [`policy_engine.h`](../../include/governance/policy_engine.h), [`policy_manager.h`](../../include/governance/policy_manager.h), [`policy_manager_versioned.h`](../../include/governance/policy_manager_versioned.h), [`policy_coordinator.h`](../../include/governance/policy_coordinator.h), [`policy_file_watcher.h`](../../include/governance/policy_file_watcher.h), [`policy_template.h`](../../include/governance/policy_template.h), [`policy_validation.h`](../../include/governance/policy_validation.h), [`policy_validator.h`](../../include/governance/policy_validator.h), [`policy_version_history.h`](../../include/governance/policy_version_history.h), [`policy_conflict_detector.h`](../../include/governance/policy_conflict_detector.h), [`policy_approval_workflow.h`](../../include/governance/policy_approval_workflow.h), [`policy_change_manager.h`](../../include/governance/policy_change_manager.h), [`policy_review.h`](../../include/governance/policy_review.h), [`review_scheduler.h`](../../include/governance/review_scheduler.h) |
| Compliance rules, mapping, and reports | [`compliance_framework.h`](../../include/governance/compliance_framework.h), [`compliance_mapping.h`](../../include/governance/compliance_mapping.h), [`compliance_validator.h`](../../include/governance/compliance_validator.h), [`compliance_reporter.h`](../../include/governance/compliance_reporter.h), [`compliance_reporting.h`](../../include/governance/compliance_reporting.h), [`ccpa_rules.h`](../../include/governance/ccpa_rules.h), [`gdpr_subject_rights.h`](../../include/governance/gdpr_subject_rights.h), [`hipaa_rules.h`](../../include/governance/hipaa_rules.h), [`iso27001_rules.h`](../../include/governance/iso27001_rules.h), [`pci_dss_rules.h`](../../include/governance/pci_dss_rules.h), [`soc2_controls.h`](../../include/governance/soc2_controls.h), [`cross_border_transfer.h`](../../include/governance/cross_border_transfer.h) |
| Data governance and evaluator integration | [`data_masker.h`](../../include/governance/data_masker.h), [`data_lineage.h`](../../include/governance/data_lineage.h), [`cross_tenant_policy_inheritance.h`](../../include/governance/cross_tenant_policy_inheritance.h), [`model_governance.h`](../../include/governance/model_governance.h), [`opa_adapter.h`](../../include/governance/opa_adapter.h) |
| Audit, diagnostics, and contract types | [`audit_batch_writer.h`](../../include/governance/audit_batch_writer.h), [`governance_audit_integrity.h`](../../include/governance/governance_audit_integrity.h), [`governance_diagnostics.h`](../../include/governance/governance_diagnostics.h), [`operational_audit.h`](../../include/governance/operational_audit.h), [`governance_api_contract.h`](../../include/governance/governance_api_contract.h) |

## Main interfaces

| Interface | Header | Inputs and outputs | Caller responsibility / edge case |
|---|---|---|---|
| `PolicyEngine` | [`policy_engine.h`](../../include/governance/policy_engine.h) | Loads YAML; evaluates headers plus route into `PolicyDecision`; offers query/inference checks and dry-run simulation. | Apply returned export/cache/ANN/redaction restrictions. Unknown/missing classification is strict in native evaluation. Simulation is dry-run and does not write an audit entry. |
| `PolicyManager` | [`policy_manager.h`](../../include/governance/policy_manager.h) | Loads/saves `PolicyRule` sets and manages rule state/version operations. | Check `PolicyResult` and lifecycle transitions; invalid transition, missing rule, conflict, or audit failure is not success. |
| `OpaAdapter` | [`opa_adapter.h`](../../include/governance/opa_adapter.h) | Evaluates request headers and route using configured OPA REST mode or an injected WASM evaluator; returns `optional<PolicyDecision>`. | REST `nullopt` may lead the engine to native fallback. In explicit WASM mode, missing/failing evaluator returns an enforced strict deny, not `nullopt`, so native fallback cannot bypass the selected mode. A bundle file alone is never evaluated. |
| `DataMasker` | [`data_masker.h`](../../include/governance/data_masker.h) | Applies a `FieldMaskingPolicy` to a JSON document or array. | Disabled/empty policy is a no-op; only matching string fields are transformed. |
| `DataLineageTracker` | [`data_lineage.h`](../../include/governance/data_lineage.h) | Records `LineageEvent`; queries dataset/upstream/downstream lineage and exports JSON. | Check `LineageRecordResult`; audit circuit-open can mean locally recorded but not forwarded. Limits can evict oldest events. |
| `ModelGovernancePolicy` | [`model_governance.h`](../../include/governance/model_governance.h) | Checks `ModelTrainingExportRequest` and returns permit/deny plus optional lineage event ID. | Restricted collections and strict classifications are denied; configure the audit and lineage sinks required by the deployment. |
| Subject rights | [`gdpr_subject_rights.h`](../../include/governance/gdpr_subject_rights.h) | Calls registered erase/export targets and returns per-store erasure reports or portability payload. | Empty subject ID throws `std::invalid_argument`; erasure can partially fail and is not atomic across stores. |
| Cross-border transfer | [`cross_border_transfer.h`](../../include/governance/cross_border_transfer.h) | Checks destination region and optional classification; returns allowed flag, mechanism, reason, and header value. | Unknown destinations resolve to `PROHIBITED`; callers must enforce the result. |
| Compliance errors | [`governance_api_contract.h`](../../include/governance/governance_api_contract.h) | `GovError`, `ComplianceCheckResult`, and supported contract-level regulations. | A default result is denied. These types do not replace the more specific rule-set APIs. |

## Result handling

- Treat a returned decision/result as structured data: check `allowed`/`is_permitted`, error code, explanatory reason, and related reports as applicable.
- Do not infer success from a non-throwing call or a non-empty report alone.
- For policy evaluation, distinguish native, OPA REST, and injected WASM. Explicit WASM mode does not silently select another evaluator.
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
- `tests/governance/CMakeLists.txt` keeps
  `test_operational_audit_evidence` and `test_policy_versioning_and_approval`
  quarantined pending canonical project configure/build/run validation. The
  operational-audit implementation and suite were built and run standalone
  locally (26/26 passed); project CMake configuration is blocked by missing
  RocksDB, and the versioning/approval suite has not been linked or run.
