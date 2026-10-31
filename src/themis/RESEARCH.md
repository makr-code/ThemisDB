---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Research and Evaluation — Themis Core

## Purpose and scope

This page records how to evaluate the deterministic runtime contracts owned by
`src/themis`: build identity, edition and license gates, module verification
and loading, dependency resolution, and the wire server. It is for maintainers
reviewing changes to those paths. It is not a research claim that the module
implements a learned model or a newly proposed algorithm.

## Questions and assumptions

The evaluation should answer whether:

1. equivalent build inputs expose consistent identity and edition data;
2. invalid or unavailable license state cannot enable gated features;
3. altered, unsigned, or unverifiable modules are rejected according to the
   configured verification policy before activation;
4. dependency and platform-loader failures are explicit and bounded; and
5. wire session lifecycle and message handling remain stable under connection
   churn and malformed input.

The relevant contracts are in
[`build_info.h`](../../include/themis/build_info.h),
[`edition.h`](../../include/themis/edition.h),
[`edition_manager.h`](../../include/themis/edition_manager.h),
[`license_info.h`](../../include/themis/license_info.h),
[`module_hash_verifier.h`](../../include/themis/module_hash_verifier.h),
[`module_signature_verifier.h`](../../include/themis/module_signature_verifier.h),
and [`runtime_license_gate.h`](../../include/themis/runtime_license_gate.h).
Platform and wire behavior must also be checked against the actual loader and
wire interfaces; the sources under `src/themis` do not make those interfaces
interchangeable.

## Evaluation method and evidence

Use the contract tests for return values and failure cases, integration tests
for cross-surface behavior, and stress/soak suites for lifecycle stability.
The benchmark suite measures latency and throughput; do not infer a production
threshold from a benchmark name or a source-only audit.

| Question | Existing evidence |
|---|---|
| Gate, build identity, and loader contracts | [`test_themis_contract_hardening_focused.cpp`](../../tests/themis/test_themis_contract_hardening_focused.cpp) |
| Cross-component and platform behavior | [`test_themis_integration.cpp`](../../tests/themis/test_themis_integration.cpp) |
| Wire protocol and session failures | [`test_themis_wire_protocol_server.cpp`](../../tests/themis/test_themis_wire_protocol_server.cpp) |
| Loader contention and high-cardinality behavior | [`test_themis_highcardinality_stress.cpp`](../../tests/themis/test_themis_highcardinality_stress.cpp) |
| Sustained load/verify, session, and dependency behavior | [`test_themis_core_soak.cpp`](../../tests/integration/test_themis_core_soak.cpp) |
| Core and release benchmark cases | [`benchmarks/themis`](../../benchmarks/themis/) and [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) |

For a reproducible comparison, record the source revision, edition/build
configuration, platform, test or benchmark target, and baseline artifact.
Run the same cases and hardware conditions for both baseline and candidate.
The module performance document defines a 10% regression comparison, p99
release-threshold checks, and benchmark-manifest completeness; the threshold
values and run artifact belong to the configured release baseline, not this
page.

## Limits and interpretation

- Unit tests establish the tested contract, not universal platform equivalence.
- Hash equality establishes content integrity relative to the expected hash;
  it does not identify who authorized that hash.
- Signature verification depends on the platform trust mechanism and key
  configuration. A successful result must not be generalized to an untested
  keyring or certificate policy.
- Build and test evidence in
  [`BUILD_TEST_EVIDENCE.md`](./BUILD_TEST_EVIDENCE.md) is a dated inventory;
  use current CMake registration and test output as execution evidence.
- The module's audit and performance documents include older snapshots; check
  their source links and current baselines before making release decisions.

See [modeling](./MODELING.md) for the deterministic state models used by these
tests and [operations](./OPERATIONS.md) for live diagnosis.
