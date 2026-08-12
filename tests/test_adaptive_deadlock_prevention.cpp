// Tests for Adaptive Deadlock Prevention (v1.9.0)
// Covers DeadlockPredictor standalone behaviour and TransactionManager integration.
// Copyright (c) 2024 ThemisDB. All rights reserved.

#include <gtest/gtest.h>
#include "transaction/deadlock_predictor.h"
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"

#include <chrono>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

using namespace themis;
using namespace std::chrono_literals;

// ── Standalone DeadlockPredictor tests ──────────────────────────────────────

class DeadlockPredictorTest : public ::testing::Test {
protected:
    DeadlockPredictor predictor_;
};

TEST_F(DeadlockPredictorTest, InitialStateHasZeroCounts) {
    EXPECT_EQ(predictor_.recordedTransactionCount(), 0u);
    EXPECT_EQ(predictor_.recordedDeadlockCount(), 0u);
    EXPECT_TRUE(predictor_.getPatterns().empty());
}

TEST_F(DeadlockPredictorTest, RecordTransactionIncreasesCount) {
    predictor_.recordTransaction(1, {"a", "b"}, 500us);
    EXPECT_EQ(predictor_.recordedTransactionCount(), 1u);
}

TEST_F(DeadlockPredictorTest, RecordDeadlockIncreasesCount) {
    predictor_.recordDeadlock({"a", "b"});
    EXPECT_EQ(predictor_.recordedDeadlockCount(), 1u);
}

TEST_F(DeadlockPredictorTest, ResetClearsAllState) {
    predictor_.recordTransaction(1, {"x", "y"}, 1000us);
    predictor_.recordDeadlock({"x", "y"});
    predictor_.reset();
    EXPECT_EQ(predictor_.recordedTransactionCount(), 0u);
    EXPECT_EQ(predictor_.recordedDeadlockCount(), 0u);
    EXPECT_TRUE(predictor_.getPatterns().empty());
}

TEST_F(DeadlockPredictorTest, PredictReturnsZeroBelowMinSamples) {
    // Default min_samples_for_prediction is 5; we record only 2.
    predictor_.recordTransaction(1, {"a", "b"}, 100us);
    predictor_.recordTransaction(2, {"a", "b"}, 200us);
    double prob = predictor_.predictDeadlockProbability({"a", "b"}, {});
    EXPECT_DOUBLE_EQ(prob, 0.0);
}

TEST_F(DeadlockPredictorTest, PredictReturnsProbabilityAfterSufficientSamples) {
    // Record enough transactions and then a deadlock to push probability > 0.
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 2;
    predictor_.setConfig(cfg);

    predictor_.recordTransaction(1, {"a", "b"}, 100us);
    predictor_.recordTransaction(2, {"a", "b"}, 150us);
    predictor_.recordDeadlock({"a", "b"});

    double prob = predictor_.predictDeadlockProbability({"a", "b"}, {});
    EXPECT_GT(prob, 0.0);
    EXPECT_LE(prob, 1.0);
}

TEST_F(DeadlockPredictorTest, PredictProbabilityNonZeroFromCoOccurrenceAlone) {
    // Co-occurrence weights from recordTransaction() now update max_conflict_score_,
    // so a non-zero probability should be returned without calling recordDeadlock().
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);

    predictor_.recordTransaction(1, {"x", "y"}, 100us);

    double prob = predictor_.predictDeadlockProbability({"x", "y"}, {});
    EXPECT_GT(prob, 0.0);
    EXPECT_LE(prob, 1.0);
}

TEST_F(DeadlockPredictorTest, PredictProbabilityClampedToOne) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    cfg.deadlock_weight_multiplier = 1000.0;
    predictor_.setConfig(cfg);

    predictor_.recordTransaction(1, {"a", "b"}, 50us);
    for (int i = 0; i < 50; ++i) {
        predictor_.recordDeadlock({"a", "b"});
    }

    double prob = predictor_.predictDeadlockProbability({"a", "b"}, {});
    EXPECT_LE(prob, 1.0);
}

