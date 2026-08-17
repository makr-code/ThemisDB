# Wave A Batches A2–A5 — Parallel Execution Action Plans

**Status:** ⏸️ Blocked on A1 exit criteria PASS  
**Entry:** Gated by A1 completion + Wave A entry criteria validation  
**Timeline:** Parallel execution from 2026-09-04 through 2026-10-02  
**Critical Path:** A1 → (A2 || A3 || A4 || A5) → Wave A Exit Criteria → Wave B

---

## Overview: Wave A Exit Gate Model

Wave A Batches A2–A5 execute in **parallel** after A1 completes, but all must achieve exit criteria before Wave B can start:

```
A1 PASS
  ├─→ A2 (Replication) ────────────→ Geo placement + WAL shipping
  ├─→ A3 (Voice) ──────────────────→ Fail-closed hardening
  ├─→ A4 (GPU) ────────────────────→ CUDA safety + fallback
  └─→ A5 (Sharding) ───────────────→ Multi-shard + stress

    [Wave A Exit Criteria Validation]
                  ↓
            Wave B Entry
```

---

## Batch A2 — Replication Geographic Placement + WAL Lag Controls

**Module:** `src/replication/`  
**Timeline:** 2026-09-04 → 2026-09-25  
**Owner:** TBD  

### Acceptance Criteria Summary

| Criterion | Target | Verification |
|-----------|--------|--------------|
| Geographic placement API callable | ✅ | `replication::PlacementPolicy::placement_for_region()` returns valid policy |
| WAL cross-region shipping works | ✅ | Async WAL ships to replica within 5s |
| Lag alerts fire correctly | ✅ | Alert triggers within 500ms of lag > threshold |
| Failover diagnostics improved | ✅ | Root-cause tracing for replication lag |
| `release_critical` CI passes | ✅ | Workflow green on `develop` |

### Phase 1: Design + API (Week of 2026-09-04)

**Task A2-1: Geo Placement Policy Design**
- Create `PlacementPolicy` interface spec
  - Input: `Region` enum + `Replica::role` + `distance_ms` + `network_quality`
  - Output: `PlacementDecision {primary_region, replicas[]}`
  - Constraints: Primary in preferred region; ≥2 replicas; cross-region spread
- Create test spec: 10-20 scenarios covering region availability, network degradation
- Owner: TBD
- Target: 2026-09-11
- Deliverable: `src/replication/GEO_PLACEMENT_POLICY_SPEC.md`

**Task A2-2: WAL Shipping Architecture**
- Design async WAL shipping pipeline
  - Input: WAL segment completed on primary
  - Output: Replica receives segment + acknowledges within SLA
  - Target RPO: 5s (RTO-derived)
- Define durability model: O_SYNC, buffer strategy, retry behavior
- Define lag measurement: write timestamp vs receive timestamp
- Owner: TBD
- Target: 2026-09-11
- Deliverable: `src/replication/ASYNC_WAL_SHIPPING_ARCH.md`

**Task A2-3: Lag Alert Integration Spec**
- Design alert system: when lag(now) - lag(5s_ago) > threshold
- Alert payload: `{replica_id, current_lag_ms, threshold_ms, rate_of_change}`
- SLA: alert delivered within 500ms of threshold breach
- Define thresholds: warning (1s), critical (5s), max acceptable (30s)
- Owner: TBD
- Target: 2026-09-11
- Deliverable: `src/replication/LAG_ALERT_SPEC.md`

### Phase 2: Implementation (Weeks of 2026-09-11, 2026-09-18)

**Task A2-4: Geographic Placement Policy Implementation**
- Implement `PlacementPolicy::placement_for_region()`
  - Handle single-region, multi-region, region-unavailable cases
  - Preference scoring: distance, network quality, availability
  - Fallback strategy: if preferred region unavailable, select next-best
- Add config: `replication.geo.preferred_regions`, `replication.geo.min_replicas`
- Owner: TBD
- Target: 2026-09-18
- Test Coverage: 15 unit tests (region selection, fallback, edge cases)

**Task A2-5: Async WAL Shipping Implementation**
- Implement `WALShippingManager::async_ship_wal_segment()`
  - Spawn background task per replica
  - Retry logic: exponential backoff; max 3 retries
  - Durability: log shipped segment to persist queue if replica unavailable
  - Timeout: per-segment deadline 5s (fail-closed to persist queue)
- Integrate with replication manager: hook on WAL segment completion
- Owner: TBD
- Target: 2026-09-25
- Test Coverage: 12 integration tests (normal path, timeouts, replica failures, durability)

