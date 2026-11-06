# Governance Module - Future Enhancements

<!-- Status: current | Last Updated: 2026-11-06 | validated: 2026-11-06 -->
<!-- Validation Cycle: Issue #7422 roadmap alignment; OPA WASM runtime remains unimplemented and requires maintainer approval -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
-->

## Scope

- hardening and refinement of governance policy/compliance/data-protection runtime behavior
- expansion of deterministic reliability under high-volume multi-policy workloads
- stronger benchmark-backed guardrails for governance hot paths
- stage the proposed work into compatibility-preserving v2.5.0 waves and
  contract/architecture evolution for v3.0.0; the canonical tasks, evidence
  requirements, risks, and estimates are in [`ROADMAP.md`](ROADMAP.md)

## Design Constraints

- governance contracts remain backward compatible within major release line.
- policy denial/fallback behavior remains explicit and fail closed.
- compliance and data-governance controls remain bounded and observable.
- policy lifecycle transitions remain auditable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| policy interfaces | deterministic evaluation and lifecycle semantics |
| compliance interfaces | explicit rule/control/reporting behavior |
| data-governance interfaces | bounded masking/lineage/model-governance protections |
| operations interfaces | stable versioning/review/watch/coordination behavior |

## Implementation Notes

- v2.5.0 Wave 1: specify native/REST/WASM, conflict, and fallback outcomes;
  resolve or explicitly maintain the blockers for the two excluded focused test
  suites.
- v2.5.0 Wave 2: require successful YAML loading in YAML-labelled benchmark
  cases and publish reproducible policy/audit baseline metadata and percentiles.
- v2.5.0 Wave 3: verify caller enforcement and failure behavior for masking,
  permissions, lineage limits, audit pressure, subject rights, and rollback.
- v3.0.0 Wave 4: define approved durable evidence, retention, recovery, and
  signing-key lifecycle contracts before claiming persistence.
- v3.0.0 Wave 5: implement local OPA WASM only after runtime/dependency approval;
  explicit WASM mode remains strict-deny when its evaluator is unavailable.
- v3.0.0 Wave 6: version tenant and AI-governance contracts and approve any
  breaking API/policy migration.

## Test Strategy

- Wave 1: table-driven contract coverage for evaluator selection, deny/allow,
  malformed responses, timeout, and unavailable evaluators; build and run both
  currently excluded suites or retain explicit owner-approved blockers.
- Wave 2: run mapped policy and audit cases with verified policy/configuration;
  retain build, hardware, workload, and p50/p95/p99 results for comparison.
- Wave 3: integration and stress regressions for caller-side enforcement,
  conflict/fallback, inheritance, partial subject-rights operations, lineage
  backpressure, audit failures, and rollback.
- Waves 4–6: crash/recovery and retention tests against the selected durable
  backend; approved OPA bundle allow/deny and failure corpus; cross-tenant
  negative tests and migration compatibility tests.

## Performance Targets

- capture a same-host, same-build baseline before setting performance release
  gates; do not treat source comments as measured results.
- provisional v2.5.0 policy p99 target is <0.5 ms, as documented in the existing
  benchmark source; approve it for a named release profile only after validating
  workload, configuration, and target hardware.
- keep policy/query-permission performance within a maintainer-approved 10%
  same-host regression budget; record p95/p99 for audit and masking cases.
- require every mapped benchmark case to report its effective policy and
  successful setup, with no silent fallback due to missing YAML.

## Security / Reliability

- maintain strict fail-closed policy enforcement before protected operations.
- preserve explicit failure signaling for fallback and conflict paths.
- enforce bounded lifecycle and reporting behavior under pressure.
- keep diagnostics actionable for production governance incidents.

## OPA WASM Evaluation

### Scope
- Add an approved WASM runtime and actual local evaluation of compiled OPA policies.
- Until then, explicit WASM mode only uses an injected evaluator and returns an
  enforced strict deny when it is absent or fails, preventing native fallback.
- It never falls through to REST/native evaluation; existence of a bundle file is not evaluation.

### Design Constraints
- Runtime and dependency selection require maintainer approval; no runtime is
  currently integrated or enabled.
- Missing bundles, invalid modules, runtime failures, and evaluation errors must never produce a permissive allow decision.
- Preserve the injected `WasmEvalFn` path and keep evaluator selection explicit.

### Required Interfaces
- Define the contract between `OpaAdapter`, the selected WASM runtime, the compiled bundle, and the resulting `PolicyDecision`.
- Keep build-time runtime enablement and runtime mode selection aligned; a build flag alone must not imply that real evaluation is available.

### Implementation Notes
- Select and approve the runtime/dependency before assigning a release target or enabling the production path.
- Integrate actual policy evaluation only with fail-closed error handling and an end-to-end tested runtime.
- **Planning target:** v3.0.0 Wave 5, tentative and blocked on maintainer runtime/dependency approval; this is not an implementation commitment.

### Test Strategy
- Verify valid bundle decisions against representative allow and deny policies.
- Cover missing and malformed bundles, unsupported modules, runtime errors, and evaluator fallback without fail-open outcomes.
- Verify injected-evaluator precedence and integration with the governance decision path.

### Performance Targets
- Establish and record evaluation latency and resource limits after runtime selection; no release performance target is approved yet.

### Security / Reliability
- The strict-deny fallback is not policy evaluation. Do not treat it as a
  substitute for an approved runtime or use it to imply that bundle policy was
  evaluated.
- Require tests proving that unavailable or failed WASM evaluation cannot authorize protected operations before enabling the implementation.
