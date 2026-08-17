# Wave A Sharding Closure — Build & CI Validation Checklist

**Date**: 2026-08-17 18:50 UTC
**Purpose**: Pre-merge validation for Wave A closure artifacts

---

## Build Environment Validation

### CMake Configuration Check
- [ ] `community-release` preset configures without RocksDB (if required)
- [ ] `community-release-allow-missing-rocksdb` allows graceful RocksDB degradation
- [ ] fmt library dependency verified
- [ ] All sharding includes compile without errors

### Preset Recommendations
**For Sandboxed Environments** (no sudo, limited packages):
```bash
# Diagnostic preset (RocksDB optional, fmt required)
cmake --preset community-release-allow-missing-rocksdb -B build

# Docker fallback (full dependencies)
docker build -f docker/Dockerfile.unified -t themisdb-wave-a-build .
docker run -v $(pwd):/workspace -w /workspace themisdb-wave-a-build \
  cmake --preset linux-release -B build
```

**For Full Dev Environments** (vcpkg available):
```bash
# Full feature set
cmake --preset linux-release -B build
```

---

## Test Execution Strategy

### Release-Critical Test Suite (92 tests)
```bash
cd build

# Phase 6 Consistency & Failover Tests (52 tests)
ctest -L "sharding" -L "phase6" -L "release_critical" --verbose

# Wave-8 Fault Injection Tests (40 tests)
ctest -L "sharding" -L "fault_injection" -L "release_critical" --verbose

# Thread-Safety & Lock-Order Tests (18 tests)
ctest -L "sharding" -L "thread-safety" -L "release_critical" --verbose

# All Release-Critical Sharding (unified run)
ctest -R "ShardingMultiShardExactPhaseCGate|test_sharding.*release_critical" \
  --output-on-failure --verbose
```

### Benchmark Execution (if environment permits)
```bash
# Wave A Release Gate Benchmarks (SRG-01..06)
ctest -R "GATE-SRG" --verbose

# Capture results to file
ctest -R "GATE-SRG" --verbose 2>&1 | tee benchmarks_wave_a.log
```

### Environment Verification
```bash
# Verify dependencies
cmake --version
g++ --version
clang++ --version
apt list --installed | grep -E "rocksdb|fmt|protobuf"
```

---

## Expected Test Results

### Phase 6 Hardening (TXC-01..32, FLR-01..20)
```
PASS: test_sharding_phase6_hardening.cpp::TXC-01..32 (2PC consistency)
PASS: test_sharding_phase6_hardening.cpp::FLR-01..20 (failover/recovery)
Expected: 52/52 PASS
```

### Fault Injection (FI-01..40)
```
PASS: test_sharding_p6_fault_injection.cpp::FI-01..40 (chaos/network/coordinator)
Expected: 40/40 PASS
```

### Thread-Safety & Consensus (TSO, LKO, CCR)
```
PASS: test_sharding_thread_safety_lock_order_focused.cpp::TSO-01..08
PASS: test_sharding_thread_safety_lock_order_focused.cpp::LKO-01..06
PASS: test_sharding_thread_safety_lock_order_focused.cpp::CCR-01..06
Expected: 18/18 PASS + 0 data races (ASAN/TSAN)
```

### Benchmark Validation
```
SRG-01 (consistent-hash routing):    p95: < baseline ns,  p99: < baseline ns
SRG-02 (2PC prepare):                 p95: < baseline ns,  p99: < baseline ns
SRG-03 (2PC commit):                  p95: < baseline ns,  p99: < baseline ns
SRG-04 (WAL append):                  p95: < baseline ns,  p99: < baseline ns
SRG-05 (health check):                p95: < baseline ns,  p99: < baseline ns
SRG-06 (route lookup):                p95: < baseline ns,  p99: < baseline ns
```

---

## CI Workflow Validation

### GitHub Actions Integration Points
**File**: `.github/workflows/ci-build.yml`
- [ ] Sharding lane runs all `release_critical` tests
- [ ] Sharding tests marked with `LABELS  sharding ... release_critical`
- [ ] Test timeout set appropriately (likely 10-30 min for full suite)
- [ ] Failure notifications go to maintainers

**File**: `.github/workflows/ci-benchmarks.yml`
- [ ] SRG-01..06 benchmarks registered
- [ ] Benchmark timeout set appropriately (likely 5-15 min per bench)
- [ ] Results captured and trended over time
- [ ] Threshold alerts configured

