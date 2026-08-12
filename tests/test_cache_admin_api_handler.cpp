#include <gtest/gtest.h>
#include "server/cache_admin_api_handler.h"
#include "cache/adaptive_query_cache.h"
#include "cache/distributed_cache_coordinator.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace http = boost::beast::http;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& body = "")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

// Minimal base64 encode helper (mirrors the decode in the handler)
static std::string base64Encode(const std::string& input) {
    static const char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class CacheAdminApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping cache admin API handler focused tests on Windows due to fixture crash in current runtime.";
#endif
        db_path_ = "/tmp/themis_test_cache_admin_" +
                   std::to_string(std::chrono::system_clock::now()
                                      .time_since_epoch()
                                      .count());

        themis::AdaptiveQueryCache::Config cfg;
        cfg.l3_db_path = db_path_;
        cfg.enable_circuit_breaker = true;
        cfg.enable_tenant_isolation = true;
        cfg.l1_max_entries = 50;
        cfg.l2_max_entries = 100;

        cache_ = std::make_shared<themis::AdaptiveQueryCache>(cfg);
        // auth = nullptr → auth disabled (dev mode)
        handler_ = std::make_unique<themis::server::CacheAdminApiHandler>(
            cache_, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        cache_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<themis::AdaptiveQueryCache> cache_;
    std::unique_ptr<themis::server::CacheAdminApiHandler> handler_;
};

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/stats
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, StatsReturns200WithJsonBody) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");

    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("stats"));
    EXPECT_TRUE(body.contains("circuit_breaker"));
    EXPECT_TRUE(body.contains("health"));
}

TEST_F(CacheAdminApiHandlerTest, StatsCircuitBreakerInitiallyClosed) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("circuit_breaker"));
    EXPECT_EQ(body["circuit_breaker"]["state"], "CLOSED");
}

TEST_F(CacheAdminApiHandlerTest, StatsIncludesSloLatencyWhenMonitorAttached) {
    auto monitor = std::make_shared<themis::cache::CacheHitRateSloMonitor>();
    handler_->setSloMonitor(monitor);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("slo"));
    EXPECT_TRUE(body["slo"].contains("p50_ms"));
    EXPECT_TRUE(body["slo"].contains("p95_ms"));
    EXPECT_TRUE(body["slo"].contains("p99_ms"));
}

TEST_F(CacheAdminApiHandlerTest, StatsOmitsSloWhenMonitorDetached) {
    handler_->setSloMonitor(std::make_shared<themis::cache::CacheHitRateSloMonitor>());
    handler_->setSloMonitor(nullptr);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_FALSE(body.contains("slo"));
}

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/circuit-breaker
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, CircuitBreakerStatusReturns200) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/circuit-breaker");
    auto res = handler_->handleCircuitBreakerStatus(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("state"));
    EXPECT_TRUE(body.contains("enabled"));
    EXPECT_EQ(body["state"], "CLOSED");
    EXPECT_TRUE(body["enabled"].get<bool>());
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/circuit-breaker/reset
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, CircuitBreakerResetReturns200) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/circuit-breaker/reset");
    auto res = handler_->handleCircuitBreakerReset(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ok");
    EXPECT_EQ(body["circuit_breaker"]["state"], "CLOSED");
}

