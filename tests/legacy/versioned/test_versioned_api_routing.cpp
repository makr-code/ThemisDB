/**
 * @file test_versioned_api_routing.cpp
 * @brief Focused tests for Versioned API Routing and /v2/ Prefix (v1.8.0)
 *
 * Acceptance criteria covered:
 *   AC-VAR-1  RouteVersionRouter: unversioned paths return redirect target to /v1/
 *   AC-VAR-2  RouteVersionRouter: already-versioned paths are not redirected
 *   AC-VAR-3  RouteVersionRouter: exempt paths (health, metrics, graphql) never redirect
 *   AC-VAR-4  RouteVersionRouter: normalize() strips /v1/ and /v2/ top-level prefixes
 *   AC-VAR-5  POST /v2/documents accepts application/x-ndjson; max 10,000 docs
 *   AC-VAR-6  POST /v2/documents rejects wrong Content-Type with 415
 *   AC-VAR-7  POST /v2/documents returns inserted count in response JSON
 *   AC-VAR-8  POST /v2/documents auto-generates keys when _key field absent
 *   AC-VAR-9  POST /v2/documents reports per-line errors for malformed JSON
 *   AC-VAR-10 AsyncJobRegistry stores job state with default TTL of 1 hour
 *   AC-VAR-11 AsyncJobApiHandler: POST /v2/jobs returns 202 with job_id
 *   AC-VAR-12 AsyncJobApiHandler: GET /v2/jobs/{id} returns job status
 *   AC-VAR-13 AsyncJobApiHandler: DELETE /v2/jobs/{id} cancels a job
 *   AC-VAR-14 GET /v2/query/stream returns 400 when 'q' parameter is missing
 *   AC-VAR-15 GET /v2/query/stream returns text/event-stream Content-Type
 *   AC-VAR-16 Bulk insert of 10,000 × 256-byte documents completes in < 500 ms
 *   AC-VAR-17 SSE first-byte latency < 5 ms after query planning completes
 *   AC-VAR-18 AsyncJobApiHandler persists completed job state in AdaptiveQueryCache
 */

#include <gtest/gtest.h>

// Headers for RouteVersionRouter (header-only)
#include "server/route_version_router.h"

// Headers for EntityApiHandler (bulk NDJSON)
#include "server/entity_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "server/auth_middleware.h"

// Headers for QueryApiHandler (SSE)
#include "server/query_api_handler.h"

// Headers for AsyncJobApiHandler
#include "server/async_job_api_handler.h"
#include "cache/adaptive_query_cache.h"

#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <condition_variable>
#include <iomanip>
#include <string>
#include <thread>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

using namespace themis::server;
using themis::AdaptiveQueryCache;
using json = nlohmann::json;
namespace http = boost::beast::http;

// ============================================================================
// AC-VAR-1 … AC-VAR-4 : RouteVersionRouter
// ============================================================================

TEST(RouteVersionRouterAC, UnversionedPath_ReturnsRedirectToV1) {
    RouteVersionRouter vr;
    auto target = vr.getRedirectTarget("/documents/abc");
    ASSERT_TRUE(target.has_value());
    EXPECT_EQ(*target, "/v1/documents/abc");
}

TEST(RouteVersionRouterAC, UnversionedPathWithQuery_RedirectPreservesQuery) {
    RouteVersionRouter vr;
    auto target = vr.getRedirectTarget("/documents/abc?limit=5");
    ASSERT_TRUE(target.has_value());
    EXPECT_EQ(*target, "/v1/documents/abc?limit=5");
}

TEST(RouteVersionRouterAC, AlreadyVersionedV1_NoRedirect) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/v1/documents/abc").has_value());
}

TEST(RouteVersionRouterAC, AlreadyVersionedV2_NoRedirect) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/v2/documents").has_value());
    EXPECT_FALSE(vr.getRedirectTarget("/v2/query/stream").has_value());
    EXPECT_FALSE(vr.getRedirectTarget("/v2/jobs/job-123").has_value());
}

