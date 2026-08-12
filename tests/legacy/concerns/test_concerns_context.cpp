#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/inmemory_secrets.h"
#include "core/concerns/lifecycle.h"
#include "core/concerns/metric_labels.h"
#include "core/concerns/i_context.h"
#include "core/concerns/i_async_logger.h"
#include "core/concerns/i_async_cache.h"
#include "core/concerns/i_audit_log.h"
#include "core/config_validator.h"
#include <gtest/gtest.h>
#include <latch>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

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
    EXPECT_TRUE(status.circuit_breaker.ok);
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

// ===== Configuration-Driven Adapter Selection Tests =====

TEST_F(ConcernsContextTest, ConfigAdapterDefaultsAreValid) {
    // Default Config should pass adapter validation without throwing
    ConcernsContext::Config cfg;
    EXPECT_EQ("spdlog",   cfg.loggerAdapter);
    EXPECT_EQ("",         cfg.tracerAdapter);
    EXPECT_EQ("",         cfg.metricsAdapter);
    EXPECT_EQ("inmemory", cfg.cacheAdapter);
    EXPECT_EQ("default",  cfg.circuitBreakerAdapter);

    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        cfg.loggerAdapter, cfg.tracerAdapter,
        cfg.metricsAdapter, cfg.cacheAdapter,
        cfg.circuitBreakerAdapter);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConcernsContextTest, ConfigAdapterNoopValuesAreValid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "noop", "noop", "noop", "noop", "noop");
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConcernsContextTest, ConfigAdapterKnownValuesAreValid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "otel", "prometheus", "inmemory");
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConcernsContextTest, ConfigAdapterJaegerTracerIsValid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "jaeger", "prometheus", "inmemory");
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConcernsContextTest, ConfigAdapterZipkinTracerIsValid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "zipkin", "prometheus", "inmemory");
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConcernsContextTest, ConfigDrivenJaegerTracerSelection) {
    // Setting tracerAdapter="jaeger" must construct a JaegerTracerAdapter
    // without throwing (no real collector needed in unit tests).
    ConcernsContext::Config cfg;
    cfg.tracerAdapter   = "jaeger";
    cfg.tracingEnabled  = true;
    cfg.tracingEndpoint = "http://127.0.0.1:14268/api/traces";
    cfg.metricsEnabled  = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    // Span creation must not crash regardless of whether the collector is reachable.
    EXPECT_NO_THROW({
        auto span = ctx->tracer().startSpan("jaeger_test_span");
        ASSERT_NE(nullptr, span);
        span->end();
    });
}

TEST_F(ConcernsContextTest, ConfigDrivenZipkinTracerSelection) {
    // Setting tracerAdapter="zipkin" must construct a ZipkinTracerAdapter
    // without throwing (no real collector needed in unit tests).
    ConcernsContext::Config cfg;
    cfg.tracerAdapter   = "zipkin";
    cfg.tracingEnabled  = true;
    cfg.tracingEndpoint = "http://127.0.0.1:9411/api/v2/spans";
    cfg.metricsEnabled  = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    EXPECT_NO_THROW({
        auto span = ctx->tracer().startSpan("zipkin_test_span");
        ASSERT_NE(nullptr, span);
        span->end();
    });
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownLoggerAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "log4cpp", "", "", "inmemory");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("loggerAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("log4cpp"));
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownTracerAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "datadog-apm", "", "inmemory");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("tracerAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("datadog-apm"));
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownMetricsAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "", "datadog", "inmemory");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("metricsAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("datadog"));
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownCacheAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "", "", "redis");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("cacheAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("redis"));
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownCircuitBreakerAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "", "", "inmemory", "hystrix");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("circuitBreakerAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("hystrix"));
}

TEST_F(ConcernsContextTest, ConfigAdapterMultipleInvalidAdaptersReportAllErrors) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "log4cpp", "datadog-apm", "datadog", "redis", "hystrix");
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(5u, result.errors.size());
}

TEST_F(ConcernsContextTest, ConfigDrivenNoopLoggerSelection) {
    // Selecting loggerAdapter="noop" via create(Config) must not throw
    // and the resulting logger must be a no-op (no crash on any call).
    ConcernsContext::Config cfg;
    cfg.loggerAdapter = "noop";
    // Use noop for everything else so we don't need a real OTel/Prometheus setup
    cfg.tracingEnabled  = false;
    cfg.metricsEnabled  = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    EXPECT_NO_THROW(ctx->logger().info("should not crash"));
}

TEST_F(ConcernsContextTest, ConfigDrivenNoopTracerSelection) {
    // Explicitly setting tracerAdapter="noop" must override tracingEnabled=true
    // (we can't reach a real OTLP endpoint in unit tests).
    ConcernsContext::Config cfg;
    cfg.tracerAdapter  = "noop";
    cfg.tracingEnabled = true; // would normally require OTel, but adapter overrides
    cfg.metricsEnabled = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    auto span = ctx->tracer().startSpan("test_span");
    EXPECT_FALSE(span->isValid()); // NoOp span is never valid
}

TEST_F(ConcernsContextTest, ConfigDrivenNoopMetricsSelection) {
    // Explicitly setting metricsAdapter="noop" must override metricsEnabled=true.
    ConcernsContext::Config cfg;
    cfg.metricsAdapter = "noop";
    cfg.metricsEnabled = true;
    cfg.tracingEnabled = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    // NoOp metrics export returns empty string
    EXPECT_TRUE(ctx->metrics().exportMetrics().empty());
}

TEST_F(ConcernsContextTest, ConfigDrivenNoopCacheSelection) {
    // Selecting cacheAdapter="noop" must result in a cache that never stores data.
    ConcernsContext::Config cfg;
    cfg.cacheAdapter   = "noop";
    cfg.tracingEnabled = false;
    cfg.metricsEnabled = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    ctx->cache().put("key", CacheEntry{"value"});
    EXPECT_EQ(0u, ctx->cache().size()); // NoOp cache stores nothing
}

TEST_F(ConcernsContextTest, ConfigDrivenInMemoryCacheSelection) {
    // Selecting cacheAdapter="inmemory" (explicit default) must work normally.
    ConcernsContext::Config cfg;
    cfg.cacheAdapter    = "inmemory";
    cfg.cacheMaxSize    = 50;
    cfg.tracingEnabled  = false;
    cfg.metricsEnabled  = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    ctx->cache().put("k1", CacheEntry{"v1"});
    EXPECT_EQ(1u, ctx->cache().size());
}

TEST_F(ConcernsContextTest, ConfigDrivenInvalidAdapterThrows) {
    // An unrecognised adapter name must cause create() to throw.
    ConcernsContext::Config cfg;
    cfg.loggerAdapter = "unknown_logger";

    EXPECT_THROW(ConcernsContext::create(cfg), std::runtime_error);
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

// ===== IContext / SimpleContext Tests =====

TEST(IContextTest, EmptyRootContextHasNoAttributes) {
    auto ctx = SimpleContext::create();
    EXPECT_FALSE(ctx->has(context_keys::kTraceId));
    EXPECT_FALSE(ctx->get(context_keys::kTraceId).has_value());
}

TEST(IContextTest, SetAndGetAttribute) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kTraceId, "abc123");
    EXPECT_TRUE(ctx->has(context_keys::kTraceId));
    EXPECT_EQ("abc123", ctx->get(context_keys::kTraceId).value());
}

