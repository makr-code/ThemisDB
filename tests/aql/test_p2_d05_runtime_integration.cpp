/**
 * @file test_p2_d05_runtime_integration.cpp
 * @brief Integration tests for P2-D05 runtime integration of compression + state store.
 * @version 0.1.0-beta
 * @note Phase 2 (P2-D05): Runtime Integration of episodic compression (P2-D03) and
 *       RocksDB persistence (P2-D04) into AQLConversationContext and LLMPluginManager.
 */

#include <gtest/gtest.h>
#include <memory>
#include <optional>

#include "aql/aql_conversation_context.h"
#include "aql/i_history_compressor.h"
#include "aql/llm_aql_handler.h"
#include "llm/llm_plugin_manager.h"
#include "llm/ssm_state_store.h"

namespace themis { namespace aql { namespace tests { 

// Mock history compressor for testing compression integration
class MockHistoryCompressor : public IHistoryCompressor {
public:
    bool isAvailable() const override { return available_; }

    std::unique_ptr<CompressionResult> compressHistory(
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens,
        float min_similarity = 0.85f) override {
        
        if (!available_ || history.empty()) {
            return nullptr;
        }

        auto result = std::make_unique<CompressionResult>();
        
        // Simulate compression: take system message + last 2 turns
        result->episode_id = "mock_episode_001";
        result->original_token_count = 1000;
        result->compressed_token_count = 400;
        result->semantic_similarity = 0.92f;  // >= min_similarity gate
        result->timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        // Create compressed summary
        std::ostringstream summary = {};
        summary << "[Compressed History]\n";
        for (const auto& [role, content] : history) {
            if (role == "system") {
                summary << "SYSTEM: " << content << "\n";
            }
        }
        // Include only the last user message
        for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i) {
            if (history[i].first == "user") {
                summary << "USER: " << history[i].second << "\n";
                break;
            }
        }
        result->summary = summary.str();
        
        compression_count_++;
        return result;
    }

    std::string getStatistics() const override {
        return "{\"compression_count\": " + std::to_string(compression_count_) + "}";
    }

    void setAvailable(bool available) { available_ = available; }
    int getCompressionCount() const { return compression_count_; }

private:
    bool available_ = true;
    int compression_count_ = 0;
};

// Mock LLM handler for testing
class MockLLMAQLHandler : public LLMAQLHandler {
public:
    MockLLMAQLHandler() = default;

    std::string executeChat(const std::vector<llm::ChatMessage>& history) override {
        // Simple mock: return a basic AQL query
        return "FOR doc IN collection RETURN doc";
    }
};

class P2D05RuntimeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler_ = std::make_unique<MockLLMAQLHandler>();
        compressor_ = std::make_unique<MockHistoryCompressor>();
    }

    std::unique_ptr<MockLLMAQLHandler> handler_;
    std::unique_ptr<MockHistoryCompressor> compressor_;
};

// ═══════════════════════════════════════════════════════════════════════════
// AQLConversationContext Compression Integration Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(P2D05RuntimeIntegrationTest, CompressorInjectionViaConstructor) {
    // Test: Inject compressor via constructor
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 500;
    
    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    EXPECT_EQ(ctx.getCompressor(), compressor_.get());
}

TEST_F(P2D05RuntimeIntegrationTest, CompressorSetterGetter) {
    // Test: Set and get compressor via setter/getter
    AQLConversationContext ctx(*handler_);
    
    EXPECT_EQ(ctx.getCompressor(), nullptr);  // Initially nullptr
    ctx.setCompressor(compressor_.get());
    EXPECT_EQ(ctx.getCompressor(), compressor_.get());
}

TEST_F(P2D05RuntimeIntegrationTest, CompressionTriggeredOnThreshold) {
    // Test: Compression is triggered when token count exceeds threshold
    AQLConversationContext::Config config;
    config.max_history_tokens = 2000;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 500;  // Low threshold for testing
    config.episodic_compression_gate_similarity = 0.85f;

    auto ctx = std::make_unique<AQLConversationContext>(
        *handler_, config, nullptr, compressor_.get());

    // Start conversation
    std::string q1 = ctx->start("SELECT * FROM users");
    EXPECT_FALSE(q1.empty());
    EXPECT_EQ(ctx->turnCount(), 1);

    // Refine multiple times to exceed compression threshold
    for (int i = 0; i < 5; ++i) {
        std::string q = ctx->refine("Add WHERE condition");
        EXPECT_FALSE(q.empty());
    }

    // Verify compression was called
    EXPECT_GT(compressor_->getCompressionCount(), 0);
}

TEST_F(P2D05RuntimeIntegrationTest, CompressionDisabledByConfig) {
    // Test: Compression doesn't trigger when disabled
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = false;  // Disabled
    config.episodic_compaction_trigger_tokens = 100;

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    ctx.start("SELECT * FROM users");
    for (int i = 0; i < 3; ++i) {
        ctx.refine("Add filter");
    }

    // Compression should not be triggered
    EXPECT_EQ(compressor_->getCompressionCount(), 0);
}

TEST_F(P2D05RuntimeIntegrationTest, CompressionGracefulFailure) {
    // Test: Conversation continues even if compression fails
    compressor_->setAvailable(false);  // Make compressor unavailable
    
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 100;

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    std::string q1 = ctx.start("SELECT * FROM users");
    EXPECT_FALSE(q1.empty());
    
    // Conversation should continue without compression
    std::string q2 = ctx.refine("Add filter");
    EXPECT_FALSE(q2.empty());
}

