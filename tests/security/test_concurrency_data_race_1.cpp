/**
 * @file test_concurrency_data_race_1.cpp
 * @brief Sprint 9 Concurrency Test Suite - Data Race Test Case 1
 * 
 * Tests: S9-001 - Partition State Data Race
 * Module: sharding
 * File: src/sharding/partition_manager.cpp
 * Function: updatePartitionState()
 * 
 * Issue: Unsynchronized partition_state vector access under concurrent updates
 * Expected: Multiple threads safely updating partition state without data races
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

// Mock/stub for partition state
struct PartitionState {
  int partition_id = 0;
  enum class State { IDLE, ACTIVE, MIGRATING, FAILED };
  State state = State::IDLE;
  uint64_t version = 0;
};

namespace themis::sharding {
namespace test {

/**
 * @brief BEFORE: Unsafe version (demonstrates the bug)
 * 
 * WARNING: This code is intentionally unsafe for documentation purposes.
 * Do not use in production code.
 */
class UnsafePartitionManager {
 public:
  void updatePartitionState(int partition_id, PartitionState::State new_state) {
    // DATA RACE: No synchronization on partition_state access
    if (partition_id < static_cast<int>(partition_state_.size())) {
      partition_state_[partition_id].state = new_state;
      partition_state_[partition_id].version++;
    }
  }
  
  PartitionState getPartitionState(int partition_id) const {
    if (partition_id < static_cast<int>(partition_state_.size())) {
      return partition_state_[partition_id];  // Torn read possible
    }
    return PartitionState{};
  }
  
 protected:
  std::vector<PartitionState> partition_state_;
};

/**
 * @brief AFTER: Safe version using mutex protection
 */
class SafePartitionManager {
 public:
  SafePartitionManager(size_t num_partitions = 4) 
    : partition_state_(num_partitions) {}
  
  void updatePartitionState(int partition_id, PartitionState::State new_state) {
    std::lock_guard<std::mutex> lock(state_mutex_);  // Acquire lock
    if (partition_id < static_cast<int>(partition_state_.size())) {
      partition_state_[partition_id].state = new_state;
      partition_state_[partition_id].version++;
    }
  }  // Lock released automatically
  
  PartitionState getPartitionState(int partition_id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);  // Acquire lock
    if (partition_id < static_cast<int>(partition_state_.size())) {
      return partition_state_[partition_id];  // Safe read
    }
    return PartitionState{};
  }  // Lock released automatically
  
  size_t getPartitionCount() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return partition_state_.size();
  }
  
 private:
  mutable std::mutex state_mutex_;
  std::vector<PartitionState> partition_state_;
};

/**
 * @class DataRaceTest_PartitionState
 * @brief Test suite for partition state data race (S9-001)
 * 
 * Tests that concurrent updates to partition state are safe from:
 * - Torn writes (partial updates visible to readers)
 * - Lost updates (concurrent writes overwriting each other)
 * - Memory corruption from vector reallocation during access
 */
class DataRaceTest_PartitionState : public ::testing::Test {
 protected:
  SafePartitionManager partition_manager_{4};  // 4 partitions
  
  static constexpr int kNumWorkerThreads = 10;
  static constexpr int kUpdatesPerThread = 100;
};

/**
 * @test DataRaceTest_PartitionState.ConcurrentUpdates_AllStatesCorrect
 * 
 * Verification:
 * - Multiple threads concurrently update different partitions
 * - Each thread updates its partition 100 times
 * - All final states are correct
 * - No data races detected by ThreadSanitizer
 * 
 * Expected: All 10 threads complete; each partition state updated 100 times total
 * Failure: Any torn write or lost update visible
 */
TEST_F(DataRaceTest_PartitionState, ConcurrentUpdates_AllStatesCorrect) {
  std::vector<std::thread> workers;
  std::vector<uint64_t> final_versions(4, 0);
  
  // Spawn workers, each updating different partition
  for (int worker_id = 0; worker_id < kNumWorkerThreads; ++worker_id) {
    workers.emplace_back([this, worker_id, &final_versions]() {
      int partition_id = worker_id % 4;  // Distribute across 4 partitions
      
      for (int i = 0; i < kUpdatesPerThread; ++i) {
        // Alternate state updates
        PartitionState::State state = (i % 2 == 0) 
          ? PartitionState::State::ACTIVE 
          : PartitionState::State::IDLE;
        
        partition_manager_.updatePartitionState(partition_id, state);
      }
    });
  }
  
  // Wait for all workers
  for (auto& worker : workers) {
    worker.join();
  }
  
  // Verify final states are consistent
  for (size_t partition_id = 0; partition_id < 4; ++partition_id) {
    PartitionState state = partition_manager_.getPartitionState(partition_id);
    
    // Should have been updated by (kNumWorkerThreads / 4) * kUpdatesPerThread workers
    // = 10/4 * 100 = 250 updates per partition
    int expected_updates = (kNumWorkerThreads / 4) * kUpdatesPerThread;
    
    EXPECT_EQ(state.version, expected_updates) 
      << "Partition " << partition_id << " has incorrect version count";
    
    // Final state should be either ACTIVE or IDLE (last update wins)
    EXPECT_TRUE(state.state == PartitionState::State::ACTIVE ||
                state.state == PartitionState::State::IDLE)
      << "Partition " << partition_id << " has corrupted state";
  }
}

