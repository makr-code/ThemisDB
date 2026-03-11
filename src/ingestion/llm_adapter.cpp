/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_adapter.cpp                                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-11                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready (Phase 1: stub + injectable interface)  ║
          📋 Phase 2: wire to Mistral 7B via llama.cpp               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/llm_adapter.h"
#include <regex>
#include <fstream>
#include <sstream>

// Phase 2: When THEMIS_ENABLE_LLM is ON, include the llama.cpp bridge.
// The include is guarded so Phase 1 compiles without llama.cpp.
#ifdef THEMIS_ENABLE_LLM
// #include "llm/llama_resource_manager.h"  // Phase 2: uncomment when wiring
#endif

namespace themis {
namespace ingestion {

// ============================================================================
// LegalLlmAdapter implementation
// ============================================================================

LegalLlmAdapter::LegalLlmAdapter() = default;
LegalLlmAdapter::~LegalLlmAdapter() = default;
LegalLlmAdapter::LegalLlmAdapter(LegalLlmAdapter&&) noexcept = default;
LegalLlmAdapter& LegalLlmAdapter::operator=(LegalLlmAdapter&&) noexcept = default;

void LegalLlmAdapter::setConfig(const LlmAdapterConfig& config) {
    config_ = config;
}

const LlmAdapterConfig& LegalLlmAdapter::getConfig() const {
    return config_;
}

bool LegalLlmAdapter::isLlmAvailable() const {
#ifdef THEMIS_ENABLE_LLM
    // Phase 2: check that the model file exists and is readable
    if (!config_.hasModel()) {
        return false;
    }
    // Attempt a lightweight file existence check
    std::ifstream f(config_.model_path);
    return f.good();
#else
    // Phase 1: LLM support not compiled in; always fall back to regex
    return false;
#endif
}

DeonticExtractor::ExtractorFn LegalLlmAdapter::buildExtractorFn() const {
    if (!isLlmAvailable()) {
        // Phase 1 / no model configured: return empty fn → DeonticExtractor
        // will use its built-in regex implementation.
        return {};
    }

#ifdef THEMIS_ENABLE_LLM
    // Phase 2: capture config by value so the function outlives this adapter.
    LlmAdapterConfig captured_config = config_;
    return [captured_config](const std::string& text) -> DeonticExtraction {
        // ── Phase 2 implementation (uncomment when wiring llama.cpp) ────────
        // auto& llm = themis::llm::LlamaResourceManager::instance();
        // auto response = llm.infer(buildPrompt(text), captured_config);
        // return parseLlmResponse(response);
        // ────────────────────────────────────────────────────────────────────

        // Temporary fallback until Phase 2 wiring is complete:
        DeonticExtractor fallback;
        fallback.setConfidenceThreshold(0.75);
        (void)captured_config;
        return fallback.extract(text);
    };
#else
    return {};
#endif
}

DeonticExtractor LegalLlmAdapter::buildExtractor(double confidence_threshold) const {
    DeonticExtractor extractor;
    extractor.setConfidenceThreshold(confidence_threshold);

    auto fn = buildExtractorFn();
    if (fn) {
        extractor.setExtractorFn(std::move(fn));
    }
    // If fn is empty (Phase 1 / no LLM), the extractor uses built-in regex.

    return extractor;
}

// ============================================================================
// Private helpers
// ============================================================================

/*static*/ std::string LegalLlmAdapter::buildPrompt(const std::string& text) {
    // Structured prompt for a German legal text deontic extraction task.
    // Instruction-tuned format (compatible with Mistral 7B Instruct):
    return
        "[INST] Du bist ein Experte für deutsches Verwaltungsrecht. "
        "Analysiere den folgenden Gesetzestext und extrahiere:\n"
        "1. Die deontische Kategorie: obligation | permission | prohibition | "
        "definition | condition | exception | reference\n"
        "2. Entitäten: law_reference, person_role, organization, temporal, threshold_value\n"
        "3. Strukturierte Pflichten: actor, action, condition\n\n"
        "Antworte NUR mit validem JSON im Format:\n"
        "{\n"
        "  \"deontic_category\": \"<category>\",\n"
        "  \"confidence\": <0.0-1.0>,\n"
        "  \"entities\": [{\"type\": \"<type>\", \"value\": \"<value>\"}],\n"
        "  \"obligations\": [{\"actor\": \"\", \"action\": \"\", \"condition\": \"\"}]\n"
        "}\n\n"
        "Gesetzestext:\n" + text + "\n[/INST]";
}

/*static*/ DeonticExtraction LegalLlmAdapter::parseLlmResponse(
        const std::string& llm_response) {
    DeonticExtraction result;

    // ── Simple JSON extraction via regex (no heavy JSON library dependency) ─
    // Phase 2 TODO: replace with a proper JSON parser (nlohmann/json or
    // the JSON utilities already present in ThemisDB's utils module).

    // Extract "deontic_category"
    static const std::regex kCatRe(
        "\"deontic_category\"\\s*:\\s*\"([^\"]+)\"",
        std::regex::ECMAScript);
    std::smatch m;
    if (std::regex_search(llm_response, m, kCatRe)) {
        auto cat = deonticCategoryFromString(m[1].str());
        if (cat != DeonticCategory::UNKNOWN) {
            result.deontic_categories.push_back(cat);
        }
    }

    // Extract "confidence"
    static const std::regex kConfRe(
        "\"confidence\"\\s*:\\s*([0-9.]+)",
        std::regex::ECMAScript);
    if (std::regex_search(llm_response, m, kConfRe)) {
        try {
            result.overall_confidence = std::stod(m[1].str());
        } catch (...) {
            result.overall_confidence = 0.0;
        }
    }

    // Extract entities (simplified: pick first type+value pair)
    static const std::regex kEntRe(
        "\"type\"\\s*:\\s*\"([^\"]+)\"[^}]*\"value\"\\s*:\\s*\"([^\"]+)\"",
        std::regex::ECMAScript);
    auto begin = std::sregex_iterator(llm_response.begin(), llm_response.end(), kEntRe);
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        const std::smatch& em = *it;
        result.entities.emplace_back(em[1].str(), em[2].str(), em[2].str(), 0.85);
    }

    if (result.deontic_categories.empty()) {
        result.warnings.push_back("LLM response did not yield a valid deontic category");
    }

    return result;
}

} // namespace ingestion
} // namespace themis
