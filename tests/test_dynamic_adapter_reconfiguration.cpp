/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_dynamic_adapter_reconfiguration.cpp           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-13                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Tests:  Dynamic Adapter Reconfiguration — Issue #63 / v1.6.0        ║
  Suite:  DynamicAdapterReconfigurationFocusedTests                   ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/lifecycle.h"
#include "core/config_validator.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <latch>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::core::concerns;

// ============================================================================
// Test helpers — instrumented adapters that record calls
// ============================================================================

/// Logger that records info() calls and exposes them for assertions.
class TrackingLogger : public ILogger {
public:
    explicit TrackingLogger(const std::string& id = "default") : id_(id) {}

    void log(Level /*level*/, const std::string& msg) override {
        last_message_ = msg;
        call_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void trace(const std::string& m)    override { log(Level::TRACE, m); }
    void debug(const std::string& m)    override { log(Level::DEBUG, m); }
    void info(const std::string& m)     override { log(Level::INFO,  m); }
    void warn(const std::string& m)     override { log(Level::WARN,  m); }
    void error(const std::string& m)    override { log(Level::ERROR, m); }
    void critical(const std::string& m) override { log(Level::CRITICAL, m); }

    void logStructured(Level l, const std::string& m, const Fields&) override { log(l, m); }
    void logWithContext(Level l, const std::string& m, const TraceContext&, const Fields&) override { log(l, m); }

    void setLevel(Level lvl) override { level_ = lvl; }
    Level getLevel() const override   { return level_; }
    void setPattern(const std::string&) override {}

    void flush()    noexcept override { flushed_ = true; }
    void shutdown() noexcept override { shut_down_ = true; }
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    const std::string& id()           const { return id_; }
    const std::string& lastMessage()  const { return last_message_; }
    int  callCount() const { return call_count_.load(std::memory_order_relaxed); }
    bool wasFlushed()    const { return flushed_; }
    bool wasShutDown()   const { return shut_down_; }

private:
    std::string id_;
    std::string last_message_;
    std::atomic<int> call_count_{0};
    Level level_ = Level::INFO;
    bool flushed_   = false;
    bool shut_down_ = false;
};

/// Tracer that records span creation and exposes them for assertions.
class TrackingTracer : public ITracer {
public:
    explicit TrackingTracer(const std::string& id = "default") : id_(id) {}

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        last_span_name_ = name;
        span_count_.fetch_add(1, std::memory_order_relaxed);
        return std::make_unique<NoOpSpan>();
    }
    std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan&) override {
        return startSpan(name);
    }
    std::unique_ptr<ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>&) override {
        return startSpan(name);
    }
    void injectContext(std::map<std::string, std::string>&) override {}

    void initialize(const std::string&, const std::string&) override {}
    void shutdown()         override { shut_down_ = true; }
    bool isInitialized()    const override { return true; }
    void flush()            override { flushed_ = true; }
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    const std::string& id()           const { return id_; }
    const std::string& lastSpanName() const { return last_span_name_; }
    int  spanCount() const { return span_count_.load(std::memory_order_relaxed); }
    bool wasFlushed()    const { return flushed_; }
    bool wasShutDown()   const { return shut_down_; }

private:
    // Minimal no-op span used internally by TrackingTracer
    struct NoOpSpan : public ISpan {
        void setAttribute(const std::string&, const std::string&) override {}
        void setAttribute(const std::string&, int64_t)            override {}
        void setAttribute(const std::string&, double)             override {}
        void setAttribute(const std::string&, bool)               override {}
        void recordError(const std::string&)                      override {}
        void setStatus(bool, const std::string&)                  override {}
        void end()                                                override {}
        bool isValid() const                                      override { return false; }
    };

    std::string id_;
    std::string last_span_name_;
    std::atomic<int> span_count_{0};
    bool flushed_   = false;
    bool shut_down_ = false;
};

/// Metrics adapter that records observations for assertions.
class TrackingMetrics : public IMetrics {
public:
    explicit TrackingMetrics(const std::string& id = "default") : id_(id) {}

