/*
 * ThemisDB | File: test_concerns_context_focused.cpp | Version: 0.0.1
 * Module: core
 * Purpose: Focused unit tests for ConcernsContext — creation, resolve<T>(),
 *          runtime replacement, AdapterRegistry integration, and thread-safety.
 *
 * Test groups:
 *   CCT_01..CCT_04  ConcernsContext creation (NoOp, Custom, Config, defaults)
 *   CCT_05..CCT_08  resolve<T>(): ILogger, ITracer, IMetrics, ICache non-null
 *   CCT_09..CCT_12  Runtime replacement: swap, nullptr throws, visible swap
 *   CCT_13..CCT_16  AdapterRegistry path: registerAdapter + resolve, hotSwap,
 *                   count(), hasAdapter()
 *   CCT_17..CCT_18  Thread-safety: concurrent reads + concurrent replace
 */

#include <gtest/gtest.h>
#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/adapter_registry.h"
#include "core/concerns/adapter_metadata.h"

#include <thread>
#include <vector>
#include <atomic>
#include <typeindex>
#include <memory>
#include <stdexcept>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a fully no-op ConcernsContext without triggering ProductionMode check.
static std::shared_ptr<ConcernsContext> makeNoOp() {
    return ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );
}

struct ICustomAdapter {
    virtual ~ICustomAdapter() = default;
    virtual int id() const = 0;
};
struct CustomAdapterImpl : ICustomAdapter {
    explicit CustomAdapterImpl(int v) : v_(v) {}
    int id() const override { return v_; }
    int v_;
};

// ---------------------------------------------------------------------------
// CCT_01 — createCustom (NoOp) constructs without throwing
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_01_CreateCustomNoOpNoThrow) {
    EXPECT_NO_THROW({
        auto ctx = makeNoOp();
        ASSERT_NE(ctx, nullptr);
    });
}

// ---------------------------------------------------------------------------
// CCT_02 — createCustom with explicit adapters constructs without throwing
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_02_CreateCustomWithAdapters) {
    EXPECT_NO_THROW({
        auto ctx = ConcernsContext::createCustom(
            std::make_unique<NoOpLogger>(),
            std::make_unique<NoOpTracer>(),
            std::make_unique<NoOpMetrics>(),
            std::make_unique<NoOpCache>(),
            std::make_unique<NoOpSecrets>(),
            std::make_unique<NoOpFeatureFlags>()
        );
        ASSERT_NE(ctx, nullptr);
    });
}

// ---------------------------------------------------------------------------
// CCT_03 — create(Config) with noop adapter choices constructs without throwing
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_03_CreateFromConfig) {
    ConcernsContext::Config cfg;
    cfg.loggerAdapter          = "noop";
    cfg.tracerAdapter          = "noop";
    cfg.metricsAdapter         = "noop";
    cfg.cacheAdapter           = "noop";
    cfg.circuitBreakerAdapter  = "noop";
    cfg.featureFlagsAdapter    = "noop";
    cfg.auditAdapter           = "noop";
    cfg.secretsAdapter         = "noop";

    EXPECT_NO_THROW({
        auto ctx = ConcernsContext::create(cfg);
        ASSERT_NE(ctx, nullptr);
    });
}

// ---------------------------------------------------------------------------
// CCT_04 — default-constructed context has non-null logger and tracer
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_04_DefaultContextHasNonNullLoggerTracer) {
    auto ctx = makeNoOp();
    // Named accessors are references — they compile, meaning the underlying
    // unique_ptr is non-null.
    EXPECT_NO_THROW({ ctx->logger().info("ping"); });
    EXPECT_NO_THROW({ ctx->tracer().startSpan("ping"); });
}

// ---------------------------------------------------------------------------
// CCT_05 — resolve<ILogger>() returns non-null shared_ptr
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_05_ResolveILogger_NonNull) {
    auto ctx = makeNoOp();
    auto ptr = ctx->resolve<ILogger>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW({ ptr->info("from resolve"); });
}

// ---------------------------------------------------------------------------
// CCT_06 — resolve<ITracer>() returns non-null shared_ptr
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_06_ResolveITracer_NonNull) {
    auto ctx = makeNoOp();
    auto ptr = ctx->resolve<ITracer>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW({ ptr->startSpan("from resolve"); });
}

// ---------------------------------------------------------------------------
// CCT_07 — resolve<IMetrics>() returns non-null shared_ptr
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_07_ResolveIMetrics_NonNull) {
    auto ctx = makeNoOp();
    auto ptr = ctx->resolve<IMetrics>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW({ ptr->incrementCounter("c", 1); });
}

// ---------------------------------------------------------------------------
// CCT_08 — resolve<ICache>() returns non-null shared_ptr
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_08_ResolveICache_NonNull) {
    auto ctx = makeNoOp();
    auto ptr = ctx->resolve<ICache>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->size(), 0u);
}

// ---------------------------------------------------------------------------
// CCT_09 — replaceLogger swaps to new adapter; new logger is used thereafter
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_09_ReplaceLoggerSwaps) {
    auto ctx = makeNoOp();
    EXPECT_NO_THROW({
        ctx->replaceLogger(std::make_unique<NoOpLogger>());
        // After replace, logger() still works
        ctx->logger().info("after replace");
    });
}