TEST(IContextTest, OverwriteExistingAttribute) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kUserId, "user1");
    ctx->set(context_keys::kUserId, "user2");
    EXPECT_EQ("user2", ctx->get(context_keys::kUserId).value());
}

TEST(IContextTest, FactoryWithCorrelationIds) {
    auto ctx = SimpleContext::create("trace-42", "req-7");
    EXPECT_EQ("trace-42", ctx->get(context_keys::kTraceId).value());
    EXPECT_EQ("req-7",    ctx->get(context_keys::kRequestId).value());
}

TEST(IContextTest, ChildInheritsParentAttributes) {
    auto parent = SimpleContext::create("trace-1", "req-1");
    parent->set(context_keys::kService, "themisdb");

    auto child = parent->createChild();
    EXPECT_EQ("trace-1",  child->get(context_keys::kTraceId).value());
    EXPECT_EQ("req-1",    child->get(context_keys::kRequestId).value());
    EXPECT_EQ("themisdb", child->get(context_keys::kService).value());
}

TEST(IContextTest, ChildCanShadowParentAttribute) {
    auto parent = SimpleContext::create("trace-1", "req-1");
    auto child  = parent->createChild();
    child->set(context_keys::kRequestId, "req-child");

    // Child sees its own overridden value.
    EXPECT_EQ("req-child", child->get(context_keys::kRequestId).value());
    // Parent is unchanged.
    EXPECT_EQ("req-1",     parent->get(context_keys::kRequestId).value());
    // Trace ID still inherited.
    EXPECT_EQ("trace-1",   child->get(context_keys::kTraceId).value());
}

TEST(IContextTest, ChildWriteDoesNotAffectParent) {
    auto parent = SimpleContext::create();
    auto child  = parent->createChild();
    child->set(context_keys::kOperation, "db.query");

    EXPECT_FALSE(parent->has(context_keys::kOperation));
    EXPECT_TRUE(child->has(context_keys::kOperation));
}

TEST(IContextTest, GrandchildInheritsAcrossMultipleLevels) {
    auto root  = SimpleContext::create("root-trace", "root-req");
    auto child = root->createChild();
    child->set(context_keys::kService, "worker");
    auto grand = child->createChild();

    EXPECT_EQ("root-trace", grand->get(context_keys::kTraceId).value());
    EXPECT_EQ("root-req",   grand->get(context_keys::kRequestId).value());
    EXPECT_EQ("worker",     grand->get(context_keys::kService).value());
}

TEST(IContextTest, ToTraceContextPopulatesFields) {
    auto ctx = SimpleContext::create("t-abc", "r-123");
    TraceContext tc = ctx->toTraceContext();
    EXPECT_EQ("t-abc", tc.trace_id);
    EXPECT_EQ("r-123", tc.request_id);
}

TEST(IContextTest, ToTraceContextEmptyWhenNoCorrelationIds) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kUserId, "user42");
    TraceContext tc = ctx->toTraceContext();
    EXPECT_TRUE(tc.trace_id.empty());
    EXPECT_TRUE(tc.request_id.empty());
    EXPECT_TRUE(tc.empty());
}

TEST(IContextTest, ToTraceContextInheritsFromParent) {
    auto parent = SimpleContext::create("p-trace", "p-req");
    auto child  = parent->createChild();
    // Child overrides request_id only.
    child->set(context_keys::kRequestId, "c-req");

    TraceContext tc = child->toTraceContext();
    EXPECT_EQ("p-trace", tc.trace_id);   // inherited
    EXPECT_EQ("c-req",   tc.request_id); // overridden
}

TEST(IContextTest, ContextKeysHaveExpectedValues) {
    EXPECT_EQ("trace_id",   context_keys::kTraceId);
    EXPECT_EQ("request_id", context_keys::kRequestId);
    EXPECT_EQ("user_id",    context_keys::kUserId);
    EXPECT_EQ("tenant_id",  context_keys::kTenantId);
    EXPECT_EQ("operation",  context_keys::kOperation);
    EXPECT_EQ("service",    context_keys::kService);
    EXPECT_EQ("session_id", context_keys::kSessionId);
}

TEST(IContextTest, IntegrationWithLogWithContext) {
    // Verify that a SimpleContext can drive ILogger::logWithContext() end-to-end.
    auto ctx = SimpleContext::create("trace-xyz", "req-999");
    ctx->set(context_keys::kOperation, "db.query");

    NoOpLogger logger;
    // Must not throw; logWithContext() falls back to logStructured() in NoOpLogger.
    EXPECT_NO_THROW(
        logger.logWithContext(ILogger::Level::INFO, "query completed",
                              ctx->toTraceContext(), {{"rows", "42"}})
    );
}

// ===== IAsyncLogger / NoOpAsyncLogger Tests =====

TEST(IAsyncLoggerTest, NoOpAsyncLoggerSyncMethodsDoNotThrow) {
    NoOpAsyncLogger logger;
    EXPECT_NO_THROW(logger.trace("t"));
    EXPECT_NO_THROW(logger.debug("d"));
    EXPECT_NO_THROW(logger.info("i"));
    EXPECT_NO_THROW(logger.warn("w"));
    EXPECT_NO_THROW(logger.error("e"));
    EXPECT_NO_THROW(logger.critical("c"));
    EXPECT_NO_THROW(logger.log(ILogger::Level::INFO, "l"));
}

TEST(IAsyncLoggerTest, NoOpAsyncLoggerAsyncMethodsReturnValidFutures) {
    NoOpAsyncLogger logger;

    auto f_info  = logger.infoAsync("hello");
    auto f_error = logger.errorAsync("oops");
    auto f_log   = logger.logAsync(ILogger::Level::WARN, "warn");
    auto f_struct = logger.logStructuredAsync(ILogger::Level::DEBUG, "debug",
                                               {{"key", "val"}});

    // Must be able to .get() without throwing
    EXPECT_NO_THROW(f_info.get());
    EXPECT_NO_THROW(f_error.get());
    EXPECT_NO_THROW(f_log.get());
    EXPECT_NO_THROW(f_struct.get());
}

TEST(IAsyncLoggerTest, NoOpAsyncLoggerAllLevelAsyncMethods) {
    NoOpAsyncLogger logger;
    EXPECT_NO_THROW(logger.traceAsync("t").get());
    EXPECT_NO_THROW(logger.debugAsync("d").get());
    EXPECT_NO_THROW(logger.infoAsync("i").get());
    EXPECT_NO_THROW(logger.warnAsync("w").get());
    EXPECT_NO_THROW(logger.errorAsync("e").get());
    EXPECT_NO_THROW(logger.criticalAsync("c").get());
}

TEST(IAsyncLoggerTest, NoOpAsyncLoggerLifecycle) {
    NoOpAsyncLogger logger;
    EXPECT_NO_THROW(logger.flush());
    EXPECT_NO_THROW(logger.shutdown());
    EXPECT_TRUE(logger.isHealthy().ok);
}

