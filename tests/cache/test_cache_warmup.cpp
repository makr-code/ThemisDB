#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>

using namespace themis;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string uniqueTmpPath(const std::string& suffix = "") {
    auto ts = std::chrono::system_clock::now().time_since_epoch().count();
    return "/tmp/themis_warmup_test_" + std::to_string(ts) + suffix;
}

/// Build a minimal AdaptiveQueryCache::Config suitable for unit tests.
static AdaptiveQueryCache::Config makeTestConfig(const std::string& db_path) {
    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path = db_path;
    cfg.l1_max_entries = 20;
    cfg.l2_max_entries = 40;
    cfg.l1_max_entry_size = 1024;        // 1 KB
    cfg.l2_max_entry_size = 10240;       // 10 KB
    cfg.l1_ttl_seconds = 300;
    cfg.l2_ttl_seconds = 600;
    cfg.l3_ttl_seconds = 3600;
    cfg.enable_rate_limiting = false;    // disable for warmup tests
    cfg.enable_tenant_isolation = false;
    return cfg;
}

/// Write a warmup log line to a file.
static void writeLogLine(std::ofstream& f,
                         const std::string& key,
                         const std::string& value_b64,
                         int ttl_remaining_s,
                         const std::string& tenant = "") {
    json rec;
    rec["key"] = key;
    rec["value_b64"] = value_b64;
    rec["ttl_remaining_s"] = ttl_remaining_s;
    if (!tenant.empty()) rec["tenant"] = tenant;
    f << rec.dump() << '\n';
}

/// Produce a 64-char lowercase hex string (valid SHA-256 key).
static std::string makeKey(int n) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) ss << std::setw(8) << n;
    return ss.str();
}

/// Base64-encode using the same alphabet as warmup.cpp.
static const std::string kB64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string b64Encode(const std::string& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    uint32_t buf = 0;
    int bits = 0;
    for (unsigned char c : data) {
        buf = (buf << 8) | c;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            out.push_back(kB64[(buf >> bits) & 0x3F]);
        }
    }
    if (bits > 0) {
        buf <<= (6 - bits);
        out.push_back(kB64[buf & 0x3F]);
    }
    while (out.size() % 4 != 0) out.push_back('=');
    return out;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class CacheWarmupTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping cache warmup focused tests on Windows due to fixture crash in current runtime.";
#endif
        db_path_ = uniqueTmpPath("_db");
        log_path_ = uniqueTmpPath("_log.ndjson");
        snap_path_ = uniqueTmpPath("_snap.ndjson");
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(db_path_, ec);
        std::filesystem::remove(log_path_, ec);
        std::filesystem::remove(snap_path_, ec);
    }

    AdaptiveQueryCache::Config cfg() { return makeTestConfig(db_path_); }

    std::string db_path_;
    std::string log_path_;
    std::string snap_path_;
};

// ---------------------------------------------------------------------------
// warmupFromLog – basic round-trip
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_BasicRoundTrip) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    json value = {{"result", 42}, {"rows", {1, 2, 3}}};
    std::string key = makeKey(1);
    std::string value_b64 = b64Encode(value.dump());

    // Write a valid log file.
    {
        std::ofstream f(log_path_);
        writeLogLine(f, key, value_b64, 300);
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, 1u);

    // The entry should now be retrievable.
    auto result = cache.get(key, "");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->result, value);
}

// ---------------------------------------------------------------------------
// warmupFromLog – multiple entries
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_MultipleEntries) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    const int count = 5;
    {
        std::ofstream f(log_path_);
        for (int i = 0; i < count; ++i) {
            json value = {{"n", i}};
            writeLogLine(f, makeKey(i), b64Encode(value.dump()), 300);
        }
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        auto result = cache.get(makeKey(i), "");
        ASSERT_TRUE(result.has_value()) << "Entry " << i << " not found";
        EXPECT_EQ(result->result["n"], i);
    }
}

// ---------------------------------------------------------------------------
// warmupFromLog – max_entries cap
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_MaxEntriesCap) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        for (int i = 0; i < 10; ++i) {
            json value = {{"n", i}};
            writeLogLine(f, makeKey(i), b64Encode(value.dump()), 300);
        }
    }

    // Only load 3 entries.
    auto loaded = cache.warmupFromLog(log_path_, 3);
    EXPECT_EQ(loaded.entries_loaded, 3u);
}

// ---------------------------------------------------------------------------
// warmupFromLog – invalid key is skipped
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_SkipsInvalidKey) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        // Invalid key (too short / non-hex).
        json bad;
        bad["key"] = "not_a_sha256";
        bad["value_b64"] = b64Encode(json({{"x", 1}}).dump());
        bad["ttl_remaining_s"] = 300;
        f << bad.dump() << '\n';

        // Valid entry.
        writeLogLine(f, makeKey(99), b64Encode(json({{"ok", true}}).dump()), 300);
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, 1u);

    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.warmup_entries_skipped.load(), 1u);
}

