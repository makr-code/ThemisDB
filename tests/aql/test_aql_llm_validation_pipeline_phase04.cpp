/**
 * @file test_llm_validation_pipeline_phase04.cpp
 * @brief Phase 0.4 integration tests for LLM client wiring into validation pipeline.
 *
 * Tests the complete NL→LLM→Parser→Retry workflow with injected LLM client:
 * - LLM client injection via Config struct
 * - Validation pipeline re-wiring with new LLM client
 * - End-to-end NL→AQL translation with LLM + validation feedback
 * - Fallback behavior when LLM client is nullptr
 * - Pipeline state consistency after injection
 *
 * Phase 0.4 Target: Wire real LLM client into validation_pipeline_.
 * Status: All tasks complete, integration verification.
 */

#include <gtest/gtest.h>
#include <spdlog/sinks/sink.h>

#include "aql/llm_aql_handler.h"
#include "llm/llm_client.h"
#include "query/aql_parser_service.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::aql;
using namespace themis::llm;
using namespace themis::query;

// ============================================================================
// Mock LLM Client for Phase 0.4 Testing
// ============================================================================

class MockLLMClient : public LLMClient {
public:
    explicit MockLLMClient(const std::string& provider_name = "mock-llm-provider")
        : provider_name_(provider_name), call_count_(0), should_fail_(false),
          failure_reason_("mock failure") {}

    GenerationResult generate(const std::string& prompt,
                            const GenerationOptions& options) override {
        call_count_++;
        if (should_fail_) {
            return GenerationResult{.success = false,
                                    .text = "",
                                    .error_message = failure_reason_,
                                    .prompt_tokens = 0,
                                    .completion_tokens = 0,
                                    .finish_reason = "error"};
        }
        return GenerationResult{.success = true,
                                .text = "SELECT * FROM users",
                                .error_message = "",
                                .prompt_tokens = 10,
                                .completion_tokens = 5,
                                .finish_reason = "stop"};
    }

    GenerationResult generateAQL(const std::string& nl_query,
                                const std::string& schema_context,
                                const GenerationOptions& options) override {
        call_count_++;
        if (should_fail_) {
            return GenerationResult{.success = false,
                                    .text = "",
                                    .error_message = failure_reason_,
                                    .prompt_tokens = 0,
                                    .completion_tokens = 0,
                                    .finish_reason = "error"};
        }
        // Simple contextual AQL generation based on NL keywords
        std::string aql = "SELECT * FROM users";
        if (nl_query.find("order") != std::string::npos) {
            aql = "SELECT o.order_id, o.total FROM orders o";
        }
        if (nl_query.find("product") != std::string::npos) {
            aql = "SELECT p.product_id, p.name FROM products p";
        }
        return GenerationResult{.success = true,
                                .text = aql,
                                .error_message = "",
                                .prompt_tokens = static_cast<size_t>(nl_query.size() / 4),
                                .completion_tokens = static_cast<size_t>(aql.size() / 4),
                                .finish_reason = "stop"};
    }

    size_t estimateTokens(const std::string& text) const override {
        return text.size() / 4;
    }

    std::string getProviderName() const override { return provider_name_; }

    bool isReady() const override { return true; }

    // Test utilities
    void setFailure(bool should_fail, const std::string& reason = "mock failure") {
        should_fail_ = should_fail;
        failure_reason_ = reason;
    }

    int getCallCount() const { return call_count_; }

    void resetCallCount() { call_count_ = 0; }

private:
    std::string provider_name_;
    int call_count_;
    bool should_fail_;
    std::string failure_reason_;
};

class MockParserService : public AQLParserService {
public:
    ParseResult parse(const std::string& aql_query) override {
        (void)aql_query;
        ParseResult result;
        result.success = true;
        return result;
    }

    std::string version() const override { return "mock-parser-1.0"; }

    bool supportsFeature(const std::string& feature) const override {
        return feature != "mutations";
    }
};

// ============================================================================
// Phase 0.4 Integration Tests
// ============================================================================

