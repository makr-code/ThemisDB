---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security - Governance Module

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page describes governance controls, not product-wide certification.

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
- Policy and masking depend on validated policy inputs and correct use by every caller; a disabled masking policy or empty rules returns data without masking.
- When configured OPA evaluation is unavailable, `PolicyEngine` falls back to native evaluation and emits a fallback metric. Missing classifications default to the strictest class, but standard configured classifications retain their policy-defined permissions.
- Do not use the current OPA WASM stub for enforcement: with WASM mode and an existing bundle, when no evaluator is injected and `THEMIS_ENABLE_OPA_WASM` is absent, it returns a permissive observe-mode decision without evaluating the bundle.
- Subject erasure is per-store and can partially fail; inspect `fully_erased` and each store result before reporting completion. The operation has no cross-store rollback.

## Incident Handling

For suspected policy bypass, masking failure, or incomplete erasure, stop the affected export/mutation path, preserve policy versions and audit evidence, identify the route/classification and evaluator/fallback in use, and verify corrected decisions and per-store erasure results before resuming. Follow [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

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

- Additional source references:
  - `src/governance/policy_engine.cpp`
  - `src/governance/opa_adapter.cpp`
  - `src/governance/data_masker.cpp`
  - `src/governance/gdpr_subject_rights.cpp`
  - [Trust and risk model](TRUST.md)