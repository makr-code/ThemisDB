/**
 * @file ccpa_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

class IComplianceRule {
public:
    /**
     * @brief ICompliance Rule.
     * @return Return value.
     */
    virtual ~IComplianceRule() = default;

    [[nodiscard]] virtual std::string id() const = 0;

    [[nodiscard]] virtual std::string framework() const = 0;

    [[nodiscard]] virtual std::string description() const = 0;

    [[nodiscard]] virtual bool evaluate(const PolicyRule& rule) const = 0;
};

struct CcpaRuleEvalResult {
    std::string rule_id;          ///< ID of the evaluated PolicyRule
    std::string ccpa_check_id;    ///< ID of the CCPA rule that was evaluated
    bool compliant = false;       ///< Whether the rule is CCPA-compliant
    std::string description;      ///< Human-readable result description
    std::string recommendation;   ///< Remediation recommendation if not compliant

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct CcpaQueryContext {
    std::string subject_id;                    ///< Data subject / user ID
    bool opted_out_of_sale = false;            ///< Subject has opted out of data sale
    std::vector<std::string> data_categories;  ///< Categories of personal data involved
    std::string action;                        ///< Query action: "read", "write", "export"
};

struct DataSubjectRequest {
    std::string request_id;
    std::string subject_id;
    std::string request_type;  ///< "right_to_know" | "right_to_delete" | "opt_out_of_sale" | "data_portability"
    int64_t timestamp = 0;
    std::string status;        ///< "pending" | "fulfilled" | "denied"
    std::string denial_reason; ///< Populated when status == "denied"

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// CCPA Concrete Rule Evaluators
// ============================================================================

class RightToKnow final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_right_to_know"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.100: Consumer right to know what personal data is "
               "collected and how it is used. Requires audit_access=true on "
               "rules covering personal data resources.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class RightToDelete final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_right_to_delete"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.105: Consumer right to delete personal data. "
               "Requires that the rule does not block deletion (retention_days "
               "must allow deletion upon request; rule must audit changes).";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class OptOutOfSale final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_opt_out_of_sale"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.120: Consumer right to opt out of sale of personal "
               "data. Rules with allow_export=true on personal data resources "
               "must enforce opt-out preference before sharing with third parties.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class DataPortability final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_data_portability"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.100(d): Consumer right to receive personal data in "
               "a portable, machine-readable format. A rule satisfies this "
               "requirement when allow_export=true (automated export) OR "
               "audit_access=true (data is discoverable for manual fulfilment). "
               "A rule with both flags false leaves no path to honour a portability "
               "request and is non-compliant.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// CcpaRuleSet
// ============================================================================

class CcpaRuleSet {
public:
    CcpaRuleSet();


    /**
     * @brief Add Opt Out.
     * @param[in] subject_id Identifier of the subject.
     */
    void addOptOut(const std::string& subject_id);

    /**
     * @brief Remove Opt Out.
     * @param[in] subject_id Identifier of the subject.
     */
    void removeOptOut(const std::string& subject_id);

    /**
     * @brief Is Opted Out.
     * @param[in] subject_id Identifier of the subject.
     * @return True when the operation succeeds.
     */
    bool isOptedOut(const std::string& subject_id) const;

    /**
     * @brief Set Opt Out Registry.
     * @param[in] subjects Input parameter.
     */
    void setOptOutRegistry(const std::unordered_set<std::string>& subjects);

    /**
     * @brief Opt Out Count.
     * @return Return value.
     */
    size_t optOutCount() const;


    /**
     * @brief Evaluate Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<CcpaRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /**
     * @brief Is Rule Compliant.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRuleCompliant(const PolicyRule& rule) const;

    /**
     * @brief Detect Hipaa Conflicts.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectHipaaConflicts(const PolicyRule& rule) const;


    /**
     * @brief Record Request.
     * @param[in] request Input parameter.
     */
    void recordRequest(const DataSubjectRequest& request);

    /**
     * @brief Get Requests For Subject.
     * @param[in] subject_id Identifier of the subject.
     * @return Return value.
     */
    std::vector<DataSubjectRequest> getRequestsForSubject(const std::string& subject_id) const;

    std::vector<DataSubjectRequest> getRequestsByType(
        const std::string& request_type,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;

    int countOptOutRequests(int64_t start_time = 0, int64_t end_time = INT64_MAX) const;

    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const { return rules_; }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;

    mutable std::mutex opt_out_mutex_;
    std::unordered_set<std::string> opt_out_subjects_;

    mutable std::mutex requests_mutex_;
    std::vector<DataSubjectRequest> requests_;
};

} // namespace governance
} // namespace themis
