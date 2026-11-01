---
Author: "ThemisDB Contributors"
Created: "2026-04-06"
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

# Governance Public Headers

`include/governance/` contains the public C++ declarations. These headers describe
interfaces; they do not establish that a caller enforces a returned decision, or
that a control demonstrates regulatory compliance. Implementation is in
[`src/governance/`](../../src/governance/).

## Header inventory

Current directory inventory (36 headers):

| Area | Headers |
|---|---|
| Policy decisions and lifecycle | `policy_engine.h`, `policy_manager.h`, `policy_manager_versioned.h`, `policy_coordinator.h`, `policy_file_watcher.h`, `policy_template.h`, `policy_validation.h`, `policy_validator.h`, `policy_version_history.h`, `policy_conflict_detector.h`, `policy_approval_workflow.h`, `policy_change_manager.h`, `policy_review.h`, `review_scheduler.h` |
| Compliance rules, mapping, and reports | `compliance_framework.h`, `compliance_mapping.h`, `compliance_validator.h`, `compliance_reporter.h`, `compliance_reporting.h`, `ccpa_rules.h`, `gdpr_subject_rights.h`, `hipaa_rules.h`, `iso27001_rules.h`, `pci_dss_rules.h`, `soc2_controls.h`, `cross_border_transfer.h` |
| Data governance and integration | `data_masker.h`, `data_lineage.h`, `cross_tenant_policy_inheritance.h`, `model_governance.h`, `opa_adapter.h` |
| Audit and diagnostics | `audit_batch_writer.h`, `governance_audit_integrity.h`, `governance_diagnostics.h`, `operational_audit.h`, `governance_api_contract.h` |

Names and declarations are owned by the individual headers. Examples include
`PolicyEngine`, `PolicyDecision`, `PolicyManager`, `PolicyManagerWithVersioning`,
`OpaAdapter`, `DataMasker`, `DataLineageTracker`,
`GdprSubjectRightsManager`, `ModelGovernancePolicy`,
`PolicyConflictDetector`, `AuditSigner`, `AuditIntegrityManager`,
`AuditBatchWriter`, `OperationalAuditLogger`, and `ComplianceFrameworkRegistry`.
Do not substitute similarly named aliases from older documentation for the
declared C++ types.

## Configuration and behavior pointers

- `PolicyEngine::loadFromYAML()` reads `vs_classification`,
  `enforcement.resource_mapping`, `enforcement.default_mode`, and
  `data_masking.enabled` / `data_masking.rules`; see
  [`policy_engine.h`](policy_engine.h) and
  [`src/governance/IMPLEMENTATION.md`](../../src/governance/IMPLEMENTATION.md).
- `OpaAdapter::Config` declares endpoint, policy path, timeout, evaluation mode,
  and WASM bundle path. The built-in WASM path is not a Rego evaluator; see
  [`opa_adapter.h`](opa_adapter.h) and
  [`src/governance/TRUST.md`](../../src/governance/TRUST.md).
- For returned decisions/results, check their documented fields and apply any
  restrictions at the caller's operation boundary.

## Build and tests

The repository's `linux-release` preset exists in `CMakePresets.json`; it
requires the documented Linux/vcpkg toolchain and configures tests and
benchmarks off by default. Consult `SETUP.md` and the selected preset before
building. There is no module-specific `ENABLE_GOVERNANCE` switch. Tests are
registered through the repository CMake configuration;
`tests/governance/CMakeLists.txt` currently excludes the
operational-audit-evidence and policy-versioning/approval suites from that
module-focused registration. Check the current configured CTest list before
selecting a test. Benchmarks are optional (`THEMIS_BUILD_BENCHMARKS`) and their
current location/targets are documented in
[`benchmarks/governance/README.md`](../../benchmarks/governance/README.md).

## Related documentation

- [Module overview](../../src/governance/README.md)
- [Module architecture](../../src/governance/ARCHITECTURE.md)
- [Public API guide](../../src/governance/API.md)
- [Module roadmap](../../src/governance/ROADMAP.md)