TEST(RouteVersionRouterAC, HealthEndpoint_NoRedirect) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/health").has_value());
    EXPECT_FALSE(vr.getRedirectTarget("/metrics").has_value());
    EXPECT_FALSE(vr.getRedirectTarget("/ready").has_value());
}

TEST(RouteVersionRouterAC, GraphQLWebSocket_NoRedirect) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/graphql").has_value());
}

TEST(RouteVersionRouterAC, RootPath_NoRedirect) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/").has_value());
}

TEST(RouteVersionRouterAC, Normalize_V1Path) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/v1/documents/abc");
    EXPECT_EQ(n.version, 1);
    EXPECT_EQ(n.path,    "/documents/abc");
}

TEST(RouteVersionRouterAC, Normalize_V2Path) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/v2/query/stream");
    EXPECT_EQ(n.version, 2);
    EXPECT_EQ(n.path,    "/query/stream");
}

TEST(RouteVersionRouterAC, Normalize_UnversionedPath) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/documents/abc");
    EXPECT_EQ(n.version, 0);
    EXPECT_EQ(n.path,    "/documents/abc");
}

TEST(RouteVersionRouterAC, ExtractVersion_V1) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/v1/documents"), 1);
}

TEST(RouteVersionRouterAC, ExtractVersion_V2) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/v2/query/stream"), 2);
}

TEST(RouteVersionRouterAC, ExtractVersion_Unversioned) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/documents"), 0);
}

TEST(RouteVersionRouterAC, StripPrefix_V1) {
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/v1/documents/abc"),
              "/documents/abc");
}

TEST(RouteVersionRouterAC, StripPrefix_V2) {
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/v2/query/stream"),
              "/query/stream");
}

// ============================================================================
// AC-VAR-5 … AC-VAR-9 : EntityApiHandler::handleBulkNdjson()
// ============================================================================

class BulkNdjsonTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = std::filesystem::temp_directory_path() /
            ("themis_bulk_ndjson_" + std::to_string(std::time(nullptr)) +
             "_" + std::to_string(getpid()));
        std::filesystem::create_directories(test_db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_.string();
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_     = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_    = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_      = std::make_shared<themis::TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_);

        key_provider_     = std::make_shared<themis::MockKeyProvider>();
        field_encryption_ = std::make_shared<themis::FieldEncryption>(key_provider_);
        auth_             = std::make_shared<themis::AuthMiddleware>();
    }

    void TearDown() override {
        tx_manager_.reset();
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        storage_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    EntityApiHandler makeHandler() {
        themis::server::EntityApiConfig cfg;
        return EntityApiHandler(
            storage_, secondary_index_, graph_index_, tx_manager_,
            field_encryption_, key_provider_, auth_, cfg);
    }

    /// Build a POST /v2/documents request with the given NDJSON body.
    static http::request<http::string_body>
    makeNdjsonRequest(const std::string& body,
                      const std::string& content_type = "application/x-ndjson")
    {
        http::request<http::string_body> req{http::verb::post, "/v2/documents", 11};
        req.set(http::field::content_type,  content_type);
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    std::filesystem::path                        test_db_path_;
    std::shared_ptr<themis::RocksDBWrapper>      storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager>   graph_index_;
    std::shared_ptr<themis::VectorIndexManager>  vector_index_;
    std::shared_ptr<themis::TransactionManager>  tx_manager_;
    std::shared_ptr<themis::FieldEncryption>     field_encryption_;
    std::shared_ptr<themis::KeyProvider>         key_provider_;
    std::shared_ptr<themis::AuthMiddleware>      auth_;
};

TEST_F(BulkNdjsonTest, AC5_AcceptsNdjsonContentType) {
    auto handler = makeHandler();
    std::string body = R"({"_key":"doc1","val":1})" "\n"
                       R"({"_key":"doc2","val":2})" "\n";
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(body));
    EXPECT_NE(resp.result(), http::status::unsupported_media_type);
    EXPECT_NE(resp.result(), http::status::bad_request);
    auto j = json::parse(resp.body());
    EXPECT_GE(j["inserted"].get<int64_t>(), 0);
}

