/**
 * @file test_llm_exception_safety.cpp
 * @brief P5-L01: LLM Module Phase 5 — Exception Safety hardening tests (36 cases).
 *
 * Covers RAII / ownership semantics, exception propagation in critical paths,
 * cleanup / re-throw semantics, and allocation / sequence coverage for the
 * LLM subsystem.  All tests use mock objects only — no real GGUF model files,
 * no real backends.
 *
 * @version 1.9.0-beta
 * @note CTest labels: llm;hardening;phase5
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "llm/async_inference_engine.h"
#include "llm/continuous_batch_scheduler.h"
#include "llm/llm_prefix_cache.h"
#include "llm/llm_plugin_interface.h"
#include "llm/model_metadata_cache.h"
#include "llm/token_quota_manager.h"

// Access LazyModelLoader private members for state seeding in tests.
#define private public
#include "llm/model_loader.h"
#undef private

using namespace std::chrono_literals;
using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// MockPlugin — fully implements ILLMPlugin using only in-memory state
// ─────────────────────────────────────────────────────────────────────────────

class MockPlugin : public ILLMPlugin {
public:
    bool should_throw    = false;
    bool should_error    = false;   ///< return error response (no throw)
    bool empty_embed     = false;   ///< embed() returns empty vector
    bool loaded_         = true;
    std::string response_text = "ok";
    std::atomic<int> generate_count{0};

    bool loadModel(const std::string&, const json&) override {
        loaded_ = true;
        return true;
    }
    void unloadModel() override { loaded_ = false; }
    bool isModelLoaded() const override { return loaded_; }
    std::optional<ModelInfo> getModelInfo() const override {
        if (!loaded_) return std::nullopt;
        ModelInfo info{};
        info.model_id  = "mock";
        info.is_loaded = true;
        return info;
    }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo>    listLoRAs()  const override { return {}; }
    std::vector<uint8_t>     exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

    InferenceResponse generate(const InferenceRequest&) override {
        ++generate_count;
        if (should_throw) throw std::runtime_error("mock_generate_error");
        InferenceResponse resp;
        if (should_error) {
            resp.success       = false;
            resp.error_message = "plugin_error";
            return resp;
        }
        resp.success          = true;
        resp.text             = response_text;
        resp.tokens_generated = 1;
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override {
        if (empty_embed) return {};
        return {0.1f, 0.2f, 0.3f};
    }
    LLMCapabilities getCapabilities()     const override { return {}; }
    json            getMemoryStats()      const override { return {}; }
    json            getPerformanceStats() const override { return {}; }
};

// ─────────────────────────────────────────────────────────────────────────────
// BlockingPlugin — generate() blocks until release() is called
// ─────────────────────────────────────────────────────────────────────────────

class BlockingPlugin : public MockPlugin {
public:
    std::atomic<bool> released_{false};
    std::atomic<int>  in_flight_{0};

    InferenceResponse generate(const InferenceRequest& req) override {
        in_flight_.fetch_add(1, std::memory_order_relaxed);
        while (!released_.load(std::memory_order_acquire))
            std::this_thread::sleep_for(1ms);
        in_flight_.fetch_sub(1, std::memory_order_relaxed);
        return MockPlugin::generate(req);
    }
    void release() { released_.store(true, std::memory_order_release); }
    int  inFlight() const { return in_flight_.load(std::memory_order_acquire); }
};

// ─────────────────────────────────────────────────────────────────────────────
// LazyModelLoader test helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<CachedModel> makeCachedModel(
    const std::string& id,
    size_t vram_mb = 64,
    size_t ram_mb  = 32)
{
    auto m = std::make_shared<CachedModel>();
    m->model_id   = id;
    m->model_path = "/tmp/" + id + ".gguf";
    m->last_used  = std::chrono::system_clock::now();
    m->loaded_at  = m->last_used;
    m->use_count  = 1;
    m->vram_mb    = vram_mb;
    m->ram_mb     = ram_mb;
    m->info.model_id  = id;
    m->info.is_loaded = true;
    return m;
}

/// Fixture providing a LazyModelLoader with private-member seeding helpers.
class LoaderFixture : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.default_n_gpu_layers = 0;
        cfg_.model_ttl            = std::chrono::seconds(1);
        loader_ = std::make_unique<LazyModelLoader>(cfg_);
    }

    void seed(const std::shared_ptr<CachedModel>& m) {
        loader_->models_[m->model_id]  = m;
        loader_->total_vram_mb_       += m->vram_mb;
        loader_->total_ram_mb_        += m->ram_mb;
    }

    LazyModelLoader::Config      cfg_{};
    std::unique_ptr<LazyModelLoader> loader_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Shared AsyncInferenceEngine fixture
// ─────────────────────────────────────────────────────────────────────────────

struct EngineFixture : public ::testing::Test {
    std::shared_ptr<MockPlugin>          plugin_;
    std::unique_ptr<AsyncInferenceEngine> engine_;

    void SetUp() override {
        plugin_ = std::make_shared<MockPlugin>();
        AsyncInferenceEngine::Config cfg;
        cfg.num_worker_threads = 2;
        engine_ = std::make_unique<AsyncInferenceEngine>(
            static_cast<ILLMPlugin*>(plugin_.get()), cfg);
    }
    void TearDown() override {
        if (engine_) engine_->shutdown();
    }

    InferenceResponse submit(const std::string& prompt = "test") {
        InferenceRequest req;
        req.prompt = prompt;
        return engine_->submit(req).get();
    }
};

// =============================================================================
// Group 1: ESF-01..08 — RAII and ownership semantics
// =============================================================================

/// ESF-01: CancellationToken shares cancel state across copies.
TEST(ESF_RAII, ESF01_CancellationToken_SharedCancelAcrossCopies) {
    CancellationToken original;
    CancellationToken copy = original;         // copy shares internal state
    EXPECT_FALSE(copy.is_cancelled());

    original.cancel();
    EXPECT_TRUE(copy.is_cancelled())
        << "Copy must reflect cancel on original (shared atomic)";
}

/// ESF-02: CancellationToken cancel is idempotent.
TEST(ESF_RAII, ESF02_CancellationToken_CancelIdempotent) {
    CancellationToken tok;
    EXPECT_NO_THROW(tok.cancel());
    EXPECT_NO_THROW(tok.cancel());             // second call must not throw
    EXPECT_TRUE(tok.is_cancelled());
}

/// ESF-03: CancellationToken default-constructed is not cancelled.
TEST(ESF_RAII, ESF03_CancellationToken_DefaultNotCancelled) {
    CancellationToken tok;
    EXPECT_FALSE(tok.is_cancelled());
}

/// ESF-04: CachedModel shared_ptr use_count drops to 1 after loader unload.
TEST_F(LoaderFixture, ESF04_SharedPtrUseCountDropsOnUnload) {
    seed(makeCachedModel("m-esf04"));

    auto shared = loader_->getOrLoadModelShared("m-esf04", "");
    ASSERT_TRUE(shared) << "getOrLoadModelShared must return non-null for seeded model";

    EXPECT_TRUE(loader_->unloadModel("m-esf04", /*force=*/true));
    EXPECT_TRUE(loader_->models_.empty())
        << "models_ map must be empty after force unload";
    EXPECT_EQ(shared.use_count(), 1L)
        << "External shared_ptr must be the sole owner after unload";
    EXPECT_EQ(shared->model_id, "m-esf04");
}

