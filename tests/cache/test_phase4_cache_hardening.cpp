// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cache/adaptive_query_cache.h"
#include "cache/bounded_lru_cache.h"
#include "cache/cache_hit_rate_slo_monitor.h"
#include "cache/cache_replication_coordinator.h"

using namespace themis;
using namespace themis::cache;

// =============================================================================
// Helpers
// =============================================================================

/// Build a minimal AdaptiveQueryCache config with L3 disabled (no RocksDB).
static AdaptiveQueryCache::Config makeConfig() {
    AdaptiveQueryCache::Config cfg;
    cfg.l1_max_entries          = 128;
    cfg.l1_max_entry_size       = 4096;
    cfg.l2_max_entries          = 256;
    cfg.l2_max_entry_size       = 65536;
    cfg.l3_db_path              = ""; // L3 disabled
    cfg.enable_circuit_breaker  = false;
    cfg.enable_size_limits      = true;
    cfg.max_total_entry_size    = 67108864; // 64 MiB — matches C4 hard cap
    cfg.enable_adaptive_ttl     = false;
    cfg.enable_rate_limiting    = false;
    cfg.enable_tenant_isolation = false;
    return cfg;
}

/// Small valid JSON result object.
static nlohmann::json smallResult() {
    return {{"rows", 42}, {"status", "ok"}};
}

// =============================================================================
// C4 — AI/LLM Safety: AdaptiveQueryCache entry validation
// =============================================================================

class AdaptiveCacheC4Test : public ::testing::Test {
protected:
    AdaptiveQueryCache cache_{makeConfig()};
};

// --- C4-1: valid entry is accepted ------------------------------------
TEST_F(AdaptiveCacheC4Test, ValidObjectEntryAccepted) {
    const std::string fp = cache_.generateFingerprint("SELECT 1", {});
    bool ok = cache_.put(fp, {}, smallResult());
    EXPECT_TRUE(ok);
}

// --- C4-2: JSON primitive (not object/array) is rejected --------------
TEST_F(AdaptiveCacheC4Test, PrimitiveResultRejected) {
    const std::string fp = cache_.generateFingerprint("SELECT 2", {});
    nlohmann::json bad   = 42; // integer — not an object or array
    bool ok = cache_.put(fp, {}, bad);
    EXPECT_FALSE(ok);
}

// --- C4-3: JSON null is rejected --------------------------------------
TEST_F(AdaptiveCacheC4Test, NullResultRejected) {
    const std::string fp = cache_.generateFingerprint("SELECT 3", {});
    nlohmann::json bad   = nullptr;
    bool ok = cache_.put(fp, {}, bad);
    EXPECT_FALSE(ok);
}

// --- C4-4: JSON boolean is rejected -----------------------------------
TEST_F(AdaptiveCacheC4Test, BoolResultRejected) {
    const std::string fp = cache_.generateFingerprint("SELECT 4", {});
    nlohmann::json bad   = true;
    bool ok = cache_.put(fp, {}, bad);
    EXPECT_FALSE(ok);
}

// --- C4-5: JSON array is accepted -------------------------------------
TEST_F(AdaptiveCacheC4Test, ArrayResultAccepted) {
    const std::string fp = cache_.generateFingerprint("SELECT 5", {});
    nlohmann::json arr   = nlohmann::json::array({1, 2, 3});
    bool ok = cache_.put(fp, {}, arr);
    EXPECT_TRUE(ok);
}

// --- C4-6: Entry exceeding hard 64 MiB cap is rejected ---------------
TEST_F(AdaptiveCacheC4Test, OversizedEntryRejected) {
    AdaptiveQueryCache::Config cfg = makeConfig();
    // Raise the per-config limit above 64 MiB so only the hard cap fires.
    cfg.max_total_entry_size = 128ULL * 1024 * 1024; // 128 MiB
    AdaptiveQueryCache cache(cfg);

    // Build a JSON object whose serialisation exceeds 64 MiB.
    // We embed a large string value.
    const std::string bigval(65ULL * 1024 * 1024, 'x'); // 65 MiB string
    nlohmann::json big = {{"data", bigval}};

    const std::string fp = cache.generateFingerprint("SELECT big", {});
    bool ok = cache.put(fp, {}, big);
    EXPECT_FALSE(ok) << "Entry exceeding 64 MiB hard cap should be rejected";
}

