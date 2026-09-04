/**
 * @file legal_metadata_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_step.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <regex>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief `builtin.legal_metadata` — regex-based legal metadata extraction.
 *
 * Extracts norm identifiers (§-numbers, Article numbers), dates, and
 * Aktenzeichen from `ctx.raw_text` and adds them as `BaseEntity` objects
 * of appropriate legal types.
 *
 * Config keys (all optional; defaults are German legal patterns):
 *  - `norm_pattern`        regex pattern for §/Art. references
 *  - `date_pattern`        regex for German date format dd.mm.yyyy
 *  - `aktenzeichen_pattern` regex for file/ref number
 */
class LegalMetadataStep : public IIngestionStep {
public:
    const char* getName()    const override { return "builtin.legal_metadata"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(cons[[maybe_unused]] t cha[[maybe_unused]] r*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& cfg) override {
        if (ctx.raw_text.empty()) return {};

        const std::string norm_pat =
            cfg.config.value("norm_pattern",
                             std::string(R"(§\s*\d+[\w\s]*)"));
        const std::string date_pat =
            cfg.config.value("date_pattern",
                             std::string(R"(\d{1,2}\.\d{1,2}\.\d{4})"));
        const std::string az_pat =
            cfg.config.value("aktenzeichen_pattern",
                             std::string(R"([A-Z]\s?\d+/\d{2,4})"));

        try {
            extractByPattern(ctx, norm_pat, EntityType::LEGAL_PROVISION,
                              "section_ref", "norm");
            extractByPattern(ctx, date_pat, EntityType::DATE,
                              "date_value", "date");
            extractByPattern(ctx, az_pat, EntityType::LEGAL_AKTENZEICHEN,
                              "aktenzeichen", "az");
        } catch (const std::regex_error& e) {
            return tl::make_unexpected(
            Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      std::string("legal_metadata: regex error: ") + e.what()});
        }
        return {};
    }

private:
    static void extractByPattern(ExtractionContext& ctx,
                                  const std::string& pattern,
                                  EntityType entity_type,
                                  const std::string& prop_key,
                                  const std::string& id_prefix) {
        const std::regex re(pattern, std::regex::ECMAScript);
        const std::string& text = ctx.raw_text;
        auto it = std::sregex_iterator(text.begin(), text.end(), re);
        const auto end = std::sregex_iterator();
        std::size_t seq = 0;
        for (; it != end; ++it, ++seq) {
            const std::string match = it->str();
            BaseEntity ent;
            ent.id          = id_prefix + ":" + ctx.manifest.file_id
                              + ":" + std::to_string(seq);
            ent.entity_type = entity_type;
            ent.source_file_id = ctx.manifest.file_id;
            ent.text        = match;
            ent.properties[prop_key] = match;
            ent.provenance.step_name    = "legal_metadata";
            ent.provenance.plugin_name  = "builtin.legal_metadata";
            ent.provenance.confidence   = 0.9;
            ctx.entities.push_back(std::move(ent));
        }
    }
};

} // namespace builtin
} // namespace ingestion
} // namespace themis
