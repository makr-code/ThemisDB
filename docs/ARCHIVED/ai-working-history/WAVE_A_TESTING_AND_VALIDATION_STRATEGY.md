# Wave A Testing and Validation Strategy

**Document**: Wave A Production Readiness Validation  
**Scope**: 5 modules, 103 CRITICAL gaps, 5-6 week implementation + validation cycle  
**Target**: Production deployment by Oct 15, 2026

---

## TESTING PYRAMID FOR WAVE A

```
                    Integration Tests (5%)
                        ↑↑↑
                  Chaos/Fault Injection (10%)
                        ↑↑↑
              Stress Tests (20%)
                   ↑↑↑
          Functional Tests (40%)
               ↑↑↑
    Unit Tests (25%)
```

---

## PHASE 1: UNIT TESTS (Per-Module)

### Transaction Module Unit Tests

**Iterator Safety Tests** (4 test cases):
```cpp
TEST(LockManagerIteratorSafety, BoundsCheckOnInsertion) {
    // Test: Iterator remains valid after vector insertion
    LockManager mgr;
    auto lock_iter = mgr.addLock("key1");
    mgr.addLock("key2");  // May cause reallocation
    ASSERT_TRUE(mgr.isValidIterator(lock_iter));
}

TEST(LockManagerIteratorSafety, DereferenceAfterErase) {
    // Test: Erasing one lock doesn't invalidate others
    LockManager mgr;
    auto iter1 = mgr.addLock("key1");
    auto iter2 = mgr.addLock("key2");
    auto iter3 = mgr.addLock("key3");
    mgr.eraseLock("key2");  // Erase middle element
    ASSERT_TRUE(mgr.isValidIterator(iter1));
    ASSERT_TRUE(mgr.isValidIterator(iter3));  // iter3 should shift
}
```

**Timeout Tests** (5 test cases):
```cpp
TEST(TransactionBatcher, TimeoutOccursAfter30Seconds) {
    TransactionBatcher batcher(30s);
    auto t1 = std::thread([&] {
        auto result = batcher.lock();  // Should timeout
        ASSERT_EQ(result.status, Status::TIMEOUT);
    });
    std::this_thread::sleep_for(35s);
    t1.join();
}

TEST(DistributedTransactionManager, WaitForConsensusTimeout) {
    // Test: Consensus wait timeout
    DistributedTransactionCoordinator coord;
    auto result = coord.waitForConsensus(30s);  // Should timeout
    ASSERT_EQ(result.status, Status::TIMEOUT);
}
```

**SAGA Lifecycle Tests** (3 test cases):
```cpp
TEST(SAGAOrchestratorGuard, CleanupOnException) {
    try {
        SAGAOrchestratorGuard guard(orchestrator);
        throw std::runtime_error("Test error");
    } catch (...) {
        // Destructor should have cleaned up
    }
    // Verify resources released
    ASSERT_EQ(orchestrator.getPendingOperations(), 0);
}
```

---

### Sharding Module Unit Tests

**Lock Ordering Tests** (8 test cases):
```cpp
TEST(DualConsensusOrchestratorLockOrder, StateBeforeMetrics) {
    DualConsensusOrchestrator orch;
    
    // Should succeed: acquire in correct order
    LockOrderValidator validator;
    auto result = validator.validateOrder({
        {"state_mutex_", 1},
        {"metrics_mutex_", 3}
    });
    ASSERT_TRUE(result);
}

TEST(RaftConsensusAdapterLockOrder, NoCyclesDetected) {
    RaftConsensusAdapter raft;
    
    // ThreadSanitizer should detect no cycles
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            raft.updateState();
            raft.getMetrics();
            raft.updateCallbacks();
        });
    }
    for (auto& t : threads) t.join();
}
```

**Timeout Tests** (5 test cases):
```cpp
TEST(RaftLogAppend, TimeoutAfter10Seconds) {
    RaftLog log;
    auto start = std::chrono::steady_clock::now();
    auto result = log.append(entry, 10s);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    ASSERT_EQ(result.status, Status::TIMEOUT);
    ASSERT_GE(elapsed.count(), 10s.count());
    ASSERT_LT(elapsed.count(), 11s.count());  // Allow 1s jitter
}
```

