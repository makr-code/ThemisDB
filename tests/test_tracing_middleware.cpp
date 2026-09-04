#include <gtest/gtest.h>
#include "api/tracing_middleware.h"
#include "utils/logger.h"
#include <thread>
#include <vector>
#include <atomic>
#include <regex>

using namespace themis::api;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class TracingMiddlewareTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure a clean context before each test
        TracingMiddleware::clearContext();
    }
    void TearDown() override {
        TracingMiddleware::clearContext();
    }

    TracingMiddleware mw_;
};

// ---------------------------------------------------------------------------
// UUID v4 generation
// ---------------------------------------------------------------------------

// RFC 4122 UUID v4: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
static const std::regex kUuidV4Regex(
    "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
    std::regex_constants::icase);

TEST_F(TracingMiddlewareTest, GenerateUuidV4_FormatValid) {
    auto uuid = TracingMiddleware::generateUuidV4();
    EXPECT_EQ(uuid.size(), 36u) << "UUID should be 36 chars";
    EXPECT_TRUE(std::regex_match(uuid, kUuidV4Regex))
        << "UUID format invalid: " << uuid;
}

TEST_F(TracingMiddlewareTest, GenerateUuidV4_UniquePerCall) {
    auto u1 = TracingMiddleware::generateUuidV4();
    auto u2 = TracingMiddleware::generateUuidV4();
    EXPECT_NE(u1, u2);
}

TEST_F(TracingMiddlewareTest, GenerateUuidV4_ThreadSafe) {
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100;
    std::vector<std::string> results(kThreads * kPerThread);
    std::atomic<int> idx{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kPerThread; ++i) {
                int pos = idx.fetch_add(1, std::memory_order_relaxed);
                results[pos] = TracingMiddleware::generateUuidV4();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // All UUIDs must be unique
    std::sort(results.begin(), results.end());
    EXPECT_EQ(std::unique(results.begin(), results.end()), results.end())
        << "Duplicate UUID detected across threads";
}

// ---------------------------------------------------------------------------
// processRequest – incoming ID provided
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, ProcessRequest_UsesIncomingId) {
    const std::string given_id = "my-trace-1234";
    auto result = mw_.processRequest(given_id);
    EXPECT_EQ(result, given_id);
}

TEST_F(TracingMiddlewareTest, ProcessRequest_StoresInThreadLocal) {
    const std::string given_id = "stored-corr-id";
    mw_.processRequest(given_id);
    EXPECT_EQ(TracingMiddleware::currentCorrelationId(), given_id);
}

TEST_F(TracingMiddlewareTest, ProcessRequest_SetsLoggerTraceContext) {
    const std::string given_id = "logger-corr-id";
    mw_.processRequest(given_id);
    // The logger should now carry this ID in its pattern
    EXPECT_EQ(themis::utils::Logger::getTraceContext(), given_id);
}

// ---------------------------------------------------------------------------
// processRequest – no incoming ID (generate UUID)
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, ProcessRequest_GeneratesWhenEmpty) {
    auto result = mw_.processRequest("");
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(std::regex_match(result, kUuidV4Regex))
        << "Generated ID is not a valid UUID v4: " << result;
}

TEST_F(TracingMiddlewareTest, ProcessRequest_GeneratedIdStoredInThreadLocal) {
    auto result = mw_.processRequest("");
    EXPECT_EQ(TracingMiddleware::currentCorrelationId(), result);
}

TEST_F(TracingMiddlewareTest, ProcessRequest_GeneratedIdSetsLoggerContext) {
    auto result = mw_.processRequest("");
    EXPECT_EQ(themis::utils::Logger::getTraceContext(), result);
}

// ---------------------------------------------------------------------------
// clearContext
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, ClearContext_EmptiesThreadLocal) {
    mw_.processRequest("clear-me");
    ASSERT_EQ(TracingMiddleware::currentCorrelationId(), "clear-me");
    TracingMiddleware::clearContext();
    EXPECT_TRUE(TracingMiddleware::currentCorrelationId().empty());
}

TEST_F(TracingMiddlewareTest, ClearContext_ClearsLoggerTraceContext) {
    mw_.processRequest("clear-logger");
    ASSERT_EQ(themis::utils::Logger::getTraceContext(), "clear-logger");
    TracingMiddleware::clearContext();
    EXPECT_TRUE(themis::utils::Logger::getTraceContext().empty());
}

TEST_F(TracingMiddlewareTest, ClearContext_SafeToCallWhenEmpty) {
    // Should not throw or crash
    EXPECT_NO_THROW(TracingMiddleware::clearContext());
}

// ---------------------------------------------------------------------------
// Thread isolation: each thread has its own context
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, ThreadIsolation) {
    const std::string id_a = "thread-a-id";
    const std::string id_b = "thread-b-id";

    TracingMiddleware mw_a;
    TracingMiddleware mw_b;

    std::string seen_a, seen_b;

    std::thread ta([&]() {
        mw_a.processRequest(id_a);
        // small pause so both threads are active simultaneously
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        seen_a = TracingMiddleware::currentCorrelationId();
        TracingMiddleware::clearContext();
    });

    std::thread tb([&]() {
        mw_b.processRequest(id_b);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        seen_b = TracingMiddleware::currentCorrelationId();
        TracingMiddleware::clearContext();
    });

    ta.join();
    tb.join();

    EXPECT_EQ(seen_a, id_a) << "Thread A saw wrong correlation ID";
    EXPECT_EQ(seen_b, id_b) << "Thread B saw wrong correlation ID";
}

// ---------------------------------------------------------------------------
// Header name constant
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, HeaderNameConstant) {
    EXPECT_EQ(TracingMiddleware::kCorrelationIdHeader, "X-Correlation-ID");
}

// ---------------------------------------------------------------------------
// Successive processRequest calls overwrite the context
// ---------------------------------------------------------------------------

TEST_F(TracingMiddlewareTest, ProcessRequest_OverwritesPreviousContext) {
    mw_.processRequest("first-id");
    ASSERT_EQ(TracingMiddleware::currentCorrelationId(), "first-id");

    mw_.processRequest("second-id");
    EXPECT_EQ(TracingMiddleware::currentCorrelationId(), "second-id");
    EXPECT_EQ(themis::utils::Logger::getTraceContext(), "second-id");
}
