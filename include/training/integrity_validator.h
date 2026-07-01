/**
 * @file integrity_validator.h
 * @brief Data and model integrity validation for training
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 1 foundation)
 * @author makr
 * 
 * Validates integrity of training data and model weights through:
 * - Cryptographic hashing (SHA-256)
 * - Anomaly detection (NaN, Inf, out-of-range)
 * - Cross-shard consistency checks
 * - Corruption detection and recovery
 * 
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace training {

using json = nlohmann::json;

/**
 * @brief Data integrity check result
 */
struct IntegrityCheckResult {
    bool passed = true;                     ///< Overall check result
    std::vector<std::string> warnings;      ///< Non-fatal issues
    std::vector<std::string> errors;        ///< Fatal issues that failed check
    std::string data_hash;                  ///< SHA-256 hash of data
    size_t sample_count = 0;                ///< Number of samples checked
    size_t corrupted_samples = 0;           ///< Count of corrupted samples
    double check_time_seconds = 0.0;        ///< Time to run check
    
    /**
     * @brief Convert to JSON
     */
    json toJSON() const;
};

/**
 * @brief Integrity validator for training data and models
 * 
 * Performs comprehensive integrity validation to detect:
 * - Corrupted or missing data
 * - Invalid model weights (NaN, Inf, out-of-range)
 * - Dataset drift
 * - Cross-shard inconsistencies
 * 
 * Example usage:
 * @code
 * IntegrityValidator validator;
 * 
 * // Validate dataset before training
 * auto result = validator.validateDataset(
 *     training_data_path,
 *     IntegrityValidator::ValidationLevel::STRICT
 * );
 * if (!result.passed) {
 *     throw std::runtime_error("Dataset validation failed: " + 
 *         result.errors[0]);
 * }
 * 
 * // Monitor model weights during training
 * for (size_t step = 0; step < total_steps; ++step) {
 *     // Training step...
 *     
 *     // Periodic weight checks
 *     if (step % 1000 == 0) {
 *         auto weight_result = validator.validateModelWeights(
 *             weights_tensor,
 *             "adapter_weights"
 *         );
 *         if (!weight_result.passed) {
 *             throw std::runtime_error("Model corruption detected!");
 *         }
 *     }
 * }
 * @endcode
 */
class IntegrityValidator {
public:
    /**
     * @brief Validation strictness level
     */
    enum class ValidationLevel {
        LENIENT,    ///< Only check critical issues
        STANDARD,   ///< Check common issues
        STRICT,     ///< Check everything (slower)
    };
    
    /**
     * @brief Construct integrity validator
     * 
     * @param validation_level How strictly to validate
     */
    explicit IntegrityValidator(
        ValidationLevel validation_level = ValidationLevel::STANDARD
    );
    
    ~IntegrityValidator();
    
    /**
     * @brief Validate dataset file integrity
     * 
     * Checks that dataset file exists, is readable, and contains
     * valid data (no corruption, correct format).
     * 
     * @param dataset_path Path to dataset file
     * @param validation_level Override default validation level
     * @return Integrity check result
     */
    IntegrityCheckResult validateDataset(
        const std::string& dataset_path,
        ValidationLevel validation_level = ValidationLevel::STANDARD
    );
    
    /**
     * @brief Validate dataset hash matches expected value
     * 
     * @param dataset_path Path to dataset
     * @param expected_hash Expected SHA-256 hash
     * @return True if hashes match, false otherwise
     */
    bool validateDatasetHash(
        const std::string& dataset_path,
        const std::string& expected_hash
    );
    
    /**
     * @brief Validate model weights integrity
     * 
     * Checks for NaN, Inf, and out-of-range values in weights.
     * 
     * @param weights_buffer Serialized weights (binary)
     * @param weights_name Name of weights (for diagnostics)
     * @return Integrity check result
     */
    IntegrityCheckResult validateModelWeights(
        const std::vector<float>& weights_buffer,
        const std::string& weights_name = "weights"
    );
    
    /**
     * @brief Validate model weights batch with anomaly detection
     * 
     * @param weights Array of weight vectors
     * @param weights_name Name of weights layer
     * @return Integrity check result
     */
    IntegrityCheckResult validateModelWeightsBatch(
        const std::vector<std::vector<float>>& weights,
        const std::string& weights_name = "weights"
    );
    
    /**
     * @brief Compute SHA-256 hash of dataset
     * 
     * @param dataset_path Path to dataset file
     * @return SHA-256 hash as hex string (64 characters)
     */
    std::string computeDatasetHash(const std::string& dataset_path);
    
    /**
     * @brief Compute SHA-256 hash of weights
     * 
     * @param weights Weight vector
     * @return SHA-256 hash as hex string
     */
    std::string computeWeightsHash(const std::vector<float>& weights);
    
    /**
     * @brief Register custom validation callback
     * 
     * @param callback Function to call for custom validation
     */
    void registerCustomValidator(
        std::function<bool(const std::vector<float>&)> callback
    );
    
    /**
     * @brief Detect anomalies in weight distribution
     * 
     * Uses statistical methods to detect anomalous patterns in weights:
     * - Sudden large changes
     * - Unusual distributions
     * - Correlated failures
     * 
     * @param weights Weight vector
     * @param sensitivity Anomaly detection sensitivity (0-1, higher = more sensitive)
     * @return True if anomalies detected, false if clean
     */
    bool detectAnomalies(
        const std::vector<float>& weights,
        double sensitivity = 0.5
    );
    
    /**
     * @brief Get validation statistics
     * 
     * @return JSON object with validation statistics
     */
    json getValidationStats() const;
    
    /**
     * @brief Reset validator state
     */
    void reset();
    
private:
    ValidationLevel validation_level_;
    std::vector<std::function<bool(const std::vector<float>&)>> custom_validators_;
    size_t total_checks_ = 0;
    size_t failed_checks_ = 0;
    
    bool hasNaN(const std::vector<float>& data) const;
    bool hasInf(const std::vector<float>& data) const;
    bool isOutOfRange(float value, float min_val, float max_val) const;
};

/**
 * @brief Cross-shard integrity synchronization
 * 
 * Validates that adapter weights are consistent across all shards
 * after distribution.
 */
class CrossShardIntegrityValidator {
public:
    /**
     * @brief Construct cross-shard validator
     * 
     * @param shard_id Local shard identifier
     * @param total_shards Total number of shards
     */
    explicit CrossShardIntegrityValidator(
        int shard_id,
        int total_shards
    );
    
    /**
     * @brief Register adapter weights for this shard
     * 
     * @param adapter_id Adapter identifier
     * @param weights_hash SHA-256 hash of weights
     */
    void registerWeights(
        const std::string& adapter_id,
        const std::string& weights_hash
    );
    
    /**
     * @brief Verify consistency across shards
     * 
     * Compares this shard's adapter hashes with other shards.
     * Requires network communication to other shards.
     * 
     * @param other_shard_hashes Map of shard_id -> adapter_hash
     * @return True if all shards match, false otherwise
     */
    bool verifyConsistency(
        const std::map<int, std::string>& other_shard_hashes
    );
    
private:
    int shard_id_;
    int total_shards_;
    std::map<std::string, std::string> local_weights_;  // adapter_id -> hash
};

} // namespace training
} // namespace themis
