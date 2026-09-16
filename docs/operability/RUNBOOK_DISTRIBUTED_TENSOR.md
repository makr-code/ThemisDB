# Runbook: Distributed Tensor Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB distributed tensor module.  Covers
the five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Shard Sync Failure

**Log pattern:** `[DIST_TENSOR:ShardSyncFailed]`

**Symptoms**
- One or more tensor shards fail to synchronise with the coordinator.
- Log lines contain `[DIST_TENSOR:ShardSyncFailed]` with shard ID and step.
- The affected tensor transitions to `STALE` or `FAILED` lifecycle state.

**Triage**
1. Check shard health: `themis_admin tensor shard status --id <shard_id>`.
2. Verify network connectivity between shard workers.
3. Confirm the manifest store is reachable and not in a degraded state.
4. Review the delta log for the affected shard — check for overflow or
   corruption indicators.

**Remediation**
1. Trigger a shard re-sync: `themis_admin tensor shard resync --id <shard_id>`.
2. If the shard state is `FAILED`, trigger a rebuild:
   `themis_admin tensor shard rebuild --id <shard_id>`.
3. For delta log overflow (> 95% capacity), flush the log and force a snapshot
   rebuild.
4. Ensure the advisory-only invariant is maintained: the Graph Truth Layer must
   remain authoritative until the shard reaches `READY`.

**Escalation**  
Shard sync failures affecting > 25% of shards simultaneously indicate a
coordinator or network outage; escalate to the distributed systems on-call.

---

## Scenario 2 — Tensor Partition Stall

**Log pattern:** `[DIST_TENSOR:PartitionStall]`

**Symptoms**
- A tensor partition stops making forward progress.
- Log lines contain `[DIST_TENSOR:PartitionStall]` with partition ID and
  elapsed seconds.
- Dependent operations queue up and latency increases.

**Triage**
1. Check partition worker status: `themis_admin tensor partition status`.
2. Identify if the stall is in the patch path, partial refit, or rebuild path.
3. Review the planner freshness gate — a stale partition may be rejected by the
   planner before it can proceed.
4. Check for lock contention in the shard summary coordinator.

**Remediation**
1. Force-release stale partition locks:
   `themis_admin tensor partition unlock --id <partition_id>`.
2. Reset the partition to `READY` state if it is stuck in `REBUILDING`:
   `themis_admin tensor partition reset --id <partition_id>`.
3. If the stall is caused by planner freshness rejection, update the freshness
   threshold: `distributed_tensor.planner_freshness_threshold_ms`.
4. Restart the partition worker to clear any internal state corruption.

**Escalation**  
If a partition stall blocks downstream serving for > 5 minutes, escalate with
the partition timeline metrics and coordinator logs.

---

## Scenario 3 — All-Reduce Timeout

**Log pattern:** `[DIST_TENSOR:AllReduceTimeout]`

**Symptoms**
- The distributed all-reduce operation times out waiting for all workers.
- Log lines contain `[DIST_TENSOR:AllReduceTimeout]` with worker IDs and ms.
- Gradient synchronisation stops and training stalls.

**Triage**
1. Check all worker heartbeats to identify which worker is not responding.
2. Review network bandwidth utilisation between worker nodes — saturation can
   cause all-reduce timeouts.
3. Check whether any worker has a significantly larger gradient tensor than
   expected (gradient overflow precursor).
4. Confirm the all-reduce topology is still consistent with the current worker
   set.

**Remediation**
1. Restart the unresponsive worker: `themis_admin tensor worker restart --id <W>`.
2. Reduce all-reduce chunk size to decrease per-round network traffic:
   `distributed_tensor.allreduce_chunk_bytes`.
3. Increase the all-reduce timeout if the workload legitimately requires more
   time: `distributed_tensor.allreduce_timeout_ms`.
4. For network saturation, enable gradient compression if available.

**Escalation**  
Persistent all-reduce timeouts across multiple workers indicate a network or
topology issue; escalate to the network operations team.

---

## Scenario 4 — Gradient Overflow

**Log pattern:** `[DIST_TENSOR:GradientOverflow]`

**Symptoms**
- Gradient values exceed the representable range (NaN or Inf gradients detected).
- Log lines contain `[DIST_TENSOR:GradientOverflow]` with tensor ID and layer.
- Training loss may spike or become NaN.

**Triage**
1. Identify the layer and step at which overflow first occurred.
2. Check whether gradient clipping is enabled and configured correctly.
3. Review the learning rate schedule — a spike in LR can trigger overflow.
4. Check for mixed precision (fp16/bf16) underflow that can cascade to overflow.

**Remediation**
1. Enable gradient clipping: `training.gradient_clip_norm: 1.0`.
2. Reduce the learning rate by 50% and resume from the last clean checkpoint.
3. For mixed precision, enable dynamic loss scaling:
   `training.use_dynamic_loss_scaling: true`.
4. If overflow is layer-specific, inspect that layer's weight initialisation.

**Escalation**  
If gradient overflow persists after clipping and LR reduction, the model
configuration may have a fundamental instability; escalate to the ML platform
team with gradient norms and loss curves.

---

## Scenario 5 — Checkpoint Divergence

**Log pattern:** `[DIST_TENSOR:CheckpointDivergence]`

**Symptoms**
- Distributed worker checkpoints are inconsistent with each other.
- Log lines contain `[DIST_TENSOR:CheckpointDivergence]` with diverging
  step numbers or Merkle proof mismatches.
- Resuming from checkpoint produces incorrect results.

**Triage**
1. Compare checkpoint step numbers across all workers.
2. Verify Merkle proof integrity for each checkpoint artefact:
   `themis_admin tensor checkpoint verify --id <checkpoint_id>`.
3. Check whether any worker wrote a partial checkpoint due to an OOM or timeout.
4. Review the receipt chain for any tampering or corruption indicators.

**Remediation**
1. Roll all workers back to the last globally consistent checkpoint:
   `themis_admin tensor checkpoint rollback --to-step <N>`.
2. Delete any divergent checkpoint artefacts and re-trigger checkpoint from the
   canonical step.
3. Enable synchronous checkpoint writes to prevent partial checkpoint scenarios:
   `distributed_tensor.synchronous_checkpoint: true`.
4. Validate Merkle proofs after every checkpoint to catch divergence early.

**Escalation**  
Checkpoint divergence that cannot be resolved by rollback requires a full
distributed tensor state reset; escalate to the ML operations team immediately.

---

## Reference

| Log Pattern                            | Scenario                    |
|----------------------------------------|-----------------------------|
| `[DIST_TENSOR:ShardSyncFailed]`        | Shard Sync Failure          |
| `[DIST_TENSOR:PartitionStall]`         | Tensor Partition Stall      |
| `[DIST_TENSOR:AllReduceTimeout]`       | All-Reduce Timeout          |
| `[DIST_TENSOR:GradientOverflow]`       | Gradient Overflow           |
| `[DIST_TENSOR:CheckpointDivergence]`   | Checkpoint Divergence       |

**Related resources**
- `src/distributed_tensor/ROADMAP.md` — Wave D operability tracking
- `tests/integration/test_distributed_tensor_soak.cpp` — Soak test evidence
- `tests/distributed_tensor/test_distributed_tensor_highcardinality_stress.cpp`
- `benchmarks/distributed_tensor/bench_distributed_tensor_dedicated_gates.cpp`
- `src/distributed_tensor/include/artifact_manifest.h` — lifecycle states
- `src/distributed_tensor/include/tensor_delta_log.h` — delta log schema
