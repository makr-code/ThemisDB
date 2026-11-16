/**
 * @file gpu_module.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include "themis/gpu/audit_log.h"
#include "themis/gpu/config.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/launcher.h"
#include "themis/gpu/memory_manager.h"
#include "themis/gpu/metrics.h"
#include "themis/gpu/mig_manager.h"
#include "themis/gpu/policy.h"
#include "themis/gpu/safe_fail.h"

namespace themis {
namespace gpu {

/**
 * @brief Top-level GPU module integration facade.
 *
 * `GPUModule` ties together all GPU sub-systems into a single, coherent entry
 * point so callers never need to orchestrate policy, safe-fail, metrics, and
 * audit-log themselves.
 *
 * Typical allocation flow
 * -----------------------
 * 1. `initialize(config)` — validate config, wire up components.
 * 2. `submitWork(caller_id, tenant_id, item)` — policy check → circuit
 *    check → VRAM alloc → launch → metrics + audit.
 * 3. `allocate(caller_id, tenant_id, bytes, tag)` — inline VRAM reservation
 *    without launching GPU work (e.g. for tensor buffer pre-allocation).
 * 4. `deallocate(tenant_id, bytes)` — release VRAM, update metrics.
 *
 * Design notes
 * ------------
 * - GPUModule is a value object; own instances are used in tests while the
 *   singleton `GPUModule::GetInstance()` is used in production code.
 * - Sub-system singletons (GPUMemoryManager, GPUMetrics) are referenced via
 *   their own `GetInstance()` calls so the facade does not duplicate state.
 * - Feature flags gate each sub-system at runtime via `GPUFeatureFlags`.
 *
 * Thread safety: all public methods are thread-safe.
 */
class GPUModule {
public:
  GPUModule() = default;

    // -----------------------------------------------------------------------
    // Initialisation result
    // -----------------------------------------------------------------------
    struct InitResult {
        bool        ok = false;
        std::string error;
    };

    // -----------------------------------------------------------------------
    // Submit result
    // -----------------------------------------------------------------------
    struct SubmitResult {
        /** @brief Whether dispatch completed successfully or via CPU degradation. */
        enum class ExecutionStatus {
            Ok,         ///< GPU completed the work.
            OkDegraded, ///< The supplied CPU fallback completed the work.
            Failed      ///< Neither execution path completed the work.
        };

        /** @brief Execution path that produced or attempted the result. */
        enum class ExecutionMode {
            None,        ///< No execution path was entered.
            Gpu,         ///< GPU dispatch was entered.
            CpuFallback  ///< CPU fallback was entered.
        };

        bool        submitted   = false;   ///< True only when the work completed.
        bool        via_gpu     = false;   ///< True only when GPU execution completed.
        std::string error;
        ExecutionStatus status = ExecutionStatus::Failed; ///< Outcome classification.
        ExecutionMode mode = ExecutionMode::None;          ///< Execution path classification.