TEST_F(BulkNdjsonTest, AC6_WrongContentType_Returns415) {
    auto handler = makeHandler();
    auto resp = handler.handleBulkNdjson(
        makeNdjsonRequest(R"({"_key":"x"})", "application/json"));
    EXPECT_EQ(resp.result(), http::status::unsupported_media_type);
}

TEST_F(BulkNdjsonTest, AC7_InsertedCountMatchesValidDocuments) {
    auto handler = makeHandler();
    std::string body = R"({"_key":"k1","value":"a"})" "\n"
                       R"({"_key":"k2","value":"b"})" "\n"
                       R"({"_key":"k3","value":"c"})" "\n";
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(body));
    auto j = json::parse(resp.body());
    EXPECT_EQ(j["inserted"].get<int64_t>(), 3);
    EXPECT_EQ(j["error_count"].get<int64_t>(), 0);
}

TEST_F(BulkNdjsonTest, AC8_AutoGeneratesKeyWhenAbsent) {
    auto handler = makeHandler();
    // Document has no _key or key field → auto-key generated
    std::string body = R"({"value":"no-key-field"})" "\n";
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(body));
    EXPECT_NE(resp.result(), http::status::bad_request);
    auto j = json::parse(resp.body());
    EXPECT_EQ(j["inserted"].get<int64_t>(), 1);
}

TEST_F(BulkNdjsonTest, AC9_MalformedLineReportedAsError) {
    auto handler = makeHandler();
    std::string body = R"({"_key":"good","val":1})" "\n"
                       "not-valid-json\n"
                       R"({"_key":"also-good","val":2})" "\n";
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(body));
    auto j = json::parse(resp.body());
    EXPECT_EQ(j["inserted"].get<int64_t>(), 2);
    EXPECT_GE(j["error_count"].get<int64_t>(), 1);
    ASSERT_TRUE(j.contains("errors"));
    EXPECT_FALSE(j["errors"].empty());
}

