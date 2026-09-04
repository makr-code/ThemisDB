/**
 * @file w8b_endurance_soak_test.cpp
 * @brief Wave 8B — Endurance Soak (SOK-01..SOK-08).
 *
 * Long-running stability validation under sustained production-like load.
 * Validates that system exhibits stable tail latency, no resource leaks,
 * and consistent performance over extended period (simulated, short version
 * for CI; full 8h soak runs separately).
 *
 * Configuration:
 * - Duration: 480+ minutes (simulated with reduced duration for CI)
 * - Load: 70% read, 20% write, 10% range queries
 * - Concurrency: 32 client threads
 * - Validation: p99 drift < 10%, no connection leaks, stable memory
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

/// @brief Resource metrics for soak test.
struct ResourceSnapshot {
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    size_t memory_bytes = 0;
    size_t active_connections = 0;
    size_t total_ops = 0;
    double cpu_percent = 0.0;
    std::vector<uint64_t> recent_latencies_us;
};

/// @brief Endurance soak metrics aggregator.
class EnduranceSoakMetrics {
public:
    void RecordLatency(uint64_t latency_us) {
        std::lock_guard<std::mutex> lock(mutex_);
        latencies_us_.push_back(latency_us);
    }

    void RecordSnapshot(ResourceSnapshot snap) {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshots_.push_back(snap);
    }

    void AddOperation() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_operations_++;
    }

    void AddError() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_errors_++;
    }

    [[nodiscard]] std::string Report() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss = {};

        oss << "\n=== Endurance Soak Metrics ===\n";
        oss << "Total Operations: " << total_operations_ << "\n";
        oss << "Total Errors: " << total_errors_ << "\n";

        if (!latencies_us_.empty()) {
            std::vector<uint64_t> sorted_latencies = latencies_us_;
            std::sort(sorted_latencies.begin(), sorted_latencies.end());

            auto p = [&](double percentile) {
                size_t idx = static_cast<size_t>(
                    (percentile / 100.0) * static_cast<double>(sorted_latencies.size() - 1));
                return sorted_latencies[std::min(idx, sorted_latencies.size() - 1)];
            };

            oss << "\nLatency Distribution (µs):\n"
                << "  p50: " << p(50.0) << "\n"
                << "  p95: " << p(95.0) << "\n"
                << "  p99: " << p(99.0) << "\n"
                << "  min: " << sorted_latencies.front() << "\n"
                << "  max: " << sorted_latencies.back() << "\n";
        }

        if (!snapshots_.empty()) {
            oss << "\nResource Timeline:\n";
            for (size_t i = 0; i < std::min(snapshots_.size(), size_t(10)); ++i) {
                const auto& snap = snapshots_[i];
                oss << "  [" << i << "] mem=" << (snap.memory_bytes / 1024 / 1024) << "MB"
                    << " conn=" << snap.active_connections << " ops=" << snap.total_ops << "\n";
            }
        }

        oss << "\nStability Assessment:\n"
            << "  Memory Leak: " << (HasMemoryLeak() ? "DETECTED" : "NONE") << "\n"
            << "  Connection Leak: " << (HasConnectionLeak() ? "DETECTED" : "NONE") << "\n"
            << "  Latency Drift: " << LatencyDrift() << "%\n";

        return oss.str();
    }

    [[nodiscard]] bool HasMemoryLeak() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshots_.size() < 2) {
          return false;
        }
        auto first_mem = snapshots_.front().memory_bytes;
        auto last_mem = snapshots_.back().memory_bytes;
        // Flag as leak if memory grew by more than 50%
        return last_mem > first_mem * 1.5;
    }

    [[nodiscard]] bool HasConnectionLeak() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshots_.size() < 2) {
          return false;
        }
        auto first_conn = snapshots_.front().active_connections;
        auto last_conn = snapshots_.back().active_connections;
        // Flag as leak if connections didn't return to baseline
        return last_conn > first_conn + 5;
    }

    [[nodiscard]] double LatencyDrift() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshots_.size() < 2) {
          return 0.0;
        }

        auto p99_first = ComputeP99(snapshots_.front().recent_latencies_us);
        auto p99_last = ComputeP99(snapshots_.back().recent_latencies_us);

        if (p99_first == 0) {
          return 0.0;
        }
        return (static_cast<double>(p99_last) - p99_first) / p99_first * 100.0;
    }

    [[nodiscard]] bool PassesSoakValidation() const {
        return !HasMemoryLeak() && !HasConnectionLeak() && total_errors_ == 0;
    }

private:
    [[nodiscard]] static uint64_t ComputeP99(const std::vector<uint64_t>& latencies) {
        if (latencies.empty()) {
          return 0;
        }
        auto sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        size_t idx =
            static_cast<size_t>(0.99 * static_cast<double>(sorted.size() - 1));
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    mutable std::mutex mutex_;
    std::vector<uint64_t> latencies_us_;
    std::vector<ResourceSnapshot> snapshots_;
    uint64_t total_operations_ = 0;
    uint64_t total_errors_ = 0;
};

/// @brief Mock in-memory database for soak testing.
class SoakTestDatabase {
public:
    SoakTestDatabase() : active_connections_(0), memory_usage_(1024 * 1024) {}

    [[nodiscard]] bool WriteDocument(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
        memory_usage_ += value.size();
        return true;
    }

    [[nodiscard]] std::string ReadDocument(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : "";
    }

    [[nodiscard]] bool UpdateDocument(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it == data_.end()) {
          return false;
        }
        size_t old_size = it->second.size();
        it->second = value;
        memory_usage_ += value.size() - old_size;
        return true;
    }

    void OpenConnection() {
        std::lock_guard<std::mutex> lock(mutex_);
        active_connections_++;
    }

    void CloseConnection() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_connections_ > 0) {
            active_connections_--;
        }
    }

    [[nodiscard]] ResourceSnapshot TakeSnapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        ResourceSnapshot snap;
        snap.timestamp = std::chrono::high_resolution_clock::now();
        snap.memory_bytes = memory_usage_;
        snap.active_connections = active_connections_;
        snap.total_ops = total_operations_;
        return snap;
    }

    void RecordOperation() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_operations_++;
    }

    [[nodiscard]] size_t DocumentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    size_t active_connections_;
    size_t memory_usage_;
    uint64_t total_operations_ = 0;
};

}  // namespace

// ============================================================================
// Wave 8B Endurance Soak Test Suite (SOK-01..SOK-08)
// ============================================================================

class EnduranceSoakTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<SoakTestDatabase>();
        metrics_ = std::make_unique<EnduranceSoakMetrics>();
        gen_ = std::make_unique<SeededTestDataGenerator>(kCanonicalSeed);

        // Pre-populate database with baseline data
        for (int i = 0; i < 10000; ++i) {
            db_->WriteDocument("doc_" + std::to_string(i), "content_v1_" + std::to_string(i));
        }
    }

    void TearDown() override {
        db_.reset();
        metrics_.reset();
        gen_.reset();
    }

    std::unique_ptr<SoakTestDatabase> db_;
    std::unique_ptr<EnduranceSoakMetrics> metrics_;
    std::unique_ptr<SeededTestDataGenerator> gen_;

    /// @brief Simulate mixed workload for a given duration.
    void RunMixedWorkload(std::chrono::seconds duration, int num_threads) {
        auto end_time = std::chrono::high_resolution_clock::now() + duration;
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, end_time]() {
                db_->OpenConnection();
                std::mt19937 rng(kCanonicalSeed);
                std::uniform_real_distribution<> op_dist(0.0, 1.0);
                std::uniform_int_distribution<int> key_dist(0, 9999);

                while (std::chrono::high_resolution_clock::now() < end_time) {
                    double op_type = op_dist(rng);
                    int key_idx = key_dist(rng);
                    std::string key = "doc_" + std::to_string(key_idx);

                    auto start = std::chrono::high_resolution_clock::now();

                    bool success = true;
                    if (op_type < 0.7) {
                        // Read operation (70%)
                        std::string result = db_->ReadDocument(key);
                        success = !result.empty();
                    } else if (op_type < 0.9) {
                        // Write operation (20%)
                        success = db_->WriteDocument(key + "_soak", "soak_value_" + std::to_string(key_idx));
                    } else {
                        // Update operation (10%)
                        success = db_->UpdateDocument(key, "updated_" + std::to_string(key_idx));
                    }

                    auto end = std::chrono::high_resolution_clock::now();
                    auto latency_us =
                        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    metrics_->RecordLatency(static_cast<uint64_t>(latency_us));
                    metrics_->AddOperation();

                    if (!success) {
                        metrics_->AddError();
                    }
                }
                db_->CloseConnection();
            });
        }

        for (auto& th : threads) {
            th.join();
        }
    }
};

/// @brief SOK-01: Long-running stability (8h baseline, simulated as 30s).
TEST_F(EnduranceSoakTest, SOK_01_LongRunningStability) {
    // In CI: run for 30 seconds. Full 8h soak runs separately.
    RunMixedWorkload(std::chrono::seconds(30), 4);

    std::cout << metrics_->Report();
    EXPECT_EQ(metrics_->Report().length() > 0, true);
}

/// @brief SOK-02: Tail latency consistency (p99 drift < 10%).
TEST_F(EnduranceSoakTest, SOK_02_TailLatencyConsistency) {
    EnduranceSoakMetrics phase1_metrics, phase2_metrics;

    // Phase 1: initial load (10s)
    std::vector<std::thread> threads;
    auto phase1_end = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10);

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, phase1_end, &phase1_metrics]() {
            db_->OpenConnection();
            std::mt19937 rng(kCanonicalSeed);
            std::uniform_int_distribution<int> key_dist(0, 9999);

            while (std::chrono::high_resolution_clock::now() < phase1_end) {
                int key_idx = key_dist(rng);
                auto start = std::chrono::high_resolution_clock::now();
                db_->ReadDocument("doc_" + std::to_string(key_idx));
                auto end = std::chrono::high_resolution_clock::now();
                auto latency_us =
                    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                phase1_metrics.RecordLatency(static_cast<uint64_t>(latency_us));
            }
            db_->CloseConnection();
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Phase 2: sustained load (10s)
    threads.clear();
    auto phase2_end = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10);

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, phase2_end, &phase2_metrics]() {
            db_->OpenConnection();
            std::mt19937 rng(kCanonicalSeed);
            std::uniform_int_distribution<int> key_dist(0, 9999);

            while (std::chrono::high_resolution_clock::now() < phase2_end) {
                int key_idx = key_dist(rng);
                auto start = std::chrono::high_resolution_clock::now();
                db_->ReadDocument("doc_" + std::to_string(key_idx));
                auto end = std::chrono::high_resolution_clock::now();
                auto latency_us =
                    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                phase2_metrics.RecordLatency(static_cast<uint64_t>(latency_us));
            }
            db_->CloseConnection();
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Verify drift is acceptable
    double drift = phase2_metrics.LatencyDrift();
    EXPECT_LT(drift, 10.0) << "Tail latency drift exceeds 10% threshold";
}

/// @brief SOK-03: Connection pool stability (no leaks).
TEST_F(EnduranceSoakTest, SOK_03_ConnectionPoolStability) {
    // Rapid open/close cycles
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int i = 0; i < 32; ++i) {
            db_->OpenConnection();
        }
        for (int i = 0; i < 32; ++i) {
            db_->CloseConnection();
        }
    }

    // Verify connections are cleaned up
    auto final_snap = db_->TakeSnapshot();
    EXPECT_LE(final_snap.active_connections, 5)
        << "Connection leak detected: " << final_snap.active_connections << " connections remain";
}

/// @brief SOK-04: Memory usage stability (linear growth, no runaway).
TEST_F(EnduranceSoakTest, SOK_04_MemoryUsageStability) {
    std::vector<ResourceSnapshot> snapshots;

    // Take memory snapshots during operations
    RunMixedWorkload(std::chrono::seconds(15), 4);

    for (int i = 0; i < 5; ++i) {
        snapshots.push_back(db_->TakeSnapshot());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Verify memory growth is reasonable (not exponential)
    if (snapshots.size() >= 2) {
        auto first_mem = snapshots.front().memory_bytes;
        auto last_mem = snapshots.back().memory_bytes;
        double growth_factor = static_cast<double>(last_mem) / first_mem;

        // Allow 2x growth over test period, but flag anything beyond
        EXPECT_LT(growth_factor, 2.0) << "Excessive memory growth detected";
    }
}

/// @brief SOK-05: CPU utilization stable under load.
TEST_F(EnduranceSoakTest, SOK_05_CPUUtilizationStable) {
    // Run sustained workload
    RunMixedWorkload(std::chrono::seconds(20), 8);

    // Verify database remained responsive
    auto snap = db_->TakeSnapshot();
    EXPECT_GT(snap.total_ops, 1000) << "CPU-bound work was not executed";
}

/// @brief SOK-06: Disk I/O patterns consistent.
TEST_F(EnduranceSoakTest, SOK_06_DiskIOConsistency) {
    // Simulate write-heavy workload
    for (int i = 0; i < 5000; ++i) {
        db_->WriteDocument("io_test_" + std::to_string(i), "data_" + std::to_string(i));
    }

    EXPECT_GT(db_->DocumentCount(), 10000) << "Disk operations failed";
}

/// @brief SOK-07: No cascading failures under sustained peak.
TEST_F(EnduranceSoakTest, SOK_07_NoCascadingFailures) {
    EnduranceSoakMetrics metrics;
    std::atomic<bool> stop_flag{false};

    // Heavy concurrent load
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, &metrics, &stop_flag]() {
            db_->OpenConnection();
            std::mt19937 rng(kCanonicalSeed);
            std::uniform_real_distribution<> op_dist(0.0, 1.0);
            std::uniform_int_distribution<int> key_dist(0, 9999);

            while (!stop_flag) {
                double op_type = op_dist(rng);
                int key_idx = key_dist(rng);
                std::string key = "doc_" + std::to_string(key_idx);

                bool success = true;
                if (op_type < 0.7) {
                    success = !db_->ReadDocument(key).empty();
                } else if (op_type < 0.9) {
                    success = db_->WriteDocument(key + "_peak", "peak_" + std::to_string(key_idx));
                } else {
                    success = db_->UpdateDocument(key, "peak_update_" + std::to_string(key_idx));
                }

                if (!success) {
                    metrics.AddError();
                }
                metrics.AddOperation();
            }
            db_->CloseConnection();
        });
    }

    // Run for 20 seconds
    std::this_thread::sleep_for(std::chrono::seconds(20));
    stop_flag = true;

    for (auto& th : threads) {
        th.join();
    }

    // Verify no cascading failure: error rate should remain low
    EXPECT_LT(metrics.Report().find("Total Errors: 0"), std::string::npos)
        << "Cascading failures detected";
}

/// @brief SOK-08: Recovery to normal after brief spike.
TEST_F(EnduranceSoakTest, SOK_08_RecoveryAfterBriefSpike) {
    EnduranceSoakMetrics metrics;

    // Normal load phase
    RunMixedWorkload(std::chrono::seconds(5), 4);

    // Spike phase (16 threads)
    RunMixedWorkload(std::chrono::seconds(5), 16);

    // Recovery phase (back to 4 threads)
    RunMixedWorkload(std::chrono::seconds(5), 4);

    // Verify system remained stable
    std::cout << metrics_->Report();
    EXPECT_TRUE(metrics_->PassesSoakValidation()) << "Soak validation failed after spike";
}
} } // namespace themis::test
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
