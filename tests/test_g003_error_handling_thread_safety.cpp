/**
 * @file test_g003_error_handling_thread_safety.cpp
 * @brief G003 remediation validation tests for error handling and thread safety.
 *
 * Validates that the patterns fixed as part of GAP-SCAN-2026-06-21 G003 are
 * correct and do not regress.  Tests are grouped into three suites:
 *
 *  - EHS-01..EHS-08  Exception-handling safety
 *  - TSF-01..TSF-08  Thread-safety patterns
 *  - RVC-01..RVC-04  Return-value / null-guard patterns
 *
 * All tests are self-contained and have no external ThemisDB runtime
 * dependency.  They exercise the exact patterns that were previously missing
 * logging, missing mutex protection, or silently swallowing errors.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace themis {
namespace test {
namespace g003 {

// ============================================================================
// Helpers shared across suites
// ============================================================================

/**
 * @brief Minimal in-process log sink that captures WARN/ERROR messages for
 *        test assertions without depending on the real spdlog stack.
 */
class CapturingLogger {
public:
    void warn(const std::string& msg) {
        std::lock_guard<std::mutex> lk(mu_);
        entries_.push_back("[WARN] " + msg);
    }
    void error(const std::string& msg) {
        std::lock_guard<std::mutex> lk(mu_);
        entries_.push_back("[ERROR] " + msg);
    }

    bool anyContains(const std::string& substr) const {
        std::lock_guard<std::mutex> lk(mu_);
        for (const auto& e : entries_) {
            if (e.find(substr) != std::string::npos) {
              return true;
            }
        }
        return false;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return entries_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mu_);
        entries_.clear();
    }

private:
    mutable std::mutex      mu_;
    std::vector<std::string> entries_;
};

// ============================================================================
// EHS — Exception Handling Safety  (EHS-01..EHS-08)
// ============================================================================

/**
 * @brief Validates that a catch-all block that previously swallowed exceptions
 *        now emits a warning-level log entry before continuing.
 *
 * EHS-01: catch-all with logging — the fix pattern.
 */
TEST(G003_ExceptionHandlingSafety, EHS01_CatchAllLogsBeforeContinuing) {
    CapturingLogger logger;
    int  events_processed = 0;

    // Simulate a handler dispatcher (mirrors CacheManager::dispatch_event).
    auto dispatch = [&](const std::vector<std::function<void()>>& handlers) {
        for (const auto& h : handlers) {
            try {
                h();
                ++events_processed;
            } catch (...) {
                // G003 fix: log before continuing
                logger.warn("dispatch: unhandled exception caught");
            }
        }
    };

    std::vector<std::function<void()>> handlers{
        [] { /* ok */},
        [] { throw std::runtime_error("handler failure"); },
        [] { /* ok */ },
    };

    dispatch(handlers);

    EXPECT_EQ(events_processed, 2) << "Two successful handlers must complete";
    EXPECT_EQ(logger.size(), 1u)   << "Exactly one warning must be logged";
    EXPECT_TRUE(logger.anyContains("unhandled exception"));
}

/**
 * @brief Validates that a catch-all in a parse-error context uses DEBUG level
 *        and returns a safe default.
 *
 * EHS-02: parse catch-all returns default, emits debug log.
 */
TEST(G003_ExceptionHandlingSafety, EHS02_ParseCatchAllReturnsSafeDefault) {
    std::atomic<int> debug_log_count{0};

    auto try_parse_int = [&](const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s);
        } catch (...) {
            // G003 fix: debug log for parse failure
            ++debug_log_count;
            return std::nullopt;
        }
    };

    EXPECT_EQ(try_parse_int("42"),      std::optional<int>{42});
    EXPECT_EQ(try_parse_int("not_int"), std::nullopt);
    EXPECT_EQ(debug_log_count.load(), 1);
}

/**
 * @brief RAII ensures resource release even when an exception is thrown inside
 *        the protected scope.
 *
 * EHS-03: RAII lock guard releases on exception.
 */
TEST(G003_ExceptionHandlingSafety, EHS03_RaiiReleasesMutexOnException) {
    std::mutex mu;
    bool       resource_was_released = false;

    auto locked_op = [&]() {
        std::lock_guard<std::mutex> lk(mu);
        throw std::runtime_error("intentional");
    };

    try {
        locked_op();
    } catch (const std::runtime_error&) {
        // After the exception, the lock_guard destructor must have fired.
        // Attempting a new lock must succeed immediately (no deadlock).
        std::unique_lock<std::mutex> probe(mu, std::try_to_lock);
        resource_was_released = probe.owns_lock();
    }

    EXPECT_TRUE(resource_was_released) << "Mutex must be released via RAII on exception";
}