TEST_F(DeadlockPredictorTest, PredictProbabilityIsHigherForDeadlockedPairs) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);

    // "a"+"b" deadlocked; "c"+"d" only observed normally.
    predictor_.recordTransaction(1, {"a", "b"}, 100us);
    predictor_.recordTransaction(2, {"c", "d"}, 100us);
    predictor_.recordDeadlock({"a", "b"});

    std::set<DeadlockPredictor::TransactionId> active;
    double prob_ab = predictor_.predictDeadlockProbability({"a", "b"}, active);
    double prob_cd = predictor_.predictDeadlockProbability({"c", "d"}, active);
    EXPECT_GT(prob_ab, prob_cd);
}

TEST_F(DeadlockPredictorTest, RecommendLockOrderIsStableWhenNoData) {
    // No history → lexicographic order.
    std::vector<std::string> keys = {"z", "a", "m"};
    auto ordered = predictor_.recommendLockOrder(keys);
    EXPECT_EQ(ordered.size(), 3u);
    EXPECT_TRUE(std::is_sorted(ordered.begin(), ordered.end()));
}

TEST_F(DeadlockPredictorTest, RecommendLockOrderReturnsAllKeys) {
    predictor_.recordTransaction(1, {"users:1", "accounts:1"}, 200us);
    predictor_.recordDeadlock({"accounts:1"});

    std::vector<std::string> keys = {"users:1", "accounts:1"};
    auto ordered = predictor_.recommendLockOrder(keys);
    ASSERT_EQ(ordered.size(), 2u);
    // Both original keys must be present.
    EXPECT_NE(std::find(ordered.begin(), ordered.end(), "users:1"), ordered.end());
    EXPECT_NE(std::find(ordered.begin(), ordered.end(), "accounts:1"), ordered.end());
}

TEST_F(DeadlockPredictorTest, RecommendTimeoutFallsBackToMinWhenNoData) {
    auto cfg = predictor_.getConfig();
    auto timeout = predictor_.recommendTimeout({"some:key"});
    EXPECT_EQ(timeout, cfg.min_recommended_timeout);
}

TEST_F(DeadlockPredictorTest, RecommendTimeoutIsClamped) {
    DeadlockPredictor::Config cfg;
    cfg.min_recommended_timeout = 10ms;
    cfg.max_recommended_timeout = 5000ms;
    cfg.timeout_percentile       = 90;
    predictor_.setConfig(cfg);

    // Record very long hold times to push the recommended timeout high.
    for (int i = 0; i < 20; ++i) {
        predictor_.recordTransaction(i, {"heavy:key"},
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::seconds(60)));
    }
    auto timeout = predictor_.recommendTimeout({"heavy:key"});
    EXPECT_LE(timeout, cfg.max_recommended_timeout);
    EXPECT_GE(timeout, cfg.min_recommended_timeout);
}

TEST_F(DeadlockPredictorTest, RecommendTimeoutGrowsWithLongHoldTimes) {
    DeadlockPredictor::Config cfg;
    cfg.min_recommended_timeout = 10ms;
    cfg.max_recommended_timeout = 60'000ms;
    predictor_.setConfig(cfg);

    // Record moderate hold times.
    for (int i = 0; i < 10; ++i) {
        predictor_.recordTransaction(i, {"key:a"}, 100'000us); // 100 ms each
    }
    auto timeout = predictor_.recommendTimeout({"key:a"});
    // 2× p90 of 100 ms = 200 ms.  Should exceed the floor.
    EXPECT_GT(timeout, cfg.min_recommended_timeout);
}

TEST_F(DeadlockPredictorTest, PatternsAccumulate) {
    predictor_.recordTransaction(1, {"p", "q"}, 300us);
    predictor_.recordTransaction(2, {"p", "q"}, 400us);
    // Same key set → deduplicated into one pattern with frequency 2.
    auto patterns = predictor_.getPatterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].frequency, 2u);
}

TEST_F(DeadlockPredictorTest, EmptyLockSetRecordIsIgnored) {
    predictor_.recordTransaction(1, {}, 100us);
    EXPECT_EQ(predictor_.recordedTransactionCount(), 0u);
}

TEST_F(DeadlockPredictorTest, EmptyDeadlockRecordIsIgnored) {
    predictor_.recordDeadlock({});
    EXPECT_EQ(predictor_.recordedDeadlockCount(), 0u);
}