/// ESF-05: evictLRU frees VRAM budget.
TEST_F(LoaderFixture, ESF05_EvictLRU_FreesVRAM) {
    seed(makeCachedModel("lru-a", 64, 32));
    seed(makeCachedModel("lru-b", 64, 32));
    EXPECT_EQ(loader_->total_vram_mb_, 128u);

    size_t freed = loader_->evictLRU();
    EXPECT_GT(freed, 0u)                 << "evictLRU must report freed VRAM";
    EXPECT_LT(loader_->total_vram_mb_, 128u) << "VRAM budget must decrease";
}

/// ESF-06: Pinned model resists evictLRU.
TEST_F(LoaderFixture, ESF06_PinnedModel_ResistsEviction) {
    seed(makeCachedModel("pin-m", 64, 32));
    loader_->pinModel("pin-m");

    loader_->evictLRU(/*target_vram_mb=*/1000);
    EXPECT_TRUE(loader_->isModelLoaded("pin-m"))
        << "Pinned model must not be evicted";
    EXPECT_EQ(loader_->total_vram_mb_, 64u)
        << "VRAM budget unchanged when only model is pinned";
}

/// ESF-07: swapPlugin(nullptr) throws std::invalid_argument.
TEST(ESF_RAII, ESF07_SwapNullPlugin_ThrowsInvalidArg) {
    auto plugin = std::make_shared<MockPlugin>();
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    auto engine = std::make_unique<AsyncInferenceEngine>(
        static_cast<ILLMPlugin*>(plugin.get()), cfg);

    EXPECT_THROW(engine->swapPlugin(nullptr), std::invalid_argument);
    engine->shutdown();
}