TEST(IAsyncLoggerTest, DefaultAsyncImplCallsSyncMethod) {
    // Verify that the default IAsyncLogger implementation actually invokes
    // the underlying sync log() method. We use a simple counter logger.
    class CountingLogger : public IAsyncLogger {
    public:
        std::atomic<int> count{0};
        void log(Level, const std::string&) override { ++count; }
        void trace(const std::string& m) override { log(Level::TRACE, m); }
        void debug(const std::string& m) override { log(Level::DEBUG, m); }
        void info(const std::string& m) override  { log(Level::INFO, m); }
        void warn(const std::string& m) override  { log(Level::WARN, m); }
        void error(const std::string& m) override { log(Level::ERROR, m); }
        void critical(const std::string& m) override { log(Level::CRITICAL, m); }
        void setLevel(Level) override {}
        Level getLevel() const override { return Level::INFO; }
        void setPattern(const std::string&) override {}
    };

    CountingLogger logger;
    auto f = logger.infoAsync("hello");
    f.get(); // wait for the async call to complete; .get() synchronizes-with the async thread
    EXPECT_EQ(1, logger.count.load());

    logger.errorAsync("oops").get();
    EXPECT_EQ(2, logger.count.load());
}

// ===== IAsyncCache / NoOpAsyncCache Tests =====

TEST(IAsyncCacheTest, NoOpAsyncCacheSyncMethodsReturnSafeDefaults) {
    NoOpAsyncCache cache;
    EXPECT_FALSE(cache.get("key").has_value());
    EXPECT_TRUE(cache.put("key", CacheEntry{"v", 1, 0}, 0));
    EXPECT_EQ(0u, cache.size());
    EXPECT_EQ(0u, cache.hitCount());
    EXPECT_EQ(0u, cache.missCount());
    EXPECT_DOUBLE_EQ(0.0, cache.hitRate());
    EXPECT_NO_THROW(cache.invalidate("key"));
    EXPECT_NO_THROW(cache.clear());
    EXPECT_NO_THROW(cache.invalidatePattern("*"));
}

TEST(IAsyncCacheTest, NoOpAsyncCacheGetAsyncReturnsMiss) {
    NoOpAsyncCache cache;
    auto f = cache.getAsync("user:42");
    auto result = f.get();
    EXPECT_FALSE(result.has_value());
}

TEST(IAsyncCacheTest, NoOpAsyncCachePutAsyncReturnsTrue) {
    NoOpAsyncCache cache;
    CacheEntry entry{"data", 1, 0};
    auto f = cache.putAsync(std::string_view{"user:42"}, entry, static_cast<uint64_t>(10000));
    EXPECT_TRUE(f.get());
}

TEST(IAsyncCacheTest, NoOpAsyncCacheInvalidateAsyncDoesNotThrow) {
    NoOpAsyncCache cache;
    EXPECT_NO_THROW(cache.invalidateAsync("user:42").get());
}

TEST(IAsyncCacheTest, NoOpAsyncCacheLifecycle) {
    NoOpAsyncCache cache;
    EXPECT_NO_THROW(cache.flush());
    EXPECT_NO_THROW(cache.shutdown());
    EXPECT_TRUE(cache.isHealthy().ok);
}

TEST(IAsyncCacheTest, DefaultAsyncImplCallsSyncMethod) {
    // IAsyncCache default methods delegate to InMemoryCacheImpl sync ops.
    class AsyncInMemoryCache : public IAsyncCache, public InMemoryCacheImpl {
    public:
        explicit AsyncInMemoryCache(size_t max) : InMemoryCacheImpl(max, 0) {}

        // Delegate all ICache pure-virtuals to InMemoryCacheImpl
        std::optional<CacheEntry> get(std::string_view k) const override {
            return InMemoryCacheImpl::get(k);
        }
        bool put(std::string_view k, const CacheEntry& e, uint64_t t) override {
            return InMemoryCacheImpl::put(k, e, t);
        }
        void invalidate(std::string_view k) override { InMemoryCacheImpl::invalidate(k); }
        void clear() override { InMemoryCacheImpl::clear(); }
        void invalidatePattern(std::string_view p) override { InMemoryCacheImpl::invalidatePattern(p); }
        size_t size() const override { return InMemoryCacheImpl::size(); }
        uint64_t hitCount()  const override { return InMemoryCacheImpl::hitCount(); }
        uint64_t missCount() const override { return InMemoryCacheImpl::missCount(); }
        double hitRate() const override { return InMemoryCacheImpl::hitRate(); }
        void setMaxSize(size_t m) override { InMemoryCacheImpl::setMaxSize(m); }
        void setDefaultTTL(uint64_t t) override { InMemoryCacheImpl::setDefaultTTL(t); }
    };

    AsyncInMemoryCache cache(100);

    // Async put
    CacheEntry entry{"hello", 1, 0};
    EXPECT_TRUE(cache.putAsync("k1", entry).get());

    // Async get — should find the entry we just put
    auto result = cache.getAsync("k1").get();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("hello", result->payload);

    // Async invalidate — entry should be gone
    cache.invalidateAsync("k1").get();
    EXPECT_FALSE(cache.getAsync("k1").get().has_value());
}

// ===== ISecrets / NoOpSecrets Tests =====

TEST(ISecretsTest, NoOpSecretsGetReturnsNullopt) {
    NoOpSecrets s;
    EXPECT_FALSE(s.getSecret("api.key").has_value());
    EXPECT_FALSE(s.getSecret("db.password").has_value());
}

TEST(ISecretsTest, NoOpSecretsHasSecretReturnsFalse) {
    NoOpSecrets s;
    EXPECT_FALSE(s.hasSecret("any.secret"));
}

TEST(ISecretsTest, NoOpSecretsListSecretNamesIsEmpty) {
    NoOpSecrets s;
    EXPECT_TRUE(s.listSecretNames().empty());
}

TEST(ISecretsTest, NoOpSecretsLifecycleDoesNotCrash) {
    NoOpSecrets s;
    EXPECT_NO_THROW(s.flush());
    EXPECT_NO_THROW(s.shutdown());
}

