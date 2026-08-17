# Wave D Operability Hardening — Soak Test Runbook

**Document Type:** Wave D Soak Test Execution & Analysis Guide  
**Target:** Q1 2027 (v2.5.0-rc1)  
**Last Updated:** 2026-08-17  
**Status:** Execution Framework Established

---

## 1. Soak Test Suite Overview

Wave D introduces 4 long-duration soak tests validating operability under sustained load and chaos injection:

| Test | Duration | Module | Focus | Gate |
|------|----------|--------|-------|------|
| **Telemetry Exporter Resilience** | 24 hours | Observability | Trace/metric sustained emission + network faults | `W9-SOAK-TELEMETRY-01` |
| **Replication WAL Shipping** | 48 hours | Replication | Multi-region shipping + periodic failover | `W9-SOAK-REPLICATION-01` |
| **Sharding Topology Changes** | 48 hours | Sharding | 8+ shard writes + rebalance injection | `W9-SOAK-SHARDING-01` |
| **Mixed Acceleration Workloads** | 24 hours | GPU/Compute | Sustained GPU + fallback injection | `W9-SOAK-ACCELERATION-01` |

---

## 2. Pre-Execution Checklist

### Environment Preparation
- [ ] Verify representative hardware profile matches `baseline_freeze` in `release_gate_manifest_wave_d.json`
- [ ] Confirm OS image: `ubuntu-22.04`
- [ ] Verify compiler: `clang-17` with flags `-O3 -march=native -DNDEBUG`
- [ ] Ensure disk space: ≥100 GB for streaming logs/metrics
- [ ] Verify network: stable connection to exporter backend (if applicable)
- [ ] Confirm runner resource availability:
  - CPU: ≥8 cores reserved for test process
  - Memory: ≥16 GB free
  - I/O: SSD preferred for benchmark result writes

### Build Configuration
```bash
# Configure with Wave D observability enabled
cmake --preset linux-release \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_WAVE_D_OBSERVABILITY=ON \
  -DTHEMIS_ENABLE_SOAK_TESTS=ON
```

### Baseline Collection
Before running soak tests:
1. Run Wave 7 baseline reference benchmarks to establish p95/p99 latency baselines
2. Capture system metrics: CPU model, memory speed, disk type, network bandwidth
3. Document any known system constraints (thermal limits, power management, network QoS)

---

## 3. Test Execution Procedures

### 3.1 Telemetry Exporter Resilience (24h)

**Location:** `benchmarks/wave9/test_wave9_otel_24h_soak.cpp`  
**Gate:** `W9-SOAK-TELEMETRY-01`

#### Setup
```bash
# Build soak test
cmake --build build --target test_wave9_otel_24h_soak -j$(nproc)

# Optional: Configure duration override for CI smoke test (default 24h)
export SOAK_DURATION_HOURS=2  # For 2-hour CI run
```

#### Execution
```bash
# Run with streaming result collection
./build/benchmarks/wave9/test_wave9_otel_24h_soak \
  --benchmark_min_time=0 \
  --benchmark_duration=86400 \
  --benchmark_out=results/telemetry_24h_soak.json \
  --benchmark_result_format=json \
  --soak_streaming_output=results/telemetry_24h_streaming.log
```

#### Validation During Run
- Monitor throughput: expect ≥500 spans/sec sustained
- Check memory growth: should plateau <256 MB
- Verify recovery: network faults injected at random; recovery should be <100 ms
- Inspect logs every 4 hours for anomalies

#### Success Criteria
- ✅ Data loss <0.1%
- ✅ Recovery time ≤100 ms (average from fault injection events)
- ✅ Memory stable (no unbounded growth)
- ✅ Gate `W9-SOAK-TELEMETRY-01` PASS

---

### 3.2 Replication WAL Shipping & Lag Tracking (48h)

**Location:** `benchmarks/wave9/test_wave9_replication_48h_soak.cpp`  
**Gate:** `W9-SOAK-REPLICATION-01`

