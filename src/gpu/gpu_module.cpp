/**
 * @file gpu_module.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/gpu_module.h"

#include "themis/gpu/feature_flags.h"
#include "themis/gpu/rocm_backend.h"
#include "themis/gpu/vulkan_backend.h"
#include "observability/trace_instrumentation.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <utility>

namespace themis {
namespace gpu {
namespace {

const char* executionStatusName(GPUModule::SubmitResult::ExecutionStatus status) {
    switch (status) {
        case GPUModule::SubmitResult::ExecutionStatus::Ok:
            return "ok";
        case GPUModule::SubmitResult::ExecutionStatus::OkDegraded:
            return "ok_degraded";
        case GPUModule::SubmitResult::ExecutionStatus::Failed:
            return "failed";
    }
    return "failed";
}

const char* executionModeName(GPUModule::SubmitResult::ExecutionMode mode) {
    switch (mode) {
        case GPUModule::SubmitResult::ExecutionMode::None:
            return "none";
        case GPUModule::SubmitResult::ExecutionMode::Gpu:
            return "gpu";
        case GPUModule::SubmitResult::ExecutionMode::CpuFallback:
            return "cpu_fallback";
    }
    return "none";
}

/**
 * @brief Create Default Launcher Backend.
 * @return Return value.
 * @details Calls: ROCmBackend::GetInstance(), isAvailable(), THEMIS_INFO(), createBackendFn(), VulkanComputeBackend::GetInstance(), THEMIS_WARN().
 */
GPULauncher::BackendFn createDefaultLauncherBackend() {
    auto& rocm_backend = ROCmBackend::GetInstance();
    if (rocm_backend.isAvailable()) {
        THEMIS_INFO("GPUModule: using ROCm backend for launcher dispatch");
        return rocm_backend.createBackendFn();
    }

    auto& vulkan_backend = VulkanComputeBackend::GetInstance();
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::VULKAN_BACKEND) &&
        vulkan_backend.isAvailable()) {
        THEMIS_INFO("GPUModule: using Vulkan backend for launcher dispatch");
        return vulkan_backend.createBackendFn();
    }

    THEMIS_WARN("GPUModule: no enabled GPU backend is available");
    return nullptr;
}

} // namespace

std::string GPUModule::SubmitResult::serialize() const {
    return nlohmann::json{
        {"status", executionStatusName(status)},
        {"mode", executionModeName(mode)},
        {"submitted", submitted},
        {"via_gpu", via_gpu},
        {"error", error},
    }.dump();
}

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * @brief Initialize.
 * @param[in] config Input parameter.
 * @param[in] backend Input parameter.
 * @return Return value.
 * @details Calls: validate(), empty(), front(), reset(), std::chrono::seconds(), GPUFeatureFlags::GetInstance(), isEnabled(), createDefaultLauncherBackend().
 */
GPUModule::InitResult GPUModule::initialize(const GPUConfig &config, GPULauncher::BackendFn backend,
                                           GPULauncher::BackendFn cpu_fallback_backend) {
    auto vr = config.validate();
    if (!vr.ok) {
        InitResult r;
        r.ok    = false;
        r.error = vr.errors.empty() ? "invalid config" : vr.errors.front();
        return r;
    }

    config_ = config;
    cpu_fallback_backend_ = std::move(cpu_fallback_backend);
    launcher_.reset();
    safe_fail_.reset([&]() {
        GPUSafeFail::Config c;
        c.failure_threshold     = static_cast<size_t>(config.circuit_failure_threshold);
        c.success_threshold     = static_cast<size_t>(config.circuit_success_threshold);
        c.circuit_reset_timeout = std::chrono::seconds(config.circuit_reset_timeout_secs);
        c.enable_cpu_fallback   = config.enable_cpu_fallback;
        return c;
    }());

    // Launcher is gated by the ASYNC_LAUNCHER feature flag.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::ASYNC_LAUNCHER)) {
        auto selected_backend = backend ? std::move(backend) : createDefaultLauncherBackend();
        if (selected_backend) {
            launcher_ = std::make_unique<GPULauncher>(std::move(selected_backend));
        }
    }

    initialized_ = true;
    return {true, ""};
}

// ============================================================================
// Work submission
// ============================================================================

