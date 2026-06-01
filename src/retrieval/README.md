# Retrieval Module Planning Scaffold

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Purpose

Landing zone for EPIC 1 hybrid retrieval contracts and future implementation files.

## Planned public headers

- `include/ann_frontdoor.h`
- `include/tensor_midlayer.h`
- `include/graph_validator.h`
- `include/lora_package.h`
- `include/model_switch.h`
- `include/federated_summaries.h`
- `include/retrieval_observability.h`

## Planned implementation files

- `src/ann_frontdoor.cc`
- `src/tensor_midlayer.cc`
- `src/graph_validator.cc`
- `src/lora_package.cc`
- `src/model_switch.cc`
- `src/federated_summaries.cc`
- `src/retrieval_observability.cc`

## References

- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC1_ANN_FRONTDOOR.md`
- `docs/EPIC1_FEDERATED_SUMMARIES.md`

## Seven-phase checkpoint

- [ ] Design and API contracts documented
- [ ] Skeleton headers and sources approved for creation
- [ ] Error handling and test expectations documented
- [ ] Benchmarks and integration points reserved before code lands

## Installation

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