// ---------------------------------------------------------------------------
// Tests: DELETE /v1/admin/cache/key/{encoded_key}
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns200ForValidKey) {
    // Pre-populate cache
    std::string fp = cache_->generateFingerprint("SELECT 1", {}, "");
    bool inserted = cache_->put(fp, {}, json::object(), "");
    ASSERT_TRUE(inserted) << "put() must succeed for this test to be meaningful";
    // Verify the entry is actually present before evicting.
    ASSERT_TRUE(cache_->get(fp, "").has_value()) << "entry must be retrievable after put()";

    std::string encoded = base64Encode(fp);
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/" + encoded);
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["key"], fp);
    // At least one entry (L1) must have been evicted.
    EXPECT_GE(body["evicted"].get<int>(), 1);
}

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns400ForMissingKey) {
    // Target ends exactly at the prefix with no trailing segment
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/");
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns400ForInvalidBase64Token) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/not*base64");
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns400ForSegmentSmugglingInToken) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/abcd/extra");
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns400ForInvalidBase64PaddingPlacement) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/YWJj=Zg==");
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// Regression test: evicting by fingerprint must also evict tenant-prefixed
// entries stored under "tenant:{id}:{fingerprint}" in L1/L2.
TEST_F(CacheAdminApiHandlerTest, EvictKeyAlsoEvictsTenantPrefixedEntry) {
    const std::string tenant = "acme";
    std::string fp = cache_->generateFingerprint("SELECT 2", {}, tenant);
    // With enable_tenant_isolation=true the L1 key is "tenant:acme:<fp>".
    bool inserted = cache_->put(fp, {}, json::object(), tenant);
    ASSERT_TRUE(inserted);
    // Verify the entry is actually present before evicting.
    ASSERT_TRUE(cache_->get(fp, tenant).has_value());

    std::string encoded = base64Encode(fp);
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/" + encoded);
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    // The tenant-scoped L1 entry "tenant:acme:<fp>" must have been evicted.
    EXPECT_GE(body["evicted"].get<int>(), 1);
}

// ---------------------------------------------------------------------------
// Tests: DELETE /v1/admin/cache/tenant/{tenant_id}
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns200) {
    // Pre-populate with a tenant entry
    std::string fp = cache_->generateFingerprint("SELECT 1", {}, "acme");
    bool inserted = cache_->put(fp, {}, json::object(), "acme");
    ASSERT_TRUE(inserted) << "put() must succeed for this test to be meaningful";

    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/acme");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["tenant_id"], "acme");
    // At least the one entry we inserted must have been evicted.
    EXPECT_GE(body["evicted"].get<int>(), 1);
}

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns400ForMissingTenantId) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns400ForInvalidTenantId) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/../bad");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Tests: null cache guard
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, Returns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);

    auto req_stats   = makeRequest(http::verb::get,  "/v1/admin/cache/stats");
    auto req_warmup  = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                                   R"({"log_path":"/tmp/dummy.ndjson"})");
    auto req_snap    = makeRequest(http::verb::post, "/v1/admin/cache/snapshot",
                                   R"({"out_path":"/tmp/dummy.ndjson"})");

    EXPECT_EQ(handler_no_cache->handleStats(req_stats).result(),
              http::status::service_unavailable);
    EXPECT_EQ(handler_no_cache->handleWarmup(req_warmup).result(),
              http::status::service_unavailable);
    EXPECT_EQ(handler_no_cache->handleSnapshot(req_snap).result(),
              http::status::service_unavailable);
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/warmup
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForMissingLogPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup", "{}");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForEmptyLogPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":""})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForInvalidLogPathTraversal) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":"../secrets.ndjson"})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForTooLargeMaxEntries) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":"C:/tmp/warmup.ndjson","max_entries":1000001})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns500ForNonExistentFile) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":"/tmp/no_such_file_xyz_abc.ndjson"})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::internal_server_error);
}

TEST_F(CacheAdminApiHandlerTest, WarmupLoadsValidEntries) {
    // Write a minimal NDJSON warmup log
    const std::string log_path = db_path_ + "_warmup.ndjson";

    // Use a valid fingerprint (64 hex chars) and a base64-encoded JSON value
    const std::string fp = "a1b2c3d4e5f60718293a4b5c6d7e8f9001234567890abcdef1234567890abcde";
    const std::string value = R"({"rows":[]})";
    const std::string value_b64 = base64Encode(value);

    {
        std::ofstream f(log_path);
        ASSERT_TRUE(f.is_open());
        f << R"({"key":")" << fp << R"(","value_b64":")" << value_b64
          << R"(","ttl_remaining_s":300,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ok");
    EXPECT_GE(body["entries_loaded"].get<int>(), 1);
    EXPECT_EQ(body["entries_total"].get<int>(), 1);

    // Verify the entry is actually retrievable from the cache (not just counted),
    // and that warmup correctly restored the original JSON content.
    auto entry = cache_->get(fp, "");
    ASSERT_TRUE(entry.has_value()) << "warmed-up entry must be retrievable via get()";
    EXPECT_EQ(entry->result, json::parse(R"({"rows":[]})"))
        << "warmed-up entry must contain the original JSON value";
}

