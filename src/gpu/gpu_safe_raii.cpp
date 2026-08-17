/**
 * @file gpu_safe_raii.cpp
 * @brief GPU RAII wrappers implementation
 */

#include "gpu/gpu_safe_raii.h"

#include <sstream>

namespace themis {
namespace gpu {

namespace detail {

[[noreturn]] void throwCudaError(const char* call,
                                 const char* file,
                                 int line,
                                 const std::string& detail) {
    std::ostringstream os;
    os << "CUDA error in " << call << ": " << detail << " (" << file << ':' << line << ')';
    throw std::runtime_error(os.str());
}

[[noreturn]] void throwCudaUnavailable(const char* call,
                                       const char* file,
                                       int line) {
    std::ostringstream os;
    os << "CUDA runtime unavailable for " << call
       << " (" << file << ':' << line << ')';
    throw std::runtime_error(os.str());
}

void destroyDeviceMemoryNoThrow(void* ptr) noexcept {
#if THEMIS_GPU_SAFE_RAII_HAS_CUDA
    if (ptr != nullptr) {
        static_cast<void>(cudaFree(ptr));
    }
#else
    static_cast<void>(ptr);
#endif
}

} // namespace detail

// ============================================================================
// KernelTimeoutGuard Implementation
// ============================================================================

KernelTimeoutGuard::KernelTimeoutGuard(cudaStream_t stream, uint32_t timeout_ms)
    : stream_(stream),
      timeout_ms_(timeout_ms) {
    monitor_thread_ = std::thread(&KernelTimeoutGuard::monitorThread, this);
}

KernelTimeoutGuard::~KernelTimeoutGuard() noexcept {
    completed_.store(true, std::memory_order_release);
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void KernelTimeoutGuard::monitorThread() {
    const auto start = std::chrono::steady_clock::now();

    while (!completed_.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (elapsed.count() >= timeout_ms_) {
            timed_out_.store(true, std::memory_order_release);
            return;
        }

        std::this_thread::sleep_for(poll_interval_);
    }
}

}} // namespace themis::gpu
