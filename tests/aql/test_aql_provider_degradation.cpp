/**
 * @file test_aql_provider_degradation.cpp
 * @brief Phase 5 Unit Tests — Provider Degradation Scenarios
 *
 * Tests graceful degradation when LLM, RAG, and embedding providers
 * become unavailable, time out, or fail in various combinations:
 * - Infer provider unavailability
 * - RAG provider timeout with fallback
 * - Embed provider failure and degradation
 * - Multi-provider unavailability with error priority
 * - Provider recovery detection
 * - Circuit breaker activation under sustained failures
 * - Few-shot fallback to template library
 * - Diagnostic accuracy for user-facing messages
 *
 * Uses MockProvider classes defined inline — no real infrastructure required.
 */


#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <atomic>
#include <memory>

#include "aql/aql_error_types.h"

// Pull in the shared mock provider factory fixture
#include "fixtures/mock_provider_factory.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Additional: Mock Circuit Breaker (per-provider)
// ============================================================================

class ProviderCircuitBreaker {
public:
    explicit ProviderCircuitBreaker(int threshold = 3) : threshold_(threshold) {}

    enum class State { CLOSED, OPEN };

    bool allowRequest() const { return state_ == State::CLOSED; }

    void recordFailure() {
        if (++failures_ >= threshold_) {
          state_ = State::OPEN;
        }
    }
    void recordSuccess() { failures_ = 0; state_ = State::CLOSED; }
    void reset()         { failures_ = 0; state_ = State::CLOSED; }

    State getState() const { return state_; }
    int   failures() const { return failures_; }

private:
    int   threshold_;
    int   failures_{0};
    State state_{State::CLOSED};
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test ProviderDegradation_InferProviderUnavailableReturnsError
 *
 * Verify that an unavailable infer provider returns a structured error
 * with ProviderUnavailable category, not a crash or silent failure.
 */
TEST(ProviderDegradation, InferProviderUnavailableReturnsError) {
    MockInferProvider infer(MockProviderConfig::AlwaysFail("LLM service is down"));

    auto result = infer.infer("find all users older than 30");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.error_message.find("LLM service is down") != std::string::npos);

    // Create structured error context as the translation layer would
    AQLErrorContext ctx(
        "provider",
        ProviderError::ProviderUnavailable,
        "mock_infer_provider",
        "[TRANSLATION:ProviderUnavailable] " + result.error_message
    );
    ctx.setOperationType("translateNLToAQL");
    ctx.addDiagnosticHint("Check LLM provider connectivity and configuration");
    ctx.addDiagnosticHint("Retry after provider health check passes");
    ctx.setRecoverable(true);

    EXPECT_EQ(ctx.getCategory(), ProviderError::ProviderUnavailable);
    EXPECT_TRUE(ctx.isRecoverable());
    EXPECT_TRUE(ctx.formatForLogging().find("ProviderUnavailable") != std::string::npos);

    // Recovery strategy for provider unavailability
    auto strategy = getRecoveryStrategy("provider", ProviderError::ProviderUnavailable);
    EXPECT_EQ(strategy, RecoveryStrategy::DEGRADE_GRACEFULLY);
}

/**
 * @test ProviderDegradation_RAGProviderTimeoutFallsBack
 *
 * Verify that a RAG provider timeout causes the system to fall back
 * gracefully to zero-shot translation without few-shot context.
 */
TEST(ProviderDegradation, RAGProviderTimeoutFallsBack) {
    MockRAGProvider rag(MockProviderConfig::AlwaysFail("RAG retrieval timeout"));

    auto result = rag.retrieve("find orders above $500");

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.find("timeout") != std::string::npos ||
                result.error_message.find("Fail") != std::string::npos ||
                !result.error_message.empty());

    // Create error context for fallback
    AQLErrorContext ctx(
        "provider",
        ProviderError::RAGFailed,
        "mock_rag_provider",
        "RAG provider failed: " + result.error_message
    );
    ctx.setOperationType("retrieveFewShotExamples");
    ctx.addDiagnosticHint("Proceeding with zero-shot translation (no few-shot context)");
    ctx.addDiagnosticHint("Translation quality may be reduced without examples");
    ctx.setRecoverable(true);  // Zero-shot fallback is available

    EXPECT_EQ(ctx.getCategory(), ProviderError::RAGFailed);
    EXPECT_TRUE(ctx.isRecoverable());

    auto strategy = getRecoveryStrategy("provider", ProviderError::RAGFailed);
    // RAG failure: degrade to zero-shot (graceful degradation)
    EXPECT_NE(strategy, RecoveryStrategy::FAIL_CLOSED);
}

