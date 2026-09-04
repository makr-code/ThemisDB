// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_p0_crud_baseline.cpp
 * @brief Phase-0 CRUD baseline performance benchmarks.
 *
 * Establishes initial performance baselines for core ThemisDB operations:
 * Insert, Read, Update, Delete. Uses canonical seeding (42) and 3-phase warmup
 * to ensure reproducible, deterministic measurement across machines and CI runs.
 *
 * **Maturity:** Baseline/Experimental
 * **Version:** 1.0
 * **Status:** Phase-0 baseline scaffolding (P0-D04)
 *
 * Workloads:
 * - INSERT_HEAVY: 80% inserts, 20% reads (write throughput focus)
 * - READ_HEAVY:   80% reads, 20% writes (read throughput focus)
 * - UPDATE_HEAVY: 80% updates, 20% reads (update throughput focus)
 * - DELETE_HEAVY: 80% deletes, 20% inserts (delete throughput focus)
 */

#include <benchmark/benchmark.h>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <string>
#include <cstring>

#include "phase0_fixtures.h"

namespace themis::benchmarks::phase0 {

// ============================================================================
// Baseline Database Stub (For Initial Testing)
// ============================================================================

/**
 * Minimal in-memory database mock for Phase-0 baseline measurements.
 * This is a temporary stub; production benchmarks will link against themis_core.
 */
class BaselineKVStore {
 public:
  struct KeyValue {
    std::string key = {};
    std::string value = {};
  };

  BaselineKVStore() : store_(), rng_(kP0CanonicalSeed) {}

  void Insert(const std::string& key, const std::string& value) {
    store_[key] = value;
  }

  bool Read(const std::string& key, std::string& value) {
    auto it = store_.find(key);
    if (it != store_.end()) {
      value = it->second;
      return true;
    }
    return false;
  }

  bool Update(const std::string& key, const std::string& new_value) {
    auto it = store_.find(key);
    if (it != store_.end()) {
      it->second = new_value;
      return true;
    }
    return false;
  }

  bool Delete(const std::string& key) {
    return store_.erase(key) > 0;
  }

  size_t Size() const { return store_.size(); }

  void Clear() { store_.clear(); }

