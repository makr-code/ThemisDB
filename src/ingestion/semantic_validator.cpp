/**
 * @file semantic_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/semantic_validator.h"
#include <regex>
#include <algorithm>
#include <numeric>
#include <sstream>

namespace themis {
namespace ingestion {

namespace {

/// Regex that matches a German legal section header (§ N Title)
static const std::regex kSectionRe(
    "^\\s*§\\s*\\d+[a-z]*",
    std::regex::ECMAScript);

/// Regex that detects a date expression in German legal text
static const std::regex kTemporalRe(
    "\\d{1,2}\\.\\s*(?:Januar|Februar|März|April|Mai|Juni|Juli|August|"
    "September|Oktober|November|Dezember)\\s*\\d{4}"
    "|\\d+\\s+(?:Tage?|Wochen?|Monate?|Jahre?)"
    "|in\\s+Kraft\\s+getreten|vom\\s+\\d{1,2}\\.",
    std::regex::ECMAScript | std::regex::icase);

/// Regex used to split a document into per-section fragments
static const std::regex kSplitRe(
    "(?=^\\s*§\\s*\\d)",
    std::regex::ECMAScript);

/// Extract the section reference (§ N) from the beginning of a fragment.
static std::string extractSectionRef(const std::string& fragment) {
    static const std::regex kRefRe(
        "^\\s*(§\\s*\\d+[a-z]*)\\s*(.*?)(?:\\n|$)",
        std::regex::ECMAScript);
    std::smatch m;
    if (std::regex_search(fragment, m, kRefRe)) {
        return m[1].str();
    }
    return "";
}

/// Build a provision_id from document_id and section_ref.
static std::string makeProvisionId(const std::string& doc_id,
                                    const std::string& section_ref) {
    std::string id = doc_id + "_" + section_ref;
    // Replace spaces and special chars with underscores
    for (char& c : id) {
        if (c == ' ' || c == '/' || c == '\\') {
          c = '_';
        }
    }
    return id;
}

} // anonymous namespace

// ============================================================================
// SemanticValidator implementation
// ============================================================================

SemanticValidator::SemanticValidator() = default;

void SemanticValidator::setQualityGates(const LegalQualityGates& gates) {
    gates_ = gates;
}

void SemanticValidator::setValidatorFn(ValidatorFn fn) {
    validator_fn_ = std::move(fn);
}

void SemanticValidator::setExtractor(DeonticExtractor extractor) {
    extractor_ = std::move(extractor);
}

SemanticValidationResult SemanticValidator::validate(
        const DeonticExtraction& extraction, const std::string& text) const {
    if (validator_fn_) {
        return validator_fn_(extraction, text);
    }
    return validateBuiltin(extraction, text);
}

SemanticValidationResult SemanticValidator::validateBuiltin(
        const DeonticExtraction& extraction, const std::string& text) const {
    SemanticValidationResult result;

    // ── Gate 1: Overall confidence ──────────────────────────────────────────
    {
        const double thresh = gates_.min_confidence.threshold;
        bool passed = (extraction.overall_confidence >= thresh);
        result.gate_results.emplace_back(
            gates_.min_confidence.name, passed,
            passed ? "" : ("overall_confidence " +
                           std::to_string(extraction.overall_confidence) +
                           " < threshold " + std::to_string(thresh)));
        if (!passed) {
            result.warnings.push_back(
                "Overall extraction confidence below threshold: " +
                std::to_string(extraction.overall_confidence));
        }
    }

    // ── Gate 2: Deontic category confidence ─────────────────────────────────
    {
        // If at least one category was extracted, confidence is met for gate 2.
        bool has_category = extraction.hasCategory();
        bool passed = has_category &&
                      (extraction.overall_confidence >=
                       gates_.deontic_confidence.threshold);
        result.gate_results.emplace_back(
            gates_.deontic_confidence.name, passed,
            passed ? "" : (has_category
                ? "deontic confidence too low"
                : "no deontic categories extracted"));
        if (!has_category) {
            result.warnings.push_back(
                "No deontic categories were identified in this provision");
        }
    }

    // ── Gate 3: Section hierarchy ────────────────────────────────────────────
    {
        bool has_sec = hasSection(text);
        bool passed  = has_sec;
        result.gate_results.emplace_back(
            gates_.section_hierarchy.name, passed,
            passed ? "" : "no § section pattern found in text");

        // Required gate: failure sets is_valid = false
        if (!passed && gates_.section_hierarchy.required) {
            result.is_valid = false;
            result.inconsistencies.push_back(
                "Document lacks detectable § section structure");
        }
    }

    // ── Gate 4: Temporal expression present ─────────────────────────────────
    {
        bool has_temp = hasTemporal(text);
        result.gate_results.emplace_back(
            gates_.temporal_present.name, has_temp,
            has_temp ? "" : "no temporal expression found");
        if (!has_temp) {
            result.warnings.push_back(
                "No temporal expression (date, period) detected in text");
        }
    }

    // ── Compute semantic score ───────────────────────────────────────────────
    result.semantic_score = computeScore(extraction, text);

    // ── Add entity-related suggestions ──────────────────────────────────────
    if (extraction.entities.empty()) {
        result.suggestions.push_back(
            "No entities extracted; consider enriching with NER model");
    }
    if (extraction.obligations.empty() &&
        extraction.hasCategory() &&
        extraction.primaryCategory() == DeonticCategory::OBLIGATION) {
        result.suggestions.push_back(
            "Obligation detected but no structured obligation extracted; "
            "consider enabling LLM adapter for richer extraction");
    }

    // Forward warnings from extraction
    for (const auto& w : extraction.warnings) {
        result.warnings.push_back(w);
    }

    return result;
}

/*static*/ bool SemanticValidator::hasSection(const std::string& text) {
    return std::regex_search(text, kSectionRe);
}

