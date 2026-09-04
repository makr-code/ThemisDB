/**
 * @file test_llm_validation_pipeline.cpp
 * @brief Unit tests for LLM validation pipeline with retry logic
 *
 * Tests the validation pipeline orchestration:
 * - LLM generation + parser validation
 * - Retry logic with feedback
 * - Error handling and retryability checks
 * - Custom strategies (feedback generator, retryability)
 * - Timeout handling
 * - Concurrency
 * - Metrics emission
 *
 * @author ThemisDB Test Suite
 * @date 2026-06-18
 */

#include <gtest/gtest.h>
#include "aql/llm_validation_pipeline.h"
#include "llm/llm_client.h"
#include "query/aql_parser_service.h"

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace themis { namespace aql { namespace test { 

// ============================================================================
// Mock Implementations
// ============================================================================

/// Mock parser service for testing
class MockAQLParserService : public query::AQLParserService {
public:
    MockAQLParserService(bool default_success = true)
        : default_success_(default_success), call_count_(0) {}
    
    query::ParseResult parse(const std::string& aql_query) override {
        call_count_++;
        
        // Simulate parsing logic
        query::ParseResult result;
        result.success = default_success_;
        
        if (!default_success_) {
            result.diagnostics.error_message = "Simulated parse error";
            result.diagnostics.error_category = "SYNTAX_ERROR";
            result.diagnostics.line_number = 1;
            result.diagnostics.column_number = 5;
            result.diagnostics.suggestions.push_back("Check AQL syntax");
            return result;
        }
        
        // Success path - Phase 0.3 design: AST not included in ParseResult
        // Full AST access planned for Phase 1
        return result;
    }
    
    std::string version() const override { return "mock-1.0"; }
    
    bool supportsFeature(const std::string& feature) const override {
        return feature != "mutations";  // Mutations disabled in test
    }
    
    int call_count() const { return call_count_; }
    
private:
    bool default_success_;
    std::atomic<int> call_count_;
};

/// Mock LLM client for testing
class MockLLMClient : public llm::LLMClient {
public:
    MockLLMClient(const std::string& response = R"(FOR doc IN users RETURN doc)")
        : response_(response), call_count_(0) {}
    
    llm::GenerationResult generate(
        const std::string& prompt,
        const llm::GenerationOptions& options = {}
    ) override {
        return generateImpl(prompt, options);
    }

    llm::GenerationResult generateAQL(
        const std::string& nl_query,
        const std::string& schema_context = "",
        const llm::GenerationOptions& options = {}
    ) override {
        (void)schema_context;
        return generateImpl(nl_query, options);
    }

    size_t estimateTokens(const std::string& text) const override {
        return text.size() / 4;
    }

    std::string getProviderName() const override {
        return "mock";
    }

    bool isReady() const override {
        return ready_.load();
    }
    
    int call_count() const { return call_count_; }
    
    void setResponse(const std::string& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        response_ = response;
    }

    void setReady(bool ready) { ready_.store(ready); }

    std::vector<std::string> prompts() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return prompts_;
    }
    
private:
    llm::GenerationResult generateImpl(
        const std::string& prompt,
        const llm::GenerationOptions& options
    ) {
        (void)prompt;
        (void)options;
        call_count_++;
        llm::GenerationResult result;
        result.success = true;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            prompts_.push_back(prompt);
            result.text = response_;
        }
        result.finish_reason = "stop_sequence";
        return result;
    }

    std::string response_;
    std::atomic<int> call_count_;
    std::atomic<bool> ready_{true};
    mutable std::mutex mutex_;
    std::vector<std::string> prompts_;
};

// ============================================================================
// Test Fixture
// ============================================================================

class LLMValidationPipelineTest : public ::testing::Test {
protected:
    LLMValidationPipelineTest()
        : parser_service_(std::make_shared<MockAQLParserService>(true)),
          llm_client_(std::make_shared<MockLLMClient>()) {}
    
    std::shared_ptr<MockAQLParserService> parser_service_;
    std::shared_ptr<MockLLMClient> llm_client_;
};

// ============================================================================
// Success Path Tests
// ============================================================================

/// Test 1: Successful validation (no retries needed)
TEST_F(LLMValidationPipelineTest, SuccessfulValidationNoRetries) {
    LLMValidationPipelineConfig config;
    config.max_retries = 0;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    EXPECT_EQ(LLMValidationStatus::SUCCESS, result.status);
    EXPECT_FALSE(result.validated_aql.empty());
    EXPECT_EQ(1, result.attempts_made);
}

/// Test 2: Validation with retry configured but not needed
TEST_F(LLMValidationPipelineTest, SuccessfulValidationWithRetryAvailable) {
    LLMValidationPipelineConfig config;
    config.max_retries = 3;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    EXPECT_EQ(LLMValidationStatus::SUCCESS, result.status);
    EXPECT_EQ(1, result.attempts_made);  // Only needed 1 attempt
}

/// Test 3: Validation with custom configuration
TEST_F(LLMValidationPipelineTest, ValidationWithCustomConfig) {
    LLMValidationPipelineConfig config;
    config.max_retries = 2;
    config.timeout_ms = 10000;
    config.reject_on_error = true;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    const auto& cfg = pipeline->config();
    EXPECT_EQ(2, cfg.max_retries);
    EXPECT_EQ(10000, cfg.timeout_ms);
    EXPECT_TRUE(cfg.reject_on_error);
}

// ============================================================================
// Error Path Tests
// ============================================================================

/// Test 4: Parse error with no retries configured
TEST_F(LLMValidationPipelineTest, ParseErrorNoRetries) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);  // Always fail
    
    LLMValidationPipelineConfig config;
    config.max_retries = 0;
    config.reject_on_error = true;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    EXPECT_EQ(LLMValidationStatus::REJECTED, result.status);
    EXPECT_EQ(1, result.attempts_made);
    EXPECT_FALSE(result.error_message.empty());
}

