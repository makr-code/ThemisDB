---
Author: "ThemisDB Team"
Created: "2026-09-16"
Last Updated: "2026-11-24"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Runbook: Distributed Tensor Module

> **Not validated for production operations:** The opt-in mTLS fragment RPC
> listener, RocksDB fragment store, publisher, and planner were built and the
> Linux fixture-graph publisher-to-mTLS case passed in `ShardTransferGrpcTests`
> (2026-11-24). The `themis_server` target compiled and linked with this adapter,
> but configured production startup was not run. No live freshness, trusted
> artifact discovery, or distributed worker service is claimed.
> The former in-process
> stub soak was removed; no transport-backed soak result is claimed.

## Purpose

Operator remediation guide for the ThemisDB distributed tensor module.  Covers
the five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Optional ShardRPC fragment serving

The main server starts this listener only when
`sharding.tensor_fragment_rpc.enabled` is `true`. A production configuration
must use a distinct listener address and provide an explicit shard ID, server
certificate/private key, client CA, and exact X.509 identity-to-principal
bindings. There is no insecure fallback:

```yaml
sharding:
  shard_id: "shard-1"
  tensor_fragment_rpc:
    enabled: true
    listen_address: "0.0.0.0:50052"
    tls_cert_path: "/etc/themisdb/tls/shard-server.crt"
    tls_key_path: "/etc/themisdb/tls/shard-server.key"
    tls_ca_cert_path: "/etc/themisdb/tls/cluster-ca.crt"
    max_tensor_fragment_bytes: 67108864
    chunk_bytes: 1048576
    max_deadline_ms: 30000
    identity_bindings:
      - property_name: "x509_subject_alternative_name"
        property_value: "spiffe://example.invalid/tenant-a/worker-2"
        principal_id: "tenant-a"
```

`property_value` must exactly match the gRPC-authenticated certificate value.
Choose `principal_id` as a tenant-bound storage authorization namespace; the
server derives it from mTLS rather than accepting it from the request. Before
advertising a manifest placement, the owner-side publisher must store the
complete fragment with `ShardArtifactFragmentStore::put(principal_id,
shard_id, artifact_id, bytes)`. The store uses a versioned RocksDB key derived
from that tuple, permits identical replay, rejects replacement with different
bytes, checks size metadata before loading payload bytes, and verifies SHA-256
on reads. It is separate from `TensorNetworkStorageEngine` field keys.
Fragment writes and identical replays request a synchronous RocksDB WAL commit
independently of the database's default write options. This is source-level
behavior, not tested power-loss evidence, and depends on the storage stack
honoring sync operations. A publisher must commit the manifest only after the
fragment write succeeds.

### Optional startup projection publication

The server can publish one adapter-similarity projection at startup when the
distributed tensor ShardRPC adapter is included in the binary. Configure this
only when the selected tenant has the query adapter and at least one eligible
similar adapter:

```yaml
sharding:
  tensor_projection_publisher:
    enabled: true
    principal_id: "tenant-a"
    tenant_id: "tenant-a"
    source_snapshot_id: "adapter-catalog-2026-11-24"
    node_id: "node-2"
    domain: "legal"
    base_model_id: "llama3-8b"
    top_k: 10
```

The principal must occur in `tensor_fragment_rpc.identity_bindings`; that
configuration pair is the operator's principal-to-tenant authorization
binding. The RPC server must also be enabled, and its shard, TLS, listener, and
identity configuration remain mandatory. At startup the host enumerates and
loads the tenant's adapter records from RocksDB, rejects an invalid or
unreadable record, builds a fingerprint graph, and publishes one query summary.
Missing query data, no publishable candidates, incompatible optional build
configuration, and persistence failures abort startup. The source snapshot ID
is an operator-supplied label, not a measured source revision or freshness
proof. This publication does not track subsequent adapter changes; operators
must restart with an updated snapshot label to publish again. The resulting
artifact ID is logged but not entered in a trusted cross-shard discovery
service.

`LocalProjectionPublisher` additionally stores a manifest record readable through
the same mTLS fragment endpoint. `loadRemote()` requires a trusted artifact ID,
tenant-bound principal, expected shard and node, and an explicitly configured
peer SAN; client and server response limits must allow at least 16 KiB. It fetches
and verifies the projection before returning the manifest; the planner may fetch
the projection again. Both RPCs share the requested deadline budget. It does
not discover current artifacts or authenticate the original producer. Records
published before this transport was added must be republished to create the
remote-readable record. The three synchronous writes are not one transaction;
retry the same publication after failure.

If enabled configuration lacks an explicit shard identity, listener/TLS fields,
or identity bindings, server startup fails. During shutdown the fragment RPC
listener stops before the shared RocksDB database closes. Missing or
over-limit fragments are not served; corrupt metadata or payload causes a
failed RPC. Graph validation accepts the versioned adapter-similarity
projection, not arbitrary opaque tensor fragments. The publisher remains
single-shard and startup-only; discovery, refresh, revocation, and
multi-shard coordination are not implemented.

The fixture path is runtime-tested; the configured host startup and production
lifecycle are not. Do not treat compilation or the focused test as production
readiness evidence.

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
- `src/distributed_tensor/TEST_EVIDENCE_COLLECTION.md` — current test and
  transport evidence boundaries
- `tests/distributed_tensor/test_distributed_tensor_highcardinality_stress.cpp`
- `benchmarks/distributed_tensor/bench_distributed_tensor_dedicated_gates.cpp`
- `src/distributed_tensor/include/artifact_manifest.h` — lifecycle states
- `src/distributed_tensor/include/tensor_delta_log.h` — delta log schema
