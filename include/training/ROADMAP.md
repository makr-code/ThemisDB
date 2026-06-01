> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/training/ROADMAP.md -->

# TRAINING Module — Public Header Roadmap

**Module Path:** `include/training/`
**Canonical implementation roadmap:** [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/training/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)

---

## Current Status

production training runtime with auto-labeling, LoRA/AdaLoRA adapters, checkpoint management, provenance tracking, and pipeline orchestration. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `auto_labeler.h` — dataset preparation contract
- [x] `database_domain_auto_labeler.h` — dataset preparation contract
- [x] `modality_parser.h` — dataset preparation contract
- [x] `knowledge_graph_enricher.h` — dataset preparation contract
- [x] `lora_data_selection.h` — dataset preparation contract
- [x] `lora_adapter.h` — adapter training lifecycle contract
- [x] `ada_lora_adapter.h` — adapter training lifecycle contract
- [x] `adalora_tt_bridge.h` — adapter training lifecycle contract
- [x] `incremental_lora_trainer.h` — adapter training lifecycle contract
- [x] `lora_adapter_merger.h` — adapter training lifecycle contract
- [x] `lora_checkpoint_manager.h` — adapter training lifecycle contract
- [x] `provenance_tracker.h` — governance and orchestration contract
- [x] `training_interfaces.h` — governance and orchestration contract
- [x] `training_pipeline.h` — governance and orchestration contract
- [x] `adapter_serving.h` — governance and orchestration contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