#### Setup
```bash
# Build soak test
cmake --build build --target test_wave9_replication_48h_soak -j$(nproc)

# Verify multi-region simulation is enabled
export SOAK_REGIONS=3  # Simulate 3 geographic regions
```

#### Execution
```bash
# Run with lag tracking and failover injection
./build/benchmarks/wave9/test_wave9_replication_48h_soak \
  --benchmark_min_time=0 \
  --benchmark_duration=172800 \
  --benchmark_out=results/replication_48h_soak.json \
  --soak_lag_tracking=true \
  --soak_failover_injection_frequency_min=30 \
  --soak_streaming_output=results/replication_48h_streaming.log
```

#### Validation During Run
- Monitor lag: p95 should stay <50 ms, p99 <100 ms
- Track failover events: expect one per 30 minutes (96 total over 48h)
- Verify recovery: after each failover, lag should recover within target time
- Inspect consistency proofs every 6 hours

#### Success Criteria
- ✅ Lag recovery success ≥99.9% (≥95 of 96 failovers successful)
- ✅ P95 lag ≤50 ms, P99 lag ≤100 ms
- ✅ Recovery time ≤5 seconds (average)
- ✅ Data consistency verified (checksum validation)
- ✅ Gate `W9-SOAK-REPLICATION-01` PASS

---

### 3.3 Sharding Topology Changes & Rebalance (48h)

**Location:** `benchmarks/wave9/test_wave9_sharding_topology_soak.cpp`  
**Gate:** `W9-SOAK-SHARDING-01`

#### Setup
```bash
# Build soak test
cmake --build build --target test_wave9_sharding_topology_soak -j$(nproc)

# Configure shard count and rebalance frequency
export SOAK_SHARD_COUNT=8
export SOAK_REBALANCE_FREQUENCY_MIN=30  # Rebalance every 30 minutes
```

#### Execution
```bash
# Run with write throughput tracking and rebalance monitoring
./build/benchmarks/wave9/test_wave9_sharding_topology_soak \
  --benchmark_min_time=0 \
  --benchmark_duration=172800 \
  --benchmark_out=results/sharding_48h_soak.json \
  --soak_rebalance_tracking=true \
  --soak_exactness_validation=true \
  --soak_streaming_output=results/sharding_48h_streaming.log
```

#### Validation During Run
- Monitor rebalance time: each should complete in <30 seconds
- Track stalls: should be zero stalls >5 seconds
- Verify write latency: p95 should stay <10 ms during normal operation, p99 <25 ms
- Validate exactness: check distributed write guarantees after each rebalance

#### Success Criteria
- ✅ Zero stall events (write latency stalls >5 seconds = 0)
- ✅ Write latency p95 ≤10 ms, p99 ≤25 ms (baseline)
- ✅ Rebalance time ≤30 seconds per topology change
- ✅ Exactness guarantee maintained (verified checksums post-rebalance)
- ✅ Gate `W9-SOAK-SHARDING-01` PASS

---

### 3.4 Mixed Acceleration Workloads (24h)

**Location:** `benchmarks/wave9/test_wave9_mixed_acceleration_soak.cpp`  
**Gate:** `W9-SOAK-ACCELERATION-01`

#### Setup
```bash
# Build soak test
cmake --build build --target test_wave9_mixed_acceleration_soak -j$(nproc)

# Verify GPU fallback injection is configured
export SOAK_FALLBACK_INJECTION_FREQUENCY_MIN=30  # Inject every 30 minutes
export SOAK_GPU_BACKEND=all  # Test all available backends (CUDA, HIP, Vulkan)
```

#### Execution
```bash
# Run with GPU health monitoring and fallback tracking
./build/benchmarks/wave9/test_wave9_mixed_acceleration_soak \
  --benchmark_min_time=0 \
  --benchmark_duration=86400 \
  --benchmark_out=results/acceleration_24h_soak.json \
  --soak_gpu_monitoring=true \
  --soak_fallback_tracking=true \
  --soak_streaming_output=results/acceleration_24h_streaming.log
```

