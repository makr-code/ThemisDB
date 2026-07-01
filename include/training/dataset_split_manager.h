/**
 * @file dataset_split_manager.h
 * @brief Deterministic split management for train/val/test datasets
 * @version 1.0
 * @date 2026-07-01
 *
 * Manages reproducible, stratified train/validation/test splits with
 * strict leakage prevention and deterministic allocation.
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/dataset_snapshot_manifest.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace themis {
namespace training {

/**
 * @brief Configuration for split generation.
 */
struct SplitConfig {
    /// Fraction of samples for training set [0..1]
    double train_ratio = 0.7;

    /// Fraction of samples for validation set [0..1]
    double validation_ratio = 0.15;

    /// Fraction of samples for test set [0..1] (computed as 1 - train - validation)
    double test_ratio = 0.15;

    /// Seed for deterministic random number generation (for reproducibility)
    /// If 0, uses timestamp-based seed
    uint64_t random_seed = 0;

    /// Enable stratified sampling by domain
    bool stratify_by_domain = false;

    /// Enable stratified sampling by difficulty
    bool stratify_by_difficulty = false;

    /// Number of folds for cross-validation (0 = no cross-validation)
    uint32_t num_folds = 0;

    /// Whether to shuffle samples before splitting
    bool shuffle = true;

    SplitConfig() = default;

    /**
     * @brief Validate split ratios sum to ~1.0
     * @return true if ratios are valid
     */
    bool validate() const;

    /**
     * @brief Get a human-readable string representation
     */
    std::string toString() const;
};

/**
 * @brief Split assignment error codes
 */
enum class SplitError {
    SUCCESS = 0,
    INVALID_RATIOS = 1,
    INSUFFICIENT_SAMPLES = 2,
    DUPLICATE_SAMPLE_ID = 3,
    INVALID_SEED = 4,
    WRITE_FAILED = 5,
};

/**
 * @brief Result of split generation operation
 */
struct SplitResult {
    /// Whether operation succeeded
    bool success = false;

    /// Error code if failed
    SplitError error = SplitError::SUCCESS;

    /// Error message
    std::string error_message;

    /// Split assignments generated
    std::vector<SplitAssignment> assignments;

    /// Verification checksum (for integrity checking)
    std::string checksum;

    SplitResult() = default;
};

/**
 * @brief Manages deterministic train/val/test splits with leakage prevention.
 *
 * Key features:
 * - Deterministic split generation using configurable random seed
 * - Stratified sampling by domain and difficulty
 * - Strict train/val/test separation (no sample duplication across splits)
 * - Support for cross-validation (k-fold)
 * - Reproducibility: same seed + same input = same split assignment
 * - Audit trail of all split operations
 *
 * Usage:
 * @code
 * SplitConfig config;
 * config.train_ratio = 0.8;
 * config.validation_ratio = 0.1;
 * config.test_ratio = 0.1;
 * config.random_seed = 42; // For reproducibility
 *
 * DatasetSplitManager manager(config);
 * auto result = manager.generateSplits(samples);
 *
 * if (result.success) {
 *     for (const auto& assignment : result.assignments) {
 *         // Use assignment.split to route sample
 *     }
 * }
 * @endcode
 */
class DatasetSplitManager {
public:
    /**
     * @brief Construct manager with split configuration.
     */
    explicit DatasetSplitManager(const SplitConfig& config);
    ~DatasetSplitManager();

    DatasetSplitManager(const DatasetSplitManager&) = delete;
    DatasetSplitManager& operator=(const DatasetSplitManager&) = delete;

    /**
     * @brief Generate deterministic splits for a set of samples.
     *
     * Produces train/val/test assignments with:
     * - No sample duplication across splits (strict separation)
     * - Deterministic allocation (reproducible with same seed)
     * - Optional stratification by domain/difficulty
     * - Optional k-fold cross-validation
     *
     * @param samples Input samples to split
     * @return SplitResult with assignments and verification checksum
     */
    SplitResult generateSplits(const std::vector<DataSample>& samples);

    /**
     * @brief Generate splits with explicit sample IDs and metadata.
     *
     * Useful when samples are already stored in a database.
     *
     * @param sample_ids Sample identifiers
     * @param sample_difficulty_scores Optional difficulty scores for stratification
     * @param sample_domains Optional domain assignments for stratification
     * @return SplitResult with assignments
     */
    SplitResult generateSplitsFromIds(
        const std::vector<std::string>& sample_ids,
        const std::map<std::string, double>* sample_difficulty_scores = nullptr,
        const std::map<std::string, std::string>* sample_domains = nullptr);

    /**
     * @brief Verify that a split result has no leakage between sets.
     *
     * Checks:
     * - No sample appears in multiple splits
     * - Split ratios match configuration (within rounding tolerance)
     * - Checksum is valid
     *
     * @param result Split result to verify
     * @return true if verification passes
     */
    bool verifySplitIntegrity(const SplitResult& result) const;

    /**
     * @brief Get the sample IDs assigned to a specific split.
     * @param result Split result
     * @param split_name "train", "validation", or "test"
     * @return Vector of sample IDs in that split
     */
    std::vector<std::string> getSamplesInSplit(
        const SplitResult& result,
        const std::string& split_name) const;

    /**
     * @brief Create a cross-validation fold mapping.
     *
     * For k-fold cross-validation, designates different validation sets
     * and consolidates training/test accordingly.
     *
     * @param result Original split result
     * @param fold_index Fold number (0..num_folds-1)
     * @return New split result with fold repositioned
     */
    SplitResult createCrossValidationFold(
        const SplitResult& result,
        uint32_t fold_index) const;

    /**
     * @brief Update the split configuration and regenerate.
     * @param new_config New configuration to apply
     * @param samples Samples to re-split (if already generated)
     * @return SplitResult with new configuration applied
     */
    SplitResult reconfigure(const SplitConfig& new_config,
                           const std::vector<DataSample>* samples = nullptr);

    /**
     * @brief Get current split configuration.
     */
    const SplitConfig& getConfig() const;

    /**
     * @brief Get audit log of all split operations.
     * @param limit Maximum entries to return (0 = all)
     * @return List of audit log entries as strings
     */
    std::vector<std::string> getAuditLog(size_t limit = 0) const;

    /**
     * @brief Get statistics on generated splits.
     * @return Map of split name to sample count
     */
    std::map<std::string, size_t> getSplitStatistics(const SplitResult& result) const;

    /**
     * @brief Get stratification verification report.
     *
     * When stratification is enabled, returns statistics on how well
     * the stratification was applied (e.g., domain/difficulty distribution).
     *
     * @param result Split result to analyze
     * @return Human-readable stratification report
     */
    std::string getStratificationReport(const SplitResult& result) const;

    /**
     * @brief Export split assignments to JSON file.
     * @param result Split result to export
     * @param file_path Path where file will be written
     * @return true if successful
     */
    bool exportSplitsToJSON(const SplitResult& result,
                           const std::string& file_path) const;

    /**
     * @brief Import split assignments from JSON file.
     * @param file_path Path to JSON file
     * @return SplitResult loaded from file
     * @throws std::runtime_error if file cannot be read
     */
    static SplitResult importSplitsFromJSON(const std::string& file_path);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    /// Helper to compute deterministic checksum of assignments
    std::string computeAssignmentChecksum(const std::vector<SplitAssignment>& assignments) const;
};

} // namespace training
} // namespace themis