TEST_F(CacheAdminApiHandlerTest, WarmupSkipsMalformedLines) {
    const std::string log_path = db_path_ + "_warmup_bad.ndjson";
    {
        std::ofstream f(log_path);
        f << "not-json\n";
        f << R"({"key":"tooshort","value_b64":"dGVzdA==","ttl_remaining_s":300})" << '\n';
        // Valid entry
        const std::string fp = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
        f << R"({"key":")" << fp << R"(","value_b64":")" << base64Encode(R"({"x":1})")
          << R"(","ttl_remaining_s":300,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["entries_total"].get<int>(), 3);
    EXPECT_EQ(body["entries_loaded"].get<int>(), 1);
    EXPECT_EQ(body["entries_skipped"].get<int>(), 2);
}

TEST_F(CacheAdminApiHandlerTest, WarmupSkipsExpiredEntries) {
    const std::string log_path = db_path_ + "_warmup_expired.ndjson";
    const std::string fp = "cafebabecafebabecafebabecafebabecafebabecafebabecafebabecafebabe";
    {
        std::ofstream f(log_path);
        // ttl_remaining_s = 0 → already expired
        f << R"({"key":")" << fp << R"(","value_b64":")" << base64Encode(R"({})")
          << R"(","ttl_remaining_s":0,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["entries_loaded"].get<int>(), 0);
    EXPECT_EQ(body["entries_skipped"].get<int>(), 1);
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/snapshot
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, SnapshotReturns400ForMissingOutPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/snapshot", "{}");
    auto res = handler_->handleSnapshot(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, SnapshotReturns400ForInvalidOutPathTraversal) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/snapshot",
                           R"({"out_path":"../snapshot.ndjson"})");
    auto res = handler_->handleSnapshot(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, SnapshotExportsAndReloads) {
    // Insert two entries into the cache
    const std::string fp1 = cache_->generateFingerprint("SELECT 10", {}, "");
    const std::string fp2 = cache_->generateFingerprint("SELECT 11", {}, "");
    ASSERT_TRUE(cache_->put(fp1, {}, json::parse(R"({"result":"r1"})"), ""));
    ASSERT_TRUE(cache_->put(fp2, {}, json::parse(R"({"result":"r2"})"), ""));

    const std::string snap_path = db_path_ + "_snapshot.ndjson";

    // Export
    {
        auto req = makeRequest(http::verb::post, "/v1/admin/cache/snapshot",
                               R"({"out_path":")" + snap_path + R"("})");
        auto res = handler_->handleSnapshot(req);
        EXPECT_EQ(res.result(), http::status::ok);
        json body = json::parse(res.body());
        EXPECT_EQ(body["status"], "ok");
        EXPECT_GE(body["entries_exported"].get<int>(), 2);
    }

    // Clear cache and reload from snapshot
    cache_->clear();
    EXPECT_FALSE(cache_->get(fp1, "").has_value());

    {
        auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                               R"({"log_path":")" + snap_path + R"("})");
        auto res = handler_->handleWarmup(req);
        EXPECT_EQ(res.result(), http::status::ok);
        json body = json::parse(res.body());
        EXPECT_GE(body["entries_loaded"].get<int>(), 2);
    }

    // Verify entries are back
    EXPECT_TRUE(cache_->get(fp1, "").has_value());
    EXPECT_TRUE(cache_->get(fp2, "").has_value());

    std::filesystem::remove(snap_path);
}

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/tenants
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, ListTenantsReturnsEnabledWhenTenantIsolationOn) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenants");
    auto res = handler_->handleListTenants(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("enabled"));
    EXPECT_TRUE(body["enabled"].get<bool>());
}