**Task A2-6: Lag Alert Implementation**
- Implement `LagMonitor::check_replica_lag()`
  - Measure: (current_timestamp - remote_wal_timestamp)
  - Compare against configurable thresholds (warning, critical, max)
  - Fire alert via `DiagnosticEmitter::emit_lag_alert()`
- Alert integration: hook into observability pipeline
- Owner: TBD
- Target: 2026-09-18
- Test Coverage: 8 unit tests (lag calculation, alert firing, threshold edge cases)

### Phase 3: Testing + Verification (Week of 2026-09-18)

**Task A2-7: Integration Test Suite**
- Create `test_replication_geo_placement.cpp` (8 tests)
  - Test placement for 3-region setup
  - Test fallback when region unavailable
  - Test replica selection scoring
- Create `test_replication_wal_shipping_async.cpp` (10 tests)
  - Test WAL ships within 5s SLA
  - Test durability queue fallback on timeout
  - Test retry logic + exponential backoff
  - Test crash recovery of pending WAL
- Create `test_replication_lag_alerts.cpp` (6 tests)
  - Test alert fires at warning threshold
  - Test alert fires at critical threshold
  - Test alert payload contains correct values
  - Test alert timing ≤ 500ms from breach
- Owner: TBD
- Target: 2026-09-25
- Evidence Location: `src/replication/A2_INTEGRATION_TEST_RESULTS.md`

**Task A2-8: Failover Diagnostics Hardening**
- Enhance failover log/trace: include replication lag details
  - Log: `[FAILOVER] Trigger reason: [lag_ms={current}, threshold={max}, rate={delta_per_sec}]`
  - Trace: root cause chain (primary down → lag increases → replica ready → promotion)
- Add observability: metrics for lag spike detection
- Owner: TBD
- Target: 2026-09-25
- Evidence Location: `src/replication/A2_FAILOVER_DIAGNOSTICS_VALIDATION.md`

### Phase 4: Gate Validation (Week of 2026-09-25)

**Task A2-9: `release_critical` CI Pass**
- Merge A2 implementation commits to `develop`
- Run: `.github/workflows/09-pr-gates_release-critical-tests.yml`
- Verify: All replication-related tests pass
- Owner: TBD
- Target: 2026-09-25
- Success: Exit 0; no flakes
- Evidence Location: `src/replication/A2_CI_VALIDATION.md`

---

## Batch A3 — Voice Fail-Closed Hardening

**Module:** `src/voice/`  
**Timeline:** 2026-09-04 → 2026-09-25  
**Owner:** TBD  

### Acceptance Criteria Summary

| Criterion | Target | Verification |
|-----------|--------|--------------|
| Stream validation rejects malformed | ✅ | All 10 malformed formats rejected |
| Stream size limits enforced | ✅ | Oversized streams error; cleanup bounded |
| Liveness check accurate under adversarial load | ✅ | 0 false positives in 10K spoofed sessions |
| Anti-spoof regressions verified | ✅ | 0/10K legitimate sessions rejected |
| Multi-session teardown safe | ✅ | 0 resource leaks under cascading errors |
| `release_critical` CI passes | ✅ | Workflow green on `develop` |

### Phase 1: Stream Validation Implementation (Week of 2026-09-04)

**Task A3-1: Malformed Stream Detection**
- Identify 10 malformed format categories:
  1. Invalid codec (not supported codec)
  2. Invalid sample rate (not power-of-2 or out of range)
  3. Invalid channel count (0 or > 8)
  4. Invalid frame size (0, negative, or huge)
  5. Invalid duration (0, negative)
  6. Missing required header fields
  7. Truncated/incomplete frame
  8. Corrupt checksum
  9. Buffer underrun in stream
  10. Invalid metadata fields
- Implement validation in `VoiceStreamValidator::validate_format()`
- Return error before any processing (fail-closed)
- Owner: TBD
- Target: 2026-09-11
- Test Coverage: `test_voice_stream_validation.cpp` (10 tests, 1 per format)

**Task A3-2: Stream Size Limits**
- Define limits:
  - Max frame size: 64KB
  - Max total stream duration: 10 minutes
  - Max concurrent streams per session: 100
  - Max session-total memory: 500MB
- Implement enforcement in `VoiceStreamHandler::receive_frame()`
- Guarantee bounded cleanup if limit exceeded (no unbounded buffer allocation)
- Owner: TBD
- Target: 2026-09-11
- Test Coverage: `test_voice_stream_size_limits.cpp` (8 tests: max frame, max duration, concurrency, memory)

