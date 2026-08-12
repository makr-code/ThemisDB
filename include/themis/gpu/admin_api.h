/**
 * @file admin_api.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=5, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include "themis/gpu/config.h"
#include "themis/gpu/memory_manager.h"
#include "themis/gpu/metrics.h"
#include "themis/gpu/load_balancer.h"
#include "themis/gpu/mig_manager.h"
#include "geo/spatial_backend.h"

namespace themis {
namespace gpu {

/**
 * @brief JSON-serialising admin/ops API for the GPU module.
 *
 * Provides three read-only and one simulation endpoint:
 *
 * GET  /admin/gpu/stats    — Global VRAM stats, circuit state, fallback count.
 * GET  /admin/gpu/tenants  — Per-tenant VRAM usage and quota breakdown.
 * GET  /admin/gpu/devices  — Per-device load from the load balancer.
 * POST /admin/gpu/simulate — Dry-run allocation check using GPUConfig rules.
 *
 * All methods return a UTF-8 JSON string; no HTTP transport is assumed so
 * the class can be used from any HTTP framework or CLI tool.
 *
 * Thread safety: methods are const and delegate to thread-safe sub-systems.
 */
class GPUAdminAPI {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Default constructor: uses the global GPUMemoryManager singleton
     *        and GPUMetrics singleton.  Pass a non-null balancer for device
     *        stats; if nullptr the devices endpoint returns an empty list.
     */
    explicit GPUAdminAPI(const GPUConfig&     config,
                         GPULoadBalancer*     balancer = nullptr);

    // -----------------------------------------------------------------------
    // Endpoints
    // -----------------------------------------------------------------------

    /**
     * @brief Serialise global GPU stats to JSON.
     *
     * Returns a JSON object with keys:
     *   edition_vram_limit_bytes, allocated_bytes, peak_bytes,
     *   allocation_count, deallocation_count, usage_percent,
     *   gpu_acceleration_enabled, edition_info
     */
    std::string getStatsJson() const;

    /**
     * @brief Serialise per-tenant VRAM breakdown to JSON.
     *
     * Returns a JSON array where each element has:
     *   tenant_id, quota_bytes, allocated_bytes, peak_bytes, headroom_bytes
     */
    std::string getTenantsJson() const;

    /**
     * @brief Serialise per-device load to JSON.
     *
     * Returns a JSON array where each element has:
     *   index, name, backend, free_vram_bytes, tracked_alloc_bytes,
     *   is_healthy, failure_reason
     *
     * Returns an empty JSON array if no load balancer was provided.
     */
    std::string getDevicesJson() const;

    /**
     * @brief Dry-run simulation: would @p bytes be accepted right now?
     *
     * Uses GPUConfig::simulateAllocation() against the current live
     * allocation counter.
     *
     * Input JSON (optional, for logging): { "bytes": <uint64>, "tag": "..." }
     * Returns JSON: { "accepted": true/false, "reason": "..." }
     */
    std::string simulateJson(uint64_t bytes) const;

    /**
     * @brief Serialise GPU spatial backend (geo) stats to JSON.
     *
     * Returns a JSON object with keys:
     *   backend_name, gpu_present, circuit_open, device_name,
     *   batch_calls, batch_fallbacks, batch_pairs_processed,
     *   exact_calls, exact_errors, batch_avg_latency_us,
     *   batch_max_latency_us
     *
     * Suitable for the endpoint: GET /admin/gpu/geo
     */
    std::string getGeoBackendStatsJson() const;

    /**
     * @brief Serialise active MIG partition list to JSON.
     *
     * Returns a JSON array where each element has:
     *   instance_id, device_index, gi_id, profile,
     *   memory_bytes, is_active, tenant_id
     *
     * Returns an empty JSON array when no MIG instances exist.  Because
     * `MIGManager::createPartition()` is gated on the `MIG_MANAGER` feature
     * flag, the array will naturally be empty when the flag is disabled.
     *
     * Suitable for the endpoint: GET /admin/gpu/mig
     */
    std::string getMIGInstancesJson() const;

private:
    GPUConfig        config_;
    GPULoadBalancer* balancer_;

    static std::string jsonEscape(const std::string& s);
};

} // namespace gpu
} // namespace themis

