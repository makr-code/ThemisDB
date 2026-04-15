/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_adapter.cpp                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
    • 7a78813496  2026-03-15  fix(ingestion): address code review - type-safe confidenc... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/llm_adapter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

// SoC NOTE: This file intentionally contains NO includes from llm/.
// The concrete LLM backend is injected via ITextGenerationBackend.
// See include/ingestion/inference_backend.h and llm/llm_ingestion_bridge.h.

namespace themis {
namespace ingestion {

// ============================================================================
// LegalLlmAdapter implementation
// ============================================================================

LegalLlmAdapter::LegalLlmAdapter()
    : backend_(std::make_shared<NullTextGenerationBackend>()) {}

LegalLlmAdapter::LegalLlmAdapter(std::shared_ptr<ITextGenerationBackend> backend)
    : backend_(backend ? std::move(backend)
                       : std::make_shared<NullTextGenerationBackend>()) {}

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
    return backend_ && backend_->isAvailable();
}

DeonticExtractor::ExtractorFn LegalLlmAdapter::buildExtractorFn() const {
    if (!isLlmAvailable()) {
        // No backend available — return empty fn so DeonticExtractor uses regex.
        return {};
    }

    // Capture backend + config by value so the lambda outlives this adapter.
    auto captured_backend = backend_;
    LlmAdapterConfig captured_config = config_;

    return [captured_backend, captured_config](const std::string& text) -> DeonticExtraction {
        try {
            const std::string response = captured_backend->generate(
                LegalLlmAdapter::buildPrompt(text),
                512,
                captured_config.temperature,
                captured_config.adapter_path);
            return LegalLlmAdapter::parseLlmResponse(response);
        } catch (const std::exception& e) {
            DeonticExtraction result;
            result.warnings.push_back(
                std::string("ITextGenerationBackend::generate() threw: ") + e.what());
            return result;
        }
    };
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
