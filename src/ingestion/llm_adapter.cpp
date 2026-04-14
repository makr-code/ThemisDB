/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_adapter.cpp                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 07:02:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7a78813496  2026-03-15  fix(ingestion): address code review - type-safe confidenc... ║
    • c4c4c27fa5  2026-03-15  feat(ingestion): LLMIngestionAdapter Phase 2 - wire llama... ║
    • 2bb85b14f2  2026-03-11  feat(ingestion): add llm_adapter.h/cpp + fix README gaps ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/llm_adapter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

// Phase 2: When THEMIS_ENABLE_LLM is ON, include the llama.cpp bridge.
// The include is guarded so Phase 1 compiles without llama.cpp.
#ifdef THEMIS_ENABLE_LLM
#include "llm/llama_resource_manager.h"
#include "llm/llm_plugin_manager.h"
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
#ifdef THEMIS_ENABLE_LLM
    // Phase 2 health check: when a model path is explicitly configured but the
    // GGUF file does not exist or is not readable, fail fast with a clear error
    // rather than silently falling back to the stub regex implementation.
    if (config_.hasModel()) {
        std::ifstream probe(config_.model_path);
        if (!probe.good()) {
            throw std::runtime_error(
                "LegalLlmAdapter: GGUF model file is not accessible: " +
                config_.model_path);
        }
    }
#endif

    if (!isLlmAvailable()) {
        // Phase 1 / no model configured: return empty fn → DeonticExtractor
        // will use its built-in regex implementation.
        return {};
    }

#ifdef THEMIS_ENABLE_LLM
    // Phase 2: capture config by value so the function outlives this adapter.
    LlmAdapterConfig captured_config = config_;
    return [captured_config](const std::string& text) -> DeonticExtraction {
        themis::llm::InferenceRequest req;
        req.prompt       = LegalLlmAdapter::buildPrompt(text);
        req.model_id     = "default";
        req.max_tokens   = 512;
        req.temperature  = static_cast<float>(captured_config.temperature);
        // "json" is a built-in grammar type supported by LLMPluginManager
        // (see InferenceRequest::grammar_type) that constrains generation to
        // produce syntactically valid JSON — a best-effort hint to the backend.
        req.grammar_type = "json";

        if (captured_config.hasAdapter()) {
            req.lora_adapter_id = captured_config.adapter_path;
        }

        auto response = themis::llm::LLMPluginManager::instance().generate(req);
        return LegalLlmAdapter::parseLlmResponse(response.text);
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

    // ── JSON extraction via nlohmann::json ──────────────────────────────────
    // The LLM may emit extra text around the JSON object; locate the outermost
    // '{' … '}' block and parse only that portion so stray preamble / suffix
    // tokens do not break the parser.
    const std::size_t first = llm_response.find('{');
    const std::size_t last  = llm_response.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last < first) {
        result.warnings.push_back("LLM response contains no JSON object");
        return result;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(llm_response.substr(first, last - first + 1));
    } catch (const nlohmann::json::parse_error& e) {
        result.warnings.push_back(
            std::string("LLM response JSON parse error: ") + e.what());
        return result;
    }

    // Extract "deontic_category"
    if (j.contains("deontic_category") && j["deontic_category"].is_string()) {
        auto cat = deonticCategoryFromString(j["deontic_category"].get<std::string>());
        if (cat != DeonticCategory::UNKNOWN) {
            result.deontic_categories.push_back(cat);
        }
    }

    // Extract "confidence"
    if (j.contains("confidence") && j["confidence"].is_number()) {
        try {
            result.overall_confidence = j["confidence"].get<double>();
        } catch (...) {
            result.overall_confidence = 0.0;
        }
    }

    // Extract entities array
    if (j.contains("entities") && j["entities"].is_array()) {
        for (const auto& ent : j["entities"]) {
            if (ent.is_object() &&
                ent.contains("type")  && ent["type"].is_string() &&
                ent.contains("value") && ent["value"].is_string()) {
                const std::string type_str  = ent["type"].get<std::string>();
                const std::string value_str = ent["value"].get<std::string>();
                result.entities.emplace_back(type_str, value_str, value_str, 0.85);
            }
        }
    }

    if (result.deontic_categories.empty()) {
        result.warnings.push_back("LLM response did not yield a valid deontic category");
    }

    return result;
}

} // namespace ingestion
} // namespace themis
