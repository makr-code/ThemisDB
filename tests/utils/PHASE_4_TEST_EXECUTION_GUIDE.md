# Phase 4 Test Execution Guide - Utils Module

## Quick Start

### Build Utils Tests
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-debug -DTHEMIS_ENABLE_GRPC=OFF -DTHEMIS_ENABLE_LLM=OFF -DTHEMIS_ENABLE_GPU=OFF
cmake --build build-community-debug --target module_utils_test -j4
```

### Run All Utils Tests
```bash
cd build-community-debug
ctest -R "utils" -V --output-on-failure
```

### Run Specific Test Categories
```bash
# Resource Management Tests
ctest -R "ResourceManagementTest" -V

# Privacy & Audit Hardening
ctest -R "PrivacyAuditHardening" -V

# Rate Limiting Tests
ctest -R "RateLimiter" -V

# UUID v7 Tests
ctest -R "UUIDv7" -V

# Compression Tests
ctest -R "LZ4|ZSTD" -V
```

## Test Scenarios by Component

### 1. Resource Management (14 tests)

**Test Suite**: `test_phase1_resource_management.cpp`

**Scenario 1: Singleton Pattern**
- Creates singleton instance
- Verifies no resource leak on cleanup
- Validates exception safety
- Tests multi-threaded access

**Expected Behavior**:
- Singleton instance created once
- Cleanup occurs during destruction
- No double-delete or dangling pointers
- Thread-safe access patterns

**Scenario 2: ThreadGuard**
- Manages thread lifecycle
- Ensures proper cleanup
- Validates timeout handling
- Rejects non-joinable threads

**Expected Behavior**:
- Thread properly joined
- No resource leaks
- Timeout enforced
- Exception-safe cleanup

**Scenario 3: Smart Pointers**
- unique_ptr ownership transfer
- shared_ptr reference counting
- Exception-safe cleanup
- Circular reference detection

**Expected Behavior**:
- Correct ownership semantics
- Reference count accuracy
- No circular references
- Proper destructor ordering

### 2. Privacy & Audit Hardening (8 tests)

**Test Suite**: `test_utils_privacy_audit_hardening.cpp`

**Scenario 1: Unicode PII Detection (PH-01)**
```cpp
Test Cases:
- Email with emoji: "user😀@example.com"
- SSN with RTL text: "123-45-6789 ‏עברית"
- Credit card with combining marks: "4111-1111-1111-1111̸"
- Phone with variation selectors: "+1-555-0123︎"

Expected: All patterns detected correctly, no false negatives
```

**Scenario 2: Multibyte Handling (PH-02)**
```cpp
Test Cases:
- Valid UTF-8: "Hello 世界"
- Incomplete sequence: "\xE2\x98"
- Invalid sequence: "\xFF\xFE"
- Boundary conditions: "\xC0\x80" (overlong encoding)

Expected: Valid sequences processed, invalid rejected
```

**Scenario 3: Buffer Overflow (PH-03)**
```cpp
Test Cases:
- Normal load: 100 audit entries
- Saturation: 10,000 entries
- Overflow: 50,000 entries with limited buffer
- Recovery: Post-overflow normal operation

Expected: Graceful overflow shedding, no crash, recovery valid
```

**Scenario 4: Rate Limiting (PH-04)**
```cpp
Test Cases:
- Single token: 1 token/sec, verify blocking
- Burst requests: 100 reqs/sec with 1 token/sec limit
- Concurrent: 4 threads at max capacity
- Recovery: Rate adjustment during operation

Expected: Accurate token enforcement, no starvation
```

**Scenario 5: Concurrent Writes (PH-06)**
```cpp
Test Cases:
- 4 threads writing 1000 entries each
- Verify ordering preservation
- Check for race conditions
- Validate no log corruption

Expected: All entries logged, chronological order preserved
```

**Scenario 6: Regex Backtracking (PH-08)**
```cpp
Test Cases:
- ReDoS attack: "(a+)+b" with "aaaaaaaaaaaaaaac"
- Timeout: Detection timeout <1ms
- Fallback: Pattern degradation
- Performance: O(n) complexity

Expected: Timeout enforced, no catastrophic backtracking
```

### 3. Rate Limiting (7 tests)

**Test Suite**: `test_utils_rate_limiter.cpp`

**Scenario 1: Token Bucket Initialization**
```cpp
RateLimiter limiter(100.0);  // 100 tokens/sec
ASSERT_EQ(limiter.available(), 100);
ASSERT_TRUE(limiter.max_rate() >= 99.0);
```

**Scenario 2: Blocking Acquire**
```cpp
RateLimiter limiter(10.0);  // 10 tokens/sec
auto start = now();
limiter.acquire(10);  // Takes 1 second
auto elapsed = now() - start;
ASSERT_GE(elapsed, 1.0s);
ASSERT_LT(elapsed, 1.1s);
```

**Scenario 3: Non-Blocking Try**
```cpp
RateLimiter limiter(1.0);
ASSERT_TRUE(limiter.try_acquire(1));  // First request succeeds
ASSERT_FALSE(limiter.try_acquire(1));  // Second fails immediately
wait(1.1s);
ASSERT_TRUE(limiter.try_acquire(1));  // Third succeeds after cooldown
```

**Scenario 4: Concurrent Access**
```cpp
RateLimiter limiter(100.0);
std::vector<std::thread> threads(4);
std::atomic<int> acquired = 0;

