/**
 * @file deontic_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_step.h"
#include "ingestion/deontic_extractor.h"
#include "ingestion/inference_backend.h"
#include "ingestion/llm_adapter.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief `builtin.deontic_extractor` — wraps `DeonticExtractor`.
 *
 * Iterates over `ctx.chunks` (or falls back to `ctx.raw_text` split into a
 * single chunk) and runs deontic extraction on each chunk.  Results are
 * appended to `ctx.entities` as `LEGAL_OBLIGATION`, `LEGAL_PROHIBITION`, or
 * `LEGAL_PERMISSION` entities.
 *
 * Config keys (all optional):
 *  - `confidence_threshold` float  default 0.5
 *  - `use_llm`              bool   default false
 */
class DeonticStep : public IIngestionStep {
public:
    explicit DeonticStep(
        std::shared_ptr<ITextGenerationBackend> backend = nullptr)
        : backend_(backend
                   ? std::move(backend)
                   : std::make_shared<NullTextGenerationBackend>()) {}

    // IThemisPlugin
    const char* getName()    const override { return "builtin.deontic_extractor"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(cons[[maybe_unused]] t cha[[maybe_unused]] r*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    // Inject backend after construction (used by WorkflowEngine / IngestionManager)
    void setBackend(std::shared_ptr<ITextGenerationBackend> b) {
        backend_ = std::move(b);
    }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& cfg) override {
        if (ctx.raw_text.empty() && ctx.chunks.empty()) return {};

        const double threshold =
            cfg.config.value("confidence_threshold", 0.5);
        const bool use_llm = cfg.config.value("use_llm", false);

        DeonticExtractor extractor;

        // Wire LLM backend when requested and available
        if (use_llm && backend_ && backend_->isAvailable()) {
            LegalLlmAdapter adapter(backend_);
            const auto fn = adapter.buildExtractorFn();
            if (fn) extractor.setExtractorFn(fn);
        }

        auto process = [&](const std::string& text,
                           const std::string& section_ref) {
            const auto result = extractor.extract(text);
            if (result.overall_confidence < threshold) return;

            // Determine primary entity type from deontic categories
            EntityType et = EntityType::LEGAL_OBLIGATION;
            if (!result.deontic_categories.empty()) {
                const auto primary = result.primaryCategory();
                if (primary == DeonticCategory::PROHIBITION)
                    et = EntityType::LEGAL_PROHIBITION;
                else if (primary == DeonticCategory::PERMISSION)
                    et = EntityType::LEGAL_PERMISSION;
            }

            BaseEntity ent;
            ent.entity_type = et;
            ent.id = "deontic:" + ctx.manifest.file_id + ":"
                     + std::to_string(ctx.entities.size());
            ent.source_file_id = ctx.manifest.file_id;
            ent.text = text.substr(0, std::min(text.size(), std::size_t{256}));
            ent.properties["deontic_category"] =
                result.deontic_categories.empty()
                    ? "OBLIGATION"
                    : deonticCategoryToString(result.primaryCategory());
            ent.properties["section_ref"] = section_ref;
            ent.provenance.step_name   = "deontic_extract";
            ent.provenance.plugin_name = "builtin.deontic_extractor";
            ent.provenance.confidence  = result.overall_confidence;
            ctx.entities.push_back(std::move(ent));
        };

        if (!ctx.chunks.empty()) {
            for (const auto& chunk : ctx.chunks) {
                process(chunk.text, chunk.section_ref);
            }
        } else {
            process(ctx.raw_text, "");
        }
        return {};
    }

private:
    std::shared_ptr<ITextGenerationBackend> backend_;
};

} // namespace builtin
} // namespace ingestion
} // namespace themis
