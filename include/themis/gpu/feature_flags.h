/**
 * @file feature_flags.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * | WASM_SANDBOX     |    no     |    yes     |    yes      |
 * | MIG_MANAGER      |    no     |    yes     |    yes      |
 * | VULKAN_BACKEND   |    yes    |    yes     |    yes      |
 * | PEER_TO_PEER     |    no     |    yes     |    yes      |
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
        WASM_SANDBOX,      ///< WASM-based kernel sandbox for untrusted third-party kernels
        MIG_MANAGER,       ///< MIG (Multi-Instance GPU) partitioning for NVIDIA A/H series
        VULKAN_BACKEND,    ///< Vulkan compute backend for cross-vendor GPU support
        PEER_TO_PEER,      ///< Peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe)
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