### Expected CI Behavior
```
✅ ci-build.yml runs:
  - test_sharding_phase6_hardening (TXC + FLR)
  - test_sharding_p6_fault_injection (FI-01..40)
  - test_sharding_thread_safety_lock_order_focused
  - test_sharding_multishard_exact
  Total: ~92 tests, ~5-10 min runtime

✅ ci-benchmarks.yml runs:
  - bench_sharding_release_gates (SRG-01..06)
  Total: ~6 benchmarks, ~2-5 min runtime
```

---

## Pre-Merge Validation Sequence

1. **Build Validation** (local or Docker)
   ```bash
   cmake --preset community-release-allow-missing-rocksdb -B build
   cd build && make sharding -j$(nproc)
   ```

2. **Unit Tests** (release_critical only)
   ```bash
   ctest -L "release_critical" -R "sharding" --verbose --output-on-failure
   ```

3. **Benchmark Run** (if applicable)
   ```bash
   ctest -R "GATE-SRG" --verbose 2>&1 | tee /tmp/baseline_wave_a.log
   ```

4. **Sanitizer Verification** (if time permits)
   ```bash
   # ASAN/UBSAN for thread-safety verification
   cmake --preset linux-asan -B build-asan
   cd build-asan && ctest -L "sharding" -L "release_critical"
   ```

5. **Verify Artifacts Created**
   - [ ] `src/sharding/PHASE_2_ACCEPTANCE_REPORT.md`
   - [ ] `src/sharding/PHASE_3_ACCEPTANCE_REPORT.md`
   - [ ] `docs/sharding/QUORUM_LOSS_RUNBOOK.md`
   - [ ] `src/sharding/MULTISHARD_GATE_VALIDATION_REPORT.md`
   - [ ] `benchmarks/sharding/WAVE_A_BASELINE_REPORT.md`
   - [ ] `src/sharding/WAVE_A_CI_READINESS_REPORT.md`
   - [ ] `src/sharding/WAVE_A_CLOSURE_EVIDENCE.md`

---

## Post-Merge Tasks

- [ ] Update root `ROADMAP.md` § Wave A section to mark COMPLETE
- [ ] Update `src/sharding/ROADMAP.md` § In Progress to remove Wave A items
- [ ] Request human sign-off at `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- [ ] Tag closure: `git tag v1.x.y-wave-a-sharding-complete develop`
- [ ] Create GitHub Release with closure summary
- [ ] Update CHANGELOG.md with Wave A completion

---

## Troubleshooting Guide

### Build Fails: "fmt library not found"
```bash
# Solution 1: Install via apt
sudo apt-get install -y libfmt-dev

# Solution 2: Docker
docker run -v $(pwd):/workspace ... cmake --preset linux-release -B build

# Solution 3: vcpkg bootstrap
vcpkg install fmt
```

### Build Fails: "RocksDB not found"
```bash
# Solution 1: Allow missing RocksDB (diagnostic preset)
cmake --preset community-release-allow-missing-rocksdb -B build

# Solution 2: Install RocksDB
sudo apt-get install -y librocksdb-dev

# Solution 3: vcpkg
vcpkg install rocksdb
```

### Test Fails: "Thread-safety violation detected"
```bash
# Re-run with TSAN for confirmation
TSAN_OPTIONS="halt_on_error=1" ctest -R "test_sharding_thread_safety" --verbose

# If confirmed, coordinate with Phase 2/3 agent output
# Check against canonical lock ordering in src/sharding/dual_consensus_orchestrator.cpp:829
```

### Benchmark Fails: "Timeout"
```bash
# Increase timeout
ctest -R "GATE-SRG" --timeout 600 --verbose

# Or run directly without ctest
./benchmarks/sharding/bench_sharding_release_gates
```

---

## Success Criteria

**Build**: ✅ All sharding module compiles without warnings/errors
**Tests**: ✅ All 92 release_critical tests pass
**Benchmarks**: ✅ All 6 SRG benchmarks complete within timeout
**Artifacts**: ✅ All 7 acceptance reports exist and are non-empty
**CI**: ✅ GitHub Actions workflows run successfully on develop
**Sign-off**: 🟡 Human approval pending at GA_PROMOTION_SIGN_OFF.md

---

**Prepared By**: Wave A Closure Agent Coordinator
**Date**: 2026-08-17 18:50 UTC