#### Validation During Run
- Monitor GPU health: CUDA/HIP/Vulkan call errors should trigger fallback
- Track fallback recovery: should complete within 500 ms
- Verify CPU degradation: performance drop should be <20% vs GPU baseline
- Monitor thermal stability: GPU should not exceed safe operating temperature

#### Success Criteria
- ✅ Fallback recovery time ≤500 ms (average from injection events)
- ✅ CPU degradation ≤15% (vs GPU baseline throughput)
- ✅ Memory utilization stable <70%
- ✅ Thermal stability verified (no GPU thermal throttling)
- ✅ Gate `W9-SOAK-ACCELERATION-01` PASS

---

## 4. Post-Execution Analysis

### Results Collection
```bash
# Gather all soak test results
mkdir -p analysis/soak_results
cp results/*.json analysis/soak_results/
cp results/*.log analysis/soak_results/

# Run variance and regression analysis
python3 benchmarks/scripts/report_variance.py \
  --baseline benchmarks/wave7/release_gate_manifest_w7.json \
  --results analysis/soak_results/ \
  --output analysis/wave_d_soak_report.json
```

### Gate Verification
```bash
# Execute gate verification script
python3 benchmarks/scripts/verify_wave_d_gates.py \
  --manifest benchmarks/wave9/release_gate_manifest_wave_d.json \
  --results analysis/soak_results/ \
  --output analysis/gate_verification.json
```

### Performance Baselines
Document and save baselines for future regression detection:
- **Latency:** p50, p95, p99 for each workload
- **Throughput:** ops/sec for all test scenarios
- **Resource Usage:** CPU %, memory (peak and average), disk I/O
- **Recovery Times:** lag recovery, failover recovery, fallback recovery
- **Stability Metrics:** variance (CV %), thermal stability, stall counts

---

## 5. Troubleshooting Guide

| Symptom | Likely Cause | Investigation | Action |
|---------|--------------|-----------------|--------|
| High data loss (>0.5%) in TELEMETRY-01 | Exporter throughput insufficient | Check exporter queue depth, network latency | Increase buffer size, reduce emission rate for smoke test |
| Lag exceeds target in REPLICATION-01 | Network congestion or WAL I/O bottleneck | Monitor network saturation, disk I/O utilization | Reduce failover injection frequency, verify NIC bandwidth |
| Write stalls >5s in SHARDING-01 | Rebalance deadlock or lock contention | Inspect rebalance logs, check lock wait times | Increase rebalance timeout, add per-shard isolation |
| Fallback recovery >500ms in ACCELERATION-01 | GPU recovery blocked by current workload | Monitor GPU queue depth, kernel execution time | Implement preemption, reduce GPU workload intensity |
| Memory growth unbounded in any test | Memory leak in instrumentation or test harness | Enable leak detector (`ASAN_OPTIONS=detect_leaks=1`), profile heap | Identify allocation site, fix allocator or instrumentation |

---

## 6. Release Criteria Summary

**Wave D soak tests are release-blocking.** All gates must PASS before promotion to v2.5.0-rc1:

- ✅ `W9-SOAK-TELEMETRY-01`: Data loss <0.1%, recovery <100 ms
- ✅ `W9-SOAK-REPLICATION-01`: Lag recovery ≥99.9%, lag <100 ms p99
- ✅ `W9-SOAK-SHARDING-01`: Zero stalls >5 sec, exactness maintained
- ✅ `W9-SOAK-ACCELERATION-01`: Fallback recovery <500 ms, CPU degradation <20%

**Evidence Retention:** All soak test results, streaming logs, and gate verification reports are retained for 12 months for audit and regression analysis.

---

**Document Owner:** platform-release@themisdb  
**Next Update:** Phase 4 completion (2026-11-30)