TEST_F(CacheAdminApiHandlerTest, ListTenantsIncludesHitMissStatsAfterAccess) {
    const std::string tenant = "stats_tenant";
    std::string fp = cache_->generateFingerprint("SELECT stats", {}, tenant);

    // One put + one hit + one miss
    ASSERT_TRUE(cache_->put(fp, {}, json::object(), tenant));
    cache_->get(fp, tenant);  // hit

    std::string fp2 = cache_->generateFingerprint("SELECT miss", {}, tenant);
    cache_->get(fp2, tenant);  // miss

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenants");
    auto res = handler_->handleListTenants(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("tenants"));
    ASSERT_TRUE(body["tenants"].contains(tenant));
    const auto& t = body["tenants"][tenant];
    EXPECT_GE(t["hits"].get<int>(), 1);
    EXPECT_GE(t["misses"].get<int>(), 1);
    EXPECT_TRUE(t.contains("hit_rate"));
    EXPECT_TRUE(t.contains("evictions"));
    EXPECT_TRUE(t.contains("bytes_used"));
    EXPECT_TRUE(t.contains("quota"));
}

TEST_F(CacheAdminApiHandlerTest, ListTenantsReturns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenants");
    EXPECT_EQ(handler_no_cache->handleListTenants(req).result(),
              http::status::service_unavailable);
}

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/tenant/{tenant_id}/stats
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);
    auto req = makeRequest(http::verb::get,
                           "/v1/admin/cache/tenant/acme/stats");
    EXPECT_EQ(handler_no_cache->handleTenantStats(req).result(),
              http::status::service_unavailable);
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturns404ForUnknownTenant) {
    auto req = makeRequest(http::verb::get,
                           "/v1/admin/cache/tenant/no_such_tenant/stats");
    auto res = handler_->handleTenantStats(req);
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturns400ForMissingTenantId) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenant//stats");
    auto res = handler_->handleTenantStats(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturns400ForInvalidTenantId) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenant/../bad/stats");
    auto res = handler_->handleTenantStats(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturns400ForTrailingPathAfterStats) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/tenant/acme/stats/extra");
    auto res = handler_->handleTenantStats(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsReturnsCorrectMetrics) {
    const std::string tenant = "perf_tenant";
    std::string fp = cache_->generateFingerprint("SELECT perf", {}, tenant);
    ASSERT_TRUE(cache_->put(fp, {}, json::array(), tenant));

    // Generate a hit and a miss for this tenant
    cache_->get(fp, tenant);   // hit

    std::string fp_miss = cache_->generateFingerprint("SELECT miss2", {}, tenant);
    cache_->get(fp_miss, tenant);  // miss

    auto req = makeRequest(http::verb::get,
                           "/v1/admin/cache/tenant/" + tenant + "/stats");
    auto res = handler_->handleTenantStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body["found"].get<bool>());
    EXPECT_EQ(body["tenant_id"], tenant);
    EXPECT_GE(body["hits"].get<int>(), 1);
    EXPECT_GE(body["misses"].get<int>(), 1);
    EXPECT_TRUE(body.contains("hit_rate"));
    EXPECT_TRUE(body.contains("evictions"));
    EXPECT_TRUE(body.contains("bytes_used"));
    EXPECT_TRUE(body.contains("quota"));
    EXPECT_TRUE(body.contains("utilization"));
}

