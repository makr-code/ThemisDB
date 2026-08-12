/**
 * @file config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Config — validation and dry-run simulation.
 */

#include "themis/gpu/config.h"
#include "themis/edition.h"

namespace themis {
namespace gpu {

// ============================================================================
// validate
// ============================================================================

GPUConfig::ValidationResult GPUConfig::validate() const {
    ValidationResult r;

    // OOM threshold must be in (0, 1].
    if (oom_warning_threshold <= 0.0f || oom_warning_threshold > 1.0f) {
        r.ok = false;
        r.errors.push_back(
            "oom_warning_threshold must be in (0, 1]; got " +
            std::to_string(oom_warning_threshold));
    }

    // Circuit breaker thresholds.
    if (circuit_failure_threshold < 1) {
        r.ok = false;
        r.errors.push_back("circuit_failure_threshold must be >= 1");
    }
    if (circuit_success_threshold < 1) {
        r.ok = false;
        r.errors.push_back("circuit_success_threshold must be >= 1");
    }
    if (circuit_reset_timeout_secs <= 0) {
        r.ok = false;
        r.errors.push_back("circuit_reset_timeout_secs must be > 0");
    }

    // Pool config.
    if (enable_pool && pool_slab_size == 0) {
        r.ok = false;
        r.errors.push_back(
            "pool_slab_size must be > 0 when enable_pool == true");
    }

    // max_vram_bytes vs min_free_vram_bytes.
    if (max_vram_bytes > 0 && min_free_vram_bytes >= max_vram_bytes) {
        r.ok = false;
        r.errors.push_back(
            "min_free_vram_bytes must be < max_vram_bytes");
    }

    // Pool num_slabs derivation warning.
    if (enable_pool && pool_num_slabs == 0 && max_vram_bytes == 0) {
        r.warnings.push_back(
            "pool_num_slabs is 0 and max_vram_bytes is 0; pool will use "
            "the edition VRAM limit to derive slab count");
    }

    // Warn if max_vram_bytes exceeds the edition limit.
    const uint64_t edition_limit =
        static_cast<uint64_t>(edition::GPU_MAX_VRAM_GB) *
        1024ULL * 1024ULL * 1024ULL;
    if (max_vram_bytes > edition_limit && edition_limit > 0) {
        r.warnings.push_back(
            "max_vram_bytes exceeds the edition VRAM limit of " +
            std::to_string(edition::GPU_MAX_VRAM_GB) +
            "GB; allocation attempts will be rejected by the memory manager");
    }

    return r;
}

// ============================================================================
// simulateAllocation
// ============================================================================

std::pair<bool, std::string>
GPUConfig::simulateAllocation(uint64_t bytes,
                               uint64_t current_allocated_bytes) const {
    // Determine effective VRAM limit.
    const uint64_t edition_limit =
        static_cast<uint64_t>(edition::GPU_MAX_VRAM_GB) *
        1024ULL * 1024ULL * 1024ULL;
    const uint64_t effective_limit =
        (max_vram_bytes > 0 && max_vram_bytes <= edition_limit)
            ? max_vram_bytes
            : edition_limit;

    if (effective_limit == 0) {
        return {false, "GPU not available in this edition (VRAM limit = 0)"};
    }

    const uint64_t after = current_allocated_bytes + bytes;

    if (after > effective_limit) {
        return {false,
                "Rejected: would use " +
                std::to_string(after / (1024ULL * 1024ULL)) +
                "MB but limit is " +
                std::to_string(effective_limit / (1024ULL * 1024ULL)) + "MB"};
    }

    // Check minimum free reserve.
    const uint64_t remaining = effective_limit - after;
    if (remaining < min_free_vram_bytes) {
        return {false,
                "Rejected: would leave only " +
                std::to_string(remaining / (1024ULL * 1024ULL)) +
                "MB free, below reserve of " +
                std::to_string(min_free_vram_bytes / (1024ULL * 1024ULL)) +
                "MB"};
    }

    // OOM warning threshold (informational — does not block in dry-run).
    const float usage_after =
        static_cast<float>(after) / static_cast<float>(effective_limit);
    if (usage_after >= oom_warning_threshold) {
        return {true,
                "Accepted (warning: would use " +
                std::to_string(static_cast<int>(usage_after * 100)) +
                "% of VRAM, above " +
                std::to_string(static_cast<int>(oom_warning_threshold * 100)) +
                "% threshold)"};
    }

    return {true, "Accepted"};
}

} // namespace gpu
} // namespace themis

