/**
 * @file eligibility_policy_engine.h
 * @brief Policy engine for managing sample eligibility and lineage
 * @version 1.0
 * @date 2026-07-01
 *
 * Manages which samples are eligible for training based on defined policies,
 * tracks lineage transformations, and prevents duplicate/ineligible samples.
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/dataset_snapshot_manifest.h"
#include "training/lora_data_selection.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace themis {
namespace training {

/**
 * @brief Result of eligibility evaluation for a sample.
 */
struct EligibilityResult {
    /// Whether sample is eligible
    bool is_eligible = false;

    /// Reason if ineligible (e.g., "quality_score_too_low", "duplicate", "pii_detected")
    std::string rejection_reason;

    /// Remediation steps that could make sample eligible
    std::vector<std::string> remediation_suggestions;

    /// Policy version applied
    std::string policy_version;

    EligibilityResult() = default;
};

/**
 * @brief Policy engine for managing sample eligibility and tracking lineage.
 *
 * Responsibilities:
 * - Evaluate sample eligibility against defined policies
 * - Track lineage transformations from source to training data
 * - Prevent duplicate samples
 * - Maintain policy versions and audit trail
 * - Support policy versioning for reproducibility
 *
 * Usage:
 * @code
 * EligibilityPolicy policy;
 * policy.min_quality_score = 0.5;
 * policy.max_difficulty_score = 0.95;
 *
 * EligibilityPolicyEngine engine(policy);
 * for (const auto& sample : samples) {
 *     auto result = engine.evaluateSample(sample);
 *     if (result.is_eligible) {
 *         selected.push_back(sample);
 *     }
 * }
 * @endcode
 */
class EligibilityPolicyEngine {
public:
    /**
     * @brief Construct engine with given eligibility policy.
     */
    explicit EligibilityPolicyEngine(const EligibilityPolicy& policy);
    ~EligibilityPolicyEngine();

    EligibilityPolicyEngine(const EligibilityPolicyEngine&) = delete;
    EligibilityPolicyEngine& operator=(const EligibilityPolicyEngine&) = delete;

    /**
     * @brief Evaluate if a sample meets eligibility criteria.
     *
     * Checks:
     * - Quality score >= min_quality_score
     * - Difficulty score <= max_difficulty_score
     * - Language in required_languages (if specified)
     * - Domain in eligible_domains (if specified)
     * - Deduplication check (not a near-duplicate of accepted samples)
     * - PII presence (based on pii_handling policy)
     * - Toxicity score <= max_toxicity_score (if enabled)
     *
     * @param sample Sample to evaluate
     * @return EligibilityResult with decision and reason
     */
    EligibilityResult evaluateSample(const DataSample& sample) const;

    /**
     * @brief Record a sample's lineage and mark it as accepted.
     *
     * Should be called when a sample is accepted for training.
     * Stores lineage info and updates deduplication tracking.
     *
     * @param sample_id Unique sample identifier
     * @param lineage Lineage information for this sample
     * @return true if successfully recorded
     */
    bool recordSampleLineage(const std::string& sample_id,
                            const SampleLineage& lineage);

    /**
     * @brief Retrieve lineage for a previously recorded sample.
     * @param sample_id Sample identifier
     * @return Optional lineage if found, empty optional otherwise
     */
    std::vector<SampleLineage> getLineageHistory(const std::string& sample_id) const;

    /**
     * @brief Check if a sample is a near-duplicate of any accepted sample.
     *
     * Uses text hash similarity to detect duplicates.
     *
     * @param sample Sample to check
     * @param source_sample_ids Output: IDs of similar samples if any
     * @return true if duplicate found
     */
    bool isDuplicate(const DataSample& sample,
                     std::vector<std::string>* source_sample_ids = nullptr) const;

    /**
     * @brief Update the eligibility policy (live reload).
     * @param new_policy New policy to apply
     */
    void updatePolicy(const EligibilityPolicy& new_policy);

    /**
     * @brief Get current eligibility policy.
     */
    const EligibilityPolicy& getCurrentPolicy() const;

    /**
     * @brief Get policy history (all policies applied, with timestamps).
     */
    std::vector<std::pair<std::string, EligibilityPolicy>> getPolicyHistory() const;

    /**
     * @brief Get audit log of all eligibility evaluations.
     * @param limit Maximum number of recent entries to return (0 = all)
     * @return Audit log entries
     */
    std::vector<std::string> getAuditLog(size_t limit = 0) const;

    /**
     * @brief Get statistics on eligibility evaluations.
     * @return Map of rejection reasons to counts
     */
    std::map<std::string, size_t> getEligibilityStatistics() const;

    /**
     * @brief Clear all recorded lineage and audit history.
     *
     * Useful for testing or when starting a fresh dataset.
     */
    void clearHistory();

    /**
     * @brief Validate a sample ID for uniqueness across lineage.
     * @param sample_id Sample identifier to check
     * @return true if sample_id is not already recorded
     */
    bool validateSampleIdUniqueness(const std::string& sample_id) const;

    /**
     * @brief Get total number of samples evaluated.
     */
    size_t getTotalEvaluated() const;

    /**
     * @brief Get number of samples accepted so far.
     */
    size_t getTotalAccepted() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
