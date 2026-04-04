<!-- Status: current | validated: 2026-03-22 -->

# Roadmap — include/training/

**Latest released version:** v1.5.0 (2026-03-12) | **Active headers:** 10

---

## Current Status

API stable — no breaking changes planned before v2.0.0.

---

## Completed ✅

- [x] Abstract training contracts (`training_interfaces.h`)
- [x] End-to-end pipeline orchestration (`training_pipeline.h`)
- [x] LoRA adapter value type with rank/alpha config (`lora_adapter.h`)
- [x] Incremental/online LoRA training (`incremental_lora_trainer.h`)
- [x] Atomic checkpoint save/load with versioned metadata (`lora_checkpoint_manager.h`)
- [x] Active-learning and uncertainty-sampling data selection (`lora_data_selection.h`)
- [x] Automatic label derivation from ThemisDB (`auto_labeler.h`)
- [x] Multimodal input normalisation (`modality_parser.h`)
- [x] Knowledge-graph sample enrichment (`knowledge_graph_enricher.h`)
- [x] Data lineage and provenance tracking (`provenance_tracker.h`)

---

## Planned Features

### Q3 2026

- [ ] **Distributed LoRA training coordinator** — `distributed_lora_trainer.h` (Target: Q3 2026)
  - Inputs: `IDataSource` shards + `LoRAConfig` + cluster topology config
  - Outputs: merged `LoRAAdapter` (all-reduce of rank matrices)
  - Constraints: fault-tolerant; worker failure triggers checkpoint + re-shard
  - Errors: quorum loss, network partition, shard checksum mismatch
  - Tests: unit (mock cluster) + integration (3-node local MPI) + chaos testing
  - Perf: ≥ 0.85 linear scaling efficiency on homogeneous GPU cluster up to 8 nodes

- [ ] **Evaluation harness interface** — `training_eval.h` (Target: Q3 2026)
  - Standard metrics: perplexity, BLEU, ROUGE, task-specific accuracy
  - Pluggable metric registry (`IMetric`)
  - Integration with `ProvenanceTracker`

- [ ] **Adapter merge/export API** — extend `lora_adapter.h` (Target: Q3 2026)
  - `LoRAAdapter::merge_into(BaseModel&)` — produce a full-weight merged model
  - `LoRAAdapter::export_gguf(path)` — GGUF-format export for llama.cpp

### Q4 2026

- [ ] **RLHF reward model interface** — `rlhf_reward_model.h` (Target: Q4 2026)
  - `IRewardModel` abstract interface; PPO and DPO strategy hooks

- [ ] **Quantisation-aware LoRA** — extend `lora_adapter.h` (Target: Q4 2026)
  - QLoRA (NF4 base + BF16 adapters); `QuantConfig` sub-struct

- [ ] **Federated / privacy-preserving training** — `federated_trainer.h` (Target: Q4 2026)
  - Differential-privacy noise injection (`DPConfig` with epsilon/delta budget)

---

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define `ITrainer` / `IAdapter` / `IDataSource` abstract interfaces
- [x] Establish `LoRAAdapter` as move-only value type
- [x] Define `ProvenanceRecord` schema and append-only contract
- [ ] Draft `distributed_lora_trainer.h` cluster topology API (Target: Q3 2026)
- [ ] Draft `rlhf_reward_model.h` preference interface (Target: Q4 2026)

### Phase 2 — Core Implementation
- [x] `IncrementalLoRATrainer` step/epoch loop
- [x] `LoRACheckpointManager` atomic write-to-temp + rename
- [x] `AutoLabeler` rule-based and embedding-similarity strategies
- [x] `KGEnricher` read-only graph traversal
- [ ] Distributed all-reduce for adapter rank matrices (Target: Q3 2026)
- [ ] QLoRA NF4 quantisation path (Target: Q4 2026)

### Phase 3 — Error Handling & Edge Cases
- [x] `LoRACheckpointManager` crash-safety (temp + rename)
- [x] `ModalityParser` input-size bounds check
- [x] `DataSelector` unknown-strategy guard
- [ ] Distributed trainer quorum-loss recovery (Target: Q3 2026)

### Phase 4 — Tests
- [x] Unit tests for all v1.x headers (≥ 90 % line coverage)
- [x] Checkpoint round-trip tests (save + crash-simulate + resume)
- [x] Provenance append-only invariant tests
- [ ] Distributed training 3-node integration suite (Target: Q3 2026)

### Phase 5 — Performance / Hardening
- [x] Move semantics on `LoRAAdapter` (eliminates rank-matrix deep copy)
- [x] Parallel KG enrichment workers (`max_parallel_enrich_workers`)
- [x] `ProvenanceTracker` lock-free append on hot path
- [ ] Distributed gradient compression (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance
- [x] ARCHITECTURE.md, README.md, AUDIT.md, SECURITY.md, CHANGELOG.md
- [ ] API reference (Doxygen HTML) published to docs site (Target: Q3 2026)
- [ ] Migration guide v1.x → v2.0 (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All public headers have `#pragma once`
- [x] Abstract interfaces have virtual destructors
- [x] `[[nodiscard]]` applied to all result-bearing free functions
- [x] Thread-safety documented per class in Doxygen comments
- [x] `LoRAAdapter` is move-constructible (tensor data not deep-copied)
- [x] `LoRACheckpointManager` is crash-safe (write-to-temp + rename)
- [x] `ProvenanceTracker` append-only invariant enforced at API level
- [x] `ModalityParser` input-size bounded (no unchecked memcpy)
- [x] CI: headers compile under C++17 and C++20 with `-Wall -Wextra -Werror`
- [ ] Distributed training fault-tolerance hardened (Target: Q3 2026)
- [ ] API reference published on docs site (Target: Q3 2026)
