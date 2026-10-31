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

# Governance Developer Module Guide

Use this page to find the contract, implementation, focused test source, and
benchmark registration relevant to a change. Public declarations are in
`include/governance/`; implementation is in `src/governance/`.

| Change area | Start with | Focused source evidence |
|---|---|---|
| Policy evaluation/configuration | `policy_engine.h`, `policy_engine.cpp` | `test_governance_policy_simulation.cpp`, `test_governance_policy_hot_reload.cpp`, `test_governance_opa_adapter.cpp` |
| Lifecycle, review, versioning | `policy_manager.h`, `policy_review.h`, versioning headers | `test_governance_review_scheduler.cpp`, `test_governance_phase2_phase3_focused.cpp`; `test_policy_versioning_and_approval.cpp` is currently excluded from the module-focused CMake loop |
| Compliance framework | compliance framework/mapping/validator headers | `test_compliance_framework.cpp`; registered by the module-focused loop |
| Masking, lineage, model governance | `data_masker.h`, `data_lineage.h`, `model_governance.h` | `test_governance_phase2c_lineage_backpressure_focused.cpp`, `test_governance_phase3b_extended_focused.cpp`; verify exact cases before claiming coverage |
| Audit integrity | `governance_audit_integrity.h`, `audit_batch_writer.h` | `test_audit_integrity.cpp` is registered by the module-focused loop |
| Operational audit/evidence | `operational_audit.h` | `test_operational_audit_evidence.cpp` exists but is currently excluded from the module-focused CMake loop |
| Governance contract types | `governance_api_contract.h` | `test_governance_contract_hardening_focused.cpp` |
| Policy evaluation performance | `PolicyEngine` APIs | `bench_governance_policy_latency.cpp`; registration in `benchmarks/governance/CMakeLists.txt` |

The test directory has additional suites. The exact current module-focused
registration is controlled by
[`../../tests/governance/CMakeLists.txt`](../../tests/governance/CMakeLists.txt).
That file skips `test_operational_audit_evidence` and
`test_policy_versioning_and_approval`; source presence is not test registration,
execution, or a passing result. For an already configured build, list matching
tests with `ctest --test-dir <build-dir> -N -R governance`. For example, the
compliance framework source currently maps to target
`module_governance_test_compliance_framework_focused` and CTest name
`test_compliance_framework_governance_FocusedTests`; confirm names in the
selected build before running. No build/test/benchmark command is run as part
of this documentation update.

## Development workflow

1. Read [architecture](ARCHITECTURE.md), then the relevant [API](API.md),
   [implementation](IMPLEMENTATION.md), [trust](TRUST.md), or [runbook](RUNBOOK.md).
2. Keep caller-visible contract changes in the public header and implementation
   details in `src/governance/`.
3. Add a focused regression for the changed outcome and its negative/degraded
   case; verify it is registered in the current CMake flow.
4. For a hot-path change, consult
   [performance expectations](PERFORMANCE_EXPECTATIONS.md); the benchmark
   source's existence does not establish a measured baseline or pass.
5. Update documentation when behavior, trust assumptions, or configuration
   semantics change.

## Behavior cautions

- `PolicyEngine` returns decisions; callers enforce them.
- OPA REST unavailability may lead to native evaluation, which is not equivalent
  to the external policy.
- Without an injected WASM evaluator, the existing built-in WASM stub can return
  a permissive observe result when a bundle exists and the real runtime is
  unavailable; do not use it to authorize production operations.
- Masking disabled or with no rules leaves input unchanged; matching applies to
  JSON string fields.
- Lineage and operational audit data are bounded in-process state, not evidence
  of durable storage.
- Multi-store subject-rights operations report individual outcomes; inspect
  their result types before treating a request as complete.

## Related documentation

- [README](README.md) · [Architecture](ARCHITECTURE.md) · [Roadmap](ROADMAP.md)
- [API](API.md) · [Implementation](IMPLEMENTATION.md) · [Trust](TRUST.md)
- [Public-header inventory](../../include/governance/README.md)
