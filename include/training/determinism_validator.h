/**
 * @file determinism_validator.h
 * @brief Training determinism enforcement and verification
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 1 foundation)
 * @author makr
 * 
 * Guarantees deterministic training reproducibility through:
 * - RNG state serialization and validation
 * - GPU determinism flags enforcement
 * - Floating-point operation ordering control
 * - Epoch boundary checkpoints
 * 
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace training {

using json = nlohmann::json;

/**
 * @brief RNG provider abstraction for pluggable random number generation
 * 
 * Enables determinism validation across different RNG implementations
 * (std::mt19937, std::random_device, GPU RNGs, etc.)
 */
class RNGProvider {
public:
    virtual ~RNGProvider() = default;
    
    /**
     * @brief Generate next random value [0, 1)
     */
    virtual double nextUniform() = 0;
    
    /**
     * @brief Generate next normal value (mean=0, stddev=1)
     */
    virtual double nextNormal() = 0;
    
    /**
     * @brief Get current RNG state as serialized bytes
     */
    virtual std::vector<uint8_t> getState() const = 0;
    
    /**
     * @brief Set RNG state from serialized bytes
     */
    virtual void setState(const std::vector<uint8_t>& state) = 0;
    
    /**
     * @brief Seed the RNG
     */
    virtual void seed(uint64_t seed_value) = 0;
    
    /**
     * @brief Get RNG type name for diagnostics
     */
    virtual std::string getTypeName() const = 0;
};

/**
 * @brief GPU determinism configuration
 * 
 * Flags to enforce deterministic behavior on CUDA/HIP/Metal devices.
 * Note: Some options may have performance cost.
 */
struct GPUDeterminismConfig {
    bool cuda_deterministic = true;              ///< CUDA_LAUNCH_BLOCKING=1
    bool cudnn_deterministic = true;             ///< cuDNN deterministic mode
    bool rocm_deterministic = true;              ///< HIP deterministic mode
    bool metal_deterministic = true;             ///< Metal deterministic mode
    bool disable_gpu_autotuning = true;          ///< Disable library autotuning
    bool use_reduced_precision = false;          ///< Use lower precision for speed (less determinism)
    
    GPUDeterminismConfig() = default;
};

/**
 * @brief RNG state checkpoint for determinism validation
 * 
 * Captures the complete RNG state at an epoch boundary to enable
 * bit-exact reproducibility verification.
 */
struct RNGStateCheckpoint {
    size_t epoch;                                 ///< Epoch number when captured
    size_t step;                                  ///< Step number when captured
    std::vector<uint8_t> rng_state;             ///< Serialized RNG state
    std::string rng_provider_name;               ///< Name of RNG provider (for diagnostics)
    std::string hardware_platform;               ///< Hardware where captured (cuda/hip/metal/cpu)
    double loss;                                 ///< Loss value at this checkpoint
    
    RNGStateCheckpoint() = default;
    
    /**
     * @brief Serialize to JSON
     */
    json toJSON() const;
    
    /**
     * @brief Deserialize from JSON
     */
    static RNGStateCheckpoint fromJSON(const json& j);
};

/**
 * @brief Determinism validator for training reproducibility
 * 
 * Enforces and verifies deterministic training behavior by:
 * - Capturing RNG state at epoch boundaries
 * - Validating GPU determinism flags
 * - Detecting non-deterministic operations
 * - Comparing training runs for bit-exact reproducibility
 * 
 * Thread-safe for concurrent validation checks.
 * 
 * Example usage:
 * @code
 * // Create validator with strict mode
 * DeterminismValidator validator(
 *     DeterminismValidator::StrictMode::ENFORCE,
 *     true  // fail_on_nondeterminism
 * );
 * 
 * // Validate GPU determinism before training starts
 * if (!validator.validateGPUDeterminism(device_id)) {
 *     throw std::runtime_error("GPU not configured for deterministic training");
 * }
 * 
 * // Seed RNG and capture initial state
 * rng->seed(12345);
 * validator.captureRNGState(0, 0, rng.get(), initial_loss, "cuda");
 * 
 * // Training loop
 * for (size_t epoch = 0; epoch < num_epochs; ++epoch) {
 *     for (size_t step = 0; step < steps_per_epoch; ++step) {
 *         // Training step...
 *         
 *         // Periodically check for non-determinism
 *         if (step % 100 == 0) {
 *             if (!validator.isOperationDeterministic()) {
 *                 throw std::runtime_error("Non-deterministic operation detected!");
 *             }
 *         }
 *     }
 *     
 *     // Capture epoch boundary state
 *     validator.captureRNGState(epoch + 1, steps_per_epoch, rng.get(), loss, "cuda");
 * }
 * 
 * // Verify all checkpoints
 * if (!validator.verifyAllCheckpoints()) {
 *     throw std::runtime_error("Determinism validation failed!");
 * }
 * @endcode
 */
class DeterminismValidator {
public:
    /**
     * @brief Determinism enforcement mode
     */
    enum class StrictMode {
        OFF,           ///< No enforcement (validation only)
        WARN,          ///< Log warnings on non-deterministic behavior
        ENFORCE,       ///< Fail-fast on non-deterministic operations
    };
    
    /**
     * @brief Construct determinism validator
     * 
     * @param strict_mode How strictly to enforce determinism
     * @param fail_on_nondeterminism If true, throw exception on violations
     */
    explicit DeterminismValidator(
        StrictMode strict_mode = StrictMode::ENFORCE,
        bool fail_on_nondeterminism = true
    );
    
    ~DeterminismValidator();
    
    /**
     * @brief Set GPU determinism configuration
     * 
     * @param config GPU determinism flags to apply
     * @throws std::runtime_error if GPU env vars cannot be set
     */
    void setGPUDeterminismConfig(const GPUDeterminismConfig& config);
    
