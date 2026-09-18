/**
 * @file policy_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "governance/data_masker.h"
#include "governance/governance_diagnostics.h"

// Forward-declare ModelGovernancePolicy so policy_engine.h stays lean
// (full type only needed in policy_engine.cpp)
namespace themis {
namespace governance {
class ModelGovernancePolicy;
struct ModelTrainingExportRequest;
struct ModelGovernanceDecision;
class SafeAccessValidator;
struct AccessRequest;
struct SafeAccessResult;
} // namespace governance
} // namespace themis

namespace themis {
namespace utils {
class AuditLogger;
}

namespace governance {

struct ClassificationProfile {
    std::string level; // offen, vs-nfd, geheim, streng-geheim
    bool encryption_required    = false;
    bool ann_allowed            = true;
    bool export_allowed         = true;
    bool cache_allowed          = true;
    std::string redaction_level = "standard";
    int retention_days          = 365;
    bool log_encryption         = false;
};

struct PolicyDecision {
    // Normalized classification: "offen", "vs-nfd", "geheim", "streng-geheim"
    std::string classification;
    // Mode: "enforce" | "observe"
    std::string mode;
    // Whether logs must be encrypted (Encrypt-then-Sign handled by logger)
    bool encrypt_logs = true;
    // Redaction profile: "none" | "standard" | "strict"
    std::string redaction = "standard";

    // Derived, route-relevant decisions
    bool ann_allowed                = true;  // Approximate NN allowed
    bool require_content_encryption = false; // Content blobs must be encrypted
    bool export_allowed             = true;
    bool cache_allowed              = true;
    int retention_days              = 365;

    // CCPA/CPRA: set to true when the data subject has opted out of data sale.
    // When true, callers must not share or export this subject's data to third
    // parties.  PolicyEngine::evaluate() sets this flag automatically when a
    // subject ID is present in the headers and the subject is registered in the
    // opt-out registry via setCcpaOptOutSubjects().
    bool ccpa_opted_out = false;
};

struct SimulationRequest {
    std::unordered_map<std::string, std::string> headers;
    std::string route;
};

struct SimulationResult {
    PolicyDecision decision;      // The computed access decision
    std::string matched_profile;  // Classification profile used ("" = heuristic fallback)
    std::string matched_resource; // Resource-mapping key that resolved the classification
    bool dry_run = true;          // Always true; confirms no audit entry was written
};

struct QueryPermissionResult {
    PolicyDecision decision;

    FieldMaskingPolicy masking_policy;
};

struct InferencePermissionResult {
    bool allowed = false;

    int http_status = 401;

    std::string denial_reason;

    PolicyDecision decision;
};

class PolicyEngine {
  public:
    struct IPolicyEvaluator {
        /**
         * @brief IPolicy Evaluator.
         * @return Return value.
         */
        virtual ~IPolicyEvaluator() = default;
        virtual std::optional<PolicyDecision> evaluate(
            const std::unordered_map<std::string, std::string>& headers,
            const std::string& route) const = 0;
    };

    PolicyEngine() = default;

    /**
     * @brief Load policies from YAML file (returns false on error)
     * @param[in] yaml_path Path to the yaml.
     * @return True when the operation succeeds.
     */
    bool loadFromYAML(const std::string &yaml_path);

    bool reloadIfChanged(std::string *err = nullptr);

    /**
     * @brief Get Loaded File Path.
     * @return Return value.
     */
    std::string getLoadedFilePath() const;

    /**
     * @brief Set audit logger for automatic logging of policy evaluations
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Set Opa Evaluator.
     * @param[in,out] evaluator Input/output parameter.
     */
    void setOpaEvaluator(IPolicyEvaluator* evaluator);

    /**
     * @brief ---- CCPA/CPRA opt-out registry ----------------------------------------
     * @param[in] opt_out_registry Input parameter.
     */

    void setCcpaOptOutSubjects(std::shared_ptr<std::unordered_set<std::string>> opt_out_registry);

    /**
     * @brief Is Ccpa Opted Out.
     * @param[in] subject_id Identifier of the subject.
     * @return True when the operation succeeds.
     */
    bool isCcpaOptedOut(const std::string &subject_id) const;

    /**
     * @brief ---- AI/ML Model Governance --------------------------------------------
     * @param[in] policy Input parameter.
     */

    void setModelGovernancePolicy(std::shared_ptr<ModelGovernancePolicy> policy);

    /**
     * @brief Check Export Permission.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    ModelGovernanceDecision checkExportPermission(const ModelTrainingExportRequest &request) const;

    // Evaluate headers for a given route key (e.g., "/vector/search" or handler name)
    // If audit logger is set and mode is "enforce", logs the policy decision
    PolicyDecision evaluate(const std::unordered_map<std::string, std::string> &headers,
                            const std::string &route) const;

    QueryPermissionResult checkQueryPermission(const std::unordered_map<std::string, std::string> &headers,
                                               const std::string &route) const;

    InferencePermissionResult checkInferencePermission(
        const std::unordered_map<std::string, std::string>& headers) const;

    /**
     * @brief Get Masking Policy.
     * @return Return value.
     */
    FieldMaskingPolicy getMaskingPolicy() const;

    /**
     * @brief Simulate Decision.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    SimulationResult simulateDecision(const SimulationRequest &request) const;

    /**
     * @brief Validate Access Safety.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    SafeAccessResult validateAccessSafety(const AccessRequest& request);

    /**
     * @brief Get Safe Access Validator.
     * @return Return value.
     */
    SafeAccessValidator& getSafeAccessValidator();

    /**
     * @brief Get classification profile by name
     * @param[in] level Input parameter.
     * @return Return value.
     */
    std::optional<ClassificationProfile> getClassificationProfile(const std::string &level) const;

    /**
     * @brief Is Strict Class.
     * @param[in] cls Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isStrictClass(const std::string &cls);

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ClassificationProfile> classification_profiles_;
    std::unordered_map<std::string, std::string> resource_mapping_;
    std::string default_mode_ = "enforce";
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;

    // Hot-reload state
    std::string loaded_yaml_path_;
    std::filesystem::file_time_type last_loaded_mtime_{};

    // CCPA/CPRA opt-out registry (may be null – treated as empty)
    std::shared_ptr<std::unordered_set<std::string>> ccpa_opt_out_subjects_;

    // AI/ML model governance policy (optional; used by checkExportPermission())
    std::shared_ptr<ModelGovernancePolicy> model_governance_policy_;

    // Data masking rules loaded from the YAML `data_masking` section.
    FieldMaskingPolicy masking_rules_;

    // External OPA evaluator (optional; raw non-owning pointer).
    IPolicyEvaluator* opa_evaluator_ = nullptr;

    // Safety validator for Phase 3B Extended (fail-closed access checks)
    std::unique_ptr<SafeAccessValidator> safety_validator_;

    /**
     * @brief Normalize.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string normalize(const std::string &s);
};

} // namespace governance
} // namespace themis
