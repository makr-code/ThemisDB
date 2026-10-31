---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
Source Level: "level1"
SOT Domain: "module-behavior"
Canonical References: "src/governance sources and include/governance headers; tests/governance/CMakeLists.txt; benchmarks/governance/CMakeLists.txt; root ROADMAP.md"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Last Updated: 2026-10-31 | Issue #7349 source reconciliation -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
-->

## Current status

Governance implementation surfaces exist for policy evaluation/lifecycle,
compliance rules/reporting, masking/lineage/model governance, audit-related
utilities, and OPA integration. This is a source inventory, not a production
readiness or release gate.

The built-in OPA WASM path remains a permissive non-evaluating stub in the
no-runtime/no-injected-evaluator case. A real evaluator and its release target
remain unverified pending maintainer confirmation. See
[`TRUST.md`](TRUST.md).

Root [`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for aggregate
release readiness and Wave C. No module-local status can close those project
gates.

## In progress / open verification

- [~] Clarify conflict/fallback diagnostics and behavior across individual
  policy surfaces; do not assume the conflict detector defines `PolicyEngine`
  precedence.
- [~] Expand policy-latency measurement and record reproducible baselines. The
  current benchmark source contains cases, but no current benchmark result is
  asserted here.
- [ ] Revisit masking/lineage/model-governance integration boundaries and
  continue focused regression coverage.
- [ ] Verify persistent storage, audit/key lifecycle, and deployment integration
  where required; module declarations alone do not establish them.
- [?] Replace the permissive built-in OPA WASM stub only after runtime approval
  and fail-closed behavior are verified. Release target: unverified.

## Implementation and evidence notes

- Focused test source files exist under `tests/governance/`. The module CMake
  loop currently skips `test_operational_audit_evidence` and
  `test_policy_versioning_and_approval`; their presence is not registered test
  evidence.
- The policy-latency benchmark target is registered conditionally when
  `THEMIS_BUILD_BENCHMARKS` is enabled. Its YAML load is best-effort; see
  [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
- `AuditBatchWriter::recordMetrics()` contains rolling percentile bookkeeping,
  and `PolicyChangeManager::executeRollback()` delegates to
  `PolicyManager::rollbackToVersion()`. These implementation facts do not
  establish production-load accuracy or an end-to-end integration test pass.

## Known issues and limits

- A failed OPA REST evaluation can lead to native evaluation; native fallback
  is not the same as evaluating the external policy.
- In the built-in WASM stub path, an existing bundle can lead to a permissive
  `observe` result. Do not use it for production authorization.
- Lineage and operational audit surfaces have bounded in-process data; durable
  audit/evidence storage is not implied.
- Compliance report output is not independent certification.
- Current test/benchmark registration does not establish that builds, tests, or
  benchmarks have passed.

## Wave context

Governance contributes to Wave C security-production validation in the root
program roadmap. Governance-specific security integration evidence and
sustained-load validation remain open; no claim is made here that the module
alone closes Wave C.

## Change policy

Record a breaking API change in the module changelog and follow the repository
versioning policy. This roadmap does not promise that no breaking changes are
planned.
