# EPIC 1 retrieval benchmarks

<!-- Status: current | partial implementation | validated: 2026-07-06 -->

## Implemented benchmark files

- `bench_ann_distance_cpu_vs_flat.cpp` (Phase B benchmark gate)
- `bench_multishard_exact.cpp` (Phase C benchmark gate)

## Planned benchmark files

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

This directory now contains initial rollout-gate benchmark implementations.
Additional benchmarks remain planned and are staged incrementally.

## Usage

Build with benchmark support enabled and execute:

- `bench_ann_distance_cpu_vs_flat`
- `bench_multishard_exact`

Use this README together with the matching epic document to track remaining benchmark coverage.
