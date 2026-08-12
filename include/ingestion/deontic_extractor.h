/**
 * @file deontic_extractor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace themis {
namespace ingestion {

// ============================================================================
// Deontic categories for German legal text
// ============================================================================

/**
 * @brief Deontic modality categories for legal provisions.
 *
 * Based on standard deontic logic applied to German administrative law:
 * - OBLIGATION   → muss, bedarf, ist verpflichtet
 * - PERMISSION   → darf, kann, ist berechtigt
 * - PROHIBITION  → darf nicht, ist verboten, ist untersagt
 * - DEFINITION   → im Sinne dieses Gesetzes, gilt als
 * - CONDITION    → wenn, falls, sofern, soweit
 * - EXCEPTION    → ausgenommen, außer, gilt nicht für
 * - REFERENCE    → gemäß, nach § , entsprechend
 * - UNKNOWN      → no pattern matched
 */
enum class DeonticCategory {
    OBLIGATION,   ///< muss / bedarf / ist verpflichtet
    PERMISSION,   ///< darf / kann / ist berechtigt
    PROHIBITION,  ///< darf nicht / ist verboten
    DEFINITION,   ///< im Sinne dieses Gesetzes / gilt als
    CONDITION,    ///< wenn / falls / sofern / soweit
    EXCEPTION,    ///< ausgenommen / außer / gilt nicht für
    REFERENCE,    ///< gemäß / nach § / entsprechend
    UNKNOWN       ///< no pattern matched
};

/// Convert a DeonticCategory to its string representation.
inline std::string deonticCategoryToString(DeonticCategory c) {
    switch (c) {
        case DeonticCategory::OBLIGATION:  return "obligation";
        case DeonticCategory::PERMISSION:  return "permission";
        case DeonticCategory::PROHIBITION: return "prohibition";
        case DeonticCategory::DEFINITION:  return "definition";
        case DeonticCategory::CONDITION:   return "condition";
        case DeonticCategory::EXCEPTION:   return "exception";
        case DeonticCategory::REFERENCE:   return "reference";
        default:                           return "unknown";
    }
}

/// Parse a string into a DeonticCategory (case-insensitive).
inline DeonticCategory deonticCategoryFromString(const std::string& s) {
    if (s == "obligation")  return DeonticCategory::OBLIGATION;
    if (s == "permission")  return DeonticCategory::PERMISSION;
    if (s == "prohibition") return DeonticCategory::PROHIBITION;
    if (s == "definition")  return DeonticCategory::DEFINITION;
    if (s == "condition")   return DeonticCategory::CONDITION;
    if (s == "exception")   return DeonticCategory::EXCEPTION;
    if (s == "reference")   return DeonticCategory::REFERENCE;
    return DeonticCategory::UNKNOWN;
}

// ============================================================================
// Extracted entity types
// ============================================================================

/**
 * @brief A single entity recognized in a legal text.
 *
 * Entities are typed spans extracted from the document content.
 * In Phase 1, extraction is regex-based; Phase 2 will use a SpaCy NER model.
 */
struct ExtractedEntity {
    std::string type;        ///< law_reference | person_role | organization | temporal | threshold_value
    std::string value;       ///< Canonical value (e.g. "BImSchG", "Betreiber", "14 Tage")
    std::string text;        ///< Raw matched text from the document
    double      confidence;  ///< Extraction confidence in [0, 1]

    ExtractedEntity() : confidence(1.0) {}
    ExtractedEntity(std::string type_, std::string value_,
                    std::string text_, double conf = 1.0)
        : type(std::move(type_)), value(std::move(value_)),
          text(std::move(text_)), confidence(conf) {}
};

// ============================================================================
// Structured obligation
// ============================================================================

/**
 * @brief A structured representation of a legal obligation.
 *
 * Captures the actor, the required action, and the condition under which
 * the obligation applies.
 *
 * Example: "Wer Anlagen betreiben will, bedarf einer Genehmigung."
 *   actor      = "Betreiber"
 *   action     = "Genehmigung einholen"
 *   condition  = "Betrieb einer Anlage"
 */
struct DeonticObligation {
    std::string actor;       ///< Who is obligated (may be empty if implicit)
    std::string action;      ///< What action is required / forbidden / permitted
    std::string condition;   ///< Under what circumstances (may be empty)
    double      confidence;  ///< Extraction confidence in [0, 1]

