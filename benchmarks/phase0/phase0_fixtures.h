// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file phase0_fixtures.h
 * @brief Canonical fixtures and constants for Phase-0 baseline benchmarks.
 *
 * Phase-0 benchmarks establish initial performance baselines for core CRUD
 * operations (Insert, Read, Update, Delete). This header provides canonical
 * seeding, 3-phase warmup constants, and helper functions to ensure
 * reproducibility and measurement hygiene across all Phase-0 tests.
 *
 * **Maturity:** Baseline/Experimental
 * **Version:** 1.0
 * **Status:** Initial Phase-0 scaffolding for release-readiness planning
 */

#ifndef THEMIS_BENCHMARKS_PHASE0_FIXTURES_H_
#define THEMIS_BENCHMARKS_PHASE0_FIXTURES_H_

#include <cstdint>
#include <string_view>
#include <chrono>
#include <filesystem>

namespace themis::benchmarks::phase0 {

// ============================================================================
// Canonical Seeding
// ============================================================================

/// Canonical RNG seed for all Phase-0 benchmarks (deterministic data sequences).
static constexpr uint64_t kP0CanonicalSeed = 42;

// ============================================================================
// 3-Phase Warmup Protocol
// ============================================================================

/// Phase 1 (Cold): Fill write buffer, prime OS I/O path.
static constexpr int kP0WarmupCold = 50;

/// Phase 2 (Warm): Sequential reads to warm OS page cache.
static constexpr int kP0WarmupWarm = 100;

/// Phase 3 (Hot): Random reads to stabilise CPU cache + branch predictor.
static constexpr int kP0WarmupHot = 200;

// ============================================================================
// Workload Profiles
// ============================================================================

/// Insert-heavy workload: 80% inserts, 20% reads.
static constexpr double kP0InsertHeavyInsertRatio = 0.80;

/// Read-heavy workload: 80% reads, 20% writes.
static constexpr double kP0ReadHeavyReadRatio = 0.80;

/// Update-heavy workload: 80% updates, 20% reads.
static constexpr double kP0UpdateHeavyUpdateRatio = 0.80;

/// Delete-heavy workload: 80% deletes, 20% inserts.
static constexpr double kP0DeleteHeavyDeleteRatio = 0.80;

// ============================================================================
// Temporary Directory Helpers
// ============================================================================

/**
 * Create a unique temporary path for Phase-0 benchmarks.
 *
 * @param prefix Prefix for the temporary directory (e.g., "p0_crud")
 * @return Filesystem path to a unique temporary directory in the OS temp area
 *
 * **Contract:**
 * - Path is in std::filesystem::temp_directory_path()
 * - Includes steady_clock timestamp to avoid collisions in parallel runs
 * - Caller is responsible for cleanup on benchmark completion
 */
inline std::filesystem::path MakeTempPath(std::string_view prefix) {
    auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() /
           (std::string(prefix) + "_" + suffix);
}

// ============================================================================
// Benchmark Registration Helpers
// ============================================================================

/**
 * Describe Phase-0 baseline workload for benchmark metadata.
 *
 * @param profile Workload profile name (e.g., "insert_heavy", "read_heavy")
 * @return Human-readable description for benchmark registration
 */
inline std::string DescribeP0Workload(std::string_view profile) {
    if (profile == "insert_heavy") {
        return "80% inserts, 20% reads; measures write throughput & latency";
    } else if (profile == "read_heavy") {
        return "80% reads, 20% writes; measures read throughput & latency";
    } else if (profile == "update_heavy") {
        return "80% updates, 20% reads; measures update throughput & latency";
    } else if (profile == "delete_heavy") {
        return "80% deletes, 20% inserts; measures delete throughput & latency";
    }
    return "Unknown workload profile";
}

}  // namespace themis::benchmarks::phase0

#endif  // THEMIS_BENCHMARKS_PHASE0_FIXTURES_H_
