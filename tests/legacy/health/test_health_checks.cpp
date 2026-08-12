/**
 * @file test_health_checks.cpp
 * @brief Tests for health, liveness and readiness check endpoints
 *
 * Validates the MonitoringApiHandler liveness and readiness probes introduced
 * as part of the Server Production Readiness initiative.
 */

#include <gtest/gtest.h>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>

#include "performance/phase3/bao.h"
#include "performance/workload_adaptive_optimizer.h"
#include "prompt_engineering/feedback_collector.h"
#include "rag/continuous_learning_orchestrator.h"
#include "server/monitoring_api_handler.h"
#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/lifecycle.h"
#include "storage/rocksdb_wrapper.h"

namespace http = boost::beast::http;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> make_get(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "localhost");
    req.prepare_payload();
    return req;
}

// Minimal MonitoringApiHandler factory that requires no storage
static std::unique_ptr<themis::server::MonitoringApiHandler> make_handler(
    const std::atomic<bool>* is_running = nullptr,
    const std::atomic<uint64_t>* active_requests = nullptr
) {
    static std::atomic<uint64_t> req_count{0};
    static std::atomic<uint64_t> err_count{0};
    static auto start = std::chrono::steady_clock::now();

    return std::make_unique<themis::server::MonitoringApiHandler>(
        nullptr,  // no storage
        nullptr,  // no auth
        &req_count,
        &err_count,
        &start,
        nullptr,  // no secondary index
        nullptr,  // no schema manager
        nullptr,  // no sharding metrics
        is_running,
        active_requests
    );
}

// Factory that injects a ConcernsContext
static std::unique_ptr<themis::server::MonitoringApiHandler> make_handler_with_concerns(
    std::shared_ptr<themis::core::concerns::ConcernsContext> concerns,
    const std::atomic<bool>* is_running = nullptr
) {
    static std::atomic<uint64_t> req_count{0};
    static std::atomic<uint64_t> err_count{0};
    static auto start = std::chrono::steady_clock::now();

    return std::make_unique<themis::server::MonitoringApiHandler>(
        nullptr,  // no storage
        nullptr,  // no auth
        &req_count,
        &err_count,
        &start,
        nullptr,  // no secondary index
        nullptr,  // no schema manager
        nullptr,  // no sharding metrics
        is_running,
        nullptr,  // no active_requests
        nullptr,  // no active_connections
        std::move(concerns)
    );
}

// ---------------------------------------------------------------------------
// Liveness Tests
// ---------------------------------------------------------------------------

class LivenessTest : public ::testing::Test {};

TEST_F(LivenessTest, ReturnsAliveWhenRunningIsTrue) {
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "alive");
    EXPECT_TRUE(body["checks"]["server_running"]);
}

TEST_F(LivenessTest, Returns503WhenRunningIsFalse) {
    std::atomic<bool> running{false};
    auto handler = make_handler(&running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "dead");
    EXPECT_FALSE(body["checks"]["server_running"]);
}

TEST_F(LivenessTest, ReturnsAliveWhenIsRunningIsNull) {
    // When no is_running pointer supplied, assume alive (e.g. lightweight usage)
    auto handler = make_handler(nullptr);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "alive");
}

TEST_F(LivenessTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW({
        auto parsed = json::parse(res.body());
        static_cast<void>(parsed);
    });
}

TEST_F(LivenessTest, ResponseContainsChecksObject) {
    auto handler = make_handler();

    auto res = handler->handleLiveness(make_get("/health/live"));

    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("checks"));
    ASSERT_TRUE(body["checks"].is_object());
}

TEST_F(LivenessTest, TransitionFromAliveToDeadReflectedImmediately) {
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    // Alive
    auto res1 = handler->handleLiveness(make_get("/health/live"));
    EXPECT_EQ(res1.result(), http::status::ok);

    // Now stop
    running.store(false);

    auto res2 = handler->handleLiveness(make_get("/health/live"));
    EXPECT_EQ(res2.result(), http::status::service_unavailable);
}

// ---------------------------------------------------------------------------
// Readiness Tests
// ---------------------------------------------------------------------------

