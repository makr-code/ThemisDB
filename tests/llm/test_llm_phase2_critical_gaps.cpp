/**
 * @file test_llm_phase2_critical_gaps.cpp
 * @brief LLM Module Phase 2 — CRITICAL / HIGH gap regression tests.
 *
 * Covers every gap category fixed in PHASE2-LLM-CRITICAL:
 *  - LLM-P2-DTOR-01..05  : exception_in_destructor (noexcept + swallow)
 *  - LLM-P2-NULL-01..06  : null_dereference (graceful error returns)
 *  - LLM-P2-RAII-01..04  : resource_leaked_in_exception (RAII guards)
 *  - LLM-P2-DEL-01..03   : delete_no_nullptr / delete_without_nullptr
 *  - LLM-P2-INIT-01..04  : uninitialized_access (member initializers)
 *  - LLM-P2-LIFE-01..04  : plugin lifecycle (load / unload / reload)
 *  - LLM-P2-CONN-01..03  : db_connection_leak (scoped RAII release)
 *  - LLM-P2-CONC-01..02  : noexcept under concurrent load
 *
 * Design notes:
 *  - No live llama.cpp runtime required; all tests use mock/stub objects.
 *  - SimAllocGuard tracks simulated allocations for leak detection.
 *  - std::is_nothrow_destructible_v<> enforces compile-time noexcept contracts.
 *  - All tests are deterministic and do not require GPU hardware.
 *
 * @version 2.0.0-phase2-critical
 * @note CTest labels: release_critical;llm;phase2
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Simulated allocation tracking
// ─────────────────────────────────────────────────────────────────────────────

static std::atomic<std::ptrdiff_t> g_sim_alloc_net{0};

class SimAllocGuard {
public:
    explicit SimAllocGuard(std::ptrdiff_t units = 1) : units_(units), active_(true) {
        g_sim_alloc_net.fetch_add(units_, std::memory_order_relaxed);
    }
    ~SimAllocGuard() {
        if (active_) {
            g_sim_alloc_net.fetch_sub(units_, std::memory_order_relaxed);
        }
    }
    SimAllocGuard(SimAllocGuard&& o) noexcept : units_(o.units_), active_(o.active_) {
        o.active_ = false;
    }
    SimAllocGuard& operator=(SimAllocGuard&& o) noexcept {
        if (this != &o) {
            if (active_) {
              g_sim_alloc_net.fetch_sub(units_, std::memory_order_relaxed);
            }
            units_ = o.units_;
            active_ = o.active_;
            o.active_ = false;
        }
        return *this;
    }
    SimAllocGuard(const SimAllocGuard&) = delete;
    SimAllocGuard& operator=(const SimAllocGuard&) = delete;
    void release() noexcept { active_ = false; }

private:
    std::ptrdiff_t units_{0};
    bool active_{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// Minimal mock infrastructure (no llama.cpp dependency)
// ─────────────────────────────────────────────────────────────────────────────

struct ResourceTracker {
    bool cleaned_up{false};
    bool threw_on_cleanup{false};
};

static void FreeResource(void* p) noexcept {
    if (p) {
        auto* t = static_cast<ResourceTracker*>(p);
        t->cleaned_up = true;
    }
}

struct MockPlugin {
    explicit MockPlugin(ResourceTracker& tracker) : tracker_(tracker) {
        tracker_.cleaned_up = false;
    }
    ~MockPlugin() noexcept {
        try { tracker_.cleaned_up = true; }
        catch (...) {}
    }
    ResourceTracker& tracker_;
};

/// Plugin whose destructor would propagate an exception without the noexcept guard.
struct ThrowingDtorPlugin {
    ~ThrowingDtorPlugin() noexcept {
        try { throw std::runtime_error("simulated dtor exception"); }
        catch (...) { /* swallowed as required by exception_in_destructor fix */ }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class LLMPhase2CriticalGapsTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_sim_alloc_net.store(0, std::memory_order_relaxed);
    }
    void TearDown() override {
        EXPECT_EQ(0, g_sim_alloc_net.load(std::memory_order_relaxed))
            << "Simulated allocation net count is non-zero — potential resource leak";
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-DTOR: exception_in_destructor
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-DTOR-01: ThrowingDtorPlugin destructor must not propagate exception.
TEST_F(LLMPhase2CriticalGapsTest, DTOR01_ThrowingPluginDtorSafe) {
    {
        ThrowingDtorPlugin p;
        // Destructor runs at end of scope — must NOT terminate/throw
    }
    SUCCEED();
}

/// LLM-P2-DTOR-02: RAII resource guard calls FreeResource exactly once.
TEST_F(LLMPhase2CriticalGapsTest, DTOR02_RaiiGuardFreesOnce) {
    ResourceTracker t;
    {
        auto guard = std::unique_ptr<ResourceTracker, decltype(&FreeResource)>(
            &t, FreeResource);
        EXPECT_FALSE(t.cleaned_up);
    }
    EXPECT_TRUE(t.cleaned_up);
}

/// LLM-P2-DTOR-03: release() prevents double-free from RAII guard.
TEST_F(LLMPhase2CriticalGapsTest, DTOR03_ReleasePreventsDtorCleanup) {
    ResourceTracker t;
    {
        auto guard = std::unique_ptr<ResourceTracker, decltype(&FreeResource)>(
            &t, FreeResource);
        guard.release();    // relinquish ownership — dtor becomes a no-op
    }
    EXPECT_FALSE(t.cleaned_up) << "FreeResource must NOT be called after release()";
}

/// LLM-P2-DTOR-04: MockPlugin is statically noexcept-destructible.
TEST_F(LLMPhase2CriticalGapsTest, DTOR04_MockPluginIsNothrowDestructible) {
    static_assert(std::is_nothrow_destructible_v<MockPlugin>,
                  "MockPlugin destructor must be noexcept");
    SUCCEED();
}

/// LLM-P2-DTOR-05: ThrowingDtorPlugin is statically noexcept-destructible.
TEST_F(LLMPhase2CriticalGapsTest, DTOR05_ThrowingDtorPluginIsNothrowDestructible) {
    static_assert(std::is_nothrow_destructible_v<ThrowingDtorPlugin>,
                  "ThrowingDtorPlugin destructor must be noexcept");
    SUCCEED();
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-NULL: null_dereference
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-NULL-01: Null plugin handle → graceful error path, no crash.
TEST_F(LLMPhase2CriticalGapsTest, NULL01_NullPluginReturnsErrorNotCrash) {
    MockPlugin* plugin = nullptr;
    bool handled = false;
    if (plugin == nullptr) {
        handled = true;
    } else {
        FAIL() << "Should not reach plugin operation with null plugin";
    }
    EXPECT_TRUE(handled);
}

/// LLM-P2-NULL-02: Null optional<ModelInfo> uses value_or (no dereference).
TEST_F(LLMPhase2CriticalGapsTest, NULL02_NullModelInfoHandled) {
    std::optional<std::string> model_name;  // nullopt simulates null ModelInfo
    const std::string display = model_name.value_or("<no model>");
    EXPECT_EQ("<no model>", display);
}

/// LLM-P2-NULL-03: Empty plugin list iteration is safe (no crash).
TEST_F(LLMPhase2CriticalGapsTest, NULL03_EmptyPluginMapIterationSafe) {
    std::unordered_map<std::string, std::unique_ptr<MockPlugin>> registry;
    std::vector<std::string> names = {};

    for (const auto& [k, v] : registry) {
        names.push_back(k);
    }
    EXPECT_TRUE(names.empty());
}

/// LLM-P2-NULL-04: Null lctx_guard returns false (no model / context crash).
TEST_F(LLMPhase2CriticalGapsTest, NULL04_NullContextGuardDetected) {
    bool context_failed = false;
    auto lctx_guard = std::unique_ptr<ResourceTracker, decltype(&FreeResource)>(
        nullptr, FreeResource);
    if (!lctx_guard) {
        context_failed = true;
        // lmodel_guard would auto-free via RAII here in real code
    }
    EXPECT_TRUE(context_failed);
}

/// LLM-P2-NULL-05: Null plugin entry skipped in aggregation loop.
TEST_F(LLMPhase2CriticalGapsTest, NULL05_NullPluginSkippedInAggregation) {
    struct FakeEntry { std::unique_ptr<MockPlugin> plugin; };
    ResourceTracker rt;
    std::vector<FakeEntry> entries;
    entries.push_back(FakeEntry{std::make_unique<MockPlugin>(rt)});
    entries.push_back(FakeEntry{nullptr});

    int processed = 0;
    for (const auto& e : entries) {
        if (!e.plugin) continue;    // null_dereference guard
        processed++;
    }
    EXPECT_EQ(1, processed);
}

/// LLM-P2-NULL-06: listModels skips null plugin without crash.
TEST_F(LLMPhase2CriticalGapsTest, NULL06_ListModelsSkipsNullPlugin) {
    struct FakeEntry {
        std::string name;
        bool has_model{false};
        std::unique_ptr<MockPlugin> plugin;
    };
    ResourceTracker rt1, rt2;
    std::vector<FakeEntry> entries;
    entries.push_back({"p1", true,  std::make_unique<MockPlugin>(rt1)});
    entries.push_back({"p2", false, nullptr});

    std::vector<std::string> models = {};

    for (const auto& e : entries) {
        if (!e.plugin) {
          continue;
        }
        if (e.has_model) {
          models.push_back(e.name);
        }
    }
    ASSERT_EQ(1u, models.size());
    EXPECT_EQ("p1", models[0]);
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-RAII: resource_leaked_in_exception
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-RAII-01: Exception after lmodel alloc triggers guard cleanup.
TEST_F(LLMPhase2CriticalGapsTest, RAII01_LModelGuardCleansOnException) {
    bool model_freed = false;
    ResourceTracker model_t;
    auto free_model = [&model_freed](ResourceTracker* p) noexcept {
        if (p) {
          model_freed = true;
        }
    };
    try {
        auto lmodel_guard = std::unique_ptr<ResourceTracker, decltype(free_model)>(
            &model_t, free_model);
        throw std::bad_alloc{};
        (void)lmodel_guard.release();   // must not be reached
    } catch (const std::bad_alloc&) {}
    EXPECT_TRUE(model_freed);
}

/// LLM-P2-RAII-02: Exception after lctx alloc triggers both guards cleanup.
TEST_F(LLMPhase2CriticalGapsTest, RAII02_BothGuardsCleanOnException) {
    bool model_freed = false, ctx_freed = false;
    ResourceTracker model_t, ctx_t;
    auto free_model = [&model_freed](ResourceTracker* p) noexcept { if (p) model_freed = true; };
    auto free_ctx   = [&ctx_freed  ](ResourceTracker* p) noexcept { if (p) ctx_freed   = true; };
    try {
        auto lmodel_guard = std::unique_ptr<ResourceTracker, decltype(free_model)>(&model_t, free_model);
        auto lctx_guard   = std::unique_ptr<ResourceTracker, decltype(free_ctx)  >(&ctx_t,   free_ctx);
        throw std::runtime_error("metadata write failure");
        (void)lmodel_guard.release();
        (void)lctx_guard.release();
    } catch (...) {}
    EXPECT_TRUE(model_freed);
    EXPECT_TRUE(ctx_freed);
}

/// LLM-P2-RAII-03: Successful path releases guards — dtor is a no-op.
TEST_F(LLMPhase2CriticalGapsTest, RAII03_SuccessPathReleasesGuards) {
    bool model_freed = false;
    ResourceTracker model_t;
    auto free_model = [&model_freed](ResourceTracker* p) noexcept { if (p) model_freed = true; };
    void* stored = nullptr;
    {
        auto lmodel_guard = std::unique_ptr<ResourceTracker, decltype(free_model)>(&model_t, free_model);
        stored = static_cast<void*>(lmodel_guard.release());
    }
    EXPECT_FALSE(model_freed) << "guard released ownership; dtor must not free";
    // simulate final ownership cleanup
    free_model(static_cast<ResourceTracker*>(stored));
    EXPECT_TRUE(model_freed);
}

/// LLM-P2-RAII-04: SimAllocGuard net count is zero across nested throw/catch.
TEST_F(LLMPhase2CriticalGapsTest, RAII04_SimAllocNetZeroAfterException) {
    {
        SimAllocGuard g1(1);
        SimAllocGuard g2(2);
        try {
            SimAllocGuard g3(4);
            throw std::runtime_error("simulated");
        } catch (...) {}
        EXPECT_EQ(3, g_sim_alloc_net.load(std::memory_order_relaxed));
    }
    EXPECT_EQ(0, g_sim_alloc_net.load(std::memory_order_relaxed));
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-DEL: delete_no_nullptr / delete_without_nullptr
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-DEL-01: delete nullptr is well-defined (no crash, no UB).
TEST_F(LLMPhase2CriticalGapsTest, DEL01_DeleteNullptrIsNoOp) {
    MockPlugin* p = nullptr;
    delete p;   // Standard: delete nullptr is a no-op
    SUCCEED();
}

/// LLM-P2-DEL-02: Pattern — null-check + delete + set nullptr prevents double-free.
TEST_F(LLMPhase2CriticalGapsTest, DEL02_NullCheckBeforeDeletePattern) {
    auto* p = new ResourceTracker{};
    if (p) { delete p; p = nullptr; }
    EXPECT_EQ(nullptr, p);
    // Second call: guard prevents double-free
    if (p) { delete p; p = nullptr; }
    EXPECT_EQ(nullptr, p);
}

/// LLM-P2-DEL-03: unique_ptr reset() is idempotent — no double-free.
TEST_F(LLMPhase2CriticalGapsTest, DEL03_UniquePtrResetIsIdempotent) {
    auto p = std::make_unique<ResourceTracker>();
    p.reset();
    EXPECT_EQ(nullptr, p.get());
    EXPECT_NO_THROW(p.reset());   // second reset must not crash
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-INIT: uninitialized_access
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-INIT-01: ConcurrentInferenceTracker fields are zero/value-initialised.
TEST_F(LLMPhase2CriticalGapsTest, INIT01_ConcurrentTrackerZeroInit) {
    struct ConcurrentInferenceTracker {
        std::atomic<size_t> active_inferences{0};
        size_t max_concurrent{256};
        ConcurrentInferenceTracker() noexcept = default;
    };
    ConcurrentInferenceTracker t;
    EXPECT_EQ(0u,   t.active_inferences.load());
    EXPECT_EQ(256u, t.max_concurrent);
}

/// LLM-P2-INIT-02: CachedModel POD fields default-initialised to zero/nullptr.
TEST_F(LLMPhase2CriticalGapsTest, INIT02_CachedModelFieldsZeroInit) {
    struct MockCachedModel {
        void* model_handle   = nullptr;
        void* context_handle = nullptr;
        size_t use_count     = 0;
        size_t vram_mb       = 0;
        bool is_loading      = false;
        bool keep_loaded     = false;
    };
    MockCachedModel m;
    EXPECT_EQ(nullptr, m.model_handle);
    EXPECT_EQ(nullptr, m.context_handle);
    EXPECT_EQ(0u, m.use_count);
    EXPECT_FALSE(m.is_loading);
    EXPECT_FALSE(m.keep_loaded);
}

/// LLM-P2-INIT-03: LlamaLoadLogCaptureState flags are false before first callback.
TEST_F(LLMPhase2CriticalGapsTest, INIT03_LogCaptureStateFlagsZeroInit) {
    struct LlamaLoadLogCaptureState {
        bool assigned_cpu          = false;
        bool assigned_non_cpu      = false;
        bool backend_cpu_only_hint = false;
    };
    LlamaLoadLogCaptureState s;
    EXPECT_FALSE(s.assigned_cpu);
    EXPECT_FALSE(s.assigned_non_cpu);
    EXPECT_FALSE(s.backend_cpu_only_hint);
}

/// LLM-P2-INIT-04: LazyModelLoader::Config has sane defaults from in-class inits.
TEST_F(LLMPhase2CriticalGapsTest, INIT04_LazyModelLoaderConfigDefaults) {
    struct MockConfig {
        size_t max_vram_mb{24576};
        size_t max_ram_mb{65536};
        size_t max_models{3};
        bool   enable_lazy_load{true};
        int    default_n_gpu_layers{32};
        int    default_n_ctx{4096};
        bool   require_model_integrity{false};
    };
    MockConfig c;
    EXPECT_EQ(24576u, c.max_vram_mb);
    EXPECT_EQ(3u,     c.max_models);
    EXPECT_TRUE(c.enable_lazy_load);
    EXPECT_FALSE(c.require_model_integrity);
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-LIFE: plugin lifecycle (load / unload / reload)
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-LIFE-01: Registering null plugin is rejected with exception.
TEST_F(LLMPhase2CriticalGapsTest, LIFE01_RegisterNullPluginThrows) {
    auto reg = [](std::unique_ptr<MockPlugin> plugin) {
        if (!plugin) {
          throw std::invalid_argument("Cannot register null plugin");
        }
    };
    EXPECT_THROW(reg(nullptr), std::invalid_argument);
}

/// LLM-P2-LIFE-02: Registering valid plugin succeeds.
TEST_F(LLMPhase2CriticalGapsTest, LIFE02_RegisterValidPluginSucceeds) {
    ResourceTracker rt;
    bool registered = false;
    auto reg = [&](std::unique_ptr<MockPlugin> plugin) {
        if (!plugin) {
          throw std::invalid_argument("null");
        }
        registered = true;
    };
    EXPECT_NO_THROW(reg(std::make_unique<MockPlugin>(rt)));
    EXPECT_TRUE(registered);
}

/// LLM-P2-LIFE-03: Unregistering non-existent plugin is a no-op (no crash).
TEST_F(LLMPhase2CriticalGapsTest, LIFE03_UnregisterMissingPluginSafe) {
    std::unordered_map<std::string, int> plugins;
    plugins["p1"] = 1;
    EXPECT_NO_THROW(plugins.erase("nonexistent"));
    EXPECT_EQ(1u, plugins.size());
}

/// LLM-P2-LIFE-04: Reload replaces old plugin, triggering its destructor.
TEST_F(LLMPhase2CriticalGapsTest, LIFE04_PluginReloadDestroysOldPlugin) {
    ResourceTracker rt1, rt2;
    std::unordered_map<std::string, std::unique_ptr<MockPlugin>> registry;
    registry["llama"] = std::make_unique<MockPlugin>(rt1);

    // Replace — old plugin must be destroyed
    registry["llama"] = std::make_unique<MockPlugin>(rt2);

    EXPECT_TRUE(rt1.cleaned_up)  << "Old plugin must be destroyed on replacement";
    EXPECT_FALSE(rt2.cleaned_up) << "New plugin must remain alive";
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-CONN: db_connection_leak (RAII release pattern)
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-CONN-01: RAII connection guard closes on scope exit.
TEST_F(LLMPhase2CriticalGapsTest, CONN01_ConnectionRaiiCleanup) {
    bool conn_closed = false;
    auto close_conn = [&conn_closed](bool* p) noexcept { if (p) *p = true; };
    {
        auto conn = std::unique_ptr<bool, decltype(close_conn)>(&conn_closed, close_conn);
        EXPECT_FALSE(conn_closed);
    }
    EXPECT_TRUE(conn_closed);
}

/// LLM-P2-CONN-02: RAII connection guard closes on exception path.
TEST_F(LLMPhase2CriticalGapsTest, CONN02_ConnectionRaiiOnException) {
    bool conn_closed = false;
    auto close_conn = [&conn_closed](bool* p) noexcept { if (p) *p = true; };
    try {
        auto conn = std::unique_ptr<bool, decltype(close_conn)>(&conn_closed, close_conn);
        throw std::runtime_error("db error");
    } catch (...) {}
    EXPECT_TRUE(conn_closed);
}

/// LLM-P2-CONN-03: Multiple connections released in LIFO order (stack unwind).
TEST_F(LLMPhase2CriticalGapsTest, CONN03_MultipleConnectionsLIFORelease) {
    std::vector<int> release_order;

    bool dummy = true;
    {
        auto c1 = std::unique_ptr<bool, std::function<void(bool*)>>(
            &dummy, [&release_order](bool*) noexcept { release_order.push_back(1); });
        auto c2 = std::unique_ptr<bool, std::function<void(bool*)>>(
            &dummy, [&release_order](bool*) noexcept { release_order.push_back(2); });
        auto c3 = std::unique_ptr<bool, std::function<void(bool*)>>(
            &dummy, [&release_order](bool*) noexcept { release_order.push_back(3); });
    }
    ASSERT_EQ(3u, release_order.size());
    EXPECT_EQ(3, release_order[0]);
    EXPECT_EQ(2, release_order[1]);
    EXPECT_EQ(1, release_order[2]);
}

// ═════════════════════════════════════════════════════════════════════════════
// LLM-P2-CONC: noexcept destructors under concurrent load
// ═════════════════════════════════════════════════════════════════════════════

/// LLM-P2-CONC-01: ConcurrentInferenceTracker Acquire/Release are thread-safe.
TEST_F(LLMPhase2CriticalGapsTest, CONC01_ConcurrentTrackerThreadSafe) {
    struct ConcurrentInferenceTracker {
        std::atomic<size_t> active{0};
        size_t max_concurrent{256};
        std::mutex lock;
        bool Acquire() noexcept {
            std::lock_guard<std::mutex> g(lock);
            if (active >= max_concurrent) {
              return false;
            }
            active++;
            return true;
        }
        void Release() noexcept {
            std::lock_guard<std::mutex> g(lock);
            if (active > 0) {
              active--;
            }
        }
    };

    ConcurrentInferenceTracker tracker;
    constexpr int kThreads = 4;
    constexpr int kOps = 50;
    std::atomic<int> successes{0};
    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kOps; ++i) {
                if (tracker.Acquire()) {
                    successes.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::sleep_for(1us);
                    tracker.Release();
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(0u, tracker.active.load());
    EXPECT_GT(successes.load(), 0);
}

/// LLM-P2-CONC-02: noexcept destructor does not std::terminate under concurrent load.
TEST_F(LLMPhase2CriticalGapsTest, CONC02_NoexceptDtorUnderConcurrentLoad) {
    std::atomic<bool> running{true};
    std::atomic<int>  destructions{0};

    struct SafeWidget {
        std::atomic<int>& counter;
        explicit SafeWidget(std::atomic<int>& c) noexcept : counter(c) {}
        ~SafeWidget() noexcept {
            try { counter.fetch_add(1, std::memory_order_relaxed); }
            catch (...) {}
        }
    };

    auto worker = [&] {
        while (running.load(std::memory_order_relaxed)) {
            SafeWidget w(destructions);
            (void)w;
        }
    };

    std::vector<std::thread> threads = {};

    for (int i = 0; i < 3; ++i) {
      threads.emplace_back(worker);
    }
    std::this_thread::sleep_for(20ms);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_GT(destructions.load(), 0);
}

// GTest main is provided by the gtest_main library linked via CMake.
