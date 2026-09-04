/**
 * @file gpu_module.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/gpu_module.h"

#include "themis/gpu/feature_flags.h"
#include "themis/gpu/rocm_backend.h"
#include "themis/gpu/vulkan_backend.h"
#include "utils/logger.h"

namespace themis {
namespace gpu {
namespace {

GPULauncher::BackendFn createDefaultLauncherBackend() {
    auto& rocm_backend = ROCmBackend::GetInstance();
    if (rocm_backend.isAvailable()) {
        THEMIS_INFO("GPUModule: using ROCm backend for launcher dispatch");
        return rocm_backend.createBackendFn();
    }

    auto& vulkan_backend = VulkanComputeBackend::GetInstance();
    if (vulkan_backend.isAvailable()) {
        THEMIS_INFO("GPUModule: using Vulkan backend for launcher dispatch");
        return vulkan_backend.createBackendFn();
    }

    THEMIS_WARN("GPUModule: no GPU runtime available; using CPU fallback backend");
    return rocm_backend.createBackendFn();
}

} // namespace

// ============================================================================
// Lifecycle
// ============================================================================

GPUModule::InitResult GPUModule::initialize(const GPUConfig &config, GPULauncher::BackendFn backend) {
    auto vr = config.validate();
    if (!vr.ok) {
        InitResult r;
        r.ok    = false;
        r.error = vr.errors.empty() ? "invalid config" : vr.errors.front();
        return r;
    }

    config_ = config;
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
        if (backend) {
            launcher_ = std::make_unique<GPULauncher>(backend);
        } else {
            launcher_ = std::make_unique<GPULauncher>(createDefaultLauncherBackend());
        }
    }

    initialized_ = true;
    return {true, ""};
}

// ============================================================================
// Work submission
// ============================================================================

GPUModule::SubmitResult GPUModule::submitWork(const std::string &caller_id, const std::string &tenant_id,
                                              const GPULauncher::WorkItem &item) {
    if (!initialized_) {
        return {false, false, "GPUModule not initialized"};
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
            return {false, false, "policy denied: " + decision.reason};
        }
    }

    // 2. Circuit-breaker / safe-fail dispatch.
    bool used_gpu = false;
    bool success  = false;

    auto gpu_op = [&]() -> bool {
        if (!launcher_) {
            return false;
        }
        auto fut = launcher_->submit(item);
        auto res = fut.get();
        used_gpu = res.success;
        return res.success;
    };

    auto cpu_fallback = [&]() -> bool {
        used_gpu = false;
        if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
            audit_log_.recordFallbackToCPU("circuit_open_or_gpu_failure", tenant_id);
        }
        if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
            GPUMetrics::GetInstance().recordFallback("circuit_open_or_gpu_failure");
        }
        return true; // CPU fallback always "succeeds"
    };

    success = safe_fail_.executeWithFallback(gpu_op, cpu_fallback, item.kernel_id);

    // 3. Metrics.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::METRICS)) {
        if (success && used_gpu) {
            GPUMetrics::GetInstance().recordAllocSuccess(0, tenant_id);
        } else if (!success) {
            GPUMetrics::GetInstance().recordAllocFailGlobal(0, tenant_id);
        }
    }

    // 4. Audit.
    if (GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG)) {
        auto etype = (success && used_gpu) ? GPUAuditLog::EventType::ALLOC_SUCCESS
                                           : GPUAuditLog::EventType::ALLOC_FAIL_GLOBAL_LIMIT;
        audit_log_.record(etype, 0, caller_id, tenant_id, used_gpu ? "gpu_launch" : "cpu_fallback");
    }

    return {success, used_gpu, success ? "" : "operation failed"};
}

// ============================================================================
// Inline VRAM management
// ============================================================================

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

void GPUModule::grantCaller(const std::string &caller_id, GPUPolicy::Capability cap) {
    policy_.grant(caller_id, cap);
}

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
    if (static_cast<int>(all.size()) > last_n) {
        all.erase(all.begin(), all.begin() + static_cast<ptrdiff_t>(static_cast<int>(all.size()) - last_n));
    }
    return all;
}

} // namespace gpu
} // namespace themis