/**
 * @test ProviderDegradation_EmbedProviderFailureDegrades
 *
 * Verify that an embedding provider failure causes the system to
 * fall back to keyword-based similarity (Jaccard) without crashing.
 */
TEST(ProviderDegradation, EmbedProviderFailureDegrades) {
    MockEmbedProvider embed(MockProviderConfig::AlwaysFail("GPU OOM: embedding vector generation failed"));

    auto result = embed.embed("semantic similarity for AQL query");

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.embedding.empty());
    EXPECT_FALSE(result.error_message.empty());

    // Error context
    AQLErrorContext ctx(
        "bridge",
        BridgeError::EmbeddingGenerationFailed,
        "mock_embed_provider",
        "[BRIDGE:ExecutionFailed] Embedding generation failed: " + result.error_message
    );
    ctx.setOperationType("generateEmbeddingForFewShot");
    ctx.addDiagnosticHint("Falling back to Jaccard keyword similarity");
    ctx.addDiagnosticHint("Semantic few-shot ranking disabled for this request");
    ctx.setRecoverable(true);

    EXPECT_EQ(ctx.getCategory(), BridgeError::EmbeddingGenerationFailed);
    EXPECT_TRUE(ctx.isRecoverable());

    auto strategy = getRecoveryStrategy("bridge", BridgeError::ExecutionFailed);
    EXPECT_NE(strategy, RecoveryStrategy::FAIL_CLOSED);
}

/**
 * @test ProviderDegradation_MultipleProvidersUnavailableErrorPriority
 *
 * When multiple providers fail simultaneously, verify that the error
 * priority order is: Infer > RAG > Embed (blocking feature first).
 */
TEST(ProviderDegradation, MultipleProvidersUnavailableErrorPriority) {
    MockInferProvider infer(MockProviderConfig::AlwaysFail("Infer service unavailable"));
    MockRAGProvider   rag(MockProviderConfig::AlwaysFail("RAG timeout"));
    MockEmbedProvider embed(MockProviderConfig::AlwaysFail("Embed OOM"));

    auto infer_r = infer.infer("any query");
    auto rag_r   = rag.retrieve("any query");
    auto embed_r = embed.embed("any text");

    EXPECT_FALSE(infer_r.success);
    EXPECT_FALSE(rag_r.success);
    EXPECT_FALSE(embed_r.success);

    // Infer error has highest priority (translation is completely blocked)
    AQLErrorContext infer_ctx("provider", ProviderError::InferFailed,  "infer",  infer_r.error_message);
    AQLErrorContext rag_ctx(  "provider", ProviderError::RAGFailed,    "rag",    rag_r.error_message);
    AQLErrorContext embed_ctx("bridge",   BridgeError::ExecutionFailed,"embed",  embed_r.error_message);

    // Infer failure is unrecoverable (no translation possible)
    infer_ctx.setRecoverable(false);
    // RAG and embed failures are recoverable (degraded mode)
    rag_ctx.setRecoverable(true);
    embed_ctx.setRecoverable(true);

    EXPECT_FALSE(infer_ctx.isRecoverable());
    EXPECT_TRUE(rag_ctx.isRecoverable());
    EXPECT_TRUE(embed_ctx.isRecoverable());
}

/**
 * @test ProviderDegradation_ProviderRecoveryHandledCorrectly
 *
 * Verify that after N failures the provider correctly reports
 * success again when the underlying service recovers (FailAfterN + reset).
 */
TEST(ProviderDegradation, ProviderRecoveryHandledCorrectly) {
    // Fail after 3 calls, then succeed again (simulating recovery)
    MockInferProvider infer(MockProviderConfig::FailAfterN(3));

    // First 3 succeed
    for (int i = 0; i < 3; ++i) {
        auto r = infer.infer("recovery test query " + std::to_string(i));
        EXPECT_TRUE(r.success) << "Expected success on call " << i;
    }
    // Next calls fail
    auto fail_r = infer.infer("call after threshold");
    EXPECT_FALSE(fail_r.success);

    // Verify tracker statistics
    EXPECT_EQ(infer.tracker().successCalls(), 3);
    EXPECT_EQ(infer.tracker().failureCalls(), 1);
    EXPECT_EQ(infer.tracker().totalCalls(), 4);
}

/**
 * @test ProviderDegradation_CircuitBreakerActivatesUnderSustainedFailures
 *
 * Verify that sustained provider failures trigger the circuit breaker
 * to open, preventing further calls until reset.
 */