    DeonticObligation() : confidence(1.0) {}
    DeonticObligation(std::string actor_, std::string action_,
                      std::string cond, double conf = 1.0)
        : actor(std::move(actor_)), action(std::move(action_)),
          condition(std::move(cond)), confidence(conf) {}
};

// ============================================================================
// Extraction result
// ============================================================================

/**
 * @brief Result of running deontic extraction on a single text fragment.
 *
 * Contains the detected deontic categories, recognized entities,
 * structured obligations, an overall confidence score, and any warnings
 * generated during extraction.
 */
struct DeonticExtraction {
    std::vector<DeonticCategory>   deontic_categories;  ///< All matched categories
    std::vector<ExtractedEntity>   entities;             ///< Named entities
    std::vector<DeonticObligation> obligations;          ///< Structured obligations
    double                          overall_confidence;  ///< Weighted average confidence [0,1]
    std::vector<std::string>        warnings;            ///< Non-fatal issues

    DeonticExtraction() : overall_confidence(0.0) {}

    /// Returns true when at least one deontic category was identified.
    bool hasCategory() const { return !deontic_categories.empty(); }

    /// Returns the primary (first) deontic category, or UNKNOWN.
    DeonticCategory primaryCategory() const {
        return deontic_categories.empty()
            ? DeonticCategory::UNKNOWN
            : deontic_categories.front();
    }
};

// ============================================================================
// DeonticExtractor
// ============================================================================

/**
 * @brief Regex-based deontic logic extractor for German legal texts.
 *
 * Implements Phase 1 of the legal ingestion pipeline using pattern matching.
 * The extractor is fully injectable: supply a custom extract function via
 * `setExtractorFn()` to override the default regex implementation (useful
 * in tests or when wiring an LLM adapter in Phase 2).
 *
 * Usage:
 * @code
 * DeonticExtractor extractor;
 * extractor.setConfidenceThreshold(0.75);
 * auto result = extractor.extract("Wer Anlagen betreiben will, bedarf einer Genehmigung.");
 * // result.primaryCategory() == DeonticCategory::OBLIGATION
 * @endcode
 */
class DeonticExtractor {
public:
    /// Signature for an injectable extraction function (used in testing / LLM Phase 2).
    using ExtractorFn = std::function<DeonticExtraction(const std::string& text)>;

    DeonticExtractor();

    /**
     * @brief Extract deontic information from a single text fragment.
     *
     * Runs the configured extractor (regex by default) against the supplied
     * text and returns a fully populated `DeonticExtraction`.  Patterns are
     * applied in priority order: PROHIBITION before PERMISSION, to avoid
     * false-positive permission matches on "darf nicht" phrases.
     *
     * @param text  Raw German legal text (single provision or sentence)
     * @return      Populated DeonticExtraction struct
     */
    DeonticExtraction extract(const std::string& text) const;

    /**
     * @brief Extract entities (law_reference, person_role, etc.) from text.
     *
     * @param text  Raw German legal text
     * @return      Vector of recognized entities
     */
    std::vector<ExtractedEntity> extractEntities(const std::string& text) const;

    /**
     * @brief Set the minimum confidence threshold for reported categories.
     *
     * Matches with confidence below this value are added to `warnings` rather
     * than `deontic_categories`.
     *
     * @param threshold Value in [0, 1], default 0.75
     */
    void setConfidenceThreshold(double threshold);

    /// Return the current confidence threshold.
    double getConfidenceThreshold() const { return confidence_threshold_; }

    /**
     * @brief Replace the default regex extractor with a custom function.
     *
     * Pass an empty `ExtractorFn{}` to restore the built-in regex logic.
     *
     * @param fn  Custom extraction function, or empty to reset
     */
    void setExtractorFn(ExtractorFn fn);

private:
    /// Built-in regex-based extraction (Phase 1).
    DeonticExtraction extractRegex(const std::string& text) const;

    /// Extract entities using compiled regex patterns.
    std::vector<ExtractedEntity> extractEntitiesRegex(const std::string& text) const;

    double      confidence_threshold_;
    ExtractorFn extractor_fn_;  ///< Custom override; empty = use built-in
};

} // namespace ingestion
} // namespace themis