 private:
  std::unordered_map<std::string, std::string> store_;
  std::mt19937_64 rng_;
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Generate deterministic test keys using canonical seed.
 *
 * @param index Index in the sequence
 * @return Deterministic string key
 */
inline std::string MakeTestKey(size_t index) {
  return "key_" + std::to_string(index);
}

/**
 * Generate deterministic test values using canonical seed.
 *
 * @param index Index in the sequence
 * @return Deterministic string value
 */
inline std::string MakeTestValue(size_t index) {
  return std::string(64, 'v') + std::to_string(index);
}

// ============================================================================
// Benchmark: Insert-Heavy Workload (80% inserts, 20% reads)
// ============================================================================

static void BM_P0_InsertHeavy(benchmark::State& state) {
  // Warmup phase 1 (cold): Initialize store with baseline data
  auto store = std::make_unique<BaselineKVStore>();
  for (int i = 0; i < kP0WarmupCold; ++i) {
    store->Insert(MakeTestKey(i), MakeTestValue(i));
  }

  // Warmup phase 2 (warm): Sequential reads to prime cache
  for (int i = 0; i < kP0WarmupWarm; ++i) {
    std::string dummy = {};
    store->Read(MakeTestKey(i % kP0WarmupCold), dummy);
  }

  // Warmup phase 3 (hot): Random reads to stabilize branch predictor
  std::mt19937_64 rng(kP0CanonicalSeed);
  for (int i = 0; i < kP0WarmupHot; ++i) {
    std::string dummy = {};
    size_t key_idx = rng() % kP0WarmupCold;
    store->Read(MakeTestKey(key_idx), dummy);
  }

  // Measurement window
  size_t op_count = 0;
  for (auto _ : state) {
    op_count++;
    if (rng() < UINT64_MAX * kP0InsertHeavyInsertRatio) {
      // Insert operation
      store->Insert(MakeTestKey(op_count), MakeTestValue(op_count));
    } else {
      // Read operation
      std::string dummy = {};
      store->Read(MakeTestKey(op_count % (kP0WarmupCold + 100)), dummy);
    }
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["operations"] = benchmark::Counter(op_count);
  state.counters["store_size"] = benchmark::Counter(static_cast<double>(store->Size()));
}

// ============================================================================
// Benchmark: Read-Heavy Workload (80% reads, 20% writes)
// ============================================================================

static void BM_P0_ReadHeavy(benchmark::State& state) {
  // Warmup phase 1 (cold): Initialize store with baseline data
  auto store = std::make_unique<BaselineKVStore>();
  for (int i = 0; i < kP0WarmupCold; ++i) {
    store->Insert(MakeTestKey(i), MakeTestValue(i));
  }

  // Warmup phase 2 (warm): Sequential reads
  for (int i = 0; i < kP0WarmupWarm; ++i) {
    std::string dummy = {};
    store->Read(MakeTestKey(i % kP0WarmupCold), dummy);
  }

  // Warmup phase 3 (hot): Random reads
  std::mt19937_64 rng(kP0CanonicalSeed);
  for (int i = 0; i < kP0WarmupHot; ++i) {
    std::string dummy = {};
    size_t key_idx = rng() % kP0WarmupCold;
    store->Read(MakeTestKey(key_idx), dummy);
  }

  // Measurement window
  size_t op_count = 0;
  for (auto _ : state) {
    op_count++;
    if (rng() < UINT64_MAX * kP0ReadHeavyReadRatio) {
      // Read operation
      std::string dummy = {};
      store->Read(MakeTestKey(op_count % kP0WarmupCold), dummy);
    } else {
      // Write operation (insert or update)
      if (rng() & 1) {
        store->Insert(MakeTestKey(op_count), MakeTestValue(op_count));
      } else {
        store->Update(MakeTestKey(op_count % kP0WarmupCold), "updated");
      }
    }
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["operations"] = benchmark::Counter(op_count);
  state.counters["store_size"] = benchmark::Counter(static_cast<double>(store->Size()));
}

// ============================================================================
// Benchmark: Update-Heavy Workload (80% updates, 20% reads)
// ============================================================================

static void BM_P0_UpdateHeavy(benchmark::State& state) {
  // Warmup phase 1 (cold): Initialize store
  auto store = std::make_unique<BaselineKVStore>();
  for (int i = 0; i < kP0WarmupCold; ++i) {
    store->Insert(MakeTestKey(i), MakeTestValue(i));
  }

  // Warmup phase 2 (warm): Sequential reads
  for (int i = 0; i < kP0WarmupWarm; ++i) {
    std::string dummy = {};
    store->Read(MakeTestKey(i % kP0WarmupCold), dummy);
  }

  // Warmup phase 3 (hot): Random reads
  std::mt19937_64 rng(kP0CanonicalSeed);
  for (int i = 0; i < kP0WarmupHot; ++i) {
    std::string dummy = {};
    size_t key_idx = rng() % kP0WarmupCold;
    store->Read(MakeTestKey(key_idx), dummy);
  }

  // Measurement window
  size_t op_count = 0;
  for (auto _ : state) {
    op_count++;
    if (rng() < UINT64_MAX * kP0UpdateHeavyUpdateRatio) {
      // Update operation
      size_t key_idx = op_count % kP0WarmupCold;
      store->Update(MakeTestKey(key_idx), MakeTestValue(op_count));
    } else {
      // Read operation
      std::string dummy = {};
      store->Read(MakeTestKey(op_count % kP0WarmupCold), dummy);
    }
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["operations"] = benchmark::Counter(op_count);
  state.counters["store_size"] = benchmark::Counter(static_cast<double>(store->Size()));
}

// ============================================================================
// Benchmark: Delete-Heavy Workload (80% deletes, 20% inserts)
// ============================================================================

static void BM_P0_DeleteHeavy(benchmark::State& state) {
  // Warmup phase 1 (cold): Initialize store with extra data (for deletions)
  auto store = std::make_unique<BaselineKVStore>();
  for (int i = 0; i < kP0WarmupCold * 2; ++i) {
    store->Insert(MakeTestKey(i), MakeTestValue(i));
  }

  // Warmup phase 2 (warm): Sequential reads
  for (int i = 0; i < kP0WarmupWarm; ++i) {
    std::string dummy = {};
    store->Read(MakeTestKey(i % (kP0WarmupCold * 2)), dummy);
  }

  // Warmup phase 3 (hot): Random reads
  std::mt19937_64 rng(kP0CanonicalSeed);
  for (int i = 0; i < kP0WarmupHot; ++i) {
    std::string dummy = {};
    size_t key_idx = rng() % (kP0WarmupCold * 2);
    store->Read(MakeTestKey(key_idx), dummy);
  }

  // Measurement window
  size_t op_count = 0;
  size_t next_insert_key = kP0WarmupCold * 2;
  for (auto _ : state) {
    op_count++;
    if (rng() < UINT64_MAX * kP0DeleteHeavyDeleteRatio) {
      // Delete operation
      size_t key_idx = op_count % (kP0WarmupCold * 2);
      store->Delete(MakeTestKey(key_idx));
    } else {
      // Insert operation (to replenish deleted items)
      store->Insert(MakeTestKey(next_insert_key++), MakeTestValue(op_count));
    }
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["operations"] = benchmark::Counter(op_count);
  state.counters["store_size"] = benchmark::Counter(static_cast<double>(store->Size()));
}

// ============================================================================
// Benchmark Registration
// ============================================================================

BENCHMARK(BM_P0_InsertHeavy)
    ->MinTime(1.0)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

BENCHMARK(BM_P0_ReadHeavy)
    ->MinTime(1.0)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

BENCHMARK(BM_P0_UpdateHeavy)
    ->MinTime(1.0)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

BENCHMARK(BM_P0_DeleteHeavy)
    ->MinTime(1.0)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

}  // namespace themis::benchmarks::phase0

BENCHMARK_MAIN();