    void incrementCounter(const std::string&, double, const Labels&) override {
        counter_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void setGauge(const std::string&, double, const Labels&)       override {}
    void incrementGauge(const std::string&, double, const Labels&) override {}
    void decrementGauge(const std::string&, double, const Labels&) override {}
    void observeHistogram(const std::string&, double, const Labels&) override {
        histogram_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void recordLatency(const std::string&, double, const Labels&)  override {}
    void recordError(const std::string&, const Labels&)            override {}
    void recordSuccess(const std::string&, const Labels&)          override {}
    std::string exportMetrics() const override { return ""; }
    void reset()                  override {}
    void flush()   noexcept override { flushed_ = true; }
    void shutdown() noexcept override { shut_down_ = true; }
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    const std::string& id()     const { return id_; }
    int  counterCount()   const { return counter_count_.load(std::memory_order_relaxed); }
    int  histogramCount() const { return histogram_count_.load(std::memory_order_relaxed); }
    bool wasFlushed()   const { return flushed_; }
    bool wasShutDown()  const { return shut_down_; }

private:
    std::string id_;
    std::atomic<int> counter_count_{0};
    std::atomic<int> histogram_count_{0};
    bool flushed_   = false;
    bool shut_down_ = false;
};

// ============================================================================
// Test fixture
// ============================================================================

class DynamicAdapterReconfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        ctx_ = ConcernsContext::createNoOp();
    }

    std::shared_ptr<ConcernsContext> ctx_;
};

// ============================================================================
// AC-1: Zero-downtime logging level changes
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, SetLogLevelChangesLevelWithoutRestart) {
    ctx_->setLogLevel(ILogger::Level::DEBUG);
    EXPECT_EQ(ILogger::Level::DEBUG, ctx_->getLogLevel());
}

TEST_F(DynamicAdapterReconfigurationTest, SetLogLevelAllLevels) {
    const std::vector<ILogger::Level> levels = {
        ILogger::Level::TRACE, ILogger::Level::DEBUG, ILogger::Level::INFO,
        ILogger::Level::WARN,  ILogger::Level::ERROR, ILogger::Level::CRITICAL
    };
    for (auto lvl : levels) {
        ctx_->setLogLevel(lvl);
        EXPECT_EQ(lvl, ctx_->getLogLevel());
    }
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceLoggerZeroDowntime) {
    auto* old_logger = dynamic_cast<TrackingLogger*>(&ctx_->logger());
    // Install a tracking logger and use it
    auto new_logger = std::make_unique<TrackingLogger>("v2");
    auto* raw = new_logger.get();
    ctx_->replaceLogger(std::move(new_logger));

    // Logging continues immediately via the new adapter — no restart needed
    ctx_->logInfo("hello after swap");
    EXPECT_EQ(1, raw->callCount());
    EXPECT_EQ("hello after swap", raw->lastMessage());
    (void)old_logger;
}

TEST_F(DynamicAdapterReconfigurationTest, LogLevelSurvivesReplace) {
    ctx_->setLogLevel(ILogger::Level::WARN);
    // Replace with a new adapter at a different level
    auto new_logger = std::make_unique<TrackingLogger>();
    new_logger->setLevel(ILogger::Level::ERROR);
    ctx_->replaceLogger(std::move(new_logger));
    // The new adapter's level is independently authoritative
    EXPECT_EQ(ILogger::Level::ERROR, ctx_->getLogLevel());
}

