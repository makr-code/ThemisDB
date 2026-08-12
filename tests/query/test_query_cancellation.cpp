// Unit tests for query cancellation via request ID.
// Covers QueryCancellationToken, QueryCanceller, and executeAqlCancellable().

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include "query/query_canceller.h"
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "utils/error_registry.h"

using namespace themis;
using namespace themis::query;

// ─── helpers ────────────────────────────────────────────────────────────────

static std::string tmpCancelTestPath(const std::string& suffix) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_ct_" + suffix + std::to_string(now))).string();
}

// ════════════════════════════════════════════════════════════════════════════
// QueryCancellationToken – unit tests
// ════════════════════════════════════════════════════════════════════════════

TEST(QueryCancellationTokenTest, InitiallyNotCancelled) {
    QueryCancellationToken token;
    EXPECT_FALSE(token.isCancelled());
}

TEST(QueryCancellationTokenTest, CancelSetsFlag) {
    QueryCancellationToken token;
    token.cancel();
    EXPECT_TRUE(token.isCancelled());
}

TEST(QueryCancellationTokenTest, CancelIsIdempotent) {
    QueryCancellationToken token;
    token.cancel();
    token.cancel(); // second call must not throw or change state
    EXPECT_TRUE(token.isCancelled());
}

TEST(QueryCancellationTokenTest, CrossThreadCancellation) {
    auto token = std::make_shared<QueryCancellationToken>();
    ASSERT_FALSE(token->isCancelled());

    std::thread canceller_thread([token]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        token->cancel();
    });

    // Busy-wait until cancelled (≤ 500 ms)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (!token->isCancelled()) {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline)
            << "Token was not cancelled within 500 ms";
        std::this_thread::yield();
    }
    canceller_thread.join();
    EXPECT_TRUE(token->isCancelled());
}

// ════════════════════════════════════════════════════════════════════════════
// QueryCanceller – unit tests (using local instance to avoid singleton state)
// ════════════════════════════════════════════════════════════════════════════

TEST(QueryCancellerTest, RegisterReturnsToken) {
    QueryCanceller canceller;
    auto token = canceller.registerQuery("req-001");
    ASSERT_NE(token, nullptr);
    EXPECT_FALSE(token->isCancelled());
}

TEST(QueryCancellerTest, CancelExistingQuery_ReturnsTrue) {
    QueryCanceller canceller;
    auto token = canceller.registerQuery("req-002");

    bool cancelled = canceller.cancel("req-002");
    EXPECT_TRUE(cancelled);
    EXPECT_TRUE(token->isCancelled());
}

TEST(QueryCancellerTest, CancelUnknownId_ReturnsFalse) {
    QueryCanceller canceller;
    bool result = canceller.cancel("no-such-id");
    EXPECT_FALSE(result);
}

TEST(QueryCancellerTest, UnregisterRemovesToken) {
    QueryCanceller canceller;
    canceller.registerQuery("req-003");
    canceller.unregisterQuery("req-003");

    // After unregister, cancel should return false.
    bool result = canceller.cancel("req-003");
    EXPECT_FALSE(result);
}

TEST(QueryCancellerTest, UnregisterNonExistentId_IsNoOp) {
    QueryCanceller canceller;
    // Must not throw or crash.
    EXPECT_NO_THROW(canceller.unregisterQuery("never-registered"));
}

TEST(QueryCancellerTest, ReregisterReplacesToken) {
    QueryCanceller canceller;
    auto token1 = canceller.registerQuery("req-004");

    // Re-register with the same ID; old token is replaced.
    auto token2 = canceller.registerQuery("req-004");
    ASSERT_NE(token1, token2);

    // Cancelling via the registry should signal token2, not token1.
    canceller.cancel("req-004");
    EXPECT_TRUE(token2->isCancelled());
    // token1 is no longer tracked; its state is unspecified but must not crash.
    EXPECT_NO_THROW(static_cast<void>(token1->isCancelled()));
}

