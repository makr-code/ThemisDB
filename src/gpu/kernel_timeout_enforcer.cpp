/**
 * @file kernel_timeout_enforcer.cpp
 * @brief KernelTimeoutEnforcer implementation
 */

#include "gpu/kernel_timeout_enforcer.h"
#include "gpu/gpu_safe_raii.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace themis {
namespace gpu {

bool KernelTimeoutEnforcer::executeWithTimeout(
    const std::function<void()>& kernel_lambda,
    const KernelConfig& config) {
    
    if (!kernel_lambda) {
        throw std::runtime_error("KernelTimeoutEnforcer: kernel_lambda is null");
    }

    kernel_timed_out_ = false;
    std::atomic<bool> kernel_completed{false};
    std::atomic<bool> timeout_triggered{false};

    // Start monitoring thread
    std::thread monitor([this, &kernel_completed, &timeout_triggered, config]() {
        auto start = std::chrono::high_resolution_clock::now();
        
        while (!kernel_completed.load()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            
            if (elapsed.count() >= config.timeout_ms) {
                timeout_triggered.store(true);
                kernel_timed_out_ = true;
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    try {
        kernel_lambda();
        kernel_completed.store(true);
    } catch (...) {
        kernel_completed.store(true);
        if (monitor.joinable()) {
            monitor.join();
        }
        throw;
    }

    kernel_completed.store(true);
    if (monitor.joinable()) {
        monitor.join();
    }

    return !timeout_triggered.load();
}

bool KernelTimeoutEnforcer::executeWithFallback(
    const std::function<void()>& gpu_kernel,
    const std::function<void()>& cpu_kernel,
    const KernelConfig& config) {
    
    // Try GPU execution with timeout
    bool gpu_success = executeWithTimeout(gpu_kernel, config);

    if (!gpu_success && config.enable_fallback) {
        // Fallback to CPU
        if (cpu_kernel) {
            try {
                cpu_kernel();
                return false;  // Fell back to CPU
            } catch (...) {
                throw;
            }
        }
    }

    return gpu_success;
}

}} // namespace themis::gpu