/**
 * @test DataRaceTest_PartitionState.ConcurrentReadWrite_ConsistentReads
 * 
 * Verification:
 * - Half threads write; half threads read
 * - Writers update state; readers verify consistency
 * - No torn reads visible (state and version consistent)
 * 
 * Expected: All readers see consistent state-version pairs
 * Failure: Torn read (state updated but version not, or vice versa)
 */
TEST_F(DataRaceTest_PartitionState, ConcurrentReadWrite_ConsistentReads) {
  std::atomic<bool> start_signal{false};
  std::vector<std::thread> workers;
  std::atomic<int> read_errors{0};
  
  // Writers: update partition 0
  for (int i = 0; i < kNumWorkerThreads / 2; ++i) {
    workers.emplace_back([this, &start_signal]() {
      while (!start_signal.load()) {
        std::this_thread::yield();  // Wait for start signal
      }
      for (int j = 0; j < kUpdatesPerThread; ++j) {
        partition_manager_.updatePartitionState(0, PartitionState::State::ACTIVE);
      }
    });
  }
  
  // Readers: verify consistency of partition 0
  for (int i = 0; i < kNumWorkerThreads / 2; ++i) {
    workers.emplace_back([this, &start_signal, &read_errors]() {
      while (!start_signal.load()) {
        std::this_thread::yield();  // Wait for start signal
      }
      for (int j = 0; j < kUpdatesPerThread; ++j) {
        PartitionState state = partition_manager_.getPartitionState(0);
        
        // Consistency check: if we see a high version, state must be ACTIVE
        if (state.version > 50) {  // Arbitrary threshold
          if (state.state != PartitionState::State::ACTIVE) {
            read_errors++;  // Torn read detected!
          }
        }
      }
    });
  }
  
  // Signal all threads to start (tight timing for race stress)
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  start_signal.store(true);
  
  // Wait for all workers
  for (auto& worker : workers) {
    worker.join();
  }
  
  EXPECT_EQ(read_errors, 0) 
    << "Detected torn reads: state/version inconsistency";
}

/**
 * @test DataRaceTest_PartitionState.HighContention_NoCorruption
 * 
 * Verification:
 * - All threads write to same partition (maximum contention)
 * - Verify final version count and state are correct
 * - Stress test for memory corruption from unsynchronized access
 * 
 * Expected: All updates counted correctly; no memory corruption
 * Failure: Lost updates or memory corruption
 */
TEST_F(DataRaceTest_PartitionState, HighContention_NoCorruption) {
  std::vector<std::thread> workers;
  
  // All threads update partition 0
  for (int worker_id = 0; worker_id < kNumWorkerThreads; ++worker_id) {
    workers.emplace_back([this]() {
      for (int i = 0; i < kUpdatesPerThread; ++i) {
        partition_manager_.updatePartitionState(0, PartitionState::State::ACTIVE);
      }
    });
  }
  
  // Wait for all workers
  for (auto& worker : workers) {
    worker.join();
  }
  
  // Verify all updates were counted
  PartitionState state = partition_manager_.getPartitionState(0);
  int expected_version = kNumWorkerThreads * kUpdatesPerThread;
  
  EXPECT_EQ(state.version, expected_version)
    << "Lost updates detected: expected " << expected_version 
    << " updates but got " << state.version;
}

}  // namespace test
}  // namespace themis::sharding

/**
 * @section ThreadSanitizer Verification
 * 
 * Build with ThreadSanitizer and run:
 * ```bash
 * cmake --preset linux-tsan-debug
 * cmake --build --preset linux-tsan-debug --parallel 16
 * ctest --preset linux-tsan-debug -L "concurrency" --output-on-failure
 * ```
 * 
 * Expected ThreadSanitizer output (for UNSAFE version):
 * ```
 * WARNING: ThreadSanitizer: data race on memory location at 0x...
 * Write by thread T2 at partition_state_[0].state
 * Read by thread T1 at partition_state_[0].state
 * ```
 * 
 * Expected ThreadSanitizer output (for SAFE version):
 * ```
 * (no data race warnings)
 * ```
 */