class ReadinessTest : public ::testing::Test {};

TEST_F(ReadinessTest, ReturnsReadyWhenRunningAndNoStorage) {
    // No storage configured: treated as ready (lightweight deployment)
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ready");
}

TEST_F(ReadinessTest, Returns503WhenNotRunning) {
    std::atomic<bool> running{false};
    auto handler = make_handler(&running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "not_ready");
}

TEST_F(ReadinessTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW({
        auto parsed = json::parse(res.body());
        static_cast<void>(parsed);
    });
}

TEST_F(ReadinessTest, ResponseContainsChecksObject) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("checks"));
    ASSERT_TRUE(body["checks"].is_object());
}

TEST_F(ReadinessTest, ChecksContainServerRunningField) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body["checks"].contains("server_running"));
}

TEST_F(ReadinessTest, ChecksContainStorageAvailableField) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body["checks"].contains("storage_available"));
}

// ---------------------------------------------------------------------------
// Health Check (existing /health endpoint) Tests
// ---------------------------------------------------------------------------

class HealthCheckTest : public ::testing::Test {};

TEST_F(HealthCheckTest, ReturnsHealthyStatus) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "healthy");
}

TEST_F(HealthCheckTest, ResponseContainsUptimeSeconds) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("uptime_seconds"));
    EXPECT_GE(body["uptime_seconds"].get<int64_t>(), 0);
}

TEST_F(HealthCheckTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW({
        auto parsed = json::parse(res.body());
        static_cast<void>(parsed);
    });
}

// ---------------------------------------------------------------------------
// ConcernsContext integration tests
// ---------------------------------------------------------------------------

using namespace themis::core::concerns;

class ConcernsLivenessTest : public ::testing::Test {};

TEST_F(ConcernsLivenessTest, LivenessIncludesConcernsSectionWhenContextProvided) {
    auto ctx = ConcernsContext::createNoOp();
    auto handler = make_handler_with_concerns(ctx);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    ASSERT_TRUE(body["checks"].contains("concerns"));
    auto& concerns = body["checks"]["concerns"];
    EXPECT_TRUE(concerns.contains("logger"));
    EXPECT_TRUE(concerns.contains("tracer"));
    EXPECT_TRUE(concerns.contains("metrics"));
    EXPECT_TRUE(concerns.contains("cache"));
    EXPECT_TRUE(concerns.contains("secrets"));
}

TEST_F(ConcernsLivenessTest, LivenessReturns200WhenAllConcernsHealthy) {
    auto ctx = ConcernsContext::createNoOp();
    std::atomic<bool> running{true};
    auto handler = make_handler_with_concerns(ctx, &running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "alive");
    EXPECT_TRUE(body["checks"]["concerns"]["logger"]["ok"].get<bool>());
    EXPECT_TRUE(body["checks"]["concerns"]["tracer"]["ok"].get<bool>());
    EXPECT_TRUE(body["checks"]["concerns"]["metrics"]["ok"].get<bool>());
    EXPECT_TRUE(body["checks"]["concerns"]["cache"]["ok"].get<bool>());
    EXPECT_TRUE(body["checks"]["concerns"]["secrets"]["ok"].get<bool>());
}