/**
 * @brief Submit Work.
 * @param[in] caller_id Identifier of the caller.
 * @param[in] tenant_id Identifier of the tenant.
 * @param[in] item Input parameter.
 * @return Return value.
 * @details Calls: GPUFeatureFlags::GetInstance(), isEnabled(), check(), record(), GPUMetrics::GetInstance(), recordAllocFailGlobal(), submit(), get().
 */
GPUModule::SubmitResult GPUModule::submitWork(const std::string &caller_id, const std::string &tenant_id,
                                              const GPULauncher::WorkItem &item) {
    auto record_dispatch_result = [&](const SubmitResult& result) {
        if (auto* span = themis::observability::getCurrentSpan()) {
            span->setAttribute("gpu.execution.status", executionStatusName(result.status));
            span->setAttribute("gpu.execution.mode", executionModeName(result.mode));
        }
        if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
            GPUMetrics::GetInstance().recordDispatchResult(executionStatusName(result.status),
                                                          executionModeName(result.mode));
        }
        if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
            audit_log_.record(GPUAuditLog::EventType::DISPATCH_RESULT, 0, "gpu_dispatch", tenant_id,
                              std::string("status=") + executionStatusName(result.status) +
                                  ";mode=" + executionModeName(result.mode));
        }
    };

    if (!initialized_) {
        SubmitResult result;
        result.error = "GPUModule not initialized";
        record_dispatch_result(result);
        return result;
    }

    // 1. Policy check.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::POLICY_GATE)) {
        auto decision = policy_.check(caller_id, GPUPolicy::Capability::GPU_ALLOCATE);
        if (!decision.allowed) {
            if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
                audit_log_.record(GPUAuditLog::EventType::ALLOC_FAIL_GLOBAL_LIMIT, 0, caller_id, tenant_id,
                                  "policy denied: " + decision.reason);
            }
            if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
                GPUMetrics::GetInstance().recordAllocFailGlobal(0, tenant_id);
            }
            SubmitResult result;
            result.error = "policy denied: " + decision.reason;
            record_dispatch_result(result);
            return result;
        }
    }

    // 2. Circuit-breaker / safe-fail dispatch.
    bool used_gpu = false;
    bool gpu_attempted = false;
    bool fallback_attempted = false;
    bool success = false;
    std::string execution_error;

    auto gpu_op = [&]() -> bool {
        if (!launcher_) {
            execution_error = "GPU launcher unavailable";
            return false;
        }
        gpu_attempted = true;
        try {
            auto fut = launcher_->submit(item);
            auto res = fut.get();
            used_gpu = res.success;
            if (!res.success) {
                execution_error = res.error_message.empty() ? "GPU backend failed" : res.error_message;
            }
            return res.success;
        } catch (...) {
            execution_error = "GPU backend execution failed";
            return false;
        }
    };

    auto cpu_fallback = [&]() -> bool {
        used_gpu = false;
        fallback_attempted = true;
        if (!cpu_fallback_backend_) {
            execution_error = "CPU fallback backend unavailable";
            return false;
        }
        try {
            if (cpu_fallback_backend_(item)) {
                execution_error.clear();
                if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
                    audit_log_.recordFallbackToCPU("gpu_dispatch_failed", tenant_id);
                }
                if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
                    GPUMetrics::GetInstance().recordFallback("cpu_fallback");
                }
                return true;
            }
            execution_error = "CPU fallback backend failed";
        } catch (...) {
            execution_error = "CPU fallback backend threw";
        }
        return false;
    };

    success = safe_fail_.executeWithFallback(gpu_op, cpu_fallback, item.kernel_id);

    SubmitResult result;
    result.submitted = success;
    result.via_gpu = used_gpu;
    if (success && used_gpu) {
        result.status = SubmitResult::ExecutionStatus::Ok;
        result.mode = SubmitResult::ExecutionMode::Gpu;
    } else if (success) {
        result.status = SubmitResult::ExecutionStatus::OkDegraded;
        result.mode = SubmitResult::ExecutionMode::CpuFallback;
    } else {
        result.status = SubmitResult::ExecutionStatus::Failed;
        result.mode = fallback_attempted ? SubmitResult::ExecutionMode::CpuFallback
                                         : gpu_attempted ? SubmitResult::ExecutionMode::Gpu
                                                         : SubmitResult::ExecutionMode::None;
        result.error = execution_error.empty() ? "operation failed" : execution_error;
    }
    record_dispatch_result(result);
    return result;
}

