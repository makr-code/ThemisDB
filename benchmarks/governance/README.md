---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
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

# Governance Benchmarks

The benchmark sources live in `benchmarks/governance/`; this is not a mirrored
copy of `src/governance/`. Benchmark targets are optional and require
`THEMIS_BUILD_BENCHMARKS=ON`.

## Files and registration

| Source | Registration / focus |
|---|---|
| `bench_governance_policy_latency.cpp` | `bench_governance_policy_latency`, registered by this directory's `CMakeLists.txt`; policy evaluation and query permission cases |
| `bench_governance_release_gates.cpp` | `bench_governance_release_gates`, registered by this directory's `CMakeLists.txt`; governance contract/result microbenchmarks |
| `bench_policy_evaluation.cpp` | Also present in this folder; top-level `benchmarks/CMakeLists.txt` handles registration |
| `bench_audit_export_high_volume.cpp` | Also present in this folder; check top-level `benchmarks/CMakeLists.txt` for registration |

[`../../src/governance/PERFORMANCE_EXPECTATIONS.md`](../../src/governance/PERFORMANCE_EXPECTATIONS.md)
maps selected policy-latency case names. A source registration or benchmark
target is not evidence that a benchmark was run or that a performance threshold
passed.

## Running

Configure an existing repository build with benchmarks enabled, build the
specific target, then invoke its generated executable with Google Benchmark
options (for example `--benchmark_list_tests=true`). For the policy-latency
target, the target-specific build step is:

```bash
cmake --build <build-dir> --target bench_governance_policy_latency
```

Invoke the resulting `bench_governance_policy_latency` executable from the
configured build tree; its exact path depends on the selected generator and
build directory. There is no governance-specific `ENABLE_GOVERNANCE` option or
portable preset command established by this module.

The policy-latency source uses `GOVERNANCE_YAML_PATH` when set and otherwise
uses `config/governance.yaml`. A failed YAML load is ignored in several benchmark
cases, so a run may measure fallback behavior rather than loaded YAML profiles;
verify the selected file and benchmark setup when comparing results. This
benchmark source does not currently have a dedicated C++ test target for its
path helper.
