/**
 * @file query_masking_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/pii_detection_engine.h"
#include "utils/pii_detector.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>

namespace themis {
namespace security {

/**
 * @brief Dynamic data masking policy for PII fields in query results.
 *
 * Applies field-level masking to JSON query result objects before they are
 * returned to clients.  Two complementary masking strategies are combined:
 *
 *  1. **Field-name hint masking** – if a JSON key (e.g. "email", "ssn",
 *     "credit_card") matches a PII hint known to PIIDetector, its string value
 *     is replaced by the appropriate masked form.
 *  2. **Auto-detect masking** – all string values in the result are scanned
 *     with PIIDetector; any detected PII spans are replaced in-place.
 *  3. **Explicitly declared fields** – callers can register specific field
 *     paths (e.g. "payment.card_number") together with a masking mode
 *     ("strict", "partial") that overrides automatic detection.
 *
 * Role-based bypass:
 *   Roles listed in `privileged_roles` receive the original, unmasked result.
 *   The default privileged role is "admin".
 *
 * Thread-safety:
 *   All public methods are safe to call from multiple threads concurrently.
 *
 * Usage:
 * @code
 * auto policy = QueryMaskingPolicy::create();
 * policy->declareField("payment_info.ssn", "strict");
 *
 * auto masked = policy->maskResult(raw_entity_json, user_roles);
 * @endcode
 */
class QueryMaskingPolicy {
public:
    /**
     * @brief Per-field masking configuration.
     */
    struct FieldMaskConfig {
        std::string mask_mode;  ///< "strict" | "partial" | "none"
        utils::PIIType pii_type = utils::PIIType::UNKNOWN;  ///< Hint for masking form
    };

    /**
     * @brief Policy-wide configuration.
     */
    struct Config {
        /// Enable/disable masking entirely (kill-switch).
        bool enabled = true;
        /// Scan string values for inline PII (e.g. phone/email embedded in free text).
        bool auto_detect_pii = true;
        /// Use field-name hints from PIIDetector to mask values.
        bool mask_by_field_name = true;
        /// Roles whose members receive fully unmasked results.
        std::unordered_set<std::string> privileged_roles = {"admin"};
    };

    /**
     * @brief Construct with default configuration.
     *
     * @param config_path  Optional path to pii_patterns.yaml passed to PIIDetector.
     */
    explicit QueryMaskingPolicy(
        const std::string& config_path = "config/pii_patterns.yaml");
    /**
     * @brief Construct with explicit configuration.
     *
     * @param config       Policy configuration.
     * @param config_path  Optional path to pii_patterns.yaml passed to PIIDetector.
     */
    QueryMaskingPolicy(
        Config config,
        const std::string& config_path = "config/pii_patterns.yaml");

    /// Factory: create with default configuration.
    static std::shared_ptr<QueryMaskingPolicy> create(
        const std::string& config_path = "config/pii_patterns.yaml");
    /// Factory: create with explicit configuration.
    static std::shared_ptr<QueryMaskingPolicy> create(
        Config config,
        const std::string& config_path = "config/pii_patterns.yaml");

    // -------------------------------------------------------------------------
    // Explicit field declarations
    // -------------------------------------------------------------------------

    /**
     * @brief Register a field (by exact key name) that must always be masked.
     *
     * Overrides both auto-detect and field-name hints for this key.
     *
     * @param field_name  JSON key name (e.g. "ssn", "credit_card_number").
     * @param mask_mode   "strict" (full replace) or "partial" (show partial).
     * @param pii_type    Hint for choosing the masking form (optional).
     */
    void declareField(
        const std::string& field_name,
        const std::string& mask_mode = "strict",
        utils::PIIType pii_type = utils::PIIType::UNKNOWN);

    /**
     * @brief Remove a previously declared explicit field.
     */
    void undeclareField(const std::string& field_name);

    // -------------------------------------------------------------------------
    // Core masking
    // -------------------------------------------------------------------------

    /**
     * @brief Mask PII in a single JSON entity object.
     *
     * Operates on a copy – the original is not modified.
     *
     * @param result      JSON object (entity) to mask.
     * @param user_roles  Roles of the requesting user; privileged roles bypass masking.
     * @return            Masked copy of @p result.
     */
    nlohmann::json maskResult(
        const nlohmann::json& result,
        const std::vector<std::string>& user_roles = {}) const;

    /**
     * @brief Mask PII in a JSON array of entity objects.
     *
     * Applies maskResult() to every element of the array.
     *
     * @param results     JSON array of entity objects.
     * @param user_roles  Roles of the requesting user.
     * @return            Masked copy of @p results.
     */
    nlohmann::json maskResultSet(
        const nlohmann::json& results,
        const std::vector<std::string>& user_roles = {}) const;

    // -------------------------------------------------------------------------
    // Policy management
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether the policy is currently enabled.
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable masking at runtime (e.g. during tests).
     */
    void setEnabled(bool enabled);

    /**
     * @brief Return true if the given role list bypasses masking.
     */
    bool isPrivileged(const std::vector<std::string>& user_roles) const;

    /**
     * @brief Access the underlying Config.
     */
    const Config& config() const;

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /// Snapshot of declared_fields_ taken under the lock for a single masking
    /// operation.  Avoids accessing the shared map without a lock from deep in
    /// the recursive call chain.
    using DeclaredFieldsSnapshot = std::unordered_map<std::string, FieldMaskConfig>;

    /**
     * @brief Recursively mask a JSON value (object, array, or scalar).
     *
     * @param node      JSON node to process.
     * @param key       The JSON key that owns this node (empty for root).
     * @param snapshot  Immutable snapshot of declared_fields_ for this call.
     */
    nlohmann::json maskNode(
        const nlohmann::json& node,
        const std::string& key,
        const DeclaredFieldsSnapshot& snapshot) const;

    /**
     * @brief Mask a single string value.
     *
     * Applies explicit field config, field-name hints, and auto-detection in
     * that priority order.
     *
     * @param value     Original string value.
     * @param key       The JSON key that owns this value.
     * @param snapshot  Immutable snapshot of declared_fields_ for this call.
     * @return          Masked string.
     */
    std::string maskStringValue(
        const std::string& value,
        const std::string& key,
        const DeclaredFieldsSnapshot& snapshot) const;

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    mutable std::mutex mutex_;
    Config config_;
    std::shared_ptr<utils::PIIDetector> detector_;

    /// Explicitly declared fields and their masking configuration.
    std::unordered_map<std::string, FieldMaskConfig> declared_fields_;
};

} // namespace security
} // namespace themis