for (auto& t : threads) {
    t = std::thread([&]() {
        for (int i = 0; i < 25; i++) {
            limiter.acquire(1);
            acquired++;
        }
    });
}

// All 100 tokens consumed, no overages
ASSERT_EQ(acquired, 100);
```

### 4. Compression Codecs (15 tests)

**Test Suite**: `test_utils_future_interfaces.cpp`

**LZ4 Tests**:
```cpp
// Round-trip
data = "Hello, World! " * 100;
compressed = LZ4::compress(data);
ASSERT_LT(compressed.size(), data.size());  // Compression works
decompressed = LZ4::decompress(compressed);
ASSERT_EQ(decompressed, data);

// Empty input
empty = "";
compressed = LZ4::compress(empty);
decompressed = LZ4::decompress(compressed);
ASSERT_EQ(decompressed, empty);

// Safe API
oversized_output = LZ4::compress_safe(large_data, buffer_size=100);
ASSERT_LE(oversized_output.size(), 100);  // Safe bound respected
```

**ZSTD Tests**:
```cpp
// Streaming
compressor = ZstdStreamCompressor();
compressed_chunks = [];
for (chunk in large_data_chunks) {
    compressed_chunks.push(compressor.compress(chunk));
}
compressed_chunks.push(compressor.finish());

decompressor = ZstdStreamDecompressor();
decompressed_chunks = [];
for (chunk in compressed_chunks) {
    decompressed_chunks.push(decompressor.decompress(chunk));
}

