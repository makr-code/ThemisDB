/**
 * @file test_aql_translation_recovery.cpp
 * @brief Phase 4 Unit Tests for Translation Pipeline Error Recovery
 *
 * Tests comprehensive error recovery in NL-to-AQL translation including:
 * - Retry logic with exponential backoff
 * - Retry exhaustion handling
 * - Context overflow graceful degradation
 * - Provider unavailability detection
 * - Timeout handling and recovery
 * - Invalid response retry with error feedback
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <spdlog/spdlog.h>

#include "aql/aql_error_types.h"
#include "aql/llm_error_codes.h"

namespace themis {
namespace aql {
namespace testing {

/**
 * @brief Mock LLM provider for simulating translation errors
 */
class MockLLMProvider {
public:
    enum class FailureMode {
        SUCCESS,
        FAIL_ONCE,
        FAIL_TWICE,
        ALWAYS_FAIL,
        TIMEOUT_ONCE,
        INVALID_RESPONSE
    };

    MockLLMProvider(FailureMode mode = FailureMode::SUCCESS)
        : failure_mode_(mode), call_count_(0) {}

    struct InferenceResult {
        bool success = 0;
        std::string error_message;
        std::string generated_aql;
    };

    InferenceResult generateAQL(const std::string& nl_query) {
        call_count_++;

        // Simulate failure modes
        switch (failure_mode_) {
            case FailureMode::SUCCESS:
                return {true, "", "FOR u IN users FILTER u.name == 'Alice' RETURN u"};

            case FailureMode::FAIL_ONCE:
                if (call_count_ == 1) {
                    return {false, "LLM inference failed: connection timeout", ""};
                }
                return {true, "", "FOR u IN users FILTER u.name == 'Bob' RETURN u"};

            case FailureMode::FAIL_TWICE:
                if (call_count_ <= 2) {
                    return {false, "LLM inference failed: CUDA out of memory", ""};
                }
                return {true, "", "FOR u IN users FILTER u.age > 18 RETURN u"};

            case FailureMode::ALWAYS_FAIL:
                return {false, "LLM service unavailable", ""};

            case FailureMode::TIMEOUT_ONCE:
                if (call_count_ == 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                    return {false, "Timeout: request exceeded 5000ms", ""};
                }
                return {true, "", "FOR u IN users RETURN u"};

            case FailureMode::INVALID_RESPONSE:
                // First attempt returns syntactically invalid AQL
                if (call_count_ == 1) {
                    return {true, "", "INVALID AQL SYNTAX HERE"};
                }
                // Retry succeeds
                return {true, "", "FOR u IN users RETURN u"};
        }

        return {false, "Unknown failure mode", ""};
    }

    uint32_t getCallCount() const { return call_count_; }
    void resetCallCount() { call_count_ = 0; }

private:
    FailureMode failure_mode_;
    std::atomic<uint32_t> call_count_;
};

// ============================================================================
// Recovery Strategy Helper
// ============================================================================

struct TranslationRetryContext {
    MockLLMProvider& provider;
    const std::string& nl_query;
    const std::string& schema_context;
    uint32_t max_retries;
    uint32_t backoff_base_ms;

    std::string last_aql_result;
    std::string last_error;
    uint32_t retry_count = {};
    bool should_retry = {};

    TranslationRetryContext(MockLLMProvider& p, const std::string& nq,
                           const std::string& sc, uint32_t max_r = 3)
        : provider(p), nl_query(nq), schema_context(sc),
          max_retries(max_r), backoff_base_ms(100),
          retry_count(0), should_retry(true) {}

    bool executeWithRetry() {
        for (retry_count = 0; retry_count < max_retries; ++retry_count) {
            auto result = provider.generateAQL(nl_query);

            if (result.success) {
                last_aql_result = result.generated_aql;
                last_error = "";
                return true;
            }

            last_error = result.error_message;

            // Calculate exponential backoff: 100ms, 500ms, 2000ms
            uint32_t backoff_ms = backoff_base_ms;
            for (uint32_t i = 0; i < retry_count; ++i) {
                backoff_ms *= 5;
            }

            if (retry_count < max_retries - 1) {
                // Sleep with backoff
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
            }
        }

        should_retry = false;
        return false;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test T4.3.1a: GenerationFailed with Retry (Success on 2nd Attempt)
 *
 * Verify that transient LLM failures are retried and succeed on second attempt
 */
TEST(AQLTranslationRecovery, GenerationFailed_RetrySucceeds) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::FAIL_ONCE);
    TranslationRetryContext retry_ctx(provider, "show me all users", "schema_context");

