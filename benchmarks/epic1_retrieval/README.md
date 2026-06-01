# EPIC 1 retrieval benchmarks

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Planned benchmark files

- `ann_retrieval_bench.cc`
- `tensor_routing_bench.cc`
- `graph_validation_bench.cc`
- `lora_loading_bench.cc`
- `federated_bench.cc`

## Measurement goals

- Capture latency, throughput, and resource cost for the matching sub-issues
- Keep hardware profile, dataset, and planner assumptions explicit
- Store enough metadata to compare approximation strategies across runs

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`

## Installation

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
