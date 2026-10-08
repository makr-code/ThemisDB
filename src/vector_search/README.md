# ThemisDB Vector Search Module

<!-- Status: DOCS_FACADE | live implementation externalized to index/ | validated: 2026-10-08 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The vector search module is the governance-facing documentation and release-contract layer for ThemisDB's similarity-search capabilities. The active implementation lives in the broader index subsystem rather than in this directory itself.

The runtime behavior of vector search is driven by the index stack under `src/index/`, `include/index/`, and the repository's test/benchmark artifacts. This module exists to keep the feature's contract, quality gates, and operational expectations readable and reviewable without duplicating the implementation surface.

## Live Implementation Footprint

The implementation is currently located in the shared index namespace rather than under `src/vector_search/` as colocated source files:

- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/ann_index.cpp`
- `src/index/multi_vector_search.cpp`
- `src/index/distributed_vector_index.cpp`
- `include/index/` and related headers for the public vector and ANN interfaces

This directory remains the canonical operational summary for the module, while the live code continues to be owned by the broader index subsystem.

## Scope

In scope:
- vector search contracts, acceptance criteria, and module governance
- ANN and index lifecycle expectations relevant to ThemisDB retrieval
- query correctness, latency, and reliability gates
- module-level risk tracking and future work planning

Out of scope:
- colocated implementation in `src/vector_search/*.cpp` or `include/vector_search/*.h`
- feature ownership that is intentionally split into the broader `index/` subsystem
- duplicate implementation details that would drift from the canonical source tree

## Runtime Behavior and Limits

- behavior depends on the active backend and index configuration in the shared `index` module
- search results are ranked by similarity or distance metric according to the selected algorithm
- production gates remain defined by benchmark and soak-test artifacts outside this directory

## Governance and Validation References

- **Implementation Phases & Status:** [ROADMAP.md](ROADMAP.md)
- **Delivered Artefacts & History:** [CHANGELOG.md](CHANGELOG.md)
- **Future Planning:** [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- **Architecture & Design:** [ARCHITECTURE.md](ARCHITECTURE.md)
- **Module Audit & Compliance:** [AUDIT.md](AUDIT.md)
- **Security & Threat Model:** [SECURITY.md](SECURITY.md)
- **Operational Constraints:** [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- **Performance Baselines:** [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- **Known Gaps & Limitations:** [MODULE_GAPS.md](MODULE_GAPS.md)