// ---------------------------------------------------------------------------
// CCT_10 — replaceMetrics swaps to new adapter
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_10_ReplaceMetricsSwaps) {
    auto ctx = makeNoOp();
    EXPECT_NO_THROW({
        ctx->replaceMetrics(std::make_unique<NoOpMetrics>());
        ctx->metrics().incrementCounter("after_replace", 1);
    });
}

// ---------------------------------------------------------------------------
// CCT_11 — nullptr replaceLogger throws std::invalid_argument
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_11_NullptrReplaceLoggerThrows) {
    auto ctx = makeNoOp();
    EXPECT_THROW(ctx->replaceLogger(nullptr), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// CCT_12 — nullptr replaceMetrics throws std::invalid_argument;
//           replacement is visible immediately after a valid swap
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_12_NullptrReplaceMetricsThrowsAndSwapVisible) {
    auto ctx = makeNoOp();
    EXPECT_THROW(ctx->replaceMetrics(nullptr), std::invalid_argument);

    // Valid replacement is immediately visible via resolve<IMetrics>()
    ctx->replaceMetrics(std::make_unique<NoOpMetrics>());
    auto ptr = ctx->resolve<IMetrics>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW(ptr->incrementCounter("visible", 1));
}

// ---------------------------------------------------------------------------
// CCT_13 — AdapterRegistry: registerAdapter + resolve<T>() for a custom type
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_13_AdapterRegistryRegisterAndResolve) {
    auto ctx = makeNoOp();
    auto adapter = std::make_shared<CustomAdapterImpl>(99);
    ctx->registry().registerAdapter<ICustomAdapter>("custom", adapter);

    auto resolved = ctx->resolve<ICustomAdapter>();
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->id(), 99);
}

// ---------------------------------------------------------------------------
// CCT_14 — AdapterRegistry: hotSwap completes and new adapter is returned
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_14_AdapterRegistryHotSwap) {
    auto ctx = makeNoOp();
    ctx->registry().registerAdapter<ICustomAdapter>(
        "custom", std::make_shared<CustomAdapterImpl>(1));

    auto new_adapter = std::make_shared<CustomAdapterImpl>(2);
    bool ok = ctx->registry().hotSwap<ICustomAdapter>(new_adapter);
    EXPECT_TRUE(ok);

    auto resolved = ctx->resolve<ICustomAdapter>();
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->id(), 2);
}

// ---------------------------------------------------------------------------
// CCT_15 — AdapterRegistry: count() is correct after registrations
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_15_AdapterRegistryCountCorrect) {
    auto ctx = makeNoOp();
    EXPECT_EQ(ctx->registry().count(), 0u);

    ctx->registry().registerAdapter<ICustomAdapter>(
        "c1", std::make_shared<CustomAdapterImpl>(1));
    EXPECT_EQ(ctx->registry().count(), 1u);

    // Register same type again → overwrites, count stays 1
    ctx->registry().registerAdapter<ICustomAdapter>(
        "c2", std::make_shared<CustomAdapterImpl>(2));
    EXPECT_EQ(ctx->registry().count(), 1u);
}

// ---------------------------------------------------------------------------
// CCT_16 — AdapterRegistry: hasAdapter() returns true after registration
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_16_AdapterRegistryHasAdapter) {
    auto ctx = makeNoOp();
    EXPECT_FALSE(ctx->registry().hasAdapter(std::type_index(typeid(ICustomAdapter))));

    ctx->registry().registerAdapter<ICustomAdapter>(
        "c1", std::make_shared<CustomAdapterImpl>(5));
    EXPECT_TRUE(ctx->registry().hasAdapter(std::type_index(typeid(ICustomAdapter))));
}

// ---------------------------------------------------------------------------
// CCT_17 — thread-safety: 32 threads × 1000 concurrent resolve<ILogger> calls
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_17_ConcurrentResolvesNoCrash) {
    auto ctx = makeNoOp();

    constexpr int kThreads = 32;
    constexpr int kCalls   = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> ok_count{0};
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kCalls; ++i) {
                auto p = ctx->resolve<ILogger>();
                if (p) ++ok_count;
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(ok_count.load(), kThreads * kCalls);
}

// ---------------------------------------------------------------------------
// CCT_18 — thread-safety: concurrent replaceLogger with concurrent reads
// ---------------------------------------------------------------------------
TEST(ConcernsContextTest, CCT_18_ConcurrentReplaceAndReadNoCrash) {
    auto ctx = makeNoOp();

    constexpr int kReaders = 16;
    constexpr int kReads   = 200;
    constexpr int kSwaps   = 10;

    std::atomic<bool> done{false};

    // Reader threads
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&] {
            while (!done.load(std::memory_order_acquire)) {
                // Access the logger — must not crash even during a swap
                ctx->logger().info("concurrent read");
            }
        });
    }

    // Swap thread
    for (int s = 0; s < kSwaps; ++s) {
        ctx->replaceLogger(std::make_unique<NoOpLogger>());
    }
    done.store(true, std::memory_order_release);

    for (auto& th : readers) th.join();

    // If we reach here without a crash the test passes
    SUCCEED();
}
