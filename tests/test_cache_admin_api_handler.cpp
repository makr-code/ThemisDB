#include <gtest/gtest.h>
#include "server/cache_admin_api_handler.h"
#include "cache/adaptive_query_cache.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>

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
    cache_->put(fp, {}, json::object(), "acme");

    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/acme");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["tenant_id"], "acme");
    EXPECT_GE(body["evicted"].get<int>(), 0);
}

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns400ForMissingTenantId) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Tests: null cache guard
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, Returns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_no_cache->handleStats(req);
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}
