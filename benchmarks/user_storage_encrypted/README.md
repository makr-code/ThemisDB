# benchmarks/user_storage_encrypted

Performance benchmarks and release gates for the encrypted user storage module.

## Overview

This directory contains benchmark suites that measure and enforce performance expectations for encrypted storage operations (mount, unmount, key derivation, key rotation, encrypted I/O).

## Benchmark Suites

### Phase 5: Release Gate Benchmarks

**File:** `bench_user_storage_encrypted_release_gates.cpp`

Measures low-level error handling and struct allocation costs for the hot path in mount/unmount operations.

| Gate ID      | Benchmark       | Operation                              | Threshold |
|--------------|-----------------|----------------------------------------|-----------|
| GATE-USE-01  | ErrorEnumCast   | Cast UserStorageEncryptedError value   | ≤ 5 ns    |
| GATE-USE-02  | SwitchDispatch  | Switch-based error dispatch            | ≤ 10 ns   |
| GATE-USE-03  | StructAlloc     | EncryptedMountDescriptor allocation    | ≤ 500 ns  |
| GATE-USE-04  | BatchCast       | 1000 mixed error codes (amortised)     | ≤ 5 µs    |

### Phase 5: Lifecycle Benchmarks

**File:** `bench_user_storage_encrypted_lifecycle_gates.cpp`

Measures latency and throughput for encrypted storage lifecycle operations.

| Gate ID      | Benchmark                    | Operation                                          | Threshold        |
|--------------|------------------------------|----------------------------------------------------|------------------|
| USK-P5-01    | MountLatency                 | Mount encrypted container                          | p99 ≤ 500 ms     |
| USK-P5-02    | UnmountLatency               | Unmount encrypted container                        | p99 ≤ 300 ms     |
| USK-P5-03    | KeyDerivationLatency         | Argon2id KDF with standard parameters              | p99 ≤ 1500 ms    |
| USK-P5-04    | KeyRotationLatency           | Full rotation cycle including Vault interaction    | p99 ≤ 10000 ms   |
| USK-P5-05    | ConcurrentMountThroughput    | Mounts/second with 4 concurrent threads            | ≥ 0.5 mounts/s   |
| USK-P5-06    | EncryptedWriteThroughput     | Bytes/second for 1MB, 10MB, 100MB files            | ≥ 50 MB/s        |

## Running Benchmarks

### Build and Run All Benchmarks

```bash
cmake --preset linux-release -D THEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset linux-release --target bench_user_storage_encrypted_release_gates
cmake --build --preset linux-release --target bench_user_storage_encrypted_lifecycle_gates

# Run individual benchmarks
./benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates

./benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates
```

### Run via CTest (Recommended)

```bash
# Run all release-critical performance tests
ctest --preset linux-release -L user_storage_encrypted,release_critical -V

# Run only lifecycle benchmarks
ctest --preset linux-release -L user_storage_encrypted,performance -V
```

## Performance Baselines

### Baseline Environment

All baselines were established on the following system configuration:

| Component     | Value                                   |
|---------------|-----------------------------------------|
| CPU           | Intel/AMD modern multi-core (4+ cores) |
| Memory        | ≥ 8 GB RAM                             |
| Storage       | SSD (NVMe preferred)                    |
| gocryptfs     | Version 1.x+ (with FUSE support)       |
| FUSE          | libfuse3.x+                             |
| OS            | Linux 5.10+ with FUSE kernel support   |

### Baseline P50/P95/P99 Latencies (ms)

These values were established as the reference baselines for regression detection.

#### Mount Latency (USK-P5-01)
- P50: ~50 ms
- P95: ~150 ms
- P99: ~250 ms
- **Gate:** p99 ≤ 500 ms (200% threshold)

#### Unmount Latency (USK-P5-02)
- P50: ~10 ms
- P95: ~50 ms
- P99: ~100 ms
- **Gate:** p99 ≤ 300 ms (300% threshold)

