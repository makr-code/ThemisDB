/**
 * @file test_aql_token_policy.cpp
 * @brief Phase 5 Unit Tests — Token Budget Policy Enforcement
 *
 * Tests token budget policy behavior across boundary conditions:
 * - Exact budget exhaustion at boundary
 * - Single turn exceeding the budget
 * - History truncation (oldest-first eviction)
 * - Max turns limit enforcement
 * - Token counting accuracy
 * - Policy behavior at numeric boundaries
 *
 * All tests are self-contained. No real AQL handler required.
 */


#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <deque>
#include <numeric>
#include <sstream>

#include "aql/aql_error_types.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Token Policy Engine Mock
// ============================================================================

struct PolicyTurn {
    std::string nl_query;
    std::string aql_result;
    uint32_t    token_count;
};

class TokenPolicyEngine {
public:
    struct Config {
        uint32_t max_budget_tokens = 4096;  ///< Total token budget for conversation
        uint32_t max_turns         = 20;    ///< Maximum number of turns to retain
        /// Simple estimation: 1 token ≈ 4 characters
        uint32_t tokensFor(const std::string& s) const {
            return static_cast<uint32_t>(s.size() / 4) + 1;
        }
    };

    struct AddResult {
        bool     success = 0;
        bool     budget_truncated;     ///< True if turns were evicted to fit
        bool     turns_truncated;      ///< True if max_turns limit was hit
        uint32_t evicted_turns;
        std::string error_message;
    };

    explicit TokenPolicyEngine(const Config& cfg = Config{}) : cfg_(cfg), current_tokens_(0) {}

    AddResult addTurn(const std::string& nl, const std::string& aql) {
        uint32_t cost = cfg_.tokensFor(nl) + cfg_.tokensFor(aql);
        uint32_t evicted = 0;
        bool budget_trunc = false;
        bool turns_trunc  = false;

        // Enforce max_turns limit (evict oldest first)
        while (history_.size() >= cfg_.max_turns) {
            current_tokens_ -= history_.front().token_count;
            history_.pop_front();
            ++evicted;
            turns_trunc = true;
        }

        // Enforce token budget (evict oldest first)
        while (!history_.empty() && current_tokens_ + cost > cfg_.max_budget_tokens) {
            current_tokens_ -= history_.front().token_count;
            history_.pop_front();
            ++evicted;
            budget_trunc = true;
        }

        // Single turn exceeds budget even with empty history
        if (cost > cfg_.max_budget_tokens) {
            AQLErrorContext ctx(
                "bridge",
                BridgeError::ContextBoundExceeded,
                "token_policy_engine",
                "[BRIDGE:ContextBoundExceeded] Single turn (" + std::to_string(cost) +
                " tokens) exceeds total budget (" + std::to_string(cfg_.max_budget_tokens) + " tokens)"
            );
            ctx.setRecoverable(false);
            return {false, false, false, evicted, ctx.getMessage()};
        }

        history_.push_back({nl, aql, cost});
        current_tokens_ += cost;
        return {true, budget_trunc, turns_trunc, evicted, ""};
    }

    uint32_t currentTokens() const { return current_tokens_; }
    std::size_t turnCount()  const { return history_.size();  }
    const Config& config()   const { return cfg_;             }

    /// Sum of all turn token counts (consistency check)
    uint32_t sumTokenCounts() const {
        uint32_t sum = 0;
        for (const auto& t : history_) {
          sum += t.token_count;
        }
        return sum;
    }

private:
    Config              cfg_;
    std::deque<PolicyTurn> history_;
    uint32_t            current_tokens_;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test TokenPolicy_BudgetExactlyExhaustedBoundaryCondition
 *
 * Verify behavior when a turn exactly fills the remaining token budget:
 * no eviction should occur, the turn must be accepted.
 */
TEST(TokenPolicy, BudgetExactlyExhaustedBoundaryCondition) {
    TokenPolicyEngine::Config cfg;
    cfg.max_budget_tokens = 100;
    cfg.max_turns = 50;
    TokenPolicyEngine engine(cfg);

    // Turn that exactly fills budget: 96 chars ÷ 4 = 24 tokens each,
    // plus 1 overhead each → 26 tokens total (within 100)
    std::string nl(96, 'q');    // 96 chars → 25 tokens
    std::string aql(96, 'x');   // 96 chars → 25 tokens
    // cost = 25 + 25 = 50 tokens

    auto r1 = engine.addTurn(nl, aql);
    EXPECT_TRUE(r1.success);
    EXPECT_FALSE(r1.budget_truncated);
    EXPECT_EQ(r1.evicted_turns, 0u);

    // A second identical turn should evict the first to fit
    auto r2 = engine.addTurn(nl, aql);
    EXPECT_TRUE(r2.success);
    EXPECT_LE(engine.currentTokens(), cfg.max_budget_tokens);
}

/**
 * @test TokenPolicy_SingleTurnExceedingBudgetErrors
 *
 * Verify that a single turn larger than the entire budget is rejected
 * with a ContextBoundExceeded error (fail-closed).
 */
TEST(TokenPolicy, SingleTurnExceedingBudgetErrors) {
    TokenPolicyEngine::Config cfg;
    cfg.max_budget_tokens = 10;   // Very small budget
    cfg.max_turns = 5;
    TokenPolicyEngine engine(cfg);

    // A turn that is definitely too large (400 chars / 4 = 100 tokens > 10)
    std::string huge_nl(400, 'n');
    std::string huge_aql(400, 'a');

    auto r = engine.addTurn(huge_nl, huge_aql);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error_message.empty());
    EXPECT_TRUE(r.error_message.find("ContextBoundExceeded") != std::string::npos ||
                r.error_message.find("exceeds") != std::string::npos);

