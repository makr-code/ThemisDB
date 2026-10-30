---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Governance Developer Module Guide

## Purpose and entry points

Use this page to find the right contract, implementation, and regression suite before changing governance behavior. Public interfaces are in `include/governance/`; the implementation is in `src/governance/`; focused module tests are in `tests/governance/`.

| Change area | Start with | Focused evidence |
|---|---|---|
| Policy evaluation, classification, or hot reload | `policy_engine.h`, `policy_engine.cpp` | `test_governance_opa_adapter.cpp`, `test_governance_policy_simulation.cpp`, `test_governance_policy_hot_reload.cpp` |
| Policy lifecycle, versioning, review | `policy_manager.h`, `policy_manager_versioned.h`, `policy_review.h` | `test_governance_review_scheduler.cpp` and policy lifecycle/versioning suites |
| OPA integration/failure handling | `opa_adapter.h`, `opa_adapter.cpp` | `test_governance_opa_adapter.cpp` |
| Compliance rules and reports | `compliance_reporter.h`, framework-specific rule headers | `test_compliance_framework.cpp`, `test_governance_compliance_time_window.cpp` |
| Masking, lineage, model export | `data_masker.h`, `data_lineage.h`, `model_governance.h` | lineage backpressure and focused model-governance tests |
| Subject rights and transfers | `gdpr_subject_rights.h`, `cross_border_transfer.h` | corresponding governance/integration tests |
| Audit integrity and diagnostics | `governance_audit_integrity.h`, `governance_diagnostics.h` | `test_audit_integrity.cpp`, focused diagnostics and lineage tests |

The test inventory evolves with CMake registration. Inspect `tests/governance/CMakeLists.txt` and the configured build’s CTest listing for the exact target and test name before running a focused C++ suite.

## Development workflow

1. Read [ARCHITECTURE.md](ARCHITECTURE.md), then the relevant [API](API.md), [implementation](IMPLEMENTATION.md), [trust](TRUST.md), or [runbook](RUNBOOK.md) section.
2. Follow the public-header/internal-source boundary. Make caller-visible contract changes in the header and document purpose, inputs, outputs, errors, ownership, and thread-safety there.
3. Add a focused regression for the changed decision and its negative/degraded case. Useful existing examples include missing/unknown classification, OPA unavailable/malformed output, CCPA opt-out, masking disabled, lineage limits/circuit state, and partial erase results.
4. Build and execute the focused CTest target for the changed suite. In a configured build, use `ctest --test-dir <build-dir> -N -R governance` to discover registered names, then select the relevant test with `-R`.
5. For hot-path changes, use the cases mapped in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) and compare against the release baseline. A unit-test pass is not benchmark evidence.
6. Update module docs and incident guidance when behavior, trust assumptions, or recovery steps change.

## Invariants to preserve

- Native policy evaluation fails closed for missing/unmapped classification; CCPA opt-out blocks export after OPA or native evaluation.
- Callers must enforce the decision fields. `PolicyEngine::evaluate()` returns a decision; it does not itself execute or cancel the downstream query.
- OPA unavailability produces a native fallback, not proof that the external policy was evaluated.
- The default built-in OPA WASM stub is permissive and must not authorize production operations.
- Masking is key-based and applies only to JSON strings; disabled or ruleless policies leave the input unchanged.
- Lineage is bounded, can evict oldest events, and may be locally recorded without audit forwarding while the circuit is open.
- Multi-store subject erasure is reported per store and has no cross-store rollback.

## Related documentation

- [README](README.md) · [Architecture](ARCHITECTURE.md) · [Roadmap](ROADMAP.md)
- [API](API.md) · [Implementation](IMPLEMENTATION.md) · [Research](RESEARCH.md)
- [Runbook](RUNBOOK.md) · [Operations](OPERATIONS.md) · [Trust](TRUST.md)
- [Public-header index](../../include/governance/README.md)