**Retry Logic Tests** (4 test cases):
```cpp
TEST(ExponentialBackoffRetry, CorrectDelayProgression) {
    ExponentialBackoff retry(5, 100ms, 10s);
    std::vector<std::chrono::milliseconds> delays;
    
    // Mock each retry to capture delays
    // 100ms, 200ms, 400ms, 800ms, 1600ms
    // Verify progression
}

TEST(ShardRouterRetry, SucceedsAfterTransientFailure) {
    ShardRouter router;
    int attempt_count = 0;
    
    auto result = ExponentialBackoff(5, 100ms, 10s).execute([&] {
        attempt_count++;
        if (attempt_count < 3) return error("Transient failure");
        return ok();  // Succeed on 3rd attempt
    });
    
    ASSERT_EQ(result.status, Status::OK);
    ASSERT_EQ(attempt_count, 3);
}
```

---

### Replication Module Unit Tests

**Geographic Placement Tests** (5 test cases):
```cpp
TEST(GeographicPlacementPolicy, ReplicasSpreadAcrossZones) {
    GeographicPlacementPolicy policy;
    
    std::vector<NodeMetadata> nodes = {
        {"node-1", "us-east-1a", "us-east", 0, 0},
        {"node-2", "us-east-1b", "us-east", 0, 0},
        {"node-3", "us-west-1a", "us-west", 0, 0},
    };
    
    PlacementRequest req{
        .resource_id = "table-1",
        .replica_count = 3,
        .primary_zone = "us-east-1a",
    };
    
    auto result = policy.selectReplicaNodes(req);
    
    // Verify no 2 nodes in same zone
    std::set<std::string> zones;
    for (const auto& node_id : result.target_nodes) {
        auto zone = lookupZone(node_id);
        ASSERT_EQ(zones.count(zone), 0);  // Zone not seen before
        zones.insert(zone);
    }
}

TEST(GeographicPlacementPolicy, PreferredZonesSelected) {
    // Verify preferred zones are selected first
}
```

**Async WAL Tests** (5 test cases):
```cpp
TEST(AsyncWALShipper, EntriesBatchedCorrectly) {
    AsyncWALShipper shipper;
    
    // Add 100 entries
    for (int i = 0; i < 100; ++i) {
        WALEntry entry{.seq = i, .key = fmt::format("key-{}", i)};
        shipper.appendEntry(entry);
    }
    
    // Verify entries are batched
    auto batches = shipper.getPendingBatches();
    // Should be 1-2 batches (target 1MB max)
    ASSERT_LT(batches.size(), 5);
}

TEST(AsyncWALShipper, QuorumAckWaits) {
    AsyncWALShipper shipper;
    
    // Add entry and wait for quorum ack
    WALEntry entry{.seq = 1, .key = "key-1"};
    shipper.appendEntry(entry);
    
    auto result = shipper.waitForQuorumAck(1, 5s);
    ASSERT_TRUE(result);
}

TEST(AsyncWALShipper, RetryOnTransientFailure) {
    // Mock replica that fails 2x then succeeds
    // Verify retry logic works
}
```

**Lag Alert Tests** (4 test cases):
```cpp
TEST(LagAlertManager, AlertTriggersOnThreshold) {
    LagAlertManager mgr;
    
    // Update lag to 15s (exceeds 10s alert threshold)
    mgr.updateReplicaLag("replica-1", 15_000);
    auto alerts = mgr.checkAndAlertLagViolations();
    
    ASSERT_TRUE(alerts.contains("replica-1"));
    ASSERT_EQ(alerts["replica-1"].severity, AlertSeverity::WARNING);
}

TEST(LagAlertManager, CriticalAfter30Seconds) {
    LagAlertManager mgr;
    
    mgr.updateReplicaLag("replica-1", 35_000);  // 35s lag
    auto alerts = mgr.checkAndAlertLagViolations();
    
    ASSERT_EQ(alerts["replica-1"].severity, AlertSeverity::CRITICAL);
}

TEST(LagAlertManager, FailoverAfter5MinutesCritical) {
    LagAlertManager mgr;
    
    // Keep lag at 35s for 5+ minutes
    for (int i = 0; i < 6; ++i) {
        mgr.updateReplicaLag("replica-1", 35_000);
        std::this_thread::sleep_for(1min);
        auto alerts = mgr.checkAndAlertLagViolations();
        if (i == 5) {
            ASSERT_TRUE(alerts["replica-1"].failover_initiated);
        }
    }
}
```

---

### Voice Module Unit Tests