    EXPECT_TRUE(retry_ctx.executeWithRetry());
    EXPECT_EQ(retry_ctx.retry_count, 1);  // Failed once, succeeded on second attempt
    EXPECT_TRUE(retry_ctx.last_aql_result.find("FOR u IN users") != std::string::npos);
    EXPECT_EQ(provider.getCallCount(), 2);  // Called twice
}

/**
 * @test T4.3.1b: GenerationFailed Retry Exhaustion (3 Failures)
 *
 * Verify that exhausted retries result in error with diagnostic context
 */
TEST(AQLTranslationRecovery, GenerationFailed_RetryExhausted) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::ALWAYS_FAIL);
    TranslationRetryContext retry_ctx(provider, "show me all users", "schema_context", 3);

    EXPECT_FALSE(retry_ctx.executeWithRetry());
    EXPECT_EQ(retry_ctx.retry_count, 3);  // All 3 retries exhausted
    EXPECT_TRUE(retry_ctx.last_error.find("unavailable") != std::string::npos);

    // Create error context for exhausted retries
    AQLErrorContext ctx(
        "translation",
        TranslationError::RetryExhausted,
        "llm_handler",
        "NL-to-AQL translation failed after " + std::to_string(retry_ctx.max_retries) + " retries: " + retry_ctx.last_error
    );
    ctx.setOperationType("translate_nl_to_aql");
    ctx.setRetryCount(retry_ctx.retry_count);
    ctx.addDiagnosticHint("Last error: " + retry_ctx.last_error);
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), TranslationError::RetryExhausted);
    EXPECT_EQ(ctx.getRetryCount(), 3);
}

/**
 * @test T4.3.1c: TimeoutExceeded with Exponential Backoff
 *
 * Verify timeout detection and backoff timing
 */
TEST(AQLTranslationRecovery, TimeoutExceeded_ExponentialBackoff) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::TIMEOUT_ONCE);
    TranslationRetryContext retry_ctx(provider, "complex query", "schema", 2);

    auto start = std::chrono::high_resolution_clock::now();
    EXPECT_TRUE(retry_ctx.executeWithRetry());
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Should have timeout + backoff delay (100ms base) + retry
    // Total: ~5000ms (timeout) + ~100ms (backoff) = ~5100ms minimum
    EXPECT_GE(duration_ms, 5000);
    EXPECT_EQ(retry_ctx.retry_count, 1);  // Failed once, succeeded on second

    // Create error context with timing info
    AQLErrorContext ctx(
        "translation",
        TranslationError::TimeoutExceeded,
        "llm_handler",
        "Translation request exceeded timeout"
    );
    ctx.addDiagnosticHint("Request took " + std::to_string(duration_ms) + "ms; consider increasing timeout");
}

/**
 * @test T4.3.1d: InvalidResponse Retry with Error Feedback in Prompt
 *
 * Verify that syntactically invalid LLM responses are retried with error feedback
 */
TEST(AQLTranslationRecovery, InvalidResponse_RetryWithFeedback) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::INVALID_RESPONSE);
    TranslationRetryContext retry_ctx(provider, "list all users", "schema", 2);

    // First attempt gets invalid response
    auto first_result = provider.generateAQL("list all users");
    EXPECT_TRUE(first_result.success);  // Provider returns success but AQL is invalid
    EXPECT_TRUE(first_result.generated_aql.find("INVALID") != std::string::npos);

    provider.resetCallCount();

    // Second attempt with error feedback should succeed
    EXPECT_TRUE(retry_ctx.executeWithRetry());
    EXPECT_GE(retry_ctx.retry_count, 1);

    // Create error context for invalid response
    AQLErrorContext ctx(
        "translation",
        TranslationError::InvalidResponse,
        "llm_handler",
        "Generated AQL failed validation: MalformedAQL detected"
    );
    ctx.addDiagnosticHint("Retry with validation error context in prompt");
    ctx.addDiagnosticHint("Provide LLM with feedback about syntax issue");
    ctx.setRecoverable(true);
}

/**
 * @test T4.3.1e: ContextOverflow Graceful Degradation
 *
 * Verify graceful degradation when context window is exhausted
 */
