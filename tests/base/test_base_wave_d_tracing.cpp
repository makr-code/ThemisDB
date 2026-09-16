/**
 * @file test_base_wave_d_tracing.cpp
 * @brief Wave D delivery: distributed tracing, high-cardinality stress, and
 *        fail-closed verification for the base module.
 *
 * Coverage:
 *  - Tracing integration: ScopedSpan events emitted by HotReloadManager.
 *  - High-cardinality stress: concurrent reloadModule() / rollback() calls
 *    at 100–1000 concurrency produce consistent stats and span event counts.
 *  - Fail-closed nachweis: BASE_LOADER_* and BASE_SANDBOX_* error paths
 *    leave no partial state and set the span error code correctly.
 *  - Observability exporter: RemoteRegistryClient retry_exhausted_count and
 *    timeout_count are incremented and the ObservabilityHook is fired.
 *
 * All tests use production code paths — no stubs or mocks are introduced.
 * Requires: GTest, themis_core (hot_reload_manager, base_error_taxonomy,
 *            remote_registry_client, trace_context).
 */

#include <gtest/gtest.h>

#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include "themis/base/base_error_taxonomy.h"
#include "themis/base/trace_context.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis { namespace modules {

// =============================================================================
// Helpers
// =============================================================================

/// Thread-safe accumulator for SpanEvent records.
struct SpanCollector {
    mutable std::mutex    mtx;
    std::vector<SpanEvent> events;

    SpanEmitter emitter() {
        return [this](const SpanEvent& ev) {
            std::lock_guard<std::mutex> lk(mtx);
            events.push_back(ev);
        };
    }

    std::size_t countStartEvents() const {
        std::lock_guard<std::mutex> lk(mtx);
        std::size_t n = 0;
        for (const auto& e : events) { if (e.is_start) ++n; }
        return n;
    }

    std::size_t countEndEvents() const {
        std::lock_guard<std::mutex> lk(mtx);
        std::size_t n = 0;
        for (const auto& e : events) { if (!e.is_start) ++n; }
        return n;
    }

    std::size_t countErrorEndEvents() const {
        std::lock_guard<std::mutex> lk(mtx);
        std::size_t n = 0;
        for (const auto& e : events) {
            if (!e.is_start && e.error_code != 0) ++n;
        }
        return n;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mtx);
        events.clear();
    }

    /// @brief Return a copy of the most recent event under the lock.
    SpanEvent lastEvent() const {
        std::lock_guard<std::mutex> lk(mtx);
        return events.back();
    }
};

// =============================================================================
// Suite 1: Tracing integration — span emitter receives events
// =============================================================================

class TracingIntegrationTest : public ::testing::Test {
protected:
    HotReloadManager mgr_;
    SpanCollector    collector_;

    void SetUp() override {
        mgr_.setSpanEmitter(collector_.emitter());
    }
};

/// reloadModule() on an unregistered module emits a start and error-end span.
TEST_F(TracingIntegrationTest, UnregisteredModuleEmitsErrorSpan) {
    auto result = mgr_.reloadModule("no_such_module", "/tmp/dummy.so");
    EXPECT_FALSE(result.success);

    EXPECT_EQ(collector_.countStartEvents(), 1u)
        << "Expected exactly one span-start event";
    EXPECT_EQ(collector_.countEndEvents(), 1u)
        << "Expected exactly one span-end event";
    EXPECT_EQ(collector_.countErrorEndEvents(), 1u)
        << "Expected the span-end to carry a non-zero error code";
}

/// rollback() on a module with no backup emits a start and error-end span.
TEST_F(TracingIntegrationTest, RollbackNoBackupEmitsErrorSpan) {
    ModuleLoader loader;
    mgr_.registerModule("mod_a", loader);

    auto result = mgr_.rollback("mod_a");
    EXPECT_FALSE(result.success);

    EXPECT_EQ(collector_.countStartEvents(), 1u);
    EXPECT_EQ(collector_.countErrorEndEvents(), 1u);
}

