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
- If OPA REST evaluation is unavailable, `PolicyEngine` falls back to native
  evaluation and records a fallback metric. Explicit WASM mode returns an
  enforced strict deny when its injected evaluator is unavailable. Operators
  must know which evaluator produced a decision.
- `DataMasker` returns the input unchanged when masking is disabled or no rules are configured; ensure policies are loaded and all output paths apply masking.
- Erasure reports set `fully_erased=false` when any store fails and retain per-store results. Retry and verify failed stores; do not treat partial completion as successful erasure.

## Explicit OPA WASM mode

- **Purpose:** local evaluation of an OPA policy bundle without an OPA sidecar;
  a built-in runtime is not currently provided.
- **Activation:** `EvalMode::WASM`; the bundle path alone does not execute policy.
- **Behavior:** only an injected `WasmEvalFn` may produce an allow decision.
  Without one, or when it fails, the adapter returns an enforced strict deny
  and does not fall back to REST or native evaluation.
- **Operational rule:** verify the injected evaluator and mode before relying
  on local policy enforcement. The failure path intentionally denies.
- **Future work:** select and approve a real WASM runtime, then verify policy
  evaluation and enforcement before enabling it.

## Incident handling

On suspected policy bypass, unexpected fallback, or incomplete erasure, stop the affected operation, preserve policy versions and audit reports, identify the exact policy/evaluator/mode and impacted stores, then validate corrected decisions or complete erasure before resuming. Report vulnerabilities privately through [project SECURITY.md](../../SECURITY.md), using [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Source references

- `src/governance/policy_engine.cpp`
- `src/governance/opa_adapter.cpp`
- `src/governance/data_masker.cpp`
- `src/governance/cross_tenant_policy_inheritance.cpp`
- `src/governance/gdpr_subject_rights.cpp`
- [Module security summary](SECURITY.md)