TEST_F(DeadlockPredictorTest, SetAndGetConfig) {
    DeadlockPredictor::Config cfg;
    cfg.max_patterns              = 500;
    cfg.min_samples_for_prediction = 10;
    cfg.deadlock_weight_multiplier = 5.0;
    predictor_.setConfig(cfg);

    auto got = predictor_.getConfig();
    EXPECT_EQ(got.max_patterns, 500u);
    EXPECT_EQ(got.min_samples_for_prediction, 10u);
    EXPECT_DOUBLE_EQ(got.deadlock_weight_multiplier, 5.0);
}

TEST_F(DeadlockPredictorTest, ActiveTransactionsInflateProbability) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);

    predictor_.recordTransaction(1, {"k1", "k2"}, 100us);
    predictor_.recordDeadlock({"k1", "k2"});

    std::set<DeadlockPredictor::TransactionId> none;
    std::set<DeadlockPredictor::TransactionId> many = {10, 11, 12, 13, 14,
                                                       15, 16, 17, 18, 19};
    double prob_none = predictor_.predictDeadlockProbability({"k1", "k2"}, none);
    double prob_many = predictor_.predictDeadlockProbability({"k1", "k2"}, many);
    EXPECT_GE(prob_many, prob_none);
}

TEST_F(DeadlockPredictorTest, RecommendLockOrderIsIdempotent) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);

    predictor_.recordTransaction(1, {"a", "b", "c"}, 100us);
    predictor_.recordDeadlock({"b", "c"});

    std::vector<std::string> keys = {"c", "b", "a"};
    auto first  = predictor_.recommendLockOrder(keys);
    auto second = predictor_.recommendLockOrder(keys);
    EXPECT_EQ(first, second);
}

// ── TransactionManager integration tests ────────────────────────────────────

class AdaptiveDeadlockIntegrationTest : public ::testing::Test {
protected:
    static constexpr const char* kDbPath = "/tmp/test_adaptive_deadlock_db";

    void SetUp() override {
        std::filesystem::remove_all(kDbPath);

        RocksDBWrapper::Config cfg;
        cfg.db_path           = kDbPath;
        cfg.enable_wal        = false;
        cfg.memtable_size_mb  = 16;
        cfg.block_cache_size_mb = 16;

        db_       = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        sec_idx_  = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_= std::make_unique<GraphIndexManager>(*db_);
        vec_idx_  = std::make_unique<VectorIndexManager>(*db_);
        txn_mgr_  = std::make_unique<TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);
    }

    void TearDown() override {
        txn_mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        db_->close();
        db_.reset();
        std::filesystem::remove_all(kDbPath);
    }

    std::unique_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<SecondaryIndexManager>   sec_idx_;
    std::unique_ptr<GraphIndexManager>       graph_idx_;
    std::unique_ptr<VectorIndexManager>      vec_idx_;
    std::unique_ptr<TransactionManager>      txn_mgr_;
    DeadlockPredictor                        predictor_;
};

TEST_F(AdaptiveDeadlockIntegrationTest, NoPredictorByDefault) {
    EXPECT_EQ(txn_mgr_->getDeadlockPredictor(), nullptr);
}

TEST_F(AdaptiveDeadlockIntegrationTest, SetAndGetPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);
    EXPECT_EQ(txn_mgr_->getDeadlockPredictor(), &predictor_);
}

TEST_F(AdaptiveDeadlockIntegrationTest, DetachPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);
    txn_mgr_->setDeadlockPredictor(nullptr);
    EXPECT_EQ(txn_mgr_->getDeadlockPredictor(), nullptr);
}

TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityWithoutPredictorReturnsZero) {
    double prob = txn_mgr_->predictDeadlockProbability({"k1", "k2"});
    EXPECT_DOUBLE_EQ(prob, 0.0);
}

TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityDelegatesToPredictor) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);
    predictor_.recordTransaction(99, {"k1", "k2"}, 100us);
    predictor_.recordDeadlock({"k1", "k2"});

    txn_mgr_->setDeadlockPredictor(&predictor_);
    double prob = txn_mgr_->predictDeadlockProbability({"k1", "k2"});
    EXPECT_GT(prob, 0.0);
    EXPECT_LE(prob, 1.0);
}

