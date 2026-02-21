/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin_api.cpp                                      ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "themis/gpu/admin_api.h"
#include <sstream>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUAdminAPI::GPUAdminAPI(const GPUConfig& config, GPULoadBalancer* balancer)
    : config_(config), balancer_(balancer) {}

// ============================================================================
// JSON escaping helper
// ============================================================================

std::string GPUAdminAPI::jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// ============================================================================
// GET /admin/gpu/stats
// ============================================================================

std::string GPUAdminAPI::getStatsJson() const {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto  s   = mgr.GetStats();

    const uint64_t limit = mgr.GetMaxGPUVRAMBytes();
    const float    pct   = mgr.GetGPUMemoryUsagePercent();
    const bool     accel = mgr.IsGPUAccelerationEnabled();
    const auto     info  = mgr.GetEditionInfo();

    std::ostringstream j;
    j << "{"
      << "\"edition_vram_limit_bytes\":" << limit << ","
      << "\"allocated_bytes\":"          << s.allocated_bytes    << ","
      << "\"peak_bytes\":"               << s.peak_bytes         << ","
      << "\"allocation_count\":"         << s.allocation_count   << ","
      << "\"deallocation_count\":"       << s.deallocation_count << ","
      << "\"usage_percent\":"            << pct                  << ","
      << "\"gpu_acceleration_enabled\":" << (accel ? "true" : "false") << ","
      << "\"edition_info\":\""           << jsonEscape(info) << "\""
      << "}";
    return j.str();
}

// ============================================================================
// GET /admin/gpu/tenants
// ============================================================================

std::string GPUAdminAPI::getTenantsJson() const {
    auto& mgr     = GPUMemoryManager::GetInstance();
    auto  tenants = mgr.GetAllTenantStats();

    std::ostringstream j;
    j << "[";
    bool first = true;
    for (const auto& t : tenants) {
        if (!first) j << ",";
        first = false;
        const uint64_t headroom = mgr.GetTenantHeadroom(t.tenant_id);
        j << "{"
          << "\"tenant_id\":\""       << jsonEscape(t.tenant_id)     << "\","
          << "\"quota_bytes\":"        << t.quota_bytes               << ","
          << "\"allocated_bytes\":"    << t.allocated_bytes           << ","
          << "\"peak_bytes\":"         << t.peak_bytes                << ","
          << "\"headroom_bytes\":"     << headroom
          << "}";
    }
    j << "]";
    return j.str();
}

// ============================================================================
// GET /admin/gpu/devices
// ============================================================================

std::string GPUAdminAPI::getDevicesJson() const {
    if (!balancer_) {
        return "[]";
    }

    auto loads = balancer_->getDeviceLoads();

    std::ostringstream j;
    j << "[";
    bool first = true;
    for (const auto& d : loads) {
        if (!first) j << ",";
        first = false;
        j << "{"
          << "\"index\":"               << d.index                               << ","
          << "\"name\":\""              << jsonEscape(d.name)                    << "\","
          << "\"backend\":\""           << jsonEscape(d.backend)                 << "\","
          << "\"free_vram_bytes\":"     << d.free_vram_bytes                     << ","
          << "\"tracked_alloc_bytes\":" << d.tracked_alloc_bytes                 << ","
          << "\"is_healthy\":"          << (d.is_healthy ? "true" : "false")     << ","
          << "\"failure_reason\":\""    << jsonEscape(d.failure_reason)          << "\""
          << "}";
    }
    j << "]";
    return j.str();
}

// ============================================================================
// POST /admin/gpu/simulate
// ============================================================================

std::string GPUAdminAPI::simulateJson(uint64_t bytes) const {
    auto& mgr     = GPUMemoryManager::GetInstance();
    auto  stats   = mgr.GetStats();

    // Use the effective max VRAM: config_.max_vram_bytes if set, else edition limit.
    GPUConfig effective = config_;
    if (effective.max_vram_bytes == 0) {
        effective.max_vram_bytes = mgr.GetMaxGPUVRAMBytes();
    }

    auto [accepted, reason] =
        effective.simulateAllocation(bytes, stats.allocated_bytes);

    std::ostringstream j;
    j << "{"
      << "\"accepted\":"   << (accepted ? "true" : "false") << ","
      << "\"reason\":\""   << jsonEscape(reason)            << "\","
      << "\"bytes\":"      << bytes                         << ","
      << "\"current_allocated_bytes\":" << stats.allocated_bytes
      << "}";
    return j.str();
}

// ============================================================================
// GET /admin/gpu/geo  — GPU spatial backend stats
// ============================================================================

std::string GPUAdminAPI::getGeoBackendStatsJson() const {
    return ::themis::geo::getGpuSpatialBackendStatsJson();
}

} // namespace gpu
} // namespace themis