TEST(ISecretsTest, NoOpSecretsIsHealthy) {
    NoOpSecrets s;
    auto result = s.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(ConcernsContextTest, SecretsAccessorReturnsNoOpByDefault) {
    // createNoOp() should return a NoOpSecrets provider
    EXPECT_FALSE(context->secrets().hasSecret("api.key"));
    EXPECT_FALSE(context->secrets().getSecret("db.password").has_value());
    EXPECT_TRUE(context->secrets().listSecretNames().empty());
}

TEST_F(ConcernsContextTest, CustomSecretsCanBeInjected) {
    // A custom ISecrets implementation for injection testing
    class StubSecrets : public ISecrets {
    public:
        std::optional<std::string> getSecret(std::string_view name) const override {
            if (name == "db.password") return "hunter2";
            if (name == "api.key")     return "sk-test-123";
            return std::nullopt;
        }
        bool hasSecret(std::string_view name) const override {
            return name == "db.password" || name == "api.key";
        }
        std::vector<std::string> listSecretNames() const override {
            return {"api.key", "db.password"};
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<StubSecrets>()
    );

    ASSERT_TRUE(ctx->secrets().hasSecret("db.password"));
    ASSERT_EQ("hunter2", ctx->secrets().getSecret("db.password").value());
    ASSERT_TRUE(ctx->secrets().hasSecret("api.key"));
    ASSERT_EQ("sk-test-123", ctx->secrets().getSecret("api.key").value());
    EXPECT_FALSE(ctx->secrets().hasSecret("unknown"));
    ASSERT_EQ(2u, ctx->secrets().listSecretNames().size());
}

TEST_F(ConcernsContextTest, CreateCustomWithoutSecretsUsesNoOp) {
    // createCustom() with no secrets argument falls back to NoOpSecrets
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );

    EXPECT_FALSE(ctx->secrets().hasSecret("anything"));
    EXPECT_FALSE(ctx->secrets().getSecret("anything").has_value());
}

TEST_F(ConcernsContextTest, HealthCheckIncludesSecretsProbe) {
    auto status = context->healthCheck();
    EXPECT_TRUE(status.secrets.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(ConcernsContextTest, UnhealthySecretsMarksContextUnhealthy) {
    class UnhealthySecrets : public ISecrets {
    public:
        std::optional<std::string> getSecret(std::string_view) const override { return std::nullopt; }
        bool hasSecret(std::string_view) const override { return false; }
        std::vector<std::string> listSecretNames() const override { return {}; }
        ProbeResult isHealthy() const override { return ProbeResult::unhealthy("vault unreachable"); }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<UnhealthySecrets>()
    );

    auto status = ctx->healthCheck();
    EXPECT_FALSE(status.secrets.ok);
    EXPECT_EQ("vault unreachable", status.secrets.message);
    EXPECT_FALSE(status.isHealthy());
}

// ===== InMemorySecrets Tests =====

TEST(InMemorySecretsTest, DefaultConstructorIsEmpty) {
    InMemorySecrets s;
    EXPECT_FALSE(s.hasSecret("any"));
    EXPECT_FALSE(s.getSecret("any").has_value());
    EXPECT_TRUE(s.listSecretNames().empty());
    EXPECT_EQ(0u, s.size());
}

TEST(InMemorySecretsTest, ConstructWithInitialMap) {
    InMemorySecrets s({{ "db.password", "hunter2" }, { "api.key", "sk-test" }});
    ASSERT_TRUE(s.hasSecret("db.password"));
    EXPECT_EQ("hunter2", s.getSecret("db.password").value());
    ASSERT_TRUE(s.hasSecret("api.key"));
    EXPECT_EQ("sk-test", s.getSecret("api.key").value());
    EXPECT_FALSE(s.hasSecret("missing"));
    EXPECT_EQ(2u, s.size());
}

TEST(InMemorySecretsTest, SetSecretAddsEntry) {
    InMemorySecrets s;
    s.setSecret("db.password", "secret123");
    ASSERT_TRUE(s.hasSecret("db.password"));
    EXPECT_EQ("secret123", s.getSecret("db.password").value());
}

TEST(InMemorySecretsTest, SetSecretOverwritesExistingValue) {
    InMemorySecrets s({{"key", "old"}});
    s.setSecret("key", "new");
    EXPECT_EQ("new", s.getSecret("key").value());
}

TEST(InMemorySecretsTest, RemoveSecretDeletesEntry) {
    InMemorySecrets s({{"key", "value"}});
    ASSERT_TRUE(s.removeSecret("key"));
    EXPECT_FALSE(s.hasSecret("key"));
    EXPECT_EQ(0u, s.size());
}

TEST(InMemorySecretsTest, RemoveSecretOnMissingReturnsFalse) {
    InMemorySecrets s;
    EXPECT_FALSE(s.removeSecret("nonexistent"));
}

TEST(InMemorySecretsTest, ListSecretNamesReturnsSortedKeys) {
    InMemorySecrets s({{"zebra", "z"}, {"alpha", "a"}, {"mango", "m"}});
    auto names = s.listSecretNames();
    ASSERT_EQ(3u, names.size());
    EXPECT_EQ("alpha", names[0]);
    EXPECT_EQ("mango", names[1]);
    EXPECT_EQ("zebra", names[2]);
}

TEST(InMemorySecretsTest, GetSecretOnMissingKeyReturnsNullopt) {
    InMemorySecrets s({{"key", "value"}});
    EXPECT_FALSE(s.getSecret("not-present").has_value());
}

TEST(InMemorySecretsTest, LifecycleDoesNotCrash) {
    InMemorySecrets s;
    EXPECT_NO_THROW(s.flush());
    EXPECT_NO_THROW(s.shutdown());
}

TEST(InMemorySecretsTest, IsHealthy) {
    InMemorySecrets s;
    EXPECT_TRUE(s.isHealthy().ok);
}

TEST(InMemorySecretsTest, ThreadSafeSetAndGet) {
    InMemorySecrets s;
    constexpr int kThreads = 8;
    constexpr int kOps = 200;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&s, t]() {
            for (int i = 0; i < kOps; ++i) {
                const std::string key = "secret." + std::to_string(t) + "." + std::to_string(i);
                s.setSecret(key, "val");
                s.hasSecret(key);
                s.getSecret(key);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(static_cast<size_t>(kThreads * kOps), s.size());
}

// ===== EnvSecretsProvider Tests =====

TEST(EnvSecretsProviderTest, DefaultPrefixIsThemisSecret) {
    EnvSecretsProvider p;
    EXPECT_EQ("THEMIS_SECRET_TEST", p.envKeyFor("test"));
}

TEST(EnvSecretsProviderTest, CustomPrefix) {
    EnvSecretsProvider p("MY_APP_");
    EXPECT_EQ("MY_APP_DB_PASSWORD", p.envKeyFor("db.password"));
}

TEST(EnvSecretsProviderTest, DotReplacedWithUnderscore) {
    EnvSecretsProvider p("PREFIX_");
    EXPECT_EQ("PREFIX_API_KEY_STRIPE", p.envKeyFor("api.key.stripe"));
}

TEST(EnvSecretsProviderTest, DashReplacedWithUnderscore) {
    EnvSecretsProvider p("P_");
    EXPECT_EQ("P_REDIS_URL", p.envKeyFor("redis-url"));
}

TEST(EnvSecretsProviderTest, GetSecretReadsEnvVar) {
    // Set an env var and verify it is read back
#if defined(_WIN32)
    _putenv_s("THEMIS_SECRET_TEST_VAL", "my_secret");
#else
    ::setenv("THEMIS_SECRET_TEST_VAL", "my_secret", 1);
#endif
    EnvSecretsProvider p;
    auto val = p.getSecret("test.val");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ("my_secret", val.value());
#if defined(_WIN32)
    _putenv_s("THEMIS_SECRET_TEST_VAL", "");
#else
    ::unsetenv("THEMIS_SECRET_TEST_VAL");
#endif
}

TEST(EnvSecretsProviderTest, HasSecretReturnsFalseWhenEnvVarAbsent) {
    EnvSecretsProvider p;
    // Use a name that is unlikely to exist in the environment
    EXPECT_FALSE(p.hasSecret("nonexistent.secret.x9z"));
}

TEST(EnvSecretsProviderTest, ListSecretNamesOnlyReturnsRegisteredAvailableNames) {
#if defined(_WIN32)
    _putenv_s("THEMIS_SECRET_DB_PASSWORD", "pw123");
#else
    ::setenv("THEMIS_SECRET_DB_PASSWORD", "pw123", 1);
#endif

    EnvSecretsProvider p;
    p.registerName("db.password");           // present in env
    p.registerName("missing.secret.xyz789"); // absent from env

    auto names = p.listSecretNames();
    ASSERT_EQ(1u, names.size());
    EXPECT_EQ("db.password", names[0]);

#if defined(_WIN32)
    _putenv_s("THEMIS_SECRET_DB_PASSWORD", "");
#else
    ::unsetenv("THEMIS_SECRET_DB_PASSWORD");
#endif
}

TEST(EnvSecretsProviderTest, LifecycleDoesNotCrash) {
    EnvSecretsProvider p;
    EXPECT_NO_THROW(p.flush());
    EXPECT_NO_THROW(p.shutdown());
}

TEST(EnvSecretsProviderTest, IsHealthy) {
    EnvSecretsProvider p;
    EXPECT_TRUE(p.isHealthy().ok);
}

// ===== Config-driven secrets adapter selection Tests =====

TEST(ConcernsContextSecretsConfigTest, NoopAdapterByDefault) {
    // The default secretsAdapter is "noop" — no env var needed
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter = "noop";
    cfg.featureFlagsAdapter   = "noop";
    cfg.auditAdapter = "noop";
    // secretsAdapter defaults to "noop"
    auto ctx = ConcernsContext::create(cfg);
    EXPECT_FALSE(ctx->secrets().hasSecret("anything"));
}

TEST(ConcernsContextSecretsConfigTest, InMemoryAdapterPrePopulated) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter = "noop";
    cfg.featureFlagsAdapter   = "noop";
    cfg.auditAdapter = "noop";
    cfg.secretsAdapter = "inmemory";
    cfg.initialSecrets = {{"db.password", "s3cr3t"}, {"api.key", "abc123"}};

    auto ctx = ConcernsContext::create(cfg);
    ASSERT_TRUE(ctx->secrets().hasSecret("db.password"));
    EXPECT_EQ("s3cr3t", ctx->secrets().getSecret("db.password").value());
    ASSERT_TRUE(ctx->secrets().hasSecret("api.key"));
    EXPECT_EQ("abc123", ctx->secrets().getSecret("api.key").value());
    EXPECT_FALSE(ctx->secrets().hasSecret("unknown"));
}

TEST(ConcernsContextSecretsConfigTest, EnvAdapterReadsEnvironmentVariable) {
#if defined(_WIN32)
    _putenv_s("MYAPP_DB_PASSWORD", "envpw");
#else
    ::setenv("MYAPP_DB_PASSWORD", "envpw", 1);
#endif

    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.circuitBreakerAdapter = "noop";
    cfg.featureFlagsAdapter   = "noop";
    cfg.auditAdapter = "noop";
    cfg.secretsAdapter   = "env";
    cfg.secretsEnvPrefix = "MYAPP_";

    auto ctx = ConcernsContext::create(cfg);
    ASSERT_TRUE(ctx->secrets().hasSecret("db.password"));
    EXPECT_EQ("envpw", ctx->secrets().getSecret("db.password").value());

#if defined(_WIN32)
    _putenv_s("MYAPP_DB_PASSWORD", "");
#else
    ::unsetenv("MYAPP_DB_PASSWORD");
#endif
}

TEST(ConcernsContextSecretsConfigTest, InvalidSecretsAdapterThrows) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.cacheAdapter   = "noop";
    cfg.secretsAdapter = "unknown_adapter";
    EXPECT_THROW(ConcernsContext::create(cfg), std::runtime_error);
}

// ===== ConfigValidator secretsAdapter Tests =====

TEST(ConfigValidatorSecretsTest, ValidSecretsAdaptersPass) {
    for (const auto& adapter : std::vector<std::string>{"noop", "inmemory", "env"}) {
        auto result = themis::core::ConfigValidator::validateAdapterConfig(
            "noop", "", "", "noop", "noop", "noop", "noop", adapter);
        EXPECT_TRUE(result.valid) << "Expected valid for secretsAdapter=" << adapter;
    }
}

TEST(ConfigValidatorSecretsTest, InvalidSecretsAdapterFails) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "noop", "", "", "noop", "noop", "noop", "noop", "vault");
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(std::string::npos, result.errors[0].find("secretsAdapter"));
}

