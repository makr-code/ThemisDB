/**
 * @file test_wave5_llm_stubs.cpp
 * @brief Wave 5 Phase 1 stub-gap coverage tests.
 *
 * Covers the three Wave-5 Phase-1 gaps resolved in this delivery:
 *
 *  W5-SS-01  initializeStateStore() with a valid rocksdb_path opens a
 *            TransactionDB and creates the directory.
 *  W5-SS-02  initializeStateStore() returns false (not throws) when disabled.
 *  W5-SS-03  initializeStateStore() returns false (not throws) when path is empty.
 *  W5-DT-01  ILLMPlugin::generateDraftTokens() uses the injected
 *            GenerateDraftTokensFn when one is set.
 *  W5-DT-02  ILLMPlugin::generateDraftTokens() falls back to the byte-modulo
 *            heuristic when no fn is set and returns k tokens.
 *  W5-DT-03  Clearing the GenerateDraftTokensFn (nullptr) restores the
 *            heuristic without throwing.
 *  W5-TL-01  InferenceEngineEnhanced::setTargetLogitsFn() accepts a callable
 *            without throwing.
 *  W5-TL-02  setTargetLogitsFn(nullptr) clears the fn without throwing.
 *
 * Tests are deterministic and do not require a real LLM backend or GPU.
 *
 * @version 1.0.0-beta
 * @note CTest labels: llm;wave5;stubs
 */

#include <gtest/gtest.h>

#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_plugin_manager.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis { namespace llm { namespace tests {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Minimal concrete ILLMPlugin subclass — only overrides what tests need.
class MinimalPlugin : public ILLMPlugin {
public:
    [[nodiscard]] bool loadModel(const std::string& /*path*/, const json& /*config*/) override {
        return true;
    }
    void unloadModel() override {}
    [[nodiscard]] std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info;
        info.model_id = "minimal";
        info.is_loaded = true;
        return info;
    }
    [[nodiscard]] bool isModelLoaded() const override { return true; }
    [[nodiscard]] bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    [[nodiscard]] bool unloadLoRA(const std::string&) override { return true; }
    [[nodiscard]] std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    [[nodiscard]] std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    [[nodiscard]] bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
    [[nodiscard]] InferenceResponse generate(const InferenceRequest& /*request*/) override {
        InferenceResponse r;
        r.text    = "hello";
        r.success = true;
        return r;
    }
    [[nodiscard]] InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    [[nodiscard]] std::vector<float> embed(const std::string&) override { return {0.0f}; }
    [[nodiscard]] LLMCapabilities getCapabilities() const override { return {}; }
    [[nodiscard]] json getMemoryStats() const override { return {}; }
    [[nodiscard]] json getPerformanceStats() const override { return {}; }
};

// ═══════════════════════════════════════════════════════════════════════════
// W5-SS: initializeStateStore tests
// ═══════════════════════════════════════════════════════════════════════════

class Wave5StateStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = fs::temp_directory_path() /
                   "themis_wave5_test_db";
        fs::remove_all(db_path_);
    }

    void TearDown() override {
        // Release manager (and owned DB) before removing the directory.
        mgr_.reset();
        fs::remove_all(db_path_);
    }

    std::unique_ptr<LLMPluginManager> mgr_ =
        std::make_unique<LLMPluginManager>();
    fs::path db_path_;
};

/// W5-SS-01: disabled config returns false without opening any DB.
TEST_F(Wave5StateStoreTest, DisabledConfigReturnsFalse) {
    LLMPluginManager::SSMStateStoreConfig cfg;
    cfg.enabled = false;
    EXPECT_FALSE(mgr_->initializeStateStore(cfg));
}

/// W5-SS-02: empty path with enabled=true returns false (guards thrown exception).
TEST_F(Wave5StateStoreTest, EmptyPathReturnsFalse) {
    LLMPluginManager::SSMStateStoreConfig cfg;
    cfg.enabled      = true;
    cfg.rocksdb_path = "";  // invalid — must throw internally → caught → false
    EXPECT_FALSE(mgr_->initializeStateStore(cfg));
}

