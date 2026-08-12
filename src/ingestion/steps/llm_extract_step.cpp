/**
 * @file llm_extract_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_step.h"
#include <stdexcept>
#include "ingestion/inference_backend.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

// ─────────────────────────────────────────────────────────────────────────────
// LlmExtractStep — builtin.llm_extract
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief `builtin.llm_extract` — generic LLM extraction step.
 *
 * Renders a prompt template from the YAML config, calls the injected
 * `ITextGenerationBackend`, and parses the response.  Extracted entities
 * (if `output_entities` is true) are appended to `ctx.entities`; raw output
 * is stored in `ctx.extra["llm_extract.<step_name>"]`.
 *
 * Config keys:
 *  - `prompt_template`  string  REQUIRED — prompt template with placeholders:
 *                              `{text}` → ctx.raw_text (first 4000 chars)
 *                              `{language}` → ctx.text_language or config `language`
 *                              `{filename}` → ctx.manifest.filename_stem
 *  - `language`         string  default "de"
 *  - `max_tokens`       int     default 512
 *  - `temperature`      float   default 0.1
 *  - `lora_adapter`     string  default ""
 *  - `output_entities`  bool    default false
 *                              when true, the response is parsed as a JSON array
 *                              of {text, type, confidence?} objects
 *  - `entity_type`      string  default "CHUNK" — entity type for all extracted entities
 *  - `min_confidence`   float   default 0.5
 *
 * Example YAML config:
 * @code
 * - name: llm_summarise
 *   plugin: builtin.llm_extract
 *   config:
 *     prompt_template: |
 *       Summarise the following {language} legal text in 3 sentences.
 *       Text: {text}
 *     max_tokens: 256
 *     temperature: 0.2
 * @endcode
 */
class LlmExtractStep : public IIngestionStep {
public:
    explicit LlmExtractStep(
        std::shared_ptr<ITextGenerationBackend> backend = nullptr)
        : backend_(backend
                   ? std::move(backend)
                   : std::make_shared<NullTextGenerationBackend>()) {}

    // IThemisPlugin
    const char* getName()    const override { return "builtin.llm_extract"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    bool canHandle(const ExtractionContext& ctx) const override {
        // Skip when no text available or backend unavailable
        return ctx.hasText() && backend_ && backend_->isAvailable();
    }

    void setBackend(std::shared_ptr<ITextGenerationBackend> b) {
        backend_ = std::move(b);
    }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        if (!ctx.hasText()) return {};
        if (!backend_ || !backend_->isAvailable()) {
            ctx.warnings.push_back(
                "builtin.llm_extract: backend unavailable, step skipped");
            return {};
        }

        // Get prompt template — required
        if (!cfg.config.contains("prompt_template")) {
            ctx.warnings.push_back(
                "builtin.llm_extract: missing prompt_template config, step skipped");
            return {};
        }
        std::string tmpl = cfg.config["prompt_template"].get<std::string>();

        const std::string lang =
            cfg.config.value("language", ctx.text_language.empty()
                             ? std::string("de") : ctx.text_language);
        const int    max_tokens  = cfg.config.value("max_tokens",  512);
        const double temperature = cfg.config.value("temperature", 0.1);
        const std::string lora   = cfg.config.value("lora_adapter", std::string(""));
        const bool   output_ents = cfg.config.value("output_entities", false);
        const double min_conf    = cfg.config.value("min_confidence",  0.5);
        const std::string etype_str =
            cfg.config.value("entity_type", std::string("CHUNK"));

        // Render template
        const std::string text_snippet = ctx.raw_text.substr(
            0, std::min(ctx.raw_text.size(), std::size_t{4000}));
        std::string prompt = tmpl;
        prompt = replaceAll(prompt, "{text}",     text_snippet);
        prompt = replaceAll(prompt, "{language}", lang);
        prompt = replaceAll(prompt, "{filename}", ctx.manifest.filename_stem);

        // Call backend
        const std::string response =
            backend_->generate(prompt, max_tokens, temperature, lora);

        if (response.empty()) {
            ctx.warnings.push_back(
                "builtin.llm_extract: LLM returned empty response");
            return {};
        }

        // Store raw output in extra
        const std::string extra_key = "llm_extract." + cfg.name;
        ctx.extra[extra_key] = response;

        // Optionally parse as entity list
        if (output_ents) {
            parseAndAppendEntities(ctx, response, etype_str, cfg.name, min_conf);
        }

        return {};
    }

private:
    std::shared_ptr<ITextGenerationBackend> backend_;

    static std::string replaceAll(std::string str,
                                   const std::string& from,
                                   const std::string& to) {
        std::size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
        return str;
    }

    static EntityType entityTypeFromStr(const std::string& s) {
        if (s == "PERSON" || s == "PER") return EntityType::PERSON;
        if (s == "ORG")                  return EntityType::ORGANIZATION;
        if (s == "LOCATION" || s == "LOC") return EntityType::LOCATION;
        if (s == "DATE")                 return EntityType::DATE;
        if (s == "LAW")                  return EntityType::LEGAL_NORM_REFERENCE;
        if (s == "LEGAL_PROVISION")      return EntityType::LEGAL_PROVISION;
        if (s == "OBLIGATION")           return EntityType::LEGAL_OBLIGATION;
        if (s == "PROHIBITION")          return EntityType::LEGAL_PROHIBITION;
        if (s == "PERMISSION")           return EntityType::LEGAL_PERMISSION;
        return EntityType::CHUNK;
    }

    static void parseAndAppendEntities(ExtractionContext& ctx,
                                        const std::string& response,
                                        const std::string& default_etype,
                                        const std::string& step_name,
                                        double min_conf) {
        // Try to parse JSON array
        try {
            auto arr = json::parse(response);
            if (!arr.is_array()) return;
            for (const auto& item : arr) {
                const std::string text   = item.value("text",       "");
                const std::string etype  = item.value("type",       default_etype);
                const double      conf   = item.value("confidence", 1.0);
                if (text.empty() || conf < min_conf) continue;

                BaseEntity ent;
                ent.entity_type    = entityTypeFromStr(etype);
                ent.source_file_id = ctx.manifest.file_id;
                ent.text           = text;
                ent.id             = "llm:" + ctx.manifest.file_id + ":"
                                     + std::to_string(
                                         std::hash<std::string>{}(text + etype + step_name));
                ent.properties["llm_type"]  = etype;
                ent.properties["step_name"] = step_name;
                ent.provenance.step_name   = step_name;
                ent.provenance.plugin_name = "builtin.llm_extract";
                ent.provenance.confidence  = conf;
                ctx.entities.push_back(std::move(ent));
            }
        } catch (...) {
            // Non-JSON response — not an error, just not entity output
        }
    }
};

} // namespace builtin
} // namespace ingestion
} // namespace themis

// ─────────────────────────────────────────────────────────────────────────────
// Factory implementation
// ─────────────────────────────────────────────────────────────────────────────

#include "ingestion/builtin_step_factories.h"

namespace themis {
namespace ingestion {
namespace builtin {

std::shared_ptr<IIngestionStep> createLlmExtractStep(
        std::shared_ptr<ITextGenerationBackend> backend) {
    return std::make_shared<LlmExtractStep>(std::move(backend));
}

} // namespace builtin
} // namespace ingestion
} // namespace themis