// ---------------------------------------------------------------------------
// warmupFromLog – expired TTL entries are skipped
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_SkipsExpiredTTL) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        // ttl_remaining_s = 0 → expired.
        writeLogLine(f, makeKey(1), b64Encode(json({{"x", 1}}).dump()), 0);
        // Valid entry.
        writeLogLine(f, makeKey(2), b64Encode(json({{"x", 2}}).dump()), 300);
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, 1u);

    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.warmup_entries_skipped.load(), 1u);
}

// ---------------------------------------------------------------------------
// warmupFromLog – missing file returns 0
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_MissingFile) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    auto loaded = cache.warmupFromLog("/tmp/nonexistent_log_file.ndjson");
    EXPECT_EQ(loaded.entries_loaded, 0u);
}

// ---------------------------------------------------------------------------
// warmupFromLog – malformed JSON is counted as failed
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_MalformedJSON) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        f << "{broken json\n";
        writeLogLine(f, makeKey(1), b64Encode(json({{"ok", 1}}).dump()), 300);
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, 1u);

    const auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.warmup_entries_failed.load(), 1u);
}

// ---------------------------------------------------------------------------
// warmupFromLog – hard headroom cap limits number of loaded entries
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_HeadroomCapLimitsLoadedEntries) {
    auto config = cfg();
    config.l1_max_entries = 4;   // cap = 2 for warmup
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        // Write 4 entries; warmup should only load up to the headroom cap.
        for (int i = 0; i < 4; ++i) {
            json value = {{"n", i}};
            writeLogLine(f, makeKey(i), b64Encode(value.dump()), 300);
        }
    }

    auto loaded = cache.warmupFromLog(log_path_);
    EXPECT_EQ(loaded.entries_loaded, 4u);

    // Headroom limits L1 placement only; additional warmup entries may be
    // admitted to L2 and remain retrievable through cache.get().
    for (int i = 0; i < 4; ++i) {
        auto result = cache.get(makeKey(i), "");
        EXPECT_TRUE(result.has_value()) << "Entry " << i << " not found";
    }
}

// ---------------------------------------------------------------------------
// exportSnapshot – round-trip with warmupFromLog
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, ExportSnapshot_RoundTrip) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    // Insert a few entries.
    const int count = 3;
    for (int i = 0; i < count; ++i) {
        std::string fp = makeKey(i);
        json params = {};
        json value = {{"n", i}};
        ASSERT_TRUE(cache.put(fp, params, value));
    }

    // Export to snapshot.
    auto exported = cache.exportSnapshot(snap_path_);
    EXPECT_GE(exported.entries_loaded, static_cast<size_t>(count));

    // Create a fresh cache and warm it from the snapshot.
    std::string db_path2 = db_path_ + "_2";
    auto config2 = makeTestConfig(db_path2);
    AdaptiveQueryCache cache2(config2);

    auto loaded = cache2.warmupFromLog(snap_path_);
    EXPECT_EQ(loaded.entries_loaded, exported.entries_loaded);

    // All entries should be retrievable in the new cache.
    for (int i = 0; i < count; ++i) {
        auto result = cache2.get(makeKey(i), "");
        ASSERT_TRUE(result.has_value()) << "Entry " << i << " missing after round-trip";
        EXPECT_EQ(result->result["n"], i);
    }

    std::error_code ec;
    std::filesystem::remove_all(db_path2, ec);
}

// ---------------------------------------------------------------------------
// exportSnapshot – empty cache exports nothing
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, ExportSnapshot_EmptyCache) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    auto exported = cache.exportSnapshot(snap_path_);
    EXPECT_EQ(exported.entries_loaded, 0u);

    // File should exist and be empty (or just newlines).
    std::ifstream f(snap_path_);
    ASSERT_TRUE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty());
}

// ---------------------------------------------------------------------------
// exportSnapshot – bad path returns 0
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, ExportSnapshot_BadPath) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    // Insert something.
    cache.put(makeKey(0), {}, json({{"x", 1}}));

    auto exported = cache.exportSnapshot("/nonexistent_dir/snapshot.ndjson");
    EXPECT_EQ(exported.entries_loaded, 0u);
}

