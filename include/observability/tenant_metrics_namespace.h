/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tenant_metrics_namespace.h                         ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:03:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     296                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 08b62168b2  2026-04-12  feat(observability): add per-tenant metric namespacing an... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tenant_metrics_namespace.h
 * @brief Per-tenant metric namespacing for ThemisDB observability.
 *
 * `TenantMetricsNamespace` provides isolated metric recording per tenant,
 * preventing cross-tenant cardinality leakage via shared label sets.
 *
 * ## Design Goals
 * - Each tenant gets its own cardinality budget enforced independently.
 * - Metric names are automatically prefixed: `themis_<tenant_id>_<metric>`.
 * - Label sets are scoped to the tenant; a label key/value from tenant A
 *   can never inflate the cardinality count of tenant B.
 * - Thread-safe: all methods acquire an internal shared/unique mutex.
 *
 * ## Usage
 * ```cpp
 * TenantMetricsConfig cfg;
 * cfg.cardinality_limit_per_tenant = 200;
 *
 * TenantMetricsNamespace registry(cfg);
 * registry.registerTenant("acme");
 *
 * registry.increment("acme", "query_total", {{"type", "select"}});
 * registry.setGauge("acme",  "active_connections", 12.0);
 * registry.observeHistogram("acme", "query_latency_ms", 42.5);
 *
 * std::string prom_text = registry.exportTenant("acme");
 * std::string all_text  = registry.exportAll();
 * ```
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for TenantMetricsNamespace.
 */
struct TenantMetricsConfig {
    /// Maximum number of unique label-set combinations per metric *per tenant*.
    /// 0 = unlimited (not recommended in production).
    size_t cardinality_limit_per_tenant{100};

    /// Maximum number of tenants that can be registered simultaneously.
    /// 0 = unlimited.
    size_t max_tenants{0};

    /// When true, metric observations for unknown (unregistered) tenants are
    /// silently dropped instead of auto-registering the tenant.
    bool strict_tenant_registration{false};
};

// ---------------------------------------------------------------------------
// Per-tenant statistics
// ---------------------------------------------------------------------------

/**
 * @brief Runtime statistics for a single tenant.
 */
struct TenantMetricsStats {
    /// Tenant identifier.
    std::string tenant_id;
    /// Total number of metric observations recorded (all types, all series).
    int64_t total_observations{0};
    /// Number of observations dropped because the per-tenant cardinality
    /// limit was reached.
    int64_t dropped_observations{0};
    /// Number of distinct metric series active for this tenant.
    size_t active_series{0};
};

// ---------------------------------------------------------------------------
// TenantMetricsNamespace
// ---------------------------------------------------------------------------

/**
 * @brief Registry that provides per-tenant metric isolation.
 *
 * Internally each tenant owns an independent set of counters, gauges and
 * histogram samples.  Prometheus export prefixes every metric name with
 * `themis_<tenant_id>_` and automatically injects `tenant_id="<id>"` into
 * every label set, so per-tenant and cross-tenant aggregations are both
 * possible from a single scrape endpoint.
 *
 * Cross-tenant cardinality isolation is enforced at the series level: the
 * cardinality limit applies independently per tenant — a single high-
 * cardinality tenant cannot starve other tenants.
 */
class TenantMetricsNamespace {
public:
    explicit TenantMetricsNamespace(
        const TenantMetricsConfig& config = TenantMetricsConfig{});
    ~TenantMetricsNamespace();

    // Non-copyable
    TenantMetricsNamespace(const TenantMetricsNamespace&) = delete;
    TenantMetricsNamespace& operator=(const TenantMetricsNamespace&) = delete;

    // -----------------------------------------------------------------------
    // Tenant lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new tenant.
     *
     * Idempotent — calling with an already-registered tenant_id is a no-op.
     * @returns true if the tenant was newly registered, false if it already
     *          existed or could not be added (max_tenants exceeded).
     */
    bool registerTenant(const std::string& tenant_id);

    /**
     * @brief Deregister a tenant and free all its metric data.
     * @returns true if the tenant existed and was removed.
     */
    bool deregisterTenant(const std::string& tenant_id);

