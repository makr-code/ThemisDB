---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "include/storage/examples/schema_layout_advisor_example.cpp"
  - "include/storage/storage_engine.h"
  - "include/storage/schema_dead_weight_detector.h"
  - "include/storage/storage_layout_advisor.h"
  - "include/storage/ARCHITECTURE.md"
  - "include/storage/ROADMAP.md"
  - "src/storage/ROADMAP.md"
  - "CMakePresets.json"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

> Exact model backend not verifiable from final document text alone.

# Storage Module — Examples

This directory contains a specification-style example for planned Paper 2
Layer 6 (`SchemaDeadWeightDetector`) and Layer 10 (`StorageLayoutAdvisor`)
patterns. It is not evidence that these APIs or behaviors are implemented.

## Contents

| File | Paper | Issue | Status |
|---|---|---|---|
| `schema_layout_advisor_example.cpp` | Paper 2 §Layer 6 + 10 | IMPL-B6, IMPL-B10 | Specification / planned API; not a current implementation example |

## `schema_layout_advisor_example.cpp`

The source simulates field-access statistics and prints illustrative expected
recommendations. The `SchemaDeadWeightDetector` and `StorageLayoutAdvisor` calls
are commented-out planned code; the GDPR/seasonal assertions, layout advice,
compression estimate, and `AIDecisionAuditor` records are not executed
implementation behavior. Do not rely on its printed expectations as verified
results.

## Build preset

The repository defines the `linux-release` preset. On Linux with the required
vcpkg setup, the project release configure/build commands are:

```sh
cmake --preset linux-release
cmake --build --preset linux-release
```

This preset requires Linux, GCC 11+, Ninja, and a bootstrapped vcpkg checkout.
It disables tests and benchmarks by default. These commands build the project;
they do not build or validate this specification-only example. No dedicated
CMake target for this file was found in the inspected project CMake sources.
See [`CMakePresets.json`](../../../CMakePresets.json) and the storage
[public-header guide](../README.md) for current project setup and API context.

## Related documentation

- Issue specifications: `docs/issues/optimization_layers/IMPL-B6-schema-deadweight.md`
  and `docs/issues/optimization_layers/IMPL-B10-layout-advisor.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md`
  (§Layer 6, §Layer 10)
- Header navigation: [`../README.md`](../README.md)
- Header roadmap: [`../ROADMAP.md`](../ROADMAP.md)
- Canonical storage roadmap: [`../../../src/storage/ROADMAP.md`](../../../src/storage/ROADMAP.md)