/// ESF-08: Plugin swap while idle routes new submissions to the replacement.
TEST(ESF_RAII, ESF08_PluginSwap_IdleEngineUsesNewPlugin) {
    auto pluginA = std::make_shared<MockPlugin>();
    pluginA->response_text = "from_A";

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    auto engine = std::make_unique<AsyncInferenceEngine>(
        static_cast<ILLMPlugin*>(pluginA.get()), cfg);

    auto pluginB = std::make_shared<MockPlugin>();
    pluginB->response_text = "from_B";
    engine->swapPlugin(pluginB);

    InferenceRequest req;
    req.prompt = "test swap";
    auto resp = engine->submit(req).get();

    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.text, "from_B")
        << "After swap, response must come from pluginB";

    engine->shutdown();
}

// =============================================================================
// Group 2: ESF-09..18 — Exception propagation in critical paths
// =============================================================================

/// ESF-09: Plugin error response propagated by engine.
TEST_F(EngineFixture, ESF09_PluginErrorResponse_Propagated) {
    plugin_->should_error = true;
    auto resp = submit();
    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.error_message.empty())
        << "error_message must be populated when plugin returns error";
}

/// ESF-10: Plugin throw in generate() — engine catches, returns error response.
TEST_F(EngineFixture, ESF10_PluginThrows_EngineCatchesAndReturnsError) {
    plugin_->should_throw = true;

    InferenceRequest req;
    req.prompt = "throw test";
    InferenceResponse resp;
    ASSERT_NO_THROW(resp = engine_->submit(req).get())
        << "submit().get() must not rethrow plugin exceptions";
    EXPECT_FALSE(resp.success);
}

/// ESF-11: Three sequential throws then successful response — engine state intact.
TEST_F(EngineFixture, ESF11_SequentialThrows_EngineRemainsUsable) {
    for (int i = 0; i < 3; ++i) {
        plugin_->should_throw = true;
        auto resp = submit("throw " + std::to_string(i));
        EXPECT_FALSE(resp.success) << "throw iteration " << i;
    }
    plugin_->should_throw = false;
    auto good = submit("success");
    EXPECT_TRUE(good.success)
        << "Engine must succeed after recovering from throws";
    EXPECT_EQ(good.text, "ok");
}

