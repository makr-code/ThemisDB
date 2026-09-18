/**
 * @file data_masker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace governance {

enum class MaskingStrategy {
    REDACT,   ///< "[REDACTED]"
    TOKENIZE, ///< Stable HMAC-SHA256 pseudonym
    TRUNCATE, ///< First N characters + "..."
    HASH,     ///< SHA-256 hex digest
};

struct FieldMaskingRule {
    std::string field_name;

    MaskingStrategy strategy = MaskingStrategy::REDACT;

    int truncate_length = 4;

    std::string collection_secret;
};

struct FieldMaskingPolicy {
    bool enabled = false;

    std::vector<FieldMaskingRule> rules;
};

class DataMasker {
  public:
    DataMasker() = default;

    /**
     * @brief Mask Fields.
     * @param[in] doc Input parameter.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    nlohmann::json maskFields(const nlohmann::json &doc, const FieldMaskingPolicy &policy) const;

    /**
     * @brief Mask Fields Array.
     * @param[in] docs Input parameter.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    nlohmann::json maskFieldsArray(const nlohmann::json &docs, const FieldMaskingPolicy &policy) const;


    /**
     * @brief Apply Strategy.
     * @param[in] value Input parameter.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    static std::string applyStrategy(const std::string &value, const FieldMaskingRule &rule);

  private:
    using RuleIndex = std::unordered_map<std::string, const FieldMaskingRule *>;

    /**
     * @brief Mask Node.
     * @param[in] node Input parameter.
     * @param[in] key Input parameter.
     * @param[in] rule_index Input parameter.
     * @return Return value.
     */
    nlohmann::json maskNode(const nlohmann::json &node, const std::string &key, const RuleIndex &rule_index) const;
};

} // namespace governance
} // namespace themis