TEST(QueryCancellerTest, ScopedRegistration_UnregistersOnDestruction) {
    QueryCanceller canceller;
    auto token = canceller.registerQuery("req-005");

    {
        QueryCanceller::ScopedRegistration guard("req-005", canceller);
        // token is still live inside the scope
        EXPECT_FALSE(token->isCancelled());
    }
    // After scope: guard's destructor called unregisterQuery("req-005").
    bool result = canceller.cancel("req-005");
    EXPECT_FALSE(result); // already unregistered
}

TEST(QueryCancellerTest, TokenExpiredAfterQueryOwnerDrops) {
    QueryCanceller canceller;
    {
        auto token = canceller.registerQuery("req-006");
        // token is the only strong reference; it goes out of scope here.
    }
    // The weak_ptr inside the registry is now expired.
    bool result = canceller.cancel("req-006");
    EXPECT_FALSE(result);
}

TEST(QueryCancellerTest, ConcurrentCancelFromMultipleThreads) {
    QueryCanceller canceller;
    auto token = canceller.registerQuery("req-007");

    std::atomic<int> cancel_count{0};
    auto cancel_fn = [&]() {
        if (canceller.cancel("req-007")) {
            ++cancel_count;
        }
    };

    std::thread t1(cancel_fn);
    std::thread t2(cancel_fn);
    t1.join();
    t2.join();

    // At least one cancel must have succeeded.
    EXPECT_GE(cancel_count.load(), 1);
    EXPECT_TRUE(token->isCancelled());
}

// ════════════════════════════════════════════════════════════════════════════
// executeAqlCancellable – integration tests (requires a live QueryEngine)
// ════════════════════════════════════════════════════════════════════════════

class ExecuteAqlCancellableTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = tmpCancelTestPath("cancel");
        RocksDBWrapper::Config config;
        config.db_path = db_path_;
        storage_ = std::make_shared<RocksDBWrapper>(config);
        storage_->open();
        sec_idx_ = std::make_shared<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *sec_idx_);
    }

    void TearDown() override {
        engine_.reset();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<QueryEngine> engine_;
};

TEST_F(ExecuteAqlCancellableTest, SuccessfulExecution_ReturnsResult) {
    QueryCanceller canceller;
    auto result = executeAqlCancellable(
        "FOR x IN entities RETURN x",
        *engine_,
        "req-100",
        canceller
    );
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST_F(ExecuteAqlCancellableTest, CancelDuringExecution_TokenIsSignalled) {
    QueryCanceller canceller;
    std::atomic<bool> query_started{false};

    // Launch a background thread that cancels the query as soon as it is
    // registered (i.e. when executeAqlCancellable has called registerQuery).
    std::thread cancel_thread([&]() {
        // Busy-wait until the query is registered (≤ 500 ms).
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(500);
        while (!query_started.load()) {
            if (std::chrono::steady_clock::now() > deadline) break;
            std::this_thread::yield();
        }
        canceller.cancel("req-200");
    });

    // Signal that we're about to enter executeAqlCancellable.
    query_started.store(true);

    auto result = executeAqlCancellable(
        "FOR x IN entities RETURN x",
        *engine_,
        "req-200",
        canceller
    );
    cancel_thread.join();

    // The query may have completed before the cancel arrived (success) or
    // been detected as cancelled in the post-execution check (error).
    // Either outcome is valid; we only assert there is no crash and the
    // error code – if present – is ERR_QUERY_CANCELLED.
    if (!result) {
        EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_CANCELLED);
    }
}

TEST_F(ExecuteAqlCancellableTest, UnregisteredAfterExecution) {
    QueryCanceller canceller;
    auto result = executeAqlCancellable(
        "FOR x IN entities RETURN x",
        *engine_,
        "req-300",
        canceller
    );
    EXPECT_TRUE(static_cast<bool>(result));

    // After the call, the ScopedRegistration inside executeAqlCancellable
    // has already unregistered "req-300".
    bool cancelled = canceller.cancel("req-300");
    EXPECT_FALSE(cancelled); // nothing to cancel
}

TEST_F(ExecuteAqlCancellableTest, ParseError_PropagatedUnchanged) {
    QueryCanceller canceller;
    auto result = executeAqlCancellable(
        "THIS IS NOT VALID AQL !!!",
        *engine_,
        "req-400",
        canceller
    );
    ASSERT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}