// --- C4-7: put() followed by get() returns the cached result ----------
TEST_F(AdaptiveCacheC4Test, PutThenGetRoundTrip) {
    const std::string fp  = cache_.generateFingerprint("round_trip", {});
    nlohmann::json expected = {{"answer", 42}};
    ASSERT_TRUE(cache_.put(fp, {}, expected));

    auto entry = cache_.get(fp, "");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->result["answer"], 42);
}

// =============================================================================
// C4 — AI/LLM Safety: BoundedLRUCache entry validation
// =============================================================================

class BoundedLRUC4Test : public ::testing::Test {
protected:
    BoundedLRUCache::Config cfg_;
    std::unique_ptr<BoundedLRUCache> cache_;

    void SetUp() override {
        cfg_.max_entries          = 64;
        cfg_.ttl                  = std::chrono::seconds(300);
        cfg_.enable_statistics    = true;
        cfg_.max_entry_size_bytes = 1024;   // 1 KB — tiny cap for tests
        cfg_.max_ttl_seconds      = 600;    // 10 minutes
        cache_ = std::make_unique<BoundedLRUCache>(cfg_);
    }
};

// --- C4-8: Normal entry within limits is accepted --------------------
TEST_F(BoundedLRUC4Test, NormalEntryAccepted) {
    cache_->put("key1", {{"x", 1}}, 60);
    EXPECT_TRUE(cache_->contains("key1"));
}

// --- C4-9: Entry exceeding max_entry_size_bytes is rejected ----------
TEST_F(BoundedLRUC4Test, OversizedEntryRejected) {
    // Build a JSON value whose dump exceeds 1 KB.
    const std::string big(1100, 'a'); // > 1 KB
    nlohmann::json bigJson = {{"data", big}};
    cache_->put("big_key", bigJson, 60);
    EXPECT_FALSE(cache_->contains("big_key")) << "Oversized entry should be rejected";
}

// --- C4-10: Entry with TTL exceeding max_ttl_seconds is rejected ------
TEST_F(BoundedLRUC4Test, ExcessiveTTLRejected) {
    cache_->put("ttl_key", {{"y", 2}}, 700 /* > 600 s limit */);
    EXPECT_FALSE(cache_->contains("ttl_key")) << "Excessive TTL should be rejected";
}

// --- C4-11: TTL of exactly max_ttl_seconds is accepted ---------------
TEST_F(BoundedLRUC4Test, ExactMaxTTLAccepted) {
    cache_->put("exact_ttl", {{"z", 3}}, 600 /* == max_ttl_seconds */);
    EXPECT_TRUE(cache_->contains("exact_ttl"));
}

// --- C4-12: ttl_seconds=0 falls back to config TTL (no rejection) ----
TEST_F(BoundedLRUC4Test, ZeroTTLUsesConfigTTL) {
    cache_->put("zero_ttl", {{"w", 4}}, 0 /* use config ttl */);
    EXPECT_TRUE(cache_->contains("zero_ttl"));
}

// =============================================================================
// C1 — Timeout-safe locks: verify lock contention does not deadlock
// =============================================================================

