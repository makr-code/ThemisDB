> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/training/FUTURE_ENHANCEMENTS.md -->

# Training Module — Public Header Future Enhancements

**Module Path:** `include/training/`
**Canonical implementation enhancements:** [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/training/`. Runtime trainer loops, data pipeline scheduling, checkpoint internals, and benchmark work remain tracked in:

→ [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` LoRA adapter headers must define stable serialisation contracts; weight-tensor layout must remain opaque.
- `[x]` `ITrainer` and `IDataIterator` must remain the public extension points for custom training backends and data sources.
- `[x]` Checkpoint headers must expose stable versioned save/restore contracts.
- `[x]` Provenance headers must define immutable recording contracts; storage backends remain internal.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `LoRAAdapter` config / serialise | `lora_adapter.h` | LLM serving and adapter deployment | ✅ Stable |
| `ITrainer` train / evaluate | `training_interfaces.h` | Training pipeline orchestration | ✅ Stable |
| `IncrementalLoRATrainer` step API | `incremental_lora_trainer.h` | Online fine-tuning services | ✅ Stable |
| `AdapterServing` serve / register | `adapter_serving.h` | LLM adapter hot-serve | ✅ Stable |
| `ProvenanceTracker` record API | `provenance_tracker.h` | Compliance and audit services | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document data-selection strategy contracts and selection-quality vs. training-efficiency trade-offs in `lora_data_selection.h`.
- Align `adapter_serving.h` hot-swap contract with `include/llm/adapter_deployment_manager.h` lifecycle events.
- Add checkpoint-format version annotations to `lora_checkpoint_manager.h` for migration support.

### Medium-Term (Q4 2026)

- Introduce `training_policy.h` to provide per-job resource quotas, data-access controls, and privacy policy contract.
- Expose benchmark-reference training-throughput targets for incremental LoRA fine-tuning and data-selection hot paths.
- Deprecate any legacy batch-only training APIs superseded by the incremental trainer and annotate migration paths.

### Long-Term

- Unify adapter serialisation and checkpoint formats behind a versioned adapter-context envelope.
- Add extension hooks for embedders to inject custom data-selection strategies alongside the built-in `LoRADataSelection` implementations.
- Provide federated training hooks via `training_interfaces.h` for privacy-preserving cross-node fine-tuning.
