> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/training/ROADMAP.md -->

# Training Module — Public Header Roadmap

**Module Path:** `include/training/`
**Canonical implementation roadmap:** [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)

---

## Overview

Tracks public training API contract stability, header coverage, and future public entry points. Runtime trainer loops, checkpoint internals, and data pipeline scheduling work remain in:

→ [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)

---

## Current Status

All 15 training headers are present. Public entry points exist for LoRA and AdaLoRA adapters, TT-bridge, adapter merging, checkpoint management, incremental and data-selection training, training pipeline and interfaces, modality parsing, auto-labelling, knowledge-graph enrichment, provenance tracking, and adapter serving.

---

## Completed ✅

- [x] `lora_adapter.h`, `ada_lora_adapter.h`, `adalora_tt_bridge.h`, `lora_adapter_merger.h`, `lora_checkpoint_manager.h` — LoRA adapter lifecycle contract
- [x] `incremental_lora_trainer.h`, `lora_data_selection.h`, `training_pipeline.h`, `training_interfaces.h`, `modality_parser.h` — training pipeline and data
- [x] `auto_labeler.h`, `database_domain_auto_labeler.h`, `knowledge_graph_enricher.h` — auto-labelling and knowledge enrichment
- [x] `provenance_tracker.h`, `adapter_serving.h` — provenance and adapter serving

---

## In Progress

- [ ] Document data-selection strategy contracts and selection-quality trade-offs in `lora_data_selection.h` (Target: 2026-Q3)
- [ ] Align `adapter_serving.h` hot-swap contract with `include/llm/adapter_deployment_manager.h` lifecycle (Target: 2026-Q3)

---

## Planned

- [ ] `training_policy.h` — per-job resource, data-access, and privacy policy contract (Target: 2026-Q4)
- [ ] Add checkpoint-format version annotations to `lora_checkpoint_manager.h` (Target: 2026-Q4)
- [ ] Expose benchmark training-throughput targets for incremental LoRA fine-tuning hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Training headers maintain backward compatibility within the active major line; checkpoint-format and adapter-serialisation changes require migration notes and changelog updates.