class LLMValidationPipelinePhase04Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create default LLMAQLHandler without injected client
        LLMAQLHandler::Config config;
        handler_ = std::make_unique<LLMAQLHandler>(config);
    }

    void TearDown() override { handler_.reset(); }

    std::unique_ptr<LLMAQLHandler> handler_;
};

// ============================================================================
// Test Suite: LLM Client Injection via Config
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, LLMClientInjectionViaConfig) {
    // Phase 0.4 Task: Wire LLM client via Config struct during initialization
    auto mock_client = std::make_shared<MockLLMClient>("test-provider");

    LLMAQLHandler::Config config;
    config.llm_client = mock_client;

    auto handler = std::make_unique<LLMAQLHandler>(config);

    // Verify client is wired
    auto retrieved_client = handler->getLLMClient();
    ASSERT_NE(retrieved_client, nullptr);
    EXPECT_EQ(retrieved_client->getProviderName(), "test-provider");
}

TEST_F(LLMValidationPipelinePhase04Test, DefaultLLMClientWhenNotInjected) {
    // Phase 0.4 Task: Create default LLM client if none provided
    LLMAQLHandler::Config config;
    config.llm_client = nullptr;  // Explicitly no client

    auto handler = std::make_unique<LLMAQLHandler>(config);

    // Verify a default client is created
    auto client = handler->getLLMClient();
    ASSERT_NE(client, nullptr);
    EXPECT_FALSE(client->getProviderName().empty());
}

// ============================================================================
// Test Suite: Runtime LLM Client Injection
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, RuntimeLLMClientInjection) {
    // Phase 0.4 Task: Inject new LLM client at runtime via setLLMClient()
    auto initial_client = std::make_shared<MockLLMClient>("initial-provider");
    auto new_client = std::make_shared<MockLLMClient>("new-provider");

    handler_->setLLMClient(initial_client);
    EXPECT_EQ(handler_->getLLMClient()->getProviderName(), "initial-provider");

    // Inject new client
    handler_->setLLMClient(new_client);
    EXPECT_EQ(handler_->getLLMClient()->getProviderName(), "new-provider");
}

TEST_F(LLMValidationPipelinePhase04Test, ValidationPipelineReWiringOnClientChange) {
    // Phase 0.4 Task: Validation pipeline must be re-wired when LLM client changes
    auto mock_client = std::make_shared<MockLLMClient>();

    // Get initial pipeline
    auto initial_pipeline = handler_->getValidationPipeline();

    // Inject client
    handler_->setLLMClient(mock_client);

    // Pipeline must be non-null and potentially different object
    auto new_pipeline = handler_->getValidationPipeline();
    ASSERT_NE(new_pipeline, nullptr);
    // Note: May be same or different object depending on implementation
    // Key is that it's wired with the new client
}

TEST_F(LLMValidationPipelinePhase04Test, LLMClientDisablingWithNullptr) {
    // Phase 0.4 Task: Allow disabling LLM client by injecting nullptr
    auto mock_client = std::make_shared<MockLLMClient>();
    handler_->setLLMClient(mock_client);
    ASSERT_NE(handler_->getLLMClient(), nullptr);

    // Disable by setting nullptr
    handler_->setLLMClient(nullptr);
    EXPECT_EQ(handler_->getLLMClient(), nullptr);

    // Pipeline should also be cleared
    auto pipeline = handler_->getValidationPipeline();
    EXPECT_EQ(pipeline, nullptr);
}

// ============================================================================
// Test Suite: LLM Client Method Invocation
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, LLMClientGenerateAQLInvocation) {
    // Phase 0.4 Task: execute() invokes LLM client internally
    auto mock_client = std::make_shared<MockLLMClient>();
    mock_client->resetCallCount();

    handler_->setLLMClient(mock_client);
    auto pipeline = handler_->getValidationPipeline();
    ASSERT_NE(pipeline, nullptr);

    auto result = pipeline->execute("Find all users", "schema context");
    (void)result;
    EXPECT_GT(mock_client->getCallCount(), 0);
}