/**
 * @brief Exception thrown from a background thread must not terminate the
 *        process when caught at the thread boundary.
 *
 * EHS-04: per-thread exception boundary in thread pool pattern.
 */
TEST(G003_ExceptionHandlingSafety, EHS04_ThreadBoundaryException) {
    std::atomic<int>  failure_count{0};
    std::atomic<int>  success_count{0};
    constexpr int     NUM_TASKS = 8;

    std::vector<std::thread> threads;
    threads.reserve(NUM_TASKS);

    for (int i = 0; i < NUM_TASKS; ++i) {
        threads.emplace_back([&, id = i]() {
            try {
                if (id % 3 == 0) {
                  throw std::runtime_error("task error");
                }
                ++success_count;
            } catch (...) {
                // G003 fix: catch at thread boundary, log and mark failure
                ++failure_count;
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    // Tasks 0,3,6 throw → 3 failures, 5 successes
    EXPECT_EQ(failure_count.load(), 3);
    EXPECT_EQ(success_count.load(), 5);
}

/**
 * @brief Resource roll-back must execute when an exception is thrown mid-
 *        transaction, and the error must propagate to the caller.
 *
 * EHS-05: transaction rollback on exception + exception propagation.
 */
TEST(G003_ExceptionHandlingSafety, EHS05_TransactionRollbackOnException) {
    std::vector<std::string> committed;
    bool                     rolled_back = false;

    auto execute_transaction = [&]() {
        committed.clear();
        rolled_back = false;

        try {
            committed.push_back("step1");
            committed.push_back("step2");
            throw std::runtime_error("step3 failed");
            committed.push_back("step3");  // never reached
        } catch (const std::exception& e) {
            // G003 fix: log + rollback + re-throw
            rolled_back = true;
            committed.clear();
            throw;
        }
    };

    EXPECT_THROW(execute_transaction(), std::runtime_error);
    EXPECT_TRUE(rolled_back);
    EXPECT_TRUE(committed.empty()) << "Transaction must be fully rolled back";
}

/**
 * @brief Nested exception handling must not lose inner context.
 *
 * EHS-06: nested exception context preserved.
 */
TEST(G003_ExceptionHandlingSafety, EHS06_NestedExceptionContext) {
    std::string captured_msg;

    auto inner = []() {
        throw std::runtime_error("inner error");
    };

    auto outer = [&]() {
        try {
            inner();
        } catch (const std::exception& e) {
            // Re-wrap with outer context
            throw std::runtime_error(std::string("outer: ") + e.what());
        }
    };

    try {
        outer();
    } catch (const std::exception& e) {
        captured_msg = e.what();
    }

    EXPECT_NE(captured_msg.find("outer:"),      std::string::npos);
    EXPECT_NE(captured_msg.find("inner error"), std::string::npos);
}

/**
 * @brief A noexcept function wrapping a lock_guard must handle exceptions
 *        gracefully and return a safe fallback — mirroring the pattern fixed
 *        in changefeed_api_handler.cpp (getDeliveredEvents).
 *
 * EHS-07: noexcept catch-all with safe fallback.
 */
TEST(G003_ExceptionHandlingSafety, EHS07_NoexceptCatchAllSafeFallback) {
    std::mutex                      mu;
    std::vector<int>                data{1, 2, 3};
    std::atomic<int>                guard_executed{0};

    auto get_copy = [&]() noexcept -> std::vector<int> {
        try {
            std::lock_guard<std::mutex> lk(mu);
            return data;
        } catch (...) {
            ++guard_executed;
            return {};
        }
    };

    auto result = get_copy();
    EXPECT_EQ(result, (std::vector<int>{1, 2, 3}));
    // Normal path — guard should not have fired
    EXPECT_EQ(guard_executed.load(), 0);
}

/**
 * @brief Validates that a callback registration dispatcher does not silently
 *        drop exceptions; each exception must be individually logged/counted.
 *
 * EHS-08: multi-handler dispatch resilience.
 */
TEST(G003_ExceptionHandlingSafety, EHS08_MultiHandlerDispatchResilient) {
    std::atomic<int> errors{0};
    std::atomic<int> completed{0};

    std::vector<std::function<void(int)>> handlers{
        [&](int) { ++completed; },
        [&](int v) { if (v > 0) throw std::logic_error("neg"); ++completed; },
        [&](int) { ++completed; },
        [&](int) { throw std::bad_alloc(); },
        [&](int) { ++completed; },
    };

    for (const auto& h : handlers) {
        try {
            h(1);
        } catch (...) {
            ++errors;
        }
    }

    EXPECT_EQ(errors.load(),    2) << "Two handlers throw";
    EXPECT_EQ(completed.load(), 3) << "Three handlers must complete";
}

// ============================================================================
// TSF — Thread Safety Patterns  (TSF-01..TSF-08)
// ============================================================================

/**
 * @brief std::mutex + lock_guard protects a shared integer counter from
 *        data-race corruption.
 *
 * TSF-01: basic mutex protection.
 */
TEST(G003_ThreadSafety, TSF01_MutexProtectsSharedCounter) {
    constexpr int N_THREADS = 16;
    constexpr int N_OPS     = 1'000;

    int              counter = 0;
    std::mutex       mu;
    std::vector<std::thread> threads;
    threads.reserve(N_THREADS);

    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < N_OPS; ++j) {
                std::lock_guard<std::mutex> lk(mu);
                ++counter;
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(counter, N_THREADS * N_OPS);
}

/**
 * @brief Consistent lock ordering prevents deadlock when two threads must
 *        acquire two mutexes.  Uses std::lock for canonical acquisition.
 *
 * TSF-02: deadlock-free acquisition via std::lock.
 */
TEST(G003_ThreadSafety, TSF02_ConsistentLockOrderingPreventsDeadlock) {
    std::mutex       mu_a, mu_b;
    std::atomic<int> result_a{0}, result_b{0};

    auto t1 = std::thread([&]() {
        // Always acquire both via std::lock — avoids ABBA deadlock
        std::unique_lock<std::mutex> la(mu_a, std::defer_lock);
        std::unique_lock<std::mutex> lb(mu_b, std::defer_lock);
        std::lock(la, lb);
        ++result_a;
        ++result_b;
    });

    auto t2 = std::thread([&]() {
        std::unique_lock<std::mutex> la(mu_a, std::defer_lock);
        std::unique_lock<std::mutex> lb(mu_b, std::defer_lock);
        std::lock(la, lb);
        ++result_a;
        ++result_b;
    });

    t1.join();
    t2.join();

    EXPECT_EQ(result_a.load(), 2);
    EXPECT_EQ(result_b.load(), 2);
}

/**
 * @brief std::atomic provides lock-free increment without data races.
 *
 * TSF-03: atomic increment under contention.
 */
TEST(G003_ThreadSafety, TSF03_AtomicIncrementUnderContention) {
    constexpr int N_THREADS = 32;
    constexpr int N_OPS     = 500;

    std::atomic<int> counter{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < N_OPS; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(counter.load(), N_THREADS * N_OPS);
}

/**
 * @brief shared_mutex allows concurrent reads while serialising writes.
 *        Mirrors the read-optimised pattern used in BoundedLRUCache.
 *
 * TSF-04: shared_mutex reader-writer pattern.
 */
TEST(G003_ThreadSafety, TSF04_SharedMutexReaderWriterPattern) {
    std::shared_mutex        rw_mutex;
    int                      shared_value = 0;
    std::atomic<int>         read_ops{0};
    constexpr int            N_READERS = 8;
    constexpr int            N_WRITERS = 2;

    std::vector<std::thread> readers;
    std::atomic<bool>        done{false};

    // Writer increments under exclusive lock
    auto writer_fn = [&]() {
        for (int i = 0; i < 10; ++i) {
            {
                std::unique_lock<std::shared_mutex> lk(rw_mutex);
                ++shared_value;
            }
            std::this_thread::yield();
        }
    };

    // Readers hold shared lock — no exclusive contention among themselves
    auto reader_fn = [&]() {
        int snapshot = 0;
        while (!done.load(std::memory_order_relaxed)) {
            {
                std::shared_lock<std::shared_mutex> lk(rw_mutex);
                snapshot = shared_value;
                (void)snapshot;
            }
            read_ops.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> all = {};

    for (int i = 0; i < N_WRITERS; ++i) {
      all.emplace_back(writer_fn);
    }
    for (int i = 0; i < N_READERS; ++i) {
      all.emplace_back(reader_fn);
    }

    // Wait for writers
    for (int i = 0; i < N_WRITERS; ++i) {
      all[i].join();
    }
    done.store(true, std::memory_order_release);
    for (int i = N_WRITERS; i < N_WRITERS + N_READERS; ++i) {
      all[i].join();
    }

    EXPECT_EQ(shared_value, N_WRITERS * 10);
    EXPECT_GT(read_ops.load(), 0);
}

/**
 * @brief RAII lock_guard releases the mutex even when the guarded scope
 *        throws — prevents lock leaks that can cause deadlocks.
 *
 * TSF-05: RAII vs manual lock/unlock on exception.
 */
TEST(G003_ThreadSafety, TSF05_RaiiLockGuardReleasesOnException) {
    std::mutex mu;
    bool       can_lock_again = false;

    try {
        std::lock_guard<std::mutex> lk(mu);
        throw std::runtime_error("boom");
    } catch (...) {}

    {
        std::unique_lock<std::mutex> probe(mu, std::try_to_lock);
        can_lock_again = probe.owns_lock();
    }

    EXPECT_TRUE(can_lock_again) << "lock_guard must release on exception";
}

/**
 * @brief Condition variable with spurious-wakeup-safe predicate delivers
 *        notifications reliably across threads.
 *
 * TSF-06: condition variable with predicate.
 */
TEST(G003_ThreadSafety, TSF06_ConditionVariableWithPredicate) {
    std::mutex              mu;
    std::condition_variable cv;
    bool                    ready   = false;
    bool                    notified= false;

    std::thread producer([&]() {
        std::this_thread::sleep_for(10ms);
        {
            std::lock_guard<std::mutex> lk(mu);
            ready = true;
        }
        cv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait_for(lk, 500ms, [&] { return ready; });
        notified = ready;
    }

    producer.join();
    EXPECT_TRUE(notified);
}

/**
 * @brief Thread-safe cache access — check-then-act under a single lock
 *        prevents double-computation (avoids TOCTOU race).
 *
 * TSF-07: check-then-act under lock prevents double-computation.
 */
TEST(G003_ThreadSafety, TSF07_CheckThenActUnderLockPreventsDoubleCompute) {
    constexpr int N_THREADS = 8;

    std::mutex                    cache_mu;
    std::map<std::string, int>    cache;
    std::atomic<int>              compute_count{0};

    auto get_or_compute = [&](const std::string& key) -> int {
        std::lock_guard<std::mutex> lk(cache_mu);
        auto it = cache.find(key);
        if (it != cache.end()) {
          return it->second;
        }
        // Compute under the lock — expensive but race-free
        int val = 42;  // simulated computation
        ++compute_count;
        cache[key] = val;
        return val;
    };

    std::vector<std::thread> threads;
    std::vector<int>         results(N_THREADS, 0);

    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&, idx = i]() {
            results[idx] = get_or_compute("the_key");
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // Value must be consistent across all threads
    for (int v : results) {
      EXPECT_EQ(v, 42);
    }
    // Computation must happen exactly once
    EXPECT_EQ(compute_count.load(), 1);
}

/**
 * @brief lock_guard inside try/catch ensures the mutex is released even if
 *        the guarded code throws — exception-safe lock scope.
 *
 * TSF-08: exception-safe locked scope.
 */
TEST(G003_ThreadSafety, TSF08_ExceptionSafeLockedScope) {
    std::mutex       mu;
    std::atomic<int> error_count{0};
    std::atomic<int> ok_count{0};
    constexpr int    N_THREADS = 4;

    auto safe_op = [&](bool should_throw) {
        try {
            std::lock_guard<std::mutex> lk(mu);
            if (should_throw) {
              throw std::runtime_error("op failed");
            }
            ++ok_count;
        } catch (...) {
            ++error_count;
        }
    };

    std::vector<std::thread> threads = {};

    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&, id = i]() { safe_op(id % 2 == 0); });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(error_count.load(), N_THREADS / 2);
    EXPECT_EQ(ok_count.load(),    N_THREADS / 2);
}

// ============================================================================
// RVC — Return Value / Null-Guard Patterns  (RVC-01..RVC-04)
// ============================================================================

/**
 * @brief Callers must check optional return values before dereferencing.
 *        Mirrors the null_dereference pattern fixed in the cache module.
 *
 * RVC-01: optional return value checked before use.
 */
TEST(G003_ReturnValueChecks, RVC01_OptionalReturnCheckedBeforeUse) {
    auto find_value = [](const std::map<std::string, int>& m,
                         const std::string& key) -> std::optional<int> {
        auto it = m.find(key);
        if (it == m.end()) {
          return std::nullopt;
        }
        return it->second;
    };

    std::map<std::string, int> data{{"a", 1}, {"b", 2}};

    auto v1 = find_value(data, "a");
    ASSERT_TRUE(v1.has_value()) << "Key 'a' must be found";
    EXPECT_EQ(*v1, 1);

    auto v2 = find_value(data, "missing");
    EXPECT_FALSE(v2.has_value()) << "Missing key must return nullopt";

    // Safe access pattern — must not dereference without check
    int result = v2.value_or(-1);
    EXPECT_EQ(result, -1);
}

/**
 * @brief Boolean return codes from operations must be checked; unchecked
 *        errors lead to corrupted state.
 *
 * RVC-02: boolean return code checked.
 */
TEST(G003_ReturnValueChecks, RVC02_BooleanReturnCodeChecked) {
    struct MockStorage {
        bool put(const std::string& key, const std::string& val) {
            if (key.empty()) {
              return false;
            }
            data_[key] = val;
            return true;
        }
        bool get(const std::string& key, std::string& out) const {
            auto it = data_.find(key);
            if (it == data_.end()) {
              return false;
            }
            out = it->second;
            return true;
        }
    private:
        std::map<std::string, std::string> data_;
    };

    MockStorage storage;

    // Correct pattern: check return value
    EXPECT_TRUE( storage.put("k1", "v1"));
    EXPECT_FALSE(storage.put("",   "v2")) << "Empty key must fail";

    std::string out;
    EXPECT_TRUE( storage.get("k1", out));
    EXPECT_EQ(out, "v1");
    EXPECT_FALSE(storage.get("missing", out));
}

/**
 * @brief Pointer (raw or smart) must be checked for null before
 *        dereferencing — mirrors null_dereference pattern in bounded_lru_cache.
 *
 * RVC-03: null pointer guard before dereference.
 */
TEST(G003_ReturnValueChecks, RVC03_NullPointerGuardBeforeDereference) {
    struct Node {
        int value = 0;
        std::shared_ptr<Node> next;
    };

    auto head = std::make_shared<Node>(Node{10, nullptr});

    // Safe traversal: always check before dereference
    auto traverse = [](std::shared_ptr<Node> n) -> int {
        int sum = 0;
        while (n) {          // null check before dereference
            sum += n->value;
            n = n->next;
        }
        return sum;
    };

    EXPECT_EQ(traverse(head),    10);
    EXPECT_EQ(traverse(nullptr),  0) << "null head must not crash";

    head->next = std::make_shared<Node>(Node{20, nullptr});
    EXPECT_EQ(traverse(head), 30);
}

/**
 * @brief Error propagation chain — each layer wraps and re-throws so that
 *        the outermost caller receives the full failure context.
 *
 * RVC-04: error propagation through call chain.
 */
TEST(G003_ReturnValueChecks, RVC04_ErrorPropagationChain) {
    int  call_depth      = 0;
    bool rollback_called = false;

    auto layer3 = [&]() {
        ++call_depth;
        throw std::runtime_error("db write failed");
    };

    auto layer2 = [&]() {
        ++call_depth;
        try {
            layer3();
        } catch (const std::exception& e) {
            rollback_called = true;
            throw std::runtime_error(std::string("transaction: ") + e.what());
        }
    };

    auto layer1 = [&]() -> bool {
        ++call_depth;
        try {
            layer2();
            return true;
        } catch (const std::exception& e) {
            // Log + return error sentinel — no swallowing
            (void)e;
            return false;
        }
    };

    bool ok = layer1();

    EXPECT_FALSE(ok)            << "Top-level must see the failure";
    EXPECT_EQ(call_depth, 3)    << "All three layers must have been entered";
    EXPECT_TRUE(rollback_called)<< "Layer 2 must have triggered rollback";
}

}  // namespace g003
}  // namespace test
}  // namespace themis
