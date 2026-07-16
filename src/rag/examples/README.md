> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# RAG Module — Examples

Examples for the `rag` module demonstrating Paper 1 (Loop Orchestration) and Paper 2 Layer 9 (Explainability) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `loop_orchestration_example.cpp` | Paper 1 §4.4 + Paper 2 L9 | IMPL-A2, IMPL-A3, IMPL-B9 | Specification / planned API |

## loop_orchestration_example.cpp

Demonstrates:

1. **Loop 1** — `triggerLoop(LoopPhase::LOOP_1_HNSW_QUERY)`: per-query HNSW optimizer feedback
2. **Loop 2** — `triggerLoop(LoopPhase::LOOP_2_WORKLOAD)`: WorkloadAdaptiveOptimizer + HNSW (60 s interval)
3. **Loop 4** — `triggerLoop(LoopPhase::LOOP_4_RLAIF)`: `IncrementalLoRATrainer` weekly cycle
4. **FEDERATED_ROUND_START** (IMPL-A3) — `TriggerEvent::FEDERATED_ROUND_START` fires automatically after a successful Loop-4 with `guardrail_passed == true`; requires `setFederationCoordinator()` and `setTrainerForFederation()` injected
5. **ExplainabilityReasonBuilder** (IMPL-B9) — generates `CausalChain` in natural language; writes `DecisionRecord` to `AIDecisionAuditor`

> **Note:** The example uses the actual API `triggerLoop(LoopPhase)` and `registerLoopCompletionHandler()`. Calls to still-planned IMPL-B9 APIs are marked with `/* PLANNED */` comments.

## Related Documentation

- Issue spec: `docs/issues/lora_loops/IMPL-A2-loop-orchestration.md`
- Issue spec: `docs/issues/lora_loops/IMPL-A3-federation-hooks.md`
- Issue spec: `docs/issues/optimization_layers/IMPL-B9-explainability.md`
- Research paper: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
- Module ROADMAP: `include/rag/ROADMAP.md` §Phase 9

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/rag/README.md`](../../include/rag/README.md) for the public API.
