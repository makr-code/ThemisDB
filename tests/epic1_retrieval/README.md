# EPIC 1 retrieval tests

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Planned test files

- `ann_frontdoor_test.cc`
- `tensor_midlayer_test.cc`
- `graph_validator_test.cc`
- `lora_package_test.cc`
- `model_switch_test.cc`
- `federated_summaries_test.cc`
- `observability_test.cc`

## Coverage goals

- Contract tests first
- Integration scenarios once public interfaces stabilize
- Benchmark comparisons live in the matching `benchmarks/` epic directory

## Reference

- `docs/TESTING_STRATEGY.md`

## Installation

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