TEST(ConfigValidatorSecretsTest, DefaultSecretsAdapterIsNoop) {
    // When 8th argument is omitted it defaults to "noop" — must pass validation
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "noop", "", "", "noop", "noop", "noop", "noop");
    EXPECT_TRUE(result.valid);
}

TEST(InMemoryFeatureFlagsTest, UnknownFlagIsDisabledByDefault) {
    InMemoryFeatureFlags flags;
    EXPECT_FALSE(flags.isEnabled("new_feature"));
    EXPECT_FALSE(flags.isEnabled("does_not_exist"));
}

TEST(InMemoryFeatureFlagsTest, SetValueEnablesFlag) {
    InMemoryFeatureFlags flags;
    flags.setValue("dark_mode", true);
    EXPECT_TRUE(flags.isEnabled("dark_mode"));
}

TEST(InMemoryFeatureFlagsTest, SetValueDisablesFlag) {
    InMemoryFeatureFlags flags;
    flags.setValue("beta_feature", true);
    EXPECT_TRUE(flags.isEnabled("beta_feature"));
    flags.setValue("beta_feature", false);
    EXPECT_FALSE(flags.isEnabled("beta_feature"));
}

TEST(InMemoryFeatureFlagsTest, ConstructWithInitialValues) {
    InMemoryFeatureFlags flags({
        {"feature_a", true},
        {"feature_b", false},
        {"feature_c", true}
    });
    EXPECT_TRUE(flags.isEnabled("feature_a"));
    EXPECT_FALSE(flags.isEnabled("feature_b"));
    EXPECT_TRUE(flags.isEnabled("feature_c"));
}

TEST(InMemoryFeatureFlagsTest, GetAllFlagsReturnsSnapshot) {
    InMemoryFeatureFlags flags({{"f1", true}, {"f2", false}});
    auto all = flags.getAllFlags();
    ASSERT_EQ(2u, all.size());
    EXPECT_TRUE(all.at("f1"));
    EXPECT_FALSE(all.at("f2"));
}

TEST(InMemoryFeatureFlagsTest, GetAllFlagsSnapshotIsIndependent) {
    InMemoryFeatureFlags flags({{"f1", true}});
    auto snapshot = flags.getAllFlags();
    snapshot["f1"] = false;
    EXPECT_TRUE(flags.isEnabled("f1"));
}

TEST(InMemoryFeatureFlagsTest, GetAllFlagsReflectsCurrentState) {
    InMemoryFeatureFlags flags;
    flags.setValue("x", true);
    auto all = flags.getAllFlags();
    EXPECT_TRUE(all.at("x"));
    flags.setValue("x", false);
    auto all2 = flags.getAllFlags();
    EXPECT_FALSE(all2.at("x"));
}

TEST(InMemoryFeatureFlagsTest, MultipleFlags) {
    InMemoryFeatureFlags flags;
    flags.setValue("alpha", true);
    flags.setValue("beta", false);
    flags.setValue("gamma", true);
    EXPECT_TRUE(flags.isEnabled("alpha"));
    EXPECT_FALSE(flags.isEnabled("beta"));
    EXPECT_TRUE(flags.isEnabled("gamma"));
    EXPECT_EQ(3u, flags.getAllFlags().size());
}

TEST(InMemoryFeatureFlagsTest, LifecycleMethods) {
    InMemoryFeatureFlags flags;
    flags.setValue("f", true);
    EXPECT_NO_THROW(flags.flush());
    EXPECT_NO_THROW(flags.shutdown());
}

