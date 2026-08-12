/**
 * @file config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "themis/gpu/cluster_config.h"

namespace themis {
namespace gpu {

/**
 * @brief GPU module configuration.
 *
 * Populated at startup (from environment, YAML, or code) and validated by
 * GPUConfig::validate() before the rest of the GPU module initialises.
 * Validation failures are surfaced as a list of human-readable error strings
 * so the caller can either abort or log all problems at once.
 *
 * Dry-run simulation: GPUConfig::simulateAllocation() lets operators test
 * whether a proposed allocation would be accepted given the current config,
 * without touching real GPU state.
 */
struct GPUConfig {
    // -----------------------------------------------------------------------
    // VRAM limits
    // -----------------------------------------------------------------------
    uint64_t    max_vram_bytes        = 0;   ///< 0 = use edition default
    uint64_t    min_free_vram_bytes   = 512ULL * 1024 * 1024;  ///< 512 MB reserve
    float       oom_warning_threshold = 0.85f; ///< 0–1: emit warning above this

    // -----------------------------------------------------------------------
    // Pool
    // -----------------------------------------------------------------------
    bool        enable_pool    = true;
    uint64_t    pool_slab_size = 256ULL * 1024 * 1024;  ///< 256 MB per slab
    size_t      pool_num_slabs = 0;  ///< 0 = derive from max_vram_bytes

    // -----------------------------------------------------------------------
    // Circuit breaker
    // -----------------------------------------------------------------------
    size_t      circuit_failure_threshold  = 3;
    size_t      circuit_success_threshold  = 2;
    int         circuit_reset_timeout_secs = 30;

    // -----------------------------------------------------------------------
    // Fallback
    // -----------------------------------------------------------------------
    bool        enable_cpu_fallback    = true;

    /**
     * @brief Maximum time the CPU fallback path may take, in milliseconds.
     *
     * When > 0 the GPU module records a budget-exceeded warning if a CPU
     * fallback operation takes longer than this value.  Set to 0 to disable
     * budget enforcement (no-limit).
     */
    uint32_t    fallback_cpu_budget_ms = 0;

    // -----------------------------------------------------------------------
    // Audit / Observability
    // -----------------------------------------------------------------------
    size_t      audit_log_capacity  = 4096;
    bool        enable_metrics      = true;

    // -----------------------------------------------------------------------
    // Multi-node cluster coordination (optional)
    //
    // Leave cluster.enabled == false (the default) for single-node deployments.
    // When cluster.enabled == true, GPUModule::initialize() will pass this
    // config to GPUClusterCoordinator::initialize() enabling topology-aware
    // scheduling and InfiniBand-backed inter-node transfers.
    // -----------------------------------------------------------------------
    ClusterConfig cluster;  ///< Default (enabled=false) = single-node mode

    // -----------------------------------------------------------------------
    // Validation result
    // -----------------------------------------------------------------------
    struct ValidationResult {
        bool                     ok = true;
        std::vector<std::string> errors;   ///< Non-empty when !ok
        std::vector<std::string> warnings; ///< Non-fatal issues
    };

    /**
     * @brief Validate all fields for consistency.
     *
     * Rules enforced
     * - pool_slab_size must be > 0 when enable_pool == true
     * - oom_warning_threshold must be in (0, 1]
     * - circuit_failure_threshold must be >= 1
     * - circuit_success_threshold must be >= 1
     * - circuit_reset_timeout_secs must be > 0
     * - If max_vram_bytes is non-zero, min_free_vram_bytes must be < max_vram_bytes
     * - If pool is enabled and pool_num_slabs is 0, max_vram_bytes must be set
     *   so it can be derived (warns if not)
     */
    ValidationResult validate() const;

    /**
     * @brief Simulate whether an allocation of @p bytes would be accepted
     *        under this config given @p current_allocated_bytes.
     *
     * Does not touch any real GPU state.  Returns a pair of (accepted, reason).
     */
    std::pair<bool, std::string> simulateAllocation(
        uint64_t bytes,
        uint64_t current_allocated_bytes) const;
};

} // namespace gpu
} // namespace themis
