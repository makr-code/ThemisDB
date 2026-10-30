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

# Governance Research and Evaluation

## Purpose and scope

This page records how to evaluate governance behavior from repository evidence. It is for maintainers comparing policy-engine, compliance, masking, lineage, and integration changes. It does not claim that a passing software test proves legal or regulatory compliance.

The unit under study is the governance runtime in `src/governance/`: given a loaded policy and request context, it produces policy decisions, compliance evidence, masking results, lineage records, or structured failures. Authentication, storage implementation, and downstream query/model execution remain owned by their respective modules.

## Questions and assumptions

| Question | Observable evidence | Assumption to verify |
|---|---|---|
| Does policy evaluation remain deterministic for a fixed input and policy snapshot? | `PolicyDecision` fields and policy-engine regression tests | Callers provide stable request headers and use the returned decision. |
| Does degraded OPA evaluation reach the documented native path? | `std::nullopt`, `governance_opa_fallback_total`, and OPA adapter tests | The configured evaluator is the one actually attached to `PolicyEngine`. |
| Are sensitive-data controls applied to every relevant output? | Masking results, caller integration tests, and `ann_allowed`/export decisions | A module decision only protects paths that consume it. |
| Can operators reconstruct data use and policy changes? | Audit events, policy versions, and lineage event chains | Audit sinks are configured and their retained evidence is available. |
| Does the hot path meet its release budget? | Mapped benchmark cases and release-baseline comparisons | Measurements use the documented build profile and comparable hardware. |

## Evaluation method

1. Select the owning contract and input state: policy YAML plus headers/route for `PolicyEngine`; policy rules for compliance checks; JSON plus enabled rules for `DataMasker`; or an event sequence and audit sink for lineage.
2. Exercise expected, denied, malformed, missing-context, and degraded-integration cases. Include unknown classifications, disabled/empty masking policies, duplicate field rules, OPA `nullopt`, and partial subject-erasure failures where applicable.
3. Compare structured outcomes, not log wording alone. Check policy fields, error/result codes, per-store reports, lineage circuit state, and emitted diagnostics.
4. Run focused regression suites before interpreting benchmark movement. Use the existing benchmark map in `PERFORMANCE_EXPECTATIONS.md` for evaluation and query-permission hot paths.
5. Record whether the result came from native evaluation, REST OPA, an injected WASM evaluator, or the built-in WASM stub. These paths are not interchangeable.

## Existing evidence and measurements

The implementation has focused coverage for OPA/native fallback, policy simulation, lineage backpressure and size limits, compliance reporting time windows, policy lifecycle, and masking/model-governance behavior. See the [module guide](MODULE_GUIDE.md) for the test source map.

Performance is evaluated by the mapped cases in `benchmarks/bench_governance_policy_latency.cpp`. The current module expectation is no more than a 10% regression against the release baseline, a p99 governance hot-path latency within the configured release threshold, and complete mapped-case coverage. Those are release-gate criteria; this page does not claim a new measurement or a universal latency guarantee.

## Interpretation limits

- Compliance and bias reports summarize the configured rules, supplied data, and available evidence; they are not independent legal determinations.
- A green unit test does not establish that all production callers apply masking, export restrictions, or `ann_allowed`.
- Native fallback can be deterministic without being equivalent to the unavailable OPA policy.
- In-memory lineage limits and audit-forwarding behavior affect the evidence window; retained events can be FIFO-evicted.
- Benchmarks are meaningful only with their exact build, hardware, workload, and baseline recorded.

## Source references

- [Policy engine](../../include/governance/policy_engine.h) and `policy_engine.cpp`
- [OPA adapter](../../include/governance/opa_adapter.h) and `opa_adapter.cpp`
- [Lineage tracker](../../include/governance/data_lineage.h) and `data_lineage.cpp`
- [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
- [Audit report](AUDIT.md) and [roadmap](ROADMAP.md)
