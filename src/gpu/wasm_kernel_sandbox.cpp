/**
 * @file wasm_kernel_sandbox.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * WASMKernelSandbox — isolated execution environment for untrusted GPU kernels.
 *
 * Provides two enforcement layers before any third-party kernel blob is
 * allowed to execute:
 *
 *   1. Whitelist + checksum validation via GPUKernelValidator.
 *   2. Sandboxed execution with memory-limit and timeout enforcement.
 *
 * When THEMIS_ENABLE_WASM is defined, execution is delegated to a WASM
 * runtime (Wasmtime / WasmEdge) for true linear-memory isolation.  In the
 * current build the CPU simulation path is used: the kernel blob is passed
 * to the caller-supplied BackendFn (or a CPU no-op) within a timed future,
 * and the memory-limit check is applied against the blob byte count.
 */

#include "themis/gpu/wasm_kernel_sandbox.h"
#include "themis/gpu/feature_flags.h"
#include <chrono>
#include <future>
#include <stdexcept>

#ifdef THEMIS_ENABLE_WASM
// Future: #include <wasmtime.h> or <wasm_edge.h>
#endif

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

WASMKernelSandbox::WASMKernelSandbox(SandboxConfig config)
    : config_(std::move(config)) {}

// ============================================================================
// Configuration
// ============================================================================

void WASMKernelSandbox::setConfig(SandboxConfig config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = std::move(config);
}

WASMKernelSandbox::SandboxConfig WASMKernelSandbox::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

// ============================================================================
// Capability query
// ============================================================================

bool WASMKernelSandbox::isWASMSupported() const noexcept {
#ifdef THEMIS_ENABLE_WASM
    return true;
#else
    return false;
#endif
}

// ============================================================================
// Execution
// ============================================================================

WASMKernelSandbox::ExecutionResult
WASMKernelSandbox::execute(const std::string&          kernel_id,
                            const std::vector<uint8_t>& blob,
                            GPULauncher::BackendFn      backend)
{
    // Step 1: Feature gate — WASM_SANDBOX must be enabled.
    if (!GPUFeatureFlags::GetInstance().isEnabled(
            GPUFeatureFlags::Feature::WASM_SANDBOX)) {
        ExecutionResult r;
        r.status    = Status::REJECTED_FEATURE_DISABLED;
        r.kernel_id = kernel_id;
        r.message   = "WASM_SANDBOX feature is disabled for this edition";
        recordResult(r);
        return r;
    }

    // Step 2: Empty blob guard.
    if (blob.empty()) {
        ExecutionResult r;
        r.status    = Status::REJECTED_EMPTY_BLOB;
        r.kernel_id = kernel_id;
        r.message   = "kernel blob is empty";
        recordResult(r);
        return r;
    }

    // Step 3: Memory limit — reject blobs that exceed the configured ceiling.
    // Read the limit under the lock, then release before calling recordResult()
    // (which also acquires the mutex) to avoid self-deadlock.
    {
        uint64_t limit = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            limit = config_.memory_limit_bytes;
        }
        if (limit > 0 && blob.size() > limit) {
            ExecutionResult r;
            r.status    = Status::REJECTED_MEMORY_LIMIT;
            r.kernel_id = kernel_id;
            r.message   = "blob size " + std::to_string(blob.size()) +
                          " bytes exceeds memory limit " +
                          std::to_string(limit) + " bytes";
            recordResult(r);
            return r;
        }
    }

    // Step 4: Whitelist + checksum validation.
    {
        auto vr = GPUKernelValidator::GetInstance().validate(kernel_id, blob);
        if (vr.status == GPUKernelValidator::Status::UNKNOWN_KERNEL) {
            ExecutionResult r;
            r.status    = Status::REJECTED_NOT_WHITELISTED;
            r.kernel_id = kernel_id;
            r.message   = vr.message;
            recordResult(r);
            return r;
        }
        if (vr.status == GPUKernelValidator::Status::CHECKSUM_MISMATCH) {
            ExecutionResult r;
            r.status    = Status::REJECTED_CHECKSUM_MISMATCH;
            r.kernel_id = kernel_id;
            r.message   = vr.message;
            recordResult(r);
            return r;
        }
        // EMPTY_BLOB is already handled above, but defensive check here.
        if (vr.status != GPUKernelValidator::Status::OK) {
            ExecutionResult r;
            r.status    = Status::EXECUTION_ERROR;
            r.kernel_id = kernel_id;
            r.message   = "validator returned unexpected status: " + vr.message;
            recordResult(r);
            return r;
        }
    }

    // Step 5–6: Execute in sandbox (CPU simulation) with timeout enforcement.
    ExecutionResult r = runInSandbox(kernel_id, blob, std::move(backend));
    recordResult(r);
    return r;
}

// ============================================================================
// Internal sandbox execution
// ============================================================================

WASMKernelSandbox::ExecutionResult
WASMKernelSandbox::runInSandbox(const std::string&          kernel_id,
                                 const std::vector<uint8_t>& blob,
                                 GPULauncher::BackendFn      backend)
{
    // Build a no-op CPU backend when none is supplied.
    GPULauncher::BackendFn fn = backend
        ? std::move(backend)
        : [](const GPULauncher::WorkItem&) -> bool { return true; };

    uint32_t timeout_ms;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timeout_ms = config_.max_execution_ms;
    }

    // Build a minimal WorkItem to pass into the backend.
    GPULauncher::WorkItem item;
    item.kernel_id = kernel_id;
    item.tag       = "wasm_sandbox";
    item.args      = blob;