TEST_F(BulkNdjsonTest, AC5_EmptyBody_Returns400) {
    auto handler = makeHandler();
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(""));
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(BulkNdjsonTest, AC5_MaxDocsLimit_10000) {
    // Inserting exactly 10,000 docs should succeed; 10,001 must fail
    auto handler = makeHandler();

    // Build 10,001 lines
    std::ostringstream oss = {};
    for (int i = 0; i <= 10000; ++i) {
        oss << R"({"_key":"bulk)" << i << R"(","v":)" << i << "}\n";
    }
    auto resp = handler.handleBulkNdjson(makeNdjsonRequest(oss.str()));
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

// ============================================================================
// AC-VAR-10 : AsyncJobRegistry default TTL = 1 hour
// ============================================================================

TEST(AsyncJobRegistryAC, DefaultTTLIsOneHour) {
    EXPECT_EQ(AsyncJobRegistry::kDefaultTTL, std::chrono::seconds(3600));
}

TEST(AsyncJobRegistryAC, ExpiredJobsPruned) {
    // Registry with 0-second TTL; terminal jobs expire immediately.
    AsyncJobRegistry reg{std::chrono::seconds(0)};

    auto job        = std::make_shared<AsyncJobRecord>();
    job->id         = "old-job";
    job->status     = AsyncJobStatus::COMPLETED;
    job->created_at = std::chrono::system_clock::now() - std::chrono::seconds(2);
    job->updated_at = job->created_at;

    // Add a second job to trigger prune inside add()
    auto trigger    = std::make_shared<AsyncJobRecord>();
    trigger->id     = "new-job";
    trigger->status = AsyncJobStatus::PENDING;
    trigger->created_at = trigger->updated_at = std::chrono::system_clock::now();

    reg.add(job);
    reg.add(trigger);   // triggers prune

    EXPECT_EQ(reg.get("old-job"), nullptr);
    EXPECT_NE(reg.get("new-job"), nullptr);
}

TEST(AsyncJobRegistryAC, NonExpiredJobSurvivesPrune) {
    // TTL = 1 h — a fresh job should still be present after prune.
    AsyncJobRegistry reg{std::chrono::seconds(3600)};

    auto job    = std::make_shared<AsyncJobRecord>();
    job->id     = "live-job";
    job->status = AsyncJobStatus::COMPLETED;
    job->created_at = job->updated_at = std::chrono::system_clock::now();
    reg.add(job);

    reg.prune();
    EXPECT_NE(reg.get("live-job"), nullptr);
}

// ============================================================================
// AC-VAR-11 … AC-VAR-13 : AsyncJobApiHandler
// ============================================================================

/// Synchronous executor: returns a static JSON result immediately.
static nlohmann::json syncExecutor(const std::string& /*query*/,
                                   const std::string& /*auth*/) {
    return json::array({json{{"_key", "r1"}, {"val", 42}}});
}

/// Failing executor: always throws.
static nlohmann::json failingExecutor(const std::string& /*query*/,
                                      const std::string& /*auth*/) {
    throw std::runtime_error("simulated AQL error");
}

static http::request<http::string_body>
makeSubmitReq(const json& body)
{
    http::request<http::string_body> req{http::verb::post, "/v2/jobs", 11};
    req.set(http::field::content_type,  "application/json");
    req.set(http::field::authorization, "Bearer test-token");
    req.body() = body.dump();
    req.prepare_payload();
    return req;
}

static http::request<http::string_body>
makeStatusReq(const std::string& job_id)
{
    http::request<http::string_body> req{http::verb::get, "/v2/jobs/" + job_id, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

static http::request<http::string_body>
makeCancelReq(const std::string& job_id)
{
    http::request<http::string_body> req{http::verb::delete_, "/v2/jobs/" + job_id, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

TEST(AsyncJobApiHandlerAC, AC11_SubmitReturns202WithJobId) {
    AsyncJobApiHandler handler{syncExecutor};
    auto resp = handler.handleSubmit(makeSubmitReq({{"query", "FOR x IN col RETURN x"}}));
    EXPECT_EQ(resp.result(), http::status::accepted);
    auto j = json::parse(resp.body());
    ASSERT_TRUE(j.contains("job_id"));
    EXPECT_FALSE(j["job_id"].get<std::string>().empty());
    EXPECT_EQ(j["status"].get<std::string>(), "pending");
}

TEST(AsyncJobApiHandlerAC, AC11_MissingQuery_Returns400) {
    AsyncJobApiHandler handler{syncExecutor};
    auto resp = handler.handleSubmit(makeSubmitReq(json::object()));
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST(AsyncJobApiHandlerAC, AC12_GetStatus_EventuallyCompleted) {
    AsyncJobApiHandler handler{syncExecutor};
    auto submit_resp = handler.handleSubmit(
        makeSubmitReq({{"query", "FOR x IN col RETURN x"}}));
    auto submit_j = json::parse(submit_resp.body());
    std::string job_id = submit_j["job_id"].get<std::string>();

    // Poll until completed (max 2 s)
    json status_j;
    for (int i = 0; i < 20; ++i) {
        auto status_resp = handler.handleGetStatus(makeStatusReq(job_id));
        status_j = json::parse(status_resp.body());
        if (status_j["status"].get<std::string>() == "completed") {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_EQ(status_j["status"].get<std::string>(), "completed");
}

TEST(AsyncJobApiHandlerAC, AC12_GetStatus_UnknownId_Returns404) {
    AsyncJobApiHandler handler{syncExecutor};
    auto resp = handler.handleGetStatus(makeStatusReq("no-such-job"));
    EXPECT_EQ(resp.result(), http::status::not_found);
}

TEST(AsyncJobApiHandlerAC, AC13_CancelPendingJob) {
    // Use an executor that blocks on a latch until cancellation is requested,
    // avoiding a fixed multi-second sleep that would slow the test suite.
    std::atomic<bool> executor_running{false};
    std::mutex latch_mu = {};
    std::condition_variable latch_cv;
    bool released = false;

    auto slow_executor = [&](const std::string&, const std::string&) -> json {
        executor_running.store(true, std::memory_order_release);
        std::unique_lock<std::mutex> lk(latch_mu);
        // Block until the test unblocks us.  The 5 s safety timeout prevents
        // the test from hanging forever if the notify is never sent (e.g. after
        // an unexpected early assertion failure).
        constexpr auto kSafetyTimeout = std::chrono::seconds(5);
        latch_cv.wait_for(lk, kSafetyTimeout, [&]{ return released; });
        return json::array();
    };
    AsyncJobApiHandler handler{slow_executor};
    auto submit_resp = handler.handleSubmit(
        makeSubmitReq({{"query", "FOR x IN col RETURN x"}}));
    auto j = json::parse(submit_resp.body());
    std::string job_id = j["job_id"].get<std::string>();

    // Wait up to 500 ms (50 × 10 ms) for the executor to start so the job is
    // in RUNNING state before we issue the cancel request.
    constexpr int kMaxPollIter  = 50;
    constexpr int kPollIntervalMs = 10;
    for (int i = 0; i < kMaxPollIter && !executor_running.load(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));

    auto cancel_resp = handler.handleCancel(makeCancelReq(job_id));
    EXPECT_EQ(cancel_resp.result(), http::status::ok);
    auto cancel_j = json::parse(cancel_resp.body());
    // Status should be cancelled or still transitioning.
    std::string s = cancel_j["status"].get<std::string>();
    EXPECT_TRUE(s == "cancelled" || s == "pending" || s == "running");

    // Unblock the executor so the handler destructor can join cleanly.
    { std::lock_guard<std::mutex> lk(latch_mu); released = true; }
    latch_cv.notify_all();
}

TEST(AsyncJobApiHandlerAC, AC13_CancelNonExistentJob_Returns404) {
    AsyncJobApiHandler handler{syncExecutor};
    auto resp = handler.handleCancel(makeCancelReq("ghost-job"));
    EXPECT_EQ(resp.result(), http::status::not_found);
}

// ============================================================================
// AC-VAR-14 … AC-VAR-15 : QueryApiHandler::handleQueryStreamSse()
//
// These tests use a minimal QueryApiHandler constructed with null optional
// dependencies so that the request validation logic at the top of the handler
// (missing 'q' param, wrong content-type, etc.) can be exercised without a
// live storage backend.
// ============================================================================

static http::request<http::string_body>
makeSseRequest(const std::string& target)
{
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

class QueryStreamSseAcTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = std::filesystem::temp_directory_path() /
            ("themis_sse_ac_" + std::to_string(std::time(nullptr)) +
             "_" + std::to_string(getpid()));
        std::filesystem::create_directories(test_db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_.string();
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_     = std::make_shared<themis::GraphIndexManager>(*storage_);

        key_provider_     = std::make_shared<themis::MockKeyProvider>();
        field_encryption_ = std::make_shared<themis::FieldEncryption>(key_provider_);
        auth_             = std::make_shared<themis::AuthMiddleware>();

        handler_ = std::make_unique<QueryApiHandler>(
            storage_,
            secondary_index_,
            graph_index_,
            field_encryption_,
            key_provider_,
            /*semantic_cache=*/nullptr,
            /*llm_store=*/nullptr,
            /*prompt_manager=*/nullptr,
            auth_,
            /*feature_llm_query_enhancement=*/false,
            /*feature_llm_store=*/false);
    }

    void TearDown() override {
        handler_.reset();
        secondary_index_.reset();
        graph_index_.reset();
        storage_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    std::filesystem::path                        test_db_path_;
    std::shared_ptr<themis::RocksDBWrapper>      storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager>   graph_index_;
    std::shared_ptr<themis::FieldEncryption>     field_encryption_;
    std::shared_ptr<themis::KeyProvider>         key_provider_;
    std::shared_ptr<themis::AuthMiddleware>      auth_;
    std::unique_ptr<QueryApiHandler>             handler_;
};

TEST_F(QueryStreamSseAcTest, AC14_MissingQParam_Returns400) {
    auto resp = handler_->handleQueryStreamSse(makeSseRequest("/v2/query/stream"));
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(QueryStreamSseAcTest, AC15_WithQueryParam_ReturnsEventStreamContentType) {
    // Use a trivially valid AQL that won't match any stored documents
    auto resp = handler_->handleQueryStreamSse(
        makeSseRequest("/v2/query/stream?q=FOR+x+IN+nothing+RETURN+x"));
    // Regardless of whether results are found, the Content-Type must be SSE
    auto ct = std::string(resp[http::field::content_type]);
    EXPECT_NE(ct.find("text/event-stream"), std::string::npos);
}

TEST_F(QueryStreamSseAcTest, AC15_SseResponseContainsDoneEvent) {
    auto resp = handler_->handleQueryStreamSse(
        makeSseRequest("/v2/query/stream?q=FOR+x+IN+nothing+RETURN+x"));
    EXPECT_NE(resp.body().find("event: done"), std::string::npos);
}

TEST_F(QueryStreamSseAcTest, QueryReturnCount_UsesCountMode) {
    ASSERT_TRUE(secondary_index_->createIndex("orders_count", "status").ok);

    for (int i = 0; i < 5; ++i) {
        const std::string pk = "ord" + std::to_string(i);
        const std::string status = (i < 3) ? "open" : "closed";
        auto ent = themis::BaseEntity::fromFields(pk, {{"status", status}});
        ASSERT_TRUE(secondary_index_->put("orders_count", ent).ok);
    }

    http::request<http::string_body> req{http::verb::post, "/query", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{
        {"table", "orders_count"},
        {"predicates", json::array({{{"column", "status"}, {"value", "open"}}})},
        {"return", "count"},
        {"allow_full_scan", false},
        {"optimize", false}
    }.dump();
    req.prepare_payload();

    auto resp = handler_->handleQuery(req);
    ASSERT_EQ(resp.result(), http::status::ok) << resp.body();

    auto body = json::parse(resp.body());
    ASSERT_TRUE(body.contains("count"));
    EXPECT_EQ(body["count"].get<size_t>(), 3u);
    EXPECT_FALSE(body.contains("keys"));
    EXPECT_FALSE(body.contains("entities"));
}

TEST_F(QueryStreamSseAcTest, QueryReturnCount_OptimizeTrue_UsesOptimizedPlan) {
    ASSERT_TRUE(secondary_index_->createIndex("orders_count_opt", "status").ok);

    for (int i = 0; i < 6; ++i) {
        const std::string pk = "ordopt" + std::to_string(i);
        const std::string status = (i < 4) ? "open" : "closed";
        auto ent = themis::BaseEntity::fromFields(pk, {{"status", status}});
        ASSERT_TRUE(secondary_index_->put("orders_count_opt", ent).ok);
    }

    http::request<http::string_body> req{http::verb::post, "/query", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{
        {"table", "orders_count_opt"},
        {"predicates", json::array({{{"column", "status"}, {"value", "open"}}})},
        {"return", "count"},
        {"allow_full_scan", false},
        {"optimize", true},
        {"explain", true}
    }.dump();
    req.prepare_payload();

    auto resp = handler_->handleQuery(req);
    ASSERT_EQ(resp.result(), http::status::ok) << resp.body();

    auto body = json::parse(resp.body());
    ASSERT_TRUE(body.contains("count"));
    EXPECT_EQ(body["count"].get<size_t>(), 4u);
    ASSERT_TRUE(body.contains("plan"));
    ASSERT_TRUE(body["plan"].contains("mode"));
    EXPECT_EQ(body["plan"]["mode"].get<std::string>(), "index_optimized");
}

TEST_F(QueryStreamSseAcTest, QueryReturnCount_FullScanFallback_WorksWithoutIndex) {
    for (int i = 0; i < 5; ++i) {
        const std::string pk = "ordfs" + std::to_string(i);
        const std::string status = (i < 2) ? "open" : "closed";
        auto ent = themis::BaseEntity::fromFields(pk, {{"status", status}});
        ASSERT_TRUE(secondary_index_->put("orders_fullscan", ent).ok);
    }

    http::request<http::string_body> req{http::verb::post, "/query", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{
        {"table", "orders_fullscan"},
        {"predicates", json::array({{{"column", "status"}, {"value", "open"}}})},
        {"return", "count"},
        {"allow_full_scan", true},
        {"optimize", true}
    }.dump();
    req.prepare_payload();

    auto resp = handler_->handleQuery(req);
    ASSERT_EQ(resp.result(), http::status::ok) << resp.body();

    auto body = json::parse(resp.body());
    ASSERT_TRUE(body.contains("count"));
    EXPECT_EQ(body["count"].get<size_t>(), 2u);
    EXPECT_FALSE(body.contains("keys"));
    EXPECT_FALSE(body.contains("entities"));
}

TEST(QueryApiHandlerSafety, HandleQuery_MissingCoreDependencies_Returns503) {
    QueryApiHandler handler(
        /*storage=*/nullptr,
        /*secondary_index=*/nullptr,
        /*graph_index=*/nullptr,
        /*field_encryption=*/nullptr,
        /*key_provider=*/nullptr,
        /*semantic_cache=*/nullptr,
        /*llm_store=*/nullptr,
        /*prompt_manager=*/nullptr,
        /*auth=*/nullptr,
        /*feature_llm_query_enhancement=*/false,
        /*feature_llm_store=*/false);

    http::request<http::string_body> req{http::verb::post, "/query", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{{"table", "users"}}.dump();
    req.prepare_payload();

    auto resp = handler.handleQuery(req);
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
}

TEST(QueryApiHandlerSafety, HandleQueryAql_MissingCoreDependencies_Returns503) {
    QueryApiHandler handler(
        /*storage=*/nullptr,
        /*secondary_index=*/nullptr,
        /*graph_index=*/nullptr,
        /*field_encryption=*/nullptr,
        /*key_provider=*/nullptr,
        /*semantic_cache=*/nullptr,
        /*llm_store=*/nullptr,
        /*prompt_manager=*/nullptr,
        /*auth=*/nullptr,
        /*feature_llm_query_enhancement=*/false,
        /*feature_llm_store=*/false);

    http::request<http::string_body> req{http::verb::post, "/query/aql", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{{"query", "FOR x IN users RETURN x"}}.dump();
    req.prepare_payload();

    auto resp = handler.handleQueryAql(req);
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
}

TEST(QueryApiHandlerSafety, HandleQueryEnhanced_MissingLlmStoreReturns503) {
    QueryApiHandler handler(
        /*storage=*/nullptr,
        /*secondary_index=*/nullptr,
        /*graph_index=*/nullptr,
        /*field_encryption=*/nullptr,
        /*key_provider=*/nullptr,
        /*semantic_cache=*/nullptr,
        /*llm_store=*/nullptr,
        /*prompt_manager=*/nullptr,
        /*auth=*/nullptr,
        /*feature_llm_query_enhancement=*/true,
        /*feature_llm_store=*/true);

    http::request<http::string_body> req{http::verb::post, "/query/enhanced", 11};
    req.set(http::field::content_type, "application/json");
    req.body() = json{{"aql", "FOR x IN users RETURN x"}}.dump();
    req.prepare_payload();

    auto resp = handler.handleQueryEnhanced(req);
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
}

// ============================================================================
// AC-VAR-16  Bulk insert performance: 10,000 × 256-byte docs in < 500 ms
// ============================================================================

TEST_F(BulkNdjsonTest, AC16_BulkInsert10k_Under500ms) {
    // Guard behind THEMIS_RUN_PERF_TESTS=1 to avoid flakiness on shared/slow CI runners.
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping bulk-insert perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-VAR-16: 10,000 × 256-byte docs in < 500 ms.";
    }

    auto handler = makeHandler();

    // Build 10,000 lines; each value padded to reach ~256 bytes total.
    // A line like {"_key":"bulk0000","value":"<240 chars>"}\n is ~265 bytes.
    const std::string pad(224, 'x');  // padding to reach ~256-byte per document
    std::ostringstream oss = {};
    for (int i = 0; i < 10000; ++i) {
        oss << "{\"_key\":\"perf" << std::setw(5) << std::setfill('0') << i
            << "\",\"value\":\"" << pad << "\"}\n";
    }

    auto req = makeNdjsonRequest(oss.str());

    auto t0   = std::chrono::steady_clock::now();
    auto resp = handler.handleBulkNdjson(req);
    auto t1   = std::chrono::steady_clock::now();

    ASSERT_NE(resp.result(), http::status::bad_request) << resp.body();
    auto j = json::parse(resp.body());
    EXPECT_EQ(j["inserted"].get<int64_t>(), 10000);

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, 500)
        << "Bulk insert of 10,000 docs took " << elapsed_ms << " ms (limit: 500 ms)";
}

// ============================================================================
// AC-VAR-17  SSE first-byte latency: < 5 ms after query planning
//
// "First-byte latency" is measured as the elapsed time from when
// handleQueryStreamSse() is called to when the response body (with the initial
// `retry:` preamble) is available.  For an empty result set the handler returns
// immediately after planning; the limit is 5 ms.
// ============================================================================

TEST_F(QueryStreamSseAcTest, AC17_SseFirstByteLatency_Under5ms) {
    // Guard behind THEMIS_RUN_PERF_TESTS=1 to avoid flakiness due to
    // host scheduling jitter on shared/slow CI runners.
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping SSE latency perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-VAR-17: SSE first-byte latency < 5 ms.";
    }

    // Warm up the handler once so any first-call initialisation overhead
    // does not pollute the timed measurement.
    handler_->handleQueryStreamSse(
        makeSseRequest("/v2/query/stream?q=FOR+x+IN+nothing+RETURN+x"));

    auto t0   = std::chrono::steady_clock::now();
    auto resp = handler_->handleQueryStreamSse(
        makeSseRequest("/v2/query/stream?q=FOR+x+IN+nothing+RETURN+x"));
    auto t1   = std::chrono::steady_clock::now();

    // The response must be valid SSE.
    auto ct = std::string(resp[http::field::content_type]);
    EXPECT_NE(ct.find("text/event-stream"), std::string::npos);

    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_us, 5000)
        << "SSE first-byte latency was " << elapsed_us << " µs (limit: 5000 µs / 5 ms)";
}

// ============================================================================
// AC-VAR-18  AsyncJobApiHandler persists completed job state in AdaptiveQueryCache
//
// The handler creates an internal AdaptiveQueryCache (TTL=1h) when none is
// supplied.  After a job reaches COMPLETED state the cache must contain a
// cached entry whose `result` JSON includes the "job_id" field so that the
// entry can be identified as belonging to that job.
// ============================================================================

TEST(AsyncJobApiHandlerAC, AC18_CompletedJobPersistedInAdaptiveCache) {
    // Provide a dedicated cache so we can inspect it after the job finishes.
    AdaptiveQueryCache::Config cfg;
    cfg.l1_ttl_seconds = 3600;
    cfg.l2_ttl_seconds = 3600;
    cfg.l3_ttl_seconds = 3600;
    auto result_cache = std::make_shared<AdaptiveQueryCache>(cfg);

    AsyncJobApiHandler handler{syncExecutor, nullptr, nullptr, result_cache};

    // Submit a job and collect its ID.
    auto submit_resp = handler.handleSubmit(
        makeSubmitReq({{"query", "FOR x IN col RETURN x"}}));
    ASSERT_EQ(submit_resp.result(), http::status::accepted);
    auto submit_j = json::parse(submit_resp.body());
    std::string job_id = submit_j["job_id"].get<std::string>();

    // Poll until the job is completed (max 2 s).
    for (int i = 0; i < 20; ++i) {
        auto status_resp = handler.handleGetStatus(makeStatusReq(job_id));
        auto sj = json::parse(status_resp.body());
        if (sj["status"].get<std::string>() == "completed") {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // The cache must contain an entry keyed by the job ID.
    auto entry = result_cache->get(job_id, "async_jobs");
    ASSERT_TRUE(entry.has_value())
        << "AdaptiveQueryCache should contain the completed job entry for " << job_id;
    EXPECT_TRUE(entry->result.contains("job_id"))
        << "Cached entry result must include 'job_id'";
    EXPECT_EQ(entry->result["job_id"].get<std::string>(), job_id);
    EXPECT_EQ(entry->result["status"].get<std::string>(), "completed");
}
