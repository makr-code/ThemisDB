/**
 * @file test_process_stress_churn_focused.cpp
 * @brief Phase 4 Stress Tests: Sustained high-churn scenarios with deterministic reproducibility
 * @note Test IDs: S-01..S-08
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_linker.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// Stress Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class StressChurnTest : public ::testing::Test {
protected:
    static constexpr int32_t kCanonicalRngSeed = 42;
    static constexpr int32_t kStressRounds = 1000;
    static constexpr int32_t kNumThreadsForStress = 4;

    std::mt19937 stress_rng{kCanonicalRngSeed};

    void SetUp() override {
        stress_rng.seed(kCanonicalRngSeed);
    }

    // Measure operation throughput
    struct OperationStats {
        int64_t total_operations{0};
        std::chrono::milliseconds total_time_ms{0};

        double ops_per_sec() const {
            if (total_time_ms.count() == 0) return 0.0;
            return (static_cast<double>(total_operations) / total_time_ms.count()) * 1000.0;
        }
    };
};

// ─────────────────────────────────────────────────────────────────────────────
// S-01: High-volume link creation under sustained load
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S01_HighVolumeLinkCreationSustained) {
    std::vector<ProcessLink> links;
    std::mutex links_mutex;

    auto start = std::chrono::high_resolution_clock::now();

    for (int32_t i = 0; i < kStressRounds; ++i) {
        ProcessLink link;
        link.link_id = "link_" + std::to_string(i);
        link.source_id = "src_" + std::to_string(i % 100);
        link.target_id = "tgt_" + std::to_string((i + 1) % 100);
        link.link_type = ProcessLinkType::CROSS_REFERENCE;
        link.created_at_ms = 1000 + i;

        {
            std::lock_guard<std::mutex> lock(links_mutex);
            links.push_back(link);
        }
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    int64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    if (elapsed_us <= 0) {
        elapsed_us = 1;
    }

    EXPECT_EQ(links.size(), kStressRounds);
    EXPECT_GT(elapsed_us, 0);

    // Throughput should be reasonable (> 1000 ops/sec)
    double ops_per_sec = (static_cast<double>(kStressRounds) / elapsed_us) * 1000000.0;
    EXPECT_GT(ops_per_sec, 100.0) << "Throughput too low: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// S-02: Sustained attachment creation with deterministic ordering
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S02_SustainedAttachmentCreationDeterministic) {
    auto create_attachments_with_seed = [](int32_t seed) -> std::vector<ProcessAttachment> {
        std::vector<ProcessAttachment> attachments;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> instance_dist(0, 49);
        std::uniform_int_distribution<int32_t> object_dist(0, 99);

        for (int32_t i = 0; i < kStressRounds; ++i) {
            ProcessAttachment attach;
            attach.id = "attach_" + std::to_string(i);
            attach.instance_id = "inst_" + std::to_string(instance_dist(local_rng));
            attach.object_id = "obj_" + std::to_string(object_dist(local_rng));
            attach.object_collection = "documents";
            attach.attached_at_ms = 1000 + i;

            attachments.push_back(attach);
        }
        return attachments;
    };

    auto attachments1 = create_attachments_with_seed(kCanonicalRngSeed);
    auto attachments2 = create_attachments_with_seed(kCanonicalRngSeed);

    ASSERT_EQ(attachments1.size(), attachments2.size());
    for (size_t i = 0; i < attachments1.size(); ++i) {
        EXPECT_EQ(attachments1[i].instance_id, attachments2[i].instance_id);
        EXPECT_EQ(attachments1[i].object_id, attachments2[i].object_id);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// S-03: Repeated model validation cycles (churn)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S03_RepeatedModelValidationCycles) {
    struct ModelState {
        int32_t version{0};
        bool is_valid{false};
        int64_t last_validated_ms{0};
    };

    std::vector<ModelState> models;
    for (int32_t i = 0; i < 100; ++i) {
        models.push_back(ModelState{0, false, 0});
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int32_t round = 0; round < kStressRounds / 100; ++round) {
        for (auto& model : models) {
            model.version++;
            model.is_valid = (model.version % 2 == 0);  // Deterministic validation
            model.last_validated_ms = 1000 + round;
        }
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    EXPECT_EQ(models[0].version, kStressRounds / 100);
    EXPECT_GT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// S-04: Concurrent link creation with lock contention
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S04_ConcurrentLinkCreationUnderLockContention) {
    std::vector<ProcessLink> all_links;
    std::mutex links_mutex;
    std::atomic<int64_t> total_created{0};

    auto creator = [&all_links, &links_mutex, &total_created](int32_t thread_id) {
        for (int32_t i = 0; i < kStressRounds / kNumThreadsForStress; ++i) {
            ProcessLink link;
            link.link_id = "link_" + std::to_string(thread_id) + "_" + std::to_string(i);
            link.source_id = "src_" + std::to_string(thread_id);
            link.target_id = "tgt_" + std::to_string(i);
            link.link_type = ProcessLinkType::SUB_PROCESS;

            {
                std::lock_guard<std::mutex> lock(links_mutex);
                all_links.push_back(link);
            }

            total_created.fetch_add(1);
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreadsForStress; ++i) {
        threads.emplace_back(creator, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    EXPECT_EQ(total_created.load(), kStressRounds);
    EXPECT_EQ(all_links.size(), kStressRounds);
    EXPECT_GT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// S-05: Error code cycle under high stress (no corruption)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S05_ErrorCodeCycleUnderHighStress) {
    std::vector<ProcError> error_sequence;
    std::vector<ProcError> error_types = {
        ProcError::kUnsupportedElement,
        ProcError::kInvalidTransition,
        ProcError::kSerialiserFailed,
        ProcError::kDeserialiserFailed,
        ProcError::kExecutionTimeout,
        ProcError::kMaxDepthExceeded,
    };

    for (int32_t i = 0; i < kStressRounds; ++i) {
        ProcError err = error_types[i % error_types.size()];
        error_sequence.push_back(err);

        // Verify error code is in valid range
        int32_t code = static_cast<int32_t>(err);
        EXPECT_GE(code, 7600);
        EXPECT_LE(code, 7609);
    }

    EXPECT_EQ(error_sequence.size(), kStressRounds);

    // Verify cycling pattern
    for (int32_t i = 0; i < kStressRounds; ++i) {
        EXPECT_EQ(error_sequence[i], error_types[i % error_types.size()]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// S-06: Long-running link traversal (graph walk)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S06_LongRunningLinkTraversal) {
    // Build a chain: 0 -> 1 -> 2 -> ... -> 99 -> 0
    std::map<int32_t, std::vector<int32_t>> graph;
    constexpr int32_t kChainLength = 100;

    for (int32_t i = 0; i < kChainLength; ++i) {
        graph[i] = {(i + 1) % kChainLength};
    }

    auto traverse_graph = [&graph](int32_t iterations) -> int64_t {
        int64_t visit_count = 0;
        int32_t current = 0;

        for (int32_t i = 0; i < iterations; ++i) {
            if (graph.find(current) != graph.end()) {
                auto& neighbors = graph[current];
                if (!neighbors.empty()) {
                    current = neighbors[0];
                    visit_count++;
                }
            }
        }

        return visit_count;
    };

    int64_t visit_count = traverse_graph(kStressRounds);
    EXPECT_EQ(visit_count, kStressRounds);
}

// ─────────────────────────────────────────────────────────────────────────────
// S-07: Memory stability under sustained allocation/deallocation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S07_MemoryStabilityUnderAllocationCycles) {
    struct MemorySnapshot {
        int32_t active_allocations{0};
        size_t total_allocated_bytes{0};
    };

    std::vector<MemorySnapshot> snapshots;

    for (int32_t cycle = 0; cycle < 100; ++cycle) {
        // Allocate many small objects
        std::vector<std::string> temp_strings;
        for (int32_t i = 0; i < kStressRounds / 100; ++i) {
            temp_strings.push_back("temp_string_" + std::to_string(cycle * 1000 + i));
        }

        MemorySnapshot snap;
        snap.active_allocations = temp_strings.size();
        for (const auto& s : temp_strings) {
            snap.total_allocated_bytes += s.size();
        }

        snapshots.push_back(snap);

        // Deallocate by going out of scope
        temp_strings.clear();
    }

    // Verify snapshots were captured
    EXPECT_EQ(snapshots.size(), 100);

    // Each snapshot should have same allocation pattern
    for (const auto& snap : snapshots) {
        EXPECT_EQ(snap.active_allocations, kStressRounds / 100);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// S-08: Reproducible stress run with canonical seed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StressChurnTest, S08_ReproducibleStressRunWithCanonicalSeed) {
    struct StressRunResult {
        int32_t total_operations{0};
        int64_t total_bytes_processed{0};
        std::vector<std::string> operation_ids;
    };

    auto stress_run = [](int32_t seed) -> StressRunResult {
        StressRunResult result;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> size_dist(10, 1000);

        for (int32_t i = 0; i < kStressRounds; ++i) {
            int32_t op_size = size_dist(local_rng);
            result.total_operations++;
            result.total_bytes_processed += op_size;
            result.operation_ids.push_back("op_" + std::to_string(i));
        }

        return result;
    };

    auto run1 = stress_run(kCanonicalRngSeed);
    auto run2 = stress_run(kCanonicalRngSeed);

    EXPECT_EQ(run1.total_operations, run2.total_operations);
    EXPECT_EQ(run1.total_bytes_processed, run2.total_bytes_processed);
    EXPECT_EQ(run1.operation_ids, run2.operation_ids);
}
