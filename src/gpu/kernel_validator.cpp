/**
 * @file kernel_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Kernel Integrity Validator
 * ================================
 * FNV-1a checksum whitelist for GPU kernel blobs.
 */

#include "themis/gpu/kernel_validator.h"

namespace themis {
namespace gpu {

// ============================================================================
// Checksum — FNV-1a 64-bit
// ============================================================================

uint64_t GPUKernelValidator::computeChecksum(const std::vector<uint8_t>& data) {
    return computeChecksum(data.data(), data.size());
}

uint64_t GPUKernelValidator::computeChecksum(const uint8_t* data,
                                               size_t length) {
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME        = 1099511628211ULL;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

// ============================================================================
// Registry management
// ============================================================================

void GPUKernelValidator::registerKernel(const std::string& kernel_id,
                                          uint64_t expected_checksum) {
    std::lock_guard<std::mutex> lock(mutex_);
    registry_[kernel_id] = expected_checksum;
}

void GPUKernelValidator::registerKernel(const std::string& kernel_id,
                                          const std::vector<uint8_t>& blob) {
    const uint64_t cs = computeChecksum(blob);
    std::lock_guard<std::mutex> lock(mutex_);
    registry_[kernel_id] = cs;
}

void GPUKernelValidator::unregisterKernel(const std::string& kernel_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    registry_.erase(kernel_id);
}

bool GPUKernelValidator::isRegistered(const std::string& kernel_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.count(kernel_id) > 0;
}

std::vector<std::string> GPUKernelValidator::registeredKernels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(registry_.size());
    for (const auto& kv : registry_) {
        result.push_back(kv.first);
    }
    return result;
}

// ============================================================================
// Validation
// ============================================================================

GPUKernelValidator::ValidationResult
GPUKernelValidator::validate(const std::string& kernel_id,
                               const std::vector<uint8_t>& blob) const {
    ValidationResult r;
    r.kernel_id = kernel_id;

    if (blob.empty()) {
        r.status  = Status::EMPTY_BLOB;
        r.message = "Kernel blob is empty; rejecting kernel '" + kernel_id + "'";
        std::lock_guard<std::mutex> lock(mutex_);
        ++total_validations_;
        ++empty_blob_count_;
        return r;
    }

    r.computed_checksum = computeChecksum(blob);

    std::lock_guard<std::mutex> lock(mutex_);
    ++total_validations_;

    auto it = registry_.find(kernel_id);
    if (it == registry_.end()) {
        r.status  = Status::UNKNOWN_KERNEL;
        r.message = "Kernel '" + kernel_id +
                    "' is not in the whitelist; rejecting";
        ++unknown_kernel_count_;
        return r;
    }

    r.expected_checksum = it->second;
    if (r.computed_checksum != r.expected_checksum) {
        r.status  = Status::CHECKSUM_MISMATCH;
        r.message = "Checksum mismatch for kernel '" + kernel_id +
                    "': computed=" + std::to_string(r.computed_checksum) +
                    " expected=" + std::to_string(r.expected_checksum);
        ++checksum_mismatch_count_;
        return r;
    }

    r.status  = Status::OK;
    r.message = "OK";
    ++ok_count_;
    return r;
}

bool GPUKernelValidator::isValid(const std::string& kernel_id,
                                   const std::vector<uint8_t>& blob) const {
    return validate(kernel_id, blob).status == Status::OK;
}

// ============================================================================
// Stats
// ============================================================================

GPUKernelValidator::Stats GPUKernelValidator::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.registered_count        = registry_.size();
    s.total_validations       = total_validations_;
    s.ok_count                = ok_count_;
    s.unknown_kernel_count    = unknown_kernel_count_;
    s.checksum_mismatch_count = checksum_mismatch_count_;
    s.empty_blob_count        = empty_blob_count_;
    return s;
}

void GPUKernelValidator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    registry_.clear();
    total_validations_       = 0;
    ok_count_                = 0;
    unknown_kernel_count_    = 0;
    checksum_mismatch_count_ = 0;
    empty_blob_count_        = 0;
}

} // namespace gpu
} // namespace themis