### Phase 2: Anti-Spoof + Liveness Regressions (Week of 2026-09-11)

**Task A3-3: Adversarial Liveness Testing**
- Create 1000-session spoofed load:
  - 500 sessions: repeated same audio chunk (spoof attempt)
  - 300 sessions: synthetic "live-like" audio (gradient noise)
  - 100 sessions: legitimate live sessions (true variable audio)
  - 100 sessions: dead channel (silence / zeros)
- Run liveness check 10 times on each category
- Verify: 0 false positives (legitimate marked as spoof)
- Measure: False negative rate (spoofs detected)
- Owner: TBD
- Target: 2026-09-18
- Evidence Location: `src/voice/A3_LIVENESS_ADVERSARIAL_RESULTS.md` (FPR, FNR, ROC curve)

**Task A3-4: Anti-Spoof Regression Suite**
- Create `test_voice_antispoof_regressions.cpp` (10 tests)
  - Test 1: Exact replay detection (detect repeated frames)
  - Test 2: Gradient-noise spoofing (synthetic but "alive-looking")
  - Test 3: Audio fragment splicing (A + B + A repeated)
  - Test 4: Pitch-shift spoofing (replay at different speed)
  - Test 5: Echo cancellation bypass (speaker playback + mic)
  - Test 6: Deepfake audio (synthetic but human-like)
  - Test 7: Background noise injection (mask real audio)
  - Test 8: Timing jitter spoofing (replay with time variation)
  - Test 9: Silence + burst (all-silence then audio burst)
  - Test 10: Multi-dialect spoofing (pre-recorded different accent)
- Owner: TBD
- Target: 2026-09-18

### Phase 3: Multi-Session Teardown Safety (Week of 2026-09-18)

**Task A3-5: Multi-Session Cascading Error Scenario**
- Setup: 50 concurrent voice sessions
- Inject: Cascade errors:
  - Session 1: codec error at frame 100
  - Session 2: network timeout at frame 50
  - Session 3: memory pressure (allocator returns nullptr)
  - Sessions 4-50: continue normally
- Verify during cascade:
  - Sessions 4-50 unaffected (isolation)
  - Sessions 1-3 clean up without cascading further (no resource leaks)
  - All 50 sessions eventually close
  - No dangling file handles, memory leaks, or orphaned locks
- Measure: Peak memory during cascade (should not spike 2x)
- Owner: TBD
- Target: 2026-09-25
- Test Coverage: `test_voice_multisession_teardown.cpp` (8 tests for different cascade patterns)

**Task A3-6: Session Cleanup Audit**
- Audit cleanup paths in `VoiceSession::close()`
  - File handle: closed ✅
  - Memory buffers: deallocated ✅
  - Thread state: joined ✅
  - Locks: released ✅
  - Observer callbacks: unregistered ✅
- Verify with ASAN/Valgrind: zero leaks under 1000 session open/close cycles
- Owner: TBD
- Target: 2026-09-25
- Evidence Location: `src/voice/A3_SESSION_CLEANUP_AUDIT.md` (ASAN/Valgrind reports)

### Phase 4: Gate Validation (Week of 2026-09-25)

**Task A3-7: `release_critical` CI Pass**
- Merge A3 implementation commits to `develop`
- Run: `.github/workflows/09-pr-gates_release-critical-tests.yml`
- Verify: All voice-related tests pass
- Owner: TBD
- Target: 2026-09-25

---

## Batch A4 — GPU CUDA Safety + RAII + Timeouts

**Module:** `src/gpu/`  
**Timeline:** 2026-09-04 → 2026-09-25  
**Owner:** TBD  

### Acceptance Criteria Summary

| Criterion | Target | Verification |
|-----------|--------|--------------|
| All CUDA calls checked | ✅ | No unchecked `cuda*` calls; all wrapped |
| RAII lifecycle verified | ✅ | Deterministic cleanup on all paths |
| Kernel timeouts enforced | ✅ | Kernel timeout ≤ 30s; CPU fallback on timeout |
| CPU fallback path correct | ✅ | Output matches GPU output (within FP tolerance) |
| No GPU resource leaks | ✅ | 0 leaks under 1000 kernel timeouts |
| `release_critical` CI passes | ✅ | Workflow green on `develop` |

### Phase 1: CUDA Call Audit + Wrapping (Week of 2026-09-04)

