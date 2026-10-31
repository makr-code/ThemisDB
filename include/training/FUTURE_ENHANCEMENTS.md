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
- `[x]` `training_interfaces.h` provides explicit checkpoint, provenance, enrichment, calibration, asynchronous pipeline, and lineage contracts; component-specific trainer/data APIs remain in their corresponding headers.
- `[x]` Checkpoint headers must expose stable versioned save/restore contracts.
- `[x]` Provenance headers must define immutable recording contracts; storage backends remain internal.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `LoRAAdapter` configuration / serialization | `lora_adapter.h` | Adapter lifecycle and persistence | Declared |
| `ITrainingPipeline` submit / cancel / status | `training_interfaces.h` | Asynchronous job orchestration contract | Declared |
| `IncrementalLoRATrainer` training API | `incremental_lora_trainer.h` | Incremental adapter-training operations | Declared |
| `ILLMRouter` and `DeployResult` | `adapter_serving.h` | Adapter traffic handoff and result reporting | Declared |
| `ProvenanceTracker` record API | `provenance_tracker.h` | Sample lineage and provenance | Declared |
| `ILoRACheckpointManager`, `ISampleProvenanceTracker`, `IKGEnrichmentInterface`, `IConfidenceCalibrator`, `ILineageQueryAPI` | `training_interfaces.h` | Checkpoint, lineage, enrichment, and calibration contracts | Declared |

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
