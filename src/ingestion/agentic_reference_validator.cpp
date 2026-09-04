/**
 * @file agentic_reference_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/agentic_reference_validator.h"
#include <regex>
#include <algorithm>

namespace themis {
namespace ingestion {

namespace {

/// Named-law shortcodes recognized as valid identifiers
static const std::vector<std::string> kKnownLawIds = {
    "BImSchG", "StGB", "DSGVO", "GG", "BGB", "HGB",
    "VwVfG", "UmwG", "KrWG", "AktG", "GmbHG", "InsO",
    "ZPO", "StPO", "VwGO", "AGG", "BBG", "BDG"
};

/// Regex patterns for reference extraction
static const std::regex kSectionRefRe(
    "§\\s*(\\d+[a-z]*)(?:\\s+Abs(?:atz|\\.)?\\s*(\\d+))?"
    "(?:\\s+Satz\\s*(\\d+))?(?:\\s+Nr(?:ummer|\\.)?\\s*(\\d+))?",
    std::regex::ECMAScript | std::regex::icase | std::regex::optimize);

static const std::regex kNamedLawRe(
    "\\b(BImSchG|StGB|DSGVO|GG|BGB|HGB|VwVfG|UmwG|KrWG|AktG"
    "|GmbHG|InsO|ZPO|StPO|VwGO|AGG|BBG|BDG)\\b",
    std::regex::ECMAScript | std::regex::optimize);

static const std::regex kArticleRefRe(
    "Art(?:ikel|\\.)?\\s*(\\d+[a-z]*)(?:\\s+Abs(?:atz|\\.)?\\s*(\\d+))?",
    std::regex::ECMAScript | std::regex::icase | std::regex::optimize);

static const std::regex kEuDirectiveRe(
    "Richtlinie\\s+(\\d+)/(\\d+)/EU",
    std::regex::ECMAScript | std::regex::icase | std::regex::optimize);

} // anonymous namespace

// ============================================================================
// AgenticReferenceValidator implementation
// ============================================================================

AgenticReferenceValidator::AgenticReferenceValidator() {
    // Pre-populate knowledge base with well-known German law identifiers
    for (const auto& law : kKnownLawIds) {
        known_laws_.insert(law);
    }
}

void AgenticReferenceValidator::setExtractorFn(ExtractorFn fn) {
    extractor_fn_ = std::move(fn);
}

void AgenticReferenceValidator::addKnownLaw(const std::string& law_id) {
    known_laws_.insert(law_id);
}

void AgenticReferenceValidator::addKnownSection(const std::string& law_id,
                                                  const std::string& section) {
    known_laws_.insert(law_id);
    known_sections_[law_id].insert(section);
}

void AgenticReferenceValidator::clearKnowledgeBase() {
    known_laws_.clear();
    known_sections_.clear();
}

size_t AgenticReferenceValidator::knownLawCount() const {
    return known_laws_.size();
}

std::vector<LegalReference> AgenticReferenceValidator::extract(
        const std::string& text) const {
    if (extractor_fn_) {
        return extractor_fn_(text);
    }
    return extractRegex(text);
}

ReferenceValidationReport AgenticReferenceValidator::validate(
        const std::string& text) const {
    ReferenceValidationReport report;
    report.extracted = extract(text);

    for (const auto& ref : report.extracted) {
        auto vr = checkReference(ref);
        report.validated.push_back(vr);
        if (!vr.found) {
            ++report.dangling_count;
            report.warnings.push_back(
                "Dangling reference: " + ref.raw_text +
                " (" + ref.canonicalId() + ")");
        }
    }

    return report;
}

std::vector<LegalReference> AgenticReferenceValidator::extractRegex(
        const std::string& text) const {
    std::vector<LegalReference> refs;

    // ── Pattern 1: § N [Abs. M] references ──────────────────────────────────
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(), kSectionRefRe);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::smatch& m = *it;
            LegalReference ref;
            ref.raw_text   = m.str();
            ref.section    = m[1].str();
            if (m[2].matched) {
              ref.subsection = m[2].str();
            }
            if (m[4].matched) {
              ref.item       = m[4].str();
            }
            // law_id is empty → same-document reference

            // Deduplicate
            bool dup = false;
            for (const auto& r : refs) {
                if (r.raw_text == ref.raw_text) { dup = true; break; }
            }
            if (!dup) {
              refs.push_back(std::move(ref));
            }
        }
    }

    // ── Pattern 2: Named-law references (BImSchG, StGB, …) ──────────────────
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(), kNamedLawRe);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::smatch& m = *it;
            // Skip if already captured as part of a § reference
            bool dup = false;
            for (const auto& r : refs) {
                if (r.law_id == m.str()) { dup = true; break; }
            }
            if (!dup) {
                LegalReference ref;
                ref.raw_text = m.str();
                ref.law_id   = m.str();
                refs.push_back(std::move(ref));
            }
        }
    }

    // ── Pattern 3: Artikel N references (EU law, GG) ────────────────────────
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(), kArticleRefRe);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::smatch& m = *it;
            LegalReference ref;
            ref.raw_text   = m.str();
            ref.section    = m[1].str();
            if (m[2].matched) {
              ref.subsection = m[2].str();
            }

            bool dup = false;
            for (const auto& r : refs) {
                if (r.raw_text == ref.raw_text) { dup = true; break; }
            }
            if (!dup) {
              refs.push_back(std::move(ref));
            }
        }
    }

    // ── Pattern 4: EU Directive references ──────────────────────────────────
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(), kEuDirectiveRe);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::smatch& m = *it;
            LegalReference ref;
            ref.raw_text = m.str();
            ref.law_id   = "EU-RL-" + m[1].str() + "/" + m[2].str();

            bool dup = false;
            for (const auto& r : refs) {
                if (r.raw_text == ref.raw_text) { dup = true; break; }
            }
            if (!dup) {
              refs.push_back(std::move(ref));
            }
        }
    }

    return refs;
}

ReferenceValidationResult AgenticReferenceValidator::checkReference(
        const LegalReference& ref) const {
    // Same-document section reference (no law_id) → always "found"
    // (we don't have the full document graph here)
    if (ref.law_id.empty()) {
        return ReferenceValidationResult(ref, true, 0.70,
            "same-document reference (existence not verified)");
    }

    // Named-law reference: check knowledge base
    bool law_known = (known_laws_.count(ref.law_id) > 0);
    if (!law_known) {
        return ReferenceValidationResult(ref, false, 0.90,
            "unknown law identifier: " + ref.law_id);
    }

    // If section-level knowledge is available, verify the section too
    if (!ref.section.empty()) {
        auto sit = known_sections_.find(ref.law_id);
        if (sit != known_sections_.end()) {
            bool sec_known = (sit->second.count(ref.section) > 0);
            if (!sec_known) {
                return ReferenceValidationResult(ref, false, 0.85,
                    "section §" + ref.section + " not found in " + ref.law_id);
            }
        }
        // No section-level data → assume the section exists (partial knowledge)
    }

    return ReferenceValidationResult(ref, true, 0.90, "resolved");
}

} // namespace ingestion
} // namespace themis
