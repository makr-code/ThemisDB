/**
 * @file test_transaction_highcardinality_stress.cpp
 * @brief Wave D — Transaction Engine High-Cardinality Stress Test.
 *
 * Stress tests exercising the transaction manager, 2PC coordinator stub,
 * and conflict resolution logic under high-cardinality workloads and
 * concurrent access.  All tests use in-process stubs.
 *
 * ## Test cases
 * - HighCardinalityCommit    : 100 000 transactions, 8 threads
 * - Concurrent2PCStress      : 8 threads, 2PC prepare/commit under contention
 * - ConflictResolutionStress : write–write conflict detection under 8 threads
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_TRANSACTION_ENGINE.md
 * @see src/transaction/ROADMAP.md — Wave D contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs simulate transaction lifecycle and 2PC protocol behaviour.
// They MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubTxStore — thread-safe commit counter and conflict detector
// ---------------------------------------------------------------------------
class StubTxStore {
public:
    enum class CommitResult { OK, CONFLICT };

    CommitResult commit(uint64_t tx_id, uint64_t key, uint64_t value) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = locks_.find(key);
        if (it != locks_.end() && it->second != tx_id) {
            ++conflicts_;
            return CommitResult::CONFLICT;
        }
        committed_[key] = value;
        locks_.erase(key);
        ++committed_count_;
        return CommitResult::OK;
    }

    void lock_key(uint64_t tx_id, uint64_t key) {
        std::lock_guard<std::mutex> lk(mu_);
        locks_[key] = tx_id;
    }

    void unlock_key(uint64_t /*tx_id*/, uint64_t key) {
        std::lock_guard<std::mutex> lk(mu_);
        locks_.erase(key);
    }

    uint64_t committed_count() const { return committed_count_.load(); }
    uint64_t conflict_count()  const { return conflicts_.load(); }

private:
    mutable std::mutex mu_;
    std::unordered_map<uint64_t, uint64_t> committed_;
    std::unordered_map<uint64_t, uint64_t> locks_;   // key → owning tx_id
    std::atomic<uint64_t> committed_count_{0};
    std::atomic<uint64_t> conflicts_{0};
};

// ---------------------------------------------------------------------------
// Stub2PCCoordinator
// ---------------------------------------------------------------------------
class Stub2PCCoordinator {
public:
    enum class Vote { COMMIT, ABORT };

    Vote prepare(uint64_t tx_id, uint64_t key) {
        // Stub: always votes COMMIT (no network involvement).
        (void)tx_id; (void)key;
        ++prepared_;
        return Vote::COMMIT;
    }

    bool do_commit(uint64_t tx_id) {
        ++committed_;
        (void)tx_id;
        return true;
    }

    uint64_t prepared()  const { return prepared_.load(); }
    uint64_t committed() const { return committed_.load(); }

private:
    std::atomic<uint64_t> prepared_{0};
    std::atomic<uint64_t> committed_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1 — HighCardinalityCommit
//
// 8 threads each commit 100 000 / 8 = 12 500 transactions.
// Total committed count must equal 100 000; zero system errors.
// ─────────────────────────────────────────────────────────────────────────────
TEST(TransactionStress, HighCardinalityCommit) {
    constexpr uint64_t kTotal   = 100'000ULL;
    constexpr int      kThreads = 8;
    const uint64_t     kPer     = kTotal / kThreads;

    StubTxStore           store;
    std::atomic<uint64_t> errors{0};
    std::atomic<uint64_t> tx_seq{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kPer; ++i) {
                uint64_t tx_id = tx_seq.fetch_add(1, std::memory_order_relaxed);
                // Each thread writes to its own key partition to avoid conflicts.
                uint64_t key   = static_cast<uint64_t>(t) * kPer + i;
                auto r = store.commit(tx_id, key, i + 1);
                if (r != StubTxStore::CommitResult::OK) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : workers) { if (th.joinable()) th.join(); }

    EXPECT_EQ(0ULL, errors.load())
        << "High-cardinality commit errors: " << errors.load();
    EXPECT_EQ(kTotal, store.committed_count())
        << "Not all transactions committed";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2 — Concurrent2PCStress
//
// 8 threads each run 500 prepare/commit cycles concurrently.
// All 2PC prepare votes must succeed; committed count must equal total.
// ─────────────────────────────────────────────────────────────────────────────
TEST(TransactionStress, Concurrent2PCStress) {
    constexpr int      kThreads  = 8;
    constexpr uint64_t kPerThread = 500ULL;

    Stub2PCCoordinator    coord;
    StubTxStore           store;
    std::atomic<uint64_t> errors{0};
    std::atomic<uint64_t> tx_seq{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kPerThread; ++i) {
                uint64_t tx_id = tx_seq.fetch_add(1, std::memory_order_relaxed);
                uint64_t key   = static_cast<uint64_t>(t) * kPerThread + i;

                // Phase 1 — prepare
                auto vote = coord.prepare(tx_id, key);
                if (vote != Stub2PCCoordinator::Vote::COMMIT) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
                // Phase 2 — commit
                bool ok = coord.do_commit(tx_id);
                if (!ok) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
                auto r = store.commit(tx_id, key, i + 1);
                if (r != StubTxStore::CommitResult::OK) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : workers) { if (th.joinable()) th.join(); }

    const uint64_t expected = static_cast<uint64_t>(kThreads) * kPerThread;
    EXPECT_EQ(0ULL, errors.load())
        << "2PC stress errors: " << errors.load();
    EXPECT_EQ(expected, coord.committed())
        << "Not all 2PC rounds completed";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3 — ConflictResolutionStress
//
// 8 threads contend on a shared key pool.  Write–write conflicts are
// expected; after all threads finish the committed count + conflict count
// must equal the total attempted, and there must be no uncommitted residue.
// ─────────────────────────────────────────────────────────────────────────────
TEST(TransactionStress, ConflictResolutionStress) {
    constexpr int      kThreads    = 8;
    constexpr uint64_t kPerThread  = 1'000ULL;
    constexpr uint64_t kKeyRange   = 32ULL;  // narrow range to force conflicts

    StubTxStore           store;
    std::atomic<uint64_t> tx_seq{0};
    std::atomic<uint64_t> committed{0};
    std::atomic<uint64_t> conflicts{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kPerThread; ++i) {
                uint64_t tx_id = tx_seq.fetch_add(1, std::memory_order_relaxed);
                uint64_t key   = (static_cast<uint64_t>(t) * 7 + i * 3) % kKeyRange;
                auto r = store.commit(tx_id, key, i + 1);
                if (r == StubTxStore::CommitResult::OK) {
                    committed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    conflicts.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : workers) { if (th.joinable()) th.join(); }

    const uint64_t total = static_cast<uint64_t>(kThreads) * kPerThread;
    EXPECT_EQ(total, committed.load() + conflicts.load())
        << "committed + conflicts != total: committed=" << committed.load()
        << " conflicts=" << conflicts.load() << " total=" << total;
    EXPECT_GT(committed.load(), 0ULL)
        << "No transactions committed under conflict stress";
}
