/**
 * @file ner_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
#include <algorithm>
#include <cctype>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Entity Type From String.
 * @param[in] s Input parameter.
 * @return Return value.
 * @details Implements entityTypeFromString without additional internal calls.
 */
EntityType entityTypeFromString(const std::string& s) {
    if (s == "PER" || s == "PERSON") {
      return EntityType::PERSON;
    }
    if (s == "ORG") {
      return EntityType::ORGANIZATION;
    }
    if (s == "LOC" || s == "LOCATION") {
      return EntityType::LOCATION;
    }
    if (s == "DATE") {
      return EntityType::DATE;
    }
    if (s == "LAW") {
      return EntityType::LEGAL_NORM_REFERENCE;
    }
    return EntityType::UNKNOWN;
}

struct RegexMatch {
    std::string text = {};
    std::string etype;
    std::size_t offset;
};

/**
 * @brief Run German Regex Ner.
 * @param[in] text Input parameter.
 * @param[in] requested_types Input parameter.
 * @return Return value.
 * @details Calls: empty(), std::find(), begin(), end(), wants(), re_law(), std::sregex_iterator(), push_back().
 */
std::vector<RegexMatch> runGermanRegexNer(const std::string& text,
                                          const std::vector<std::string>& requested_types) {
    std::vector<RegexMatch> results;

    auto wants = [&](const std::string& t) {
        return requested_types.empty()
               || std::find(requested_types.begin(), requested_types.end(), t)
                  != requested_types.end();
    };

    // §-references → LAW / LEGAL_NORM_REFERENCE
    if (wants("LAW")) {
        static const std::regex re_law(
            R"((?:§{1,2}|Art\.)\s*\d+(?:\s*(?:Abs\.|Abs)\s*\d+)?(?:\s*(?:S\.|Satz)\s*\d+)?(?:\s+\w+)?)",
            std::regex::optimize);
        for (auto it = std::sregex_iterator(text.begin(), text.end(), re_law);
             it != std::sregex_iterator(); ++it) {
            results.push_back({it->str(), "LAW",
                               static_cast<std::size_t>(it->position())});
        }
    }

    // ISO dates and German dates → DATE
    if (wants("DATE")) {
        static const std::regex re_date(
            R"(\b(?:\d{1,2}\.\d{1,2}\.\d{2,4}|\d{4}-\d{2}-\d{2})\b)",
            std::regex::optimize);
        for (auto it = std::sregex_iterator(text.begin(), text.end(), re_date);
             it != std::sregex_iterator(); ++it) {
            results.push_back({it->str(), "DATE",
                               static_cast<std::size_t>(it->position())});
        }
    }

    // Aktenzeichen → maps to LEGAL_AKTENZEICHEN via properties
    if (wants("LAW")) {
        static const std::regex re_az(
            R"(\bAz\.\s*[A-Z]\s*\d+\s*/\s*\d{2,4}\b)",
            std::regex::optimize);
        for (auto it = std::sregex_iterator(text.begin(), text.end(), re_az);
             it != std::sregex_iterator(); ++it) {
            results.push_back({it->str(), "LAW",
                               static_cast<std::size_t>(it->position())});
        }
    }

    return results;
}

/**
 * @brief Parse Ner Json.
 * @param[in] json_str Input parameter.
 * @return Return value.
 * @details Calls: json::parse(), is_array(), value(), empty(), push_back(), std::move().
 */
std::vector<RegexMatch> parseNerJson(const std::string& json_str) {
    std::vector<RegexMatch> out;
    try {
        auto arr = json::parse(json_str);
        if (!arr.is_array()) {
          return out;
        }
        for (const auto& item : arr) {
            RegexMatch m;
            m.text   = item.value("text",   "");
            m.etype  = item.value("type",   "UNKNOWN");
            m.offset = static_cast<std::size_t>(item.value("offset", 0));
            if (!m.text.empty()) {
              out.push_back(std::move(m));
            }
        }
    } catch (...) {}
    return out;
}

/**
 * @brief Build Ner Prompt.
 * @param[in] text Input parameter.
 * @param[in] language Input parameter.
 * @param[in] types Input parameter.
 * @return Return value.
 * @details Calls: size(), resize(), substr().
 */
