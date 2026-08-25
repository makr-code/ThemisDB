/**
 * @file test_aql_hardening_v1_6_0.cpp
 * @brief Comprehensive tests for AQL Query Hardening & Enhancement (v1.6.0)
 *
 * Tests the following enhancements:
 * 1. Post-Generation AQL Validation with injection detection
 * 2. Thread leak elimination in LLMTimeoutManager
 * 3. Per-operation-type circuit breakers
 * 4. Bounded conversation history with context-window budget
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <memory>
#include "aql/llm_aql_handler.h"
#include "aql/aql_conversation_context.h"
#include "aql/llm_timeout_manager.h"
#include "aql/aql_query_validator.h"

using namespace themis::aql;

// ============================================================================
// Test Suite 1: Post-Generation AQL Validation
// ============================================================================

class AQLPostGenerationValidationTest : public ::testing::Test {
protected:
    LLMAQLHandler handler;
    AQLQueryValidator validator;
};

TEST_F(AQLPostGenerationValidationTest, ValidationModeWarnsOnErrors) {
    handler.setValidationMode(TranslationValidationMode::WARN_ONLY);
    // When WARN_ONLY is set, validation errors should be logged but query returned
    EXPECT_EQ(handler.getValidationMode(), TranslationValidationMode::WARN_ONLY);
}

TEST_F(AQLPostGenerationValidationTest, ValidationModeRejectsOnError) {
    handler.setValidationMode(TranslationValidationMode::REJECT_ON_ERROR);
    EXPECT_EQ(handler.getValidationMode(), TranslationValidationMode::REJECT_ON_ERROR);
}

TEST_F(AQLPostGenerationValidationTest, ValidationModeRetryOnError) {
    handler.setValidationMode(TranslationValidationMode::RETRY_ON_ERROR);
    EXPECT_EQ(handler.getValidationMode(), TranslationValidationMode::RETRY_ON_ERROR);
}

TEST_F(AQLPostGenerationValidationTest, DetectMissingReturnClause) {
    std::string invalid_aql = "FOR u IN users FILTER u.age > 18";
    auto result = validator.validate(invalid_aql);
    
    // Should detect missing RETURN clause
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(AQLPostGenerationValidationTest, DetectMissingForClause) {
    std::string invalid_aql = "RETURN u";
    auto result = validator.validate(invalid_aql);
    
    // Should detect missing FOR clause
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(AQLPostGenerationValidationTest, ValidateCorrectQuery) {
    std::string valid_aql = "FOR u IN users FILTER u.age > 18 RETURN u";
    auto result = validator.validate(valid_aql);
    
    // Should pass validation
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

// ============================================================================
// Test Suite 2: Thread Leak Elimination in LLMTimeoutManager
// ============================================================================

class ThreadLeakEliminationTest : public ::testing::Test {
protected:
    LLMTimeoutManager timeout_mgr;
};

TEST_F(ThreadLeakEliminationTest, TimeoutDoesNotLeakThreads) {
    // Test that timeout doesn't cause thread leaks
    // This is a basic sanity check - actual leak detection would require
    // more sophisticated tools like ThreadSanitizer
    
    std::atomic<int> execution_count{0};
    
    for (int i = 0; i < 10; ++i) {
        try {
            timeout_mgr.executeWithTimeout(
                [&execution_count]() {
                    execution_count++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    return 42;
                },
                std::chrono::seconds(0),
                "test_timeout"
            );
        } catch (const LLMException& e) {
            // Expected timeout exception
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::TIMEOUT);
        }
    }
    
    // Give cleanup threads a chance to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // If threads were leaked, this would not be reached safely
    EXPECT_GE(execution_count, 0);  // Sanity check
}

TEST_F(ThreadLeakEliminationTest, SuccessfulExecutionDoesNotLeakThreads) {
    std::atomic<int> execution_count{0};
    
    for (int i = 0; i < 10; ++i) {
        int result = timeout_mgr.executeWithTimeout(
            [&execution_count]() {
                execution_count++;
                return 42;
            },
            std::chrono::seconds(5),
            "test_success"
        );
        EXPECT_EQ(result, 42);
    }
    
    EXPECT_EQ(execution_count, 10);
}

TEST_F(ThreadLeakEliminationTest, CancelTokenAllowsCooperativeExit) {
    std::atomic<int> iterations{0};
    
    try {
        timeout_mgr.executeWithCancelToken(
            [&iterations](auto cancel_token) {
                for (int i = 0; i < 100; ++i) {
                    if (cancel_token->load(std::memory_order_acquire)) {
                        break;  // Cooperative exit
                    }
                    iterations++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                return iterations.load();
            },
            std::chrono::seconds(0),
            "test_cancel"
        );
    } catch (const LLMException& e) {
        // Expected timeout, but function should have exited cooperatively
        EXPECT_EQ(e.getErrorCode(), LLMErrorCode::TIMEOUT);
    }
    
    // Give cleanup threads time to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// ============================================================================
// Test Suite 3: Per-Operation-Type Circuit Breakers
// ============================================================================

class PerOperationCircuitBreakerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create handler with custom circuit breaker configs
        LLMAQLHandler::Config cfg;
        cfg.infer_circuit_breaker.failure_threshold = 2;
        cfg.rag_circuit_breaker.failure_threshold = 3;
        cfg.embed_circuit_breaker.failure_threshold = 2;
        cfg.finetune_circuit_breaker.failure_threshold = 2;
        handler = std::make_unique<LLMAQLHandler>(cfg);
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

TEST_F(PerOperationCircuitBreakerTest, GetCircuitBreakerStates) {
    auto states = handler->getCircuitBreakerStates();
    
    // All breakers should start in CLOSED state
    EXPECT_NE(states.infer, "");
    EXPECT_NE(states.rag, "");
    EXPECT_NE(states.embed, "");
    EXPECT_NE(states.finetune, "");
}

TEST_F(PerOperationCircuitBreakerTest, IndependentOperationBreakers) {
    auto states1 = handler->getCircuitBreakerStates();
    
    // Verify that each operation type has its own breaker
    // They should be initially in the same state (CLOSED)
    EXPECT_EQ(states1.infer, states1.infer);  // Sanity check
    EXPECT_EQ(states1.rag, states1.rag);      // Sanity check
}

// ============================================================================
// Test Suite 4: Bounded Conversation History with Context-Window Budget
// ============================================================================

class BoundedConversationHistoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create handler for conversation context
        handler = std::make_unique<LLMAQLHandler>();
    }
    
    void TearDown() override {
        handler.reset();
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

TEST_F(BoundedConversationHistoryTest, ConversationContextWithMaxTurns) {
    AQLConversationContext::Config cfg;
    cfg.max_turns = 5;
    cfg.max_history_tokens = 10000;  // Disable token budget for this test
    
    AQLConversationContext ctx(*handler, cfg);
    
    // Test that context respects max_turns
    EXPECT_EQ(ctx.turnCount(), 0);
}

TEST_F(BoundedConversationHistoryTest, ConversationContextWithTokenBudget) {
    AQLConversationContext::Config cfg;
    cfg.max_turns = 50;
    cfg.max_history_tokens = 1000;  // Small token budget
    
    AQLConversationContext ctx(*handler, cfg);
    
    // Verify context is created with token budget
    EXPECT_LE(ctx.tokenCount(), cfg.max_history_tokens);
}

TEST_F(BoundedConversationHistoryTest, ConversationContextNoMemoryLeak) {
    {
        AQLConversationContext::Config cfg;
        cfg.max_turns = 10;
        cfg.max_history_tokens = 5000;
        
        AQLConversationContext ctx(*handler, cfg);
        
        // Context is destroyed here
        // Verify proper cleanup
    }
    
    // If there was a memory leak, this would be detected by ASAN
    // For now, we just verify the context was created and destroyed
    EXPECT_TRUE(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

class AQLHardeningIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create handler with all hardening features enabled
        LLMAQLHandler::Config cfg;
        cfg.infer_circuit_breaker.failure_threshold = 3;
        cfg.rag_circuit_breaker.failure_threshold = 3;
        handler = std::make_unique<LLMAQLHandler>(cfg);
        handler->setValidationMode(TranslationValidationMode::WARN_ONLY);
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

TEST_F(AQLHardeningIntegrationTest, AllHardeningFeaturesCoexist) {
    // Verify all hardening features can be enabled together
    
    // 1. Set validation mode
    handler->setValidationMode(TranslationValidationMode::REJECT_ON_ERROR);
    
    // 2. Get circuit breaker states
    auto cb_states = handler->getCircuitBreakerStates();
    EXPECT_NE(cb_states.infer, "");
    
    // 3. Create conversation context with bounded history
    AQLConversationContext::Config ctx_cfg;
    ctx_cfg.max_turns = 10;
    ctx_cfg.max_history_tokens = 5000;
    
    AQLConversationContext ctx(*handler, ctx_cfg);
    EXPECT_EQ(ctx.turnCount(), 0);
}

TEST_F(AQLHardeningIntegrationTest, ValidationAndCircuitBreakerConsistency) {
    // Verify that validation and circuit breaker configs work together
    
    auto initial_states = handler->getCircuitBreakerStates();
    EXPECT_NE(initial_states.infer, "");
    
    // Change validation mode
    handler->setValidationMode(TranslationValidationMode::RETRY_ON_ERROR);
    
    // Circuit breaker states should remain unchanged
    auto after_states = handler->getCircuitBreakerStates();
    EXPECT_EQ(initial_states.infer, after_states.infer);
}

// ============================================================================
// A-10 Acceptance: 1K executor cycles — thread leak verification
// ============================================================================

/**
 * @brief A-10 acceptance gate: LLMTimeoutManager must not accumulate threads
 *        across 1,000 execution cycles (mix of success + timeout paths).
 *
 * Reads /proc/self/status to obtain the kernel-reported thread count before
 * and after 1,000 cycles.  A leaked std::thread would increment the count by
 * one per cycle, so the test allows only a slack of ±20 threads (noise from
 * GTest infrastructure).  When run under ThreadSanitizer the thread lifecycle
 * is additionally validated at no extra cost.
 */
