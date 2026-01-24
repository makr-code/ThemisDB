#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include <gtest/gtest.h>
#include <memory>

using namespace themis::core::concerns;

// Test fixture for concerns
class ConcernsContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a no-op context for testing
        context = ConcernsContext::createNoOp();
    }

    std::shared_ptr<ConcernsContext> context;
};

// ===== Logger Tests =====

TEST_F(ConcernsContextTest, LoggerLevelConversion) {
    EXPECT_EQ(ILogger::Level::TRACE, ILogger::levelFromString("trace"));
    EXPECT_EQ(ILogger::Level::DEBUG, ILogger::levelFromString("debug"));
    EXPECT_EQ(ILogger::Level::INFO, ILogger::levelFromString("info"));
    EXPECT_EQ(ILogger::Level::WARN, ILogger::levelFromString("warn"));
    EXPECT_EQ(ILogger::Level::ERROR, ILogger::levelFromString("error"));
    EXPECT_EQ(ILogger::Level::CRITICAL, ILogger::levelFromString("critical"));
    EXPECT_EQ(ILogger::Level::INFO, ILogger::levelFromString("unknown")); // Default
}

TEST_F(ConcernsContextTest, LoggerLevelToString) {
    EXPECT_STREQ("TRACE", ILogger::levelToString(ILogger::Level::TRACE));
    EXPECT_STREQ("DEBUG", ILogger::levelToString(ILogger::Level::DEBUG));
    EXPECT_STREQ("INFO", ILogger::levelToString(ILogger::Level::INFO));
    EXPECT_STREQ("WARN", ILogger::levelToString(ILogger::Level::WARN));
    EXPECT_STREQ("ERROR", ILogger::levelToString(ILogger::Level::ERROR));
    EXPECT_STREQ("CRITICAL", ILogger::levelToString(ILogger::Level::CRITICAL));
}

TEST_F(ConcernsContextTest, NoOpLoggerWorks) {
    // Should not crash or throw
    context->logger().trace("trace message");
    context->logger().debug("debug message");
    context->logger().info("info message");
    context->logger().warn("warn message");
    context->logger().error("error message");
    context->logger().critical("critical message");
    
    context->logger().setLevel(ILogger::Level::DEBUG);
    EXPECT_EQ(ILogger::Level::DEBUG, context->logger().getLevel());
}

// ===== Tracer Tests =====

TEST_F(ConcernsContextTest, NoOpTracerWorks) {
    // Should not crash or throw
    auto span = context->tracer().startSpan("test_span");
    EXPECT_FALSE(span->isValid()); // NoOp span is not valid
    
    span->setAttribute("key", "value");
    span->setAttribute("count", 42);
    span->setAttribute("ratio", 3.14);
    span->setAttribute("flag", true);
    span->recordError("test error");
    span->setStatus(false, "failed");
    span->end();
}

TEST_F(ConcernsContextTest, ScopedSpanWorks) {
    // Should not crash or throw
    ScopedSpan span(context->tracer(), "scoped_span");
    span.setAttribute("operation", "test");
    span.setAttribute("count", 10);
    // Span ends automatically when scope exits
}

// ===== Metrics Tests =====

TEST_F(ConcernsContextTest, NoOpMetricsWorks) {
    // Should not crash or throw
    context->metrics().incrementCounter("test_counter", 1);
    context->metrics().setGauge("test_gauge", 42.0);
    context->metrics().incrementGauge("test_gauge", 1.0);
    context->metrics().decrementGauge("test_gauge", 1.0);
    context->metrics().observeHistogram("test_histogram", 100.0);
    context->metrics().recordLatency("test_op", 50.0);
    context->metrics().recordError("test_op");
    context->metrics().recordSuccess("test_op");
    
    std::string metrics = context->metrics().exportMetrics();
    EXPECT_TRUE(metrics.empty()); // NoOp returns empty string
}

TEST_F(ConcernsContextTest, LatencyTimerWorks) {
    // Should not crash or throw
    {
        LatencyTimer timer(context->metrics(), "test_operation");
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_GT(timer.elapsedMs(), 0.0);
    }
    // Timer records latency on destruction
}

// ===== Cache Tests =====

