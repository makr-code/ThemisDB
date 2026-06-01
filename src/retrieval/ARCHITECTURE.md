# Retrieval Module Architecture

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

`src/retrieval` provides the EPIC 1 retrieval contract layer with seven sub-surfaces:
ANN frontdoor, tensor mid-layer, graph validation, LoRA packaging, model switching,
federated summaries, and retrieval observability.

At current scope, architecture is contract-first: public headers in `include/` define
types/interfaces and matching translation units in `src/` keep scaffold ownership clear.

## Component Map

| Component | Contract | Implementation file |
|---|---|---|
| ANN frontdoor | `include/ann_frontdoor.h` | `src/ann_frontdoor.cc` |
| Tensor mid-layer | `include/tensor_midlayer.h` | `src/tensor_midlayer.cc` |
| Graph validation | `include/graph_validator.h` | `src/graph_validator.cc` |
| LoRA package | `include/lora_package.h` | `src/lora_package.cc` |
| Model switch | `include/model_switch.h` | `src/model_switch.cc` |
| Federated summaries | `include/federated_summaries.h` | `src/federated_summaries.cc` |
| Retrieval observability | `include/retrieval_observability.h` | `src/retrieval_observability.cc` |

## Boundaries

In scope:
- EPIC 1 retrieval contracts and ownership boundaries
- cross-epic integration seams with EPIC 2 evaluation and EPIC 3 distributed artifacts
- phase-gated implementation planning references

Out of scope at scaffold stage:
- production retrieval algorithm behavior claims
- benchmark-backed SLO/SLA claims
- default enablement in product build/test pipelines

## Integration Surfaces

- Planning: `docs/EPIC1_ARCHITECTURE.md` and per-sub-issue EPIC 1 design docs
- Contracts: `src/retrieval/include/*.h`
- Implementation scaffolds: `src/retrieval/src/*.cc`
- Cross-epic dependency sequencing: `docs/EPIC1_2_3_DEPENDENCIES.md`