TEST(InMemoryFeatureFlagsTest, IsHealthyReturnsTrue) {
    InMemoryFeatureFlags flags;
    auto result = flags.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST(InMemoryFeatureFlagsTest, ThreadSafeConcurrentReadsAndWrites) {
    InMemoryFeatureFlags flags;
    flags.setValue("concurrent_flag", false);

    std::atomic<int> read_true_count{0};
    std::atomic<int> read_false_count{0};
    std::latch start_gate{3}; // writer + 2 readers all wait here

    std::thread writer([&]() {
        start_gate.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            flags.setValue("concurrent_flag", i % 2 == 0);
        }
    });

    std::thread reader1([&]() {
        start_gate.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            if (flags.isEnabled("concurrent_flag")) ++read_true_count;
            else ++read_false_count;
        }
    });
    std::thread reader2([&]() {
        start_gate.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            if (flags.isEnabled("concurrent_flag")) ++read_true_count;
            else ++read_false_count;
        }
    });

    writer.join();
    reader1.join();
    reader2.join();

    EXPECT_EQ(200, read_true_count.load() + read_false_count.load());
}

// ===== NoOpFeatureFlags Tests =====

TEST(NoOpFeatureFlagsTest, IsEnabledAlwaysReturnsFalse) {
    NoOpFeatureFlags flags;
    EXPECT_FALSE(flags.isEnabled("any_flag"));
    EXPECT_FALSE(flags.isEnabled("feature_x"));
    EXPECT_FALSE(flags.isEnabled(""));
}

TEST(NoOpFeatureFlagsTest, SetValueIsNoOp) {
    NoOpFeatureFlags flags;
    flags.setValue("feature", true);
    EXPECT_FALSE(flags.isEnabled("feature"));
}

TEST(NoOpFeatureFlagsTest, GetAllFlagsReturnsEmpty) {
    NoOpFeatureFlags flags;
    flags.setValue("f1", true);
    auto all = flags.getAllFlags();
    EXPECT_TRUE(all.empty());
}

TEST(NoOpFeatureFlagsTest, LifecycleMethods) {
    NoOpFeatureFlags flags;
    EXPECT_NO_THROW(flags.flush());
    EXPECT_NO_THROW(flags.shutdown());
}

TEST(NoOpFeatureFlagsTest, IsHealthyReturnsTrue) {
    NoOpFeatureFlags flags;
    EXPECT_TRUE(flags.isHealthy().ok);
}

// ===== NoOpCircuitBreaker Tests =====

TEST(NoOpCircuitBreakerTest, AllowRequestAlwaysTrue) {
    NoOpCircuitBreaker cb;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(cb.allowRequest());
    }
}

TEST(NoOpCircuitBreakerTest, StateIsAlwaysClosed) {
    NoOpCircuitBreaker cb;
    EXPECT_EQ(ICircuitBreaker::State::CLOSED, cb.getState());
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(ICircuitBreaker::State::CLOSED, cb.getState());
    cb.forceOpen();
    EXPECT_EQ(ICircuitBreaker::State::CLOSED, cb.getState());
}

TEST(NoOpCircuitBreakerTest, CountersAreAlwaysZero) {
    NoOpCircuitBreaker cb;
    cb.recordFailure();
    cb.recordSuccess();
    EXPECT_EQ(0u, cb.getFailureCount());
    EXPECT_EQ(0u, cb.getSuccessCount());
}

TEST(NoOpCircuitBreakerTest, ResetDoesNothing) {
    NoOpCircuitBreaker cb;
    EXPECT_NO_THROW(cb.reset());
    EXPECT_EQ(ICircuitBreaker::State::CLOSED, cb.getState());
}

TEST(NoOpCircuitBreakerTest, ForceOpenDoesNothing) {
    NoOpCircuitBreaker cb;
    EXPECT_NO_THROW(cb.forceOpen());
    EXPECT_EQ(ICircuitBreaker::State::CLOSED, cb.getState());
    EXPECT_TRUE(cb.allowRequest());
}

TEST(NoOpCircuitBreakerTest, IsHealthyReturnsTrueWhenClosed) {
    NoOpCircuitBreaker cb;
    EXPECT_TRUE(cb.isHealthy().ok);
}

TEST(NoOpCircuitBreakerTest, LifecycleMethods) {
    NoOpCircuitBreaker cb;
    EXPECT_NO_THROW(cb.flush());
    EXPECT_NO_THROW(cb.shutdown());
}

TEST(NoOpCircuitBreakerTest, StateToStringHelpers) {
    EXPECT_EQ("CLOSED",    ICircuitBreaker::stateToString(ICircuitBreaker::State::CLOSED));
    EXPECT_EQ("OPEN",      ICircuitBreaker::stateToString(ICircuitBreaker::State::OPEN));
    EXPECT_EQ("HALF_OPEN", ICircuitBreaker::stateToString(ICircuitBreaker::State::HALF_OPEN));
}

// ===== ConcernsContext Feature Flags Integration Tests =====

TEST_F(ConcernsContextTest, NoOpContextFeatureFlagsAreDisabled) {
    EXPECT_FALSE(context->featureFlags().isEnabled("any_flag"));
}

TEST_F(ConcernsContextTest, CustomContextWithInMemoryFeatureFlags) {
    auto flags = std::make_unique<InMemoryFeatureFlags>(
        std::unordered_map<std::string, bool>{{"dark_mode", true}, {"beta", false}});
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<NoOpSecrets>(),
        std::move(flags)
    );
    EXPECT_TRUE(ctx->featureFlags().isEnabled("dark_mode"));
    EXPECT_FALSE(ctx->featureFlags().isEnabled("beta"));
    EXPECT_FALSE(ctx->featureFlags().isEnabled("unknown"));
}

TEST_F(ConcernsContextTest, FeatureFlagsMutableViaAccessor) {
    auto flags = std::make_unique<InMemoryFeatureFlags>();
    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<NoOpSecrets>(),
        std::move(flags)
    );
    ctx->featureFlags().setValue("new_feature", true);
    EXPECT_TRUE(ctx->featureFlags().isEnabled("new_feature"));
}

TEST_F(ConcernsContextTest, HealthCheckIncludesFeatureFlags) {
    auto ctx = ConcernsContext::createNoOp();
    auto status = ctx->healthCheck();
    EXPECT_TRUE(status.featureFlags.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(ConcernsContextTest, FlushIncludesFeatureFlags) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->flush());
}

TEST_F(ConcernsContextTest, ShutdownIncludesFeatureFlags) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->shutdown());
}

TEST_F(ConcernsContextTest, ConstFeatureFlagsAccessor) {
    const auto ctx = ConcernsContext::createNoOp();
    EXPECT_FALSE(ctx->featureFlags().isEnabled("f"));
}

// ===== Dynamic Log Level Adjustment Tests (Issue #1412) =====

TEST_F(ConcernsContextTest, SetLogLevelChangesActiveLevel) {
    auto ctx = ConcernsContext::createNoOp();
    ctx->setLogLevel(ILogger::Level::DEBUG);
    EXPECT_EQ(ILogger::Level::DEBUG, ctx->getLogLevel());
}

TEST_F(ConcernsContextTest, SetLogLevelThenRevert) {
    auto ctx = ConcernsContext::createNoOp();
    const auto original = ctx->getLogLevel();
    ctx->setLogLevel(ILogger::Level::WARN);
    EXPECT_EQ(ILogger::Level::WARN, ctx->getLogLevel());
    ctx->setLogLevel(original);
    EXPECT_EQ(original, ctx->getLogLevel());
}