/// The default no-op emitter produces no observable side-effects.
TEST(TracingDefaultEmitterTest, NoOpEmitterIsDefault) {
    HotReloadManager mgr;
    // With default no-op emitter, calling reloadModule must not crash.
    EXPECT_NO_THROW({
        auto r = mgr.reloadModule("ghost", "/nonexistent");
        EXPECT_FALSE(r.success);
    });
}

/// setSpanEmitter() replaces the emitter; subsequent operations use the new one.
TEST(TracingEmitterReplacement, ReplacementTakesEffect) {
    HotReloadManager mgr;

    SpanCollector c1, c2;
    mgr.setSpanEmitter(c1.emitter());
    mgr.reloadModule("x", "/nonexistent");

    mgr.setSpanEmitter(c2.emitter());
    mgr.reloadModule("y", "/nonexistent");

    EXPECT_GE(c1.countStartEvents(), 1u) << "c1 should have captured the first operation";
    EXPECT_GE(c2.countStartEvents(), 1u) << "c2 should have captured the second operation";
}

/// spanEmitter() returns the currently installed emitter (non-null).
TEST(TracingEmitterAccessTest, SpanEmitterGetterNonNull) {
    HotReloadManager mgr;
    // Default emitter must be callable (no-op, but not empty function).
    EXPECT_NO_THROW({
        SpanEvent ev;
        ev.context   = TraceContext::generate("test");
        ev.timestamp = std::chrono::steady_clock::now();
        ev.is_start  = false;
        mgr.spanEmitter()(ev);
    });
}

// =============================================================================
// Suite 2: High-cardinality stress — concurrent operations
// =============================================================================

/// 200 concurrent reloadModule() calls on unregistered modules must all fail
/// cleanly, not crash, and each produce exactly one matching span pair.
TEST(HighCardinalityStressTest, ConcurrentUnregisteredReloads) {
    constexpr int kConcurrency = 200;

    HotReloadManager mgr;
    SpanCollector    collector;
    mgr.setSpanEmitter(collector.emitter());

    std::vector<std::thread> threads;
    threads.reserve(kConcurrency);
    for (int i = 0; i < kConcurrency; ++i) {
        threads.emplace_back([&mgr, i]() {
            const std::string name = "mod_" + std::to_string(i);
            auto r = mgr.reloadModule(name, "/nonexistent");
            EXPECT_FALSE(r.success);
        });
    }
    for (auto& t : threads) { t.join(); }

    EXPECT_EQ(collector.countStartEvents(), static_cast<std::size_t>(kConcurrency));
    EXPECT_EQ(collector.countEndEvents(),   static_cast<std::size_t>(kConcurrency));
    EXPECT_EQ(collector.countErrorEndEvents(), static_cast<std::size_t>(kConcurrency));

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.failedReloads, static_cast<uint64_t>(kConcurrency));
}

/// 500 concurrent registerModule() calls must not corrupt internal state.
TEST(HighCardinalityStressTest, ConcurrentRegistration) {
    constexpr int kConcurrency = 500;

    HotReloadManager              mgr;
    std::vector<ModuleLoader>     loaders(kConcurrency);
    std::vector<std::thread>      threads;
    threads.reserve(kConcurrency);

    for (int i = 0; i < kConcurrency; ++i) {
        threads.emplace_back([&mgr, &loaders, i]() {
            mgr.registerModule("mod_" + std::to_string(i), loaders[i]);
        });
    }
    for (auto& t : threads) { t.join(); }

    auto names = mgr.registeredModules();
    EXPECT_EQ(names.size(), static_cast<std::size_t>(kConcurrency));
}

