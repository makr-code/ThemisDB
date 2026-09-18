/**
 * @file tenant_buffer_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Tenant-Aware CDC Buffer Manager
 * 
 * Provides multi-tenant isolation for CDC operations with per-tenant
 * buffers, metrics, and rate limiting.
 * 
 * Features:
 * - Per-tenant buffer isolation
 * - Per-tenant metrics and statistics
 * - Per-tenant rate limiting
 * - Automatic tenant buffer lifecycle management
 * - Thread-safe multi-tenant operations
 * - Backward compatible with single-tenant mode
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "cdc/changefeed_buffer.h"
#include "cdc/cdc_metrics.h"
#include "cdc/cdc_error.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <functional>

namespace themis {
namespace cdc {

struct TenantConfig {
    std::string tenant_id;
    
    // Buffer configuration
    ChangefeedBufferConfig buffer_config;
    
    // Per-tenant quotas
    bool enable_quotas = false;
    size_t max_events_per_second = 10000;    // Events/sec quota
    size_t max_memory_bytes = 100 * 1024 * 1024;  // 100MB per tenant
    size_t max_buffered_events = 10000;      // Max events in buffer
    
    // Tenant-specific settings
    bool enabled = true;                     // Tenant is active
    int priority = 0;                        // Higher priority = more resources
};

struct TenantStats {
    std::string tenant_id;
    
    // Counters
    uint64_t events_recorded = 0;
    uint64_t events_flushed = 0;
    uint64_t errors = 0;
    uint64_t quota_violations = 0;
    
    // Resource usage
    size_t current_buffer_size = 0;
    size_t current_memory_bytes = 0;
    
    // Rates (calculated)
    double events_per_second = 0.0;
    double memory_usage_percent = 0.0;
    
    nlohmann::json toJson() const {
        return {
            {"tenant_id", tenant_id},
            {"counters", {
                {"events_recorded", events_recorded},
                {"events_flushed", events_flushed},
                {"errors", errors},
                {"quota_violations", quota_violations}
            }},
            {"resource_usage", {
                {"current_buffer_size", current_buffer_size},
                {"current_memory_bytes", current_memory_bytes},
                {"events_per_second", events_per_second},
                {"memory_usage_percent", memory_usage_percent}
            }}
        };
    }
};

class TenantBufferManager {
public:
    TenantBufferManager(Changefeed* changefeed, 
                       const ChangefeedBufferConfig& default_config = ChangefeedBufferConfig());
    
    ~TenantBufferManager() noexcept;
    
    /**
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    /**
     * @brief Record Event.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    Changefeed::ChangeEvent recordEvent(const std::string& tenant_id,
                                       Changefeed::ChangeEvent event);
    
    /**
     * @brief Flush Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t flushTenant(const std::string& tenant_id);
    
    /**
     * @brief Flush All.
     * @return Return value.
     */
    size_t flushAll();
    
    /**
     * @brief Configure Tenant.
     * @param[in] config Input parameter.
     */
    void configureTenant(const TenantConfig& config);
    
    /**
     * @brief Get Tenant Config.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::optional<TenantConfig> getTenantConfig(const std::string& tenant_id) const;
    
    /**
     * @brief Get Tenant Stats.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::optional<TenantStats> getTenantStats(const std::string& tenant_id) const;
    
    /**
     * @brief Get Tenant Metrics.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::optional<std::reference_wrapper<const CDCMetrics>> getTenantMetrics(const std::string& tenant_id) const;
    
        /**
         * @brief Get Global Metrics.
         * @return Return value.
         */
        nlohmann::json getGlobalMetrics() const;
    
    std::map<std::string, TenantStats> getAllTenantStats() const;
    
    /**
     * @brief Get Active Tenants.
     * @return Return value.
     */
    std::vector<std::string> getActiveTenants() const;
    
    /**
     * @brief Has Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool hasTenant(const std::string& tenant_id) const;
    
    /**
     * @brief Disable Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void disableTenant(const std::string& tenant_id);
    
    /**
     * @brief Enable Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void enableTenant(const std::string& tenant_id);
    
    /**
     * @brief Remove Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void removeTenant(const std::string& tenant_id);

private:
    // Per-tenant buffer state
    struct TenantBufferState {
        std::unique_ptr<ChangefeedBuffer> buffer;
        TenantConfig config;
        TenantStats stats;
        std::chrono::steady_clock::time_point last_event_time;
        std::atomic<bool> enabled{true};
    };
    
    Changefeed* changefeed_;
    ChangefeedBufferConfig default_config_;
    
    // Tenant buffers: tenant_id -> buffer state
    std::map<std::string, TenantBufferState> tenant_buffers_;
    mutable std::mutex buffers_mutex_;
    
    // Manager state
    std::atomic<bool> running_{false};
    
    // Helper methods
    /**
     * @brief Get Or Create Tenant Buffer.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    TenantBufferState& getOrCreateTenantBuffer(const std::string& tenant_id);
    /**
     * @brief Check Tenant Quota.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in,out] state Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool checkTenantQuota(const std::string& tenant_id, TenantBufferState& state);
    /**
     * @brief Update Tenant Stats.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in,out] state Input/output parameter.
     */
    void updateTenantStats(const std::string& tenant_id, TenantBufferState& state);
};

} // namespace cdc
} // namespace themis
