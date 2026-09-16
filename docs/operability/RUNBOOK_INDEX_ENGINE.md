# RUNBOOK: Index Engine

<!-- Wave D operability runbook — approved 2026-09-16 -->
<!-- Cross-links: src/index/ROADMAP.md Wave D contribution; D1 trace spans -->

## Overview

This runbook covers the five most operator-critical failure scenarios for the
ThemisDB Index Engine.  Each scenario lists detection signals, diagnostic
steps, remediation actions, and the expected log pattern used for alerting.

Trace span prefix: `D1-INDEX-*` (emitted by the index module for Wave D
distributed tracing; see `docs/operability/WAVE_D_ROADMAP.md §D1`).

---

## Scenario 1 — HNSW Index Corruption

**Log pattern:** `[INDEX:HNSWCorruption]`

### Symptoms

- ANN queries return empty results or `IndexErrorCode::HNSWGraphCorrupted`.
- Alert fires on `index_hnsw_corruption_detected == 1` Prometheus gauge.
- Trace span `D1-INDEX-HNSW` shows `status=corrupted`.
- Soak test `IndexSoak_QueryStability` fails in CI.

### Diagnosis

1. Run the focused query stability soak:
   ```
   ctest -R "IndexSoak_QueryStability" --output-on-failure
   ```
2. Check the HNSW graph file integrity:
   ```
   themis-admin index verify --component hnsw --shard all
   ```
3. Inspect recent index mutation events:
   ```
   themis-admin audit log --module index --event hnsw_mutation --since 2h
   ```
4. Look for process crash or OOM events near the corruption time:
   ```
   journalctl -u themis-index --since "2 hours ago" | grep -E "OOM|killed|crash"
   ```

### Remediation

1. Quarantine the corrupted shard and trigger a rebuild:
   ```
   themis-admin index quarantine --shard <shard_id>
   themis-admin index rebuild --component hnsw --shard <shard_id>
   ```
2. While rebuilding, route queries to a replica or fallback to CPU flat scan:
   ```
   themis-admin config set index.hnsw_fallback_strategy=cpu_flat
   ```
3. After rebuild completes, verify integrity before re-enabling:
   ```
   themis-admin index verify --component hnsw --shard <shard_id>
   themis-admin index enable --shard <shard_id>
   ```

### Rollback

Restore the HNSW graph from the last known-good snapshot:
```
themis-admin index restore --component hnsw --snapshot latest-known-good
```

---

## Scenario 2 — GPU Kernel Fallback

**Log pattern:** `[INDEX:GPUKernelFallback]`

### Symptoms

- Index operations are running significantly slower than baseline.
- Alert fires on `index_gpu_kernel_fallback_total > 0`.
- Trace span `D1-INDEX-GPU` shows `backend=cpu_fallback`.
- `GPUVectorIndex` logs `BACKEND_NOT_ENABLED` or `FALLBACK_CPU_ENABLED`.

### Diagnosis

1. Check GPU availability:
   ```
   themis-admin gpu status
   ```
2. Inspect backend gate diagnostics:
   ```
   grep "\[INDEX:GPUKernelFallback\]" /var/log/themis/index.log | tail -20
   ```
3. Confirm CUDA / Vulkan runtime is installed:
   ```
   nvidia-smi || echo "CUDA not available"
   vulkaninfo --summary 2>/dev/null || echo "Vulkan not available"
   ```
4. Check `GPUVectorIndex::allowCPUFallback` configuration:
   ```
   themis-admin config get index.gpu_allow_cpu_fallback
   ```

### Remediation

1. If the GPU is temporarily unavailable (driver restart), allow CPU fallback:
   ```
   themis-admin config set index.gpu_allow_cpu_fallback=true
   ```
2. If the GPU driver needs reinstalling, follow the hardware setup guide:
   `docs/setup/GPU_DRIVER_SETUP.md`
3. Once the GPU is back, disable CPU fallback to restore performance:
   ```
   themis-admin config set index.gpu_allow_cpu_fallback=false
   themis-admin index restart --component gpu_vector
   ```
4. If fallback is expected in this environment (CI, dev), set
   `index.expected_backend=cpu` to suppress the alert.

---

## Scenario 3 — Buffer Lifecycle OOM

**Log pattern:** `[INDEX:BufferOOM]`

### Symptoms

- Index insert or search operations fail with `IndexErrorCode::GpuMemoryError`.
- Alert fires on `index_buffer_oom_total > 0`.
- Trace span `D1-INDEX-BUFFER` shows `status=oom`.
- Process logs show `cudaErrorMemoryAllocation` or equivalent OOM.

### Diagnosis

1. Check GPU memory usage:
   ```
   nvidia-smi --query-gpu=memory.used,memory.free --format=csv
   ```
2. Inspect OOM log entries:
   ```
   grep "\[INDEX:BufferOOM\]" /var/log/themis/index.log | tail -30
   ```
