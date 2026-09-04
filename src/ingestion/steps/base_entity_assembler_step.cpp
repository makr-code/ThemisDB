/**
 * @file base_entity_assembler_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_step.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief `builtin.base_entity_assembler` — deduplicates and finalises entities.
 *
 * This step runs last in the pipeline.  It deduplicates `ctx.entities` by
 * `BaseEntity::id` (keeping the version with the highest confidence), then
 * ensures every entity carries the `source_file_id` from the manifest.
 *
 * Config keys (all optional):
 *  - `dedup_strategy`  string  "canonical_id" (default) | "none"
 *  - `graph_relations` array   list of relation type strings to retain
 *                              (empty = retain all)
 */
class BaseEntityAssemblerStep : public IIngestionStep {
public:
    const char* getName()    const override {
        return "builtin.base_entity_assembler";
    }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& cfg) override {
        const std::string dedup =
            cfg.config.value("dedup_strategy", std::string("canonical_id"));

        // Ensure all entities have source_file_id
        for (auto& ent : ctx.entities) {
            if (ent.source_file_id.empty())
                ent.source_file_id = ctx.manifest.file_id;
        }

        if (dedup == "canonical_id") {
            dedupById(ctx);
        }
        // else: no dedup, leave as-is

        return {};
    }

private:
    static void dedupById(ExtractionContext& ctx) {
        std::unordered_map<std::string, std::size_t> id_to_idx;
        std::vector<BaseEntity> deduped = {};

        deduped.reserve(ctx.entities.size());

        for (auto& ent : ctx.entities) {
            auto it = id_to_idx.find(ent.id);
            if (it == id_to_idx.end()) {
                id_to_idx[ent.id] = deduped.size();
                deduped.push_back(std::move(ent));
            } else {
                // Keep the version with higher confidence
                if (ent.provenance.confidence
                    > deduped[it->second].provenance.confidence) {
                    deduped[it->second] = std::move(ent);
                }
            }
        }
        ctx.entities = std::move(deduped);
    }
};

} // namespace builtin
} // namespace ingestion
} // namespace themis