TEST_F(CacheAdminApiHandlerTest, TenantStatsEvictionsIncrementAfterEvict) {
    const std::string tenant = "evict_stats_tenant";
    std::string fp = cache_->generateFingerprint("SELECT evict", {}, tenant);
    ASSERT_TRUE(cache_->put(fp, {}, json::object(), tenant));

    // Evict all entries for the tenant
    auto evict_req = makeRequest(http::verb::delete_,
                                 "/v1/admin/cache/tenant/" + tenant);
    auto evict_res = handler_->handleEvictTenant(evict_req);
    EXPECT_EQ(evict_res.result(), http::status::ok);

    // Now query tenant stats
    auto req = makeRequest(http::verb::get,
                           "/v1/admin/cache/tenant/" + tenant + "/stats");
    auto res = handler_->handleTenantStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body["found"].get<bool>());
    EXPECT_GE(body["evictions"].get<int>(), 1);
}

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/health
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, HealthReturns200WithJsonBody) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");

    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("healthy"));
    EXPECT_TRUE(body.contains("warnings"));
    EXPECT_TRUE(body.contains("tiers"));
    EXPECT_TRUE(body.contains("circuit_breaker"));
}

TEST_F(CacheAdminApiHandlerTest, HealthContainsPerTierStatus) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());

    ASSERT_TRUE(body.contains("tiers"));
    const auto& tiers = body["tiers"];

    // L1 tier
    ASSERT_TRUE(tiers.contains("l1"));
    EXPECT_TRUE(tiers["l1"].contains("status"));
    EXPECT_TRUE(tiers["l1"].contains("entries"));
    EXPECT_TRUE(tiers["l1"].contains("max_entries"));
    EXPECT_TRUE(tiers["l1"].contains("utilization"));
    EXPECT_TRUE(tiers["l1"].contains("ttl_seconds"));
    EXPECT_EQ(tiers["l1"]["status"], "OK");

    // L2 tier
    ASSERT_TRUE(tiers.contains("l2"));
    EXPECT_TRUE(tiers["l2"].contains("status"));
    EXPECT_TRUE(tiers["l2"].contains("entries"));
    EXPECT_TRUE(tiers["l2"].contains("max_entries"));
    EXPECT_TRUE(tiers["l2"].contains("utilization"));
    EXPECT_TRUE(tiers["l2"].contains("ttl_seconds"));
    EXPECT_EQ(tiers["l2"]["status"], "OK");

    // L3 tier
    ASSERT_TRUE(tiers.contains("l3"));
    EXPECT_TRUE(tiers["l3"].contains("status"));
    EXPECT_TRUE(tiers["l3"].contains("enabled"));
    EXPECT_TRUE(tiers["l3"].contains("path"));
    EXPECT_TRUE(tiers["l3"].contains("ttl_seconds"));
}

TEST_F(CacheAdminApiHandlerTest, HealthContainsCircuitBreakerState) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());

    ASSERT_TRUE(body.contains("circuit_breaker"));
    EXPECT_TRUE(body["circuit_breaker"].contains("state"));
    EXPECT_TRUE(body["circuit_breaker"].contains("enabled"));
    EXPECT_EQ(body["circuit_breaker"]["state"], "CLOSED");
}

TEST_F(CacheAdminApiHandlerTest, HealthReturns200WhenHealthy) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body["healthy"].get<bool>());
}

TEST_F(CacheAdminApiHandlerTest, HealthReturns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    EXPECT_EQ(handler_no_cache->handleHealth(req).result(),
              http::status::service_unavailable);
}

TEST_F(CacheAdminApiHandlerTest, HealthContainsCoordinatorField) {
    // When no coordinator is registered the health response must still contain
    // a "coordinator" key (with enabled: false).
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());

    ASSERT_TRUE(body.contains("coordinator"))
        << "health response must include 'coordinator' key";
    EXPECT_FALSE(body["coordinator"]["enabled"].get<bool>());
}

TEST_F(CacheAdminApiHandlerTest, HealthCoordinatorConnectedFalseWhenOffline) {
    // Register an offline coordinator and verify the health field reflects the
    // disconnected state.
    themis::cache::RedisCacheCoordinatorConfig cfg;
    cfg.host               = "127.0.0.1";
    cfg.port               = 16399;   // Nothing listening
    cfg.connect_timeout_ms = 100;
    cfg.reconnect_interval_ms = 5000;

    auto coord = std::make_shared<themis::cache::RedisCacheCoordinator>(cfg);
    cache_->setCoordinator(coord);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/health");
    auto res = handler_->handleHealth(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());

    ASSERT_TRUE(body.contains("coordinator"));
    EXPECT_TRUE(body["coordinator"]["enabled"].get<bool>());
    EXPECT_FALSE(body["coordinator"]["connected"].get<bool>());
    EXPECT_TRUE(body["coordinator"].contains("name"));
}