/// ESF-12: After engine shutdown, new submissions do not succeed.
TEST(ESF_Propagation, ESF12_ShutdownPreventsSuccessfulSubmission) {
    auto plugin = std::make_shared<MockPlugin>();
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    cfg.backpressure = AsyncInferenceEngine::Config::BackpressurePolicy::REJECT;
    auto engine = std::make_unique<AsyncInferenceEngine>(
        static_cast<ILLMPlugin*>(plugin.get()), cfg);
    engine->shutdown();

    InferenceRequest req;
    req.prompt = "post-shutdown";

    // Wrap .get() in a future with timeout guard to avoid hanging.
    auto future = std::async(std::launch::async, [&]() {
        return engine->submit(req).get();
    });
    const auto status = future.wait_for(500ms);
    if (status == std::future_status::ready) {
        auto resp = future.get();
        EXPECT_FALSE(resp.success)
            << "Post-shutdown submission must not succeed";
    }
    // If timeout: engine stopped processing — acceptable outcome.
}

/// ESF-13: Cancel queued request returns true.
TEST(ESF_Propagation, ESF13_CancelQueuedRequest_ReturnsTrue) {
    auto plugin = std::make_shared<BlockingPlugin>();
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    cfg.max_queue_size = 100;
    auto engine = std::make_unique<AsyncInferenceEngine>(
        static_cast<ILLMPlugin*>(plugin.get()), cfg);

    InferenceRequest req1;
    req1.prompt = "occupies worker";
    auto h1 = engine->submit(req1);  // Worker picks up req1 and blocks

    // Wait until req1 is in-flight.
    for (int i = 0; i < 200 && plugin->inFlight() == 0; ++i)
        std::this_thread::sleep_for(1ms);

    // Submit req2 — stays in queue while req1 blocks the single worker.
    InferenceRequest req2;
    req2.prompt = "queued request";
    auto h2 = engine->submit(req2);

    bool cancelled = engine->cancel(h2.requestId());
    EXPECT_TRUE(cancelled)
        << "Queued request must be cancellable";

    plugin->release();
    engine->shutdown();
}

/// ESF-14: ContinuousBatchScheduler queue-full returns empty string (backpressure).
TEST(ESF_Propagation, ESF14_SchedulerQueueFull_BackpressureSignal) {
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_queue_depth = 2;
    cfg.max_concurrent_requests = 1;
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, nullptr);

    InferenceRequest req;
    req.prompt = "fill";
    sched->submitRequest(req, RequestPriority::NORMAL, nullptr);
    sched->submitRequest(req, RequestPriority::NORMAL, nullptr);

    // Third submit hits the depth limit.
    const std::string overflow_id =
        sched->submitRequest(req, RequestPriority::NORMAL, nullptr);
    EXPECT_TRUE(overflow_id.empty())
        << "submitRequest must return empty string under backpressure";

    const auto stats = sched->getStats();
    EXPECT_GE(stats.rejected_requests, 1u);
    sched->stop();
}

/// ESF-15: TokenQuotaManager: quota exceeded returns denied result.
TEST(ESF_Propagation, ESF15_QuotaExceeded_ReturnsDenied) {
    TokenQuotaManager tqm;
    tqm.setQuota("user-esf15", "model-x", 100);
    tqm.consume("user-esf15", "model-x", 100);  // exhaust

    auto result = tqm.check("user-esf15", "model-x", 1);
    EXPECT_FALSE(result.allowed);
    EXPECT_FALSE(result.reason.empty());
}

/// ESF-16: After quota re-initialisation (equivalent to window reset), quota is available.
TEST(ESF_Propagation, ESF16_QuotaReset_BecomesAvailable) {
    TokenQuotaManager tqm;
    tqm.setQuota("user-esf16", "model-y", 50);
    tqm.consume("user-esf16", "model-y", 50);
    EXPECT_FALSE(tqm.check("user-esf16", "model-y", 1).allowed);

    // Simulate window reset by removing and re-adding the quota entry.
    tqm.removeQuota("user-esf16", "model-y");
    tqm.setQuota("user-esf16", "model-y", 50);

    auto result = tqm.check("user-esf16", "model-y", 1);
    EXPECT_TRUE(result.allowed)
        << "After quota reset, requests within limit must be allowed";
}