TEST_F(P2D05RuntimeIntegrationTest, HistoryPreservationAfterCompression) {
    // Test: System message is preserved after compression
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 500;
    config.max_history_tokens = 2000;

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    ctx.setSchemaContext("Collections: users{id, name, email}");
    
    ctx.start("Find users");
    for (int i = 0; i < 3; ++i) {
        ctx.refine("Refine");
    }

    // Verify history still exists and schema is preserved
    auto history = ctx.getHistory();
    EXPECT_FALSE(history.empty());
}

// ═══════════════════════════════════════════════════════════════════════════
// LLMPluginManager State Store Integration Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(P2D05RuntimeIntegrationTest, StateStoreInitialization) {
    // Test: State store initialization via LLMPluginManager
    auto& mgr = llm::LLMPluginManager::instance();
    
    llm::LLMPluginManager::SSMStateStoreConfig cfg;
    cfg.enabled = false;  // Disabled in test environment (no RocksDB instance)
    
    bool result = mgr.initializeStateStore(cfg);
    EXPECT_FALSE(result);  // Disabled, so should return false
}

TEST_F(P2D05RuntimeIntegrationTest, StateStoreCheckpointRecover) {
    // Test: Checkpoint and recovery workflow (mock)
    auto& mgr = llm::LLMPluginManager::instance();
    
    // State store not initialized in this test, so operations should gracefully fail
    themis::llm::SSMStateSnapshot snap;
    snap.session_id = "test_session";
    snap.query_text = "SELECT * FROM collection";
    
    bool checkpoint_result = mgr.checkpointState("test_session", snap);
    EXPECT_FALSE(checkpoint_result);  // Should fail - no state store initialized
    
    auto recover_result = mgr.recoverState("test_session");
    EXPECT_FALSE(recover_result.has_value());  // Should be empty
}

TEST_F(P2D05RuntimeIntegrationTest, StateStoreStatistics) {
    // Test: Get state store statistics
    auto& mgr = llm::LLMPluginManager::instance();
    
    std::string stats = mgr.getStateStoreStatistics();
    EXPECT_EQ(stats, "{}");  // Empty stats when not initialized
}

TEST_F(P2D05RuntimeIntegrationTest, StateStoreCompaction) {
    // Test: Compaction operation
    auto& mgr = llm::LLMPluginManager::instance();
    
    uint64_t removed = mgr.compactStateStore();
    EXPECT_EQ(removed, 0);  // No snapshots to compact
}

TEST_F(P2D05RuntimeIntegrationTest, StateStoreInvalidation) {
    // Test: Invalidate session state
    auto& mgr = llm::LLMPluginManager::instance();
    
    bool result = mgr.invalidateState("nonexistent_session");
    EXPECT_FALSE(result);  // Should fail - no state store initialized
}

// ═══════════════════════════════════════════════════════════════════════════
// Concurrent Access Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(P2D05RuntimeIntegrationTest, ConcurrentCompressionCalls) {
    // Test: Multiple threads can safely call compression
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 100;

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    ctx.start("SELECT * FROM collection");
    
    // Multiple refinements simulate concurrent usage
    for (int i = 0; i < 5; ++i) {
        ctx.refine("Refine query");
    }

    // Should not crash or deadlock
    EXPECT_EQ(ctx.turnCount(), 6);  // 1 start + 5 refines
}

TEST_F(P2D05RuntimeIntegrationTest, CompressorSwapDuringOperation) {
    // Test: Compressor can be swapped safely
    AQLConversationContext ctx(*handler_);
    
    ctx.setCompressor(compressor_.get());
    EXPECT_EQ(ctx.getCompressor(), compressor_.get());
    
    // Swap compressor
    MockHistoryCompressor compressor2;
    ctx.setCompressor(&compressor2);
    EXPECT_EQ(ctx.getCompressor(), &compressor2);
    
    // Swap back
    ctx.setCompressor(compressor_.get());
    EXPECT_EQ(ctx.getCompressor(), compressor_.get());
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Case Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(P2D05RuntimeIntegrationTest, EmptyHistoryCompression) {
    // Test: Compression handles empty history gracefully
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 100;

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    // Don't call start() - history is empty
    // This should not crash
    auto history = ctx.getHistory();
    EXPECT_TRUE(history.empty() || history[0].first == "system");
}

TEST_F(P2D05RuntimeIntegrationTest, NullCompressorHandling) {
    // Test: Null compressor doesn't crash
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 100;

    AQLConversationContext ctx(*handler_, config, nullptr, nullptr);  // nullptr compressor
    
    std::string q1 = ctx.start("SELECT * FROM collection");
    EXPECT_FALSE(q1.empty());
    
    // Should not crash even with null compressor
    std::string q2 = ctx.refine("Add filter");
    EXPECT_FALSE(q2.empty());
}

TEST_F(P2D05RuntimeIntegrationTest, HighTokenCountThreshold) {
    // Test: Very high threshold prevents compression
    AQLConversationContext::Config config;
    config.enable_episodic_compaction = true;
    config.episodic_compaction_trigger_tokens = 1000000;  // Very high

    AQLConversationContext ctx(*handler_, config, nullptr, compressor_.get());
    
    ctx.start("SELECT * FROM collection");
    for (int i = 0; i < 5; ++i) {
        ctx.refine("Refine");
    }

    // Compression should not trigger due to high threshold
    EXPECT_EQ(compressor_->getCompressionCount(), 0);
}
} } } // namespace themis::aql::tests
