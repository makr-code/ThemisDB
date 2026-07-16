# PERFORMANCE_EXPECTATIONS - src/governance

## Scope

- Module: src/governance
- This file defines measurable governance module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_governance_policy_latency.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| GOVP-1 | evaluate path remains bounded across no-YAML, YAML, and route-mapped scenarios | BM_Evaluate_NoYAML_Offen, BM_Evaluate_NoYAML_Geheim, BM_Evaluate_YAML_AllClassifications, BM_Evaluate_ResourceMappingRoute |
| GOVP-2 | CCPA opted-out/non-opted-out evaluation paths remain bounded | BM_Evaluate_CCPA_OptedOut, BM_Evaluate_CCPA_NotOptedOut |
| GOVP-3 | query permission paths remain bounded with masking and strict classes | BM_CheckQueryPermission_NoYAML, BM_CheckQueryPermission_WithMaskingRules, BM_CheckQueryPermission_StrictClassification |
| GOVP-4 | high-volume permission checks remain stable under benchmark load scaling | BM_CheckQueryPermission_HighVolume |
| GOVP-5 | evaluate throughput path remains bounded | BM_Evaluate_Throughput |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GOVG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| GOVG-2 | governance hot-path p99 <= release threshold | p99 from mapped governance benchmark cases |
| GOVG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional governance benchmark scenarios are introduced.

## Sourcecode Verification (Module: governance/performance)

- Verified benchmark sources:
  - benchmarks/bench_governance_policy_latency.cpp
- Verified mapping surfaces:
  - evaluate path variants and throughput benchmark cases
  - CCPA path benchmark cases
  - query permission/no-yaml/masking/strict/high-volume benchmark cases
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.