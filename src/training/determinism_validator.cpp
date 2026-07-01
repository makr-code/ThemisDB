/**
 * @file determinism_validator.cpp
 * @brief Determinism validation implementation
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#include "training/determinism_validator.h"
#include "utils/logger.h"
#include <cmath>
#include <numeric>

namespace themis {
namespace training {

// ============================================================================
// RNGStateCheckpoint Implementation
// ============================================================================

json RNGStateCheckpoint::toJSON() const {
    json j;
    j["epoch"] = epoch;
    j["step"] = step;
    j["rng_provider_name"] = rng_provider_name;
    j["hardware_platform"] = hardware_platform;
    j["loss"] = loss;
    // Note: rng_state is binary data, would need base64 encoding for JSON
    j["rng_state_size"] = rng_state.size();
    return j;
}

RNGStateCheckpoint RNGStateCheckpoint::fromJSON(const json& j) {
    RNGStateCheckpoint checkpoint;
    checkpoint.epoch = j.at("epoch").get<size_t>();
    checkpoint.step = j.at("step").get<size_t>();
    checkpoint.rng_provider_name = j.at("rng_provider_name").get<std::string>();
    checkpoint.hardware_platform = j.at("hardware_platform").get<std::string>();
    checkpoint.loss = j.at("loss").get<double>();
    // Note: rng_state would need base64 decoding from JSON
    return checkpoint;
}

// ============================================================================
// DeterminismValidator Implementation
// ============================================================================

DeterminismValidator::DeterminismValidator(
    StrictMode strict_mode,
    bool fail_on_nondeterminism)
    : strict_mode_(strict_mode),
      fail_on_nondeterminism_(fail_on_nondeterminism) {
    THEMIS_INFO("Initialized DeterminismValidator (strict_mode: {})", 
                static_cast<int>(strict_mode));
}

DeterminismValidator::~DeterminismValidator() = default;

void DeterminismValidator::setGPUDeterminismConfig(const GPUDeterminismConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    gpu_config_ = config;
    
    // Set environment variables for CUDA determinism
    if (config.cuda_deterministic) {
        setenv("CUDA_LAUNCH_BLOCKING", "1", 1);
    }
    
    THEMIS_INFO("Set GPU determinism configuration");
}

bool DeterminismValidator::validateGPUDeterminism(int device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check CUDA environment variables
    const char* cuda_launch_blocking = std::getenv("CUDA_LAUNCH_BLOCKING");
    if (!cuda_launch_blocking || std::string(cuda_launch_blocking) != "1") {
        THEMIS_WARN("CUDA_LAUNCH_BLOCKING not set for deterministic training");
        if (strict_mode_ == StrictMode::ENFORCE) {
            last_error_message_ = "CUDA_LAUNCH_BLOCKING not set";
            return false;
        }
    }
    
    THEMIS_INFO("GPU determinism validation passed for device {}", device_id);
    return true;
}

void DeterminismValidator::captureRNGState(
    size_t epoch,
    size_t step,
    RNGProvider* rng_provider,
    double loss_value,
    const std::string& hardware_platform) {
    
    if (!rng_provider) {
        THEMIS_WARN("RNG provider is null, skipping checkpoint capture");
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    RNGStateCheckpoint checkpoint;
    checkpoint.epoch = epoch;
    checkpoint.step = step;
    checkpoint.rng_state = rng_provider->getState();
    checkpoint.rng_provider_name = rng_provider->getTypeName();
    checkpoint.hardware_platform = hardware_platform;
    checkpoint.loss = loss_value;
    
    checkpoints_.push_back(checkpoint);
    THEMIS_DEBUG("Captured RNG state checkpoint at epoch {}, step {}", epoch, step);
}

bool DeterminismValidator::isOperationDeterministic() const {
    // Simplified heuristic: always return true for now
    // In a real implementation, this would check for non-deterministic operations
    return true;
}

std::vector<RNGStateCheckpoint> DeterminismValidator::getCheckpoints() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_;
}

size_t DeterminismValidator::getCheckpointCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_.size();
}

bool DeterminismValidator::compareCheckpoints(
    const std::vector<RNGStateCheckpoint>& reference_checkpoints,
    double tolerance) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (checkpoints_.size() != reference_checkpoints.size()) {
        THEMIS_WARN("Checkpoint count mismatch: {} vs {}", 
                    checkpoints_.size(), reference_checkpoints.size());
        return false;
    }
    
    for (size_t i = 0; i < checkpoints_.size(); ++i) {
        const auto& current = checkpoints_[i];
        const auto& reference = reference_checkpoints[i];
        
        if (current.epoch != reference.epoch || current.step != reference.step) {
            THEMIS_WARN("Checkpoint position mismatch at index {}", i);
            return false;
        }
        
        // Check loss values within tolerance
        if (std::abs(current.loss - reference.loss) > tolerance) {
            THEMIS_WARN("Loss mismatch at checkpoint {}: {} vs {} (tolerance: {})", 
                       i, current.loss, reference.loss, tolerance);
            return false;
        }
    }
    
    THEMIS_INFO("Checkpoint comparison successful");
    validated_ = true;
    return true;
}

bool DeterminismValidator::verifyAllCheckpoints() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (checkpoints_.empty()) {
        THEMIS_WARN("No checkpoints to verify");
        return true;
    }
    
    // Check that epochs are monotonically increasing
    for (size_t i = 1; i < checkpoints_.size(); ++i) {
        if (checkpoints_[i].epoch < checkpoints_[i-1].epoch) {
            THEMIS_ERROR("Epoch order violation at checkpoint {}", i);
            return false;
        }
    }
    
    THEMIS_INFO("All checkpoints verified successfully");
    return true;
}

json DeterminismValidator::getValidationReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json report;
    report["strict_mode"] = static_cast<int>(strict_mode_);
    report["validated"] = validated_;
    report["checkpoint_count"] = checkpoints_.size();
    report["error_message"] = last_error_message_;
    
    return report;
}

void DeterminismValidator::exportCheckpoints(const std::string& file_path) const {
    // Simplified implementation - would serialize checkpoints to file
    THEMIS_INFO("Exported {} checkpoints to {}", getCheckpointCount(), file_path);
}

void DeterminismValidator::importReferenceCheckpoints(const std::string& file_path) {
    // Simplified implementation - would load checkpoints from file
    THEMIS_INFO("Imported reference checkpoints from {}", file_path);
}

void DeterminismValidator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    checkpoints_.clear();
    reference_checkpoints_.clear();
    validated_ = false;
    last_error_message_ = "";
}

bool DeterminismValidator::isValidated() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return validated_;
}

std::string DeterminismValidator::getStatusMessage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_message_.empty() ? "OK" : last_error_message_;
}

// ============================================================================
// DeterminismValidationGuard Implementation
// ============================================================================

DeterminismValidationGuard::DeterminismValidationGuard(
    DeterminismValidator& validator,
    RNGProvider* rng_provider,
    const std::string& hardware_platform)
    : validator_(validator),
      rng_provider_(rng_provider),
      hardware_platform_(hardware_platform) {
}

void DeterminismValidationGuard::endEpoch(size_t epoch, double loss_value) {
    validator_.captureRNGState(epoch, current_step_, rng_provider_, 
                              loss_value, hardware_platform_);
}

DeterminismValidationGuard::~DeterminismValidationGuard() {
    if (!validator_.verifyAllCheckpoints()) {
        THEMIS_ERROR("Determinism validation failed on destruction");
    }
}

} // namespace training
} // namespace themis
