/**
 * @file policy_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=13; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=10, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Request passed to simulateDecision() for dry-run policy preview.
struct SimulationRequest {
    std::unordered_map<std::string, std::string> headers;
    std::string route;
};

/// Result returned by simulateDecision().
/// Contains the computed PolicyDecision plus dry-run metadata.
/// No audit entry is written when this result is produced.
struct SimulationResult {
    PolicyDecision decision;      // The computed access decision
    std::string matched_profile;  // Classification profile used ("" = heuristic fallback)
    std::string matched_resource; // Resource-mapping key that resolved the classification
    bool dry_run = true;          // Always true; confirms no audit entry was written
};

/// Result returned by PolicyEngine::checkQueryPermission().
/// Bundles the standard PolicyDecision together with the FieldMaskingPolicy
/// that the query executor must apply before serialising the result.
struct QueryPermissionResult {
    /// Standard policy decision (classification, redaction level, flags, etc.)
    PolicyDecision decision;

    /// Data masking rules to apply to each result document.
    /// The query executor calls DataMasker::maskFields(doc, masking_policy)
    /// on every document before returning it to the client.
    FieldMaskingPolicy masking_policy;
};

/// Result returned by PolicyEngine::checkInferencePermission().
/// Communicates whether a caller may submit an LLM inference request and,
/// on denial, why the request was rejected so that the HTTP layer can return
/// a properly structured error response (HTTP 401 or 403).
struct InferencePermissionResult {
    /// Whether the inference request is permitted.
    bool allowed = false;

    /// When @c allowed is false, the HTTP status code to return to the client.
    /// 401 = missing or invalid API key; 403 = valid identity, but denied by
    /// policy (e.g. data-classification restriction, rate-limit exceeded).
    int http_status = 401;

    /// Human-readable denial reason forwarded in the OpenAI-style error body.
    std::string denial_reason;

    /// Standard policy decision so callers can inspect classification flags.
    PolicyDecision decision;
};

/// @brief Policy engine for data governance, classification, and field-level masking.
class PolicyEngine {
  public:
    /**
     * @brief Pluggable policy evaluator interface for external engines (e.g. OPA).
     *
     * Implement this interface and pass it to setOpaEvaluator() to route
     * governance decisions through an external policy agent.
     * evaluate() returns std::nullopt when the external evaluator is
     * unavailable so PolicyEngine can fall back to native evaluation.
     */
    struct IPolicyEvaluator {
        virtual ~IPolicyEvaluator() = default;
        virtual std::optional<PolicyDecision> evaluate(
            const std::unordered_map<std::string, std::string>& headers,
            const std::string& route) const = 0;
    };

    PolicyEngine() = default;

    // Load policies from YAML file (returns false on error)
    bool loadFromYAML(const std::string &yaml_path);

    /**
     * @brief Reload policies if the source YAML file has changed on disk.
     *
     * Checks the modification time of the file last passed to loadFromYAML().
     * If the file has been modified, the new policy set is loaded atomically.
     * If the path is empty or the file has not changed, this is a fast no-op.
     *
     * @param err  Optional: populated with a human-readable error message on
     *             failure (file read error, parse error, etc.).
     * @return true  if policies are up-to-date (either reloaded or unchanged),
     *         false on error.
     */
    bool reloadIfChanged(std::string *err = nullptr);

    /// @return The file path last passed to loadFromYAML(), or empty string if
    ///         no file has been loaded yet.
    std::string getLoadedFilePath() const;

    // Set audit logger for automatic logging of policy evaluations
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Attach an external policy evaluator (e.g. OPA) for governance decisions.
     *
     * When set, evaluate() calls evaluator->evaluate() first.  If the
     * evaluator returns std::nullopt (OPA unreachable / timeout), native
     * PolicyEngine evaluation is used as a fallback and a
     * governance_opa_fallback_total Prometheus counter is incremented.
     *
     * Pass nullptr to detach.  The PolicyEngine does NOT take ownership; the
     * caller must ensure the evaluator outlives the engine.
     */
    void setOpaEvaluator(IPolicyEvaluator* evaluator);

    // ---- CCPA/CPRA opt-out registry ----------------------------------------

    /// Register a set of data subject IDs that have opted out of data sale.
    /// PolicyEngine::evaluate() will set PolicyDecision::ccpa_opted_out=true
    /// and PolicyDecision::export_allowed=false for any request whose
    /// "X-User-Id" header matches a subject in this registry.
    /// Thread-safe; atomically replaces the previous registry.
    void setCcpaOptOutSubjects(std::shared_ptr<std::unordered_set<std::string>> opt_out_registry);

    /// Return true if the given subject ID is registered as opted-out.
    bool isCcpaOptedOut(const std::string &subject_id) const;

    // ---- AI/ML Model Governance --------------------------------------------

    /// Attach a ModelGovernancePolicy used by checkExportPermission().
    /// Thread-safe; atomically replaces the previous instance.
    void setModelGovernancePolicy(std::shared_ptr<ModelGovernancePolicy> policy);

    /// Evaluate whether a training-data export is permitted.
    ///
    /// Delegates to the configured ModelGovernancePolicy (if set).  When no
    /// ModelGovernancePolicy has been attached, the method applies the built-in
    /// classification fallback: "geheim" and "streng-geheim" datasets are
    /// always denied; all other classifications are permitted.
    ///
    /// Must be called before any training-purpose export begins.
    /// @return ModelGovernanceDecision with is_permitted and, on denial,
    ///         denial_reason; on approval, lineage_event_id is populated.
    ModelGovernanceDecision checkExportPermission(const ModelTrainingExportRequest &request) const;

    // Evaluate headers for a given route key (e.g., "/vector/search" or handler name)
    // If audit logger is set and mode is "enforce", logs the policy decision
    PolicyDecision evaluate(const std::unordered_map<std::string, std::string> &headers,
                            const std::string &route) const;

    /**
     * @brief Evaluate query permissions and return the applicable masking policy.
     *
     * Combines the standard policy evaluation (classification, redaction, CCPA,
     * etc.) with the data masking rules configured for the current context.
     * The caller must apply the returned `FieldMaskingPolicy` to every result
     * document via `DataMasker::maskFields()` before serialising the response.
     *
     * Writes an audit entry when mode is "enforce" (identical to evaluate()).
     *
     * @param headers  Request headers (same set accepted by evaluate()).
     * @param route    API route key (e.g. "/vector/search").
     * @return QueryPermissionResult containing the PolicyDecision and the
     *         FieldMaskingPolicy to apply to query results.
     */
    QueryPermissionResult checkQueryPermission(const std::unordered_map<std::string, std::string> &headers,
                                               const std::string &route) const;

    /**
     * @brief Validate that the caller is authorised to submit an LLM inference
     *        request to the @c /v1/chat/completions endpoint.
     *
     * Extracts the caller identity from the @c Authorization header
     * (`Bearer <api-key>`), evaluates the standard governance policy for the
     * @c /v1/chat/completions route, and returns an @c InferencePermissionResult
     * that the HTTP layer can act on:
     *   - @c allowed=true  → proceed with inference
     *   - @c allowed=false → return HTTP @c http_status with the @c denial_reason
     *
     * The method never throws; governance errors are reflected in the result's
     * @c denial_reason field so the caller can propagate a structured OpenAI-style
     * error body.
     *
     * @param headers  HTTP request headers.  The @c Authorization header must
     *                 contain a `Bearer <api-key>` value for the identity to be
     *                 extracted; missing or malformed tokens result in HTTP 401.
     * @return @c InferencePermissionResult with the access decision and,
     *         on denial, an HTTP status code and a human-readable reason.
     */
    InferencePermissionResult checkInferencePermission(
        const std::unordered_map<std::string, std::string>& headers) const;

    /// @return A snapshot of the currently loaded FieldMaskingPolicy.
    FieldMaskingPolicy getMaskingPolicy() const;

    /// Evaluate policies in dry-run (simulation) mode without writing an audit entry.
    ///
    /// Performs the same classification lookup, profile resolution, and header-override
    /// steps as evaluate(), but intentionally suppresses audit logging so that the
    /// caller can preview the access decision without any side effects on the audit
    /// trail.  This satisfies the "deterministic and side-effect-free" requirement for
    /// policy_validator.cpp dry-run usage described in FUTURE_ENHANCEMENTS.md.
    ///
    /// @param request  The simulation request (headers + route).
    /// @return SimulationResult containing the decision and which rule/profile was matched.
    SimulationResult simulateDecision(const SimulationRequest &request) const;

    /**
     * @brief Validate access request for safety before policy evaluation.
     *
     * Returns SafeAccessResult. If !result.is_safe, the policy evaluation
     * should be skipped and access denied.
     *
     * @param request AccessRequest to validate
     * @return SafeAccessResult with detailed findings
     */
    SafeAccessResult validateAccessSafety(const AccessRequest& request);

    /**
     * @brief Get mutable reference to the safety validator.
     * @return SafeAccessValidator instance
     */
    SafeAccessValidator& getSafeAccessValidator();

    // Get classification profile by name
    std::optional<ClassificationProfile> getClassificationProfile(const std::string &level) const;

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

    static std::string normalize(const std::string &s);
};

} // namespace governance
} // namespace themis
