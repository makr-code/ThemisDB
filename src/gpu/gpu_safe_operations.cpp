/**
 * @file gpu_safe_operations.cpp
 * @brief Implementation of RAII wrappers and error handling for GPU operations.
 * @version 1.0.0
 * @date 2026-08-16
 */

#include "gpu/gpu_safe_operations.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace gpu {

// =============================================================================
// CudaError Implementation
// =============================================================================

// The CudaError class is header-only (implementation in constructor).
// This file is a placeholder for future extensions and logging integration.

/**
 * @brief Log a CUDA error for diagnostics.
 * 
 * Called by CUDA_CHECK macro when an error is encountered.
 */
void log_cuda_error(const CudaError& err) noexcept {
    try {
        spdlog::error("CUDA error: {}", err.what());
    } catch (...) {
        // Ignore logging errors to avoid cascading failures.
    }
}

}  // namespace gpu
}  // namespace themis
