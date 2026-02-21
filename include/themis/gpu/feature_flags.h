/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feature_flags.h                                    ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     162                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/edition.h"

namespace themis {
namespace gpu {

/**
 * @brief Per-edition GPU feature flag registry.
 *
 * Controls which GPU capabilities are active at runtime.  Feature enablement
 * is initialised from the compiled edition but can be overridden at startup
 * (e.g. via config file or environment variable) to allow staged rollouts and
 * emergency disablement without a recompile.
 *
 * Default enablement per edition
 * --------------------------------
 * | Feature          | COMMUNITY | ENTERPRISE | HYPERSCALER |
 * |------------------|-----------|------------|-------------|
 * | MEMORY_POOL      |    yes    |    yes     |    yes      |
 * | ASYNC_LAUNCHER   |    yes    |    yes     |    yes      |
 * | MULTI_GPU        |    no     |    yes     |    yes      |
 * | TENSOR_OPS       |    no     |    yes     |    yes      |
 * | POLICY_GATE      |    yes    |    yes     |    yes      |
 * | AUDIT_LOG        |    yes    |    yes     |    yes      |
 * | METRICS          |    yes    |    yes     |    yes      |
 * | LOAD_BALANCER    |    no     |    yes     |    yes      |
 * | KERNEL_VALIDATOR |    yes    |    yes     |    yes      |
 * | ALERTS           |    yes    |    yes     |    yes      |
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUFeatureFlags {
public:
    // -----------------------------------------------------------------------
    // Feature enumeration
    // -----------------------------------------------------------------------
    enum class Feature {
        MEMORY_POOL,       ///< Slab-based pre-allocator (GPUMemoryPool)
        ASYNC_LAUNCHER,    ///< Async work-item launcher (GPULauncher)
        MULTI_GPU,         ///< Multi-GPU device management (GPULoadBalancer)
        TENSOR_OPS,        ///< GPU tensor kernel operations (future)
        POLICY_GATE,       ///< Default-deny capability gate (GPUPolicy)
        AUDIT_LOG,         ///< Ring-buffer audit event log (GPUAuditLog)
        METRICS,           ///< Prometheus-compatible metrics (GPUMetrics)
        LOAD_BALANCER,     ///< Load balancing across devices (GPULoadBalancer)
        KERNEL_VALIDATOR,  ///< Kernel checksum/signature validation
        ALERTS,            ///< Threshold-based alert manager (GPUAlertManager)
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUFeatureFlags& GetInstance() {
        static GPUFeatureFlags inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief Return true if @p feature is currently enabled.
     */
    bool isEnabled(Feature feature) const;

    /**
     * @brief Return the feature name as a string (for logging).
     */
    static const char* featureName(Feature feature) noexcept;

    // -----------------------------------------------------------------------
    // Override (startup / config / test)
    // -----------------------------------------------------------------------

    /**
     * @brief Explicitly enable a feature, overriding the edition default.
     */
    void enable(Feature feature);

    /**
     * @brief Explicitly disable a feature, overriding the edition default.
     */
    void disable(Feature feature);

    /**
     * @brief Reset all overrides; features revert to their edition defaults.
     */
    void resetToDefaults();

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return a list of all features and their current enabled state.
     */
    struct FeatureStatus {
        Feature     feature;
        std::string name;
        bool        enabled    = false;
        bool        overridden = false;  ///< true = explicitly set, not edition default
    };

    std::vector<FeatureStatus> getAll() const;

    /**
     * @brief Return the human-readable edition name.
     */
    static std::string editionName();

private:
    GPUFeatureFlags();

    mutable std::mutex mutex_;

    // Edition defaults: computed once at construction from THEMIS_EDITION.
    std::unordered_map<int, bool> defaults_;

    // Runtime overrides (key = Feature as int, value = enabled/disabled).
    // Only entries that were explicitly set via enable()/disable() appear here.
    std::unordered_map<int, bool> overrides_;

    static int key(Feature f) noexcept { return static_cast<int>(f); }

    void initDefaults();
    static bool editionDefaultFor(Feature f);
};

} // namespace gpu
} // namespace themis