/// W5-SS-03: valid path creates the directory and initialises the state store.
TEST_F(Wave5StateStoreTest, ValidPathCreatesDirectoryAndStore) {
    LLMPluginManager::SSMStateStoreConfig cfg;
    cfg.enabled              = true;
    cfg.rocksdb_path         = db_path_.string();
    cfg.retention_window_ms  = 3600 * 1000;
    cfg.max_snapshots_per_session = 10;
    cfg.enable_compression   = false;
    cfg.sync_on_checkpoint   = false;

    // The call must succeed: it opens a TransactionDB and creates the store.
    const bool ok = mgr_->initializeStateStore(cfg);
    EXPECT_TRUE(ok) << "initializeStateStore() failed for path: " << db_path_;

    // The directory must exist (created by create_directories or by RocksDB).
    EXPECT_TRUE(fs::exists(db_path_))
        << "RocksDB directory was not created at: " << db_path_;
}

/// W5-SS-04: calling initializeStateStore() twice with the same path is safe.
TEST_F(Wave5StateStoreTest, SecondCallWithSamePathIsIdempotent) {
    LLMPluginManager::SSMStateStoreConfig cfg;
    cfg.enabled      = true;
    cfg.rocksdb_path = db_path_.string();
    cfg.enable_compression = false;

    const bool first = mgr_->initializeStateStore(cfg);
    // Second call on the same manager replaces the owned DB; should not crash.
    // (The existing owned_state_db_ is reset before re-opening.)
    if (first) {
        // We only call again if the first succeeded to avoid interference.
        EXPECT_NO_THROW(mgr_->initializeStateStore(cfg));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// W5-DT: generateDraftTokens / STUB #261 bridge tests
// ═══════════════════════════════════════════════════════════════════════════

class Wave5DraftTokensTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Always start with no injected fn so tests are isolated.
        ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);
        plugin_ = std::make_unique<MinimalPlugin>();
    }
    void TearDown() override {
        ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);
    }

    std::unique_ptr<MinimalPlugin> plugin_;
};

/// W5-DT-01: injected GenerateDraftTokensFn is invoked, overriding heuristic.
TEST_F(Wave5DraftTokensTest, InjectedFnIsUsed) {
    bool called = false;
    constexpr size_t kK    = 3;
    constexpr size_t kVsz  = 128;

    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [&](const InferenceRequest& /*req*/, size_t k, size_t vocab) {
            called = true;
            ILLMPlugin::DraftTokensResult r;
            r.vocab_size = vocab;
            for (size_t i = 0; i < k; ++i) {
                r.tokens.push_back(static_cast<int>(i + 1));
                r.logits.push_back(std::vector<float>(vocab, 0.0f));
            }
            return r;
        });

    InferenceRequest req;
    req.prompt     = "test";
    req.max_tokens = static_cast<int>(kK);

    const auto result = plugin_->generateDraftTokens(req, kK, kVsz);

    EXPECT_TRUE(called);
    ASSERT_EQ(result.tokens.size(), kK);
    EXPECT_EQ(result.vocab_size, kVsz);
}

/// W5-DT-02: without an injected fn, byte-modulo heuristic produces k tokens.
TEST_F(Wave5DraftTokensTest, HeuristicFallbackProducesKTokens) {
    constexpr size_t kK   = 4;
    constexpr size_t kVsz = 256;

    // generate() on MinimalPlugin returns "hello" (5 chars).
    InferenceRequest req;
    req.prompt     = "test";
    req.max_tokens = static_cast<int>(kK);

    const auto result = plugin_->generateDraftTokens(req, kK, kVsz);

    ASSERT_EQ(result.tokens.size(), kK)
        << "Heuristic must produce exactly k=" << kK << " token IDs";
    ASSERT_EQ(result.logits.size(), kK)
        << "Heuristic must produce exactly k=" << kK << " logit rows";
    EXPECT_EQ(result.vocab_size, kVsz);

    // Each token ID must be in [0, vocab_size).
    for (const int tok : result.tokens) {
        EXPECT_GE(tok, 0);
        EXPECT_LT(static_cast<size_t>(tok), kVsz);
    }

    // Each logit row must have exactly vocab_size entries.
    for (const auto& row : result.logits) {
        EXPECT_EQ(row.size(), kVsz);
    }
}

