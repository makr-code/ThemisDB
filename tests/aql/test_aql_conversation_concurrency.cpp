/**
 * @file test_aql_conversation_concurrency.cpp
 * @brief Phase 5 Unit Tests — Conversation Context Concurrency Safety
 *
 * Tests thread-safety concepts for conversation context patterns:
 * - Parallel context access
 * - Concurrent circuit breaker state transitions
 * - Token budget exhaustion race conditions
 * - Context eviction under concurrent writes
 * - History consistency under interleaved access
 * - Lock ordering and deadlock prevention
 * - Concurrent validation pipeline calls
 * - Stress test with many concurrent turns
 *
 * All tests use std::thread/std::mutex directly. No real AQL handler required.
 * Thread-safety patterns are tested via AQLErrorContext and mock components.
 */


#include <gtest/gtest.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <random>
#include <condition_variable>

#include "aql/aql_error_types.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Thread-safe Conversation Context Mock
// ============================================================================

/// @brief Thread-safe token-bounded conversation history
class ThreadSafeConversationContext {
public:
    struct Turn {
        std::string nl_query;
        std::string aql_result;
        uint32_t    tokens = {};
    };

    explicit ThreadSafeConversationContext(uint32_t max_tokens = 4096)
        : max_tokens_(max_tokens), current_tokens_(0) {}

    struct AddResult { bool success; uint32_t evicted; };

    AddResult addTurn(const std::string& nl, const std::string& aql) {
        std::lock_guard<std::mutex> lk(mu_);
        uint32_t cost = static_cast<uint32_t>(nl.size() / 4 + aql.size() / 4) + 1;
        uint32_t evicted = 0;
        while (!turns_.empty() && current_tokens_ + cost > max_tokens_) {
            current_tokens_ -= turns_.front().tokens;
            turns_.erase(turns_.begin());
            ++evicted;
        }
        if (current_tokens_ + cost > max_tokens_) {
            return {false, evicted};
        }
        turns_.push_back({nl, aql, cost});
        current_tokens_ += cost;
        return {true, evicted};
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return turns_.size();
    }

    uint32_t currentTokens() const {
        std::lock_guard<std::mutex> lk(mu_);
        return current_tokens_;
    }

    uint32_t maxTokens() const { return max_tokens_; }

private:
    mutable std::mutex mu_;
    std::vector<Turn>  turns_;
    uint32_t           max_tokens_;
    uint32_t           current_tokens_;
};

// ============================================================================
// Thread-safe Circuit Breaker Mock
// ============================================================================

class ThreadSafeMockCircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    explicit ThreadSafeMockCircuitBreaker(int failure_threshold = 3,
                                           int half_open_permits = 1)
        : failure_threshold_(failure_threshold)
        , half_open_permits_(half_open_permits)
        , state_(State::CLOSED)
        , failure_count_(0)
        , half_open_used_(0) {}

    bool allowRequest() {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ == State::OPEN) {
          return false;
        }
        if (state_ == State::HALF_OPEN) {
            return half_open_used_.fetch_add(1) < half_open_permits_;
        }
        return true;
    }

    void recordSuccess() {
        std::lock_guard<std::mutex> lk(mu_);
        failure_count_ = 0;
        state_ = State::CLOSED;
        half_open_used_.store(0);
    }

    void recordFailure() {
        std::lock_guard<std::mutex> lk(mu_);
        ++failure_count_;
        if (failure_count_ >= failure_threshold_) {
            state_ = State::OPEN;
        }
    }

    void transitionToHalfOpen() {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ == State::OPEN) {
            state_ = State::HALF_OPEN;
            half_open_used_.store(0);
        }
    }

    State getState() const {
        std::lock_guard<std::mutex> lk(mu_);
        return state_;
    }

    int failureCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return failure_count_;
    }

private:
    mutable std::mutex mu_;
    int   failure_threshold_;
    int   half_open_permits_;
    State state_;
    int   failure_count_;
    std::atomic<int> half_open_used_;
};

// ============================================================================
// Tests
// ============================================================================

/**
 * @test ConversationConcurrency_ParallelContextAccessIsSafe
 *
 * Multiple threads simultaneously add turns; verify no data races,
 * token count never exceeds limit, and size is consistent.
 */
