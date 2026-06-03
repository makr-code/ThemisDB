# Phase 5 Test Execution Guide - W1-R Replication Module

## Prerequisites

### 1. Install RocksDB Dependency

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y librocksdb-dev
```

**macOS:**
```bash
brew install rocksdb
```

**From Source (if needed):**
```bash
cd /tmp
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
make shared_lib
sudo make install
```

### 2. Verify CMake Configuration

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

Expected output:
```
-- RocksDB found
-- Configuring done
-- Generating done
```

## Test Execution Commands

### Phase 5A: Core Replication Tests

**Build:**
```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_replication_ha -j4
```

**Run with Output:**
```bash
./test_replication_ha --gtest_print_time=1 \
    --gtest_filter="ReplicationConfigTest.*" \
    2>&1 | tee test_replication_ha_results.log
```

**Individual Test Cases:**
```bash
# Configuration tests (10 tests)
./test_replication_ha --gtest_filter="ReplicationConfigTest.*"

# Checksum tests (2 tests)
./test_replication_ha --gtest_filter="WALChecksumTest.*"

# Last-Write-Wins tests (4 tests)
./test_replication_ha --gtest_filter="LWWResolverTest.*"

# CRDT tests (3 tests) - validates Batch C optimization
./test_replication_ha --gtest_filter="CRDTResolverTest.*"

# Leadership election (8+ tests) - validates Batch B CRITICAL fix
./test_replication_ha --gtest_filter="LeaderElectionTest.*"
```

### Phase 5B: Raft V2 Protocol Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_replication_raft_v2 -j4
./test_replication_raft_v2 --gtest_print_time=1
```

### Phase 5C: New Features Tests (Phase 4 Validations)

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_replication_new_features -j4

# Run all new feature tests (validates all Phase 4 optimizations)
./test_replication_new_features --gtest_print_time=1

# Specific optimization tests:
./test_replication_new_features --gtest_filter="*WALSerialize*"      # Batch A
./test_replication_new_features --gtest_filter="*LeaderElection*"    # Batch B Critical
./test_replication_new_features --gtest_filter="*HealthCheck*"       # Batch B
./test_replication_new_features --gtest_filter="*CRDTMerge*"         # Batch C
```

### Phase 5D: CRDT Types Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_replication_crdt_types -j4
./test_replication_crdt_types --gtest_print_time=1
```

### Phase 5E: WAL Replication Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_wal_replication -j4

# All WAL tests
./test_wal_replication --gtest_print_time=1

# Specific test classes:
./test_wal_replication --gtest_filter="WALReplicationTest.*"        # 12+ tests
./test_wal_replication --gtest_filter="WALShipperCompressionTest.*" # 7+ tests
```

### Phase 5F: Integration Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_wal_replication_integration -j4
./test_wal_replication_integration --gtest_print_time=1

cmake --build . --target test_geo_replication_consistency -j4
./test_geo_replication_consistency --gtest_print_time=1
```

### Phase 5G: API Handler Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build
cmake --build . --target test_replication_topology_api_handler -j4
./test_replication_topology_api_handler --gtest_print_time=1
```

## Complete Test Suite Execution

### Run All Replication Tests

```bash
cd /tmp/workspace/makr-code/ThemisDB/build

# Build all test targets
for target in test_replication_ha test_replication_raft_v2 \
              test_replication_new_features test_replication_crdt_types \
              test_wal_replication test_wal_replication_integration \
              test_geo_replication_consistency test_replication_topology_api_handler; do
    echo "Building $target..."
    cmake --build . --target $target -j4
done

# Run all tests
ctest --preset replication-tests-release --output-on-failure --parallel 4
```

### Using CMake/CTest Presets

```bash
cd /tmp/workspace/makr-code/ThemisDB

# If presets are configured:
ctest --preset replication-tests-release --output-on-failure --parallel 2
```

## Performance Profiling

### Memory Allocation Profiling

```bash
# With valgrind (if available):
valgrind --tool=massif ./test_replication_ha

# With heaptrack (if available):
heaptrack ./test_replication_ha
heaptrack_gui heaptrack.test_replication_ha.*
```

### Performance Benchmarking

```bash
# If benchmark targets are built:
./benchmarks/wal_serialization_benchmark
./benchmarks/conflict_resolution_benchmark
./benchmarks/leadership_election_benchmark
```

## Interpreting Test Results

### Pass Criteria

- ✅ **All tests pass** with green checkmarks
- ✅ **No memory leaks** detected in valgrind output
- ✅ **Benchmark improvements** show 15-40% speedup in critical paths

### Expected Results by Test Suite

| Suite | Tests | Expected Result |
|-------|-------|-----------------|
| Core Replication | 30+ | 30/30 pass |
| Raft V2 | 5+ | 5+/5+ pass |
| New Features | 20+ | 20+/20+ pass |
| CRDT Types | 15+ | 15+/15+ pass |
| WAL Replication | 20+ | 20+/20+ pass |
| Integration | 10+ | 10+/10+ pass |
| Consistency | 8+ | 8+/8+ pass |
| API Handler | 5+ | 5+/5+ pass |

### Failure Analysis

If tests fail:

1. **Capture full output**:
   ```bash
   ./test_replication_ha --gtest_print_time=1 2>&1 | tee failure.log
   ```

2. **Check build errors**:
   ```bash
   grep -i "error\|warning" build.log
   ```

3. **Verify dependencies**:
   ```bash
   ldd ./test_replication_ha | grep rocksdb
   ```

## Continuous Integration

### GitHub Actions Execution

```bash
# To run in GitHub Actions, update .github/workflows with:
- name: Run Phase 5 Tests
  run: |
    cd build
    ctest --preset replication-tests-release --output-on-failure --parallel 4
```

## Reporting Results

### Generate Test Report

```bash
cd /tmp/workspace/makr-code/ThemisDB/build

# Generate XML report for CI systems
ctest -T Test --output-on-failure

# Generate JSON report
ctest --output-json results.json

# Generate HTML report (if CDash is configured)
ctest -T Coverage
```

### Archive Results

```bash
mkdir -p ~/phase5_results
cp test_*.log ~/phase5_results/
cp *.xml ~/phase5_results/ 2>/dev/null
tar -czf ~/phase5_results.tar.gz ~/phase5_results/
```

## Troubleshooting

### Build Fails with "RocksDB not found"

**Solution:**
```bash
# Check RocksDB installation
pkg-config --modversion rocksdb

# If not found, reinstall:
sudo apt-get remove -y rocksdb librocksdb-dev
sudo apt-get install -y librocksdb-dev

# Rebuild CMake
cd build
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Tests Fail with Timeout

**Solution:**
```bash
# Increase timeout (default 300s):
ctest --preset replication-tests-release --timeout 600
```

### Memory Issues During Test

**Solution:**
```bash
# Run with limited parallelism
ctest --preset replication-tests-release --parallel 1
```

### Specific Test Failure

**Solution:**
```bash
# Run single test with verbose output
./test_replication_ha --gtest_filter="TestName" --gtest_also_run_disabled_tests -v
```

## Next Steps After Phase 5

1. **Collect Performance Metrics**: Compare before/after for all optimizations
2. **Document Results**: Update REMEDIATION_SUMMARY.txt
3. **Archive Logs**: Save all test logs for audit trail
4. **Plan Phase 6**: Hardening and production validation