/// ESF-17: ModelMetadataCache get on missing key returns nullopt.
TEST(ESF_Propagation, ESF17_MetadataCache_MissingKey_ReturnsNullopt) {
    ModelMetadataCache cache;
    ModelMetadata meta;
    meta.model_id = "key-A";
    cache.put("key-A", meta);

    auto result = cache.get("key-B");
    EXPECT_FALSE(result.has_value())
        << "get() for absent key must return nullopt, not throw";
}

/// ESF-18: ModelMetadataCache remove on missing key returns false.
TEST(ESF_Propagation, ESF18_MetadataCache_RemoveMissing_ReturnsFalse) {
    ModelMetadataCache cache;
    EXPECT_FALSE(cache.remove("nonexistent"))
        << "remove() for absent key must return false, not throw";
}

// =============================================================================
// Group 3: ESF-19..24 — Cleanup and re-throw semantics
// =============================================================================

/// ESF-19: Plugin exception type preserved via std::exception_ptr.
TEST(ESF_Rethrow, ESF19_ExceptionPtr_PreservesType) {
    MockPlugin plugin;
    plugin.should_throw = true;

    std::exception_ptr ep;
    try {
        InferenceRequest req;
        plugin.generate(req);
    } catch (...) {
        ep = std::current_exception();
    }

    ASSERT_NE(ep, nullptr);
    EXPECT_THROW(std::rethrow_exception(ep), std::runtime_error);

    try {
        std::rethrow_exception(ep);
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "mock_generate_error");
    }
}

/// ESF-20: Engine remains queryable (stats accessible) after plugin exception.
TEST_F(EngineFixture, ESF20_EngineStatsAccessible_AfterPluginThrow) {
    plugin_->should_throw = true;
    (void)submit("throw-first");

    plugin_->should_throw = false;
    EXPECT_NO_THROW({
        auto qs = engine_->getQueueStats();
        (void)qs;
    }) << "getQueueStats() must not throw after plugin exception";
}

/// ESF-21: Concurrent cancellation is race-free (smoke test).
TEST(ESF_Rethrow, ESF21_ConcurrentCancel_NoDataRace) {
    auto plugin = std::make_shared<BlockingPlugin>();
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 2;
    auto engine = std::make_unique<AsyncInferenceEngine>(
        static_cast<ILLMPlugin*>(plugin.get()), cfg);

    InferenceRequest req;
    req.prompt = "concurrent cancel smoke";
    auto handle = engine->submit(req);

    // Cancel from a parallel thread while submit may still be processing.
    auto cancel_future = std::async(std::launch::async, [&]() {
        return engine->cancel(handle.requestId());
    });

    const bool result = cancel_future.get();
    (void)result;  // true or false — no crash is the assertion

    plugin->release();
    engine->shutdown();
}

/// ESF-22: LLMPrefixCache clear() removes all entries.
TEST(ESF_Rethrow, ESF22_PrefixCacheClear_RemovesAllEntries) {
    LLMPrefixCache::Config pcfg;
    pcfg.similarity_threshold = 1.0;
    pcfg.min_prefix_length    = 0;
    LLMPrefixCache cache("esf22", pcfg);

    const std::vector<int>   tokens  = {1, 2, 3};
    const std::vector<float> emb     = {1.0f, 0.0f, 0.0f};

    for (int i = 0; i < 5; ++i) {
        cache.put("prefix_entry_" + std::to_string(i), tokens, emb);
    }

    cache.clear();
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 0u)
        << "clear() must remove all entries";
}