#ifdef THEMIS_ENABLE_WASM
    // Future: instantiate the WASM module, set memory limit, execute, check
    // result.  For now fall through to the CPU simulation path even when the
    // WASM guard is defined, until a real runtime is wired in.
    (void)0;
#endif

    // CPU simulation: run the backend in an async future and apply a timeout.
    const auto start = std::chrono::steady_clock::now();

    if (timeout_ms == 0) {
        // No timeout — execute synchronously.
        bool ok = false;
        try {
            ok = fn(item);
        } catch (...) {
            ExecutionResult r;
            r.status    = Status::EXECUTION_ERROR;
            r.kernel_id = kernel_id;
            r.message   = "exception during sandbox execution";
            r.elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start);
            return r;
        }
        ExecutionResult r;
        r.status    = ok ? Status::OK : Status::EXECUTION_ERROR;
        r.kernel_id = kernel_id;
        r.message   = ok ? "OK" : "backend returned failure";
        r.elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - start);
        return r;
    }

    // With timeout: use std::async and wait_for.
    auto fut = std::async(std::launch::async, [&fn, &item]() -> bool {
        return fn(item);
    });

    const auto deadline = std::chrono::milliseconds(timeout_ms);
    if (fut.wait_for(deadline) == std::future_status::timeout) {
        ExecutionResult r;
        r.status    = Status::REJECTED_TIMEOUT;
        r.kernel_id = kernel_id;
        r.message   = "execution exceeded timeout of " +
                      std::to_string(timeout_ms) + " ms";
        r.elapsed   = deadline;
        return r;
    }

    bool ok = false;
    try {
        ok = fut.get();
    } catch (...) {
        ExecutionResult r;
        r.status    = Status::EXECUTION_ERROR;
        r.kernel_id = kernel_id;
        r.message   = "exception during sandbox execution";
        r.elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - start);
        return r;
    }

    ExecutionResult r;
    r.status    = ok ? Status::OK : Status::EXECUTION_ERROR;
    r.kernel_id = kernel_id;
    r.message   = ok ? "OK" : "backend returned failure";
    r.elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - start);
    return r;
}

// ============================================================================
// Statistics
// ============================================================================

void WASMKernelSandbox::recordResult(const ExecutionResult& r) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++total_submitted_;
    total_elapsed_ms_ += static_cast<uint64_t>(r.elapsed.count());
    switch (r.status) {
        case Status::OK:
            ++ok_count_;
            break;
        case Status::REJECTED_NOT_WHITELISTED:
            ++rejected_not_whitelisted_;
            break;
        case Status::REJECTED_CHECKSUM_MISMATCH:
            ++rejected_checksum_;
            break;
        case Status::REJECTED_EMPTY_BLOB:
            ++rejected_empty_;
            break;
        case Status::REJECTED_FEATURE_DISABLED:
            ++rejected_feature_disabled_;
            break;
        case Status::REJECTED_MEMORY_LIMIT:
            ++rejected_memory_limit_;
            break;
        case Status::REJECTED_TIMEOUT:
            ++rejected_timeout_;
            break;
        case Status::EXECUTION_ERROR:
            ++execution_errors_;
            break;
    }
}

WASMKernelSandbox::Stats WASMKernelSandbox::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_submitted          = total_submitted_;
    s.ok_count                 = ok_count_;
    s.rejected_not_whitelisted = rejected_not_whitelisted_;
    s.rejected_checksum        = rejected_checksum_;
    s.rejected_empty           = rejected_empty_;
    s.rejected_feature_disabled = rejected_feature_disabled_;
    s.rejected_memory_limit    = rejected_memory_limit_;
    s.rejected_timeout         = rejected_timeout_;
    s.execution_errors         = execution_errors_;
    s.total_elapsed_ms         = total_elapsed_ms_;
    return s;
}

void WASMKernelSandbox::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    total_submitted_          = 0;
    ok_count_                 = 0;
    rejected_not_whitelisted_ = 0;
    rejected_checksum_        = 0;
    rejected_empty_           = 0;
    rejected_feature_disabled_ = 0;
    rejected_memory_limit_    = 0;
    rejected_timeout_         = 0;
    execution_errors_         = 0;
    total_elapsed_ms_         = 0;
}

// ============================================================================
// Status name helper
// ============================================================================

const char* sandboxStatusName(WASMKernelSandbox::Status s) noexcept {
    using S = WASMKernelSandbox::Status;
    switch (s) {
        case S::OK:                         return "OK";
        case S::REJECTED_NOT_WHITELISTED:   return "REJECTED_NOT_WHITELISTED";
        case S::REJECTED_CHECKSUM_MISMATCH: return "REJECTED_CHECKSUM_MISMATCH";
        case S::REJECTED_EMPTY_BLOB:        return "REJECTED_EMPTY_BLOB";
        case S::REJECTED_FEATURE_DISABLED:  return "REJECTED_FEATURE_DISABLED";
        case S::REJECTED_MEMORY_LIMIT:      return "REJECTED_MEMORY_LIMIT";
        case S::REJECTED_TIMEOUT:           return "REJECTED_TIMEOUT";
        case S::EXECUTION_ERROR:            return "EXECUTION_ERROR";
    }
    return "UNKNOWN";
}

} // namespace gpu
} // namespace themis

