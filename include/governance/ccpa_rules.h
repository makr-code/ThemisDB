/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ccpa_rules.h                                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Data subject record for CCPA/CPRA compliance tracking
struct CcpaSubjectRecord {
    std::string subject_id;                            // Unique subject identifier
    std::vector<std::string> data_categories;          // Categories of personal information held
    bool opt_out_of_sale = false;                      // Cal. Civ. Code § 1798.120 opt-out flag
    bool opt_out_of_sharing = false;                   // CPRA § 1798.135 cross-context sharing opt-out
    bool deletion_requested = false;                   // Right to delete (§ 1798.105) pending request
    bool right_to_know_requested = false;              // Right to know (§ 1798.110) pending request
    bool portability_requested = false;                // Right to portability (§ 1798.100) pending
    std::vector<std::string> third_party_disclosures;  // Third parties data was shared with
    int64_t opt_out_timestamp = 0;                     // Unix ms when opt-out was recorded
    int64_t last_updated = 0;                          // Unix ms of last record update

    nlohmann::json toJson() const;
    static CcpaSubjectRecord fromJson(const nlohmann::json& j);
};

/// Input context for evaluating a CCPA rule against a specific access request
struct CcpaEvaluationContext {
    std::string subject_id;                            // Subject whose data is being accessed
    std::string requesting_party;                      // Identifier of the data requestor/consumer
    std::string action;                                // "read", "share", "sell", "delete", "export"
    std::vector<std::string> data_categories;          // Categories of data involved in the request
    bool is_third_party = false;                       // Whether requestor is a third party
    bool is_service_provider = false;                  // Service providers are exempt from sale rules
};

/// Result of evaluating a single CCPA rule
struct CcpaEvaluationResult {
    bool allowed = true;                               // Whether the action is permitted
    std::string rule_id;                               // Rule that produced this result
    std::string reason;                                // Human-readable justification
    std::vector<std::string> required_actions;         // Actions operator must take (e.g. "notify_subject")
    bool audit_required = true;                        // Whether this decision must be audit-logged
};

/// Aggregated CCPA compliance report for a time window
struct CcpaReport {
    std::string report_id;
    int64_t generated_at = 0;                          // Unix ms
    int64_t window_start = 0;                          // Report window start (Unix ms)
    int64_t window_end = 0;                            // Report window end (Unix ms)

    int total_subjects = 0;
    int opted_out_of_sale = 0;
    int opted_out_of_sharing = 0;
    int deletion_requests = 0;
    int right_to_know_requests = 0;
    int portability_requests = 0;

    // Data category inventory: category → count of subjects
    std::unordered_map<std::string, int> subjects_by_category;

    // Third-party disclosure summary: third_party → count of disclosures
    std::unordered_map<std::string, int> disclosures_by_third_party;

    nlohmann::json toJson() const;
};

/// CCPA/CPRA compliance rule set
///
/// Implements four data subject rights mandated by the California Consumer
/// Privacy Act (Cal. Civ. Code §§ 1798.100–1798.199) and the California
/// Privacy Rights Act (CPRA) amendments:
///   - Right to Know   (§ 1798.110 / § 1798.115)
///   - Right to Delete (§ 1798.105)
///   - Opt-Out of Sale (§ 1798.120)
///   - Data Portability (§ 1798.100)
///
/// All evaluators are pure functions (no side effects) so they can be
/// called from dry-run/simulation paths without touching the audit trail.
class CcpaRuleSet {
public:
    static constexpr const char* RULE_OPT_OUT_OF_SALE   = "ccpa.opt_out_of_sale";
    static constexpr const char* RULE_RIGHT_TO_DELETE    = "ccpa.right_to_delete";
    static constexpr const char* RULE_RIGHT_TO_KNOW      = "ccpa.right_to_know";
    static constexpr const char* RULE_DATA_PORTABILITY   = "ccpa.data_portability";

    /// Register a data subject record.  Replaces any existing record for the
    /// same subject_id.  Thread-safe via internal mutex.
    void registerSubject(const CcpaSubjectRecord& record);

    /// Remove a subject record (right-to-delete fulfillment).
    /// Returns true if the record existed and was removed.
    bool removeSubject(const std::string& subject_id);

    /// Retrieve a copy of a subject record.  Returns std::nullopt if not found.
    std::optional<CcpaSubjectRecord> getSubject(const std::string& subject_id) const;

    /// Evaluate the opt-out-of-sale rule.
    ///
    /// Blocks a "sell" or "share" action when the subject has opted out,
    /// unless the requestor qualifies as a service provider.
    CcpaEvaluationResult evaluateOptOutOfSale(const CcpaEvaluationContext& ctx) const;

    /// Evaluate the right-to-delete rule.
    ///
    /// Requires that any "read" or "share" action on a subject with a pending
    /// deletion request is blocked until the deletion is fulfilled.
    CcpaEvaluationResult evaluateRightToDelete(const CcpaEvaluationContext& ctx) const;

    /// Evaluate the right-to-know rule.
    ///
    /// Allows the data subject (or their authorised agent) to obtain a
    /// structured disclosure of the data categories and third-party
    /// disclosures recorded for them.
    CcpaEvaluationResult evaluateRightToKnow(const CcpaEvaluationContext& ctx) const;

    /// Evaluate the data portability rule.
    ///
    /// Permits a machine-readable export of the subject's personal information
    /// when a portability request is on record.
    CcpaEvaluationResult evaluateDataPortability(const CcpaEvaluationContext& ctx) const;

    /// Evaluate all applicable CCPA rules for the given context.
    ///
    /// Returns the most restrictive result (deny beats allow) across all
    /// evaluated rules.  Individual per-rule results are included in the
    /// returned vector via the out-parameter @p individual_results.
    CcpaEvaluationResult evaluateAll(
        const CcpaEvaluationContext& ctx,
        std::vector<CcpaEvaluationResult>* individual_results = nullptr
    ) const;

    /// Generate a CCPA compliance report for the given time window.
    ///
    /// @param window_start_ms  Inclusive lower bound (Unix epoch milliseconds).
    ///                         Pass 0 to include all records.
    /// @param window_end_ms    Exclusive upper bound (Unix epoch milliseconds).
    ///                         Pass INT64_MAX to include all records.
    CcpaReport generateReport(
        int64_t window_start_ms = 0,
        int64_t window_end_ms   = INT64_MAX
    ) const;

    /// Return the number of registered subject records.
    int subjectCount() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CcpaSubjectRecord> subjects_;
};

} // namespace governance
} // namespace themis