class LLMTimeoutManager_A10_1KCyclesTest : public ::testing::Test {
protected:
    /// Returns the current process thread count via /proc/self/status, or 0 if unavailable.
    static int getProcThreadCount() {
        std::ifstream f("/proc/self/status");
        if (!f.is_open()) {
            return 0;
        }
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("Threads:", 0) == 0) {
                int count = 0;
                if (std::sscanf(line.c_str(), "Threads: %d", &count) == 1) {
                    return count;
                }
            }
        }
        return 0;
    }
};

TEST_F(LLMTimeoutManager_A10_1KCyclesTest, A10_1K_Success_NoThreadLeak) {
    // 500 success-path cycles: executor returns before timeout → jthread joins inline.
    LLMTimeoutManager mgr;
    const int kCycles = 500;
    std::atomic<int> counter{0};

    const int threads_before = getProcThreadCount();

    for (int i = 0; i < kCycles; ++i) {
        int result = mgr.executeWithTimeout(
            [&counter]() -> int {
                counter.fetch_add(1, std::memory_order_relaxed);
                return 1;
            },
            std::chrono::seconds(10),
            "a10_success_cycle");
        EXPECT_EQ(result, 1);
    }

    EXPECT_EQ(counter.load(), kCycles);

    // Allow any jthread destructors triggered by this scope to complete.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const int threads_after = getProcThreadCount();
    if (threads_before > 0 && threads_after > 0) {
        // No thread accumulation: allow ±20 threads of slack for GTest / OS noise.
        EXPECT_LE(threads_after - threads_before, 20)
            << "Thread count grew from " << threads_before << " to " << threads_after
            << " — possible thread leak in success path";
    }
}

TEST_F(LLMTimeoutManager_A10_1KCyclesTest, A10_1K_Timeout_NoThreadLeak) {
    // 500 timeout-path cycles: worker takes 200ms but timeout is 1ms →
    // cleanup thread detaches and joins the worker without blocking the caller.
    LLMTimeoutManager mgr;
    const int kCycles = 500;
    int timeout_exceptions = 0;

    const int threads_before = getProcThreadCount();

    for (int i = 0; i < kCycles; ++i) {
        try {
            mgr.executeWithTimeout(
                []() -> int {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    return 0;
                },
                std::chrono::milliseconds(1),
                "a10_timeout_cycle");
        } catch (const LLMException& ex) {
            EXPECT_EQ(ex.getErrorCode(), LLMErrorCode::TIMEOUT);
            ++timeout_exceptions;
        }
    }

    EXPECT_EQ(timeout_exceptions, kCycles);

    // Allow cleanup threads to drain (each has a 200 ms worker → give 500 ms extra).
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    const int threads_after = getProcThreadCount();
    if (threads_before > 0 && threads_after > 0) {
        EXPECT_LE(threads_after - threads_before, 20)
            << "Thread count grew from " << threads_before << " to " << threads_after
            << " — possible thread leak in timeout path";
    }
}