// ============================================================================
// AC-2: Switch between tracing backends without restart
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, ReplaceTracerSwitchesBackend) {
    auto new_tracer = std::make_unique<TrackingTracer>("jaeger-sim");
    auto* raw = new_tracer.get();

    ctx_->replaceTracer(std::move(new_tracer));

    auto span = ctx_->startSpan("my_op");
    EXPECT_EQ(1, raw->spanCount());
    EXPECT_EQ("my_op", raw->lastSpanName());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceTracerMultipleTimes) {
    for (int i = 0; i < 5; ++i) {
        ctx_->replaceTracer(std::make_unique<TrackingTracer>("t" + std::to_string(i)));
    }
    auto span = ctx_->startSpan("after_multiple_replaces");
    EXPECT_NE(nullptr, span.get());
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadConfigSwitchesTracerToNoop) {
    ConcernsContext::Config cfg;
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    cfg.loggerAdapter  = "noop";
    ASSERT_NO_THROW(ctx_->reloadConfig(cfg));
    // After reload with noop tracer, spans are still created (no crash)
    EXPECT_NO_THROW(ctx_->startSpan("after_reload"));
}

// ============================================================================
// AC-3: Enable/disable metrics dynamically
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, ReplaceMetricsEnablesMetrics) {
    auto new_metrics = std::make_unique<TrackingMetrics>("prom-sim");
    auto* raw = new_metrics.get();

    ctx_->replaceMetrics(std::move(new_metrics));

    ctx_->metrics().incrementCounter("requests_total", 1.0);
    EXPECT_EQ(1, raw->counterCount());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceMetricsDisablesMetrics) {
    // Start with tracking metrics
    auto tracking = std::make_unique<TrackingMetrics>("prom-sim");
    ctx_->replaceMetrics(std::move(tracking));

    // Switch to noop (disabling metrics)
    ctx_->replaceMetrics(std::make_unique<NoOpMetrics>());

    // Confirm we can still call metrics without crash
    EXPECT_NO_THROW(ctx_->metrics().incrementCounter("requests_total", 1.0));
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadMetricsConfigToNoop) {
    ConcernsContext::Config cfg;
    cfg.metricsEnabled = false;
    cfg.metricsAdapter = "noop";
    ASSERT_NO_THROW(ctx_->reloadMetricsConfig(cfg));
    EXPECT_NO_THROW(ctx_->metrics().incrementCounter("x", 1.0));
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadMetricsConfigToPrometheus) {
    ConcernsContext::Config cfg;
    cfg.metricsEnabled = true;
    cfg.metricsAdapter = "prometheus";
    // Should not throw — creates a fresh PrometheusMetricsAdapter
    ASSERT_NO_THROW(ctx_->reloadMetricsConfig(cfg));
    EXPECT_TRUE(ctx_->metrics().isHealthy().ok);
}

// ============================================================================
// AC-4: Thread-safe adapter swapping
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, ConcurrentReplaceLoggerNoRaceOrCrash) {
    constexpr int kReplacers = 4;
    constexpr int kIterations = 50;
    std::latch start_gate{kReplacers};

    std::vector<std::thread> threads;
    threads.reserve(kReplacers);
    for (int i = 0; i < kReplacers; ++i) {
        threads.emplace_back([&, i]() {
            start_gate.arrive_and_wait();
            for (int j = 0; j < kIterations; ++j) {
                ctx_->replaceLogger(
                    std::make_unique<TrackingLogger>("t" + std::to_string(i)));
            }
        });
    }
    for (auto& t : threads) t.join();
    // After concurrent replaces, the logger must still be functional
    EXPECT_NO_THROW(ctx_->logInfo("post-concurrent-replace"));
}

