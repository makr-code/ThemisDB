/**
 * @file bench_di_logging.cpp
 * @brief Performance benchmarks for DI overhead and logging throughput (Issue #1420).
 *
 * Measures:
 *   1. ConcernsContext creation cost (NoOp and custom variants)
 *   2. NoOpLogger throughput (baseline — zero-overhead floor)
 *   3. SpdlogLoggerAdapter throughput with null sink (plain-text and JSON modes)
 *   4. Structured log (logStructured) throughput with field maps
 *   5. logWithContext throughput (trace/span ID injection)
 *   6. ConcernsContext convenience logging wrappers (logInfo / logError)
 *   7. NoOpMetrics counter/histogram overhead
 *   8. NoOpCache get/put overhead via ConcernsContext (dispatch-only baseline)
 *   9. InMemoryCacheImpl get-hit, get-miss, and put throughput (real LRU)
 */

#include <benchmark/benchmark.h>

#include "core/concerns/i_logger.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/concerns_context.h"
#include "core/concerns/inmemory_cache_impl.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

#include <memory>
#include <string>

using namespace themis::core::concerns;

// ============================================================================
// Utilities
// ============================================================================

/// Build a SpdlogLoggerAdapter backed by a null sink (no I/O, measures pure
/// formatting and dispatch overhead).
static std::unique_ptr<SpdlogLoggerAdapter> makeNullSpdlogAdapter(bool json_mode = false) {
    auto sink   = std::make_shared<spdlog::sinks::null_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("bench_null", sink);
    logger->set_level(spdlog::level::trace);
    return std::make_unique<SpdlogLoggerAdapter>(logger, json_mode);
}

// ============================================================================
// 1. ConcernsContext creation overhead
// ============================================================================

