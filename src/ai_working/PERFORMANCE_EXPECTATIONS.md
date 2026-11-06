# PERFORMANCE_EXPECTATIONS — ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Scope

- Module: `src/ai_working`
- This module contains no C++ runtime code and has no production performance-critical paths.

## Performance Requirements

Not applicable. The `ai_working` module is a documentation-only artifact container. No latency, throughput, or memory targets apply.

## Future Benchmark Scope

There is no current benchmark target or measured baseline. If the planned artifact index or other utility is approved, measure the governance check and compactor against fixed 1,000- and 10,000-artifact corpora. Record corpus file sizes and total bytes, runner, 20 repeated runs, p50/p95 elapsed time, and peak memory before setting an absolute latency budget.

| Target | Expectation | Benchmark Case |
|---|---|---|
| Compaction and artifact discovery | Proposed v3.0.0 gate: p95 elapsed time no more than 15% above the same-runner baseline; not yet measured | Fixed 1,000- and 10,000-artifact corpora; publish environment and raw results |

## Module Hard Gates

None. No benchmark regression gates apply to this module.

## Validation

No module benchmark target exists, so no benchmark run is currently available. The proposed relative regression threshold is not a measured result or gate until a reproducible baseline has been captured and reviewed. See `ROADMAP.md` for the staged benchmark plan.