TEST_F(ConcernsContextTest, SetLogLevelAllLevels) {
    auto ctx = ConcernsContext::createNoOp();
    for (auto level : {ILogger::Level::TRACE, ILogger::Level::DEBUG,
                       ILogger::Level::INFO,  ILogger::Level::WARN,
                       ILogger::Level::ERROR, ILogger::Level::CRITICAL}) {
        ctx->setLogLevel(level);
        EXPECT_EQ(level, ctx->getLogLevel());
    }
}

TEST_F(ConcernsContextTest, GetLogLevelMatchesLoggerDirectly) {
    auto ctx = ConcernsContext::createNoOp();
    ctx->setLogLevel(ILogger::Level::ERROR);
    EXPECT_EQ(ctx->logger().getLevel(), ctx->getLogLevel());
}

// ===== IAuditLog / NoOpAuditLog Tests =====

TEST(NoOpAuditLogTest, RecordDoesNotCrash) {
    NoOpAuditLog log;
    EXPECT_NO_THROW(log.record(AuditEvent::success("auth", "user-1", "login", "login")));
}

TEST(NoOpAuditLogTest, LifecycleDoesNotCrash) {
    NoOpAuditLog log;
    EXPECT_NO_THROW(log.flush());
    EXPECT_NO_THROW(log.shutdown());
}

TEST(NoOpAuditLogTest, IsHealthy) {
    NoOpAuditLog log;
    EXPECT_TRUE(log.isHealthy().ok);
}

// ===== InMemoryAuditLog Tests =====

TEST(InMemoryAuditLogTest, RecordAndRetrieve) {
    InMemoryAuditLog log;
    EXPECT_EQ(0u, log.size());

    auto ev = AuditEvent::success("data_access", "alice", "users", "read");
    log.record(ev);
    EXPECT_EQ(1u, log.size());

    auto events = log.getEvents();
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("data_access", events[0].event_type);
    EXPECT_EQ("alice",        events[0].actor);
    EXPECT_EQ("users",        events[0].resource);
    EXPECT_EQ("read",         events[0].action);
    EXPECT_EQ("success",      events[0].outcome);
    EXPECT_GT(events[0].timestamp_ms, 0);
}

TEST(InMemoryAuditLogTest, MultipleEventsPreserveOrder) {
    InMemoryAuditLog log;
    log.record(AuditEvent::success("auth", "u1", "login", "login"));
    log.record(AuditEvent::denied("auth", "u2", "admin", "write"));
    log.record(AuditEvent::error("data_access", "u3", "orders", "delete"));

    auto events = log.getEvents();
    ASSERT_EQ(3u, events.size());
    EXPECT_EQ("success", events[0].outcome);
    EXPECT_EQ("denied",  events[1].outcome);
    EXPECT_EQ("error",   events[2].outcome);
}

TEST(InMemoryAuditLogTest, ClearRemovesAllEvents) {
    InMemoryAuditLog log;
    log.record(AuditEvent::success("auth", "u1", "login", "login"));
    log.record(AuditEvent::success("auth", "u2", "login", "login"));
    EXPECT_EQ(2u, log.size());
    log.clear();
    EXPECT_EQ(0u, log.size());
    EXPECT_TRUE(log.getEvents().empty());
}

TEST(InMemoryAuditLogTest, ShutdownClearsEvents) {
    InMemoryAuditLog log;
    log.record(AuditEvent::success("auth", "u1", "login", "login"));
    log.shutdown();
    EXPECT_EQ(0u, log.size());
}

TEST(InMemoryAuditLogTest, IsHealthy) {
    InMemoryAuditLog log;
    EXPECT_TRUE(log.isHealthy().ok);
}

TEST(InMemoryAuditLogTest, GetEventsReturnsSnapshot) {
    InMemoryAuditLog log;
    log.record(AuditEvent::success("auth", "u1", "login", "login"));
    auto snap1 = log.getEvents();
    log.record(AuditEvent::success("auth", "u2", "login", "login"));
    auto snap2 = log.getEvents();
    // snap1 is a copy, unaffected by the second record
    EXPECT_EQ(1u, snap1.size());
    EXPECT_EQ(2u, snap2.size());
}

TEST(AuditEventTest, MakeFactoryPopulatesAllFields) {
    auto ev = AuditEvent::make(
        "config_change", "admin", "config/log_level", "write", "success",
        {{"old_value", "info"}, {"new_value", "debug"}});
    EXPECT_EQ("config_change",       ev.event_type);
    EXPECT_EQ("admin",               ev.actor);
    EXPECT_EQ("config/log_level",    ev.resource);
    EXPECT_EQ("write",               ev.action);
    EXPECT_EQ("success",             ev.outcome);
    EXPECT_GT(ev.timestamp_ms, 0);
    EXPECT_EQ("info",  ev.details.at("old_value"));
    EXPECT_EQ("debug", ev.details.at("new_value"));
}

TEST(AuditEventTest, FactoryMethodsSetCorrectOutcome) {
    auto ok  = AuditEvent::success("auth", "u", "r", "login");
    auto den = AuditEvent::denied("auth",  "u", "r", "write");
    auto err = AuditEvent::error("data",   "u", "r", "delete");
    EXPECT_EQ("success", ok.outcome);
    EXPECT_EQ("denied",  den.outcome);
    EXPECT_EQ("error",   err.outcome);
}

// ===== ConcernsContext Audit Log Integration Tests =====

TEST_F(ConcernsContextTest, NoOpContextAuditLogDiscards) {
    // createNoOp() should install a NoOpAuditLog — record() must not crash
    EXPECT_NO_THROW(context->auditLog().record(
        AuditEvent::success("test", "u", "r", "read")));
}

TEST_F(ConcernsContextTest, ConstAuditLogAccessor) {
    const auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->auditLog().isHealthy());
}

TEST_F(ConcernsContextTest, CustomContextWithInMemoryAuditLog) {
    auto audit = std::make_unique<InMemoryAuditLog>();
    auto* audit_ptr = audit.get();

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<NoOpSecrets>(),
        std::make_unique<NoOpFeatureFlags>(),
        std::move(audit)
    );

    ctx->auditLog().record(AuditEvent::success("auth", "alice", "login", "login"));
    ctx->auditLog().record(AuditEvent::denied("data_access", "bob", "payroll", "read"));

    EXPECT_EQ(2u, audit_ptr->size());
    auto events = audit_ptr->getEvents();
    EXPECT_EQ("alice", events[0].actor);
    EXPECT_EQ("bob",   events[1].actor);
}

TEST_F(ConcernsContextTest, HealthCheckIncludesAuditLog) {
    auto ctx = ConcernsContext::createNoOp();
    auto status = ctx->healthCheck();
    EXPECT_TRUE(status.auditLog.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(ConcernsContextTest, UnhealthyAuditLogMarksContextUnhealthy) {
    class UnhealthyAuditLog : public IAuditLog {
    public:
        void record(const AuditEvent&) noexcept override {}
        ProbeResult isHealthy() const override {
            return ProbeResult::unhealthy("audit backend unreachable");
        }
    };

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<NoOpSecrets>(),
        std::make_unique<NoOpFeatureFlags>(),
        std::make_unique<UnhealthyAuditLog>()
    );

    auto status = ctx->healthCheck();
    EXPECT_FALSE(status.auditLog.ok);
    EXPECT_EQ("audit backend unreachable", status.auditLog.message);
    EXPECT_FALSE(status.isHealthy());
}

TEST_F(ConcernsContextTest, FlushIncludesAuditLog) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->flush());
}

