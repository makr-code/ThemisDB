# Runbook: Training Pipeline

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB training pipeline module.  Covers
the five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Checkpoint Failure

**Log pattern:** `[TRAINING:CheckpointFailed]`

**Symptoms**
- Training job stops with a checkpoint write error.
- Subsequent restarts cannot find a valid checkpoint file.
- Log lines contain `[TRAINING:CheckpointFailed]` with a step number and path.

**Triage**
1. Confirm the checkpoint storage path is reachable and has sufficient free space.
2. Check filesystem permissions for the training process user.
3. Inspect recent checkpoint files: confirm no partial writes (zero-byte or
   truncated files).
4. Review the training coordinator logs for any preceding OOM or timeout events.

**Remediation**
1. Free disk space or extend the storage volume.
2. If the last checkpoint is corrupt, roll back to the previous valid checkpoint
   using `themis_admin checkpoint rollback --step <N>`.
3. Restart the training job with `--resume-from-checkpoint <path>`.
4. If recurring, increase `checkpoint_timeout_ms` in the training configuration
   and ensure the checkpoint directory is on fast local storage.

**Escalation**  
If checkpoint failures persist after remediation, open a P1 incident and attach
the training coordinator logs and storage utilisation metrics.

---

## Scenario 2 — Gradient Sync Stall

**Log pattern:** `[TRAINING:GradientSyncStall]`

**Symptoms**
- Distributed training workers report a sync barrier timeout.
- Log lines contain `[TRAINING:GradientSyncStall]` with a worker ID and step.
- Training throughput drops to zero.

**Triage**
1. Check all worker processes are alive (`themis_admin training workers status`).
2. Confirm network connectivity between worker nodes (latency, packet loss).
3. Look for any worker that is significantly behind in step count — a straggler
   worker will block the barrier.
4. Review GPU utilisation on each node; a single OOM on one node can stall sync.

**Remediation**
1. Restart the straggling worker with the same shard assignment.
2. If a worker has crashed, re-launch it with `--rejoin-training-group`.
3. For persistent network issues, reduce the gradient sync batch size to reduce
   synchronisation latency.
4. Enable straggler mitigation via `gradient_sync_straggler_timeout_ms`.

**Escalation**  
If the stall cannot be resolved within 15 minutes, trigger a full training group
restart from the latest valid checkpoint.

---

## Scenario 3 — OOM During Batch

**Log pattern:** `[TRAINING:BatchOOM]`

**Symptoms**
- Training process exits with an out-of-memory error during batch processing.
- Log lines contain `[TRAINING:BatchOOM]` with the batch ID and allocated bytes.
- GPU or CPU memory pressure metrics spike before the failure.

**Triage**
1. Identify the batch size and model configuration at time of failure.
2. Check GPU VRAM usage trend leading up to the OOM event.
3. Verify gradient accumulation steps are within memory budget.
4. Review any recent model configuration changes (rank increase, layer expansion).

**Remediation**
1. Reduce the per-device batch size in `training_config.batch_size`.
2. Enable gradient checkpointing (`use_gradient_checkpointing: true`) to trade
   compute for memory.
3. Increase gradient accumulation steps to maintain effective batch size while
   reducing peak memory.
4. If using mixed precision, verify `fp16` / `bf16` is enabled for the training
   run.

**Escalation**  
For OOM on models that previously ran successfully, investigate recent
configuration drift and review VRAM budget in `PERFORMANCE_EXPECTATIONS.md`.

---

## Scenario 4 — Distributed Training Hang

**Log pattern:** `[TRAINING:DistributedHang]`

**Symptoms**
- All workers are alive but no training progress is observed.
- Log lines contain `[TRAINING:DistributedHang]` with elapsed seconds.
- The training heartbeat metric stops incrementing.

**Triage**
1. Check all worker heartbeats: `themis_admin training heartbeat status`.
2. Inspect distributed coordination locks — a deadlock between workers can cause
   a hang without any worker crashing.
3. Review the all-reduce topology for any asymmetric routing that might cause
   a cycle.
4. Check for NCCL / communication library errors in the system logs.

**Remediation**
1. Force-release any stale distributed locks:
   `themis_admin training locks release --force`.
2. Restart the training coordinator: `themis_admin training restart --soft`.
3. If the hang recurs, enable distributed heartbeat watchdog
   (`distributed_hang_timeout_ms`) to auto-restart stalled groups.
4. Reduce the all-reduce topology depth for the affected training group.

**Escalation**  
If hangs recur more than twice per training run, escalate to the distributed
systems on-call with full coordinator logs and topology configuration.

---

## Scenario 5 — Model Export Failure

**Log pattern:** `[TRAINING:ExportFailed]`

**Symptoms**
- Post-training model export step fails with a serialisation error.
- Log lines contain `[TRAINING:ExportFailed]` with the adapter ID and format.
- The serving pipeline cannot find the exported model artifact.

**Triage**
1. Check that the export target directory exists and is writable.
2. Confirm the adapter state is `READY` (not `REBUILDING` or `FAILED`):
   `themis_admin adapter status --id <adapter_id>`.
3. Inspect the export log for format-specific errors (ONNX, safetensors, etc.).
4. Verify there is sufficient disk space for the export artefact.

**Remediation**
1. Clear any partial export artefacts and retry:
   `themis_admin adapter export --id <adapter_id> --retry`.
2. If the adapter state is `FAILED`, trigger a rebuild:
   `themis_admin adapter rebuild --id <adapter_id>`.
3. For format-specific errors, check the adapter configuration for unsupported
   operator types and consult `docs/training/ADAPTER_EXPORT_GUIDE.md`.
4. Ensure the export format version matches the serving pipeline expectation.

**Escalation**  
For repeated export failures on the same adapter, open a P2 incident and attach
the adapter lifecycle logs and export configuration.

---

## Reference

| Log Pattern                        | Scenario                   |
|------------------------------------|----------------------------|
| `[TRAINING:CheckpointFailed]`      | Checkpoint Failure         |
| `[TRAINING:GradientSyncStall]`     | Gradient Sync Stall        |
| `[TRAINING:BatchOOM]`              | OOM During Batch           |
| `[TRAINING:DistributedHang]`       | Distributed Training Hang  |
| `[TRAINING:ExportFailed]`          | Model Export Failure       |

**Related resources**
- `src/training/ROADMAP.md` — Wave D operability tracking
- `tests/integration/test_training_pipeline_soak.cpp` — Soak test evidence
- `tests/training/test_training_highcardinality_stress.cpp` — Stress test evidence
- `benchmarks/training/bench_training_dedicated_gates.cpp` — TR-BM-01..04 gates