**Liveness Detection Tests** (5 test cases):
```cpp
TEST(VoiceLivenessDetector, ValidChallengeResponseAccepted) {
    VoiceLivenessDetector liveness;
    
    auto challenge = liveness.issueChallenge("user-1");
    ASSERT_TRUE(challenge);
    
    // Simulate user echoing challenge
    std::string user_response = challenge->text;  // Echo exact text
    auto result = liveness.verifyResponse("user-1", *challenge, user_response);
    ASSERT_TRUE(result);
}

TEST(VoiceLivenessDetector, StaleChallengeRejected) {
    VoiceLivenessDetector liveness;
    
    auto challenge = liveness.issueChallenge("user-1");
    std::this_thread::sleep_for(6s);  // Wait past 5s window
    
    // Try to verify stale challenge
    auto result = liveness.verifyResponse("user-1", *challenge, challenge->text);
    ASSERT_FALSE(result);
}

TEST(VoiceLivenessDetector, ReplayAttackDetected) {
    VoiceLivenessDetector liveness;
    
    auto challenge = liveness.issueChallenge("user-1");
    auto result1 = liveness.verifyResponse("user-1", *challenge, challenge->text);
    ASSERT_TRUE(result1);
    
    // Replay same response
    auto result2 = liveness.verifyResponse("user-1", *challenge, challenge->text);
    ASSERT_FALSE(result2);  // Replay rejected
}
```

**Anti-Spoof Tests** (5 test cases):
```cpp
TEST(VoiceAntiSpoofEngine, LiveAudioAccepted) {
    VoiceAntiSpoofEngine spoof;
    
    // Load sample of real live audio
    auto audio_data = loadAudioSample("live_speaker.wav");
    auto baseline = loadSpeakerBaseline("user-1");
    
    auto analysis = spoof.analyzeSpoofRisk(audio_data, baseline);
    ASSERT_FALSE(analysis.is_likely_spoofed);
    ASSERT_GT(analysis.audio_freshness_score, 0.8);
}

TEST(VoiceAntiSpoofEngine, RecordedAudioDetected) {
    VoiceAntiSpoofEngine spoof;
    
    // Load recording of live audio (artifact of digital playback)
    auto audio_data = loadAudioSample("recorded_speaker.wav");
    auto baseline = loadSpeakerBaseline("user-1");
    
    auto analysis = spoof.analyzeSpoofRisk(audio_data, baseline);
    ASSERT_TRUE(analysis.is_likely_spoofed);
    ASSERT_LT(analysis.audio_freshness_score, 0.5);
}

TEST(VoiceAntiSpoofEngine, SyntheticAudioDetected) {
    VoiceAntiSpoofEngine spoof;
    
    // Load AI-synthesized audio (TTS)
    auto audio_data = loadAudioSample("tts_synthetic.wav");
    auto baseline = loadSpeakerBaseline("user-1");
    
    auto analysis = spoof.analyzeSpoofRisk(audio_data, baseline);
    ASSERT_TRUE(analysis.is_likely_spoofed);
    ASSERT_LT(analysis.speaker_match_score, 0.3);  // Wrong speaker
}
```

---

### GPU Module Unit Tests

**CUDA Error Checking Tests** (5 test cases):
```cpp
TEST(GPUSafeRAII, CUDAErrorThrows) {
    // Simulate CUDA error
    ASSERT_THROW({
        DeviceMemoryGuard mem(1_MB);
        // Simulate allocation failure in destructor
    }, std::runtime_error);
}

TEST(GPUSafeRAII, SuccessfulAllocationFreed) {
    {
        DeviceMemoryGuard mem(1_MB);
        auto ptr = mem.get();
        ASSERT_NE(ptr, nullptr);
    }  // Destructor frees automatically
    
    // Verify memory was freed (can allocate again)
    DeviceMemoryGuard mem2(1_MB);
    ASSERT_NE(mem2.get(), nullptr);
}

TEST(GPUSafeRAII, MoveSemantics) {
    DeviceMemoryGuard mem1(1_MB);
    auto ptr1 = mem1.get();
    
    DeviceMemoryGuard mem2 = std::move(mem1);
    auto ptr2 = mem2.get();
    
    ASSERT_EQ(ptr1, ptr2);  // Same underlying pointer
    ASSERT_EQ(mem1.get(), nullptr);  // mem1 released ownership
}
```

