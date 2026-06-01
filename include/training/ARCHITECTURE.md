> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/training/ARCHITECTURE.md -->

# TRAINING Module — Public Header Architecture

**Module Path:** `include/training/`
**Implementation:** `../../src/training/`
**Canonical architecture doc:** [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)

---

## 1. Overview

The `include/training/` directory contains the **public C++ header contract** for ThemisDB's auto-labeling and enrichment, LoRA/AdaLoRA training lifecycle, checkpoint/provenance handling, and training-pipeline orchestration. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::training`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Dataset preparation` | `auto_labeler.h`, `database_domain_auto_labeler.h`, `modality_parser.h`... |
| `Adapter training lifecycle` | `lora_adapter.h`, `ada_lora_adapter.h`, `adalora_tt_bridge.h`... |
| `Governance and orchestration` | `provenance_tracker.h`, `training_interfaces.h`, `training_pipeline.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP` | ada_lora_adapter.h, adalora_tt_bridge.h, incremental_lora_trainer.h | GPU-accelerated LoRA/AdaLoRA training |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)
- Module overview: [`../../src/training/README.md`](../../src/training/README.md)
- Roadmap: [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md)
- Future enhancements: [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