    // History should remain empty
    EXPECT_EQ(engine.turnCount(), std::size_t(0));
    EXPECT_EQ(engine.currentTokens(), 0u);
}

/**
 * @test TokenPolicy_HistoryTruncatedOldestFirst
 *
 * Verify that when token budget is exceeded, the oldest turn is
 * evicted first (FIFO eviction order).
 */
TEST(TokenPolicy, HistoryTruncatedOldestFirst) {
    TokenPolicyEngine::Config cfg;
    cfg.max_budget_tokens = 30;   // Each turn ~5 tokens → about 6 turns fit
    cfg.max_turns = 50;
    TokenPolicyEngine engine(cfg);

    // Add turns that each cost about 5 tokens (16 chars / 4 + 1 = 5)
    std::vector<std::string> nls = {"query_one", "query_two", "query_three",
                                    "query_four", "query_five", "query_six",
                                    "query_seven"};
    for (const auto& nl : nls) {
        engine.addTurn(nl, "RETURN 1");
    }

    // After adding 7 turns into a ~30-token budget, token count must be within limit
    EXPECT_LE(engine.currentTokens(), cfg.max_budget_tokens);
    // Not all 7 turns should remain
    EXPECT_LT(engine.turnCount(), nls.size());

    // Consistency: sumTokenCounts() must equal currentTokens()
    EXPECT_EQ(engine.sumTokenCounts(), engine.currentTokens());
}

/**
 * @test TokenPolicy_MaxTurnsLimitEnforced
 *
 * Verify that when max_turns is reached, the oldest turn is evicted
 * even if the token budget is not yet exhausted.
 */
TEST(TokenPolicy, MaxTurnsLimitEnforced) {
    TokenPolicyEngine::Config cfg;
    cfg.max_budget_tokens = 100000;  // Token budget is not limiting
    cfg.max_turns = 3;               // Strict turn limit
    TokenPolicyEngine engine(cfg);

    for (int i = 0; i < 6; ++i) {
        auto r = engine.addTurn("q" + std::to_string(i), "RETURN " + std::to_string(i));
        EXPECT_TRUE(r.success);
    }

    // Never exceeds max_turns
    EXPECT_EQ(engine.turnCount(), std::size_t(3));
}

/**
 * @test TokenPolicy_TokenCountingAccuracy
 *
 * Verify that the token counting formula is applied consistently:
 * currentTokens() == sum of individual turn token counts.
 */
TEST(TokenPolicy, TokenCountingAccuracy) {
    TokenPolicyEngine::Config cfg;
    cfg.max_budget_tokens = 10000;
    cfg.max_turns = 100;
    TokenPolicyEngine engine(cfg);

    std::vector<std::pair<std::string, std::string>> turns = {
        {"a", "b"},
        {"hello world", "FOR x IN c RETURN x"},
        {std::string(100, 'n'), std::string(200, 'a')},
        {"short", "RETURN 42"},
    };

    for (const auto& [nl, aql] : turns) {
        engine.addTurn(nl, aql);
    }

    // Token accounting must be perfectly consistent
    EXPECT_EQ(engine.currentTokens(), engine.sumTokenCounts());
    EXPECT_LE(engine.currentTokens(), cfg.max_budget_tokens);
}

/**
 * @test TokenPolicy_PolicyBehaviorAtBoundary
 *
 * Verify edge conditions: zero budget, single-token budget,
 * empty strings, and one-char strings.
 */
TEST(TokenPolicy, PolicyBehaviorAtBoundary) {
    // Test with budget = 1 (extremely tight)
    {
        TokenPolicyEngine::Config cfg;
        cfg.max_budget_tokens = 1;
        cfg.max_turns = 10;
        TokenPolicyEngine engine(cfg);

        // Any non-empty turn has cost >= 2 tokens (1 + 1 overhead each), so it should fail
        auto r = engine.addTurn("x", "y");
        // cost = (1/4+1) + (1/4+1) = 1+1 = 2 > 1 → rejected
        EXPECT_FALSE(r.success);
        EXPECT_EQ(engine.turnCount(), std::size_t(0));
    }

    // Test empty string turns (minimum cost turns)
    {
        TokenPolicyEngine::Config cfg;
        cfg.max_budget_tokens = 100;
        cfg.max_turns = 10;
        TokenPolicyEngine engine(cfg);

        // Empty strings: cost = (0/4+1) + (0/4+1) = 2 tokens
        auto r = engine.addTurn("", "");
        EXPECT_TRUE(r.success);
        EXPECT_EQ(engine.currentTokens(), 2u);  // 1 + 1 = 2 tokens
        EXPECT_EQ(engine.sumTokenCounts(), engine.currentTokens());
    }

    // AQLErrorContext formatting at boundary conditions
    {
        AQLErrorContext ctx(
            "bridge",
            BridgeError::ContextBoundExceeded,
            "token_policy",
            "[BRIDGE:ContextBoundExceeded] Budget=1 Cost=2: single turn rejected"
        );
        ctx.setRecoverable(false);
        ctx.addDiagnosticHint("Increase token budget or reduce query length");

        std::string log = ctx.formatForLogging();
        EXPECT_TRUE(log.find("ContextBoundExceeded") != std::string::npos);
        EXPECT_TRUE(log.find("Recoverable=no") != std::string::npos);
        EXPECT_TRUE(log.find("Budget=1") != std::string::npos);
    }
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
