/**
 * @file data_augmentation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/data_augmentation.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace exporters {

// ─────────────────────────────────────────────────────────────────────────────
// Built-in synonym table
// ─────────────────────────────────────────────────────────────────────────────

const std::map<std::string, std::vector<std::string>> &DataAugmentationPipeline::builtinSynonyms() {
    // Each entry maps a lowercase word to an ordered list of synonyms.
    // The variant index selects among the synonyms (wraps around).
    static const std::map<std::string, std::vector<std::string>> kSynonyms = {
        {"big", {"large", "great", "substantial"}},
        {"small", {"little", "tiny", "compact"}},
        {"fast", {"quick", "rapid", "swift"}},
        {"slow", {"gradual", "leisurely", "unhurried"}},
        {"good", {"excellent", "fine", "superior"}},
        {"bad", {"poor", "inferior", "substandard"}},
        {"important", {"significant", "critical", "essential"}},
        {"show", {"display", "present", "demonstrate"}},
        {"get", {"obtain", "retrieve", "acquire"}},
        {"use", {"utilize", "employ", "apply"}},
        {"make", {"create", "produce", "generate"}},
        {"find", {"locate", "discover", "identify"}},
        {"give", {"provide", "supply", "offer"}},
        {"tell", {"explain", "describe", "inform"}},
        {"know", {"understand", "recognize", "realize"}},
        {"think", {"consider", "believe", "conclude"}},
        {"say", {"state", "mention", "indicate"}},
        {"help", {"assist", "support", "aid"}},
        {"start", {"begin", "initiate", "launch"}},
        {"end", {"finish", "complete", "conclude"}},
        {"best", {"optimal", "ideal", "top"}},
        {"correct", {"accurate", "right", "proper"}},
        {"wrong", {"incorrect", "inaccurate", "erroneous"}},
        {"example", {"instance", "case", "illustration"}},
        {"list", {"enumerate", "catalog", "itemize"}},
        {"explain", {"describe", "clarify", "elaborate"}},
        {"define", {"specify", "characterize", "describe"}},
        {"compare", {"contrast", "differentiate", "evaluate"}},
        {"create", {"generate", "produce", "build"}},
        {"delete", {"remove", "erase", "eliminate"}},
        {"update", {"modify", "change", "revise"}},
        {"search", {"query", "lookup", "retrieve"}},
        {"process", {"handle", "execute", "perform"}},
        {"return", {"provide", "output", "yield"}},
        {"store", {"save", "persist", "record"}},
        {"check", {"verify", "validate", "confirm"}},
    };
    return kSynonyms;
}

// ─────────────────────────────────────────────────────────────────────────────
// Question reformulation phrases
// ─────────────────────────────────────────────────────────────────────────────

// Returns a reformulated version of @p text using a round-robin set of phrasings.
// The input may or may not end with a '?'; all variants produce a question.
/*static*/ std::string DataAugmentationPipeline::reformulateQuestion(const std::string &text, uint32_t variant) {
    if (text.empty()) {
        return text;
    }

    // Strip trailing '?' and whitespace to get the bare question body.
    std::string body = text;
    while (!body.empty() && (body.back() == '?' || std::isspace(static_cast<unsigned char>(body.back())))) {
        body.pop_back();
    }
    if (body.empty()) {
        return text;
    }

    // Lowercase first character of the body for a natural continuation.
    std::string lbody = body;
    if (!lbody.empty()) {
        lbody[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(lbody[0])));
    }

    static const std::vector<std::string> kPrefixes = {
        "Can you explain ", "Could you describe ",     "Please describe ",           "How would you explain ",
        "Tell me about ",   "What do you know about ", "Provide an explanation of ", "Describe ",
    };

    const size_t idx = static_cast<size_t>(variant) % kPrefixes.size();
    return kPrefixes[idx] + lbody + "?";
}