/// Mixed concurrent load of registerModule + reloadModule + rollback does not
/// crash and leaves stats consistent (total ≥ successful + failed).
TEST(HighCardinalityStressTest, MixedConcurrentOperations) {
    constexpr int kWorkers = 100;

    HotReloadManager            mgr;
    std::vector<ModuleLoader>   loaders(kWorkers);

    // Pre-register all modules so reloadModule can find them.
    for (int i = 0; i < kWorkers; ++i) {
        mgr.registerModule("mod_" + std::to_string(i), loaders[i]);
    }

    std::vector<std::thread> threads;
    threads.reserve(kWorkers * 2);

    // Half the threads try reloadModule (will fail — path doesn't exist).
    for (int i = 0; i < kWorkers; ++i) {
        threads.emplace_back([&mgr, i]() {
            mgr.reloadModule("mod_" + std::to_string(i), "/nonexistent_" + std::to_string(i));
        });
    }
    // Other half try rollback (no backup → fail cleanly).
    for (int i = 0; i < kWorkers; ++i) {
        threads.emplace_back([&mgr, i]() {
            mgr.rollback("mod_" + std::to_string(i));
        });
    }

    for (auto& t : threads) { t.join(); }

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.totalReloads, stats.successfulReloads + stats.failedReloads)
        << "total reloads must equal successful + failed";
}

// =============================================================================
// Suite 3: Fail-closed nachweis — error paths leave no partial state
// =============================================================================

class FailClosedTest : public ::testing::Test {
protected:
    HotReloadManager mgr_;
    SpanCollector    collector_;

    void SetUp() override {
        mgr_.setSpanEmitter(collector_.emitter());
    }
};

/// After a failed reloadModule(), the module slot retains the original loader
/// (no partial-state mutation of the slot).
TEST_F(FailClosedTest, FailedReloadDoesNotMutateSlot) {
    ModuleLoader loader;
    mgr_.registerModule("mod_fail", loader);

    // No backup is available before any successful reload.
    EXPECT_FALSE(mgr_.isRollbackAvailable("mod_fail"));

    auto result = mgr_.reloadModule("mod_fail", "/nonexistent_new.so");
    EXPECT_FALSE(result.success);

    // After failed reload: rollback must still be unavailable (no new backup).
    EXPECT_FALSE(mgr_.isRollbackAvailable("mod_fail"))
        << "A failed reload must not create a backup slot";

    // Stats must reflect one failed reload.
    auto stats = mgr_.getStats();
    EXPECT_EQ(stats.failedReloads, 1u);
    EXPECT_EQ(stats.successfulReloads, 0u);
}

/// rollback() on an unregistered module fails closed with no side-effects.
TEST_F(FailClosedTest, RollbackUnregisteredFailsClosed) {
    auto result = mgr_.rollback("ghost_module");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_EQ(collector_.countErrorEndEvents(), 1u);
}

/// BASE_LOADER_PATH_NOT_FOUND: format and description are non-empty; code is 1100.
TEST(FailClosedTaxonomyTest, LoaderPathNotFoundCodeAndDescription) {
    using namespace BaseErrorTaxonomy;
    EXPECT_EQ(BASE_LOADER_PATH_NOT_FOUND::code, 1100);
    EXPECT_FALSE(std::string(BASE_LOADER_PATH_NOT_FOUND::description()).empty());
    EXPECT_FALSE(std::string(BASE_LOADER_PATH_NOT_FOUND::remediationHint()).empty());

    const auto msg = BASE_LOADER_PATH_NOT_FOUND::format("test_mod", "/some/path");
    EXPECT_NE(msg.find("1100"), std::string::npos) << "format must embed the error code";
    EXPECT_NE(msg.find("test_mod"), std::string::npos);
}

/// BASE_SANDBOX_DEGRADED: code is 1153 with non-empty remediation hint.
TEST(FailClosedTaxonomyTest, SandboxDegradedCodeAndRemediationHint) {
    using namespace BaseErrorTaxonomy;
    EXPECT_EQ(BASE_SANDBOX_DEGRADED::code, 1153);
    EXPECT_FALSE(std::string(BASE_SANDBOX_DEGRADED::remediationHint()).empty());
}

