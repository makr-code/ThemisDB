/**
 * @file semantic_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/deontic_extractor.h"
#include <string>
#include <vector>
#include <functional>

namespace themis {
namespace ingestion {

// ============================================================================
// Quality gate configuration
// ============================================================================

/**
 * @brief Configuration for a single quality gate.
 *
 * Quality gates are pass/fail checks applied to extracted legal provisions.
 * Each gate has a name, a numeric threshold (where applicable), and an action
 * to take when the gate fails.
 */
struct QualityGateConfig {
    std::string name;             ///< Gate identifier (e.g. "min_confidence")
    std::string description;      ///< Human-readable description
    double      threshold = 0.0;  ///< Numeric threshold (meaning depends on gate type)
    bool        required  = false; ///< If true, failure blocks ingestion; if false, warn only

    QualityGateConfig() = default;
    QualityGateConfig(std::string name_, std::string desc,
                      double thresh, bool req = false)
        : name(std::move(name_)), description(std::move(desc)),
          threshold(thresh), required(req) {}
};

/**
 * @brief Default quality gate configuration for German legal texts.
 *
 * Provides the gate set defined in `legal-ingestion-schema.yaml`:
 *  - min_confidence (0.80, warn)
 *  - deontic_confidence (0.75, warn)
 *  - section_hierarchy (required)
 */
struct LegalQualityGates {
    static constexpr double kMinConfidence      = 0.80;
    static constexpr double kDeonticConfidence  = 0.75;

    QualityGateConfig min_confidence {
        "min_confidence",
        "Overall extraction confidence must meet threshold",
        kMinConfidence, false
    };
    QualityGateConfig deontic_confidence {
        "deontic_confidence",
        "Deontic extraction confidence must meet threshold",
        kDeonticConfidence, false
    };
    QualityGateConfig section_hierarchy {
        "section_hierarchy",
        "Document must have detectable section structure",
        0.0, true
    };
    QualityGateConfig temporal_present {
        "temporal_present",
        "Document should contain at least one temporal expression",
        0.0, false
    };
    QualityGateConfig no_dangling_refs {
        "no_dangling_refs",
        "All cross-references should resolve in the knowledge base",
        0.0, false
    };
};

// ============================================================================
// Quality gate result
// ============================================================================

/**
 * @brief Result of evaluating a single quality gate.
 */
struct QualityGateResult {
    std::string name;    ///< Gate name (matches QualityGateConfig::name)
    bool        passed;  ///< true if the gate check passed
    std::string reason;  ///< Human-readable explanation (populated on failure)

    QualityGateResult() : passed(true) {}
    QualityGateResult(std::string n, bool p, std::string r = "")
        : name(std::move(n)), passed(p), reason(std::move(r)) {}
};

// ============================================================================
// Semantic validation result
// ============================================================================

/**
 * @brief Semantic validation result for a single document or provision.
 *
 * Produced by `SemanticValidator::validate()`.  Contains the overall
 * validity flag, a list of inconsistencies detected, a semantic score
 * in [0, 1], actionable suggestions, and per-gate results.
 */
struct SemanticValidationResult {
    bool is_valid    = true;                      ///< false when a required gate fails
    double semantic_score = 0.0;                  ///< Overall quality score in [0,1]
    std::vector<std::string>   inconsistencies;   ///< Detected logical inconsistencies
    std::vector<std::string>   suggestions;       ///< Improvement suggestions
    std::vector<std::string>   warnings;          ///< Non-blocking issues
    std::vector<QualityGateResult> gate_results;  ///< Per-gate pass/fail details

    SemanticValidationResult() = default;

    /// Return only the gates that failed.
    std::vector<QualityGateResult> failedGates() const {
        std::vector<QualityGateResult> out;
        for (const auto& g : gate_results) {
            if (!g.passed) {
              out.push_back(g);
            }
        }
        return out;
    }

    /// Return only the gates that passed.
    std::vector<QualityGateResult> passedGates() const {
        std::vector<QualityGateResult> out;
        for (const auto& g : gate_results) {
            if (g.passed) {
              out.push_back(g);
            }
        }
        return out;
    }
};

// ============================================================================
// Legal extraction result (document-level)
// ============================================================================

/**
 * @brief Aggregated extraction result for a complete legal document.
 *
 * Produced after running the full legal ingestion pipeline on a document.
 * Corresponds to the JSON output format described in the problem statement.
 */
struct LegalProvision {
    std::string                    provision_id;    ///< e.g. "BImSchG_§4_Abs1"
    std::string                    section_ref;     ///< e.g. "§4 Abs. 1"
    std::string                    text;            ///< Raw provision text
    DeonticCategory                deontic_category; ///< Primary deontic category
    double                         category_confidence; ///< Category confidence [0,1]
    std::vector<ExtractedEntity>   entities;        ///< Recognized entities
    std::vector<DeonticObligation> obligations;     ///< Structured obligations
    std::vector<std::string>       references_to;   ///< Cross-references extracted
    std::string                    effective_from;  ///< ISO-8601 date or empty
    std::string                    effective_to;    ///< ISO-8601 date or empty

