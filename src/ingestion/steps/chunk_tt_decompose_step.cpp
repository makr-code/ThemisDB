/**
 * @file chunk_tt_decompose_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/inference_backend.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/** @brief Chunk tt decompose step. */
class ChunkTtDecomposeStep final : public IIngestionStep {
public:
    explicit ChunkTtDecomposeStep(
        std::shared_ptr<ITensorDecompositionBackend> backend)
        : backend_(std::move(backend)) {}

    // ── IThemisPlugin boilerplate ──────────────────────────────────────────
    const char* getName()    const override { return "builtin.chunk_tt_decompose"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(cons[[maybe_unused]] t cha[[maybe_unused]] r*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& [[maybe_unused]] ctx, cons[[maybe_unused]] t StepConfig& [[maybe_unused]] cfg) override {
        // ── Config ─────────────────────────────────────────────────────────
        bool        skip_when_unavailable = true;
        double      epsilon   = 0.01;
        std::size_t max_rank  = 0;
        double      min_kappa = 1.3;

        if (cfg.config.contains("skip_when_unavailable") &&
            cfg.config["skip_when_unavailable"].is_boolean()) {
            skip_when_unavailable =
                cfg.config["skip_when_unavailable"].get<bool>();
        }
        if (cfg.config.contains("epsilon") &&
            cfg.config["epsilon"].is_number()) {
            epsilon = cfg.config["epsilon"].get<double>();
        }
        if (cfg.config.contains("max_rank") &&
            cfg.config["max_rank"].is_number_unsigned()) {
            max_rank = cfg.config["max_rank"].get<std::size_t>();
        }
        if (cfg.config.contains("min_kappa") &&
            cfg.config["min_kappa"].is_number()) {
            min_kappa = cfg.config["min_kappa"].get<double>();
        }

        // ── Resolve backend ────────────────────────────────────────────────
        ITensorDecompositionBackend* backend = backend_.get();
        std::shared_ptr<NullTensorDecompositionBackend> fallback;

        if (!backend) {
            fallback = std::make_shared<NullTensorDecompositionBackend>();
            backend  = fallback.get();
        }

        // ── Availability check ─────────────────────────────────────────────
        if (!backend->isAvailable()) {
            if (skip_when_unavailable) {
                ctx.warnings.push_back(
                    "chunk_tt_decompose: backend '" +
                    backend->description() +
                    "' unavailable — step skipped (skip_when_unavailable=true)");
                return {};
            }
            return ErrVoid(
                errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                "chunk_tt_decompose: tensor decomposition backend '" +
                backend->description() +
                "' is not available and skip_when_unavailable=false");
        }

        // ── Nothing to decompose ───────────────────────────────────────────
        if (ctx.embeddings.empty()) {
            ctx.warnings.push_back(
                "chunk_tt_decompose: ctx.embeddings is empty — "
                "run builtin.chunk_embed before this step");
            return {};
        }

        // ── Decompose each embedding that passes the κ-gate ────────────────
        const std::string& file_id = ctx.manifest.file_id;
        int skipped_kappa = 0;

        for (const auto& vec : ctx.embeddings) {
            if (vec.embedding.empty()) {
                ctx.warnings.push_back(
                    "chunk_tt_decompose: skipping chunk_id='" +
                    vec.chunk_id + "' — embedding is empty");
                continue;
            }

            // κ-gate: skip data that won't compress well
            if (!backend->shouldDecompose(vec.embedding, min_kappa)) {
                ++skipped_kappa;
                continue;
            }

            TensorCoreRecord rec = backend->decompose(
                vec.embedding,
                vec.chunk_id,
                file_id,
                epsilon,
                max_rank);

            // Propagate provenance metadata from the original VectorRecord
            rec.metadata["source_file"] = ctx.manifest.original_path;
            // Copy any section/page hints from the VectorRecord
            auto it_sec = vec.metadata.find("section_ref");
            if (it_sec != vec.metadata.end()) {
                rec.metadata["section_ref"] = it_sec->second;
            }
            auto it_pg = vec.metadata.find("page");
            if (it_pg != vec.metadata.end()) {
                rec.metadata["page"] = it_pg->second;
            }

            if (rec.serialized_train.empty()) {
                ctx.warnings.push_back(
                    "chunk_tt_decompose: backend returned empty train for "
                    "chunk_id='" + vec.chunk_id + "' — skipped");
                continue;
            }

            ctx.tensor_cores.push_back(std::move(rec));
        }

        if (skipped_kappa > 0) {
            ctx.warnings.push_back(
                "chunk_tt_decompose: " + std::to_string(skipped_kappa) +
                " chunk(s) skipped — compressibility below κ=" +
                std::to_string(min_kappa));
        }

        return {};
    }

private:
    std::shared_ptr<ITensorDecompositionBackend> backend_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IIngestionStep> createChunkTtDecomposeStep(
    std::shared_ptr<ITensorDecompositionBackend> backend)
{
    return std::make_shared<ChunkTtDecomposeStep>(std::move(backend));
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