        /**
         * @brief Serialize dispatch outcome for API and trace consumers.
         * @return JSON with status, mode, legacy booleans, and error text.
         */
        [[nodiscard]] std::string serialize() const;
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    /**
     * @brief Return the process-wide GPU module instance.
     * @details Function-local static initialization is thread-safe.
     * @return Reference to the function-local singleton.
     */
    static GPUModule& GetInstance() {
        static GPUModule inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Validate @p config and wire up internal components.
     *
     * Safe to call multiple times; re-initialises the module in place.
     * Returns an error result if the config is invalid. The optional CPU
     * backend is invoked only after GPU dispatch is unavailable or fails;
     * without it, fallback is reported as failed rather than successful.
     *
     * @param config Validated resource and circuit-breaker configuration.
     * @param backend Optional GPU backend override; an empty callback selects
     *        the configured default backend. If no enabled hardware backend
     *        is available, GPU dispatch remains unavailable and the explicit
     *        CPU fallback callback is used by submitWork.
     * @param cpu_fallback_backend Optional CPU implementation for the same work
     *        item; it must return true only after completing the work.
     * @return Initialization status and validation error, if any.
     */
    InitResult initialize(const GPUConfig& config,
                          GPULauncher::BackendFn backend = nullptr,
                          GPULauncher::BackendFn cpu_fallback_backend = nullptr);

    /**
     * @brief Return true if the module has been successfully initialised.
     */
    bool isInitialized() const noexcept { return initialized_; }

    // -----------------------------------------------------------------------
    // Work submission
    // -----------------------------------------------------------------------

    /**
     * @brief Submit a GPU work item.
     *
     * Checks policy and safe-fail state, then dispatches through the GPU
     * launcher or the supplied CPU fallback. A fallback is not successful
     * unless its callback reports that it completed the work. Records
     * status/mode metrics and an audit event when those features are enabled.
     *
     * @param caller_id  Identity of the submitting component (policy check).
     * @param tenant_id  Tenant for VRAM quota enforcement.
     * @param item       Work item (kernel_id, args, timeout).
     * @return SubmitResult distinguishing GPU success, successful CPU
     *         degradation, and failure. `mode` records the execution path;
     *         `via_gpu` remains available for existing callers.
     */
    SubmitResult submitWork(const std::string&      caller_id,
                            const std::string&      tenant_id,
                            const GPULauncher::WorkItem& item);

    // -----------------------------------------------------------------------
    // Inline VRAM management
    // -----------------------------------------------------------------------

    /**
     * @brief Reserve @p bytes of VRAM for @p caller_id / @p tenant_id.
     *
     * Checks policy (GPU_ALLOCATE), tenant quota, and edition limit.
     * Records metrics and an audit event on success or failure.
     *
     * @return true if the allocation was granted.
     */
    bool allocate(const std::string& caller_id,
                  const std::string& tenant_id,
                  uint64_t           bytes,
                  const std::string& tag = "gpu_module");

    /**
     * @brief Release @p bytes of VRAM previously granted to @p tenant_id.
     *
     * Records a dealloc metric and audit event.
     *
     * @param tenant_id Tenant that owns the allocation, or empty for a
     *        non-tenant allocation.
     * @param bytes Number of bytes to release.
     */
    void deallocate(const std::string& tenant_id, uint64_t bytes);

    // -----------------------------------------------------------------------
    // Policy management (delegates to internal GPUPolicy)
    // -----------------------------------------------------------------------
    /**
     * @brief Grant a caller the requested GPU capability.
     * @param caller_id Identity of the caller receiving permission.
     * @param cap Capability to grant; defaults to all GPU operations.
     */
    void grantCaller(const std::string& caller_id,
                     GPUPolicy::Capability cap = GPUPolicy::Capability::GPU_ANY);
    /**
     * @brief Revoke all GPU capabilities previously granted to a caller.
     * @param caller_id Identity of the caller whose permissions are removed.
     */
    void revokeCaller(const std::string& caller_id);

    // -----------------------------------------------------------------------
    // MIG (Multi-Instance GPU) management
    // -----------------------------------------------------------------------

    /**
     * @brief Access the process-wide MIG partition manager.
     *
     * Returns the `MIGManager` singleton so callers can create/destroy
     * MIG partitions, assign them to tenants, and query their status without
     * importing `mig_manager.h` directly.
     *
     * The `MIG_MANAGER` feature flag is enforced inside `MIGManager`:
     * mutating operations (`createPartition`, `destroyPartition`) return
     * `Status::MIG_FEATURE_DISABLED` when the flag is off; read-only
     * queries (`getInstances`, `getInstance`, etc.) are always permitted.
     * Callers can check `GPUFeatureFlags::GetInstance().isEnabled(
     * GPUFeatureFlags::Feature::MIG_MANAGER)` before calling mutating
     * operations if they want to short-circuit early.
     *
     * @details The manager applies the MIG feature gate to mutating operations.
     * @return Mutable reference to the process-wide MIG manager.
     */
    MIGManager& mig() noexcept { return MIGManager::GetInstance(); }
    /**
     * @brief Access the process-wide MIG partition manager as read-only.
     * @details The manager applies the MIG feature gate to mutating operations.
     * @return Const reference to the process-wide MIG manager.
     */
    const MIGManager& mig() const noexcept { return MIGManager::GetInstance(); }

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------
    GPUMemoryManager::Stats   getMemoryStats()  const;
    GPUSafeFail::HealthStatus getSafeFailStatus() const;
    std::vector<GPUAuditLog::Event> getAuditLog(size_t last_n = 64) const;

private:
    bool         initialized_ = false;
    GPUConfig    config_;
    GPUPolicy    policy_;
    GPUSafeFail  safe_fail_;
    GPUAuditLog  audit_log_;

    std::unique_ptr<GPULauncher> launcher_;
    GPULauncher::BackendFn cpu_fallback_backend_;
};

} // namespace gpu
} // namespace themis
