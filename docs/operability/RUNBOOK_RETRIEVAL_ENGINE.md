# Runbook: Retrieval Engine

> **Module**: `src/retrieval`
> **Wave**: D
> **Version**: 1.0.0
> **Owner**: ThemisDB Platform Engineering
> **See also**: [`src/retrieval/ROADMAP.md`](../../src/retrieval/ROADMAP.md),
> [`docs/operability/WAVE_D_ROADMAP.md`](WAVE_D_ROADMAP.md)

---

## Purpose

This runbook provides operator triage and remediation procedures for the
ThemisDB Retrieval Engine. It covers the five operator-critical failure
scenarios identified during Wave D.

---

## Scenario 1 — Shard Failure

### Symptom
Log pattern: `[RETRIEVAL:ShardFailure]`

Queries to one or more shards return errors or are routed to fallback shards.
Latency may spike; throughput may drop.

### Diagnosis
1. Check shard health in the cluster topology dashboard.
2. Search logs for `[RETRIEVAL:ShardFailure]` entries; note the shard IDs
   and timestamps.
3. Confirm whether the failure is network-partition, process crash, or
   disk exhaustion.

### Remediation
1. If the shard process crashed, restart it via the process manager:
   ```
   themis-ctl shard restart --shard-id <ID>
   ```
2. If the disk is full, expand storage or move the shard:
   ```
   themis-ctl shard relocate --shard-id <ID> --target-node <NODE>
   ```
3. Verify the routing table has converged:
   ```
   themis-ctl routing verify
   ```
4. Monitor for `[RETRIEVAL:ShardFailure]` clearance in logs.

---

## Scenario 2 — GPU Path Stall

### Symptom
Log pattern: `[RETRIEVAL:GPUPathStall]`

The GPU advisory acceleration path stops returning results. CPU fallback
continues to operate, but p99 latency may rise.

### Diagnosis
1. Check GPU device health:
   ```
   nvidia-smi  # or: themis-ctl gpu status
   ```
2. Search logs for `[RETRIEVAL:GPUPathStall]` to confirm the stall time.
3. Check if the GPU watchdog has marked the device as faulted.

### Remediation
1. Clear the transient error and re-enable the GPU path:
   ```
   themis-ctl gpu reset --device <DEVICE_ID>
   ```
2. If the GPU is permanently unavailable, disable the advisory path and
   pin the engine to CPU-only mode:
   ```
   themis-ctl config set retrieval.gpu.enabled=false
   ```
3. File a hardware replacement ticket if `nvidia-smi` reports hardware errors.
4. Monitor for `[RETRIEVAL:GPUPathStall]` clearance.

---

## Scenario 3 — Routing Table Corruption

### Symptom
Log pattern: `[RETRIEVAL:RoutingCorruption]`

Keys are routed to the wrong shards. Results may be missing or incorrect.

### Diagnosis
1. Run the routing table integrity check:
   ```
   themis-ctl routing verify --full
   ```
2. Look for entries in logs tagged `[RETRIEVAL:RoutingCorruption]`.
3. Compare the current routing table checksum against the last known-good
   snapshot in `themis_admin_cache/routing/`.

### Remediation
1. Restore the routing table from the last known-good snapshot:
   ```
   themis-ctl routing restore --snapshot <SNAPSHOT_ID>
   ```
2. Trigger a full routing table rebuild if no valid snapshot exists:
   ```
   themis-ctl routing rebuild
   ```
3. Validate consistency across all shards:
   ```
   themis-ctl routing verify --full
   ```
4. Monitor for `[RETRIEVAL:RoutingCorruption]` clearance.

---

## Scenario 4 — Phase-C Gate Failure

### Symptom
Log pattern: `[RETRIEVAL:PhaseGateFailed]`

The Phase-C multi-shard gate test (`test_sharding_multishard_exact`) is
failing in CI or in production readiness validation.

### Diagnosis
1. Review the CI run for the `ShardingMultiShardExactPhaseCGate` ctest target.
2. Check whether shard environment is fully provisioned (all shards healthy).
3. Look for `[RETRIEVAL:PhaseGateFailed]` in the test or operational logs.

### Remediation
1. Confirm all shards are healthy before re-running the gate:
   ```
   themis-ctl shard list --status
   ```
2. Re-run the gate test with verbose logging:
   ```
   ctest -R ShardingMultiShardExactPhaseCGate -V
   ```
3. If the failure is environment-specific, escalate to the platform team
   with the full log output.
4. Do not promote to Phase C production enable until the gate passes cleanly.

---

## Scenario 5 — GPU Break-Even Miss

### Symptom
GPU advisory path is enabled but benchmark results show no GPU break-even
latency improvement over CPU-only paths.

### Diagnosis
1. Run the GPU break-even benchmark:
   ```
   benchmarks/retrieval/bench_retrieval_dedicated_gates --benchmark_filter=RT-BM-03
   ```
2. Compare `gpu_advisory_throughput` against `cpu_baseline_throughput`.
3. Check if the GPU is memory-bandwidth bound (small batch sizes won't
   benefit from GPU acceleration).

### Remediation
1. Increase the minimum batch size for GPU dispatch:
   ```
   themis-ctl config set retrieval.gpu.min_batch=<N>
   ```
2. If GPU break-even cannot be achieved with current hardware, disable
   the advisory path to avoid the overhead:
   ```
   themis-ctl config set retrieval.gpu.enabled=false
   ```
3. Record the benchmark results in
   `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` and mark the
   GPU break-even item accordingly.

---

## Log Pattern Reference

| Pattern                            | Scenario                        |
|------------------------------------|---------------------------------|
| `[RETRIEVAL:ShardFailure]`         | Shard unavailable or crashed    |
| `[RETRIEVAL:GPUPathStall]`         | GPU advisory path not responding|
| `[RETRIEVAL:RoutingCorruption]`    | Routing table inconsistency     |
| `[RETRIEVAL:PhaseGateFailed]`      | Phase-C/D gate test failing     |

---

## Escalation

If remediation steps do not resolve the incident within 30 minutes, escalate
to the ThemisDB on-call engineer with:
- Relevant log excerpts (tagged with the log patterns above)
- Output of `themis-ctl routing verify --full`
- Output of `themis-ctl shard list --status`
- Benchmark results if GPU break-even is the suspected cause