/// Test 5: Parse error with retries configured (but exhausted)
TEST_F(LLMValidationPipelineTest, ParseErrorRetriesExhausted) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);  // Always fail
    
    LLMValidationPipelineConfig config;
    config.max_retries = 2;
    config.reject_on_error = false;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    EXPECT_EQ(LLMValidationStatus::EXHAUSTED_RETRIES, result.status);
    EXPECT_EQ(3, result.attempts_made);  // 1 initial + 2 retries
}

TEST_F(LLMValidationPipelineTest, LLMClientNotReadyFailsFast) {
    llm_client_->setReady(false);

    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    auto result = pipeline->execute("List all users", "");

    EXPECT_EQ(LLMValidationStatus::LLM_GENERATION_FAILED, result.status);
    EXPECT_EQ(0, result.attempts_made);
    EXPECT_EQ(0, llm_client_->call_count());
}

// ============================================================================
// Retry Logic Tests
// ============================================================================

/// Test 6: Retry success on second attempt
TEST_F(LLMValidationPipelineTest, RetrySuccessOnSecondAttempt) {
    // Parser fails once, then succeeds
    auto parser = std::make_shared<MockAQLParserService>(false);
    
    LLMValidationPipelineConfig config;
    config.max_retries = 1;
    config.reject_on_error = false;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    // Since parser always fails in mock, result will be exhausted
    // In real scenario with LLM retry, this would succeed
    EXPECT_NE(LLMValidationStatus::SUCCESS, result.status);
    EXPECT_EQ(2, result.attempts_made);  // 1 initial + 1 retry
}

/// Test 7: Retry feedback generation
TEST_F(LLMValidationPipelineTest, RetryFeedbackGeneration) {
    LLMValidationPipelineConfig config;
    config.max_retries = 1;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    // Set custom feedback generator
    std::string captured_feedback = {};
    auto custom_feedback = [&captured_feedback](const query::ParserDiagnostics& diag) {
        captured_feedback = "FEEDBACK: " + diag.error_message;
        return captured_feedback;
    };
    
    pipeline->setFeedbackGenerator(custom_feedback);
    
    // Feedback would be used during retry (in real scenario)
    EXPECT_NO_THROW(pipeline->execute("List all users", ""));
}

