# Security - Governance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in governance focuses on safe policy enforcement boundaries, compliance control integrity, privacy-preserving masking/lineage controls, and explicit failure behavior for unsafe governance outcomes.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| policy bypass or weak enforcement outcomes | explicit policy-engine checks and fail-closed denial behavior |
| compliance drift or missing control evidence | dedicated control evaluators and reporting surfaces |
| leakage of sensitive data in governed outputs | masking/redaction and governance permission gates |
| degraded external policy integration behavior | bounded fallback behavior with explicit outcomes |
| hidden governance regressions | versioning, review, and observability/audit surfaces |

## Implemented Security Controls

- policy and query permission paths gate protected operations.
- compliance evaluators and reporters provide control/evidence traces.
- data masking and governance checks constrain sensitive output exposure.
- integration fallbacks surface deterministic non-silent outcomes.

## Security Follow-ups

- continue hardening cross-tenant and model-governance edge behavior.
- tighten diagnostics for fallback, denial, and policy-conflict paths.
- expand stress coverage for high-volume governance evaluation scenarios.

## Sourcecode Verification (Module: governance/security)

- Verified files:
  - src/governance/policy_engine.cpp
  - src/governance/data_masker.cpp
  - src/governance/compliance_reporter.cpp
  - src/governance/compliance_reporting.cpp
  - src/governance/model_governance.cpp
  - src/governance/opa_adapter.cpp
- Verified controls:
  - fail-closed policy enforcement and governance permission checks
  - compliance evidence/control evaluation paths
  - masking and fallback-aware security behavior