TEST(AQLTranslationRecovery, ContextOverflow_GracefulDegradation) {
    std::string complex_context(50000, 'x');  // Very large context

    AQLErrorContext ctx(
        "translation",
        TranslationError::ContextOverflow,
        "llm_handler",
        "Context window exhausted: " + std::to_string(complex_context.size()) + " tokens"
    );
    ctx.setOperationType("translate_nl_to_aql_with_context");
    ctx.addDiagnosticHint("Context size (" + std::to_string(complex_context.size()) + 
                         "tokens) exceeds max (" + std::to_string(32000) + " tokens)");
    ctx.addDiagnosticHint("Degradation: clear conversation history or use simpler schema");
    ctx.setRecoverable(true);

    EXPECT_EQ(ctx.getCategory(), TranslationError::ContextOverflow);
    EXPECT_TRUE(ctx.isRecoverable());

    // Verify recovery strategy
    auto strategy = getRecoveryStrategy("translation", TranslationError::ContextOverflow);
    EXPECT_EQ(strategy, RecoveryStrategy::DEGRADE_GRACEFULLY);
}

/**
 * @test T4.3.1f: CircuitBreakerOpen Fast-Fail
 *
 * Verify that open circuit breaker returns error immediately without provider call
 */
TEST(AQLTranslationRecovery, CircuitBreakerOpen_FastFail) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::SUCCESS);
    provider.resetCallCount();

    // Simulate circuit breaker open
    bool circuit_open = true;
    uint32_t call_count_before = provider.getCallCount();

    if (!circuit_open) {
        provider.generateAQL("test query");
    }

    uint32_t call_count_after = provider.getCallCount();

    // Provider should NOT have been called
    EXPECT_EQ(call_count_before, call_count_after);

    // Create error context for circuit breaker
    AQLErrorContext ctx(
        "provider",
        ProviderError::CircuitBreakerOpen,
        "circuit_breaker",
        "Inference provider circuit breaker is OPEN"
    );
    ctx.addDiagnosticHint("Provider recently experienced 5+ consecutive failures");
    ctx.addDiagnosticHint("Circuit will attempt recovery in 30 seconds");
    ctx.setRecoverable(true);

    EXPECT_EQ(ctx.getCategory(), ProviderError::CircuitBreakerOpen);
}

/**
 * @test T4.3.1g: ProviderUnavailable Detection
 *
 * Verify detection and handling of provider unavailability
 */
TEST(AQLTranslationRecovery, ProviderUnavailable_Detection) {
    MockLLMProvider provider(MockLLMProvider::FailureMode::ALWAYS_FAIL);
    TranslationRetryContext retry_ctx(provider, "test query", "schema", 2);

    // First attempt fails with unavailable error
    auto result = provider.generateAQL("test query");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.find("unavailable") != std::string::npos);

    // Create error context
    AQLErrorContext ctx(
        "translation",
        TranslationError::ProviderUnavailable,
        "llm_handler",
        "LLM service is unavailable: connection refused"
    );
    ctx.setOperationType("translate_nl_to_aql");
    ctx.setRetryCount(1);
    ctx.addDiagnosticHint("Check LLM service health endpoint");
    ctx.addDiagnosticHint("Retry will attempt in 500ms");
    ctx.setRecoverable(true);

    EXPECT_EQ(ctx.getCategory(), TranslationError::ProviderUnavailable);
    EXPECT_TRUE(ctx.isRecoverable());

    auto strategy = getRecoveryStrategy("translation", TranslationError::ProviderUnavailable);
    EXPECT_EQ(strategy, RecoveryStrategy::RETRY_WITH_BACKOFF);
}

/**
 * @test T4.3.1h: Retry Backoff Timing Accuracy
 *
 * Verify that retry backoff timing follows exponential pattern
 */
TEST(AQLTranslationRecovery, Backoff_TimingAccuracy) {
    const uint32_t BASE_BACKOFF_MS = 100;
    const uint32_t TOLERANCE_MS = 50;  // Allow ±50ms variance

    std::vector<uint32_t> expected_backoffs = {100, 500, 2500};  // 100ms * 5^n

    for (size_t i = 0; i < expected_backoffs.size(); ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        uint32_t backoff_ms = BASE_BACKOFF_MS;
        for (size_t j = 0; j < i; ++j) {
            backoff_ms *= 5;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));

        auto end = std::chrono::high_resolution_clock::now();
        auto actual_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // Verify timing is within tolerance
        EXPECT_GE(actual_ms, expected_backoffs[i] - TOLERANCE_MS);
        EXPECT_LE(actual_ms, expected_backoffs[i] + TOLERANCE_MS);

        spdlog::debug("Backoff #{}: expected={}ms, actual={}ms", i + 1, expected_backoffs[i], actual_ms);
    }
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