// ---------------------------------------------------------------------------
// Tests: PATCH /v1/admin/cache/tenant/{tenant_id}/quota
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns200) {
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant/acme/quota",
                           R"({"quota_bytes":52428800})");
    auto res = handler_->handleUpdateTenantQuota(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ok");
    EXPECT_EQ(body["tenant_id"], "acme");
    EXPECT_EQ(body["quota_bytes"].get<size_t>(), 52428800ULL);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReflectedInTenantStats) {
    const std::string tenant = "quota_test_tenant";
    const size_t new_quota = 52428800;  // 50 MB

    // Update quota
    auto patch_req = makeRequest(http::verb::patch,
                                 "/v1/admin/cache/tenant/" + tenant + "/quota",
                                 R"({"quota_bytes":)" + std::to_string(new_quota) + "}");
    auto patch_res = handler_->handleUpdateTenantQuota(patch_req);
    EXPECT_EQ(patch_res.result(), http::status::ok);

    // Verify quota is reflected in per-tenant stats
    auto stats_req = makeRequest(http::verb::get,
                                 "/v1/admin/cache/tenant/" + tenant + "/stats");
    auto stats_res = handler_->handleTenantStats(stats_req);

    EXPECT_EQ(stats_res.result(), http::status::ok);
    json body = json::parse(stats_res.body());
    EXPECT_TRUE(body["found"].get<bool>());
    EXPECT_EQ(body["quota"].get<size_t>(), new_quota);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaZeroResetToGlobalDefault) {
    const std::string tenant = "reset_quota_tenant";

    // First set a custom quota
    auto patch_req = makeRequest(http::verb::patch,
                                 "/v1/admin/cache/tenant/" + tenant + "/quota",
                                 R"({"quota_bytes":52428800})");
    EXPECT_EQ(handler_->handleUpdateTenantQuota(patch_req).result(), http::status::ok);

    // Reset to global default (quota_bytes=0)
    auto reset_req = makeRequest(http::verb::patch,
                                 "/v1/admin/cache/tenant/" + tenant + "/quota",
                                 R"({"quota_bytes":0})");
    auto reset_res = handler_->handleUpdateTenantQuota(reset_req);
    EXPECT_EQ(reset_res.result(), http::status::ok);
    json reset_body = json::parse(reset_res.body());
    EXPECT_EQ(reset_body["quota_bytes"].get<size_t>(), 0ULL);

    // Stats should now show the global default quota
    auto stats_req = makeRequest(http::verb::get,
                                 "/v1/admin/cache/tenant/" + tenant + "/stats");
    auto stats_res = handler_->handleTenantStats(stats_req);
    EXPECT_EQ(stats_res.result(), http::status::ok);
    json body = json::parse(stats_res.body());
    // quota should be the global default (per_tenant_max_bytes = 104857600 default)
    EXPECT_GT(body["quota"].get<size_t>(), 0ULL);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns400ForMissingField) {
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant/acme/quota",
                           "{}");
    auto res = handler_->handleUpdateTenantQuota(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns400ForMissingTenantId) {
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant//quota",
                           R"({"quota_bytes":1024})");
    auto res = handler_->handleUpdateTenantQuota(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns400ForInvalidTenantId) {
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant/../bad/quota",
                           R"({"quota_bytes":1024})");
    auto res = handler_->handleUpdateTenantQuota(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns400ForTrailingPathAfterQuota) {
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant/acme/quota/extra",
                           R"({"quota_bytes":1024})");
    auto res = handler_->handleUpdateTenantQuota(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaReturns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);
    auto req = makeRequest(http::verb::patch,
                           "/v1/admin/cache/tenant/acme/quota",
                           R"({"quota_bytes":1024})");
    EXPECT_EQ(handler_no_cache->handleUpdateTenantQuota(req).result(),
              http::status::service_unavailable);
}

