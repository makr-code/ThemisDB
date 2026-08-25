/**
 * @file admin_api.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=4, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/admin_api.h"

#include <sstream>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUAdminAPI::GPUAdminAPI(const GPUConfig &config, GPULoadBalancer *balancer) : config_(config), balancer_(balancer) {}

// ============================================================================
// JSON escaping helper
// ============================================================================

std::string GPUAdminAPI::jsonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

// ============================================================================
// GET /admin/gpu/stats
// ============================================================================

std::string GPUAdminAPI::getStatsJson() const {
    try {
        auto &mgr = GPUMemoryManager::GetInstance();
        auto s    = mgr.GetStats();

        const uint64_t limit = mgr.GetMaxGPUVRAMBytes();
        const float pct      = mgr.GetGPUMemoryUsagePercent();
        const bool accel     = mgr.IsGPUAccelerationEnabled();
        const auto info      = mgr.GetEditionInfo();

        std::ostringstream j;
        j << "{"
          << "\"edition_vram_limit_bytes\":" << limit << ","
          << "\"allocated_bytes\":" << s.allocated_bytes << ","
          << "\"peak_bytes\":" << s.peak_bytes << ","
          << "\"allocation_count\":" << s.allocation_count << ","
          << "\"deallocation_count\":" << s.deallocation_count << ","
          << "\"usage_percent\":" << pct << ","
          << "\"gpu_acceleration_enabled\":" << (accel ? "true" : "false") << ","
          << "\"edition_info\":\"" << jsonEscape(info) << "\""
          << "}";
        return j.str();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getStatsJson() memory allocation failure");
        }
        return "{\"error\":\"stats unavailable\",\"details\":\"memory allocation failure\"}";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getStatsJson() runtime error: {}", e.what());
        }
        return "{\"error\":\"stats unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getStatsJson() threw exception: {}", e.what());
        }
        // Return error JSON on exception
        return "{\"error\":\"stats unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    }
}

// ============================================================================
// GET /admin/gpu/tenants
// ============================================================================

std::string GPUAdminAPI::getTenantsJson() const {
    try {
        auto &mgr    = GPUMemoryManager::GetInstance();
        auto tenants = mgr.GetAllTenantStats();

        std::ostringstream j;
        j << "[";
        bool first = true;
        for (const auto &t : tenants) {
            if (!first) {
                j << ",";
            }
            first                   = false;
            const uint64_t headroom = mgr.GetTenantHeadroom(t.tenant_id);
            j << "{"
              << "\"tenant_id\":\"" << jsonEscape(t.tenant_id) << "\","
              << "\"quota_bytes\":" << t.quota_bytes << ","
              << "\"allocated_bytes\":" << t.allocated_bytes << ","
              << "\"peak_bytes\":" << t.peak_bytes << ","
              << "\"headroom_bytes\":" << headroom << "}";
        }
        j << "]";
        return j.str();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getTenantsJson() memory allocation failure");
        }
        return "[{\"error\":\"tenants unavailable\",\"details\":\"memory allocation failure\"}]";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getTenantsJson() runtime error: {}", e.what());
        }
        return "[{\"error\":\"tenants unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getTenantsJson() threw exception: {}", e.what());
        }
        // Return error JSON on exception
        return "[{\"error\":\"tenants unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    }
}

// ============================================================================
// GET /admin/gpu/devices
// ============================================================================

std::string GPUAdminAPI::getDevicesJson() const {
    try {
        if (!balancer_) {
            return "[]";
        }

        auto loads = balancer_->getDeviceLoads();

        std::ostringstream j;
        j << "[";
        bool first = true;
        for (const auto &d : loads) {
            if (!first) {
                j << ",";
            }
            first = false;
            j << "{"
              << "\"index\":" << d.index << ","
              << "\"name\":\"" << jsonEscape(d.name) << "\","
              << "\"backend\":\"" << jsonEscape(d.backend) << "\","
              << "\"free_vram_bytes\":" << d.free_vram_bytes << ","
              << "\"tracked_alloc_bytes\":" << d.tracked_alloc_bytes << ","
              << "\"is_healthy\":" << (d.is_healthy ? "true" : "false") << ","
              << "\"failure_reason\":\"" << jsonEscape(d.failure_reason) << "\""
              << "}";
        }
        j << "]";
        return j.str();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getDevicesJson() memory allocation failure");
        }
        return "[{\"error\":\"devices unavailable\",\"details\":\"memory allocation failure\"}]";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getDevicesJson() runtime error: {}", e.what());
        }
        return "[{\"error\":\"devices unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getDevicesJson() threw exception: {}", e.what());
        }
        // Return error JSON on exception
        return "[{\"error\":\"devices unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    }
}

// ============================================================================
// POST /admin/gpu/simulate
// ============================================================================

std::string GPUAdminAPI::simulateJson(uint64_t bytes) const {
    try {
        auto &mgr  = GPUMemoryManager::GetInstance();
        auto stats = mgr.GetStats();

        // Use the effective max VRAM: config_.max_vram_bytes if set, else edition limit.
        GPUConfig effective = config_;
        if (effective.max_vram_bytes == 0) {
            effective.max_vram_bytes = mgr.GetMaxGPUVRAMBytes();
        }

        auto [accepted, reason] = effective.simulateAllocation(bytes, stats.allocated_bytes);

        std::ostringstream j;
        j << "{"
          << "\"accepted\":" << (accepted ? "true" : "false") << ","
          << "\"reason\":\"" << jsonEscape(reason) << "\","
          << "\"bytes\":" << bytes << ","
          << "\"current_allocated_bytes\":" << stats.allocated_bytes << "}";
        return j.str();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::simulateJson() memory allocation failure");
        }
        return "{\"error\":\"simulation failed\",\"details\":\"memory allocation failure\"}";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::simulateJson() runtime error: {}", e.what());
        }
        return "{\"error\":\"simulation failed\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::simulateJson() threw exception: {}", e.what());
        }
        // Return error JSON on exception
        return "{\"error\":\"simulation failed\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    }
}

// ============================================================================
// GET /admin/gpu/geo  — GPU spatial backend stats
// ============================================================================

std::string GPUAdminAPI::getGeoBackendStatsJson() const {
    try {
        return ::themis::geo::getGpuSpatialBackendStatsJson();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getGeoBackendStatsJson() memory allocation failure");
        }
        return "{\"error\":\"geo stats unavailable\",\"details\":\"memory allocation failure\"}";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getGeoBackendStatsJson() runtime error: {}", e.what());
        }
        return "{\"error\":\"geo stats unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getGeoBackendStatsJson() threw exception: {}", e.what());
        }
        return "{\"error\":\"geo stats unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}";
    }
}

// ============================================================================
// GET /admin/gpu/mig  — MIG partition list
// ============================================================================

std::string GPUAdminAPI::getMIGInstancesJson() const {
    try {
        // getInstances() is a read-only query that bypasses the feature flag;
        // it returns an empty vector when the MIG_MANAGER flag is disabled
        // (because createPartition() is gated on the flag, so no instances will
        // have been registered).  No explicit feature-flag check is needed here.
        const auto instances = MIGManager::GetInstance().getInstances();

        std::ostringstream j;
        j << "[";
        bool first = true;
        for (const auto &inst : instances) {
            if (!first) {
                j << ",";
            }
            first = false;
            j << "{"
              << "\"instance_id\":\"" << jsonEscape(inst.instance_id) << "\","
              << "\"device_index\":" << inst.device_index << ","
              << "\"gi_id\":" << inst.gi_id << ","
              << "\"profile\":\"" << jsonEscape(inst.profile) << "\","
              << "\"memory_bytes\":" << inst.memory_bytes << ","
              << "\"is_active\":" << (inst.is_active ? "true" : "false") << ","
              << "\"tenant_id\":\"" << jsonEscape(inst.tenant_id) << "\""
              << "}";
        }
        j << "]";
        return j.str();
    } catch (const std::bad_alloc &) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getMIGInstancesJson() memory allocation failure");
        }
        return "[{\"error\":\"MIG instances unavailable\",\"details\":\"memory allocation failure\"}]";
    } catch (const std::runtime_error &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getMIGInstancesJson() runtime error: {}", e.what());
        }
        return "[{\"error\":\"MIG instances unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    } catch (const std::exception &e) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->error("GPUAdminAPI::getMIGInstancesJson() threw exception: {}", e.what());
        }
        return "[{\"error\":\"MIG instances unavailable\",\"details\":\"" + jsonEscape(std::string(e.what())) + "\"}]";
    }
}

} // namespace gpu
} // namespace themis
