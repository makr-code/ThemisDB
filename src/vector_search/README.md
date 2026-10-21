---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Vector Search Module

## Purpose

This module documents the governance-facing contract for ThemisDB vector search. The live implementation is not maintained under `src/vector_search/`; the canonical code lives in the shared index subsystem under `src/index/` and `include/index/`.

The directory therefore serves as the module boundary for release evidence, operational expectations, and documented limits rather than as a duplicate implementation tree.

## Scope

In scope:
- vector-search governance and contract boundaries
- release evidence and benchmark gate references
- operational limits and known gaps
- alignment to the live shared-index implementation

Out of scope:
- duplicating vector-search implementation in `src/vector_search/`
- claiming future capabilities as deployed features without source evidence
- shadowing subsystem ownership in `src/index/` and `include/index/`

## Quickstart (Build/Run)

```bash
# list available presets and select the one that matches your platform
cmake --list-presets

# example: a platform-matching preset such as linux-release or windows-release
cmake --preset <your-preset>
cmake --build --preset <your-preset> --target test_vector_search_highcardinality_stress

# focused validation
ctest --preset <your-preset> -R 'vector_search' --output-on-failure
```

The repository's live artifacts for this module are the shared index sources, the validation test in `tests/vector_search/`, and the release benchmark in `benchmarks/vector_search/`.

## API / CLI Entry Points

- Canonical implementation: `include/index/vector_index.h`, `include/index/multi_vector_search.h`
- Shared runtime sources: `src/index/vector_index.cpp`, `src/index/advanced_vector_index.cpp`, `src/index/multi_vector_search.cpp`
- Stress coverage: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- Benchmark gate: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`

This module should be read as the governance and evidence layer for those real APIs, not as a second code path.

## Integration Overview

```mermaid
flowchart LR
    A[Client / Retrieval / Service] --> B[vector search contract]
    B --> C[shared index subsystem]
    C --> D[metric validation + search execution]
    D --> E[tests and benchmark evidence]
    E --> F[release readiness review]
```

Interpretation: the vector search module coordinates governance and validation around the shared index subsystem. Runtime behavior, performance gates, and API contracts are still owned by the index implementation under `src/index/` and `include/index/`.

## Known Limitations

- The module does not contain a separate implementation tree; it documents the contract around the shared index subsystem.
- Distributed indexing and persistent sharding remain planned work rather than current capabilities.
- Performance and reliability expectations must be validated against the real benchmark/test artifacts and not inferred from historical documentation alone.

## References

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [ROADMAP.md](ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- [AUDIT.md](AUDIT.md)
- [SECURITY.md](SECURITY.md)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- [MODULE_GAPS.md](MODULE_GAPS.md)
- [CHANGELOG.md](CHANGELOG.md)