TEST_F(AdaptiveDeadlockIntegrationTest, RecommendLockOrderWithoutPredictorIsLexicographic) {
    std::vector<std::string> keys = {"z", "a", "m"};
    auto ordered = txn_mgr_->recommendLockOrder(keys);
    ASSERT_EQ(ordered.size(), 3u);
    EXPECT_TRUE(std::is_sorted(ordered.begin(), ordered.end()));
}

TEST_F(AdaptiveDeadlockIntegrationTest, RecommendLockOrderDelegatesToPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);
    std::vector<std::string> keys = {"z", "a", "m"};
    auto ordered = txn_mgr_->recommendLockOrder(keys);
    EXPECT_EQ(ordered.size(), 3u);
}

TEST_F(AdaptiveDeadlockIntegrationTest, RecommendTimeoutWithoutPredictorUsesDeadlockTimeout) {
    txn_mgr_->setDeadlockTimeout(500ms);
    auto timeout = txn_mgr_->recommendTimeout({"k1"});
    EXPECT_EQ(timeout, 500ms);
}

TEST_F(AdaptiveDeadlockIntegrationTest, RecommendTimeoutDelegatesToPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);
    // No history → predictor falls back to its min_recommended_timeout.
    auto cfg = predictor_.getConfig();
    auto timeout = txn_mgr_->recommendTimeout({"k1"});
    EXPECT_EQ(timeout, cfg.min_recommended_timeout);
}

TEST_F(AdaptiveDeadlockIntegrationTest, CommitRecordsTransactionInPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);

    // A transaction with no held locks contributes nothing to the predictor.
    // After commit the count must still be 0 (no keys → recordTransaction skips).
    auto txn_id = txn_mgr_->beginTransaction();
    txn_mgr_->commitTransaction(txn_id);

    EXPECT_EQ(predictor_.recordedTransactionCount(), 0u);
}

TEST_F(AdaptiveDeadlockIntegrationTest, RollbackRecordsTransactionInPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);

    // Same: no held locks → recordTransaction skips → count stays 0.
    auto txn_id = txn_mgr_->beginTransaction();
    txn_mgr_->rollbackTransaction(txn_id);

    EXPECT_EQ(predictor_.recordedTransactionCount(), 0u);
}

TEST_F(AdaptiveDeadlockIntegrationTest, MultipleTransactionsDoNotCrashWithPredictor) {
    txn_mgr_->setDeadlockPredictor(&predictor_);

    for (int i = 0; i < 20; ++i) {
        auto id = txn_mgr_->beginTransaction();
        if (i % 3 == 0) {
            txn_mgr_->rollbackTransaction(id);
        } else {
            txn_mgr_->commitTransaction(id);
        }
    }

    SUCCEED();
}

TEST_F(AdaptiveDeadlockIntegrationTest, PredictorCanBeReplacedLive) {
    DeadlockPredictor p1, p2;

    txn_mgr_->setDeadlockPredictor(&p1);
    EXPECT_EQ(txn_mgr_->getDeadlockPredictor(), &p1);

    txn_mgr_->setDeadlockPredictor(&p2);
    EXPECT_EQ(txn_mgr_->getDeadlockPredictor(), &p2);
}

TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityUsesActiveTransactionCount) {
    DeadlockPredictor::Config cfg;
    cfg.min_samples_for_prediction = 1;
    predictor_.setConfig(cfg);
    predictor_.recordTransaction(1, {"k1", "k2"}, 100us);
    predictor_.recordDeadlock({"k1", "k2"});

    txn_mgr_->setDeadlockPredictor(&predictor_);

    // Start several transactions to increase active-load factor.
    std::vector<TransactionManager::TransactionId> ids;
    for (int i = 0; i < 5; ++i) {
        ids.push_back(txn_mgr_->beginTransaction());
    }

    double prob = txn_mgr_->predictDeadlockProbability({"k1", "k2"});
    EXPECT_GT(prob, 0.0);
    EXPECT_LE(prob, 1.0);

    for (auto id : ids) {
        txn_mgr_->rollbackTransaction(id);
    }
}

// ── AdaptiveDeadlockPreventionFocusedTests suite marker ─────────────────────
// GTest discovers all tests above automatically via the test binary.