TEST_F(ConcernsLivenessTest, LivenessReturns503WhenConcernUnhealthy) {
    // Create a context where the logger reports unhealthy
    class UnhealthyLogger : public NoOpLogger {
    public:
        ProbeResult isHealthy() const override {
            return ProbeResult::unhealthy("sink unreachable");
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<UnhealthyLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );

    std::atomic<bool> running{true};
    auto handler = make_handler_with_concerns(ctx, &running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "dead");
    EXPECT_FALSE(body["checks"]["concerns"]["logger"]["ok"].get<bool>());
    EXPECT_EQ(body["checks"]["concerns"]["logger"]["message"], "sink unreachable");
}

TEST_F(ConcernsLivenessTest, LivenessWithoutContextHasNoConcernsSection) {
    auto handler = make_handler();

    auto res = handler->handleLiveness(make_get("/health/live"));

    auto body = json::parse(res.body());
    EXPECT_FALSE(body["checks"].contains("concerns"));
}

class ConcernsReadinessTest : public ::testing::Test {};

TEST_F(ConcernsReadinessTest, ReadinessIncludesConcernsSectionWhenContextProvided) {
    auto ctx = ConcernsContext::createNoOp();
    auto handler = make_handler_with_concerns(ctx);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    ASSERT_TRUE(body["checks"].contains("concerns"));
    auto& concerns = body["checks"]["concerns"];
    EXPECT_TRUE(concerns.contains("logger"));
    EXPECT_TRUE(concerns.contains("tracer"));
    EXPECT_TRUE(concerns.contains("metrics"));
    EXPECT_TRUE(concerns.contains("cache"));
    EXPECT_TRUE(concerns.contains("secrets"));
}

TEST_F(ConcernsReadinessTest, ReadinessReturns200WhenAllConcernsReady) {
    auto ctx = ConcernsContext::createNoOp();
    std::atomic<bool> running{true};
    auto handler = make_handler_with_concerns(ctx, &running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ready");
}

TEST_F(ConcernsReadinessTest, ReadinessReturns503WhenConcernNotReady) {
    class UnhealthyCache : public NoOpCache {
    public:
        ProbeResult isHealthy() const override {
            return ProbeResult::unhealthy("cache backend unavailable");
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<UnhealthyCache>()
    );

    std::atomic<bool> running{true};
    auto handler = make_handler_with_concerns(ctx, &running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "not_ready");
    EXPECT_FALSE(body["checks"]["concerns"]["cache"]["ok"].get<bool>());
    EXPECT_EQ(body["checks"]["concerns"]["cache"]["message"], "cache backend unavailable");
}

TEST_F(ConcernsReadinessTest, ReadinessWithoutContextHasNoConcernsSection) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    EXPECT_FALSE(body["checks"].contains("concerns"));
}

TEST(MonitoringStatsTest, IncludesContinuousLearningStatus) {
    const auto temp_dir =
        std::filesystem::temp_directory_path() / "themis_monitoring_cl_status_test";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = temp_dir.string();
    cfg.enable_blobdb = false;
    auto storage = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(storage->open());

    std::atomic<uint64_t> req_count{0};
    std::atomic<uint64_t> err_count{0};
    const auto start = std::chrono::steady_clock::now();

    {
        themis::server::MonitoringApiHandler handler(
            storage, nullptr, &req_count, &err_count, &start, nullptr, nullptr, nullptr);

        auto bao = std::make_shared<themis::performance::phase3::BaoOptimizer>();
        auto workload = std::make_shared<themis::performance::WorkloadAdaptiveOptimizer>();
        auto feedback = std::make_shared<themis::prompt_engineering::FeedbackCollector>();
        auto orchestrator = std::make_shared<themis::rag::learning::ContinuousLearningOrchestrator>(
            themis::rag::learning::ContinuousLearningConfig{});
        orchestrator->wireLiveSignalProviders(bao, workload, feedback);

        const auto plans = bao->generate_plans("SELECT * FROM health_checks");
        const auto plan = bao->select_plan("SELECT * FROM health_checks", plans);
        bao->update_model(plan, themis::performance::phase3::QueryResult{900.0, 1, true});

        const auto profile = workload->classify_workload();
        const auto strategy = workload->get_strategy(profile);
        workload->apply_strategy(strategy);
        workload->apply_strategy(strategy);

        feedback->recordFeedback(
            "adapter-health",
            "hello",
            "world",
            themis::prompt_engineering::FeedbackType::USER_POSITIVE,
            "ok",
            0.2);

        orchestrator->triggerLoop1QueryExecution({"stats-test", 900.0, "{}", true});
        orchestrator->triggerLoop2WorkloadAdaptation();
        orchestrator->triggerLoop3IndexLifecycle();
        orchestrator->triggerLoop4AdapterImprovement();
        handler.setContinuousLearningOrchestrator(orchestrator);

        const auto res = handler.handleStats(make_get("/stats"));
        ASSERT_EQ(res.result(), http::status::ok);

        const auto body = json::parse(res.body());
        ASSERT_TRUE(body.contains("continuous_learning"));
        ASSERT_TRUE(body["continuous_learning"].contains("loops"));
        EXPECT_FALSE(body["continuous_learning"]["loops"].empty());
        bool loop3_present = false;
        for (const auto& loop : body["continuous_learning"]["loops"]) {
            if (loop.value("phase", std::string{}) == "LOOP_3_SCHEMA_INDEX") {
                loop3_present = true;
                EXPECT_TRUE(loop.value("guardrail", false));
                EXPECT_EQ(loop.value("signal_source", std::string{}), "advisory");
                EXPECT_DOUBLE_EQ(loop.value("signal_value", -1.0), 0.0);
                break;
            }
        }
        EXPECT_TRUE(loop3_present);
    }

    storage.reset();
    std::filesystem::remove_all(temp_dir);
}

TEST(MonitoringStatsTest, ExposesContinuousLearningLoopTelemetryInPrometheusMetrics) {
    const auto temp_dir =
        std::filesystem::temp_directory_path() / "themis_monitoring_cl_metrics_test";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = temp_dir.string();
    cfg.enable_blobdb = false;
    auto storage = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(storage->open());

    std::atomic<uint64_t> req_count{0};
    std::atomic<uint64_t> err_count{0};
    const auto start = std::chrono::steady_clock::now();

    {
        themis::server::MonitoringApiHandler handler(
            storage, nullptr, &req_count, &err_count, &start, nullptr, nullptr, nullptr);

        auto bao = std::make_shared<themis::performance::phase3::BaoOptimizer>();
        auto workload = std::make_shared<themis::performance::WorkloadAdaptiveOptimizer>();
        auto feedback = std::make_shared<themis::prompt_engineering::FeedbackCollector>();
        auto orchestrator = std::make_shared<themis::rag::learning::ContinuousLearningOrchestrator>(
            themis::rag::learning::ContinuousLearningConfig{});
        orchestrator->wireLiveSignalProviders(bao, workload, feedback);

        const auto plans = bao->generate_plans("SELECT * FROM health_checks");
        const auto plan = bao->select_plan("SELECT * FROM health_checks", plans);
        bao->update_model(plan, themis::performance::phase3::QueryResult{900.0, 1, true});

        const auto profile = workload->classify_workload();
        const auto strategy = workload->get_strategy(profile);
        workload->apply_strategy(strategy);
        workload->apply_strategy(strategy);

        feedback->recordFeedback(
            "adapter-health",
            "hello",
            "world",
            themis::prompt_engineering::FeedbackType::USER_POSITIVE,
            "ok",
            0.2);

        orchestrator->triggerLoop1QueryExecution({"metrics-test", 900.0, "{}", true});
        orchestrator->triggerLoop2WorkloadAdaptation();
        orchestrator->triggerLoop3IndexLifecycle();
        orchestrator->triggerLoop4AdapterImprovement();
        handler.setContinuousLearningOrchestrator(orchestrator);

        const auto res = handler.handleMetrics(make_get("/metrics"));
        ASSERT_EQ(res.result(), http::status::ok);

        const std::string body = res.body();
        EXPECT_NE(body.find("themis_continuous_learning_loop_signal_value"), std::string::npos);
        EXPECT_NE(body.find("themis_continuous_learning_loop_guardrail_passed"), std::string::npos);
        EXPECT_NE(body.find("themis_continuous_learning_loop_live_signal"), std::string::npos);
        EXPECT_NE(body.find("phase=\"LOOP_1_HNSW_QUERY\",source=\"live\""), std::string::npos);
        EXPECT_NE(body.find("phase=\"LOOP_2_WORKLOAD\",source=\"live\""), std::string::npos);
        EXPECT_NE(body.find("phase=\"LOOP_3_SCHEMA_INDEX\",source=\"advisory\""), std::string::npos);
        EXPECT_NE(body.find("phase=\"LOOP_4_RLAIF\",source=\"live\""), std::string::npos);
    }

    storage.reset();
    std::filesystem::remove_all(temp_dir);
}
