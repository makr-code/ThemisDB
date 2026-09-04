/**
 * @file deontic_extractor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/deontic_extractor.h"
#include <regex>
#include <algorithm>
#include <cctype>

namespace themis {
namespace ingestion {

// ============================================================================
// Regex pattern tables (compiled once at construction)
// ============================================================================

namespace {

/// Case-insensitive pattern helper
static std::regex makeRe(const std::string& pattern) {
    return std::regex(pattern,
        std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
}

// Deontic patterns ordered by specificity.
// PROHIBITION must be checked before PERMISSION (darf nicht vs. darf).
struct DeonticPattern {
    DeonticCategory  category;
    std::regex       re = {};
    double           base_confidence;
};

static const std::vector<DeonticPattern>& getDeonticPatterns() {
    static const std::vector<DeonticPattern> kPatterns = {
        // PROHIBITION (check before PERMISSION)
        { DeonticCategory::PROHIBITION,
          makeRe("darf\\s+nicht|dürfen\\s+nicht|ist\\s+verboten|sind\\s+verboten"
                 "|ist\\s+unzulässig|sind\\s+unzulässig"
                 "|ist\\s+nicht\\s+gestattet|ist\\s+untersagt|sind\\s+untersagt"),
          0.95 },

        // OBLIGATION
        { DeonticCategory::OBLIGATION,
          makeRe("\\bmuss\\b|\\bmüssen\\b|\\bhat\\s+zu\\b|\\bhaben\\s+zu\\b"
                 "|ist\\s+verpflichtet|sind\\s+verpflichtet"
                 "|\\bbedarf\\b|\\bbedürfen\\b"
                 "|ist\\s+erforderlich|sind\\s+erforderlich"
                 "|hat\\s+sicherzustellen"),
          0.95 },

        // PERMISSION
        { DeonticCategory::PERMISSION,
          makeRe("\\bdarf\\b|\\bdürfen\\b|\\bkann\\b|\\bkönnen\\b"
                 "|ist\\s+berechtigt|sind\\s+berechtigt"
                 "|ist\\s+zulässig|sind\\s+zulässig"
                 "|steht\\s+frei|ist\\s+gestattet"),
          0.90 },

        // DEFINITION
        { DeonticCategory::DEFINITION,
          makeRe("im\\s+Sinne\\s+dieses\\s+Gesetzes"
                 "|im\\s+Sinne\\s+dieser\\s+Verordnung"
                 "|im\\s+Sinne\\s+des\\s+§"
                 "|\\bgilt\\s+als\\b|\\bgelten\\s+als\\b"
                 "|\\bversteht\\s+man\\b"
                 "|\\bsind\\s+Anlagen\\b|\\bist\\s+Anlage\\b"
                 "|Begriffsbestimmung"),
          0.85 },

        // EXCEPTION
        { DeonticCategory::EXCEPTION,
          makeRe("\\bausgenommen\\b|\\baußer\\b"
                 "|mit\\s+Ausnahme|gilt\\s+nicht\\s+für"
                 "|findet\\s+keine\\s+Anwendung|es\\s+sei\\s+denn"
                 "|Absatz\\s+\\d+\\s+gilt\\s+nicht"),
          0.85 },

        // CONDITION
        { DeonticCategory::CONDITION,
          makeRe("\\bwenn\\b|\\bfalls\\b|\\bsofern\\b"
                 "|unter\\s+der\\s+Voraussetzung"
                 "|\\bsoweit\\b|\\bvorbehaltlich\\b"
                 "|\\bim\\s+Fall\\b|bei\\s+Vorliegen"),
          0.80 },

        // REFERENCE
        { DeonticCategory::REFERENCE,
          makeRe("\\bgemäß\\b|nach\\s+Maßgabe|im\\s+Sinne\\s+des\\s+§"
                 "|nach\\s+§\\s*\\d|entsprechend\\s+§"
                 "|nach\\s+Absatz|nach\\s+Satz"),
          0.75 },
    };
    return kPatterns;
}

// Entity extraction patterns
struct EntityPattern {
    std::string  type;
    std::regex   re;
};

static const std::vector<EntityPattern>& getEntityPatterns() {
    static const std::vector<EntityPattern> kPatterns = {
        { "law_reference",
          makeRe("§\\s*\\d+[a-z]*(?:\\s+Abs\\.?\\s*\\d+)?(?:\\s+Satz\\s*\\d+)?"
                 "(?:\\s+Nr\\.?\\s*\\d+)?"
                 "|\\b(?:BImSchG|StGB|DSGVO|GG|BGB|HGB|VwVfG|UmwG|KrWG)\\b"
                 "|Art(?:ikel|\\.)?\\s*\\d+[a-z]*"
                 "|Richtlinie\\s+\\d+/\\d+/EU") },

        { "person_role",
          makeRe("\\b(?:Antragsteller|Genehmigungsinhaber|Betreiber|Behörde"
                 "|Beamter|Kläger|Beklagter|Beigeladener"
                 "|Bevollmächtigter|Vertreter|Beistand"
                 "|natürliche\\s+Person|juristische\\s+Person)\\b") },

        { "organization",
          makeRe("\\b(?:Umweltbundesamt|UBA|Bundesministerium|Ministerium"
                 "|Verwaltungsbehörde|Genehmigungsbehörde"
                 "|Bundesamt|Landesamt|Landkreis|Gemeinde)\\b") },

        { "temporal",
          makeRe("\\d+\\s+(?:Tage?|Wochen?|Monate?|Jahre?)"
                 "|innerhalb\\s+von\\s+\\d+"
                 "|binnen\\s+\\d+"
                 "|\\d{1,2}\\.\\s*(?:Januar|Februar|März|April|Mai|Juni"
                 "|Juli|August|September|Oktober|November|Dezember)\\s*\\d{4}") },

        { "threshold_value",
          makeRe("\\d+(?:[,.]\\d+)?\\s*(?:mg|kg|t|m³|l|kW|MW|GW|dB|µg|%)"
                 "|mehr\\s+als\\s+\\d+"
                 "|mindestens\\s+\\d+"
                 "|höchstens\\s+\\d+"
                 "|bis\\s+zu\\s+\\d+") },
    };
    return kPatterns;
}

} // anonymous namespace

// ============================================================================
// DeonticExtractor implementation
// ============================================================================

DeonticExtractor::DeonticExtractor()
    : confidence_threshold_(0.75) {}

void DeonticExtractor::setConfidenceThreshold([[maybe_unused]] double threshold) {
    confidence_threshold_ = threshold;
}

void DeonticExtractor::setExtractorFn(ExtractorFn fn) {
    extractor_fn_ = std::move(fn);
}

DeonticExtraction DeonticExtractor::extract(const std::string& text) const {
    if (extractor_fn_) {
        return extractor_fn_(text);
    }
    return extractRegex(text);
}

std::vector<ExtractedEntity> DeonticExtractor::extractEntities(
        const std::string& text) const {
    return extractEntitiesRegex(text);
}

DeonticExtraction DeonticExtractor::extractRegex(const std::string& text) const {
    DeonticExtraction result;

    const auto& patterns = getDeonticPatterns();
    double total_confidence = 0.0;
    int    matched_count    = 0;
    bool   matched_prohibition = false;

    for (const auto& dp : patterns) {
        if (std::regex_search(text, dp.re)) {
            if (dp.category == DeonticCategory::PERMISSION && matched_prohibition) {
                continue;
            }
            double conf = dp.base_confidence;
            if (conf >= confidence_threshold_) {
                result.deontic_categories.push_back(dp.category);
                if (dp.category == DeonticCategory::PROHIBITION) {
                    matched_prohibition = true;
                }
                total_confidence += conf;
                ++matched_count;
            } else {
                result.warnings.push_back(
                    "Low-confidence match for category '" +
                    deonticCategoryToString(dp.category) +
                    "' (" + std::to_string(conf) + " < threshold " +
                    std::to_string(confidence_threshold_) + ")");
            }
        }
    }

    // Compute overall confidence
    if (matched_count > 0) {
        result.overall_confidence = total_confidence / matched_count;
    } else {
        result.overall_confidence = 0.0;
        result.warnings.push_back("No deontic patterns matched in text");
    }

    // Extract entities
    result.entities = extractEntitiesRegex(text);

    // Build structured obligations for OBLIGATION provisions
    bool is_obligation = std::find(result.deontic_categories.begin(),
                                    result.deontic_categories.end(),
                                    DeonticCategory::OBLIGATION)
                         != result.deontic_categories.end();
    if (is_obligation) {
        // Heuristic: extract actor from entity list
        std::string actor = {};
        for (const auto& ent : result.entities) {
            if (ent.type == "person_role") {
                actor = ent.value;
                break;
            }
        }

        // Derive action from obligation pattern match (simplified heuristic)
        std::string action = {};
        static const std::regex kBedarf(
            "bedarf(?:.*?)(einer|des|der)\\s+([\\w]+)",
            std::regex::ECMAScript | std::regex::icase);
        std::smatch m = {};
        if (std::regex_search(text, m, kBedarf) && static_cast<int>(m.size()) >= 3) {
            action = m[1].str() + " " + m[2].str() + " einholen";
        }

        if (!action.empty() || !actor.empty()) {
            result.obligations.emplace_back(actor, action, "", 0.75);
        }
    }

    return result;
}

std::vector<ExtractedEntity> DeonticExtractor::extractEntitiesRegex(
        const std::string& text) const {
    std::vector<ExtractedEntity> entities;

    const auto& patterns = getEntityPatterns();
    for (const auto& ep : patterns) {
        auto begin = std::sregex_iterator(text.begin(), text.end(), ep.re);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const std::smatch& match = *it;
            std::string raw = match.str();
            // Deduplicate: skip if already present with same type+value
            bool dup = false;
            for (const auto& e : entities) {
                if (e.type == ep.type && e.value == raw) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                entities.emplace_back(ep.type, raw, raw, 0.85);
            }
        }
    }

    return entities;
}

} // namespace ingestion
} // namespace themis