TEST_F(ConcernsContextTest, ShutdownIncludesAuditLog) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->shutdown());
}

TEST_F(ConcernsContextTest, ConfigDrivenInMemoryAuditLogSelection) {
    ConcernsContext::Config cfg;
    cfg.auditAdapter   = "inmemory";
    cfg.tracingEnabled = false;
    cfg.metricsEnabled = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    EXPECT_NO_THROW(ctx->auditLog().record(
        AuditEvent::success("test", "u", "r", "read")));
    EXPECT_TRUE(ctx->auditLog().isHealthy().ok);
}

TEST_F(ConcernsContextTest, ConfigDrivenNoopAuditLogSelection) {
    ConcernsContext::Config cfg;
    cfg.auditAdapter   = "noop";
    cfg.tracingEnabled = false;
    cfg.metricsEnabled = false;

    std::shared_ptr<ConcernsContext> ctx;
    ASSERT_NO_THROW(ctx = ConcernsContext::create(cfg));
    ASSERT_NE(nullptr, ctx);
    EXPECT_NO_THROW(ctx->auditLog().record(
        AuditEvent::success("test", "u", "r", "read")));
}

TEST_F(ConcernsContextTest, ConfigAdapterUnknownAuditAdapterIsInvalid) {
    auto result = themis::core::ConfigValidator::validateAdapterConfig(
        "spdlog", "", "", "inmemory", "default", "inmemory", "kafka");
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(1u, result.errors.size());
    EXPECT_NE(std::string::npos, result.errors[0].find("auditAdapter"));
    EXPECT_NE(std::string::npos, result.errors[0].find("kafka"));
}

TEST(InMemoryAuditLogTest, ThreadSafetyUnderConcurrentRecord) {
    InMemoryAuditLog log;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&log, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                log.record(AuditEvent::success(
                    "test", "actor-" + std::to_string(t),
                    "resource", "read"));
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(static_cast<size_t>(kThreads * kPerThread), log.size());
}

// ===========================================================================
// DAR: Dynamic Adapter Reconfiguration tests
//
//  DAR-01  replaceLogger() swaps adapter; new adapter receives subsequent calls
//  DAR-02  replaceTracer() swaps adapter
//  DAR-03  replaceMetrics() swaps adapter
//  DAR-04  replaceCache() swaps adapter
//  DAR-05  replaceLogger(nullptr) throws std::invalid_argument
//  DAR-06  replaceLogger() is safe to call from a concurrent thread while
//          another thread is actively logging
// ===========================================================================

namespace {

/// Minimal spy logger that counts how many times info() is called.
class SpyLogger : public ILogger {
public:
    std::atomic<int> call_count{0};

    void log(Level, const std::string&) override {}
    void trace(const std::string&) override {}
    void debug(const std::string&) override {}
    void info(const std::string&) override { ++call_count; }
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
    void critical(const std::string&) override {}
    void logStructured(Level, const std::string&, const Fields& = {}) override {}
    void logWithContext(Level, const std::string&,
                        const TraceContext&, const Fields&) override {}
    void setLevel(Level) override {}
    Level getLevel() const override { return Level::TRACE; }
    void setPattern(const std::string&) override {}
    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }
};

/// Minimal spy metrics that counts how many times incrementCounter() is called.
class SpyMetrics : public IMetrics {
public:
    std::atomic<int> inc_count{0};

    void incrementCounter(const std::string&, int64_t = 1, const Labels& = {}) override { ++inc_count; }
    void setGauge(const std::string&, double, const Labels& = {}) override {}
    void incrementGauge(const std::string&, double, const Labels& = {}) override {}
    void decrementGauge(const std::string&, double, const Labels& = {}) override {}
    void observeHistogram(const std::string&, double, const Labels& = {}) override {}
    void recordLatency(const std::string&, double, const Labels& = {}) override {}
    void recordError(const std::string&, const Labels& = {}) override {}
    void recordSuccess(const std::string&, const Labels& = {}) override {}
    std::string exportMetrics() const override { return ""; }
    void reset() override {}
    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }
};

} // anonymous namespace

// DAR-01: replaceLogger() installs the new adapter
TEST_F(ConcernsContextTest, DAR_01_ReplaceLogger_SwapsAdapter) {
    auto spy = std::make_unique<SpyLogger>();
    auto* spy_ptr = spy.get();

    context->replaceLogger(std::move(spy));

    context->logger().info("after swap");
    EXPECT_EQ(1, spy_ptr->call_count.load())
        << "Calls after swap must be delivered to the new adapter";
}

// DAR-02: replaceTracer() installs the new adapter
TEST_F(ConcernsContextTest, DAR_02_ReplaceTracer_SwapsAdapter) {
    // NoOp tracer; just verify the swap does not crash and the new adapter
    // is active (startSpan returns non-null).
    context->replaceTracer(std::make_unique<NoOpTracer>());
    auto span = context->startSpan("after-tracer-swap");
    EXPECT_NE(nullptr, span.get());
}

// DAR-03: replaceMetrics() installs the new adapter
TEST_F(ConcernsContextTest, DAR_03_ReplaceMetrics_SwapsAdapter) {
    auto spy = std::make_unique<SpyMetrics>();
    auto* spy_ptr = spy.get();

    context->replaceMetrics(std::move(spy));

    context->metrics().incrementCounter("test_counter");
    EXPECT_EQ(1, spy_ptr->inc_count.load())
        << "Calls after swap must be delivered to the new metrics adapter";
}

// DAR-04: replaceCache() installs the new adapter
TEST_F(ConcernsContextTest, DAR_04_ReplaceCache_SwapsAdapter) {
    context->replaceCache(std::make_unique<NoOpCache>());
    // NoOp cache put/get must not crash.
    using Entry = CacheEntry;
    context->cache().put("key", Entry{"value"});
    auto val = context->cache().get("key");
    (void)val; // NoOp always returns nullopt
}

// DAR-05: replaceLogger(nullptr) throws std::invalid_argument
TEST_F(ConcernsContextTest, DAR_05_ReplaceLogger_NullptrThrows) {
    EXPECT_THROW(context->replaceLogger(nullptr), std::invalid_argument);
}

// DAR-06: replaceLogger() is race-free when called concurrently with logging
TEST_F(ConcernsContextTest, DAR_06_ReplaceLogger_ConcurrentSafe) {
    constexpr int kLogThreads  = 4;
    constexpr int kLogsPerThread = 200;

    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    threads.reserve(kLogThreads + 1);

    // Logging threads
    for (int t = 0; t < kLogThreads; ++t) {
        threads.emplace_back([&] {
            while (!start.load()) { std::this_thread::yield(); }
            for (int i = 0; i < kLogsPerThread; ++i) {
                context->logger().info("concurrent log");
            }
        });
    }

    // Adapter-swapper thread
    threads.emplace_back([&] {
        while (!start.load()) { std::this_thread::yield(); }
        for (int i = 0; i < 10; ++i) {
            context->replaceLogger(std::make_unique<NoOpLogger>());
        }
    });

    start.store(true);
    for (auto& th : threads) th.join();

    // If we reach here without ASAN/TSAN error the test passes.
    SUCCEED();
}