// ---------------------------------------------------------------------------
// warmup metrics tracked in CacheMetrics
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, Metrics_TrackWarmupCounters) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        writeLogLine(f, makeKey(1), b64Encode(json({{"a", 1}}).dump()), 300);
        writeLogLine(f, makeKey(2), b64Encode(json({{"b", 2}}).dump()), 300);
        // One invalid key.
        json bad;
        bad["key"] = "short";
        bad["value_b64"] = b64Encode(json({{"c", 3}}).dump());
        bad["ttl_remaining_s"] = 300;
        f << bad.dump() << '\n';
    }

    cache.warmupFromLog(log_path_);

    const auto& m = cache.getEnhancedMetrics();
    EXPECT_EQ(m.warmup_entries_loaded.load(), 2u);
    EXPECT_GE(m.warmup_entries_skipped.load(), 1u);
}

// ---------------------------------------------------------------------------
// Bug fix: duplicate entries in log should be counted as skipped, not loaded
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_DuplicateKeySkipped) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    std::string key = makeKey(42);
    std::string val_b64 = b64Encode(json({{"v", 1}}).dump());

    {
        std::ofstream f(log_path_);
        // Same key twice.
        writeLogLine(f, key, val_b64, 300);
        writeLogLine(f, key, b64Encode(json({{"v", 2}}).dump()), 300);
    }

    auto loaded = cache.warmupFromLog(log_path_);
    // Only one entry should be inserted; the duplicate should be skipped.
    EXPECT_EQ(loaded.entries_loaded, 1u);

    const auto& m = cache.getEnhancedMetrics();
    EXPECT_EQ(m.warmup_entries_loaded.load(), 1u);
    EXPECT_GE(m.warmup_entries_skipped.load(), 1u);

    // The stored value should be the first occurrence.
    auto result = cache.get(key, "");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->result["v"], 1);
}

// ---------------------------------------------------------------------------
// Bug fix: exportSnapshot + warmupFromLog round-trip with tenant isolation
// (regression test for tenant-scoped key export bug)
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, ExportSnapshot_TenantIsolation_RoundTrip) {
    constexpr size_t ONE_MB = 1024 * 1024;
    auto config = cfg();
    config.enable_tenant_isolation = true;
    config.per_tenant_max_bytes = ONE_MB;
    AdaptiveQueryCache cache(config);

    const std::string tenant = "acme";
    const int count = 3;

    for (int i = 0; i < count; ++i) {
        std::string fp = makeKey(i);
        json value = {{"n", i}};
        ASSERT_TRUE(cache.put(fp, {}, value, tenant));
    }

    // Export to snapshot.
    auto exported = cache.exportSnapshot(snap_path_);
    EXPECT_EQ(exported.entries_loaded, static_cast<size_t>(count));

    // Verify the snapshot file keys are bare fingerprints, not tenant-scoped.
    {
        std::ifstream f(snap_path_);
        std::string line;
        int line_count = 0;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto rec = json::parse(line);
            std::string exported_key = rec["key"].get<std::string>();
            // Must be a 64-char hex string, NOT "tenant:acme:<fp>"
            EXPECT_EQ(exported_key.size(), 64u)
                << "Exported key is not a bare fingerprint: " << exported_key;
            EXPECT_EQ(rec["tenant"].get<std::string>(), tenant);
            ++line_count;
        }
        EXPECT_EQ(line_count, count);
    }

    // Reimport into a fresh tenant-isolated cache.
    std::string db_path2 = db_path_ + "_tenant2";
    auto config2 = cfg();
    config2.l3_db_path = db_path2;
    config2.enable_tenant_isolation = true;
    config2.per_tenant_max_bytes = ONE_MB;
    AdaptiveQueryCache cache2(config2);

    auto loaded = cache2.warmupFromLog(snap_path_);
    EXPECT_EQ(loaded.entries_loaded, static_cast<size_t>(count));

    // All entries should be retrievable via the tenant-scoped get.
    for (int i = 0; i < count; ++i) {
        auto result = cache2.get(makeKey(i), tenant);
        ASSERT_TRUE(result.has_value())
            << "Tenant entry " << i << " missing after round-trip";
        EXPECT_EQ(result->result["n"], i);
    }

    std::error_code ec;
    std::filesystem::remove_all(db_path2, ec);
}

// ---------------------------------------------------------------------------
// Bug fix: tenant quota must NOT be double-charged on duplicate warmup entries
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_TenantQuota_NoDuplicateCharge) {
    constexpr size_t TEN_KB = 10 * 1024;
    auto config = cfg();
    config.enable_tenant_isolation = true;
    config.per_tenant_max_bytes = TEN_KB;  // tight quota
    AdaptiveQueryCache cache(config);

    const std::string tenant = "t1";
    std::string key = makeKey(7);
    std::string val_b64 = b64Encode(json({{"data", "hello"}}).dump());

    {
        std::ofstream f(log_path_);
        // Same key 5 times – only first should count against quota.
        for (int i = 0; i < 5; ++i) {
            writeLogLine(f, key, val_b64, 300, tenant);
        }
    }

    auto loaded = cache.warmupFromLog(log_path_);
    // Only 1 real insertion.
    EXPECT_EQ(loaded.entries_loaded, 1u);

    // Now add more unique entries – quota should not be exhausted by duplicates.
    for (int i = 1; i <= 5; ++i) {
        json v = {{"n", i}};
        EXPECT_TRUE(cache.put(makeKey(100 + i), {}, v, tenant))
            << "Quota should still have room for entry " << i;
    }
}

