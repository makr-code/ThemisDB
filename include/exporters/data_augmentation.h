/**
 * @file data_augmentation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Augmentation strategies that the pipeline can apply to text fields.
enum class AugmentationStrategy {
    /// Replaces common words with single built-in synonyms.
    /// Example: "big" → "large", "fast" → "quick"
    SYNONYM_REPLACEMENT,

    /// Reformulates question-style instructions by varying the leading question
    /// phrase. Works on the instruction/question field only.
    /// Example: "What is X?" → "Can you explain X?", "Tell me about X?"
    QUESTION_REFORMULATION,

    /// Strips leading/trailing whitespace from all string fields and normalises
    /// runs of internal whitespace to a single space.
    WHITESPACE_NORMALIZATION,

    /// Generates a lowercased copy of every string field.
    LOWERCASE,

    /// Generates a copy where every sentence in every string field has its
    /// first character uppercased ("title sentence" casing).
    SENTENCE_CASING,
};

/// Per-strategy knob: how many augmented copies to produce for each input entity.
/// count = 0 disables that strategy entirely.
struct AugmentationStrategyConfig {
    AugmentationStrategy strategy;
    uint32_t             count = 1;   ///< Copies per input entity (0 = disabled)
};

/// Configuration for the DataAugmentationPipeline.
struct AugmentationConfig {
    /// Ordered list of strategies to apply.  Each enabled strategy (count > 0)
    /// produces `count` augmented copies of every input entity.
    std::vector<AugmentationStrategyConfig> strategies;

    /// Field name that holds the instruction / question text.
    /// Only used by QUESTION_REFORMULATION.
    std::string instruction_field = "question";

    /// Fields to augment.  Empty means "augment all string-valued fields".
    std::vector<std::string> augment_fields;

    /// When true, the original (non-augmented) entities are included in the
    /// output alongside the synthetic copies.  When false, only the synthetic
    /// copies are returned.
    bool include_originals = true;

    /// Prefix appended to the primary key of every augmented entity so that
    /// the caller can distinguish originals from synthetic copies.
    /// Default: "aug_"
    std::string augmented_key_prefix = "aug_";

    /// Optional user-supplied synonym map that extends / overrides the built-in
    /// one.  Format: { "word" → "synonym" }.  Matching is case-insensitive.
    std::map<std::string, std::string> custom_synonyms;
};

/// Statistics produced by a single augmentation run.
struct AugmentationStats {
    size_t input_entities      = 0;  ///< Entities received
    size_t augmented_entities  = 0;  ///< Synthetic copies produced
    size_t output_entities     = 0;  ///< Total entities in result (originals + copies)
    size_t fields_augmented    = 0;  ///< Field-level augmentation operations applied
    size_t strategies_applied  = 0;  ///< Strategy runs executed
};

/// Synthetic data augmentation pipeline.
///
/// Takes a collection of BaseEntity objects and returns an expanded collection
/// that contains the originals (when include_originals == true) plus one or
/// more synthetic variants per entity, depending on the configured strategies.
///
/// All augmentation is purely deterministic text transformation; no external
/// model or network call is made.  This design keeps the pipeline usable in
/// air-gapped and offline environments.
///
/// Typical integration with the JSONL exporter:
/// @code
///   AugmentationConfig aug_cfg;
///   aug_cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 1});
///   aug_cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION, 2});
///
///   DataAugmentationPipeline pipeline(aug_cfg);
///   AugmentationStats aug_stats;
///   auto expanded = pipeline.augment(entities, &aug_stats);
///
///   JSONLLLMExporter exporter(config);
///   auto export_stats = exporter.exportEntities(expanded, options);
/// @endcode
class DataAugmentationPipeline {
public:
    explicit DataAugmentationPipeline(const AugmentationConfig& config = {});

    /// Apply all configured strategies to @p entities and return the expanded
    /// entity collection.  If @p stats is non-null it is populated with counts
    /// for the current run.
    std::vector<BaseEntity> augment(
        const std::vector<BaseEntity>& entities,
        AugmentationStats* stats = nullptr
    ) const;

    /// Apply a single strategy to produce @p count synthetic copies of @p entity.
    /// Returns the generated copies (not including the original).
    std::vector<BaseEntity> applyStrategy(
        const BaseEntity&       entity,
        AugmentationStrategy    strategy,
        uint32_t                count
    ) const;

    const AugmentationConfig& getConfig() const { return config_; }
    void setConfig(const AugmentationConfig& config) { config_ = config; }

private:
    AugmentationConfig config_;

    // ── Strategy implementations ──────────────────────────────────────────

    BaseEntity applySynonymReplacement(const BaseEntity& entity, uint32_t variant) const;
    BaseEntity applyQuestionReformulation(const BaseEntity& entity, uint32_t variant) const;
    BaseEntity applyWhitespaceNormalization(const BaseEntity& entity) const;
    BaseEntity applyLowercase(const BaseEntity& entity) const;
    BaseEntity applySentenceCasing(const BaseEntity& entity) const;

    // ── Text helpers ──────────────────────────────────────────────────────

    /// Determine which fields of @p entity should be augmented (respects
    /// AugmentationConfig::augment_fields; empty = all string fields).
    std::vector<std::string> selectFields(const BaseEntity& entity) const;

    /// Replace words in @p text using the built-in + custom synonym map.
    /// @p variant selects among multiple synonyms when the map has alternatives.
    std::string replaceSynonyms(const std::string& text, uint32_t variant) const;

    /// Return the i-th question reformulation of @p text (wraps around).
    static std::string reformulateQuestion(const std::string& text, uint32_t variant);

    /// Normalise whitespace: trim and collapse internal runs of spaces/tabs.
    static std::string normalizeWhitespace(const std::string& text);

    /// Lowercase every character in @p text.
    static std::string toLowercase(const std::string& text);

    /// Uppercase the first character of every sentence (split by '. ').
    static std::string toSentenceCasing(const std::string& text);

    // ── Synonym data ──────────────────────────────────────────────────────

    /// Returns the built-in synonym map: word → synonyms (ordered list).
    static const std::map<std::string, std::vector<std::string>>& builtinSynonyms();
};

} // namespace exporters
} // namespace themis