**Task A4-1: Audit Unchecked CUDA Calls**
- Scan `src/gpu/` for all `cuda*` function calls
- Categorize:
  - Checked (error return tested): ✅ ignore
  - Unchecked (no error test): ❌ list for wrapping
  - Asserted (debug only): ⚠️ upgrade to error-aware
- Target unchecked categories:
  - `cudaMalloc`, `cudaFree`, `cudaMemcpy` (memory management)
  - `cudaLaunchKernel`, `cudaKernelLaunchCapture` (execution)
  - `cudaDeviceSynchronize`, `cudaStreamSynchronize` (sync)
  - `cudaEventRecord`, `cudaEventElapsedTime` (timing)
- Owner: TBD
- Target: 2026-09-11
- Evidence Location: `src/gpu/A4_UNCHECKED_CALLS_AUDIT.md` (list + locations)

**Task A4-2: CUDA Error Wrapper Implementation**
- Create `CudaErrorHandler::check_error(cudaError_t err, const char* context)`
  - If error: log + throw `CudaException(err, context)`
  - If success: return cudaError_t::cudaSuccess
- Create `CudaMemoryGuard` RAII wrapper:
  - Constructor: allocate memory via `cudaMalloc`
  - Destructor: `cudaFree` (always called)
  - Move semantics for ownership transfer
- Create `CudaStreamGuard` RAII wrapper:
  - Constructor: create stream via `cudaStreamCreate`
  - Destructor: destroy stream
  - Ensures deterministic cleanup
- Owner: TBD
- Target: 2026-09-18
- Test Coverage: `test_gpu_cuda_error_handling.cpp` (10 tests for error paths)

### Phase 2: RAII Lifecycle + Timeouts (Week of 2026-09-11)

**Task A4-3: Kernel Launch RAII Wrapper**
- Create `CudaKernelGuard`:
  - Tracks kernel launch, execution state, timeout
  - Constructor: launch kernel + record start event
  - Destructor: ensure synchronization complete; clean up resources
  - Enforces timeout: `cudaEventElapsedTime` ≤ 30s
  - Timeout path: triggers CPU fallback (no exception, silent handoff)
- Integrate with all kernel launches: geospatial, tensor ops, sort, filter
- Owner: TBD
- Target: 2026-09-18
- Test Coverage: `test_gpu_kernel_raii_lifecycle.cpp` (12 tests)

**Task A4-4: CPU Fallback Integration**
- For each GPU kernel, implement CPU-equivalent algorithm
  - Geospatial distance: CPU distance matrix calculation
  - Tensor operations: CPU matrix multiply / elementwise ops
  - Sort: CPU quicksort / std::sort
  - Filter: CPU linear scan + filter
- Add `is_gpu_available()` check:
  - If GPU unavailable at runtime: silently fallback to CPU
  - If kernel timeout: fallback to CPU
  - Transparent to caller (same output expected)
- Owner: TBD
- Target: 2026-09-18

### Phase 3: Testing + Verification (Week of 2026-09-18)

**Task A4-5: Deterministic Cleanup Under Errors**
- Create `test_gpu_lifecycle_under_errors.cpp` (10 tests)
  - Test 1: Normal kernel success → cleanup
  - Test 2: Kernel timeout → fallback → cleanup
  - Test 3: Allocation failure (nullptr) → cleanup without crash
  - Test 4: Synchronization timeout → fallback + cleanup
  - Test 5: Device reset during execution → error + cleanup
  - Test 6: Concurrent kernels, one times out → timeouts cleanup, others continue
  - Test 7: Repeated timeout cycles (100×): no resource exhaustion
  - Test 8: Memory pressure during cleanup (low VRAM) → bounded cleanup
  - Test 9: Exception in cleanup path (rare) → no double-free
  - Test 10: Destructor-driven cleanup (no explicit close) → automatic
- Owner: TBD
- Target: 2026-09-25

**Task A4-6: GPU vs CPU Parity Validation**
- Create `test_gpu_cpu_parity.cpp` (8 tests)
  - Test 1: Geospatial distance matrix (1000 points → verify CRC match)
  - Test 2: Tensor matrix multiply (100×100 @ float32 → verify FP tolerance)
  - Test 3: Sort operation (10K integers → verify order match)
  - Test 4: Filter operation (1M elements, 50% pass rate → verify selectivity)
  - Test 5: Combined workload (distance + sort + filter → verify pipeline output)
  - Test 6: Edge cases (0 input, 1 input, max size → verify handling)
  - Test 7: Numeric stability (FP rounding → verify within epsilon)
  - Test 8: Performance sanity check (GPU ≥ CPU speedup or close parity)