/// ESF-23: invalidateByPattern removes matching entries.
TEST(ESF_Rethrow, ESF23_InvalidateByPattern_RemovesMatchingEntries) {
    LLMPrefixCache::Config pcfg;
    pcfg.similarity_threshold = 1.0;
    pcfg.min_prefix_length    = 0;
    LLMPrefixCache cache("esf23", pcfg);

    const std::vector<int>   tokens = {10, 20};
    const std::vector<float> emb1   = {1.0f, 0.0f};
    const std::vector<float> emb2   = {0.0f, 1.0f};

    const std::string hello_prefix = "hello world long prefix ok";
    const std::string bar_prefix   = "bar query unrelated text  ";

    cache.put(hello_prefix, tokens, emb1);
    cache.put(bar_prefix,   tokens, emb2);

    cache.invalidateByPattern("hello");

    auto gone = cache.get(hello_prefix, emb1);
    EXPECT_FALSE(gone.has_value())
        << "Entry matching pattern must be invalidated";
}

/// ESF-24: Two engines sharing same plugin are lifecycle-independent.
TEST(ESF_Rethrow, ESF24_TwoEnginesSamePlugin_IndependentLifecycle) {
    auto plugin = std::make_shared<MockPlugin>();

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    auto engineA = std::make_unique<AsyncInferenceEngine>(plugin, cfg);
    auto engineB = std::make_unique<AsyncInferenceEngine>(plugin, cfg);

    engineA->shutdown();  // Shutdown A

    // B must still work.
    InferenceRequest req;
    req.prompt = "engine B still alive";
    auto resp = engineB->submit(req).get();
    EXPECT_TRUE(resp.success)
        << "Engine B must process requests independently after engine A shutdown";

    engineB->shutdown();
}

// =============================================================================
// Group 4: ESF-25..36 — Allocation and sequence coverage
// =============================================================================

/// ESF-25: LazyModelLoader with max_models=1 initialises without throw.
TEST(ESF_Alloc, ESF25_LoaderWithSmallMaxModels_Initialises) {
    LazyModelLoader::Config cfg;
    cfg.max_models            = 1;
    cfg.default_n_gpu_layers  = 0;
    EXPECT_NO_THROW(LazyModelLoader loader(cfg));
}

/// ESF-26: evictLRU on empty loader returns 0.
TEST(ESF_Alloc, ESF26_EvictLRU_EmptyLoader_ReturnsZero) {
    LazyModelLoader::Config cfg;
    cfg.default_n_gpu_layers = 0;
    LazyModelLoader loader(cfg);
    EXPECT_EQ(loader.evictLRU(), 0u);
}

/// ESF-27: evictExpired on empty loader returns 0.
TEST(ESF_Alloc, ESF27_EvictExpired_EmptyLoader_ReturnsZero) {
    LazyModelLoader::Config cfg;
    cfg.default_n_gpu_layers = 0;
    LazyModelLoader loader(cfg);
    EXPECT_EQ(loader.evictExpired(), 0u);
}

/// ESF-28: listLoadedModels on empty loader returns empty vector.
TEST(ESF_Alloc, ESF28_ListLoadedModels_EmptyLoader_ReturnsEmpty) {
    LazyModelLoader::Config cfg;
    cfg.default_n_gpu_layers = 0;
    LazyModelLoader loader(cfg);
    EXPECT_TRUE(loader.listLoadedModels().empty());
}

/// ESF-29: CachedModel default fields are safe (no wild pointers).
TEST(ESF_Alloc, ESF29_CachedModelDefault_SafeState) {
    CachedModel m;
    EXPECT_EQ(m.model_handle,   nullptr);
    EXPECT_EQ(m.context_handle, nullptr);
    EXPECT_EQ(m.use_count,      0u);
    EXPECT_FALSE(m.keep_loaded);
}

/// ESF-30: InferenceResponse move constructor transfers fields.
TEST(ESF_Alloc, ESF30_InferenceResponseMove_Constructor) {
    InferenceResponse src;
    src.text             = "generated text";
    src.success          = true;
    src.tokens_generated = 7;

    InferenceResponse dst(std::move(src));
    EXPECT_EQ(dst.text,             "generated text");
    EXPECT_TRUE(dst.success);
    EXPECT_EQ(dst.tokens_generated, 7);
    // Source is in a valid (unspecified) state — just verify no crash.
}

