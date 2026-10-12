# ThemisDB Vector Search Module

<!-- Status: current | implementation lives under src/index and include/index | validated: 2026-10-12 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

This module documents the operational contract and release-readiness evidence for ThemisDB vector-search behavior. The implementation itself is not colocated under `src/vector_search/`; it is owned by the shared index subsystem under `src/index/` and `include/index/`.

This directory exists as the governance-facing summary for the module: it describes the runtime contract, validation evidence, and known gaps without duplicating the live source tree.

## Implementation Footprint

The relevant live code and evidence are here:

- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/multi_vector_search.cpp`
- `include/index/multi_vector_search.h`
- `tests/integration/test_vector_search_soak.cpp`
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`

This module should be treated as the documentation and release boundary for vector search, not as an alternate implementation tree.

## Scope

In scope:
- vector search contract and module governance
- ANN/index query expectations for retrieval workflows
- operational limits and release gates
- known gaps, tuning, and future work planning

Out of scope:
- creating a shadow implementation under `src/vector_search/`
- duplicating index logic that lives in the shared `index` subsystem
- undocumented "future" behavior that is not traceable to the live source tree

## Runtime Behavior and Limits

- behavior is driven by the active backend and configuration in the shared index module
- results are ordered by similarity or distance according to the selected metric and backend
- performance and correctness expectations are validated through the existing benchmark and soak tests

## Governance and Validation References

- **Architecture:** [ARCHITECTURE.md](ARCHITECTURE.md)
- **Implementation phases and status:** [ROADMAP.md](ROADMAP.md)
- **Planned enhancements:** [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- **Audit summary:** [AUDIT.md](AUDIT.md)
- **Security notes:** [SECURITY.md](SECURITY.md)
- **Operational requirements:** [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- **Performance objectives:** [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- **Known gaps:** [MODULE_GAPS.md](MODULE_GAPS.md)
- **Change history:** [CHANGELOG.md](CHANGELOG.md)
