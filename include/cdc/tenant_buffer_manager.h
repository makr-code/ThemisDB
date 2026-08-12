/**
 * @file tenant_buffer_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Per-tenant configuration and quotas
 */
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

/**
 * @brief Per-tenant statistics
 */
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

/**
 * @brief Manages per-tenant CDC buffers with isolation
 */
class TenantBufferManager {
public:
    /**
     * @brief Constructor
     * @param changefeed Shared changefeed instance for all tenants
     * @param default_config Default configuration for new tenants
     */
    TenantBufferManager(Changefeed* changefeed, 
                       const ChangefeedBufferConfig& default_config = ChangefeedBufferConfig());
    
    /**
     * @brief Destructor - stops all tenant buffers and suppresses shutdown exceptions.
     */
    ~TenantBufferManager() noexcept;
    
    /**
     * @brief Start buffer management
     */
    void start();
    
    /**
     * @brief Stop all tenant buffers
     */
    void stop();
    
    /**
     * @brief Record event for specific tenant
     * @param tenant_id Tenant identifier
     * @param event Event to record
     * @return Recorded event with sequence
     * @throws CDCException if tenant disabled or quota exceeded
     */
    Changefeed::ChangeEvent recordEvent(const std::string& tenant_id,
                                       Changefeed::ChangeEvent event);
    
    /**
     * @brief Flush buffers for specific tenant
     * @param tenant_id Tenant identifier
     * @return Number of events flushed
     */
    size_t flushTenant(const std::string& tenant_id);
    
    /**
     * @brief Flush all tenant buffers
     * @return Total events flushed across all tenants
     */
    size_t flushAll();
    
    /**
     * @brief Configure specific tenant
     * @param config Tenant configuration
     */
    void configureTenant(const TenantConfig& config);
    
    /**
     * @brief Get tenant configuration
     * @param tenant_id Tenant identifier
     * @return Tenant config if exists
     */
    std::optional<TenantConfig> getTenantConfig(const std::string& tenant_id) const;
    
    /**
     * @brief Get tenant statistics
     * @param tenant_id Tenant identifier
     * @return Tenant stats if exists
     */
    std::optional<TenantStats> getTenantStats(const std::string& tenant_id) const;
    
    /**
     * @brief Get metrics for specific tenant
     * @param tenant_id Tenant identifier
     * @return Tenant metrics if exists
     */
    std::optional<std::reference_wrapper<const CDCMetrics>> getTenantMetrics(const std::string& tenant_id) const;
    
    /**
     * @brief Get aggregated metrics across all tenants
        * @return Aggregated metrics snapshot
     */
        nlohmann::json getGlobalMetrics() const;
    
    /**
     * @brief Get all tenant statistics
     * @return Map of tenant_id -> TenantStats
     */
    std::map<std::string, TenantStats> getAllTenantStats() const;
    
    /**
     * @brief Get list of active tenants
     * @return Vector of tenant IDs
     */
    std::vector<std::string> getActiveTenants() const;
    
    /**
     * @brief Check if tenant exists
     * @param tenant_id Tenant identifier
     * @return True if tenant has buffer
     */
    bool hasTenant(const std::string& tenant_id) const;
    
    /**
     * @brief Disable tenant (stop accepting events)
     * @param tenant_id Tenant identifier
     */
    void disableTenant(const std::string& tenant_id);
    
    /**
     * @brief Enable tenant (resume accepting events)
     * @param tenant_id Tenant identifier
     */
    void enableTenant(const std::string& tenant_id);
    
    /**
     * @brief Remove tenant buffer (flush first)
     * @param tenant_id Tenant identifier
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
    TenantBufferState& getOrCreateTenantBuffer(const std::string& tenant_id);
    bool checkTenantQuota(const std::string& tenant_id, TenantBufferState& state);
    void updateTenantStats(const std::string& tenant_id, TenantBufferState& state);
};

} // namespace cdc
} // namespace themis