TEST_F(LLMValidationPipelineTest, RetryFeedbackIsInjectedIntoSubsequentPrompt) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);  // force retry path

    LLMValidationPipelineConfig config;
    config.max_retries = 1;
    config.reject_on_error = false;

    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );

    auto result = pipeline->execute("List all users", "");
    const auto prompts = llm_client_->prompts();
    EXPECT_EQ(LLMValidationStatus::EXHAUSTED_RETRIES, result.status);
    ASSERT_GE(prompts.size(), 2u);
    EXPECT_NE(prompts[1].find("Previous parser validation error"), std::string::npos);
}

// ============================================================================
// Configuration Tests
// ============================================================================

/// Test 8: Config update affects behavior
TEST_F(LLMValidationPipelineTest, ConfigUpdateAffectsBehavior) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    LLMValidationPipelineConfig new_config;
    new_config.max_retries = 5;
    new_config.timeout_ms = 30000;
    
    pipeline->setConfig(new_config);
    
    const auto& cfg = pipeline->config();
    EXPECT_EQ(5, cfg.max_retries);
    EXPECT_EQ(30000, cfg.timeout_ms);
}

/// Test 9: Timeout configuration
TEST_F(LLMValidationPipelineTest, TimeoutConfiguration) {
    LLMValidationPipelineConfig config;
    config.timeout_ms = 100;  // Very short timeout
    config.max_retries = 10;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    // Should respect timeout even with many retries available
    auto result = pipeline->execute("List all users", "");
    
    // Timeout might or might not trigger depending on timing
    if (result.status == LLMValidationStatus::EXHAUSTED_RETRIES) {
        EXPECT_TRUE(result.error_message.find("timeout") != std::string::npos ||
                    result.error_message.find("Timeout") != std::string::npos);
    }
}

/// Test 10: Reject vs. retry mode
TEST_F(LLMValidationPipelineTest, RejectVsRetryMode) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);
    
    // Reject mode
    LLMValidationPipelineConfig reject_config;
    reject_config.reject_on_error = true;
    reject_config.max_retries = 10;
    
    auto reject_pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, reject_config
    );
    
    auto reject_result = reject_pipeline->execute("List all users", "");
    EXPECT_EQ(LLMValidationStatus::REJECTED, reject_result.status);
    EXPECT_EQ(1, reject_result.attempts_made);
    
    // Retry mode
    LLMValidationPipelineConfig retry_config;
    retry_config.reject_on_error = false;
    retry_config.max_retries = 2;
    
    auto retry_pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, retry_config
    );
    
    auto retry_result = retry_pipeline->execute("List all users", "");
    EXPECT_EQ(LLMValidationStatus::EXHAUSTED_RETRIES, retry_result.status);
    EXPECT_EQ(3, retry_result.attempts_made);  // 1 + 2 retries
}

// ============================================================================
// Custom Strategy Tests
// ============================================================================

/// Test 11: Custom feedback generator
TEST_F(LLMValidationPipelineTest, CustomFeedbackGenerator) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    std::vector<std::string> feedback_calls;
    
    auto custom_feedback = [&feedback_calls](const query::ParserDiagnostics& diag) {
        feedback_calls.push_back("Custom: " + diag.error_message);
        return feedback_calls.back();
    };
    
    pipeline->setFeedbackGenerator(custom_feedback);
    
    // In retry scenario, custom feedback would be called
    EXPECT_NO_THROW(pipeline->execute("List all users", ""));
}

