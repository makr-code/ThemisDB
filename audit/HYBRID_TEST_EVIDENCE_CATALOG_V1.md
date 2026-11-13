---
Author: "ThemisDB Contributors"
Created: "2026-11-13"
Last Updated: "2026-11-13"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Hybrid Test Evidence Catalog v1

## Purpose and authority

This draft defines test obligations for the proposed profile **DB core + hybrid
profile v1 (RAG, LLM, LoRA, Tensor)**. It turns the current hybrid target
contracts into traceable test requirements before implementation work starts.
It is a profile-level evidence plan, not a release decision, a maturity score,
or a claim that the listed tests have run. `/audit/` remains the audit source
of truth; root `ROADMAP.md` and release governance remain authoritative for
release status.

The machine-readable catalog is
[`hybrid_test_evidence_catalog_v1.json`](hybrid_test_evidence_catalog_v1.json).
Its source snapshot is `develop` at `ee492ee342f11d14d61d55bbc27d89a07da4e797`
(2026-11-13). Existing test references below were inspected statically at that
snapshot. No build, CTest, model inference, or benchmark was run to create this
baseline.

## Proposed profile boundary

Include authoritative QueryEngine/storage and access-policy boundaries, source
ingestion and retrieval, local llama.cpp inference, bounded context assembly,
authorized DB-sample selection, AdaLoRA checkpointing, Multi-LoRA application,
and tensor/adapter handoff.

Exclude optional third-party framework adapters, durable human-approval
workflows, and cloud-provider failover. They may receive separate profiles and
must not be treated as prerequisites for this profile.

All 14 criteria in the JSON are **proposed required obligations**, not yet
maintainer-ratified acceptance policy. Numeric quality and performance
thresholds remain unset until representative baselines and an accountable
owner approve them. Existing targets in
[`src/rag/EVALUATION_CONTRACT_V1.md`](../src/rag/EVALUATION_CONTRACT_V1.md)
remain specifications, not measured baselines or automatically adopted gates.

## Test model

Test dimensions are orthogonal; they must not be collapsed into one exclusive
category:

| Dimension | Examples | Meaning |
|---|---|---|
| Verification level | unit, module integration, cross-module, end-to-end | What boundary or behavior is exercised |
| Selection | focused, aggregate, release-critical | Which tests are selected for a run |
| Workload/evidence | deterministic fixture, real model, database replay, benchmark | What makes the evidence representative |
| Environment | CPU, GPU, model artifact, RocksDB, sanitizer | Preconditions for a valid run |

`focused` is a selection mechanism, not a separate verification level.
Benchmark registration is not a benchmark result. A test source or CTest entry
is static inventory evidence only until the configured target is built and its
test cases execute.

### Coverage and execution states

Coverage and execution use separate vocabularies:

| Field | State | Meaning |
|---|---|---|
| `coverage_status` | `TEST_MISSING` | No test evidence was found for the required behavior. |
| `coverage_status` | `PARTIAL` | Existing tests cover only part of the stated contract. |
| `coverage_status` | `REGISTERED_UNVERIFIED` | A test and registration were found, but execution has not been verified for the pinned build profile. |
| `coverage_status` | `BLOCKED` | Test-to-criterion coverage cannot be assessed until a profile/scope prerequisite is resolved; this is not a runtime failure. |
| `coverage_status` | `NOT_APPLICABLE` | Excluded from this profile with a reviewed rationale and explicit disposition. |
| `execution_status` | `NOT_RUN` | No execution result is attached; this is the initial state for this catalog. |
| `execution_status` | `PASS` / `FAIL` | Reserved for a captured, reproducible execution result. |
| `execution_status` | `SKIPPED` | A registered case reported a skip; required skipped cases do not count as passing evidence. |
| `execution_status` | `BLOCKED` | A mapped test could not run because a prerequisite such as a model, dataset, or hardware lane was unavailable. |
| `execution_status` | `NOT_APPLICABLE` | Excluded from this profile with a reviewed rationale; it is not an implicit success. |

`BLOCKED` in `coverage_status` means the coverage mapping itself is unresolved;
`BLOCKED` in `execution_status` means execution could not proceed. They are
not interchangeable. Every criterion record uses the same JSON fields.
`threshold` is `null` until a numeric value is approved;
`threshold_approval` is `not_applicable` when no numeric threshold applies and
`pending_owner_approval` when one is needed; `blocked_reasons` is an empty
array when no execution blocker is known.

The catalog records static test-coverage state separately from runtime
execution state. A criterion can have existing tests and still be unverified.
Empty suites, absent targets, build failures, assertion failures, skips, and
not-applicable decisions must remain distinguishable in collected results.

## Result and readiness rules

1. Define the criterion and its test obligations before implementing the
   corresponding feature. Add executable tests against the intended contract
   first when the public behavior exists; when it does not, retain the
   obligation as `TEST_MISSING` rather than adding a production stub or a
   placeholder `FAIL()` test.
2. Keep the profile readiness lane separate from ordinary regression CI.
   Missing future-feature evidence blocks this profile's readiness result, not
   unrelated regression runs.
3. Build required test targets before CTest discovery/execution. Capture
   CTest discovery JSON, CTest JUnit, and GoogleTest XML (or equivalent
   per-case output) so zero executed tests and internal skips cannot be hidden
   by a successful wrapper exit.
4. Every result must identify source revision, build profile/options, test
   target and cases, environment/model artifact identity, execution status,
   and retained raw output. Benchmarks additionally require the workload,
   corpus, hardware, sample count, warm-up, and measurement method.
5. A profile result is blocked if a required criterion has no valid evidence.
   Do not calculate a weighted percentage: one criterion with many tests must
   not outweigh a missing security or correctness contract.
6. Where a declared threshold has no representative baseline or owner
   approval, report the measurement and mark the gate `BLOCKED`; do not
   silently turn the target into a measured pass.

## Current inventory boundary

The catalog maps existing source and CMake registration patterns. It does not
assert that every target is enabled in every build profile. The profile's
first implementation increment must resolve configuration-specific
registration, run the focused targets, and attach real results before any
criterion can become `PASS`.

The strongest currently visible focused coverage is in
`tests/rag/test_rag_provider.cpp` (source materialization, policy isolation,
budget and cooperative deadlines). This does not replace HTTP-route,
representative-dataset, or end-to-end evidence. Existing LoRA and tensor
fixtures include mock/stub or injected paths; they do not establish production
llama.cpp binding or numerical tensor round trips.

## Open approvals

Before calling this a binding gate, maintainers must ratify:

- The exact profile and milestone/release use.
- Required versus optional criteria and any justified `NOT_APPLICABLE`
  dispositions.
- Representative data/model artifacts and their privacy/licensing constraints.
- RAG quality and end-to-end latency/resource thresholds after baseline runs.
- CPU/GPU lanes and supported model/tokenizer combinations.
- Evidence retention and artifact location for the readiness result.

Until then, this catalog is a reviewable proposal and all runtime states remain
unmeasured.