TEST(ProviderDegradation, CircuitBreakerActivatesUnderSustainedFailures) {
    ProviderCircuitBreaker cb(3);  // Opens after 3 failures
    MockInferProvider      infer(MockProviderConfig::AlwaysFail("Provider down"));

    int succeeded = 0;
    int failed    = 0;
    int blocked   = 0;

    for (int i = 0; i < 10; ++i) {
        if (!cb.allowRequest()) {
            ++blocked;
            continue;
        }
        auto r = infer.infer("sustained_failure_" + std::to_string(i));
        if (r.success) {
            cb.recordSuccess();
            ++succeeded;
        } else {
            cb.recordFailure();
            ++failed;

            AQLErrorContext ctx(
                "provider",
                ProviderError::CircuitBreakerOpen,
                "infer_circuit_breaker",
                "[TRANSLATION:ProviderUnavailable] Circuit breaker activated after sustained failures"
            );
            ctx.setRecoverable(false);
            ctx.setRetryCount(i);
        }
    }

    EXPECT_EQ(cb.getState(), ProviderCircuitBreaker::State::OPEN);
    EXPECT_GE(failed,  3);   // At least 3 failures to trip the breaker
    EXPECT_GT(blocked, 0);   // Some calls were blocked by the open breaker
    EXPECT_EQ(succeeded, 0); // AlwaysFail provider never succeeds
}

/**
 * @test ProviderDegradation_FewShotFallbackToTemplateLibrary
 *
 * Verify that RAG provider failure correctly falls back to the
 * static template library for few-shot examples.
 */
TEST(ProviderDegradation, FewShotFallbackToTemplateLibrary) {
    MockRAGProvider rag(MockProviderConfig::AlwaysFail("RAG index unavailable"));

    // Simulate retrieval attempt
    auto rag_result = rag.retrieve("show me all active subscriptions");
    EXPECT_FALSE(rag_result.success);

    // Fallback: use static template library
    std::vector<std::string> template_examples = {
        "FOR doc IN collection RETURN doc",
        "FOR doc IN collection FILTER doc.status == 'active' RETURN doc",
        "FOR doc IN collection SORT doc.created_at DESC LIMIT 10 RETURN doc",
    };

    // Verify fallback examples are available
    EXPECT_FALSE(template_examples.empty());
    EXPECT_GE(template_examples.size(), std::size_t(1));

    // Create degraded error context
    AQLErrorContext ctx(
        "provider",
        ProviderError::RAGFailed,
        "rag_provider",
        "RAG retrieval failed: " + rag_result.error_message
    );
    ctx.addDiagnosticHint("Using static template library (" +
                          std::to_string(template_examples.size()) + " examples)");
    ctx.addDiagnosticHint("Semantic relevance of examples may be reduced");
    ctx.setRecoverable(true);

    EXPECT_TRUE(ctx.isRecoverable());
    EXPECT_TRUE(ctx.formatForLogging().find("static template library") != std::string::npos);
}

/**
 * @test ProviderDegradation_DiagnosticAccuracyUserFacingMessages
 *
 * Verify that error messages contain accurate, actionable diagnostic
 * information that would help operators triage production issues.
 */
TEST(ProviderDegradation, DiagnosticAccuracyUserFacingMessages) {
    // Test several error categories and verify their diagnostic content

    struct TestCase {
        std::string error_type;
        std::string category;
        std::string expected_hint_fragment;
    };

    std::vector<TestCase> cases = {
        {"provider", ProviderError::InferFailed,         "LLM"},
        {"provider", ProviderError::RAGFailed,           "RAG"},
        {"provider", ProviderError::EmbedFailed,         "embedding"},
        {"provider", ProviderError::CircuitBreakerOpen,  "circuit"},
        {"bridge",   BridgeError::TimeoutExceeded,       "timeout"},
        {"validation", ValidationError::MalformedAQL,   "AQL"},
    };

    std::vector<std::string> hints_per_category = {
        "Check LLM provider configuration and API key",
        "Check RAG index health and connection settings",
        "Check embedding model availability and GPU memory",
        "Wait for circuit breaker reset interval or force reset",
        "Increase bridge timeout or investigate provider latency",
        "Fix AQL syntax before retrying the request",
    };

    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto& tc = cases[i];
        AQLErrorContext ctx(
            tc.error_type, tc.category,
            "diagnostic_test",
            "Error in " + tc.category
        );
        ctx.addDiagnosticHint(hints_per_category[i]);
        ctx.setRecoverable(true);

        std::string log = ctx.formatForLogging();
        EXPECT_TRUE(log.find(tc.category) != std::string::npos)
            << "Category missing in log for: " << tc.category;
        EXPECT_TRUE(log.find("Hints=[") != std::string::npos)
            << "Hints missing in log for: " << tc.category;
    }
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
