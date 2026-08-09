# Training Module — Frozen Lifecycle & Error Taxonomy Contracts

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document freezes the **training, checkpoint, and adapter lifecycle
contracts** for the Training module's current major line (v2.x), and provides
a summary reference for the **explicit error taxonomy** for labeling,
checkpoint, and serving incidents.

The full error taxonomy is defined in machine-readable form in
`include/training/training_error_codes.h` (`TrainingErrorCode` enum).

---

## 2. Training Lifecycle Contract

### 2.1 Training Pipeline States

```
IDLE → LABELING → TRAINING → CHECKPOINTING → SERVING_HANDOFF → IDLE
            │                       │
            └── ERROR (recoverable or terminal)
```

| State | Description |
|---|---|
| IDLE | No active training job |
| LABELING | Dataset annotation / enrichment in progress |
| TRAINING | LoRA / AdaLoRA gradient-update loop running |
| CHECKPOINTING | Writing model weights + metadata to disk |
| SERVING_HANDOFF | Promoting trained adapter to serving |
| ERROR | See § 4 for recovery semantics |

**Invariants:**
- At most one training job per adapter slot may be active at a time.
- A checkpoint written during the CHECKPOINTING state MUST be atomic:
  a partially written checkpoint MUST NOT be visible to the serving path.
- Cancellation of a training job MUST NOT corrupt the last valid checkpoint.

### 2.2 LoRA / AdaLoRA Adapter Lifecycle

| Operation | Contract |
|---|---|
| Load adapter | Returns `false` on missing file; throws `AdapterException` on corrupt file |
| Save adapter | Atomic write (write to temp path → rename); returns `false` on I/O failure |
| Merge adapters | Requires ≥ 2 loaded adapters; throws `MergeException(MERGE_NO_ADAPTERS)` if fewer |
| Unload adapter | Safe to call when not loaded (idempotent) |
| Rollback | Restores last valid checkpoint; MUST NOT leave the adapter in partial state |

---

## 3. Checkpoint Contract

- Checkpoints MUST include a SHA-256 manifest entry for each saved weight file.
- Loading a checkpoint with a SHA-256 mismatch MUST return
  `TrainingErrorCode::CHECKPOINT_SHA256_MISMATCH` (0x8044).
- Checkpoint save MUST NOT overwrite a valid checkpoint until the new checkpoint
  is fully written and verified.
- Checkpoint corruption (truncated, format mismatch) MUST be detected at load
  time, not at inference time.

**Checkpoint atomicity:**
1. Write to `<path>.tmp`
2. Write manifest to `<path>.manifest.tmp`
3. Rename both atomically (or best-effort sequential with rollback on failure)
4. On rename failure, leave the prior valid checkpoint intact

---

## 4. Error Taxonomy Reference

The full `TrainingErrorCode` enum is in `include/training/training_error_codes.h`.

### 4.1 Labeling Errors (0x9000–0x90FF)

| Code | Name | Retryable | Condition |
|---|---|---|---|
| 0x9000 | `LABELING_NO_CONTENT` | No | Empty dataset submitted |
| 0x9001 | `LABELING_INVALID_FORMAT` | No | Dataset format unrecognised |
| 0x9002 | `LABELING_SCHEMA_MISMATCH` | No | Schema does not match model |
| 0x9010 | `LABELING_RESOURCE_EXHAUSTED` | Yes | Memory/CPU saturation |
| 0x9020 | `LABELING_IO_FAILED` | Yes (transient) | Disk or network read error |

### 4.2 Checkpoint Errors (0x8000–0x80FF)

Key codes (see full list in `training_error_codes.h`):

| Code | Name | Retryable | Condition |
|---|---|---|---|
| 0x8000 | `CHECKPOINT_DIR_INVALID` | No | Directory path malformed or missing |
| 0x8003 | `CHECKPOINT_NOT_FOUND` | No | Checkpoint file absent |
| 0x8044 | `CHECKPOINT_SHA256_MISMATCH` | No | Integrity check failed |
| 0x8045 | `CHECKPOINT_TRUNCATED` | No | File shorter than expected |
| 0x8011 | `CHECKPOINT_DISK_SPACE_EXHAUSTED` | Yes | Free space below threshold |
| 0x8034 | `CHECKPOINT_IO_TIMEOUT` | Yes | Write exceeded time budget |

### 4.3 Serving / Adapter Errors (0xA000–0xA0FF)

| Code | Name | Retryable | Condition |
|---|---|---|---|
| 0xA000 | `SERVING_ADAPTER_NOT_FOUND` | No | Adapter not loaded or unregistered |
| 0xA001 | `SERVING_ADAPTER_INCOMPATIBLE` | No | Adapter dim/arch mismatch |
| 0xA010 | `SERVING_RESOURCE_EXHAUSTED` | Yes | Inference-time memory saturation |

---

## 5. Recovery Semantics

| Error class | Recovery path |
|---|---|
| Labeling input error | Fix input and resubmit; existing checkpoint unaffected |
| Checkpoint I/O failure | Retry with backoff; fall back to prior valid checkpoint |
| SHA-256 mismatch | Delete corrupt checkpoint; restore from remote or prior backup |
| Training OOM | Reduce batch size; reduce LoRA rank; restart job |
| Serving adapter not found | Reload adapter or rebuild checkpoint |

---

## 6. Backward Compatibility

- Error code values in `TrainingErrorCode` are immutably frozen for v2.x.
- New error codes may be added in vacant ranges (additive).
- Lifecycle state transitions and adapter lifecycle invariants are frozen for v2.x.
- A v3.x version bump is required to change existing state names or code values.