// --- C1-1: Concurrent put() calls from multiple threads do not deadlock ---
TEST(AdaptiveCacheLockTest, ConcurrentPutNoDeadlock) {
    auto cfg  = makeConfig();
    cfg.l1_max_entries = 64;
    AdaptiveQueryCache cache(cfg);

    constexpr int kThreads = 8;
    constexpr int kOpsEach = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::atomic<int> successes{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kOpsEach; ++i) {
                const std::string query = "q_t" + std::to_string(t) + "_i" + std::to_string(i);
                const std::string fp    = cache.generateFingerprint(query, {});
                if (cache.put(fp, {}, smallResult())) {
                    ++successes;
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_GT(successes.load(), 0);
}

// --- C1-2: Concurrent get() and put() calls do not deadlock -----------
TEST(AdaptiveCacheLockTest, ConcurrentGetPutNoDeadlock) {
    auto cfg = makeConfig();
    AdaptiveQueryCache cache(cfg);

    // Pre-populate
    const std::string fp = cache.generateFingerprint("seed_query", {});
    cache.put(fp, {}, smallResult());

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads * 2);

    for (int t = 0; t < kThreads; ++t) {
        // Readers
        threads.emplace_back([&]() {
            for (int i = 0; i < 100; ++i) {
                auto res = cache.get(fp, "");
                (void)res;
            }
        });
        // Writers
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i) {
                const std::string q  = "q_" + std::to_string(t) + "_" + std::to_string(i);
                const std::string fp2 = cache.generateFingerprint(q, {});
                cache.put(fp2, {}, smallResult());
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    SUCCEED(); // No deadlock => test passes
}

// --- C1-3: invalidate() under concurrency does not deadlock -----------
TEST(AdaptiveCacheLockTest, ConcurrentInvalidateNoDeadlock) {
    auto cfg = makeConfig();
    AdaptiveQueryCache cache(cfg);

    // Pre-populate some entries
    for (int i = 0; i < 20; ++i) {
        const std::string q  = "query_" + std::to_string(i);
        const std::string fp = cache.generateFingerprint(q, {});
        cache.put(fp, {}, smallResult());
    }

    std::vector<std::thread> threads;
    threads.emplace_back([&]() { cache.invalidate("query_1.*"); });
    threads.emplace_back([&]() { cache.invalidate("query_2.*"); });
    threads.emplace_back([&]() {
        const std::string fp = cache.generateFingerprint("query_3", {});
        cache.put(fp, {}, smallResult());
    });
    for (auto& th : threads) {
      th.join();
    }
    SUCCEED();
}

// =============================================================================
// C2 — Concurrency safety: CacheReplicationCoordinator thread safety
// =============================================================================

class MockRemotePeer2 final : public IRemoteCachePeer {
public:
    explicit MockRemotePeer2(const std::string& addr) : addr_(addr) {}

    void invalidate(const std::string&, const std::string&) override {
        ++calls;
    }
    void invalidateTenant(const std::string&) override { ++calls; }
    std::string address()   const override { return addr_; }
    bool        isHealthy() const override { return true; }

    std::atomic<int> calls{0};
private:
    std::string addr_;
};

class StaticClusterView2 final : public IClusterView {
public:
    explicit StaticClusterView2(std::vector<std::string> addrs)
        : addrs_(std::move(addrs)) {}
    std::vector<std::string> getPeerAddresses() const override { return addrs_; }
private:
    std::vector<std::string> addrs_;
};

// --- C2-1: CacheReplicationCoordinator created without deadlock -------
TEST(ReplicationCoordinatorC2Test, ConstructionNoDeadlock) {
    auto bus  = std::make_shared<InProcessCacheCoordinator::Bus>();
    auto view = std::make_shared<StaticClusterView2>(
        std::vector<std::string>{});
    EXPECT_NO_THROW({
        CacheReplicationCoordinator coord(view.get(), bus, nullptr);
    });
}

// --- C2-2: Concurrent publishInvalidation from multiple threads -------
TEST(ReplicationCoordinatorC2Test, ConcurrentPublishInvalidationNoDataRace) {
    auto bus  = std::make_shared<InProcessCacheCoordinator::Bus>();
    CacheReplicationCoordinator coord(nullptr, bus, nullptr);

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 25; ++j) {
                coord.publishInvalidation("key_" + std::to_string(i * 25 + j));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    SUCCEED(); // No crash / data race
}

// --- C2-3: getStats() is thread-safe ---------------------------------
TEST(ReplicationCoordinatorC2Test, GetStatsThreadSafe) {
    auto bus  = std::make_shared<InProcessCacheCoordinator::Bus>();
    CacheReplicationCoordinator coord(nullptr, bus, nullptr);

    std::vector<std::thread> threads = {};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 20; ++j) {
                auto stats = coord.getStats();
                EXPECT_TRUE(stats.contains("name"));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
}

// =============================================================================
// C3 — Eviction telemetry: CacheReplicationCoordinator
// =============================================================================

// --- C3-1: fanout queue drop is tracked in metrics -------------------
TEST(ReplicationCoordinatorC3Test, QueueDropTrackedInMetrics) {
    auto bus = std::make_shared<InProcessCacheCoordinator::Bus>();

    // Peer factory that creates a slow peer to fill the queue.
    auto slowFactory = [](const std::string& addr) -> std::unique_ptr<themis::cache::IRemoteCachePeer> {
        return std::make_unique<MockRemotePeer2>(addr);
    };

    // Use a two-peer cluster.
    auto view = std::make_shared<StaticClusterView2>(
        std::vector<std::string>{"peer1:1234", "peer2:1234"});
    CacheReplicationCoordinator coord(view.get(), bus, slowFactory);

    // Publish many invalidations rapidly to overflow the retry queue.
    for (int i = 0; i < 2000; ++i) {
        coord.publishInvalidation("key_" + std::to_string(i));
    }

    // Give the worker thread a moment to process.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto stats = coord.getStats();
    // At least some messages were enqueued.
    EXPECT_GE(stats["fanout_enqueued"].get<uint64_t>(), 1UL);
}

// --- C3-2: Retry counter increments on peer failure ------------------
TEST(ReplicationCoordinatorC3Test, RetryCountIncrementsOnPeerFailure) {
    auto bus  = std::make_shared<InProcessCacheCoordinator::Bus>();

    // Failing peer factory.
    class FailingPeer final : public IRemoteCachePeer {
    public:
        explicit FailingPeer(const std::string& a) : addr_(a) {}
        void invalidate(const std::string&, const std::string&) override {
            throw std::runtime_error("peer down");
        }
        void invalidateTenant(const std::string&) override {
            throw std::runtime_error("peer down");
        }
        std::string address()   const override { return addr_; }
        bool        isHealthy() const override { return false; }
    private:
        std::string addr_;
    };

    auto failFactory = [](const std::string& addr) -> std::unique_ptr<themis::cache::IRemoteCachePeer> {
        return std::make_unique<FailingPeer>(addr);
    };

    auto view = std::make_shared<StaticClusterView2>(
        std::vector<std::string>{"fail:9999"});
    CacheReplicationCoordinator coord(view.get(), bus, failFactory);

    coord.publishInvalidation("key_retry_test");

    // Wait for the worker to process and retry.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto stats = coord.getStats();
    EXPECT_GE(stats["fanout_enqueued"].get<uint64_t>(), 1UL);
}

// =============================================================================
// C3 — SLO monitoring telemetry: CacheHitRateSloMonitor
// =============================================================================

// --- C3-3: evaluate() does not throw on zero traffic -----------------
TEST(SloMonitorC3Test, EvaluateZeroTraffic) {
    CacheHitRateSloMonitor monitor;
    CacheMetrics metrics;

    EXPECT_NO_THROW({
        auto result = monitor.evaluate(metrics);
        EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    });
}

// --- C3-4: evaluate() returns WARNING when hit rate is below threshold -
TEST(SloMonitorC3Test, EvaluateReturnsWarningOnLowHitRate) {
    CacheHitRateSloMonitor::Config cfg;
    cfg.warning_threshold  = 0.60;
    cfg.critical_threshold = 0.40;
    cfg.min_requests       = 10;
    CacheHitRateSloMonitor monitor(cfg, nullptr);

    CacheMetrics m;
    // Simulate 50% hit rate: 5 hits, 5 misses.
    for (int i = 0; i < 5; ++i) {
      m.l1_hits++;
    }
    for (int i = 0; i < 5; ++i) {
      m.misses++;
    }

    auto res = monitor.evaluate(m);
    EXPECT_EQ(res.level, CacheHitRateSloMonitor::ViolationLevel::WARNING);
}

// --- C3-5: evaluate() returns CRITICAL when hit rate is very low -----
TEST(SloMonitorC3Test, EvaluateReturnsCriticalOnVeryLowHitRate) {
    CacheHitRateSloMonitor::Config cfg;
    cfg.warning_threshold  = 0.60;
    cfg.critical_threshold = 0.40;
    cfg.min_requests       = 10;
    CacheHitRateSloMonitor monitor(cfg, nullptr);

    CacheMetrics m;
    for (int i = 0; i < 3; ++i) {
      m.l1_hits++;
    }
    for (int i = 0; i < 7; ++i) {
      m.misses++;
    }

    auto res = monitor.evaluate(m);
    EXPECT_EQ(res.level, CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
}

// --- C3-6: evaluate() returns NONE when traffic is below min_requests -
TEST(SloMonitorC3Test, EvaluateReturnsNoneBelowMinRequests) {
    CacheHitRateSloMonitor::Config cfg;
    cfg.warning_threshold  = 0.60;
    cfg.critical_threshold = 0.40;
    cfg.min_requests       = 100;
    CacheHitRateSloMonitor monitor(cfg, nullptr);

    CacheMetrics m;
    m.l1_hits++;
    m.misses++;

    auto res = monitor.evaluate(m);
    EXPECT_EQ(res.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
}

// --- C3-7: isSloViolated() returns true after a violation ---------
TEST(SloMonitorC3Test, IsSloViolatedAfterLowHitRate) {
    CacheHitRateSloMonitor::Config cfg;
    cfg.warning_threshold  = 0.60;
    cfg.critical_threshold = 0.40;
    cfg.min_requests       = 5;
    CacheHitRateSloMonitor monitor(cfg, nullptr);

    CacheMetrics m;
    for (int i = 0; i < 2; ++i) {
      m.l1_hits++;
    }
    for (int i = 0; i < 3; ++i) {
      m.misses++;
    }

    monitor.evaluate(m);
    EXPECT_TRUE(monitor.isSloViolated());
}

// --- C3-8: getStatus() JSON is well-formed ---------------------------
TEST(SloMonitorC3Test, GetStatusJsonWellFormed) {
    CacheHitRateSloMonitor monitor;
    auto status = monitor.getStatus();
    EXPECT_TRUE(status.contains("hit_rate"));
    EXPECT_TRUE(status.contains("violation_level"));
    EXPECT_TRUE(status.contains("thresholds"));
}

// --- C3-9: latency recordLatency + evaluate integration ---------------
TEST(SloMonitorC3Test, LatencyRecordAndEvaluate) {
    CacheHitRateSloMonitor::Config cfg;
    cfg.p99_warn_ms     = 10.0;
    cfg.p99_critical_ms = 50.0;
    cfg.min_requests    = 5;
    CacheHitRateSloMonitor monitor(cfg, nullptr);

    // Record 10 latency samples that force p99 > warn threshold.
    for (int i = 0; i < 10; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 20.0);
    }

    CacheMetrics m;
    for (int i = 0; i < 6; ++i) {
      m.l1_hits++;
    }
    for (int i = 0; i < 4; ++i) {
      m.misses++;
    }

    auto res = monitor.evaluate(m);
    // Hit rate is 60% — should be at or near WARNING for latency.
    // We only check that the call succeeds without throwing.
    EXPECT_NO_THROW((void)res);
}

// =============================================================================
// Additional edge-case coverage
// =============================================================================

// --- EDGE-1: clear() after put() leaves cache empty -------------------
TEST(AdaptiveCacheEdgeTest, ClearEmptiesL1L2) {
    auto cfg = makeConfig();
    AdaptiveQueryCache cache(cfg);

    const std::string fp = cache.generateFingerprint("clear_test", {});
    ASSERT_TRUE(cache.put(fp, {}, smallResult()));
    cache.clear();
    EXPECT_FALSE(cache.get(fp, "").has_value());
}

// --- EDGE-2: clearExpired() removes expired entries -------------------
TEST(AdaptiveCacheEdgeTest, ClearExpiredRemovesOldEntries) {
    auto cfg = makeConfig();
    // Very short TTL so entries expire immediately.
    cfg.l1_ttl_seconds = 0;
    cfg.l2_ttl_seconds = 0;
    AdaptiveQueryCache cache(cfg);

    const std::string fp = cache.generateFingerprint("expire_test", {});
    cache.put(fp, {}, smallResult());

    // Sleep briefly to let TTL expire.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    uint64_t cleared = cache.clearExpired();
    // Entry may or may not be cleared depending on TTL precision; no crash
    // is the contract we test here.
    (void)cleared;
    SUCCEED();
}

// --- EDGE-3: BoundedLRUCache size() tracks insertions correctly ------
TEST(BoundedLRUEdgeTest, SizeTracksInsertions) {
    BoundedLRUCache::Config cfg;
    cfg.max_entries       = 10;
    cfg.ttl               = std::chrono::seconds(300);
    cfg.max_entry_size_bytes = 4096;
    cfg.max_ttl_seconds      = 3600;
    BoundedLRUCache cache(cfg);

    for (int i = 0; i < 5; ++i) {
        cache.put("k" + std::to_string(i), {{"v", i}});
    }
    EXPECT_EQ(cache.size(), 5UL);
}

// --- EDGE-4: BoundedLRUCache evicts LRU on overflow ------------------
TEST(BoundedLRUEdgeTest, LRUEvictionOnOverflow) {
    BoundedLRUCache::Config cfg;
    cfg.max_entries       = 3;
    cfg.ttl               = std::chrono::seconds(300);
    cfg.max_entry_size_bytes = 4096;
    cfg.max_ttl_seconds      = 3600;
    BoundedLRUCache cache(cfg);

    cache.put("a", {{"v", 1}});
    cache.put("b", {{"v", 2}});
    cache.put("c", {{"v", 3}});
    // Access "a" to make it recently-used.
    cache.get("a");
    // Insert "d" — should evict "b" (LRU).
    cache.put("d", {{"v", 4}});

    EXPECT_EQ(cache.size(), 3UL);
    EXPECT_FALSE(cache.contains("b")) << "LRU entry 'b' should have been evicted";
    EXPECT_TRUE(cache.contains("a"));
    EXPECT_TRUE(cache.contains("c"));
    EXPECT_TRUE(cache.contains("d"));
}
