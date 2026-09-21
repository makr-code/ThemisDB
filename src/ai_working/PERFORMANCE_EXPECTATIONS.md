# PERFORMANCE_EXPECTATIONS — ai_working Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ROADMAP.md -->

## Scope

- Module: `src/ai_working`
- This module contains no C++ runtime code and has no production performance-critical paths.

## Performance Requirements

Not applicable. The `ai_working` module is a documentation-only artifact container. No latency, throughput, or memory targets apply.

## Future Benchmark Scope

If utility C++ code is introduced in a future roadmap phase (e.g., artifact indexing), performance expectations will be defined at that time and tracked in this document.

| Placeholder Target | Expectation | Benchmark Case |
|---|---|---|
| Archival script execution | < 10 seconds for typical repository size | N/A (script, not C++) |

## Module Hard Gates

None. No benchmark regression gates apply to this module.

## Validation

No benchmark run required for this module in current state.