#### Key Derivation Latency (USK-P5-03)
- P50: ~500 ms (Argon2id with standard params)
- P95: ~1000 ms
- P99: ~1200 ms
- **Gate:** p99 ≤ 1500 ms (125% threshold)

#### Key Rotation Latency (USK-P5-04)
- P50: ~2000 ms (includes Vault RPC)
- P95: ~5000 ms
- P99: ~7000 ms
- **Gate:** p99 ≤ 10000 ms (143% threshold)

#### Concurrent Mount Throughput (USK-P5-05)
- Per-thread baseline: ~1.0 mounts/second
- 4-thread aggregate: ~4.0 mounts/second
- **Gate:** ≥ 0.5 mounts/second per thread (50% of baseline)

#### Encrypted Write Throughput (USK-P5-06)
- Baseline (1MB file): ~100 MB/s
- Baseline (10MB file): ~90 MB/s
- Baseline (100MB file): ~80 MB/s
- **Gate:** ≥ 50 MB/s (50-60% of baseline)

### Regression Detection

Regressions are detected when:
- Any p99 latency increases by **> 20%** from baseline
- Throughput decreases by **> 20%** from baseline

For example:
- USK-P5-01 regression if p99 > 300 ms (was ~250 ms)
- USK-P5-06 regression if throughput < 80 MB/s (was ~100 MB/s)

## Baseline Stabilization Procedure

To re-establish baselines after infrastructure changes:

1. Run benchmarks 5-10 times on a stable, isolated system
2. Capture p50/p95/p99 and throughput values for each run
3. Compute mean and standard deviation
4. Update baseline values in this document
5. Document any infrastructure changes (CPU, storage, gocryptfs version, etc.)
6. Commit changes with justification in the commit message

Example:

```bash
# Run multiple iterations for stability analysis
for i in {1..5}; do
    echo "=== Baseline run $i ==="
    ./bench_user_storage_encrypted_lifecycle_gates \
        --benchmark_repetitions=10 \
        --benchmark_report_aggregates_only=false
done
```

## Assumptions and Constraints

- Benchmarks assume gocryptfs and FUSE are available and functional
- Network latency to Vault (if configured) is assumed to be ≤ 10ms
- Benchmarks are designed to measure user-space operations only
- I/O performance may vary based on storage backend (SSD vs. HDD)
- Concurrent benchmarks assume at least 4 CPU cores available

## CTest Labels

All performance benchmarks are tagged with the following CTest labels:

- `user_storage_encrypted`: Module filter
- `release_critical`: Performance gate enforcement
- `performance`: Benchmark category
- `phase5`: Release phase

To run only release-critical tests:

```bash
ctest --preset linux-release -L "user_storage_encrypted.*release_critical" -V
```

## Performance Expectations

### User-Facing Guarantees

- **Mount operation:** Users should expect encrypted containers to mount in <500ms under normal conditions
- **Data I/O:** Encrypted writes should achieve ≥50 MB/s throughput even on slower storage
- **Key rotation:** Rotation should complete transparently with <10s latency (all tiers)

### Operator Expectations

- **Vault integration:** Assume ≤10s latency for key rotation with network overhead
- **Concurrent operations:** System should support ≥0.5 concurrent mounts/second per thread
- **Scalability:** Performance should not degrade with increasing number of encrypted containers

## Future Improvements

- [ ] Extend benchmarks for multi-tenant scenarios (Phase 6)
- [ ] Add benchmark variance tracking and automated regression alerts (Phase 7)
- [ ] Establish GPU-accelerated encryption performance baselines (Phase 8)
- [ ] Profile cache efficiency for high-concurrency workloads (Phase 8)

## References

- [user_storage_encrypted ROADMAP](../../src/user_storage_encrypted/ROADMAP.md)
- [API Contract](../../include/user_storage_encrypted/user_storage_encrypted_api_contract.h)
- [Architecture](../../include/user_storage_encrypted/ARCHITECTURE.md)