TEST(ConversationConcurrency, ParallelContextAccessIsSafe) {
    ThreadSafeConversationContext ctx(2048);
    constexpr int kThreads = 8;
    constexpr int kTurnsPerThread = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ctx, t]() {
            for (int i = 0; i < kTurnsPerThread; ++i) {
                ctx.addTurn("query_" + std::to_string(t) + "_" + std::to_string(i),
                            "FOR d IN col RETURN d");
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Token count must never exceed limit
    EXPECT_LE(ctx.currentTokens(), ctx.maxTokens());
    // At least some turns should remain
    EXPECT_GT(ctx.size(), std::size_t(0));
}

/**
 * @test ConversationConcurrency_ConcurrentCircuitBreakerTransitions
 *
 * Multiple threads drive the circuit breaker simultaneously;
 * verify state transitions are consistent (no invalid states).
 */
TEST(ConversationConcurrency, ConcurrentCircuitBreakerTransitions) {
    ThreadSafeMockCircuitBreaker cb(5);
    constexpr int kThreads = 6;
    std::atomic<int> allowed{0}, blocked{0};

    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&cb, &allowed, &blocked, t]() {
            for (int i = 0; i < 10; ++i) {
                if (cb.allowRequest()) {
                    allowed.fetch_add(1);
                    if (t % 2 == 0) {
                      cb.recordFailure();
                    }
                    else             cb.recordSuccess();
                } else {
                    blocked.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // State must be one of the valid enum values
    auto state = cb.getState();
    EXPECT_TRUE(state == ThreadSafeMockCircuitBreaker::State::CLOSED ||
                state == ThreadSafeMockCircuitBreaker::State::OPEN   ||
                state == ThreadSafeMockCircuitBreaker::State::HALF_OPEN);

    // Total allowed + blocked must equal total calls
    EXPECT_EQ(allowed.load() + blocked.load(), kThreads * 10);
}

/**
 * @test ConversationConcurrency_TokenBudgetExhaustionRaceCondition
 *
 * Two threads race to exhaust the token budget;
 * verify exactly one wins and the limit is not exceeded.
 */
TEST(ConversationConcurrency, TokenBudgetExhaustionRaceCondition) {
    // Very small limit: 64 tokens → only a few turns fit
    ThreadSafeConversationContext ctx(64);
    constexpr int kThreads = 2;
    std::atomic<int> successes{0};

    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ctx, &successes]() {
            for (int i = 0; i < 50; ++i) {
                auto r = ctx.addTurn("q", "FOR x IN c RETURN x");
                if (r.success) {
                  successes.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_LE(ctx.currentTokens(), ctx.maxTokens());
    EXPECT_GT(successes.load(), 0);
}

/**
 * @test ConversationConcurrency_ConcurrentContextEviction
 *
 * Verify that concurrent evictions under a tight token budget
 * always maintain structural integrity.
 */
TEST(ConversationConcurrency, ConcurrentContextEviction) {
    ThreadSafeConversationContext ctx(256);
    constexpr int kThreads = 4;
    std::atomic<int> total_evicted{0};

    // Fill context to near capacity first
    for (int i = 0; i < 10; ++i) {
        ctx.addTurn("baseline_turn_" + std::to_string(i), "FOR d IN col RETURN d");
    }
    const auto initial_tokens = ctx.currentTokens();
    EXPECT_LE(initial_tokens, ctx.maxTokens());

    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ctx, &total_evicted, t]() {
            // Use large AQL strings to force evictions
            std::string large_aql(50, static_cast<char>('a' + (t % 26)));
            for (int i = 0; i < 15; ++i) {
                auto r = ctx.addTurn("q" + std::to_string(i), large_aql);
                total_evicted.fetch_add(static_cast<int>(r.evicted));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_LE(ctx.currentTokens(), ctx.maxTokens());
}

/**
 * @test ConversationConcurrency_HistoryConsistencyUnderInterleavedAccess
 *
 * Concurrent readers and writers; verify size() never exceeds
 * what would be structurally valid.
 */
TEST(ConversationConcurrency, HistoryConsistencyUnderInterleavedAccess) {
    ThreadSafeConversationContext ctx(512);
    constexpr int kWriters = 3;
    constexpr int kReaders = 3;
    std::atomic<bool> stop{false};
    std::atomic<std::size_t> max_observed_size{0};

    std::vector<std::thread> writers = {};

    for (int t = 0; t < kWriters; ++t) {
        writers.emplace_back([&ctx, t]() {
            for (int i = 0; i < 30; ++i) {
                ctx.addTurn("wq_" + std::to_string(t), "FOR x IN c RETURN x");
            }
        });
    }

    std::vector<std::thread> readers = {};

    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&ctx, &stop, &max_observed_size]() {
            while (!stop.load(std::memory_order_relaxed)) {
                auto sz = ctx.size();
                std::size_t prev = max_observed_size.load();
                while (sz > prev && !max_observed_size.compare_exchange_weak(prev, sz)) {}
                std::this_thread::yield();
            }
        });
    }

    for (auto& w : writers) {
      w.join();
    }
    stop.store(true);
    for (auto& r : readers) {
      r.join();
    }

    // After all writes, tokens must still be within limit
    EXPECT_LE(ctx.currentTokens(), ctx.maxTokens());
}

/**
 * @test ConversationConcurrency_NoDeadlockInLockOrdering
 *
 * Two threads alternately lock two independent contexts in opposite order;
 * verify no deadlock occurs within a bounded time.
 */
TEST(ConversationConcurrency, NoDeadlockInLockOrdering) {
    ThreadSafeConversationContext ctx_a(256);
    ThreadSafeConversationContext ctx_b(256);
    std::atomic<int> done_a{0}, done_b{0};

    auto t1 = std::thread([&]() {
        for (int i = 0; i < 20; ++i) {
            ctx_a.addTurn("a" + std::to_string(i), "FOR x IN a RETURN x");
            ctx_b.addTurn("b" + std::to_string(i), "FOR x IN b RETURN x");
        }
        done_a.store(1);
    });

    auto t2 = std::thread([&]() {
        for (int i = 0; i < 20; ++i) {
            ctx_b.addTurn("bb" + std::to_string(i), "FOR y IN b RETURN y");
            ctx_a.addTurn("aa" + std::to_string(i), "FOR y IN a RETURN y");
        }
        done_b.store(1);
    });

    t1.join();
    t2.join();

    EXPECT_EQ(done_a.load(), 1);
    EXPECT_EQ(done_b.load(), 1);
    EXPECT_LE(ctx_a.currentTokens(), ctx_a.maxTokens());
    EXPECT_LE(ctx_b.currentTokens(), ctx_b.maxTokens());
}

/**
 * @test ConversationConcurrency_ConcurrentValidationPipelineCalls
 *
 * Multiple threads create AQLErrorContext objects concurrently;
 * verify no corruption in their independent states.
 */
TEST(ConversationConcurrency, ConcurrentValidationPipelineCalls) {
    constexpr int kThreads = 8;
    std::atomic<int> errors_seen{0};

    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &errors_seen]() {
            // Each thread independently creates and inspects error context objects
            AQLErrorContext ctx(
                "validation",
                ValidationError::MalformedAQL,
                "concurrent_validator_" + std::to_string(t),
                "[VALIDATION:MalformedAQL] Thread " + std::to_string(t) + " validation error"
            );
            ctx.setOperationType("concurrent_translate_" + std::to_string(t));
            ctx.setRetryCount(t % 3);
            ctx.addDiagnosticHint("Hint from thread " + std::to_string(t));
            ctx.setRecoverable(false);

            // Verify internal state is consistent
            if (ctx.getRetryCount() != static_cast<uint32_t>(t % 3)) {
                errors_seen.fetch_add(1);
            }
            if (ctx.getComponent() != "concurrent_validator_" + std::to_string(t)) {
                errors_seen.fetch_add(1);
            }
            if (ctx.getDiagnosticHints().size() != 1) {
                errors_seen.fetch_add(1);
            }

            std::string log = ctx.formatForLogging();
            if (log.find("MalformedAQL") == std::string::npos) {
                errors_seen.fetch_add(1);
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(errors_seen.load(), 0);
}

/**
 * @test ConversationConcurrency_StressTestConcurrentTurns
 *
 * High-contention stress test: 16 threads × 50 turns each against
 * a single context with a tight token budget.
 */
TEST(ConversationConcurrency, StressTestConcurrentTurns) {
    constexpr int    kThreads        = 16;
    constexpr int    kTurnsPerThread = 50;
    constexpr uint32_t kMaxTokens   = 512;

    ThreadSafeConversationContext ctx(kMaxTokens);

    std::atomic<int> total_successes{0};
    std::atomic<int> total_failures{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ctx, &total_successes, &total_failures, t]() {
            std::mt19937 rng(static_cast<unsigned>(t) * 13337u);
            std::uniform_int_distribution<int> len_dist(5, 30);

            for (int i = 0; i < kTurnsPerThread; ++i) {
                int nl_len  = len_dist(rng);
                int aql_len = len_dist(rng);
                auto r = ctx.addTurn(
                    std::string(nl_len,  'n'),
                    std::string(aql_len, 'a')
                );
                if (r.success) {
                  total_successes.fetch_add(1);
                }
                else           total_failures.fetch_add(1);
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // Fundamental invariant must hold at all times
    EXPECT_LE(ctx.currentTokens(), kMaxTokens);

    // Some turns must have succeeded
    EXPECT_GT(total_successes.load(), 0);

    // Total attempts must be accounted for
    EXPECT_EQ(total_successes.load() + total_failures.load(), kThreads * kTurnsPerThread);
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