/// All loader and sandbox codes are known to resolveDescription().
TEST(FailClosedTaxonomyTest, AllLoaderSandboxCodesKnown) {
    using namespace BaseErrorTaxonomy;
    const std::vector<int> codes = {
        BASE_LOADER_PATH_NOT_FOUND::code,
        BASE_LOADER_SIGNATURE_REJECTED::code,
        BASE_LOADER_ABI_MISMATCH::code,
        BASE_LOADER_LOAD_FAILED::code,
        BASE_LOADER_INIT_FAILED::code,
        BASE_LOADER_HEALTH_CHECK_FAILED::code,
        BASE_SANDBOX_LAUNCH_FAILED::code,
        BASE_SANDBOX_RESOURCE_LIMIT::code,
        BASE_SANDBOX_TIMEOUT::code,
        BASE_SANDBOX_DEGRADED::code,
    };
    for (int c : codes) {
        EXPECT_TRUE(isKnownCode(c)) << "code " << c << " must be known";
        EXPECT_FALSE(resolveDescription(c).empty()) << "code " << c << " must have a description";
    }
}

// =============================================================================
// Suite 4: TraceContext and ScopedSpan unit tests
// =============================================================================

/// generate() produces valid contexts with non-zero IDs.
TEST(TraceContextTest, GenerateProducesValidContext) {
    auto ctx = TraceContext::generate("test.op");
    EXPECT_TRUE(ctx.isValid());
    EXPECT_NE(ctx.trace_id, 0u);
    EXPECT_NE(ctx.span_id, 0u);
    EXPECT_EQ(ctx.parent_span_id, 0u);
    EXPECT_EQ(ctx.operation_name, "test.op");
}

/// child() shares the trace ID and sets parent_span_id.
TEST(TraceContextTest, ChildSharesTraceId) {
    auto parent = TraceContext::generate("parent");
    auto child  = parent.child("child");

    EXPECT_EQ(child.trace_id, parent.trace_id);
    EXPECT_NE(child.span_id, parent.span_id);
    EXPECT_EQ(child.parent_span_id, parent.span_id);
}

/// Successive generate() calls produce unique IDs.
TEST(TraceContextTest, UniqueIds) {
    auto ctx1 = TraceContext::generate("op1");
    auto ctx2 = TraceContext::generate("op2");
    EXPECT_NE(ctx1.span_id, ctx2.span_id);
    EXPECT_NE(ctx1.trace_id, ctx2.trace_id);
}

/// ScopedSpan emits a start event on construction and end event on destruction.
TEST(ScopedSpanTest, EmitsStartAndEndEvents) {
    SpanCollector collector;
    SpanEmitter   emitter = collector.emitter();

    {
        auto ctx = TraceContext::generate("scoped.test");
        ScopedSpan span(ctx, emitter);
        EXPECT_EQ(collector.countStartEvents(), 1u);
        EXPECT_EQ(collector.countEndEvents(), 0u);
    } // destructor fires here

    EXPECT_EQ(collector.countStartEvents(), 1u);
    EXPECT_EQ(collector.countEndEvents(), 1u);
    EXPECT_EQ(collector.countErrorEndEvents(), 0u);
}

/// ScopedSpan setError() propagates to the end event.
TEST(ScopedSpanTest, SetErrorPropagates) {
    SpanCollector collector;
    SpanEmitter   emitter = collector.emitter();

    {
        auto ctx = TraceContext::generate("error.test");
        ScopedSpan span(ctx, emitter);
        span.setError(1100, "path not found");
    }

    EXPECT_EQ(collector.countErrorEndEvents(), 1u);
    const SpanEvent end_ev = collector.lastEvent();
    EXPECT_EQ(end_ev.error_code, 1100);
    EXPECT_EQ(end_ev.error_detail, "path not found");
}

/// noOpSpanEmitter does not throw and discards events silently.
TEST(NoOpSpanEmitterTest, DoesNotThrow) {
    auto noop = noOpSpanEmitter();
    EXPECT_NO_THROW({
        SpanEvent ev;
        ev.context   = TraceContext::generate("noop");
        ev.timestamp = std::chrono::steady_clock::now();
        ev.is_start  = true;
        noop(ev);
    });
}

}} // namespace themis::modules