TEST_F(CacheAdminApiHandlerTest, UpdateTenantQuotaAppearsInListTenants) {
    const std::string tenant = "list_quota_tenant";
    const size_t new_quota = 20971520;  // 20 MB

    auto patch_req = makeRequest(http::verb::patch,
                                 "/v1/admin/cache/tenant/" + tenant + "/quota",
                                 R"({"quota_bytes":)" + std::to_string(new_quota) + "}");
    EXPECT_EQ(handler_->handleUpdateTenantQuota(patch_req).result(), http::status::ok);

    auto list_req = makeRequest(http::verb::get, "/v1/admin/cache/tenants");
    auto list_res = handler_->handleListTenants(list_req);
    EXPECT_EQ(list_res.result(), http::status::ok);

    json body = json::parse(list_res.body());
    ASSERT_TRUE(body.contains("tenants"));
    ASSERT_TRUE(body["tenants"].contains(tenant));
    EXPECT_EQ(body["tenants"][tenant]["quota"].get<size_t>(), new_quota);
}

// ---------------------------------------------------------------------------
// Tests: DELETE /v1/admin/cache/pii/{pii_uuid}
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, PiiEvictReturns400ForMissingUuid) {
    // Path ends exactly at the prefix with no UUID segment
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/pii/");
    auto res = handler_->handlePiiEvict(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
    json body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
}

TEST_F(CacheAdminApiHandlerTest, PiiEvictReturns400ForInvalidUuid) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/pii/../bad");
    auto res = handler_->handlePiiEvict(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, PiiEvictReturns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);
    auto req = makeRequest(http::verb::delete_,
                           "/v1/admin/cache/pii/some-pii-uuid");
    EXPECT_EQ(handler_no_cache->handlePiiEvict(req).result(),
              http::status::service_unavailable);
}

TEST_F(CacheAdminApiHandlerTest, PiiEvictReturns200WithZeroEvictedForUnknownUuid) {
    // A UUID that was never registered returns 0 evicted entries (not an error).
    auto req = makeRequest(http::verb::delete_,
                           "/v1/admin/cache/pii/unknown-pii-uuid-99999");
    auto res = handler_->handlePiiEvict(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["pii_uuid"], "unknown-pii-uuid-99999");
    EXPECT_GE(body["evicted"].get<int>(), 0);
}

TEST_F(CacheAdminApiHandlerTest, PiiEvictReturnsCorrectUuidInResponse) {
    const std::string pii_uuid = "test-pii-uuid-abc123";
    auto req = makeRequest(http::verb::delete_,
                           "/v1/admin/cache/pii/" + pii_uuid);
    auto res = handler_->handlePiiEvict(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["pii_uuid"].get<std::string>(), pii_uuid);
    EXPECT_TRUE(body.contains("evicted"));
}

TEST_F(CacheAdminApiHandlerTest, PiiEvictPurgesTaggedCacheEntries) {
    // Insert a cache entry tagged with a PII UUID, then verify it is purged
    // via the admin API endpoint.
    const std::string pii_uuid = "pii-evict-test-uuid";
    const std::string tenant   = "acme";

    std::string fp = cache_->generateFingerprint("SELECT pii", {}, tenant);
    bool ok = cache_->put(fp, {}, json::object(), tenant, {pii_uuid});
    ASSERT_TRUE(ok) << "put() with pii_uuids must succeed";
    ASSERT_TRUE(cache_->get(fp, tenant).has_value()) << "entry must be retrievable";

    auto req = makeRequest(http::verb::delete_,
                           "/v1/admin/cache/pii/" + pii_uuid);
    auto res = handler_->handlePiiEvict(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_GE(body["evicted"].get<int>(), 1)
        << "at least one tagged entry must have been purged";
    EXPECT_FALSE(cache_->get(fp, tenant).has_value())
        << "entry must no longer be in cache after PII eviction";
}