    /** @returns true when the given tenant is currently registered. */
    bool hasTenant(const std::string& tenant_id) const;

    /** @returns the list of all currently registered tenant IDs. */
    std::vector<std::string> tenants() const;

    /** @returns the number of registered tenants. */
    size_t tenantCount() const;

    // -----------------------------------------------------------------------
    // Metric recording
    // -----------------------------------------------------------------------

    /**
     * @brief Increment a counter for the given tenant by 1.
     *
     * In non-strict mode an unknown @p tenant_id is auto-registered before
     * recording the observation.
     */
    void increment(const std::string& tenant_id,
                   const std::string& metric_name,
                   const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Set a gauge to an absolute value for the given tenant.
     */
    void setGauge(const std::string& tenant_id,
                  const std::string& metric_name,
                  double value,
                  const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Record a histogram observation for the given tenant.
     */
    void observeHistogram(const std::string& tenant_id,
                          const std::string& metric_name,
                          double value,
                          const std::map<std::string, std::string>& labels = {});

    // -----------------------------------------------------------------------
    // Export
    // -----------------------------------------------------------------------

    /**
     * @brief Export Prometheus text-format metrics for a single tenant.
     *
     * Every metric name is prefixed with `themis_<tenant_id>_` and every
     * label set includes `tenant_id="<id>"`.
     *
     * @returns Prometheus text or an empty string if the tenant is unknown.
     */
    std::string exportTenant(const std::string& tenant_id) const;

    /**
     * @brief Export Prometheus text-format metrics for all registered tenants.
     *
     * Equivalent to concatenating exportTenant() for every registered tenant.
     */
    std::string exportAll() const;

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /** @returns runtime statistics for the given tenant, or a zeroed struct if unknown. */
    TenantMetricsStats stats(const std::string& tenant_id) const;

    /** @returns runtime statistics for all registered tenants. */
    std::vector<TenantMetricsStats> allStats() const;

    /** @returns the active configuration. */
    TenantMetricsConfig config() const;

    /** @brief Remove all metrics data for all tenants (useful in tests). */
    void reset();

private:
    // -----------------------------------------------------------------------
    // Internal per-tenant store
    // -----------------------------------------------------------------------

    struct TenantStore {
        std::string tenant_id;

        // Counters
        std::map<std::string, int64_t> counters;

        // Gauges
        std::map<std::string, double> gauges;

        // Histograms (raw samples, capped to avoid unbounded growth)
        struct HistogramData {
            std::vector<double> samples;
            static constexpr size_t kMaxSamples = 1000;
        };
        std::map<std::string, HistogramData> histograms;

        // Series cardinality tracking: metric_name -> set of serialised label keys
        std::map<std::string, size_t> series_count;

        // Per-tenant stats
        std::atomic<int64_t> total_observations{0};
        std::atomic<int64_t> dropped_observations{0};

        TenantStore() = default;
        TenantStore(const TenantStore&) = delete;
        TenantStore& operator=(const TenantStore&) = delete;
    };

    /// Make a fully qualified metric key from a metric name and label map.
    static std::string makeKey(const std::string& metric_name,
                                const std::map<std::string, std::string>& labels);

    /// Check and enforce per-tenant cardinality.  Returns true when the
    /// observation should proceed, false when it must be dropped.
    bool checkCardinality(TenantStore& store,
                          const std::string& metric_name,
                          const std::string& key) const;

    /// Format a single Prometheus metric line.
    static std::string formatLine(const std::string& prefixed_name,
                                  const std::map<std::string, std::string>& labels,
                                  double value);

    /// Export all metrics from a single TenantStore.
    std::string exportStore(const TenantStore& store) const;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    TenantMetricsConfig config_;
    mutable std::shared_mutex mutex_;

    /// tenant_id → TenantStore (owned via unique_ptr for stable address)
    std::unordered_map<std::string, std::unique_ptr<TenantStore>> stores_;
};

} // namespace observability
} // namespace themis