// ─────────────────────────────────────────────────────────────────────────────
// Text helpers
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string DataAugmentationPipeline::normalizeWhitespace(const std::string &text) {
    std::string result = {};
    result.reserve(text.size());
    bool last_was_space = true; // treat leading whitespace as "already had a space"
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_was_space) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += c;
            last_was_space = false;
        }
    }
    // Remove trailing space added by the loop
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

/*static*/ std::string DataAugmentationPipeline::toLowercase(const std::string &text) {
    std::string result = text;
    for (char &c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

/*static*/ std::string DataAugmentationPipeline::toSentenceCasing(const std::string &text) {
    if (text.empty()) {
        return text;
    }
    std::string result = text;
    bool next_upper    = true;
    for (size_t i = 0; i < result.size(); ++i) {
        if (next_upper && std::isalpha(static_cast<unsigned char>(result[i]))) {
            result[i]  = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
            next_upper = false;
        } else if (result[i] == '.' || result[i] == '!' || result[i] == '?') {
            next_upper = true;
        }
    }
    return result;
}

std::string DataAugmentationPipeline::replaceSynonyms(const std::string &text, uint32_t variant) const {
    const auto &builtin = builtinSynonyms();

    // Merge built-in and custom synonyms (custom takes precedence).
    // Build a combined lookup: lowercase_word → vector of synonyms
    std::map<std::string, std::vector<std::string>> combined;
    for (const auto &kv : builtin) {
        combined[kv.first] = kv.second;
    }
    for (const auto &kv : config_.custom_synonyms) {
        std::string lk = kv.first;
        std::transform(lk.begin(), lk.end(), lk.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        combined[lk] = {kv.second};
    }

    // Tokenize the text into word/non-word runs and replace words.
    std::string result = {};
    result.reserve(text.size());
    std::string word = {};
    std::string nonword = {};

    auto flush_word = [&]() {
        if (word.empty()) {
            return;
        }
        std::string lw = word;
        std::transform(lw.begin(), lw.end(), lw.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        auto it = combined.find(lw);
        if (it != combined.end() && !it->second.empty()) {
            // Pick the synonym according to variant (round-robin).
            const auto &syns       = it->second;
            const std::string &syn = syns[static_cast<size_t>(variant) % syns.size()];
            // Preserve the capitalisation of the original word.
            if (!word.empty() && std::isupper(static_cast<unsigned char>(word[0]))) {
                std::string capitalized = syn;
                if (!capitalized.empty()) {
                    capitalized[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(capitalized[0])));
                }
                result += capitalized;
            } else {
                result += syn;
            }
        } else {
            result += word;
        }
        word.clear();
    };

    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '\'' || c == '-') {
            result += nonword;
            nonword.clear();
            word += c;
        } else {
            flush_word();
            nonword += c;
        }
    }
    flush_word();
    result += nonword;

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Field selection
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> DataAugmentationPipeline::selectFields(const BaseEntity &entity) const {
    if (!config_.augment_fields.empty()) {
        return config_.augment_fields;
    }
    // Default: all string-valued fields.
    std::vector<std::string> fields = {};

    for (const auto &kv : entity.getAllFields()) {
        if (entity.getFieldAsString(kv.first)) {
            fields.push_back(kv.first);
        }
    }
    return fields;
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-strategy apply helpers
// ─────────────────────────────────────────────────────────────────────────────

BaseEntity DataAugmentationPipeline::applySynonymReplacement(const BaseEntity &entity, uint32_t variant) const {
    BaseEntity result = entity;
    for (const auto &field : selectFields(entity)) {
        auto val = entity.getFieldAsString(field);
        if (val) {
            result.setField(field, replaceSynonyms(*val, variant));
        }
    }
    return result;
}

BaseEntity DataAugmentationPipeline::applyQuestionReformulation(const BaseEntity &entity, uint32_t variant) const {
    BaseEntity result = entity;
    auto val          = entity.getFieldAsString(config_.instruction_field);
    if (val) {
        result.setField(config_.instruction_field, reformulateQuestion(*val, variant));
    }
    return result;
}

BaseEntity DataAugmentationPipeline::applyWhitespaceNormalization(const BaseEntity &entity) const {
    BaseEntity result = entity;
    for (const auto &field : selectFields(entity)) {
        auto val = entity.getFieldAsString(field);
        if (val) {
            result.setField(field, normalizeWhitespace(*val));
        }
    }
    return result;
}

BaseEntity DataAugmentationPipeline::applyLowercase(const BaseEntity &entity) const {
    BaseEntity result = entity;
    for (const auto &field : selectFields(entity)) {
        auto val = entity.getFieldAsString(field);
        if (val) {
            result.setField(field, toLowercase(*val));
        }
    }
    return result;
}

BaseEntity DataAugmentationPipeline::applySentenceCasing(const BaseEntity &entity) const {
    BaseEntity result = entity;
    for (const auto &field : selectFields(entity)) {
        auto val = entity.getFieldAsString(field);
        if (val) {
            result.setField(field, toSentenceCasing(*val));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

DataAugmentationPipeline::DataAugmentationPipeline(const AugmentationConfig &config) : config_(config) {}

std::vector<BaseEntity> DataAugmentationPipeline::applyStrategy(const BaseEntity &entity, AugmentationStrategy strategy,
                                                                uint32_t count) const {
    std::vector<BaseEntity> copies;
    copies.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        BaseEntity copy;
        switch (strategy) {
            case AugmentationStrategy::SYNONYM_REPLACEMENT:
                copy = applySynonymReplacement(entity, i);
                break;
            case AugmentationStrategy::QUESTION_REFORMULATION:
                copy = applyQuestionReformulation(entity, i);
                break;
            case AugmentationStrategy::WHITESPACE_NORMALIZATION:
                copy = applyWhitespaceNormalization(entity);
                break;
            case AugmentationStrategy::LOWERCASE:
                copy = applyLowercase(entity);
                break;
            case AugmentationStrategy::SENTENCE_CASING:
                copy = applySentenceCasing(entity);
                break;
            default:
                copy = entity;
                break;
        }
        // Assign a unique primary key: <prefix><strategy_idx>_<variant>_<original_pk>
        copy.setPrimaryKey(config_.augmented_key_prefix + std::to_string(static_cast<int>(strategy)) + "_"
                           + std::to_string(i) + "_" + entity.getPrimaryKey());
        copies.push_back(std::move(copy));
    }
    return copies;
}

std::vector<BaseEntity> DataAugmentationPipeline::augment(const std::vector<BaseEntity> &entities,
                                                          AugmentationStats *stats) const {
    AugmentationStats local_stats;
    local_stats.input_entities = entities.size();

    std::vector<BaseEntity> output;

    if (config_.include_originals) {
        output.insert(output.end(), entities.begin(), entities.end());
    }

    for (const auto &strategy_cfg : config_.strategies) {
        if (strategy_cfg.count == 0) {
            continue;
        }
        local_stats.strategies_applied++;

        for (const auto &entity : entities) {
            auto copies = applyStrategy(entity, strategy_cfg.strategy, strategy_cfg.count);
            local_stats.augmented_entities += copies.size();

            // Count field-level operations: fields augmented × copies produced.
            local_stats.fields_augmented += selectFields(entity).size() * copies.size();

            output.insert(output.end(), std::make_move_iterator(copies.begin()), std::make_move_iterator(copies.end()));
        }
    }

    local_stats.output_entities = output.size();

    THEMIS_INFO("DataAugmentationPipeline: {} input entities → {} synthetic copies "
                "(total output: {}), {} strategies applied",
                local_stats.input_entities, local_stats.augmented_entities, local_stats.output_entities,
                local_stats.strategies_applied);

    if (stats) {
        *stats = local_stats;
    }
    return output;
}

} // namespace exporters
} // namespace themis