- Owner: TBD
- Target: 2026-09-25

**Task A4-7: Resource Leak Detection**
- Run test suite under CUDA-MEMCHECK (or equivalent profiler)
  - `cuda-memcheck --leak-check full <test_binary>`
- Create `test_gpu_leak_detection.cpp`:
  - Execute 1000 consecutive kernel launches with timeouts
  - Measure peak VRAM; should not accumulate unbounded
  - Verify: peak memory < 1.5× single-kernel requirement
- Owner: TBD
- Target: 2026-09-25
- Evidence Location: `src/gpu/A4_LEAK_DETECTION_REPORT.md` (CUDA-MEMCHECK output)

### Phase 4: Gate Validation (Week of 2026-09-25)

**Task A4-8: `release_critical` CI Pass**
- Merge A4 implementation commits to `develop`
- Run: `.github/workflows/09-pr-gates_release-critical-tests.yml`
- GPU CI consideration: may run on CPU-only runner (CUDA unavailable)
  - Expect CPU fallback path to execute; verify correctness
  - If self-hosted GPU runner available: additional GPU-specific tests
- Owner: TBD
- Target: 2026-09-25

---

## Batch A5 — Sharding Multi-Shard + Rebalance + Distributed Write Stress

**Module:** `src/sharding/`  
**Timeline:** 2026-09-04 → 2026-10-02  
**Owner:** TBD  

### Acceptance Criteria Summary

| Criterion | Target | Verification |
|-----------|--------|--------------|
| Multi-shard exact-path consistency | ✅ | 1M writes → 1M reads; all data verified |
| Topology rebalance transparent | ✅ | Shard add/remove without write downtime |
| Latency-aware routing works | ✅ | Fallback to low-latency shard within 1s |
| Distributed write stress passed | ✅ | 24h continuous; 0 corruption |
| Long-run stability verified | ✅ | 1M writes across 3 shards; exact-once |
| `release_critical` CI passes | ✅ | Workflow green on `develop` |

### Phase 1: Multi-Shard Exact-Path Gating (Weeks of 2026-09-04, 2026-09-11)

**Task A5-1: Exact-Path Consistency Model**
- Define: Write to all 3 shards; read from any shard → all return same value
- Implement: `MultiShardWriter::write_all_shards(key, value)`
  - Sequence: shard 1 → shard 2 → shard 3 (ordered)
  - Atomicity: all-or-nothing (no partial writes on fail)
  - Rollback: if shard 2 fails, rollback shard 1
- Implement: `MultiShardReader::read_consistent(key)` 
  - Read from primary; verify replicas have same value
  - If mismatch detected: trigger reconciliation + log
- Owner: TBD
- Target: 2026-09-18
- Test Coverage: `test_sharding_exact_path_consistency.cpp` (10 tests)

**Task A5-2: Verification Gate: 1M Writes → Reads**
- Write phase: 1M sequential keys to all 3 shards
  - Track: write timestamp per key
  - Verify: all shards acknowledge
- Read phase: 1M reads from random shard
  - Verify: all keys present
  - Verify: all values match original
  - Verify: no extra keys (exactly 1M)
- Measure: read latency (should be <100ms per key)
- Owner: TBD
- Target: 2026-09-25
- Test Coverage: `test_sharding_1m_write_read_gate.cpp` (1 test, ~10 min runtime)

### Phase 2: Topology Change Auto-Rebalance (Weeks of 2026-09-11, 2026-09-18)

**Task A5-3: Add Shard Rebalancing**
- Implement: `ShardingManager::add_shard(new_shard_id)`
  - Rebalance trigger: automatically move keys to new shard
  - No write downtime: writes continue to old shards during rebalance
  - Completion SLA: rebalance finalizes within 300s
  - Verification: key → shard mapping updated atomically
- Implement: `ShardingManager::remove_shard(old_shard_id)`
  - Migrate keys from old → remaining shards
  - No write downtime
  - No key loss
- Owner: TBD
- Target: 2026-09-25
- Test Coverage: `test_sharding_rebalance_add_remove.cpp` (8 tests)

**Task A5-4: Latency-Aware Routing**
- Implement: `ShardRouter::route_with_latency_awareness(key)`
  - Measure latency to each shard (e.g., via ping)
  - Rank shards by latency (low to high)
  - Route to lowest-latency shard
  - Timeout: if primary latency > 1s, fallback to 2nd-ranked shard
