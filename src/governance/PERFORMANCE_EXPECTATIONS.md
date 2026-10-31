---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
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

# Governance Performance Expectations

## Benchmark source and registration

The policy-latency source is
[`benchmarks/governance/bench_governance_policy_latency.cpp`](../../benchmarks/governance/bench_governance_policy_latency.cpp);
its target is registered in
[`benchmarks/governance/CMakeLists.txt`](../../benchmarks/governance/CMakeLists.txt).
Benchmark building is optional and requires `THEMIS_BUILD_BENCHMARKS=ON`.

| Case | Path |
|---|---|
| `BM_Evaluate_NoYAML_Offen`, `BM_Evaluate_NoYAML_Geheim` | `PolicyEngine::evaluate()` without loaded YAML |
| `BM_Evaluate_YAML_AllClassifications`, `BM_Evaluate_ResourceMappingRoute` | YAML/configuration-backed evaluation cases |
| `BM_Evaluate_CCPA_OptedOut`, `BM_Evaluate_CCPA_NotOptedOut` | CCPA opt-out state |
| `BM_CheckQueryPermission_NoYAML`, `BM_CheckQueryPermission_WithMaskingRules`, `BM_CheckQueryPermission_StrictClassification` | query permission result path |
| `BM_CheckQueryPermission_HighVolume` | query-permission batch sizes 1, 10, 100, 1,000 |
| `BM_Evaluate_Throughput` | evaluation throughput |

The benchmark source comments state a p99 target below 0.5 ms for `evaluate()`
and `checkQueryPermission()`. This is a source comment/expectation, not evidence
of a measured result or a confirmed repository-wide release gate. Older tables
in this file listed different cases and thresholds and are superseded.

## Measurement limits

- The YAML-backed benchmark calls `loadFromYAML()` best-effort and does not
  assert success. Some runs may therefore measure fallback behavior rather than
  the configured profiles.
- The source documents `config/governance.yaml` as its default path and attempts
  a `GOVERNANCE_YAML_PATH` override; the current helper does not reliably return
  the environment value. Verify the source and test configuration before
  treating a run as YAML-backed.
- The benchmark measures selected module calls, not end-to-end authorization,
  audit persistence, compliance-report generation, or caller-side masking.
- No benchmark result, release baseline, hardware specification, or current CI
  pass is asserted by this document.

Record benchmark binary, exact case/filter, build profile, effective YAML,
hardware, and baseline when comparing results. See
[`benchmarks/governance/README.md`](../../benchmarks/governance/README.md).