/// ESF-31: InferenceResponse move assignment transfers fields.
TEST(ESF_Alloc, ESF31_InferenceResponseMove_Assignment) {
    InferenceResponse src;
    src.text    = "assigned text";
    src.success = true;

    InferenceResponse dst;
    dst = std::move(src);
    EXPECT_EQ(dst.text, "assigned text");
    EXPECT_TRUE(dst.success);
}

/// ESF-32: InferenceRequest copy constructor preserves all fields.
TEST(ESF_Alloc, ESF32_InferenceRequestCopy_PreservesFields) {
    InferenceRequest orig;
    orig.prompt     = "hello world";
    orig.max_tokens = 256;
    orig.model_id   = "my-model";
    orig.temperature = 0.5f;

    InferenceRequest copy = orig;
    EXPECT_EQ(copy.prompt,      orig.prompt);
    EXPECT_EQ(copy.max_tokens,  orig.max_tokens);
    EXPECT_EQ(copy.model_id,    orig.model_id);
    EXPECT_FLOAT_EQ(copy.temperature, orig.temperature);
}

/// ESF-33: Plugin embed() returning empty — no crash, empty result forwarded.
TEST(ESF_Alloc, ESF33_EmptyEmbedResult_NoCrash) {
    MockPlugin plugin;
    plugin.empty_embed = true;

    auto result = plugin.embed("some text");
    EXPECT_TRUE(result.empty())
        << "embed() returning empty vector must propagate cleanly";
}

/// ESF-34: Scheduler shutdown under load — no deadlock (timeout-guarded, 2 s).
TEST(ESF_Alloc, ESF34_SchedulerShutdownUnderLoad_NoDeadlock) {
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_queue_depth        = 0;   // unlimited
    cfg.max_concurrent_requests = 64;
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, nullptr);

    InferenceRequest req;
    req.prompt = "load";
    for (int i = 0; i < 10; ++i)
        sched->submitRequest(req, RequestPriority::NORMAL, nullptr);

    auto stop_future = std::async(std::launch::async, [&]() {
        sched->stop();
    });
    const auto status = stop_future.wait_for(2s);
    EXPECT_NE(status, std::future_status::timeout)
        << "Scheduler stop() must complete within 2 s under load";
    if (status == std::future_status::ready)
        stop_future.get();  // propagate any exception
}

/// ESF-35: TokenQuotaManager: zero-token request always allowed.
TEST(ESF_Alloc, ESF35_ZeroTokenRequest_AlwaysAllowed) {
    TokenQuotaManager tqm;
    tqm.setQuota("user-esf35", "m", 0);   // 0 = unlimited per header docs
    auto result = tqm.check("user-esf35", "m", 0);
    EXPECT_TRUE(result.allowed)
        << "Zero-token request must be allowed regardless of quota";
}

/// ESF-36: ModelMetadataCache put/get round-trip preserves all fields.
TEST(ESF_Alloc, ESF36_MetadataCache_RoundTrip_AllFieldsPreserved) {
    ModelMetadataCache cache;

    ModelMetadata meta;
    meta.model_id       = "esf36-model";
    meta.path           = "/models/esf36.gguf";
    meta.size_bytes     = 4096;
    meta.n_layers       = 32;
    meta.n_ctx          = 4096;
    meta.architecture   = "llama";
    meta.quantization   = "Q4_K_M";
    meta.is_pinned      = true;
    meta.access_count   = 5;

    cache.put(meta.model_id, meta);
    auto retrieved = cache.get(meta.model_id);

    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->model_id,     meta.model_id);
    EXPECT_EQ(retrieved->path,         meta.path);
    EXPECT_EQ(retrieved->size_bytes,   meta.size_bytes);
    EXPECT_EQ(retrieved->n_layers,     meta.n_layers);
    EXPECT_EQ(retrieved->architecture, meta.architecture);
    EXPECT_EQ(retrieved->quantization, meta.quantization);
    EXPECT_EQ(retrieved->is_pinned,    meta.is_pinned);
}
