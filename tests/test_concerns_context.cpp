#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/lifecycle.h"
#include "core/concerns/metric_labels.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

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
    span->setAttribute("count", 42.0);
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
    span.setAttribute("count", 10.0);
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

// ===== Lifecycle Hook Tests =====

TEST_F(ConcernsContextTest, FlushDoesNotCrash) {
    // flush() should be callable without errors on a no-op context
    EXPECT_NO_THROW(context->flush());
}

TEST_F(ConcernsContextTest, ShutdownDoesNotCrash) {
    // shutdown() should be callable without errors on a no-op context
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->shutdown());
}

TEST_F(ConcernsContextTest, NoOpLoggerLifecycle) {
    NoOpLogger logger;
    EXPECT_NO_THROW(logger.flush());
    EXPECT_NO_THROW(logger.shutdown());
    auto result = logger.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(ConcernsContextTest, NoOpTracerLifecycle) {
    NoOpTracer tracer;
    EXPECT_NO_THROW(tracer.flush());
    EXPECT_NO_THROW(tracer.shutdown());
    auto result = tracer.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(ConcernsContextTest, NoOpMetricsLifecycle) {
    NoOpMetrics metrics;
    EXPECT_NO_THROW(metrics.flush());
    EXPECT_NO_THROW(metrics.shutdown());
    auto result = metrics.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(ConcernsContextTest, NoOpCacheLifecycle) {
    NoOpCache cache;
    EXPECT_NO_THROW(cache.flush());
    EXPECT_NO_THROW(cache.shutdown());
    auto result = cache.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(ConcernsContextTest, InMemoryCacheShutdownClearsEntries) {
    auto cache = std::make_unique<InMemoryCacheImpl>(100, 0);
    cache->put("k1", CacheEntry{"v1"});
    cache->put("k2", CacheEntry{"v2"});
    EXPECT_EQ(2u, cache->size());

    cache->shutdown();
    EXPECT_EQ(0u, cache->size());
}

TEST_F(ConcernsContextTest, InMemoryCacheIsHealthy) {
    auto cache = std::make_unique<InMemoryCacheImpl>(100, 0);
    auto result = cache->isHealthy();
    EXPECT_TRUE(result.ok);
    EXPECT_EQ("in-memory cache operational", result.message);
}

// ===== Health / Readiness Probe Tests =====

TEST_F(ConcernsContextTest, HealthCheckAllHealthy) {
    auto status = context->healthCheck();
    EXPECT_TRUE(status.logger.ok);
    EXPECT_TRUE(status.tracer.ok);
    EXPECT_TRUE(status.metrics.ok);
    EXPECT_TRUE(status.cache.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(ConcernsContextTest, ReadinessCheckAllReady) {
    auto status = context->readinessCheck();
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(ConcernsContextTest, HealthStatusUnhealthyWhenOneConcernFails) {
    class UnhealthyLogger : public NoOpLogger {
    public:
        ProbeResult isHealthy() const override {
            return ProbeResult::unhealthy("sink not reachable");
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<UnhealthyLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );

    auto status = ctx->healthCheck();
    EXPECT_FALSE(status.logger.ok);
    EXPECT_EQ("sink not reachable", status.logger.message);
    EXPECT_FALSE(status.isHealthy());
}

TEST_F(ConcernsContextTest, ProbeResultHelpers) {
    auto ok = ProbeResult::healthy("all good");
    EXPECT_TRUE(ok.ok);
    EXPECT_EQ("all good", ok.message);

    auto bad = ProbeResult::unhealthy("connection refused");
    EXPECT_FALSE(bad.ok);
    EXPECT_EQ("connection refused", bad.message);
}

TEST_F(ConcernsContextTest, HealthStatusDefaultIsHealthy) {
    HealthStatus status;
    // All default-constructed ProbeResults have ok=true
    EXPECT_TRUE(status.isHealthy());
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

    // Verify lifecycle hooks work on a fully-used context
    EXPECT_NO_THROW(ctx->flush());
    auto status = ctx->healthCheck();
    EXPECT_TRUE(status.isHealthy());
}

// ===== MetricLabels Tests =====

using namespace themis::core::concerns;

TEST(MetricLabelsTest, EmptyByDefault) {
    MetricLabels ml;
    EXPECT_TRUE(ml.empty());
    EXPECT_EQ(0u, ml.size());
}

TEST(MetricLabelsTest, AddSingleLabel) {
    MetricLabels ml;
    ml.add("method", "GET");
    EXPECT_EQ(1u, ml.size());
    EXPECT_FALSE(ml.empty());
}

TEST(MetricLabelsTest, FluentChaining) {
    auto ml = MetricLabels()
        .add(labels::kMethod,   "POST")
        .add(labels::kStatus,   "201")
        .add(labels::kEndpoint, "/api/v1/doc");
    EXPECT_EQ(3u, ml.size());
}

TEST(MetricLabelsTest, ToLabelsReturnsCorrectMap) {
    auto ml = MetricLabels()
        .add(labels::kMethod, "GET")
        .add(labels::kStatus, "200");

    IMetrics::Labels m = ml.toLabels();
    ASSERT_EQ(2u, m.size());
    EXPECT_EQ("GET", m.at(std::string(labels::kMethod)));
    EXPECT_EQ("200", m.at(std::string(labels::kStatus)));
}

TEST(MetricLabelsTest, ImplicitConversionToLabels) {
    // MetricLabels must be usable wherever IMetrics::Labels is expected
    MetricLabels ml;
    ml.add(labels::kOperation, "SELECT");
    ml.add(labels::kTable,     "users");

    IMetrics::Labels m = ml;  // implicit conversion
    EXPECT_EQ("SELECT", m.at(std::string(labels::kOperation)));
    EXPECT_EQ("users",  m.at(std::string(labels::kTable)));
}

TEST(MetricLabelsTest, DuplicateKeyOverwritesPrevious) {
    MetricLabels ml;
    ml.add(labels::kStatus, "200");
    ml.add(labels::kStatus, "404");  // overwrite

    IMetrics::Labels m = ml.toLabels();
    EXPECT_EQ(1u, m.size());
    EXPECT_EQ("404", m.at(std::string(labels::kStatus)));
}

TEST(MetricLabelsTest, PredefinedConstantsHaveExpectedValues) {
    EXPECT_EQ("method",       labels::kMethod);
    EXPECT_EQ("status",       labels::kStatus);
    EXPECT_EQ("endpoint",     labels::kEndpoint);
    EXPECT_EQ("operation",    labels::kOperation);
    EXPECT_EQ("table",        labels::kTable);
    EXPECT_EQ("database",     labels::kDatabase);
    EXPECT_EQ("service",      labels::kService);
    EXPECT_EQ("env",          labels::kEnv);
    EXPECT_EQ("instance",     labels::kInstance);
    EXPECT_EQ("error",        labels::kError);
    EXPECT_EQ("result",       labels::kResult);
    EXPECT_EQ("cache_name",   labels::kCacheName);
    EXPECT_EQ("cache_result", labels::kCacheResult);
}

TEST(MetricLabelsTest, PassedToNoOpMetricsWithoutCrash) {
    // Verify the builder integrates end-to-end with IMetrics methods
    NoOpMetrics metrics;
    EXPECT_NO_THROW(
        metrics.incrementCounter("http_requests_total", 1,
            MetricLabels()
                .add(labels::kMethod,   "GET")
                .add(labels::kStatus,   "200")
                .add(labels::kEndpoint, "/health"))
    );
    EXPECT_NO_THROW(
        metrics.recordLatency("db.query", 5.0,
            MetricLabels().add(labels::kOperation, "SELECT"))
    );
    EXPECT_NO_THROW(
        metrics.recordError("cache.get",
            MetricLabels()
                .add(labels::kCacheResult, "miss")
                .add(labels::kCacheName,   "semantic"))
    );
}