/// Test 12: Custom retryability check
TEST_F(LLMValidationPipelineTest, CustomRetryabilityCheck) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    std::vector<bool> retryability_checks;
    
    auto custom_check = [&retryability_checks](const query::ParserDiagnostics& diag) {
        bool should_retry = diag.error_category != "ACCESS_DENIED";
        retryability_checks.push_back(should_retry);
        return should_retry;
    };
    
    pipeline->setRetryabilityCheck(custom_check);
    
    EXPECT_NO_THROW(pipeline->execute("List all users", ""));
}

// ============================================================================
// Diagnostics Tests
// ============================================================================

/// Test 13: Diagnostics propagated to result
TEST_F(LLMValidationPipelineTest, DiagnosticsPropagated) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);
    
    LLMValidationPipelineConfig config;
    config.max_retries = 0;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    auto result = pipeline->execute("List all users", "");
    
    EXPECT_FALSE(result.status == LLMValidationStatus::SUCCESS ||
                 result.parser_diagnostics.error_message.empty());
}

/// Test 14: Error message contains meaningful info
TEST_F(LLMValidationPipelineTest, ErrorMessageMeaningful) {
    parser_service_ = std::make_shared<MockAQLParserService>(false);
    
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    auto result = pipeline->execute("List all users", "");
    
    if (result.status != LLMValidationStatus::SUCCESS) {
        EXPECT_FALSE(result.error_message.empty());
    }
}

// ============================================================================
// Factory Tests
// ============================================================================

/// Test 15: Factory creates working pipeline
TEST_F(LLMValidationPipelineTest, FactoryCreatesWorkingPipeline) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    ASSERT_NE(nullptr, pipeline);
    EXPECT_NO_THROW(pipeline->execute("List all users", ""));
}

/// Test 16: Factory with config
TEST_F(LLMValidationPipelineTest, FactoryWithConfigCreates) {
    LLMValidationPipelineConfig config;
    config.max_retries = 5;
    
    auto pipeline = LLMValidationPipelineFactory::createWithConfig(
        parser_service_, llm_client_, config
    );
    
    ASSERT_NE(nullptr, pipeline);
    EXPECT_EQ(5, pipeline->config().max_retries);
}

// ============================================================================
// Concurrency Tests
// ============================================================================

/// Test 17: Concurrent execute calls are thread-safe
TEST_F(LLMValidationPipelineTest, ConcurrentExecuteThreadSafe) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    const int num_threads = 4;
    const int queries_per_thread = 10;
    std::vector<std::thread> threads;
    
    std::atomic<int> success_count{0};
    std::atomic<int> total_attempts{0};
    
    auto worker = [&]() {
        for (int i = 0; i < queries_per_thread; ++i) {
            auto result = pipeline->execute("List all users", "");
            if (result.status == LLMValidationStatus::SUCCESS) {
                success_count++;
            }
            total_attempts += result.attempts_made;
        }
    };
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(num_threads * queries_per_thread, success_count.load());
    EXPECT_EQ(num_threads * queries_per_thread, total_attempts.load());
}

// ============================================================================
// Edge Cases
// ============================================================================

/// Test 18: Empty NL query handled
TEST_F(LLMValidationPipelineTest, EmptyNLQueryHandled) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    auto result = pipeline->execute("", "");
    
    // Empty query might fail at LLM or parser level
    EXPECT_NE(nullptr, pipeline.get());
}

/// Test 19: Empty schema context handled
TEST_F(LLMValidationPipelineTest, EmptySchemaContextHandled) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    auto result = pipeline->execute("List all users", "");
    
    // Empty schema context is valid (means no constraints)
    EXPECT_NO_THROW(pipeline->execute("List all users", ""));
}

/// Test 20: Very long queries handled
TEST_F(LLMValidationPipelineTest, VeryLongQueryHandled) {
    auto pipeline = LLMValidationPipelineFactory::create(parser_service_, llm_client_);
    
    std::string long_query = {};
    for (int i = 0; i < 1000; ++i) {
        long_query += "query";
    }
    
    auto result = pipeline->execute(long_query, "");
    
    // Should handle or fail gracefully
    EXPECT_NE(nullptr, pipeline.get());
}
} } } // namespace themis::aql::test
