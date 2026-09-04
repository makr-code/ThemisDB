/**
 * @file scraper_llm_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_llm_evaluator.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

#ifdef THEMIS_ENABLE_LLM
#include "llm/llm_plugin_manager.h"
#include "llm/llm_plugin_interface.h"
#endif

namespace themis {
namespace scraper {

using json = nlohmann::json;

// ============================================================================
// ScraperLLMEvaluator
// ============================================================================

bool ScraperLLMEvaluator::isLlmAvailable() const {
#ifdef THEMIS_ENABLE_LLM
    try {
        auto& mgr = themis::llm::LLMPluginManager::instance();
        return !mgr.listPlugins().empty() && mgr.getDefaultPlugin() != nullptr;
    } catch (...) {}
#endif
    return false;
}

// ============================================================================
// Prompt builder
// ============================================================================

/*static*/ std::string ScraperLLMEvaluator::buildPrompt(
        const std::string& text,
        const GapContext&  gap) {
    // Truncate to ~3000 chars to stay within context window
    const std::string snippet = (text.size() > 3000) ? text.substr(0, 3000) + "…" : text;

    std::ostringstream keywords_str = {};
    for (std::size_t i = 0; i <static_cast<int>(gap.keywords.size()); ++i) {
        if (i > 0) {
          keywords_str << ", ";
        }
        keywords_str << gap.keywords[i];
    }

    return
        "[INST] Du bist ein Experte für Datenqualitätsbewertung im Bereich deutsches Recht "
        "und EU-Recht. Analysiere den folgenden Webseiteninhalt und bewerte ihn bezüglich "
        "des angegebenen Datenlücken-Kontexts.\n\n"
        "Gap-ID: " + gap.gap_id + "\n"
        "Gap-Beschreibung: " + gap.description + "\n"
        "Schlüsselbegriffe: " + keywords_str.str() + "\n\n"
        "Webseiteninhalt:\n" + snippet + "\n\n"
        "Antworte NUR mit validem JSON im Format:\n"
        "{\n"
        "  \"quality_score\": <0.0-1.0>,\n"
        "  \"gap_relevance\": <0.0-1.0>,\n"
        "  \"summary\": \"<Ein-Satz-Zusammenfassung>\",\n"
        "  \"key_entities\": [\"<Entity1>\", \"<Entity2>\"],\n"
        "  \"discard_reason\": \"<Grund oder leer>\"\n"
        "}\n\n"
        "Kriterien:\n"
        "- quality_score: Textqualität, Vollständigkeit, Fachlichkeit (0=unbrauchbar, 1=exzellent)\n"
        "- gap_relevance: Wie gut deckt der Inhalt die Datenlücke ab (0=irrelevant, 1=perfekt)\n"
        "- key_entities: Relevante Rechtsbegriffe, Gesetze, Gerichte, Institutionen\n"
        "- discard_reason: Leer wenn der Inhalt behalten werden soll; sonst kurze Begründung\n"
        "[/INST]";
}

// ============================================================================
// LLM response parser
// ============================================================================

/*static*/ EvaluationResult ScraperLLMEvaluator::parseLlmResponse(
        const std::string& response,
        double threshold) {
    EvaluationResult result;

    // Find outermost JSON object
    const std::size_t first = response.find('{');
    const std::size_t last  = response.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last < first) {
        result.discard_reason = "LLM returned no JSON";
        result.below_threshold = true;
        return result;
    }

    try {
        const json j = json::parse(response.substr(first, last - first + 1));

        if (j.contains("quality_score") && j["quality_score"].is_number())
            result.quality_score = j["quality_score"].get<double>();
        if (j.contains("gap_relevance") && j["gap_relevance"].is_number())
            result.gap_relevance = j["gap_relevance"].get<double>();
        if (j.contains("summary") && j["summary"].is_string())
            result.summary = j["summary"].get<std::string>();
        if (j.contains("discard_reason") && j["discard_reason"].is_string())
            result.discard_reason = j["discard_reason"].get<std::string>();
        if (j.contains("key_entities") && j["key_entities"].is_array()) {
            for (const auto& e : j["key_entities"]) {
                if (e.is_string()) {
                  result.key_entities.push_back(e.get<std::string>());
                }
            }
        }
    } catch (const json::parse_error& e) {
        result.discard_reason = std::string("JSON parse error: ") + e.what();
        result.below_threshold = true;
        return result;
    }

    result.below_threshold = result.quality_score < threshold;
    return result;
}

// ============================================================================
// Heuristic fallback
// ============================================================================

/*static*/ EvaluationResult ScraperLLMEvaluator::heuristicScore(
        const std::string& text,
        const GapContext&  gap,
        double             threshold) {
    EvaluationResult result = {};

    if (text.empty()) {
        result.discard_reason  = "Empty text";
        result.below_threshold = true;
        return result;
    }

    // Lowercase text for case-insensitive matching
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    // Keyword density scoring
    int hits = 0;
    for (const auto& kw : gap.keywords) {
        std::string lkw = kw;
        std::transform(lkw.begin(), lkw.end(), lkw.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        std::size_t pos = 0;
        while ((pos = lower.find(lkw, pos)) != std::string::npos) {
            ++hits;
            pos += lkw.size();
        }
    }

    // Normalise: score based on hit density per 1000 chars, capped at 1.0
    const double density = gap.keywords.empty() ? 0.0
        : static_cast<double>(hits) /
          (static_cast<double>(gap.keywords.size()) *
           (static_cast<double>(text.size()) / 1000.0 + 1.0));
    result.gap_relevance = std::min(1.0, density * 3.0);

    // Quality score: length-based proxy (short pages are lower quality)
    const double len_score = std::min(1.0, static_cast<double>(text.size()) / 2000.0);
    result.quality_score = 0.4 * len_score + 0.6 * result.gap_relevance;

    result.summary = "(heuristic) " + std::to_string(hits) + " keyword hits in "
                   + std::to_string(text.size()) + " chars";
    result.below_threshold = result.quality_score < threshold;

    if (result.below_threshold && result.discard_reason.empty()) {
        result.discard_reason = "Heuristic quality score "
            + std::to_string(result.quality_score)
            + " below threshold " + std::to_string(threshold);
    }
    return result;
}

// ============================================================================
// evaluate()
// ============================================================================

EvaluationResult ScraperLLMEvaluator::evaluate(
        const std::string& text,
        const std::string& /*url*/,
        const GapContext&  gap,
        double             threshold) const {
#ifdef THEMIS_ENABLE_LLM
    if (isLlmAvailable() && !text.empty()) {
        try {
            themis::llm::InferenceRequest req;
            req.prompt       = buildPrompt(text, gap);
            req.model_id     = "default";
            req.max_tokens   = 256;
            req.temperature  = 0.1f;
            req.grammar_type = "json";

            const auto response =
                themis::llm::LLMPluginManager::instance().generate(req);
            return parseLlmResponse(response.text, threshold);
        } catch (...) {
            // Fall through to heuristic on any LLM error
        }
    }
#endif
    return heuristicScore(text, gap, threshold);
}

} // namespace scraper
} // namespace themis

