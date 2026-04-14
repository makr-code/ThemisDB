/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_masker.h                                      ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:38:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • ffc2b43f84  2026-02-26  feat(governance): automated data masking for sensitive fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace governance {

/**
 * @brief Masking strategy applied to a sensitive field value.
 *
 * | Strategy | Output                                                    |
 * |----------|-----------------------------------------------------------|
 * | REDACT   | Replaces value with the literal string "[REDACTED]".      |
 * | TOKENIZE | Replaces value with a stable HMAC-SHA256 pseudonym keyed  |
 * |          | on a per-collection secret (format: "tkn_<hex>").         |
 * |          | The same input always produces the same pseudonym within  |
 * |          | the same collection, supporting join queries.             |
 * | TRUNCATE | Keeps only the first `truncate_length` characters and     |
 * |          | appends "..." (e.g. "Alice..." for length=5).             |
 * | HASH     | Replaces value with its SHA-256 hex digest               |
 * |          | (format: "sha_<hex>"). Not reversible; not stable across  |
 * |          | different salt inputs.                                    |
 */
enum class MaskingStrategy {
    REDACT,   ///< "[REDACTED]"
    TOKENIZE, ///< Stable HMAC-SHA256 pseudonym
    TRUNCATE, ///< First N characters + "..."
    HASH,     ///< SHA-256 hex digest
};

/**
 * @brief Configuration for a single field masking rule.
 */
struct FieldMaskingRule {
    /// JSON key name to mask (exact, case-sensitive match).
    std::string field_name;

    /// Strategy to apply when masking this field.
    MaskingStrategy strategy = MaskingStrategy::REDACT;

    /// Number of characters to retain for TRUNCATE strategy (default: 4).
    int truncate_length = 4;

    /// HMAC key material used by the TOKENIZE strategy.
    /// Should be a per-collection secret stored in the key management service.
    /// Must not be empty when strategy == TOKENIZE.
    std::string collection_secret;
};

/**
 * @brief Collection of field masking rules that govern a single query context.
 *
 * A `FieldMaskingPolicy` is returned by
 * `PolicyEngine::checkQueryPermission()` alongside the `PolicyDecision`.
 * The query executor calls `DataMasker::maskFields(doc, policy)` before
 * serialising each result document.
 */
struct FieldMaskingPolicy {
    /// Whether data masking is active for this context.
    /// When false, DataMasker::maskFields() is a pass-through no-op.
    bool enabled = false;

    /// Ordered list of masking rules.  The first rule whose `field_name`
    /// matches the JSON key wins; subsequent rules for the same key are
    /// ignored.
    std::vector<FieldMaskingRule> rules;
};

/**
 * @brief Applies configured field-level masking to JSON query result documents.
 *
 * `DataMasker` is the last in-process defence before data leaves the query
 * engine.  It is required for GDPR Article 25 (data protection by design).
 *
 * Masking is applied to string-valued fields only; numeric, boolean, and null
 * values pass through unchanged.  Object and array nodes are recursively
 * traversed; masking rules match against the immediate JSON key at every
 * nesting level.
 *
 * Observability:
 *   Every field that is masked increments the Prometheus counter
 *   `governance_fields_masked_total` with a label `strategy` set to one of
 *   "redact", "tokenize", "truncate", or "hash".
 *
 * Thread-safety:
 *   `DataMasker` is stateless; all public methods are safe to call
 *   concurrently from multiple threads without external locking.
 *
 * Usage:
 * @code
 * DataMasker masker;
 * auto result = engine.checkQueryPermission(headers, route);
 * nlohmann::json safe_doc = masker.maskFields(raw_doc, result.masking_policy);
 * @endcode
 */
class DataMasker {
  public:
    DataMasker() = default;

    /**
     * @brief Apply masking rules to a single JSON document.
     *
     * @param doc     JSON object returned by the query engine.
     * @param policy  Masking policy produced by PolicyEngine.
     * @return A copy of @p doc with all governed fields masked.
     *         Returns @p doc unchanged if `policy.enabled == false` or
     *         `policy.rules` is empty.
     */
    nlohmann::json maskFields(const nlohmann::json &doc, const FieldMaskingPolicy &policy) const;

    /**
     * @brief Apply masking rules to a JSON array of documents.
     *
     * Equivalent to calling `maskFields()` on every element of the array.
     *
     * @param docs    JSON array of result documents.
     * @param policy  Masking policy.
     * @return Masked copy of @p docs.  If @p docs is not an array,
     *         delegates to `maskFields()` and wraps the result.
     */
    nlohmann::json maskFieldsArray(const nlohmann::json &docs, const FieldMaskingPolicy &policy) const;

    // -------------------------------------------------------------------------
    // Strategy helpers (public for unit testing)
    // -------------------------------------------------------------------------

    /**
     * @brief Apply a single masking rule to a string value.
     *
     * @param value  Original string value.
     * @param rule   Rule describing the strategy and its parameters.
     * @return Masked replacement string.
     */
    static std::string applyStrategy(const std::string &value, const FieldMaskingRule &rule);

  private:
    /// Build an index from field_name → rule for O(1) lookup during traversal.
    using RuleIndex = std::unordered_map<std::string, const FieldMaskingRule *>;

    /**
     * @brief Recursively traverse a JSON node and mask matching fields.
     *
     * @param node       JSON node to process.
     * @param key        The JSON key that owns this node (empty for root).
     * @param rule_index Pre-built index of active rules.
     * @return Masked copy of @p node.
     */
    nlohmann::json maskNode(const nlohmann::json &node, const std::string &key, const RuleIndex &rule_index) const;
};

} // namespace governance
} // namespace themis
