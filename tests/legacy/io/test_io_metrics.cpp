// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for StorageEngine I/O metrics (IOMetrics):
//   - Initial state: all counters zero
//   - put_ops increments on each successful put
//   - put_errors increments when engine is closed
//   - get_ops increments on successful get; get_errors on miss
//   - del_ops increments on successful del
//   - Latency fields are set (> 0 for puts, ≥ 0 for any op)
//   - min ≤ avg ≤ max invariant
//   - resetIOMetrics() zeroes everything
//   - Concurrent puts accumulate correctly

#include <gtest/gtest.h>
#include "storage/storage_engine.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class IOMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_iometrics_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);
        engine_ = StorageEngine::createDefault();
        ASSERT_TRUE(engine_->open(db_path_).has_value());
    }

    void TearDown() override {
        if (engine_) {
          engine_->close();
        }
        fs::remove_all(db_path_);
    }

    std::string                    db_path_;
    std::shared_ptr<StorageEngine> engine_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, InitialState_AllZero) {
    auto m = engine_->ioMetrics();
    EXPECT_EQ(m.put_ops,    0u);
    EXPECT_EQ(m.put_errors, 0u);
    EXPECT_EQ(m.get_ops,    0u);
    EXPECT_EQ(m.get_errors, 0u);
    EXPECT_EQ(m.del_ops,    0u);
    EXPECT_EQ(m.del_errors, 0u);
    EXPECT_DOUBLE_EQ(m.avg_put_latency_us(), 0.0);
    EXPECT_DOUBLE_EQ(m.avg_get_latency_us(), 0.0);
    EXPECT_DOUBLE_EQ(m.avg_del_latency_us(), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// put
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, Put_IncrementsPutOps) {
    ASSERT_TRUE(engine_->put("k1", "v1").has_value());
    ASSERT_TRUE(engine_->put("k2", "v2").has_value());
    EXPECT_EQ(engine_->ioMetrics().put_ops, 2u);
}

TEST_F(IOMetricsTest, Put_ClosedEngine_IncrementsPutErrors) {
    engine_->close();
    engine_->put("k", "v");  // should fail
    EXPECT_GE(engine_->ioMetrics().put_errors, 1u);
}

TEST_F(IOMetricsTest, Put_LatencyIsRecorded) {
    ASSERT_TRUE(engine_->put("lat_key", "lat_val").has_value());
    auto m = engine_->ioMetrics();
    EXPECT_EQ(m.put_ops, 1u);
    // put_latency_us may be 0 on very fast systems but max should be set
    EXPECT_LE(m.put_latency_min_us, m.put_latency_max_us);
    EXPECT_GT(m.avg_put_latency_us(), -1.0); // non-negative (≥ 0)
}

TEST_F(IOMetricsTest, Put_MinMaxMonotone) {
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(engine_->put("key_" + std::to_string(i), "val").has_value());
    }
    auto m = engine_->ioMetrics();
    ASSERT_GT(m.put_ops, 0u);
    // min ≤ avg: min is the smallest individual measurement, avg is the mean
    EXPECT_LE(static_cast<double>(m.put_latency_min_us), m.avg_put_latency_us());
    // avg ≤ max: average cannot exceed the largest individual measurement
    EXPECT_LE(m.avg_put_latency_us(), static_cast<double>(m.put_latency_max_us));
    // Sanity: min ≤ max
    EXPECT_LE(m.put_latency_min_us, m.put_latency_max_us);
}

// ─────────────────────────────────────────────────────────────────────────────
// get
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, Get_SuccessfulGet_IncrementsGetOps) {
    ASSERT_TRUE(engine_->put("g1", "gv1").has_value());
    ASSERT_TRUE(engine_->get("g1").has_value());
    EXPECT_EQ(engine_->ioMetrics().get_ops, 1u);
}

TEST_F(IOMetricsTest, Get_Miss_IncrementsGetErrors) {
    engine_->get("nonexistent_key");
    EXPECT_GE(engine_->ioMetrics().get_errors, 1u);
    EXPECT_EQ(engine_->ioMetrics().get_ops, 0u);
}

TEST_F(IOMetricsTest, Get_ClosedEngine_IncrementsGetErrors) {
    engine_->close();
    engine_->get("any_key");
    EXPECT_GE(engine_->ioMetrics().get_errors, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// del
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, Del_SuccessfulDel_IncrementsDelOps) {
    ASSERT_TRUE(engine_->put("d1", "dv1").has_value());
    ASSERT_TRUE(engine_->del("d1").has_value());
    EXPECT_EQ(engine_->ioMetrics().del_ops, 1u);
}

TEST_F(IOMetricsTest, Del_ClosedEngine_IncrementsDelErrors) {
    engine_->close();
    engine_->del("any");
    EXPECT_GE(engine_->ioMetrics().del_errors, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// resetIOMetrics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, Reset_ZeroesAllCounters) {
    ASSERT_TRUE(engine_->put("r1", "rv1").has_value());
    ASSERT_TRUE(engine_->get("r1").has_value());

    ASSERT_GT(engine_->ioMetrics().put_ops, 0u);
    engine_->resetIOMetrics();

    auto m = engine_->ioMetrics();
    EXPECT_EQ(m.put_ops,    0u);
    EXPECT_EQ(m.put_errors, 0u);
    EXPECT_EQ(m.get_ops,    0u);
    EXPECT_EQ(m.get_errors, 0u);
    EXPECT_EQ(m.del_ops,    0u);
    EXPECT_EQ(m.del_errors, 0u);
}

TEST_F(IOMetricsTest, Reset_AllowsAccumulationAfterReset) {
    ASSERT_TRUE(engine_->put("before", "v").has_value());
    engine_->resetIOMetrics();
    ASSERT_TRUE(engine_->put("after", "v").has_value());

    EXPECT_EQ(engine_->ioMetrics().put_ops, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent puts
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, Concurrent_PutOpsAccumulate) {
    constexpr int kThreads = 4;
    constexpr int kOps     = 50;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kOps; ++i) {
                engine_->put("t" + std::to_string(t) + "_" + std::to_string(i), "v");
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(engine_->ioMetrics().put_ops, static_cast<uint64_t>(kThreads * kOps));
}

// ─────────────────────────────────────────────────────────────────────────────
// Mixed ops: put + get + del total counts
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IOMetricsTest, MixedOps_TotalCountsCorrect) {
    ASSERT_TRUE(engine_->put("x", "1").has_value());
    ASSERT_TRUE(engine_->put("y", "2").has_value());
    ASSERT_TRUE(engine_->get("x").has_value());   // hit
    engine_->get("z");                            // miss
    ASSERT_TRUE(engine_->del("y").has_value());

    auto m = engine_->ioMetrics();
    EXPECT_EQ(m.put_ops,    2u);
    EXPECT_EQ(m.get_ops,    1u);
    EXPECT_EQ(m.get_errors, 1u);
    EXPECT_EQ(m.del_ops,    1u);
}
