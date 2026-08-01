# ThemisDB Governance Module

<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The governance module provides policy enforcement and compliance governance runtime surfaces for ThemisDB, including policy evaluation, masking, lineage, compliance rules/reporting, policy lifecycle/versioning, and OPA integration.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| policy_engine.cpp | core policy decision and query permission evaluation |
| policy_manager.cpp | policy lifecycle management |
| policy_manager_versioned.cpp | policy version history, rollback, and conflict handling |
| policy_validation.cpp | policy validation execution paths |
| policy_validator.cpp | validation helper/runtime rules |
| policy_template.cpp | policy template provisioning |
| policy_review.cpp | policy review workflow logic |
| review_scheduler.cpp | review scheduling operations |
| policy_file_watcher.cpp | policy hot-reload trigger surfaces |
| policy_coordinator.cpp | multi-policy coordination paths |
| compliance_reporter.cpp | compliance evidence/report aggregation |
| compliance_reporting.cpp | compliance report rendering/export |
| gdpr_subject_rights.cpp | GDPR rights handling |
| ccpa_rules.cpp | CCPA/CPRA rule evaluation |
| hipaa_rules.cpp | HIPAA rule evaluators |
| iso27001_rules.cpp | ISO 27001 control evaluators |
| pci_dss_rules.cpp | PCI-DSS rule evaluators |
| soc2_controls.cpp | SOC 2 control evaluators |
| data_masker.cpp | field-level masking/redaction |
| data_lineage.cpp | data lineage tracking |
| model_governance.cpp | AI/ML governance policy controls |
| cross_tenant_policy_inheritance.cpp | tenant policy inheritance logic |
| opa_adapter.cpp | OPA integration adapter |

## Scope

In scope:
- policy-based governance evaluation and enforcement paths
- compliance control/rule and reporting workflows
- masking/lineage/model-governance and policy lifecycle/versioning

Out of scope:
- authentication ownership outside governance policy decisions
- cryptographic primitive ownership outside governance interfaces
- external audit storage subsystem ownership

## Runtime Behavior and Limits

- policy outcomes depend on loaded policy definitions and runtime headers/context.
- compliance/masking/model-governance behavior is driven by configured rule sets.
- integration fallback behavior (e.g. OPA) follows explicit bounded failure paths.

## Sourcecode Verification (Module: governance/readme)

- Verified files:
  - src/governance/policy_engine.cpp
  - src/governance/policy_manager.cpp
  - src/governance/policy_manager_versioned.cpp
  - src/governance/policy_validation.cpp
  - src/governance/policy_validator.cpp
  - src/governance/policy_template.cpp
  - src/governance/policy_review.cpp
  - src/governance/review_scheduler.cpp
  - src/governance/policy_file_watcher.cpp
  - src/governance/policy_coordinator.cpp
  - src/governance/compliance_reporter.cpp
  - src/governance/compliance_reporting.cpp
  - src/governance/gdpr_subject_rights.cpp
  - src/governance/ccpa_rules.cpp
  - src/governance/hipaa_rules.cpp
  - src/governance/iso27001_rules.cpp
  - src/governance/pci_dss_rules.cpp
  - src/governance/soc2_controls.cpp
  - src/governance/data_masker.cpp
  - src/governance/data_lineage.cpp
  - src/governance/model_governance.cpp
  - src/governance/cross_tenant_policy_inheritance.cpp
  - src/governance/opa_adapter.cpp
- Verified behavior surfaces:
  - policy lifecycle/evaluation and compliance rule/reporting paths
  - masking/lineage/model-governance integration boundaries
  - versioning/review/hot-reload governance operations
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md