3. Check the configured GPU memory budget:
   ```
   themis-admin config get index.gpu_memory_budget_bytes
   ```
4. Look for memory leak indicators (rapid growth with no plateau):
   ```
   themis-admin metrics dump --filter "index_gpu_memory_" | tail -20
   ```

### Remediation

1. Reduce the batch size to lower peak memory usage:
   ```
   themis-admin config set index.search_batch_size=256
   ```
2. Lower the GPU memory budget to leave headroom:
   ```
   themis-admin config set index.gpu_memory_budget_bytes=2147483648
   ```
3. Restart the index service to reclaim leaked allocations:
   ```
   systemctl restart themis-index
   ```
4. If leaks persist after restart, escalate to the GPU module team —
   confirm `CudaUniquePtr<T>` RAII wrappers are in use on all allocation sites
   (`include/index/cuda_utils.h`).

---

## Scenario 4 — Index Rebuild Stall

**Log pattern:** `[INDEX:RebuildStall]`

### Symptoms

- `themis-admin index status` shows a rebuild stuck at the same percentage
  for more than 5 minutes.
- Alert fires on `index_rebuild_stall_duration_seconds > 300`.
- Trace span `D1-INDEX-REBUILD` shows `progress=stalled`.
- Soak test `IndexSoak_BuildThroughput` reports insert failures.

### Diagnosis

1. Check rebuild progress:
   ```
   themis-admin index status --verbose
   ```
2. Inspect rebuild logs for blocking operations:
   ```
   grep "\[INDEX:RebuildStall\]" /var/log/themis/index.log | tail -30
   ```
3. Check for disk I/O saturation:
   ```
   iostat -x 1 5
   ```
4. Look for thread pool exhaustion:
   ```
   themis-admin metrics dump --filter "index_rebuild_worker"
   ```

### Remediation

1. Cancel the stalled rebuild and restart it:
   ```
   themis-admin index rebuild cancel --component all
   themis-admin index rebuild start --component all --priority low
   ```
2. If disk I/O is the bottleneck, throttle the rebuild I/O rate:
   ```
   themis-admin config set index.rebuild_io_rate_mb_per_sec=100
   ```
3. If thread pool is exhausted, reduce concurrent rebuild workers:
   ```
   themis-admin config set index.rebuild_worker_threads=4
   ```
4. Monitor until completion:
   ```
   watch -n 10 'themis-admin index status'
   ```

---

## Scenario 5 — Multi-GPU Routing Failure

**Log pattern:** `[INDEX:MultiGPURoutingFailed]`

### Symptoms

- Queries intended for GPU 1 are failing while GPU 0 remains healthy.
- Alert fires on `index_multigpu_routing_failure_total > 0`.
- Trace span `D1-INDEX-ROUTING` shows `gpu_id=<n> status=failed`.
- Soak test `IndexSoak_ConcurrentAccessReliability` reports reader errors.

### Diagnosis

1. Check the GPU topology:
   ```
   nvidia-smi topo --matrix
   ```
2. Inspect routing failure logs:
   ```
   grep "\[INDEX:MultiGPURoutingFailed\]" /var/log/themis/index.log | tail -30
   ```
3. Verify each GPU is reachable:
   ```
   themis-admin gpu status --all
   ```
4. Check `AnnFrontdoor` routing configuration:
   ```
   themis-admin config get index.ann_frontdoor_gpu_routing
   ```

### Remediation

1. Route all traffic to the healthy GPU temporarily:
   ```
   themis-admin config set index.ann_frontdoor_gpu_ids=0
   ```
2. If the failing GPU has a driver issue, reset it:
   ```
   nvidia-smi --gpu-reset -i <gpu_id>
   ```
3. Re-enable multi-GPU routing after recovery:
   ```
   themis-admin config set index.ann_frontdoor_gpu_ids=0,1
   ```
4. If the failure is persistent, file a hardware incident and continue
   operating in single-GPU mode.

---

## Escalation

| Severity | Condition | Action |
|----------|-----------|--------|
| P1 | HNSWCorruption on primary shard | Page on-call; quarantine + rebuild |
| P1 | BufferOOM causing index unavailability | Page on-call; restart + reduce batch |
| P2 | GPUKernelFallback > 10 min | Enable CPU fallback; notify GPU team |
| P2 | RebuildStall > 10 min | Cancel and restart rebuild |
| P3 | MultiGPURoutingFailed | Route to single GPU; file hardware incident |

---

## Related Resources

- Soak tests: `tests/integration/test_index_engine_soak.cpp`
- Stress tests: `tests/index/test_index_highcardinality_stress.cpp`
- CUDA utilities: `include/index/cuda_utils.h`
- ANN Frontdoor rollout: `src/index/ANNFRONTDOOR_ROLLOUT.md`
- Wave D acceptance checklist: `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md`
- Index module roadmap: `src/index/ROADMAP.md`
- Architecture: `src/index/ARCHITECTURE.md`
