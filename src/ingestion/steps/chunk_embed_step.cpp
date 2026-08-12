/**
 * @file chunk_embed_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.4.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/inference_backend.h"
#include "ingestion/base_entity.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/** @brief Chunk embed step. */
class ChunkEmbedStep final : public IIngestionStep {
public:
    explicit ChunkEmbedStep(std::shared_ptr<IEmbeddingBackend> backend)
        : backend_(std::move(backend)) {}

    // IThemisPlugin boilerplate
    const char* getName()    const override { return "builtin.chunk_embed"; }
    const char* getVersion() const override { return "1.4.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        // ── Config ────────────────────────────────────────────────────────
        bool skip_when_unavailable = true;
        if (cfg.config.contains("skip_when_unavailable") &&
            cfg.config["skip_when_unavailable"].is_boolean()) {
            skip_when_unavailable = cfg.config["skip_when_unavailable"].get<bool>();
        }

        // ── Ensure we have a backend ───────────────────────────────────────
        IEmbeddingBackend* backend = backend_.get();
        std::shared_ptr<NullEmbeddingBackend> fallback;

        if (!backend) {
            int dims = 768;
            if (cfg.config.contains("dims") && cfg.config["dims"].is_number_integer()) {
                dims = cfg.config["dims"].get<int>();
            }
            fallback  = std::make_shared<NullEmbeddingBackend>(dims);
            backend   = fallback.get();
        }

        // ── Check availability ─────────────────────────────────────────────
        if (!backend->isAvailable()) {
            if (skip_when_unavailable) {
                ctx.warnings.push_back(
                    "chunk_embed: backend '" + backend->description() +
                    "' unavailable — step skipped (skip_when_unavailable=true)");
                return {};
            }
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                "chunk_embed: embedding backend '" + backend->description() +
                "' is not available and skip_when_unavailable=false");
        }

        // ── No chunks → nothing to embed ──────────────────────────────────
        if (ctx.chunks.empty()) {
            ctx.warnings.push_back(
                "chunk_embed: ctx.chunks is empty — no embeddings produced");
            return {};
        }

        // ── Embed each chunk ───────────────────────────────────────────────
        const std::string& file_id = ctx.manifest.file_id;

        for (const auto& chunk : ctx.chunks) {
            std::vector<float> vec = backend->embed(chunk.text);

            VectorRecord rec;
            rec.chunk_id       = file_id + ":" + std::to_string(chunk.seq);
            rec.source_file_id = file_id;
            rec.text_snippet   = chunk.text;
            rec.embedding      = std::move(vec);

            if (!chunk.section_ref.empty()) {
                rec.metadata["section_ref"] = chunk.section_ref;
            }
            if (!chunk.page_ref.empty()) {
                rec.metadata["page"] = chunk.page_ref;
            }

            ctx.embeddings.push_back(std::move(rec));
        }

        return {};
    }

private:
    std::shared_ptr<IEmbeddingBackend> backend_; ///< Injected backend; nullptr → NullEmbeddingBackend
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IIngestionStep> createChunkEmbedStep(
    std::shared_ptr<IEmbeddingBackend> backend)
{
    return std::make_shared<ChunkEmbedStep>(std::move(backend));
}

} // namespace builtin
} // namespace ingestion
} // namespace themis