/*static*/ bool SemanticValidator::hasTemporal(const std::string& text) {
    return std::regex_search(text, kTemporalRe);
}

/*static*/ double SemanticValidator::computeScore(
        const DeonticExtraction& extraction, const std::string& text) {
    double score = 0.0;
    int    factors = 0;

    // Factor 1: overall confidence (weight 0.4)
    if (extraction.overall_confidence > 0.0) {
        score += extraction.overall_confidence * 0.4;
        ++factors;
    }

    // Factor 2: category identified (weight 0.2)
    if (extraction.hasCategory()) {
        score += 0.2;
        ++factors;
    }

    // Factor 3: entities extracted (weight 0.2)
    if (!extraction.entities.empty()) {
        double ent_score = std::min(
            1.0, static_cast<double>(extraction.entities.size()) / 5.0);
        score += ent_score * 0.2;
        ++factors;
    }

    // Factor 4: temporal expression present (weight 0.1)
    if (hasTemporal(text)) {
        score += 0.1;
        ++factors;
    }

    // Factor 5: section structure (weight 0.1)
    if (hasSection(text)) {
        score += 0.1;
        ++factors;
    }

    return (factors > 0) ? score : 0.0;
}

// ============================================================================
// Document-level extraction
// ============================================================================

LegalExtractionResult SemanticValidator::extractDocument(
        const std::string& document_id,
        const std::string& full_text) const {
    LegalExtractionResult doc_result;
    doc_result.document_id = document_id;

    // --- Split into per-section fragments ---
    // We use a simple line-based split on § markers.
    std::vector<std::string> fragments;
    {
        std::istringstream ss(full_text);
        std::string current_fragment;
        std::string line;
        static const std::regex kSectionStart(
            "^\\s*§\\s*\\d+", std::regex::ECMAScript);
        while (std::getline(ss, line)) {
            if (std::regex_search(line, kSectionStart) &&
                !current_fragment.empty()) {
                fragments.push_back(current_fragment);
                current_fragment.clear();
            }
            current_fragment += line + "\n";
        }
        if (!current_fragment.empty()) {
            fragments.push_back(current_fragment);
        }
    }

    // If no sections found, treat the whole text as one provision
    if (fragments.empty()) {
        fragments.push_back(full_text);
    }

    // --- Extract each fragment ---
    double total_score = 0.0;
    for (const auto& frag : fragments) {
        DeonticExtraction extraction = extractor_.extract(frag);
        SemanticValidationResult val_result = validate(extraction, frag);

        LegalProvision prov;
        prov.section_ref          = extractSectionRef(frag);
        prov.provision_id         = makeProvisionId(document_id, prov.section_ref);
        prov.text                 = frag;
        prov.deontic_category     = extraction.primaryCategory();
        prov.category_confidence  = extraction.overall_confidence;
        prov.entities             = extraction.entities;
        prov.obligations          = extraction.obligations;

        doc_result.provisions.push_back(std::move(prov));
        total_score += val_result.semantic_score;

        // Propagate warnings
        for (const auto& w : val_result.warnings) {
            doc_result.warnings.push_back(w);
        }
    }

    // --- Compute document-level score ---
    if (!fragments.empty()) {
        doc_result.quality_score =
            total_score / static_cast<double>(fragments.size());
    }

    // --- Run overall validation on full document ---
    DeonticExtraction full_extraction = extractor_.extract(full_text);
    doc_result.validation = validate(full_extraction, full_text);

    return doc_result;
}

} // namespace ingestion
} // namespace themis