TEST_F(ConcernsContextTest, InMemoryCacheBasicOperations) {
    auto cache = std::make_unique<InMemoryCacheImpl>(100, 0);
    
    // Put and Get
    CacheEntry entry{"test_data", 1, 12345};
    EXPECT_TRUE(cache->put("key1", entry));
    
    auto retrieved = cache->get("key1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ("test_data", retrieved->payload);
    EXPECT_EQ(1, retrieved->version);
    
    // Miss
    auto missing = cache->get("nonexistent");
    EXPECT_FALSE(missing.has_value());
    
    // Stats
    EXPECT_EQ(1, cache->size());
    EXPECT_EQ(1, cache->hitCount());
    EXPECT_EQ(1, cache->missCount());
    EXPECT_DOUBLE_EQ(0.5, cache->hitRate());
}

TEST_F(ConcernsContextTest, InMemoryCacheInvalidation) {
    auto cache = std::make_unique<InMemoryCacheImpl>(100, 0);
    
    CacheEntry entry{"data", 1, 0};
    cache->put("key1", entry);
    cache->put("key2", entry);
    
    EXPECT_EQ(2, cache->size());
    
    cache->invalidate("key1");
    EXPECT_EQ(1, cache->size());
    EXPECT_FALSE(cache->get("key1").has_value());
    
    cache->clear();
    EXPECT_EQ(0, cache->size());
}

TEST_F(ConcernsContextTest, InMemoryCacheTTL) {
    auto cache = std::make_unique<InMemoryCacheImpl>(100, 100); // 100ms default TTL
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    CacheEntry entry{"data", 1, static_cast<uint64_t>(now)};
    cache->put("key1", entry, 50); // 50ms TTL
    
    // Should be available immediately
    EXPECT_TRUE(cache->get("key1").has_value());
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    
    // Should be expired
    EXPECT_FALSE(cache->get("key1").has_value());
}

TEST_F(ConcernsContextTest, InMemoryCacheEviction) {
    auto cache = std::make_unique<InMemoryCacheImpl>(2, 0); // Max 2 entries
    
    CacheEntry entry{"data", 1, 0};
    cache->put("key1", entry);
    cache->put("key2", entry);
    EXPECT_EQ(2, cache->size());
    
    // Adding third entry should evict oldest
    cache->put("key3", entry);
    EXPECT_EQ(2, cache->size());
}

// ===== Context Tests =====

TEST_F(ConcernsContextTest, CreateNoOpContext) {
    auto ctx = ConcernsContext::createNoOp();
    ASSERT_NE(nullptr, ctx);
    
    // Should be able to use all concerns without crashes
    ctx->logger().info("test");
    auto span = ctx->tracer().startSpan("test");
    ctx->metrics().incrementCounter("test");
    ctx->cache().put("key", CacheEntry{"data"});
}

TEST_F(ConcernsContextTest, CreateCustomContext) {
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );
    
    ASSERT_NE(nullptr, ctx);
    ctx->logInfo("test info");
    ctx->logError("test error");
    ctx->logWarn("test warn");
    ctx->logDebug("test debug");
}

TEST_F(ConcernsContextTest, ConvenienceMethods) {
    // Test convenience methods
    context->logInfo("info");
    context->logError("error");
    context->logWarn("warn");
    context->logDebug("debug");
    
    auto span = context->startSpan("operation");
    EXPECT_NE(nullptr, span);
    
    context->recordMetric("test_metric", 42.0);
}

// ===== Integration Test =====

TEST_F(ConcernsContextTest, FullIntegration) {
    // Create a custom context with real implementations
    auto logger = std::make_unique<NoOpLogger>(); // Use NoOp for testing
    auto tracer = std::make_unique<NoOpTracer>();
    auto metrics = std::make_unique<NoOpMetrics>();
    auto cache = std::make_unique<InMemoryCacheImpl>(1000, 5000);
    
    auto ctx = ConcernsContext::createCustom(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache)
    );
    
    // Simulate a component using the context
    {
        ctx->logInfo("Starting operation");
        
        auto span = ctx->startSpan("database_query");
        span->setAttribute("table", "users");
        
        LatencyTimer timer(ctx->metrics(), "query_latency");
        
        // Check cache
        auto cached = ctx->cache().get("user:123");
        if (!cached) {
            ctx->metrics().incrementCounter("cache_miss");
            // Simulate DB query
            CacheEntry entry{"user_data", 1, 0};
            ctx->cache().put("user:123", entry, 10000);
        } else {
            ctx->metrics().incrementCounter("cache_hit");
        }
        
        ctx->logInfo("Operation completed");
    }
    
    // Verify cache state
    EXPECT_EQ(1, ctx->cache().size());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
