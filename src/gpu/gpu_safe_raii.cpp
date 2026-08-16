/**
 * @file gpu_safe_raii.cpp
 * @brief GPU RAII wrappers implementation
 */

#include "gpu/gpu_safe_raii.h"
#include <iostream>

namespace themis {
namespace gpu {

// ============================================================================
// KernelTimeoutGuard Implementation
// ============================================================================

KernelTimeoutGuard::KernelTimeoutGuard(cudaStream_t stream, uint32_t timeout_ms)
    : stream_(stream), timeout_ms_(timeout_ms) {
    
    // Start monitor thread
    monitor_thread_ = std::thread([this]() { monitorThread(); });
}

KernelTimeoutGuard::~KernelTimeoutGuard() noexcept {
    // Signal completion if not already done
    if (!completed_.load(std::memory_order_acquire)) {
        // Wait a bit for completion
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Mark as completed to signal monitor thread to exit
    completed_.store(true, std::memory_order_release);

    // Wait for monitor thread to finish
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void KernelTimeoutGuard::monitorThread() {
    // Wait for either completion or timeout
    auto start = std::chrono::high_resolution_clock::now();
    
    while (!completed_.load(std::memory_order_acquire)) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        
        if (elapsed.count() >= timeout_ms_) {
            // Timeout exceeded!
            timed_out_.store(true, std::memory_order_release);
            
            // Attempt to cancel stream (best-effort)
            try {
                cudaStreamDestroy(stream_);
            } catch (...) {
                // Ignore errors in destructor-like context
            }
            return;
        }
        
        // Sleep a bit to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

}} // namespace themis::gpu