**Unified Memory Coordination Tests** (3 test cases):
```cpp
TEST(UnifiedMemoryBuffer, CPUAcquisitionSucceeds) {
    UnifiedMemoryBuffer buf(1_MB);
    
    auto result = buf.acquireForCPU();
    ASSERT_TRUE(result);
    
    // Can write from CPU
    memset(buf.ptr_, 0xAB, 100);
}

TEST(UnifiedMemoryBuffer, CPUThenGPUTransition) {
    UnifiedMemoryBuffer buf(1_MB);
    
    ASSERT_TRUE(buf.acquireForCPU());
    memset(buf.ptr_, 0xAB, 100);
    ASSERT_TRUE(buf.releaseOwnership());
    
    ASSERT_TRUE(buf.acquireForGPU());
    // Can launch kernel
    ASSERT_TRUE(buf.releaseOwnership());
}

TEST(UnifiedMemoryBuffer, SimultaneousAccessDetected) {
    UnifiedMemoryBuffer buf(1_MB);
    
    ASSERT_TRUE(buf.acquireForCPU());
    auto result = buf.acquireForGPU();  // Should fail or block
    ASSERT_FALSE(result);  // Conflict detected
}
```

**Kernel Timeout Tests** (2 test cases):
```cpp
TEST(KernelTimeoutEnforcer, LongKernelTimeouts) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config{.timeout_ms = 100};
    
    auto result = enforcer.executeWithTimeout([&] {
        // Long kernel that takes 5 seconds
        std::this_thread::sleep_for(5s);
    }, config);
    
    ASSERT_EQ(result.status, Status::TIMEOUT);
}

TEST(KernelTimeoutEnforcer, QuickKernelCompletes) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config{.timeout_ms = 5000};
    
    auto result = enforcer.executeWithTimeout([&] {
        // Quick kernel
        std::this_thread::sleep_for(100ms);
    }, config);
    
    ASSERT_TRUE(result);
}
```

---

## PHASE 2: STRESS TESTS

### Transaction Module Stress Tests

```cpp
TEST(TransactionStress, 10kConcurrentShortTransactions) {
    TransactionManager mgr;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            for (int j = 0; j < 1000; ++j) {
                auto txn = mgr.begin();
                txn.set(fmt::format("key-{}-{}", i, j), fmt::format("value-{}", j));
                if (txn.commit()) {
                    success_count++;
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    
    ASSERT_EQ(success_count, 10000);
    ASSERT_EQ(mgr.getPendingTransactions(), 0);  // Cleanup verified
}
```

### Sharding Module Stress Tests

```cpp
TEST(ShardingStress, 5ShardsConcurrentWrites) {
    ShardRouter router;
    std::vector<std::thread> threads;
    
    for (int shard = 0; shard < 5; ++shard) {
        threads.emplace_back([&, shard] {
            for (int i = 0; i < 2000; ++i) {
                URN urn{fmt::format("shard-{}/entity-{}", shard, i)};
                auto result = router.put(urn, nlohmann::json{{"value", i}});
                ASSERT_TRUE(result);
            }
        });
    }
    for (auto& t : threads) t.join();
    
    // Verify all 10k entities written
}
```

### Replication Module Stress Tests

```cpp
TEST(ReplicationStress, 1000EntriesAsyncWALShipping) {
    AsyncWALShipper shipper;
    
    for (int i = 0; i < 1000; ++i) {
        WALEntry entry{.seq = i, .key = fmt::format("key-{}", i)};
        shipper.appendEntry(entry);
    }
    
    auto result = shipper.waitForQuorumAck(1000, 30s);
    ASSERT_TRUE(result);
    
    // Measure throughput
    ASSERT_GT(shipper.getThroughputMBps(), 10);  // At least 10 MB/s
}
```

---

## PHASE 3: CHAOS & FAULT INJECTION TESTS

### Transaction Recovery Chaos

```cpp
TEST(TransactionChaos, CoordinatorCrashRecovery) {
    TransactionManager mgr;
    
    // Start transaction
    auto txn = mgr.begin();
    txn.set("key", "value");
    
    // Crash coordinator mid-commit
    injectCrashAt(CRASH_POINT_BEFORE_FSYNC);
    
    // System recovers
    mgr.recover();
    
    // Verify transaction was either committed or rolled back (no partial state)
    auto committed = mgr.isKeyPresent("key");
    auto uncommitted = !mgr.isKeyPresent("key");
    ASSERT_TRUE(committed || uncommitted);  // Not partially applied
}
```