- Fallback logic: deterministic, reproducible
- Owner: TBD
- Target: 2026-09-25
- Test Coverage: `test_sharding_latency_aware_routing.cpp` (6 tests)

### Phase 3: Distributed Write Stress (Weeks of 2026-09-18, 2026-09-25)

**Task A5-5: Write Stress Test Suite**
- Create `test_sharding_distributed_write_stress.cpp`:
  - Duration: 24 hours continuous
  - Load: 1M total writes distributed across 3 shards
  - Concurrency: 100 concurrent writers
  - Failure injection: random shard failures (1% rate), recovery within 30s
  - Verify:
    - All 1M keys present at end
    - Zero data corruption (CRC check)
    - Zero message loss (count mismatch)
    - Zero orphaned locks (lingering state)
  - Measure: throughput (keys/sec), latency p95/p99
- Owner: TBD
- Target: 2026-10-02 (24h test, started 2026-09-25)
- Evidence Location: `src/sharding/A5_24H_STRESS_TEST_RESULTS.md`

**Task A5-6: Long-Run Stability Validation**
- Run on representative hardware (e.g., production-like config)
- Monitor: CPU, memory, disk I/O, network during 24h test
- Alerting: if any metric anomaly, log + investigate
- Pass criteria: 
  - Memory stable (no leak)
  - CPU even distribution (no contention/deadlock)
  - Disk I/O bounded (no runaway)
  - Network traffic steady (no cascading failures)
- Owner: TBD
- Target: 2026-10-02

### Phase 4: Gate Validation (Week of 2026-10-02)

**Task A5-7: `release_critical` CI Pass**
- Merge A5 implementation commits to `develop`
- Run: `.github/workflows/09-pr-gates_release-critical-tests.yml`
- Verify: All sharding-related tests pass (may take 24+ hours for stress test)
- Owner: TBD
- Target: 2026-10-02

---

## Wave A Exit Criteria Validation (Week of 2026-10-02)

Once all A1-A5 are PASS, **Wave Orchestrator** validates:

| Criterion | Verification | Owner |
|-----------|--------------|-------|
| A1 build/run complete + chaos evidence signed off | Check `src/transaction/WAVE_A1_CLOSURE_EVIDENCE_BUNDLE.md` | A1 owner |
| A2 geo placement + WAL shipping + alerts delivered | Check `src/replication/A2_INTEGRATION_TEST_RESULTS.md` | A2 owner |
| A3 stream validation + anti-spoof + multi-session safe | Check `src/voice/A3_LIVENESS_ADVERSARIAL_RESULTS.md` | A3 owner |
| A4 CUDA safety + RAII + CPU fallback verified | Check `src/gpu/A4_LEAK_DETECTION_REPORT.md` | A4 owner |
| A5 multi-shard + rebalance + 24h stress verified | Check `src/sharding/A5_24H_STRESS_TEST_RESULTS.md` | A5 owner |
| Deterministic chaos evidence for all 5 | All evidence blocks filed | Wave Orchestrator |
| Fail-closed behavior verified | All path audits complete | Wave Orchestrator |
| `release_critical` CI green | All 5 modules PASS | CI team |
| Representative-hardware p95/p99 baselines | Benchmark reports filed | Ops team |

**Gate Pass:** If all ✅, unblock Wave B start  
**Gate Fail:** If any ❌, identify root cause + remediation plan before Wave B

---

## Master Schedule

| Week | Mon | Wed | Fri |
|------|-----|-----|-----|
| 08/14 | A1 build start | A1-1/2 pending | A2-A5 plans ready |
| 08/21 | A1-3/4 run verify | A1-5 build verify | Gate decision |
| 08/28 | A1-6 run + chaos plan | Chaos start (AC-6) | A2-A5 kick-off (if A1 PASS) |
| 09/04 | A2 design / A3 validation / A4 audit / A5 design | Chaos week mid-point | Chaos evidence review |
| 09/11 | A2 impl start / A3 impl / A4 RAII / A5 multi-shard | Impl week mid-point | A2-A5 integration tests |
| 09/18 | A2-A5 testing | A2-A5 gate validation | CI run / A5 stress start |
| 09/25 | A2-A5 CI pass | Wave A exit review | Documentation + closure |
| 10/02 | Wave A EXIT CRITERIA PASS | B1-B3 kickoff | Wave B start |

---

**Last Updated:** 2026-08-14  
**Next Sync:** Upon A1 completion (targeting 2026-08-21)
