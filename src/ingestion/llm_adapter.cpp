/**
 * @file llm_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/llm_adapter.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

// SoC NOTE: This file intentionally contains NO includes from llm/.
// The concrete LLM backend is injected via ITextGenerationBackend.
// See include/ingestion/inference_backend.h and llm/llm_ingestion_bridge.h.

namespace themis {
namespace ingestion {

namespace {

bool isReadableModelFile(const std::string& model_path) {
    if (model_path.empty()) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(model_path, ec) || ec) {
        return false;
    }

    auto model_stream = std::ifstream(model_path, std::ios::binary);
    return model_stream.good();
}

} // namespace

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
    if (config_.hasModel()) {
        return isReadableModelFile(config_.model_path);
    }

    return backend_ && backend_->isAvailable();
}

DeonticExtractor::ExtractorFn LegalLlmAdapter::buildExtractorFn() const {
    if (config_.hasModel() && !isReadableModelFile(config_.model_path)) {
        throw std::runtime_error(
            "Configured LLM model file is not accessible: " + config_.model_path);
    }

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


