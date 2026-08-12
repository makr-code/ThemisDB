/**
 * @file legal_reference_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.4.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/agentic_reference_validator.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/** @brief Legal reference extractor step. */
class LegalReferenceExtractorStep final : public IIngestionStep {
public:
    // IThemisPlugin boilerplate
    const char* getName()    const override { return "builtin.legal_reference_extractor"; }
    const char* getVersion() const override { return "1.4.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        if (ctx.raw_text.empty()) {
            ctx.warnings.push_back(
                "legal_reference_extractor: ctx.raw_text is empty — skipping");
            ctx.extra["legal_refs.extracted_count"] = "0";
            ctx.extra["legal_refs.dangling_count"]  = "0";
            ctx.extra["legal_refs.warnings_json"]   = "[]";
            return {};
        }

        // ── Configure validator ────────────────────────────────────────────
        AgenticReferenceValidator validator;

        if (cfg.config.contains("known_laws") && cfg.config["known_laws"].is_array()) {
            for (const auto& law : cfg.config["known_laws"]) {
                if (law.is_string()) {
                    validator.addKnownLaw(law.get<std::string>());
                }
            }
        }

        // ── Run extraction + validation ────────────────────────────────────
        ReferenceValidationReport report = validator.validate(ctx.raw_text);

        // ── Store results in ctx.extra ─────────────────────────────────────
        ctx.extra["legal_refs.extracted_count"] =
            std::to_string(report.extracted.size());
        ctx.extra["legal_refs.dangling_count"] =
            std::to_string(report.dangling_count);

        // Serialize validation warnings as a JSON array string
        json warnings_arr = json::array();
        for (const auto& w : report.warnings) {
            warnings_arr.push_back(w);
        }
        ctx.extra["legal_refs.warnings_json"] = warnings_arr.dump();

        // ── Propagate dangling refs to ctx.warnings ────────────────────────
        for (const auto& vr : report.validated) {
            if (!vr.found) {
                ctx.warnings.push_back(
                    "legal_reference_extractor: dangling reference — law='" +
                    vr.reference.law_id + "' section='" +
                    vr.reference.section + "' [confidence=" +
                    std::to_string(vr.confidence) + "]");
            }
        }

        return {};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IIngestionStep> createLegalReferenceExtractorStep() {
    return std::make_shared<LegalReferenceExtractorStep>();
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
