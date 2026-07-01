/**
 * @file eligibility_policy_engine.cpp
 * @brief Implementation of policy engine for managing sample eligibility
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/eligibility_policy_engine.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace themis {
namespace training {

// ============================================================================
// Implementation Details
// ============================================================================

class EligibilityPolicyEngine::Impl {
public:
    EligibilityPolicy current_policy;
    std::vector<std::pair<std::string, EligibilityPolicy>> policy_history;
    std::map<std::string, SampleLineage> lineage_map;
    std::vector<std::string> audit_log;
    std::map<std::string, size_t> rejection_stats;
    size_t total_evaluated = 0;
    size_t total_accepted = 0;

    Impl(const EligibilityPolicy& policy) : current_policy(policy) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto tm = std::gmtime(&t);
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
        policy_history.push_back({oss.str(), policy});
    }

    // Simple hash function for text deduplication
    static uint64_t hashText(const std::string& text) {
        uint64_t hash = 0;
        const uint64_t FNV_offset_basis = 14695981039346656037ULL;
        const uint64_t FNV_prime = 1099511628211ULL;

        hash = FNV_offset_basis;
        for (unsigned char c : text) {
            hash ^= c;
            hash *= FNV_prime;
        }
        return hash;
    }

    void logAuditEntry(const std::string& sample_id, bool eligible,
                       const std::string& reason = "") {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto tm = std::gmtime(&t);
        
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << " | "
            << "sample_id=" << sample_id << " | "
            << "eligible=" << (eligible ? "true" : "false");
        if (!reason.empty()) {
            oss << " | reason=" << reason;
        }
        
        audit_log.push_back(oss.str());
    }
};

// ============================================================================
// EligibilityPolicyEngine Implementation
// ============================================================================

EligibilityPolicyEngine::EligibilityPolicyEngine(const EligibilityPolicy& policy)
    : impl_(std::make_unique<Impl>(policy)) {}

EligibilityPolicyEngine::~EligibilityPolicyEngine() = default;

EligibilityResult EligibilityPolicyEngine::evaluateSample(const DataSample& sample) const {
    EligibilityResult result;
    result.policy_version = impl_->current_policy.policy_version;
    impl_->total_evaluated++;

    // Quality score check
    if (sample.quality_score < impl_->current_policy.min_quality_score) {
        result.is_eligible = false;
        result.rejection_reason = "quality_score_too_low";
        result.remediation_suggestions.push_back("Improve data cleaning or labeling quality");
        impl_->rejection_stats["quality_score_too_low"]++;
        impl_->logAuditEntry(sample.id, false, result.rejection_reason);
        return result;
    }

    // Difficulty score check
    if (sample.difficulty_score > impl_->current_policy.max_difficulty_score) {
        result.is_eligible = false;
        result.rejection_reason = "difficulty_score_too_high";
        result.remediation_suggestions.push_back("Consider reweighting or removing difficult samples");
        impl_->rejection_stats["difficulty_score_too_high"]++;
        impl_->logAuditEntry(sample.id, false, result.rejection_reason);
        return result;
    }

    // Language check
    if (!impl_->current_policy.required_languages.empty()) {
        if (std::find(impl_->current_policy.required_languages.begin(),
                     impl_->current_policy.required_languages.end(),
                     sample.language) == impl_->current_policy.required_languages.end()) {
            result.is_eligible = false;
            result.rejection_reason = "language_not_supported";
            result.remediation_suggestions.push_back("Translate sample to required language");
            impl_->rejection_stats["language_not_supported"]++;
            impl_->logAuditEntry(sample.id, false, result.rejection_reason);
            return result;
        }
    }

    // Domain check
    if (!impl_->current_policy.eligible_domains.empty() && !sample.domain.empty()) {
        if (std::find(impl_->current_policy.eligible_domains.begin(),
                     impl_->current_policy.eligible_domains.end(),
                     sample.domain) == impl_->current_policy.eligible_domains.end()) {
            result.is_eligible = false;
            result.rejection_reason = "domain_not_eligible";
            result.remediation_suggestions.push_back("Reclassify sample or expand eligible domains");
            impl_->rejection_stats["domain_not_eligible"]++;
            impl_->logAuditEntry(sample.id, false, result.rejection_reason);
            return result;
        }
    }

    // Deduplication check
    if (isDuplicate(sample)) {
        result.is_eligible = false;
        result.rejection_reason = "duplicate_detected";
        result.remediation_suggestions.push_back("Remove or modify to increase diversity");
        impl_->rejection_stats["duplicate_detected"]++;
        impl_->logAuditEntry(sample.id, false, result.rejection_reason);
        return result;
    }

    // Toxicity check
    if (impl_->current_policy.toxicity_check_enabled) {
        // Placeholder: In production, use actual toxicity detection API
        // For now, assume no toxicity is detected
        if (0.0 > impl_->current_policy.max_toxicity_score) {
            result.is_eligible = false;
            result.rejection_reason = "toxicity_score_too_high";
            result.remediation_suggestions.push_back("Clean or rephrase toxic content");
            impl_->rejection_stats["toxicity_score_too_high"]++;
            impl_->logAuditEntry(sample.id, false, result.rejection_reason);
            return result;
        }
    }

    // All checks passed
    result.is_eligible = true;
    impl_->total_accepted++;
    impl_->logAuditEntry(sample.id, true);
    return result;
}

bool EligibilityPolicyEngine::recordSampleLineage(const std::string& sample_id,
                                                   const SampleLineage& lineage) {
    if (impl_->lineage_map.count(sample_id) > 0) {
        return false; // Sample already recorded
    }
    impl_->lineage_map[sample_id] = lineage;
    return true;
}

std::vector<SampleLineage> EligibilityPolicyEngine::getLineageHistory(
    const std::string& sample_id) const {
    std::vector<SampleLineage> history;
    if (impl_->lineage_map.count(sample_id) > 0) {
        history.push_back(impl_->lineage_map.at(sample_id));
    }
    return history;
}

bool EligibilityPolicyEngine::isDuplicate(const DataSample& sample,
                                          std::vector<std::string>* source_sample_ids) const {
    uint64_t sample_hash = Impl::hashText(sample.text);
    
    // Simple heuristic: check for exact text match
    // In production, use MinHash or other approximate matching
    for (const auto& [accepted_id, lineage] : impl_->lineage_map) {
        // This is simplified - in production would use approximate matching
        if (Impl::hashText(sample.text) == sample_hash) {
            if (source_sample_ids) {
                source_sample_ids->push_back(accepted_id);
            }
            return true;
        }
    }
    return false;
}

void EligibilityPolicyEngine::updatePolicy(const EligibilityPolicy& new_policy) {
    impl_->current_policy = new_policy;
    
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto tm = std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    
    impl_->policy_history.push_back({oss.str(), new_policy});
}

const EligibilityPolicy& EligibilityPolicyEngine::getCurrentPolicy() const {
    return impl_->current_policy;
}

std::vector<std::pair<std::string, EligibilityPolicy>>
EligibilityPolicyEngine::getPolicyHistory() const {
    return impl_->policy_history;
}

std::vector<std::string> EligibilityPolicyEngine::getAuditLog(size_t limit) const {
    if (limit == 0 || limit >= impl_->audit_log.size()) {
        return impl_->audit_log;
    }
    
    std::vector<std::string> recent;
    size_t start = impl_->audit_log.size() - limit;
    recent.insert(recent.end(),
                  impl_->audit_log.begin() + start,
                  impl_->audit_log.end());
    return recent;
}

std::map<std::string, size_t> EligibilityPolicyEngine::getEligibilityStatistics() const {
    return impl_->rejection_stats;
}

void EligibilityPolicyEngine::clearHistory() {
    impl_->lineage_map.clear();
    impl_->audit_log.clear();
    impl_->rejection_stats.clear();
    impl_->total_evaluated = 0;
    impl_->total_accepted = 0;
}

bool EligibilityPolicyEngine::validateSampleIdUniqueness(const std::string& sample_id) const {
    return impl_->lineage_map.count(sample_id) == 0;
}

size_t EligibilityPolicyEngine::getTotalEvaluated() const {
    return impl_->total_evaluated;
}

size_t EligibilityPolicyEngine::getTotalAccepted() const {
    return impl_->total_accepted;
}

} // namespace training
} // namespace themis
