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

# Governance Trust Boundaries and Risk Model

**Scope:** `src/governance`; source-of-truth domain: security. Governance decisions constrain data use and exports; they do not replace caller authentication or server authorization.

## Trust boundaries

- Request identity/tenant and classification metadata to policy evaluation: callers and request headers are not policy authorities unless an upstream authenticated component establishes and protects them.
- Native policy or OPA evaluator to data-access/export callers: a decision is trusted only if the active policy source and evaluation mode are known.
- Masking policy to output: masking only applies when enabled with applicable rules and when every relevant output path invokes the masker.
- Erasure request to registered stores: each store reports its own outcome; a multi-store request is not atomic.
- Policy/audit evidence to operators: policy versions, evaluation results, and erasure reports are sensitive operational records.

## Controls and operating conditions

- Native policy evaluation defaults an absent classification to `streng-geheim` and treats unknown classifications strictly. Known classification profiles still apply their configured export, cache, encryption, and redaction permissions.
- Cross-tenant policy combination uses restrictive AND semantics for allow/export/cache decisions; any denying input remains denying.
- If an external OPA evaluation is unavailable, `PolicyEngine` falls back to native evaluation and records a fallback metric. Operators must know which evaluator produced a decision.
- `DataMasker` returns the input unchanged when masking is disabled or no rules are configured; ensure policies are loaded and all output paths apply masking.
- Erasure reports set `fully_erased=false` when any store fails and retain per-store results. Retry and verify failed stores; do not treat partial completion as successful erasure.

## Explicit non-production OPA WASM path

- **Purpose:** local bundle evaluation without an OPA sidecar.
- **Activation:** `EvalMode::WASM`, a configured bundle path, no injected `WasmEvalFn`, and a build without `THEMIS_ENABLE_OPA_WASM`.
- **Behavior difference:** the current stub checks only that the bundle exists, then returns a permissive `offen`/`observe` decision; it does not evaluate Rego.
- **Operational rule:** do not rely on this path for production authorization or export controls. Use the supported external/native evaluator and verify the active mode.
- **Removal condition:** replace the stub with a real WASM evaluator and verify enforcement before enabling this mode. The source comment lists target `v1.6.0`, but the repository version is `2.4.0` and no matching target appears in the current governance roadmap or future-enhancements file; treat the milestone as unverified and reconcile it in the source of truth.

## Incident handling

On suspected policy bypass, unexpected fallback, or incomplete erasure, stop the affected operation, preserve policy versions and audit reports, identify the exact policy/evaluator/mode and impacted stores, then validate corrected decisions or complete erasure before resuming. Report vulnerabilities privately through [project SECURITY.md](../../SECURITY.md), using [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Source references

- `src/governance/policy_engine.cpp`
- `src/governance/opa_adapter.cpp`
- `src/governance/data_masker.cpp`
- `src/governance/cross_tenant_policy_inheritance.cpp`
- `src/governance/gdpr_subject_rights.cpp`
- [Module security summary](SECURITY.md)