    /**
     * @brief Validate GPU determinism support and configuration
     * 
     * Checks that the target GPU/device supports deterministic operations
     * and that required environment variables are set.
     * 
     * @param device_id GPU device index (0-based)
     * @return True if GPU is properly configured for determinism
     */
    bool validateGPUDeterminism(int device_id = 0);
    
    /**
     * @brief Capture RNG state at epoch boundary
     * 
     * Records the exact RNG state to enable reproducibility verification.
     * Should be called at predictable points (epoch boundaries) during training.
     * 
     * @param epoch Epoch number
     * @param step Global step number
     * @param rng_provider RNG provider to capture from
     * @param loss_value Current loss value
     * @param hardware_platform Hardware type (cuda/hip/metal/cpu)
     */
    void captureRNGState(
        size_t epoch,
        size_t step,
        RNGProvider* rng_provider,
        double loss_value,
        const std::string& hardware_platform
    );
    
    /**
     * @brief Check if operation appears to be deterministic
     * 
     * Uses heuristics to detect non-deterministic operations
     * (unordered operations, floating-point precision issues, etc.)
     * 
     * @return True if operation is likely deterministic, false otherwise
     */
    bool isOperationDeterministic() const;
    
    /**
     * @brief Get all captured RNG checkpoints
     * 
     * @return Vector of captured RNG state checkpoints
     */
    std::vector<RNGStateCheckpoint> getCheckpoints() const;
    
    /**
     * @brief Get checkpoint count
     * 
     * @return Number of captured RNG state checkpoints
     */
    size_t getCheckpointCount() const;
    
    /**
     * @brief Verify consistency between two training runs
     * 
     * Compares RNG checkpoints from a reference training run to verify
     * bit-exact reproducibility. Useful for validating that training is
     * deterministic across runs.
     * 
     * @param reference_checkpoints Checkpoints from reference run
     * @param tolerance Acceptable deviation in loss values (for numerical stability)
     * @return True if checkpoints match (within tolerance), false otherwise
     */
    bool compareCheckpoints(
        const std::vector<RNGStateCheckpoint>& reference_checkpoints,
        double tolerance = 1e-6
    ) const;
    
    /**
     * @brief Verify all captured checkpoints are consistent
     * 
     * Validates that RNG states transition properly across epochs
     * and that no corruptions occurred.
     * 
     * @return True if all checkpoints are valid, false otherwise
     */
    bool verifyAllCheckpoints() const;
    
    /**
     * @brief Get detailed validation report
     * 
     * Returns comprehensive report of determinism validation results
     * including any warnings or issues detected.
     * 
     * @return JSON object with validation details
     */
    json getValidationReport() const;
    
    /**
     * @brief Export checkpoints to file for archival
     * 
     * @param file_path Target file path
     */
    void exportCheckpoints(const std::string& file_path) const;
    
    /**
     * @brief Import reference checkpoints from file
     * 
     * @param file_path Source file path
     * @throws std::runtime_error if file not found or malformed
     */
    void importReferenceCheckpoints(const std::string& file_path);
    
    /**
     * @brief Reset validator state
     */
    void reset();
    
    /**
     * @brief Get whether determinism has been validated
     * 
     * @return True if at least one complete training run has been validated
     */
    bool isValidated() const;
    
    /**
     * @brief Get validation status message
     * 
     * @return Human-readable status/issue description
     */
    std::string getStatusMessage() const;
    
private:
    StrictMode strict_mode_;
    bool fail_on_nondeterminism_;
    std::vector<RNGStateCheckpoint> checkpoints_;
    std::vector<RNGStateCheckpoint> reference_checkpoints_;
    GPUDeterminismConfig gpu_config_;
    mutable std::mutex mutex_;
    bool validated_ = false;
    std::string last_error_message_;
};

/**
 * @brief RAII guard for determinism validation during training
 * 
 * Automatically captures RNG state at epoch boundaries and verifies
 * determinism throughout training. Throws exception if non-determinism
 * is detected.
 * 
 * Example:
 * @code
 * {
 *     DeterminismValidationGuard guard(validator, rng.get(), "cuda");
 *     
 *     for (size_t epoch = 0; epoch < num_epochs; ++epoch) {
 *         // Training loop...
 *         
 *         // Guard automatically validates determinism at epoch end
 *         guard.endEpoch(epoch, current_loss);
 *     }
 *     
 *     // Guard verifies all checkpoints on destruction
 * }
 * @endcode
 */
class DeterminismValidationGuard {
public:
    /**
     * @brief Construct validation guard
     * 
     * @param validator Reference to determinism validator
     * @param rng_provider RNG provider for state capture
     * @param hardware_platform Hardware type (cuda/hip/metal/cpu)
     */
    explicit DeterminismValidationGuard(
        DeterminismValidator& validator,
        RNGProvider* rng_provider,
        const std::string& hardware_platform = "cuda"
    );
    
    /**
     * @brief Mark end of epoch and capture state
     * 
     * @param epoch Epoch number
     * @param loss_value Loss at epoch end
     */
    void endEpoch(size_t epoch, double loss_value);
    
    /**
     * @brief Verify checkpoints on destruction
     */
    ~DeterminismValidationGuard();
    
    // Delete copy
    DeterminismValidationGuard(const DeterminismValidationGuard&) = delete;
    DeterminismValidationGuard& operator=(const DeterminismValidationGuard&) = delete;
    
private:
    DeterminismValidator& validator_;
    RNGProvider* rng_provider_;
    std::string hardware_platform_;
    size_t current_step_ = 0;
};

} // namespace training
} // namespace themis
