# Evaluation Module Planning Scaffold

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Purpose

Landing zone for EPIC 2 evaluation, hardware profile, lifecycle, and planner contracts.

## Planned public headers

- `include/hardware_profile.h`
- `include/benchmark_matrix.h`
- `include/evaluation_metrics.h`
- `include/approximation_rules.h`
- `include/query_planner.h`
- `include/artifact_lifecycle.h`
- `include/storage_strategy.h`

## Planned implementation files

- `src/hardware_profile.cc`
- `src/benchmark_matrix.cc`
- `src/evaluation_metrics.cc`
- `src/approximation_rules.cc`
- `src/query_planner.cc`
- `src/artifact_lifecycle.cc`
- `src/storage_strategy.cc`

## References

- `docs/EPIC2_ARCHITECTURE.md`
- `docs/EPIC2_HARDWARE_PROFILES.md`
- `docs/EPIC2_QUERY_PLANNER.md`

## Seven-phase checkpoint

- [ ] Design and API contracts documented
- [ ] Skeleton headers and sources approved for creation
- [ ] Error handling and test expectations documented
- [ ] Benchmarks and integration points reserved before code lands

## Installation

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
