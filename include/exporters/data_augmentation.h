/**
 * @file data_augmentation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/base_entity.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace exporters {

enum class AugmentationStrategy {
    SYNONYM_REPLACEMENT,

    QUESTION_REFORMULATION,

    WHITESPACE_NORMALIZATION,

    LOWERCASE,

    SENTENCE_CASING,
};

struct AugmentationStrategyConfig {
    AugmentationStrategy strategy;
    uint32_t             count = 1;   ///< Copies per input entity (0 = disabled)
};

struct AugmentationConfig {
    std::vector<AugmentationStrategyConfig> strategies;

    std::string instruction_field = "question";

    std::vector<std::string> augment_fields;

    bool include_originals = true;

    std::string augmented_key_prefix = "aug_";

    std::map<std::string, std::string> custom_synonyms;
};

struct AugmentationStats {
    size_t input_entities      = 0;  ///< Entities received
    size_t augmented_entities  = 0;  ///< Synthetic copies produced
    size_t output_entities     = 0;  ///< Total entities in result (originals + copies)
    size_t fields_augmented    = 0;  ///< Field-level augmentation operations applied
    size_t strategies_applied  = 0;  ///< Strategy runs executed
};

class DataAugmentationPipeline {
public:
    explicit DataAugmentationPipeline(const AugmentationConfig& config = {});

    std::vector<BaseEntity> augment(
        const std::vector<BaseEntity>& entities,
        AugmentationStats* stats = nullptr
    ) const;

    /**
     * @brief Apply Strategy.
     * @param[in] entity Input parameter.
     * @param[in] strategy Input parameter.
     * @param[in] count Input parameter.
     * @return Return value.
     */
    std::vector<BaseEntity> applyStrategy(
        const BaseEntity&       entity,
        AugmentationStrategy    strategy,
        uint32_t                count
    ) const;

    const AugmentationConfig& getConfig() const { return config_; }
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const AugmentationConfig& config) { config_ = config; }

private:
    AugmentationConfig config_;

    /**
     * @brief ── Strategy implementations ──────────────────────────────────────────
     * @param[in] entity Input parameter.
     * @param[in] variant Input parameter.
     * @return Return value.
     */

    BaseEntity applySynonymReplacement(const BaseEntity& entity, uint32_t variant) const;
    /**
     * @brief Apply Question Reformulation.
     * @param[in] entity Input parameter.
     * @param[in] variant Input parameter.
     * @return Return value.
     */
    BaseEntity applyQuestionReformulation(const BaseEntity& entity, uint32_t variant) const;
    /**
     * @brief Apply Whitespace Normalization.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    BaseEntity applyWhitespaceNormalization(const BaseEntity& entity) const;
    /**
     * @brief Apply Lowercase.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    BaseEntity applyLowercase(const BaseEntity& entity) const;
    /**
     * @brief Apply Sentence Casing.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    BaseEntity applySentenceCasing(const BaseEntity& entity) const;

    /**
     * @brief ── Text helpers ──────────────────────────────────────────────────────
     * @param[in] entity Input parameter.
     * @return Return value.
     */

    std::vector<std::string> selectFields(const BaseEntity& entity) const;

    /**
     * @brief Replace Synonyms.
     * @param[in] text Input parameter.
     * @param[in] variant Input parameter.
     * @return Return value.
     */
    std::string replaceSynonyms(const std::string& text, uint32_t variant) const;

    /**
     * @brief Reformulate Question.
     * @param[in] text Input parameter.
     * @param[in] variant Input parameter.
     * @return Return value.
     */
    static std::string reformulateQuestion(const std::string& text, uint32_t variant);

    /**
     * @brief Normalize Whitespace.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string normalizeWhitespace(const std::string& text);

    /**
     * @brief To Lowercase.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string toLowercase(const std::string& text);

    /**
     * @brief To Sentence Casing.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string toSentenceCasing(const std::string& text);

    // ── Synonym data ──────────────────────────────────────────────────────

    static const std::map<std::string, std::vector<std::string>>& builtinSynonyms();
};

} // namespace exporters
} // namespace themis