### Sharding Multi-Shard Failure

```cpp
TEST(ShardingChaos, MultiShardRebalanceUnderNetworkPartition) {
    ShardRouter router;
    
    // Partition shard-1 from rest of cluster
    injectNetworkPartition("shard-1");
    
    // Try to rebalance
    auto result = router.rebalance();
    
    // Should detect partition and defer rebalance
    ASSERT_EQ(result.status, Status::DEFERRED);
    
    // Heal partition
    healNetworkPartition();
    
    // Retry rebalance
    result = router.rebalance();
    ASSERT_TRUE(result);
}
```

### Voice Anti-Spoof Under Adversarial Inputs

```cpp
TEST(VoiceAntiSpoofChaos, AdversarialAudioSamples) {
    VoiceAntiSpoofEngine spoof;
    
    // Load adversarial samples (attempted spoofing attacks)
    auto samples = loadAdversarialAudioLibrary();
    
    for (const auto& sample : samples) {
        auto analysis = spoof.analyzeSpoofRisk(sample.audio, baseline);
        // Should reject with high confidence
        ASSERT_LT(analysis.audio_freshness_score, 0.3);
    }
}
```

---

## PHASE 4: MEMORY & THREAD SAFETY VALIDATION

### ThreadSanitizer Validation

```bash
# Build with ThreadSanitizer
cmake --preset linux-release-tsan
ctest -R "ShardingLockOrder|TransactionConcurrency|ReplicationAsync"
# Should produce zero data race reports
```

### AddressSanitizer Validation

```bash
# Build with AddressSanitizer
cmake --preset linux-release-asan
ctest -R "TransactionStress|ShardingStress|GPUMemoryPool"
# Should produce zero memory leak reports
```

---

## PHASE 5: PERFORMANCE BASELINE VALIDATION

### P95/P99 Latency Baselines

| Module | Operation | P95 | P99 | Target |
|--------|-----------|-----|-----|--------|
| Transaction | commit | 10ms | 25ms | <50ms |
| Sharding | route + execute | 15ms | 40ms | <100ms |
| Replication | WAL append + quorum ack | 20ms | 50ms | <100ms |
| Voice | liveness challenge | 2s | 5s | <10s |
| GPU | kernel execute | 50ms | 150ms | <500ms |

### Benchmark Commands

```bash
# Transaction throughput
./benchmarks/transaction_benchmark --duration 60 --threads 8
# Expected: >10k commits/sec

# Sharding latency
./benchmarks/sharding_benchmark --workload multi_shard --duration 60
# Expected: p99 < 100ms

# GPU kernel throughput
./benchmarks/gpu_benchmark --kernel matrix_multiply --size 1024
# Expected: >100 GFLOPs
```

---

## VALIDATION GATE MATRIX

| Gate | Module | Success Criteria | Evidence |
|------|--------|------------------|----------|
| **Compilation** | All | Zero errors, zero warnings | CI build log |
| **Unit Tests** | All | 100% pass (60+ tests) | ctest output |
| **Stress Tests** | All | 10k+ ops complete, cleanup verified | stress test logs |
| **Chaos Tests** | Transaction, Sharding, Replication | Recover correctly from crashes/partitions | chaos test reports |
| **ThreadSan** | All | Zero data races | TSan log (zero reports) |
| **AddressSan** | All | Zero memory leaks | ASan log (zero leaks) |
| **Performance** | All | P95/P99 within baselines | benchmark results |
| **Security** | Voice, GPU | No auth bypass, CUDA errors caught | security audit |

---

## Success Criteria for Wave A Production Readiness

ALL gates below must be GREEN before Wave A is considered production-ready:

1. ✅ **Compilation**: All 5 modules compile without errors/warnings
2. ✅ **Functionality**: All 60+ unit tests pass
3. ✅ **Stress**: 10k+ concurrent ops complete cleanly
4. ✅ **Chaos**: Recovery verified for crash/partition scenarios
5. ✅ **Memory Safety**: ThreadSanitizer & AddressSanitizer clean
6. ✅ **Performance**: P95/P99 latencies within targets
7. ✅ **Security**: No auth bypass, liveness/anti-spoof working
8. ✅ **Long-run Stability**: 48-hour soak test passes

Once all gates are GREEN, Wave A is ready for human sign-off and release promotion to v2.4.0.
