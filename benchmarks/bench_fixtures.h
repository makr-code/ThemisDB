// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file bench_fixtures.h
 * @brief Shared benchmark fixtures and utilities for ThemisDB benchmarks.
 *
 * Consolidates repeated setup code that was duplicated across:
 *   - bench_advanced_patterns.cpp  (RandomGenerator, BenchmarkDB)
 *   - bench_comprehensive.cpp      (genVec, SimpleVectorBench)
 *   - bench_graph_traversal.cpp    (GraphTraversalBenchmarkFixture)
 *   - bench_query.cpp              (key generation helpers)
 *   - bench_storage_performance.cpp (TempDir utilities)
 *
 * Note: TPCCLiteFixture and YCSBLiteFixture are intentionally kept in their
 * canonical files (bench_tpcc.cpp / bench_ycsb.cpp) because they own the
 * complete TPC-C / YCSB specification.  Benchmarks that need those workloads
 * should BENCHMARK_REGISTER_F against them directly rather than copy/paste.
 *
 * Usage:
 * @code
 *   #include "bench_fixtures.h"
 *
 *   BENCHMARK_F(themis::bench::VectorBenchFixture, MyBench)(benchmark::State& state) {
 *       for (auto _ : state) {
 *           auto q = rng_.genVec(128);
 *           benchmark::DoNotOptimize(q);
 *       }
 *   }
 * @endcode
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <atomic>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {

// ---------------------------------------------------------------------------
// Measurement-hygiene constants
// ---------------------------------------------------------------------------

/**
 * @brief Canonical RNG seed for all ThemisDB benchmarks.
 *
 * All benchmarks MUST use this seed (or a clearly documented derivative) so
 * that measurements are reproducible across runs and comparable across
 * benchmark targets.  Never seed from std::random_device or from timestamps
 * inside benchmark bodies.
 *
 * Usage:
 * @code
 *   std::mt19937 rng{themis::bench::kCanonicalRngSeed};
 * @endcode
 */
static constexpr uint64_t kCanonicalRngSeed = 42;

// ---------------------------------------------------------------------------
// RandomGenerator – thread-local PRNG utilities
// ---------------------------------------------------------------------------

/**
 * @brief Thread-local random number generator for benchmark bodies.
 *
 * All distributions are seeded deterministically from a per-thread seed so
 * benchmark results are reproducible.  Use kCanonicalRngSeed as the default
 * seed unless there is an explicit, documented reason to use a different value.
 */
class RandomGenerator {
public:
    explicit RandomGenerator(uint64_t seed = kCanonicalRngSeed)
        : rng_(static_cast<std::mt19937_64::result_type>(seed))
    {}

    /** @brief Generate a unit-sphere float vector of dimension @p dim. */
    std::vector<float> genVec(std::size_t dim) {
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) {
          x = dis(rng_);
        }
        return v;
    }

    /** @brief Generate a random alphanumeric key of length @p len. */
    std::string genKey(std::size_t len = 16) {
        static constexpr char kChars[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<std::size_t> dis(0, sizeof(kChars) - 2);
        std::string s(len, ' ');
        for (auto& c : s) {
          c = kChars[dis(rng_)];
        }
        return s;
    }

    /** @brief Generate a random integer in [lo, hi]. */
    int64_t genInt(int64_t lo, int64_t hi) {
        std::uniform_int_distribution<int64_t> dis(lo, hi);
        return dis(rng_);
    }

    /** @brief Return a reference to the underlying Mersenne Twister engine. */
    std::mt19937_64& engine() noexcept { return rng_; }

    /** @brief Static thread-local instance (lazily initialised). */
    static RandomGenerator& instance() {
        static thread_local RandomGenerator gen;
        return gen;
    }

private:
    std::mt19937_64 rng_;
};

// ---------------------------------------------------------------------------
// TempDir – RAII temporary directory
// ---------------------------------------------------------------------------

/**
 * @brief Creates a unique temporary directory in SetUp() and removes it in
 *        TearDown().  Safe for use inside benchmark fixtures.
 */
class TempDir {
public:
    TempDir() {
        path_ = fs::temp_directory_path() / ("themis_bench_" + randomSuffix());
        fs::create_directories(path_);
    }

    ~TempDir() {
        try { fs::remove_all(path_); } catch (...) {}
    }

    const fs::path& path() const noexcept { return path_; }
    std::string str() const { return path_.string(); }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

private:
    static std::string randomSuffix() {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::to_string(now);
    }

    fs::path path_;
};

// ---------------------------------------------------------------------------
// StorageBenchFixture – base fixture for storage benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Base fixture that opens an in-memory (or tmp-dir) RocksDB instance
 *        for each benchmark iteration group.
 */
class StorageBenchFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        tmpDir_ = std::make_unique<TempDir>();
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = tmpDir_->str();
        cfg.create_if_missing = true;
        db_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        db_->open();
    }

    void TearDown(::benchmark::State& /*state*/) override {
        db_.reset();
        tmpDir_.reset();
    }

