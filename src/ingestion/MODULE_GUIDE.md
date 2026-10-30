---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Module Guide

## Audience and scope

Use this guide to locate the ingestion implementation, public contracts, focused
tests, and performance evidence. It is a developer onboarding page, not a
replacement for the normative interface comments or operational configuration.

## Repository map

| Path | Responsibility |
|---|---|
| [`src/ingestion/`](./) | Connector implementations, manager/coordinator, workflow and validation logic. |
| [`include/ingestion/`](../../include/ingestion/) | Public C++ interfaces and contract comments. |
| [`tests/ingestion/`](../../tests/ingestion/) | Focused module tests; target names are generated from `test_*.cpp`. |
| [`tests/integration/test_ingestion_pipeline_soak.cpp`](../../tests/integration/test_ingestion_pipeline_soak.cpp) | Integration soak coverage for the ingestion pipeline. |
| [`benchmarks/ingestion/`](../../benchmarks/ingestion/) | Hot-path, extraction, and judge benchmarks. |
| [`docs/operability/RUNBOOK_INGESTION_PIPELINE.md`](../../docs/operability/RUNBOOK_INGESTION_PIPELINE.md) | Existing cross-module operator runbook; verify deployment-specific instructions before use. |

## Read by task

- Runtime boundaries and failures: [`ARCHITECTURE.md`](./ARCHITECTURE.md).
- Source-level execution and invariants: [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).
- Public C++ contracts: [`API.md`](./API.md).
- Extraction and evaluation assumptions: [`RESEARCH.md`](./RESEARCH.md) and [`MODELING.md`](./MODELING.md).
- Vector/model version changes: [`EMBEDDING.md`](./EMBEDDING.md).
- Diagnosis and maintenance: [`RUNBOOK.md`](./RUNBOOK.md) and [`OPERATIONS.md`](./OPERATIONS.md).
- Trust and security boundaries: [`TRUST.md`](./TRUST.md) and [`SECURITY.md`](./SECURITY.md).
- Planning and benchmark thresholds: [`ROADMAP.md`](./ROADMAP.md) and [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

## Build and test

Configure with tests enabled before building or running module tests. In
particular, `linux-release` sets `THEMIS_BUILD_TESTS=OFF` and therefore does not
provide ingestion test targets:

```bash
cmake -S . -B build-ingestion-tests -DTHEMIS_BUILD_TESTS=ON
cmake --build build-ingestion-tests --target module_ingestion_test_ingestion_contract_hardening_focused
ctest --test-dir build-ingestion-tests -L ingestion --output-on-failure
```

The module test CMake file creates one focused target per `test_*.cpp`, with
`ingestion` labels. It currently skips `test_ingestion_error_contract_focused`
because that suite targets a superseded API surface; do not treat that skipped
suite as passing coverage. For a narrow change, build/run the relevant target
from the configured build tree and confirm its CTest registration with
`ctest --test-dir <build-dir> -N -L ingestion`.

Benchmarks are not unit tests. Use
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) to choose the
mapped case and distinguish mock/stub results from live backend measurements.

## Safe integration sequence

1. Configure a source and finite bounds for remote or untrusted inputs.
2. Use `previewSource()` or dry-run to inspect extraction before writing.
3. Register schema validation and a workflow profile where required.
4. Confirm the selected sink (if the execution path uses one), target collection,
   checkpoint policy, and inference backend. For the filesystem workflow manager
   path, check [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) before treating a
   processed count as persisted output.
5. Run a focused fixture and inspect the complete report, errors, and
   quarantine entries before enabling production traffic.

> Exact model backend not verifiable from final document text alone.