std::string buildNerPrompt(const std::string& text,
                            const std::string& language,
                            const std::vector<std::string>& types) {
    std::string type_list = {};
    for (const auto& t : types) {
      type_list += t + ", ";
    }
    if (type_list.size() > 2) {
      type_list.resize(type_list.size() - 2);
    }

    return "Extract named entities from the following " + language
           + " text. Return a JSON array with objects containing "
           + "'text', 'type' (one of: " + type_list
           + "), and 'offset' (character position).\n"
           + "Text:\n" + text.substr(0, 2000)
           + "\n\nJSON output only:";
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// NerDeStep — builtin.ner_de
// ─────────────────────────────────────────────────────────────────────────────

class NerDeStep : public IIngestionStep {
public:
    explicit NerDeStep(std::shared_ptr<ITextGenerationBackend> backend = nullptr)
        : backend_(backend
                   ? std::move(backend)
                   : std::make_shared<NullTextGenerationBackend>()) {}

    // IThemisPlugin
    const char* getName()    const override { return "builtin.ner_de"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    /**
     * @brief Inject backend after construction (used by WorkflowEngine / IngestionManager)
     * @param[in] b Input parameter.
     * @details Calls: std::move().
     */
    void setBackend(std::shared_ptr<ITextGenerationBackend> b) {
        backend_ = std::move(b);
    }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        if (!ctx.hasText() && !ctx.hasChunks()) return {};

        const json& config_json = cfg.config.is_object() ? cfg.config : json::object();

        // Config
        std::vector<std::string> requested_types = {};

        if (config_json.contains("entity_types") && config_json["entity_types"].is_array()) {
            for (const auto& t : config_json["entity_types"])
                requested_types.push_back(t.get<std::string>());
        }
        const bool use_llm    = config_json.value("use_llm", false);
        const std::string lang = config_json.value("language", std::string("de"));
        const double conf_min  = config_json.value("confidence", 0.8);

        // Determine texts to process
        std::vector<std::pair<std::string, std::string>> texts_with_refs;
        if (!ctx.chunks.empty()) {
            for (const auto& c : ctx.chunks)
                texts_with_refs.emplace_back(c.text, c.section_ref);
        } else {
            texts_with_refs.emplace_back(ctx.raw_text, "");
        }

        const std::size_t prior_count = ctx.entities.size();

        for (const auto& [text, section_ref] : texts_with_refs) {
            // --- regex NER (always) ---
            auto regex_hits = runGermanRegexNer(text, requested_types);
            for (auto& hit : regex_hits) {
                appendEntity(ctx, hit.text, hit.etype, section_ref,
                             static_cast<double>(conf_min), "regex");
            }

            // --- LLM NER (when available and requested) ---
            if (use_llm && backend_ && backend_->isAvailable()) {
                const std::string prompt =
                    buildNerPrompt(text, lang, requested_types);
                const std::string response =
                    backend_->generate(prompt, 512, 0.1);
                if (!response.empty()) {
                    auto llm_hits = parseNerJson(response);
                    for (auto& hit : llm_hits) {
                        appendEntity(ctx, hit.text, hit.etype, section_ref,
                                     conf_min, "llm");
                    }
                }
            }
        }

        // Record language in context if detected from config
        if (ctx.text_language.empty() && !lang.empty())
            ctx.text_language = lang;

        const std::size_t added = ctx.entities.size() - prior_count;
        if (added == 0) {
            ctx.warnings.push_back(
                "builtin.ner_de: no entities extracted from text");
        }
        return {};
    }

private:
    std::shared_ptr<ITextGenerationBackend> backend_;

    void appendEntity(ExtractionContext& ctx,
                      const std::string& text,
                      const std::string& etype_str,
                      const std::string& section_ref,
                      double confidence,
                      const std::string& source_tag) const {
        if (text.empty()) {
          return;
        }

        BaseEntity ent;
        ent.entity_type    = entityTypeFromString(etype_str);
        ent.source_file_id = ctx.manifest.file_id;
        ent.text           = text;
        ent.id             = "ner:" + ctx.manifest.file_id + ":"
                             + std::to_string(std::hash<std::string>{}(text + etype_str));
        ent.properties["ner_type"]    = etype_str;
        ent.properties["section_ref"] = section_ref;
        ent.properties["ner_source"]  = source_tag;
        ent.provenance.step_name   = "ner";
        ent.provenance.plugin_name = "builtin.ner_de";
        ent.provenance.confidence  = confidence;
        ctx.entities.push_back(std::move(ent));
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

/**
 * @brief Create Ner De Step.
 * @param[in] backend Input parameter.
 * @return Return value.
 * @details Calls: std::move().
 */
std::shared_ptr<IIngestionStep> createNerDeStep(
        std::shared_ptr<ITextGenerationBackend> backend) {
    return std::make_shared<NerDeStep>(std::move(backend));
}

/**
 * @brief Set Step Backend.
 * @param[in,out] step Input/output parameter.
 * @param[in] backend Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: setBackend(), std::move().
 */
bool setStepBackend(IIngestionStep* step,
                    std::shared_ptr<ITextGenerationBackend> backend) {
    if (auto* p = dynamic_cast<NerDeStep*>(step)) {
        p->setBackend(std::move(backend));
        return true;
    }
    return false;
}

} // namespace builtin
} // namespace ingestion
} // namespace themis