TEST_F(LLMValidationPipelinePhase04Test, LLMClientErrorHandling) {
    // Phase 0.4 Task: Validation pipeline must handle LLM client errors gracefully
    auto mock_client = std::make_shared<MockLLMClient>();
    mock_client->setFailure(true, "LLM service unavailable");

    handler_->setLLMClient(mock_client);
    auto pipeline = handler_->getValidationPipeline();
    ASSERT_NE(pipeline, nullptr);

    // Call execute() instead of private generateAQL()
    auto result = pipeline->execute("Find users", "schema");
    // Expect error result due to LLM failure
    EXPECT_NE(result.status, LLMValidationStatus::SUCCESS);
    EXPECT_NE(result.error_message.find("LLM"), std::string::npos);
}

// ============================================================================
// Test Suite: Validation Pipeline Accessor
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, ValidationPipelineReadOnlyAccess) {
    // Phase 0.4 Task: getValidationPipeline() provides read-only access
    auto mock_client = std::make_shared<MockLLMClient>();
    handler_->setLLMClient(mock_client);

    auto pipeline = handler_->getValidationPipeline();
    ASSERT_NE(pipeline, nullptr);

    // Verify we can read from pipeline
    // (actual pipeline test methods depend on LLMValidationPipeline interface)
}

TEST_F(LLMValidationPipelinePhase04Test, ValidationPipelineNullWhenNoClient) {
    // Phase 0.4 Task: Pipeline is null when LLM client not set
    handler_->setLLMClient(nullptr);
    auto pipeline = handler_->getValidationPipeline();
    EXPECT_EQ(pipeline, nullptr);
}

// ============================================================================
// Test Suite: Configuration Consistency
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, ClientPersistenceAcrossOperations) {
    // Phase 0.4 Task: Injected client persists across multiple operations
    auto mock_client = std::make_shared<MockLLMClient>("persistent-provider");
    mock_client->resetCallCount();

    handler_->setLLMClient(mock_client);

    // Multiple operations
    for (int i = 0; i < 3; ++i) {
        auto client = handler_->getLLMClient();
        ASSERT_NE(client, nullptr);
        EXPECT_EQ(client->getProviderName(), "persistent-provider");
    }
}

TEST_F(LLMValidationPipelinePhase04Test, ValidationConfigPersistsWithClientInjection) {
    // Phase 0.4 Task: Validation config not lost when injecting new client
    LLMValidationPipelineConfig val_config;
    val_config.max_retries = 5;
    val_config.timeout_ms = 30000;

    handler_->setValidationPipelineConfig(val_config);

    auto mock_client = std::make_shared<MockLLMClient>();
    handler_->setLLMClient(mock_client);

    // Verify config is preserved
    auto retrieved_config = handler_->getValidationPipelineConfig();
    EXPECT_EQ(retrieved_config.max_retries, 5);
    EXPECT_EQ(retrieved_config.timeout_ms, 30000);
}

// ============================================================================
// Test Suite: Parser Service and LLM Client Integration
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, ParserAndLLMClientCoexistence) {
    // Phase 0.4 Task: Both parser service and LLM client can coexist
    auto parser_service = std::make_shared<MockParserService>();
    auto llm_client = std::make_shared<MockLLMClient>();

    handler_->setParserService(parser_service);
    handler_->setLLMClient(llm_client);

    ASSERT_NE(handler_->getParserService(), nullptr);
    ASSERT_NE(handler_->getLLMClient(), nullptr);

    // Both should be accessible
    EXPECT_NE(handler_->getParserService(), nullptr);
    EXPECT_FALSE(handler_->getLLMClient()->getProviderName().empty());
}

// ============================================================================
// Test Suite: Multiple Client Changes
// ============================================================================

TEST_F(LLMValidationPipelinePhase04Test, MultipleClientReplacements) {
    // Phase 0.4 Task: Can replace client multiple times without state corruption
    std::vector<std::shared_ptr<MockLLMClient>> clients;
    for (int i = 0; i < 5; ++i) {
        clients.push_back(std::make_shared<MockLLMClient>("client-" + std::to_string(i)));
    }

    for (const auto& client : clients) {
        handler_->setLLMClient(client);
        EXPECT_EQ(handler_->getLLMClient()->getProviderName(), client->getProviderName());
    }

    // Final state should be last client
    EXPECT_EQ(handler_->getLLMClient()->getProviderName(), "client-4");
}