static void BM_ConcernsContext_CreateNoOp(benchmark::State& state) {
    for (auto _ : state) {
        auto ctx = ConcernsContext::createNoOp();
        benchmark::DoNotOptimize(ctx);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcernsContext_CreateNoOp);

static void BM_ConcernsContext_CreateCustom(benchmark::State& state) {
    for (auto _ : state) {
        auto ctx = ConcernsContext::createCustom(
            std::make_unique<NoOpLogger>(),
            std::make_unique<NoOpTracer>(),
            std::make_unique<NoOpMetrics>(),
            std::make_unique<NoOpCache>(),
            std::make_unique<NoOpCircuitBreaker>()
        );
        benchmark::DoNotOptimize(ctx);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcernsContext_CreateCustom);

// ============================================================================
// 2. NoOpLogger throughput (zero-overhead baseline)
// ============================================================================

static void BM_NoOpLogger_Info(benchmark::State& state) {
    NoOpLogger logger;
    const std::string msg = "benchmark log message with some content";
    for (auto _ : state) {
        logger.info(msg);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NoOpLogger_Info);

static void BM_NoOpLogger_LogStructured(benchmark::State& state) {
    NoOpLogger logger;
    const std::string msg = "structured benchmark event";
    const ILogger::Fields fields = {{"component", "db"}, {"op", "insert"}, {"latency_ms", "2"}};
    for (auto _ : state) {
        logger.logStructured(ILogger::Level::INFO, msg, fields);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NoOpLogger_LogStructured);

// ============================================================================
// 3. SpdlogLoggerAdapter throughput (null sink, plain-text mode)
// ============================================================================

static void BM_SpdlogAdapter_Info_PlainText(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/false);
    const std::string msg = "benchmark log message with some content";
    for (auto _ : state) {
        logger->info(msg);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_Info_PlainText);

static void BM_SpdlogAdapter_Info_JsonMode(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/true);
    const std::string msg = "benchmark log message with some content";
    for (auto _ : state) {
        logger->info(msg);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_Info_JsonMode);

// ============================================================================
// 4. Structured log (logStructured) throughput
// ============================================================================

static void BM_SpdlogAdapter_LogStructured_PlainText(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/false);
    const std::string msg = "structured benchmark event";
    const ILogger::Fields fields = {{"component", "db"}, {"op", "insert"}, {"latency_ms", "2"}};
    for (auto _ : state) {
        logger->logStructured(ILogger::Level::INFO, msg, fields);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_LogStructured_PlainText);

static void BM_SpdlogAdapter_LogStructured_JsonMode(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/true);
    const std::string msg = "structured benchmark event";
    const ILogger::Fields fields = {{"component", "db"}, {"op", "insert"}, {"latency_ms", "2"}};
    for (auto _ : state) {
        logger->logStructured(ILogger::Level::INFO, msg, fields);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_LogStructured_JsonMode);

// ============================================================================
// 5. logWithContext throughput (trace/span ID injection)
// ============================================================================

static void BM_SpdlogAdapter_LogWithContext_PlainText(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/false);
    const std::string msg = "request processed";
    const TraceContext ctx{"abc123trace", "def456span", "req-789"};
    const ILogger::Fields fields = {{"user", "alice"}};
    for (auto _ : state) {
        logger->logWithContext(ILogger::Level::INFO, msg, ctx, fields);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_LogWithContext_PlainText);

static void BM_SpdlogAdapter_LogWithContext_JsonMode(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/true);
    const std::string msg = "request processed";
    const TraceContext ctx{"abc123trace", "def456span", "req-789"};
    const ILogger::Fields fields = {{"user", "alice"}};
    for (auto _ : state) {
        logger->logWithContext(ILogger::Level::INFO, msg, ctx, fields);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_LogWithContext_JsonMode);

// ============================================================================
// 6. ConcernsContext convenience logging wrapper overhead
// ============================================================================

// Fixture shared across DI wrapper benchmarks.
class DIConcernsBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        ctx_ = ConcernsContext::createCustom(
            std::make_unique<NoOpLogger>(),
            std::make_unique<NoOpTracer>(),
            std::make_unique<NoOpMetrics>(),
            std::make_unique<NoOpCache>(),
            std::make_unique<NoOpCircuitBreaker>()
        );
    }
    void TearDown(const benchmark::State&) override {
        ctx_.reset();
    }
protected:
    std::shared_ptr<ConcernsContext> ctx_;
};

BENCHMARK_F(DIConcernsBenchFixture, LogInfo_ViaContext)(benchmark::State& state) {
    const std::string msg = "benchmark info message";
    for (auto _ : state) {
        ctx_->logInfo(msg);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(DIConcernsBenchFixture, LogError_ViaContext)(benchmark::State& state) {
    const std::string msg = "benchmark error message";
    for (auto _ : state) {
        ctx_->logError(msg);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(DIConcernsBenchFixture, MetricsIncrementCounter)(benchmark::State& state) {
    for (auto _ : state) {
        ctx_->metrics().incrementCounter("bench_ops_total");
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(DIConcernsBenchFixture, MetricsObserveHistogram)(benchmark::State& state) {
    for (auto _ : state) {
        ctx_->metrics().observeHistogram("bench_latency_ms", 1.23);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(DIConcernsBenchFixture, CacheGetMiss)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = ctx_->cache().get("nonexistent_key");
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(DIConcernsBenchFixture, CachePut)(benchmark::State& state) {
    const CacheEntry entry{"bench_payload", 1, 0};
    int64_t i = 0;
    for (auto _ : state) {
        // Use distinct keys to avoid eviction masking real put cost.
        std::string key = "bench_key_" + std::to_string(i++);
        ctx_->cache().put(key, entry);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 8. InMemoryCacheImpl overhead (real LRU cache, no NoOp)
// ============================================================================

class InMemoryCacheBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        cache_ = std::make_unique<InMemoryCacheImpl>(/*maxSize=*/10000, /*defaultTTL=*/0);
        // Pre-populate for get-hit benchmark
        for (int i = 0; i < 100; ++i) {
            cache_->put("preloaded_" + std::to_string(i),
                        CacheEntry{"value_" + std::to_string(i), 1, 0});
        }
    }
    void TearDown(const benchmark::State&) override {
        cache_.reset();
    }
protected:
    std::unique_ptr<InMemoryCacheImpl> cache_;
};

BENCHMARK_F(InMemoryCacheBenchFixture, GetHit)(benchmark::State& state) {
    int i = 0;
    for (auto _ : state) {
        auto result = cache_->get("preloaded_" + std::to_string(i % 100));
        benchmark::DoNotOptimize(result);
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(InMemoryCacheBenchFixture, GetMiss)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = cache_->get("nonexistent_key");
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(InMemoryCacheBenchFixture, Put)(benchmark::State& state) {
    const CacheEntry entry{"bench_payload", 1, 0};
    int64_t i = 0;
    for (auto _ : state) {
        cache_->put("bench_key_" + std::to_string(i++), entry);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 7. Logging throughput at different levels (NoOp baseline vs Spdlog)
// ============================================================================

static void BM_SpdlogAdapter_AllLevels(benchmark::State& state) {
    auto logger = makeNullSpdlogAdapter(/*json_mode=*/false);
    const std::string msg = "level benchmark";
    int i = 0;
    for (auto _ : state) {
        switch (i % 4) {
            case 0: logger->debug(msg); break;
            case 1: logger->info(msg);  break;
            case 2: logger->warn(msg);  break;
            case 3: logger->error(msg); break;
        }
        ++i;
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpdlogAdapter_AllLevels);

BENCHMARK_MAIN();