// ============================================================================
// Inline VRAM management
// ============================================================================

/**
 * @brief Allocate.
 * @param[in] caller_id Identifier of the caller.
 * @param[in] tenant_id Identifier of the tenant.
 * @param[in] bytes Input parameter.
 * @param[in] tag Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: GPUFeatureFlags::GetInstance(), isEnabled(), isAllowed(), record(), GPUMemoryManager::GetInstance(), empty(), TryAllocateGPU(), GPUMetrics::GetInstance().
 */
bool GPUModule::allocate(const std::string &caller_id, const std::string &tenant_id, uint64_t bytes,
                         const std::string &tag) {
    if (!initialized_) {
        return false;
    }

    // Policy check.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::POLICY_GATE)) {
        if (!policy_.isAllowed(caller_id, GPUPolicy::Capability::GPU_ALLOCATE)) {
            if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
                audit_log_.record(GPUAuditLog::EventType::ALLOC_FAIL_GLOBAL_LIMIT, bytes, tag, tenant_id,
                                  "policy denied");
            }
            return false;
        }
    }

    // VRAM allocation.
    auto &mgr          = GPUMemoryManager::GetInstance();
    const bool granted = tenant_id.empty() ? mgr.TryAllocateGPU(bytes, tag) : mgr.TryAllocateGPU(bytes, tag, tenant_id);

    // Metrics.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
        auto &met = GPUMetrics::GetInstance();
        if (granted) {
            met.recordAllocSuccess(bytes, tenant_id);
            met.setVRAMAllocated(mgr.GetGPUMemoryUsed(), tenant_id);
            met.setVRAMPeak(mgr.GetStats().peak_bytes);
        } else {
            met.recordAllocFailGlobal(bytes, tenant_id);
        }
    }

    // Audit.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
        if (granted) {
            audit_log_.recordAllocSuccess(bytes, tag, tenant_id);
        } else {
            audit_log_.recordAllocFailGlobalLimit(bytes, tag, tenant_id);
        }
    }

    return granted;
}

/**
 * @brief Deallocate.
 * @param[in] tenant_id Identifier of the tenant.
 * @param[in] bytes Input parameter.
 * @details Calls: GPUMemoryManager::GetInstance(), empty(), DeallocateGPU(), GPUFeatureFlags::GetInstance(), isEnabled(), GPUMetrics::GetInstance(), recordDealloc(), setVRAMAllocated().
 */
void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
    if (!initialized_) {
        return;
    }

    auto &mgr = GPUMemoryManager::GetInstance();
    if (tenant_id.empty()) {
        mgr.DeallocateGPU(bytes);
    } else {
        mgr.DeallocateGPU(bytes, tenant_id);
    }

    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
        auto &met = GPUMetrics::GetInstance();
        met.recordDealloc(bytes, tenant_id);
        met.setVRAMAllocated(mgr.GetGPUMemoryUsed(), tenant_id);
    }

    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
        audit_log_.recordDealloc(bytes, "gpu_module", tenant_id);
    }
}

// ============================================================================
// Policy delegation
// ============================================================================

/**
 * @brief Grant Caller.
 * @param[in] caller_id Identifier of the caller.
 * @param[in] cap Input parameter.
 * @details Calls: grant().
 */
void GPUModule::grantCaller(const std::string &caller_id, GPUPolicy::Capability cap) {
    policy_.grant(caller_id, cap);
}

/**
 * @brief Revoke Caller.
 * @param[in] caller_id Identifier of the caller.
 * @details Calls: revokeAll().
 */
void GPUModule::revokeCaller(const std::string &caller_id) {
    policy_.revokeAll(caller_id);
}

// ============================================================================
// Diagnostics
// ============================================================================

GPUMemoryManager::Stats GPUModule::getMemoryStats() const {
    return GPUMemoryManager::GetInstance().GetStats();
}

GPUSafeFail::HealthStatus GPUModule::getSafeFailStatus() const {
    return safe_fail_.getStatus();
}

std::vector<GPUAuditLog::Event> GPUModule::getAuditLog(size_t last_n) const {
    auto all = audit_log_.snapshot();
    if (all.size() > last_n) {
        all.erase(all.begin(), all.begin() + static_cast<ptrdiff_t>(all.size() - last_n));
    }
    return all;
}

} // namespace gpu
} // namespace themis