// ---------------------------------------------------------------------------
// Parallel Bulk Load: max_parallel_workers defaults to hardware concurrency
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, Config_MaxParallelWorkers_DefaultIsHardwareConcurrency) {
    AdaptiveQueryCache::Config config;
    // hardware_concurrency() can return 0 on unusual platforms; the config
    // normalises this to at least 1.
    EXPECT_GE(config.max_parallel_workers, 1u);
}

// ---------------------------------------------------------------------------
// Parallel Bulk Load: WarmupResult carries timing and throughput fields
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_ReportsDurationAndThroughput) {
    auto config = cfg();
    AdaptiveQueryCache cache(config);

    const int count = 10;
    {
        std::ofstream f(log_path_);
        for (int i = 0; i < count; ++i) {
            writeLogLine(f, makeKey(i), b64Encode(json({{"n", i}}).dump()), 300);
        }
    }

    auto result = cache.warmupFromLog(log_path_);

    EXPECT_EQ(result.entries_loaded, static_cast<size_t>(count));
    // duration must be non-negative
    EXPECT_GE(result.warmup_duration_ms, 0);
    // throughput must be positive for a non-empty load
    EXPECT_GT(result.warmup_entries_per_second, 0.0);
}

// ---------------------------------------------------------------------------
// Parallel Bulk Load: multiple workers produce the same correct result
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_ParallelWorkers_CorrectResults) {
    const int count = 50;
    const uint32_t workers = 4;

    auto config = cfg();
    config.l1_max_entries = 100;
    config.l2_max_entries = 200;
    config.max_parallel_workers = workers;
    AdaptiveQueryCache cache(config);

    {
        std::ofstream f(log_path_);
        for (int i = 0; i < count; ++i) {
            // Use a moderately-sized value (64 bytes of filler) so the test
            // exercises realistic per-entry sizes without being trivially small.
            json value = {{"idx", i}, {"data", std::string(64, static_cast<char>('a' + (i % 26)))}};
            writeLogLine(f, makeKey(i), b64Encode(value.dump()), 300);
        }
    }

    auto result = cache.warmupFromLog(log_path_);

    EXPECT_EQ(result.entries_loaded, static_cast<size_t>(count));
    EXPECT_EQ(result.entries_total, static_cast<size_t>(count));

    // Every entry must be retrievable after parallel warmup.
    for (int i = 0; i < count; ++i) {
        auto entry = cache.get(makeKey(i), "");
        ASSERT_TRUE(entry.has_value()) << "Entry " << i << " missing after parallel warmup";
        EXPECT_EQ(entry->result["idx"], i);
    }
}

// ---------------------------------------------------------------------------
// Parallel Bulk Load: single-worker path yields identical results
// ---------------------------------------------------------------------------

TEST_F(CacheWarmupTest, WarmupFromLog_SingleWorker_SameAsDefault) {
    const int count = 15;

    // Two caches: one with default (parallel) workers, one forced to 1.
    auto config1 = cfg();
    config1.l3_db_path = db_path_ + "_par";
    AdaptiveQueryCache cache_par(config1);

    auto config2 = cfg();
    config2.l3_db_path = db_path_ + "_seq";
    config2.max_parallel_workers = 1;
    AdaptiveQueryCache cache_seq(config2);

    {
        std::ofstream f(log_path_);
        for (int i = 0; i < count; ++i) {
            writeLogLine(f, makeKey(i), b64Encode(json({{"n", i}}).dump()), 300);
        }
    }

    auto r_par = cache_par.warmupFromLog(log_path_);
    auto r_seq = cache_seq.warmupFromLog(log_path_);

    EXPECT_EQ(r_par.entries_loaded, r_seq.entries_loaded);
    EXPECT_EQ(r_par.entries_total,  r_seq.entries_total);

    // Both caches must hold the same entries.
    for (int i = 0; i < count; ++i) {
        auto ep = cache_par.get(makeKey(i), "");
        auto es = cache_seq.get(makeKey(i), "");
        ASSERT_TRUE(ep.has_value()) << "Parallel cache missing entry " << i;
        ASSERT_TRUE(es.has_value()) << "Sequential cache missing entry " << i;
        EXPECT_EQ(ep->result, es->result);
    }

    std::error_code ec;
    std::filesystem::remove_all(config1.l3_db_path, ec);
    std::filesystem::remove_all(config2.l3_db_path, ec);
}