ASSERT_EQ(join(decompressed_chunks), original_data);
```

### 5. UUID v7 Tests (7 tests)

**Test Suite**: `test_utils_future_interfaces.cpp`

**Scenario 1: Format Validation**
```cpp
uuid = UUID::v7();
format_string = uuid.to_string();
// Must be: xxxxxxxx-xxxx-7xxx-yxxx-xxxxxxxxxxxx
ASSERT_REGEX(format_string, "^[0-9a-f]{8}-[0-9a-f]{4}-7[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
```

**Scenario 2: Monotonicity**
```cpp
prev = UUID::v7();
for (int i = 0; i < 1000; i++) {
    curr = UUID::v7();
    ASSERT_GT(curr, prev);  // Strictly monotonic
    prev = curr;
}
```

**Scenario 3: Timestamp Embedding**
```cpp
before = now_ms();
uuid = UUID::v7();
after = now_ms();

// UUID timestamp should be between before and after
timestamp = uuid.timestamp_ms();
ASSERT_GE(timestamp, before);
ASSERT_LE(timestamp, after);
```

### 6. Contract Hardening Tests (16 tests)

**Test Suite**: `test_utils_contract_hardening_focused.cpp`

**Scenario 1: Error Code Validation**
```cpp
// All error codes must be unique
error_codes = get_all_error_codes();
ASSERT_EQ(error_codes.size(), set(error_codes).size());

// All must be in valid range
for (code in error_codes) {
    ASSERT_GE(code, ERR_MIN);
    ASSERT_LE(code, ERR_MAX);
}
```

**Scenario 2: Configuration Defaults**
```cpp
config = BloomFilterConfig();
ASSERT_EQ(config.false_positive_rate, 0.01);  // Default FPR
ASSERT_EQ(config.expected_elements, 100000);  // Default size
ASSERT_GT(config.bits, 0);  // Calculated correctly
```

**Scenario 3: Type Constructibility**
```cpp
// All config types constructible
RetryPolicy retry;
BloomFilterConfig bloom;
SamplerConfig sampler;

// All copyable
RetryPolicy retry2 = retry;
ASSERT_EQ(retry, retry2);
```

### 7. Core Interfaces Tests (36 tests)

**Test Suite**: `test_utils_interfaces.cpp`

**Scenario 1: PII Detection**
```cpp
detector = StreamingPIIDetector::create();

// Email patterns
ASSERT_DETECTED(detector, "user@example.com");
ASSERT_NOT_DETECTED(detector, "not an email");

// SSN patterns
ASSERT_DETECTED(detector, "123-45-6789");
ASSERT_NOT_DETECTED(detector, "123-45-678");

// Credit card patterns
ASSERT_DETECTED(detector, "4111-1111-1111-1111");
ASSERT_DETECTED(detector, "5555555555554444");  // No separators
```

**Scenario 2: Audit Log Integrity**
```cpp
audit_log = HashChainAuditLog::create();

// Add entries
audit_log.append("event1", metadata1);
audit_log.append("event2", metadata2);
audit_log.append("event3", metadata3);

// Verify chain
entries = audit_log.get_all();
ASSERT_EQ(entries.size(), 3);

for (i in 1..3) {
    current = entries[i];
    previous = entries[i-1];
    ASSERT_EQ(current.previous_hash, hash(previous));  // Chain intact
}

// Verify tampering detected
entries[1].data = corrupted_data;
ASSERT_FALSE(audit_log.verify_integrity());
```

**Scenario 3: HKDF Key Cache**
```cpp
cache = HKDFCache::create();

master_key = random_bytes(32);
salt = random_bytes(32);
info = "auth.session.v1";

// First derivation (cache miss)
key1 = cache.derive_cached(master_key, salt, info, 32);
stats1 = cache.stats();
ASSERT_EQ(stats1.misses, 1);
ASSERT_EQ(stats1.hits, 0);

// Second derivation (cache hit)
key2 = cache.derive_cached(master_key, salt, info, 32);
ASSERT_EQ(key1, key2);  // Same result
stats2 = cache.stats();
ASSERT_EQ(stats2.hits, 1);

// Eviction testing
cache.evict(master_key);
key3 = cache.derive_cached(master_key, salt, info, 32);
ASSERT_EQ(key1, key3);  // Still correct after eviction
```

### 8. Standalone Utilities Tests (19 tests)

**Test Suite**: `test_utils_standalone.cpp`

**Scenario 1: Checksum Computation**
```cpp
data = "The quick brown fox jumps over the lazy dog";

sha256 = SHA256::compute(data);
ASSERT_EQ(sha256, "d7a8fbb307d7d6ecbc00cab2f000280ff3d5d778453fc18ef45e1f50ff59166f");

md5 = MD5::compute(data);
ASSERT_EQ(md5, "9e107d9d372bb6826bd81d3542a419d6");

// Empty input
ASSERT_EQ(SHA256::compute(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
```

**Scenario 2: File Operations**
```cpp
// Create temp file
path = create_temp_file("test data");

// Read file
content = FileUtils::read(path);
ASSERT_EQ(content, "test data");

// Normalize path
normalized = PathNormalizer::normalize("./path/../to/file.txt");
ASSERT_EQ(normalized, "path/to/file.txt");
```

**Scenario 3: Feature Flags**
```cpp
flags = FeatureFlags::parse("FEATURE_A=true,FEATURE_B=false");
ASSERT_TRUE(flags.is_enabled("FEATURE_A"));
ASSERT_FALSE(flags.is_enabled("FEATURE_B"));
ASSERT_FALSE(flags.is_enabled("FEATURE_C"));  // Default: false
```

## Continuous Integration Commands

### Pre-commit Check
```bash
# Quick sanity check
cmake --build build-community-debug --target module_utils_test -j4
ctest --preset community-debug -R "utils" --timeout 30
```

### Full Test Suite
```bash
# Complete validation
cmake --build build-community-debug -j4
ctest --preset community-debug -R "utils" -V --output-on-failure

# Generate coverage report
cmake --build build-community-debug --target coverage
```

### Benchmark Gates
```bash
# Run performance benchmarks
./build-community-debug/benchmarks/utils/bench_utils_release_gates --benchmark_min_time=1.0

# Verify against baselines
python3 verify_benchmark_gates.py --baseline=performance_expectations.md
```

## Expected Results

| Test Category | Count | Expected Pass | Timeout |
|---------------|-------|---------------|---------|
| Resource Management | 14 | 14 | 30s |
| Contract Hardening | 16 | 16 | 15s |
| Future Interfaces | 22 | 22 | 45s |
| Core Interfaces | 36 | 36 | 60s |
| Privacy & Audit | 8 | 8 | 30s |
| Rate Limiting | 7 | 7 | 20s |
| Standalone Utils | 19 | 19 | 25s |
| **TOTAL** | **122** | **122** | **225s** |

**Overall Expected Result**: ✅ ALL PASS (100%)
**Estimated Execution Time**: 2-5 minutes
**Required Pass Rate**: >95%

## Troubleshooting

### Test Failures

**Problem**: Timeout in concurrency tests
**Solution**: 
- Increase timeout: `ctest --timeout 120`
- Run single test: `ctest -R "ConcurrentAuditWrites" -VV`
- Check system load: Should be <80% CPU

**Problem**: Memory leak detection
**Solution**:
- Run with ASan: `cmake --preset community-asan`
- Check for: `LeakSanitizer` findings
- Profile: `valgrind --leak-check=full ./test_binary`

**Problem**: Non-deterministic failures
**Solution**:
- Repeat test: `ctest --repeat-until-fail 10`
- Run with different thread count: `GTEST_FILTER="..." ./test_binary --gtest_filter="*ThreadTest*"`
- Check for: Race conditions, timing assumptions

## Post-Execution Validation

After test execution:
1. ✅ All tests pass
2. ✅ No memory leaks (if running with ASan)
3. ✅ No race conditions (if running with ThreadSanitizer)
4. ✅ Performance within acceptable bounds
5. ✅ Coverage metrics recorded
6. ✅ Report generated for CI/CD

---

**Last Updated**: 2026-08-08  
**Next Review**: Phase 5 (Execution Integration)