TEST_F(DynamicAdapterReconfigurationTest, ConcurrentReplaceTracerNoRaceOrCrash) {
    constexpr int kReplacers = 4;
    constexpr int kIterations = 50;
    std::latch start_gate{kReplacers};

    std::vector<std::thread> threads;
    threads.reserve(kReplacers);
    for (int i = 0; i < kReplacers; ++i) {
        threads.emplace_back([&, i]() {
            start_gate.arrive_and_wait();
            for (int j = 0; j < kIterations; ++j) {
                ctx_->replaceTracer(
                    std::make_unique<TrackingTracer>("t" + std::to_string(i)));
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_NO_THROW(ctx_->startSpan("post-concurrent-replace"));
}

TEST_F(DynamicAdapterReconfigurationTest, ConcurrentReplaceMetricsNoRaceOrCrash) {
    constexpr int kReplacers = 4;
    constexpr int kIterations = 50;
    std::latch start_gate{kReplacers};

    std::vector<std::thread> threads;
    threads.reserve(kReplacers);
    for (int i = 0; i < kReplacers; ++i) {
        threads.emplace_back([&, i]() {
            start_gate.arrive_and_wait();
            for (int j = 0; j < kIterations; ++j) {
                ctx_->replaceMetrics(
                    std::make_unique<TrackingMetrics>("t" + std::to_string(i)));
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_NO_THROW(ctx_->recordMetric("post-replace", 1.0));
}

TEST_F(DynamicAdapterReconfigurationTest,
       ConcurrentReadersAndReplacerNoRaceOrCrash) {
    // Reader threads and one replacer thread run simultaneously
    constexpr int kReaders   = 4;
    constexpr int kIterations = 200;
    std::atomic<bool> stop_readers{false};
    std::latch start_gate{kReaders + 1};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            start_gate.arrive_and_wait();
            while (!stop_readers.load(std::memory_order_relaxed)) {
                ctx_->logInfo("concurrent read");
            }
        });
    }

    // Replacer thread
    std::thread replacer([&]() {
        start_gate.arrive_and_wait();
        for (int j = 0; j < kIterations; ++j) {
            ctx_->replaceLogger(std::make_unique<TrackingLogger>());
            std::this_thread::yield();
        }
        stop_readers.store(true, std::memory_order_relaxed);
    });

    replacer.join();
    for (auto& r : readers) r.join();

    // Context must remain usable after the storm
    EXPECT_NO_THROW(ctx_->logInfo("after storm"));
}

// ============================================================================
// AC-5: Graceful handling of in-flight operations
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, ReplaceLoggerFlushesOldAdapter) {
    auto old_logger = std::make_unique<TrackingLogger>("old");
    auto* raw_old = old_logger.get();
    ctx_->replaceLogger(std::move(old_logger));  // install old

    // Now replace again — old must have been flushed before discard
    ctx_->replaceLogger(std::make_unique<NoOpLogger>());
    EXPECT_TRUE(raw_old->wasFlushed());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceLoggerShutsDownOldAdapter) {
    auto old_logger = std::make_unique<TrackingLogger>("old");
    auto* raw_old = old_logger.get();
    ctx_->replaceLogger(std::move(old_logger));

    ctx_->replaceLogger(std::make_unique<NoOpLogger>());
    EXPECT_TRUE(raw_old->wasShutDown());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceTracerFlushesOldAdapter) {
    auto old_tracer = std::make_unique<TrackingTracer>("old");
    auto* raw_old = old_tracer.get();
    ctx_->replaceTracer(std::move(old_tracer));

    ctx_->replaceTracer(std::make_unique<NoOpTracer>());
    EXPECT_TRUE(raw_old->wasFlushed());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceTracerShutsDownOldAdapter) {
    auto old_tracer = std::make_unique<TrackingTracer>("old");
    auto* raw_old = old_tracer.get();
    ctx_->replaceTracer(std::move(old_tracer));

    ctx_->replaceTracer(std::make_unique<NoOpTracer>());
    EXPECT_TRUE(raw_old->wasShutDown());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceMetricsFlushesOldAdapter) {
    auto old_metrics = std::make_unique<TrackingMetrics>("old");
    auto* raw_old = old_metrics.get();
    ctx_->replaceMetrics(std::move(old_metrics));

    ctx_->replaceMetrics(std::make_unique<NoOpMetrics>());
    EXPECT_TRUE(raw_old->wasFlushed());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceMetricsShutsDownOldAdapter) {
    auto old_metrics = std::make_unique<TrackingMetrics>("old");
    auto* raw_old = old_metrics.get();
    ctx_->replaceMetrics(std::move(old_metrics));

    ctx_->replaceMetrics(std::make_unique<NoOpMetrics>());
    EXPECT_TRUE(raw_old->wasShutDown());
}

TEST_F(DynamicAdapterReconfigurationTest, LoggerHandleKeepsOldAdapterAlive) {
    auto logger_v1 = std::make_unique<TrackingLogger>("v1");
    ctx_->replaceLogger(std::move(logger_v1));

    // Obtain a shared_ptr handle to the current logger
    auto handle = ctx_->loggerHandle();
    ASSERT_NE(nullptr, handle.get());

    // Replace with a new adapter
    ctx_->replaceLogger(std::make_unique<TrackingLogger>("v2"));

    // The handle still refers to v1 — it must be usable
    EXPECT_EQ("v1", dynamic_cast<TrackingLogger*>(handle.get())->id());
    EXPECT_NO_THROW(handle->info("still alive"));
}

TEST_F(DynamicAdapterReconfigurationTest, TracerHandleKeepsOldAdapterAlive) {
    auto tracer_v1 = std::make_unique<TrackingTracer>("v1");
    ctx_->replaceTracer(std::move(tracer_v1));

    auto handle = ctx_->tracerHandle();
    ASSERT_NE(nullptr, handle.get());

    ctx_->replaceTracer(std::make_unique<TrackingTracer>("v2"));

    EXPECT_EQ("v1", dynamic_cast<TrackingTracer*>(handle.get())->id());
    EXPECT_NO_THROW(handle->startSpan("alive"));
}

TEST_F(DynamicAdapterReconfigurationTest, MetricsHandleKeepsOldAdapterAlive) {
    auto metrics_v1 = std::make_unique<TrackingMetrics>("v1");
    ctx_->replaceMetrics(std::move(metrics_v1));

    auto handle = ctx_->metricsHandle();
    ASSERT_NE(nullptr, handle.get());

    ctx_->replaceMetrics(std::make_unique<TrackingMetrics>("v2"));

    EXPECT_EQ("v1", dynamic_cast<TrackingMetrics*>(handle.get())->id());
    EXPECT_NO_THROW(handle->incrementCounter("x", 1.0));
}

// ============================================================================
// AC-6: Configuration validation before swap
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, ReplaceLoggerNullThrows) {
    EXPECT_THROW(ctx_->replaceLogger(nullptr), std::invalid_argument);
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceTracerNullThrows) {
    EXPECT_THROW(ctx_->replaceTracer(nullptr), std::invalid_argument);
}

TEST_F(DynamicAdapterReconfigurationTest, ReplaceMetricsNullThrows) {
    EXPECT_THROW(ctx_->replaceMetrics(nullptr), std::invalid_argument);
}

TEST_F(DynamicAdapterReconfigurationTest,
       ReloadMetricsConfigInvalidAdapterThrows) {
    ConcernsContext::Config cfg;
    cfg.metricsAdapter = "unknown_backend";
    EXPECT_THROW(ctx_->reloadMetricsConfig(cfg), std::runtime_error);
}

TEST_F(DynamicAdapterReconfigurationTest,
       ReloadMetricsConfigInvalidAdapterLeavesAdapterUnchanged) {
    // Install a tracking metrics adapter first
    auto tracking = std::make_unique<TrackingMetrics>("sentinel");
    auto* raw = tracking.get();
    ctx_->replaceMetrics(std::move(tracking));

    // Attempt an invalid reload — must leave the sentinel in place
    ConcernsContext::Config bad_cfg;
    bad_cfg.metricsAdapter = "unknown_backend";
    ASSERT_THROW(ctx_->reloadMetricsConfig(bad_cfg), std::runtime_error);

    // The sentinel is still the active adapter
    ctx_->metrics().incrementCounter("still_active", 1.0);
    EXPECT_EQ(1, raw->counterCount());
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadConfigInvalidLogLevelThrows) {
    ConcernsContext::Config cfg;
    cfg.logLevel = "not_a_level";
    EXPECT_THROW(ctx_->reloadConfig(cfg), std::runtime_error);
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadConfigInvalidTracerAdapterThrows) {
    ConcernsContext::Config cfg;
    cfg.tracerAdapter = "unknown_tracer";
    EXPECT_THROW(ctx_->reloadConfig(cfg), std::runtime_error);
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadConfigInvalidLoggerAdapterThrows) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter = "unknown_logger";
    EXPECT_THROW(ctx_->reloadConfig(cfg), std::runtime_error);
}

TEST_F(DynamicAdapterReconfigurationTest,
       ReloadConfigValidationFailureLeavesAdaptersUnchanged) {
    // Install tracking adapters so we can verify they're still in place
    auto logger  = std::make_unique<TrackingLogger>("sentinel-l");
    auto tracer  = std::make_unique<TrackingTracer>("sentinel-t");
    auto metrics = std::make_unique<TrackingMetrics>("sentinel-m");
    auto* l_raw  = logger.get();
    auto* t_raw  = tracer.get();
    auto* m_raw  = metrics.get();
    ctx_->replaceLogger(std::move(logger));
    ctx_->replaceTracer(std::move(tracer));
    ctx_->replaceMetrics(std::move(metrics));

    // Invalid config — validation must fail before any adapter is replaced
    ConcernsContext::Config bad_cfg;
    bad_cfg.logLevel = "not_a_level";
    ASSERT_THROW(ctx_->reloadConfig(bad_cfg), std::runtime_error);

    // Sentinels are still active
    ctx_->logInfo("still using sentinel");
    EXPECT_EQ(1, l_raw->callCount());
    ctx_->startSpan("still using sentinel");
    EXPECT_EQ(1, t_raw->spanCount());
    ctx_->recordMetric("still_active", 1.0);
    EXPECT_EQ(1, m_raw->histogramCount());
}

TEST_F(DynamicAdapterReconfigurationTest, ReloadConfigValidSucceeds) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter  = "noop";
    cfg.tracerAdapter  = "noop";
    cfg.metricsAdapter = "noop";
    ASSERT_NO_THROW(ctx_->reloadConfig(cfg));

    // All three adapters must be functional
    EXPECT_NO_THROW(ctx_->logInfo("after reload"));
    EXPECT_NO_THROW(ctx_->startSpan("after reload"));
    EXPECT_NO_THROW(ctx_->recordMetric("after_reload", 1.0));
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(DynamicAdapterReconfigurationTest, HealthCheckAfterReplaceReflectsNewAdapter) {
    ctx_->replaceLogger(std::make_unique<TrackingLogger>());
    ctx_->replaceTracer(std::make_unique<TrackingTracer>());
    ctx_->replaceMetrics(std::make_unique<TrackingMetrics>());

    auto status = ctx_->healthCheck();
    EXPECT_TRUE(status.logger.ok);
    EXPECT_TRUE(status.tracer.ok);
    EXPECT_TRUE(status.metrics.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST_F(DynamicAdapterReconfigurationTest, FlushAfterReplaceFlushesNewAdapter) {
    auto new_logger = std::make_unique<TrackingLogger>("new");
    auto* raw = new_logger.get();
    ctx_->replaceLogger(std::move(new_logger));

    ctx_->flush();
    EXPECT_TRUE(raw->wasFlushed());
}

TEST_F(DynamicAdapterReconfigurationTest, ShutdownAfterReplaceShutdownNewAdapter) {
    auto new_tracer = std::make_unique<TrackingTracer>("new");
    auto* raw = new_tracer.get();
    ctx_->replaceTracer(std::move(new_tracer));

    ctx_->shutdown();
    EXPECT_TRUE(raw->wasShutDown());
}

TEST_F(DynamicAdapterReconfigurationTest, ReplacePreservesOtherConcerns) {
    // Replacing the logger must not disturb cache or featureFlags
    ctx_->replaceLogger(std::make_unique<TrackingLogger>());

    EXPECT_NO_THROW(ctx_->cache().put("key", "value", 0));
    EXPECT_NO_THROW(ctx_->featureFlags().isEnabled("any_flag"));
}
