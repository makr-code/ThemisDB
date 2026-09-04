/**
 * @file agentic_reference_validator.h
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
#include <unordered_set>
#include <functional>

namespace themis {
namespace ingestion {

// ============================================================================
// Reference types
// ============================================================================

/**
 * @brief A single cross-reference extracted from a legal text.
 *
 * Represents a reference from one provision to another provision (within the
 * same law, a different German law, or an EU directive).
 *
 * Examples:
 *  - § 4 Abs. 1  → same-document reference
 *  - BImSchG § 4 → inter-law reference
 *  - Richtlinie 2010/75/EU → EU directive reference
 */
struct LegalReference {
    std::string raw_text;     ///< Original matched text (e.g. "§ 4 Abs. 1")
    std::string law_id;       ///< Law identifier (e.g. "BImSchG", "StGB", "" = current doc)
    std::string section;      ///< Section/paragraph number (e.g. "4", "4a")
    std::string subsection;   ///< Absatz number (e.g. "1", empty if not specified)
    std::string item;         ///< Nummer within subsection (e.g. "2", empty if not specified)

    LegalReference() = default;
    LegalReference(std::string raw, std::string law,
                   std::string sec, std::string sub = "", std::string it = "")
        : raw_text(std::move(raw)), law_id(std::move(law)),
          section(std::move(sec)), subsection(std::move(sub)),
          item(std::move(it)) {}

    /// Returns a canonical identifier string for this reference.
    std::string canonicalId() const {
        std::string id = law_id.empty() ? "" : (law_id + ":");
        id += "§" + section;
        if (!subsection.empty()) {
          id += ".Abs." + subsection;
        }
        if (!item.empty()) {
          id += ".Nr." + item;
        }
        return id;
    }
};

// ============================================================================
// Reference validation result
// ============================================================================

/**
 * @brief Result of validating a single cross-reference.
 */
struct ReferenceValidationResult {
    LegalReference reference;   ///< The reference that was checked
    bool           found;        ///< true when the reference resolves in the knowledge base
    double         confidence;   ///< Confidence in the validation result [0, 1]
    std::string    message;      ///< Human-readable status or error message

    ReferenceValidationResult() : found(true), confidence(1.0) {}
    ReferenceValidationResult(LegalReference ref, bool found_, double conf,
                               std::string msg = "")
        : reference(std::move(ref)), found(found_), confidence(conf),
          message(std::move(msg)) {}
};

/**
 * @brief Aggregated reference validation results for a document.
 */
struct ReferenceValidationReport {
    std::vector<LegalReference>          extracted;     ///< All extracted references
    std::vector<ReferenceValidationResult> validated;   ///< Per-reference validation
    size_t                                dangling_count; ///< References that did not resolve
    std::vector<std::string>              warnings;     ///< Validation warnings

    ReferenceValidationReport() : dangling_count(0) {}

    /// Returns true when there are no dangling (unresolved) references.
    bool hasNoDanglingRefs() const { return dangling_count == 0; }
};

// ============================================================================
// AgenticReferenceValidator
// ============================================================================

/**
 * @brief Regex-based agentic reference validator for German legal texts.
 *
 * Implements Phase 1 of the agentic verification loop.  The validator:
 *  1. Extracts all cross-references using regex patterns.
 *  2. Validates each reference against an in-memory knowledge base
 *     (set of known law identifiers and section numbers).
 *  3. Flags dangling references (references to unknown provisions).
 *
 * The knowledge base is populated via `addKnownLaw()` /
 * `addKnownSection()`.  In Phase 2, the knowledge base will be backed by
 * the full ThemisDB graph containing all integrated German laws.
 *
 * The extractor function is injectable via `setExtractorFn()` so that tests
 * can supply synthetic reference lists without needing real legal text.
 *
 * Usage:
 * @code
 * AgenticReferenceValidator validator;
 * validator.addKnownLaw("BImSchG");
 * validator.addKnownSection("BImSchG", "4");
 * validator.addKnownSection("BImSchG", "4a");
 *
 * auto report = validator.validate(document_text);
 * if (report.dangling_count > 0) {
 *     // warn or flag for human review
 * }
 * @endcode
 */
class AgenticReferenceValidator {
public:
    /// Signature for an injectable reference-extraction function.
    using ExtractorFn = std::function<std::vector<LegalReference>(const std::string& text)>;

    AgenticReferenceValidator();

    /**
     * @brief Extract and validate all cross-references in the given text.
     *
     * @param text  Raw German legal text (one provision or a full document)
     * @return      Aggregated ReferenceValidationReport
     */
    ReferenceValidationReport validate(const std::string& text) const;

    /**
     * @brief Extract cross-references from text without validating them.
     *
     * @param text  Raw German legal text
     * @return      Vector of extracted LegalReference objects
     */
    std::vector<LegalReference> extract(const std::string& text) const;

    /**
     * @brief Register a known law identifier in the knowledge base.
     *
     * @param law_id  Law identifier (e.g. "BImSchG", "StGB", "DSGVO")
     */
    void addKnownLaw(const std::string& law_id);

    /**
     * @brief Register a known section within a law.
     *
     * @param law_id   Law identifier (e.g. "BImSchG")
     * @param section  Section/paragraph number (e.g. "4", "4a")
     */
    void addKnownSection(const std::string& law_id, const std::string& section);

    /**
     * @brief Remove all entries from the knowledge base.
     */
    void clearKnowledgeBase();

    /**
     * @brief Return the number of known law identifiers.
     */
    size_t knownLawCount() const;

    /**
     * @brief Replace the built-in regex extractor with a custom function.
     *
     * Pass an empty `ExtractorFn{}` to restore the built-in logic.
     *
     * @param fn  Custom extraction function, or empty to reset
     */
    void setExtractorFn(ExtractorFn fn);

private:
    /// Built-in regex-based reference extraction (Phase 1).
    std::vector<LegalReference> extractRegex(const std::string& text) const;

    /// Check whether a reference resolves in the knowledge base.
    ReferenceValidationResult checkReference(const LegalReference& ref) const;

    /// Set of known law identifiers.
    std::unordered_set<std::string> known_laws_;

    /// Map of law_id → set of known section numbers.
    std::unordered_map<std::string, std::unordered_set<std::string>> known_sections_;

    ExtractorFn extractor_fn_;  ///< Custom override; empty = use built-in
};

} // namespace ingestion
} // namespace themis
