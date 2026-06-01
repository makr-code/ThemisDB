# retrieval/include documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

This directory defines the EPIC 1 public contracts used to stage implementation,
review dependency order, and prepare test/benchmark handoff before production logic lands.

## Header Ownership Map

| Header | EPIC sub-issue | Contract focus |
|---|---|---|
| `ann_frontdoor.h` | 1.1 | ANN routing, query/result envelopes, backend policy knobs |
| `tensor_midlayer.h` | 1.2 | tensor summary exchange and compression-aware request/response contracts |
| `graph_validator.h` | 1.3 | graph evidence validation contracts and confidence model |
| `lora_package.h` | 1.4 | LoRA adapter packaging/provenance metadata interfaces |
| `model_switch.h` | 1.5 | model version switch policy, compatibility checks, switch outcomes |
| `federated_summaries.h` | 1.6 | federated summary query/result contracts and operating modes |
| `retrieval_observability.h` | 1.7 | retrieval trace and governance decision interfaces |

## Entry Criteria for Contract Changes

Before changing these headers:
- confirm the owning EPIC document reflects the same terminology
- keep cross-epic dependencies consistent with `docs/EPIC1_2_3_DEPENDENCIES.md`
- define expected test ownership for the change (`tests/epic1_retrieval/`)

## Exit Criteria for Phase-2 to Phase-3 Transition

- each interface has explicit error-path semantics (status, fallback, retry expectations)
- each request/result type has validation and bounds assumptions documented
- each externally consumed contract has at least one planned contract test scenario

## Installation

No separate installation step is required for header contracts in scaffold mode.

## Usage

Use this README as the header ownership and review checklist before contract changes.

## References

- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC1_ANN_FRONTDOOR.md`
- `docs/EPIC1_TENSOR_MIDLAYER.md`
- `docs/EPIC1_GRAPH_VALIDATION.md`
- `docs/EPIC1_LORA_ARTIFACTS.md`
- `docs/EPIC1_MODEL_SWITCH.md`
- `docs/EPIC1_FEDERATED_SUMMARIES.md`