    LegalProvision() : deontic_category(DeonticCategory::UNKNOWN),
                       category_confidence(0.0) {}
};

/**
 * @brief Full extraction result for a legal document.
 */
struct LegalExtractionResult {
    std::string                    document_id;
    std::string                    effective_from;     ///< Document-level effective date
    std::string                    effective_to;       ///< Document-level expiry date (empty if current)
    std::vector<LegalProvision>    provisions;         ///< Per-provision results
    double                          quality_score;      ///< Overall quality score [0,1]
    SemanticValidationResult        validation;         ///< Quality gate results
    std::vector<std::string>        warnings;

    LegalExtractionResult() : quality_score(0.0) {}
};

// ============================================================================
// SemanticValidator
// ============================================================================

/**
 * @brief Semantic validator for legal text extractions.
 *
 * Applies quality gates defined in `LegalQualityGates` to a
 * `DeonticExtraction` result and produces a `SemanticValidationResult`.
 *
 * The validator is injectable: supply a custom validation function via
 * `setValidatorFn()` to override the built-in logic (useful in tests or
 * when wiring an LLM-based semantic checker in Phase 2).
 *
 * Usage:
 * @code
 * DeonticExtractor extractor;
 * SemanticValidator validator;
 * validator.setQualityGates(LegalQualityGates{});
 *
 * auto extraction = extractor.extract(text);
 * auto result = validator.validate(extraction, text);
 * if (!result.is_valid) {
 *     // handle required gate failure
 * }
 * @endcode
 */
class SemanticValidator {
public:
    /// Signature for an injectable validation function.
    using ValidatorFn = std::function<SemanticValidationResult(
        const DeonticExtraction& extraction, const std::string& text)>;

    SemanticValidator();

    /**
     * @brief Validate a deontic extraction result against the configured gates.
     *
     * @param extraction  Result from DeonticExtractor::extract()
     * @param text        Original document text (used for pattern checks)
     * @return            SemanticValidationResult with gate details and score
     */
    SemanticValidationResult validate(const DeonticExtraction& extraction,
                                       const std::string& text) const;

    /**
     * @brief Build a LegalExtractionResult for a complete document.
     *
     * Splits the document into provisions (by § pattern), runs extraction
     * and validation on each, and returns an aggregated result.
     *
     * @param document_id   Identifier for the document (e.g. "BImSchG_2024")
     * @param full_text     Complete text of the legal document
     * @return              Aggregated LegalExtractionResult
     */
    LegalExtractionResult extractDocument(const std::string& document_id,
                                           const std::string& full_text) const;

    /**
     * @brief Configure the quality gates to apply.
     *
     * @param gates   Quality gate configuration struct
     */
    void setQualityGates(const LegalQualityGates& gates);

    /**
     * @brief Replace the built-in validation logic with a custom function.
     *
     * Pass an empty `ValidatorFn{}` to restore the built-in logic.
     *
     * @param fn  Custom validation function, or empty to reset
     */
    void setValidatorFn(ValidatorFn fn);

    /**
     * @brief Replace the internal `DeonticExtractor` with a pre-configured one.
     *
     * ## SoC / DIP note
     *
     * The default extractor uses built-in regex rules.  To activate an
     * LLM-backed extractor, build one via `LegalLlmAdapter::buildExtractor()`
     * and inject it here.  The `SemanticValidator` remains entirely free of
     * any `llm/` or `LegalLlmAdapter` knowledge — the caller provides the
     * pre-built extractor.
     *
     * Example:
     * @code
     * // In wiring code (not in ingestion/):
     * LegalLlmAdapter adapter(bridge);
     * DeonticExtractor llm_extractor = adapter.buildExtractor(0.75);
     *
     * SemanticValidator validator;
     * validator.setExtractor(std::move(llm_extractor));
     * @endcode
     *
     * @param extractor  Pre-configured DeonticExtractor (moved in).
     */
    void setExtractor(DeonticExtractor extractor);

private:
    SemanticValidationResult validateBuiltin(const DeonticExtraction& extraction,
                                              const std::string& text) const;

    /// Check the section-hierarchy gate: does the text contain a § pattern?
    static bool hasSection(const std::string& text);

    /// Check the temporal-present gate: does the text contain a date pattern?
    static bool hasTemporal(const std::string& text);

    /// Compute a composite score from extraction results.
    static double computeScore(const DeonticExtraction& extraction,
                                const std::string& text);

    LegalQualityGates gates_;
    ValidatorFn       validator_fn_;  ///< Custom override; empty = built-in
    DeonticExtractor  extractor_;     ///< Shared extractor instance
};

} // namespace ingestion
} // namespace themis