/// W5-DT-03: clearing the GenerateDraftTokensFn (nullptr) restores heuristic.
TEST_F(Wave5DraftTokensTest, ClearingFnRestoresHeuristic) {
    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [](const InferenceRequest&, size_t k, size_t v) {
            ILLMPlugin::DraftTokensResult r;
            r.vocab_size = v;
            r.tokens.assign(k, 99);
            r.logits.assign(k, std::vector<float>(v, 0.0f));
            return r;
        });

    EXPECT_NO_THROW(ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr));

    // After clearing, heuristic runs again — token IDs must not all be 99.
    InferenceRequest req;
    req.prompt     = "abc";
    req.max_tokens = 3;
    const auto result = plugin_->generateDraftTokens(req, 3u, 128u);
    ASSERT_EQ(result.tokens.size(), 3u);
    // At least the first token should differ from 99 (heuristic uses 'a'=97 % 128 = 97).
    EXPECT_NE(result.tokens[0], 99);
}

// ═══════════════════════════════════════════════════════════════════════════
// W5-TL: setTargetLogitsFn / STUB #262 bridge tests
// ═══════════════════════════════════════════════════════════════════════════

class Wave5TargetLogitsFnTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceEngineEnhanced::Config cfg;
        cfg.num_worker_threads         = 1;
        cfg.enable_batch_processing    = false;
        cfg.enable_context_caching     = false;
        cfg.enable_speculative_decoding = false;
        engine_ = std::make_unique<InferenceEngineEnhanced>(cfg);
    }

    void TearDown() override {
        // Clear fn before destroying engine to avoid dangling capture refs.
        engine_->setTargetLogitsFn(nullptr);
        engine_.reset();
    }

    std::unique_ptr<InferenceEngineEnhanced> engine_;
};

/// W5-TL-01: setTargetLogitsFn() accepts a valid callable without throwing.
TEST_F(Wave5TargetLogitsFnTest, RegisteringFnDoesNotThrow) {
    InferenceEngineEnhanced::TargetLogitsFn fn =
        [](const InferenceRequest&, size_t K, size_t vocab,
           std::shared_ptr<ILLMPlugin> /*target*/) {
            std::vector<std::vector<float>> mat(
                K + 1, std::vector<float>(vocab, 0.0f));
            return mat;
        };

    EXPECT_NO_THROW(engine_->setTargetLogitsFn(std::move(fn)));
}

/// W5-TL-02: setTargetLogitsFn(nullptr) clears the fn without throwing.
TEST_F(Wave5TargetLogitsFnTest, ClearingFnWithNullptrDoesNotThrow) {
    // Register then clear.
    engine_->setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t v, std::shared_ptr<ILLMPlugin>) {
            return std::vector<std::vector<float>>(
                K + 1, std::vector<float>(v, 0.0f));
        });

    EXPECT_NO_THROW(engine_->setTargetLogitsFn(nullptr));
}

/// W5-TL-03: replacing an existing fn with a new fn does not throw.
TEST_F(Wave5TargetLogitsFnTest, ReplacingFnDoesNotThrow) {
    engine_->setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t v, std::shared_ptr<ILLMPlugin>) {
            return std::vector<std::vector<float>>(K + 1, std::vector<float>(v, 0.0f));
        });

    EXPECT_NO_THROW(engine_->setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t v, std::shared_ptr<ILLMPlugin>) {
            return std::vector<std::vector<float>>(K + 1, std::vector<float>(v, 1.0f));
        }));
}

}}}  // namespace themis::llm::tests