protected:
    std::unique_ptr<TempDir>              tmpDir_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    RandomGenerator                        rng_;
};

// ---------------------------------------------------------------------------
// VectorBenchFixture – base fixture for vector index benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Fixture that pre-populates a VectorIndex with @p kDefaultVectors
 *        random 128-D vectors for use in search/insert benchmarks.
 */
class VectorBenchFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t kDim            = 128;
    static constexpr std::size_t kDefaultVectors = 10'000;

    void SetUp(::benchmark::State& state) override {
        tmpDir_ = std::make_unique<TempDir>();
        themis::VectorIndexConfig cfg;
        cfg.dimension = kDim;
        cfg.db_path   = tmpDir_->str();
        idx_ = std::make_shared<themis::VectorIndexManager>(cfg);

        const std::size_t n = static_cast<std::size_t>(
            state.range(0) > 0 ? state.range(0) : static_cast<int64_t>(kDefaultVectors));
        for (std::size_t i = 0; i < n; ++i) {
            auto v = rng_.genVec(kDim);
            idx_->insert("doc_" + std::to_string(i), v);
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        idx_.reset();
        tmpDir_.reset();
    }

protected:
    std::unique_ptr<TempDir>                    tmpDir_;
    std::shared_ptr<themis::VectorIndexManager> idx_;
    RandomGenerator                              rng_;
};

// ---------------------------------------------------------------------------
// GraphBenchFixture – base fixture for graph index benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Fixture that pre-populates a GraphIndex with @p kDefaultNodes nodes
 *        and random edges for use in traversal/path-finding benchmarks.
 *
 * Uses the canonical API: RocksDBWrapper (opened) + GraphIndexManager(RocksDBWrapper&).
 * Edges are represented as BaseEntity instances with _from/_to/_graph fields;
 * there is no separate addNode() call — node presence is inferred from edges.
 */
class GraphBenchFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t kDefaultNodes = 5'000;
    static constexpr std::size_t kEdgesPerNode = 5;

    void SetUp(::benchmark::State& state) override {
        tmpDir_ = std::make_unique<TempDir>();
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = tmpDir_->str();
        cfg.create_if_missing = true;
        dbStorage_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        dbStorage_->open();
        graph_ = std::make_shared<themis::GraphIndexManager>(*dbStorage_);

        const std::size_t n = static_cast<std::size_t>(
            state.range(0) > 0 ? state.range(0) : static_cast<int64_t>(kDefaultNodes));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t e = 0; e < kEdgesPerNode; ++e) {
                std::size_t j = static_cast<std::size_t>(
                    rng_.genInt(0, static_cast<int64_t>(n) - 1));
                if (j == i) {
                  continue;
                }
                std::string eid =
                    "e_" + std::to_string(i) + "_" + std::to_string(j);
                themis::BaseEntity edge(eid);
                edge.setField("_from", "node_" + std::to_string(i));
                edge.setField("_to",   "node_" + std::to_string(j));
                edge.setField("_graph", "bench_graph");
                graph_->addEdge(edge);
            }
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        graph_.reset();
        dbStorage_.reset();
        tmpDir_.reset();
    }

protected:
    std::unique_ptr<TempDir>                    tmpDir_;
    std::unique_ptr<themis::RocksDBWrapper>     dbStorage_;
    std::shared_ptr<themis::GraphIndexManager>  graph_;
    RandomGenerator                              rng_;
};

} // namespace bench
} // namespace themis
