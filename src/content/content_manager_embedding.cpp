/** @file content_manager_embedding.cpp @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata. @author makr-code @version 0.0.1 @date 2026-09-18 21:43:08 @note Maturity: 🟢 PRODUCTION-READY @note Score: 100/100 @note Lines: 42 @note Module Context: src/content @note Ownership Scope: production-code @note Primary Symbols: none-detected @note PR History (last 5): #5950 [CP-1] Content Module Batch... (2026-08-15) | #4241 feat(content): Embedding Ge... (2026-03-15) | #3619 fix(content): build system ... (2026-03-12) | #3167 fix(content): add content_m... (2026-03-12) | #2812 feat(content): implement em... (2026-03-12) @note Governance: BranchModel=develop-first; CanonicalBranches=develop,community,military @note Release Context: GateModel=WaveA→B→C→D on develop @note Status: Production Ready @note Generator: .github/scripts/code_maturity_header_writer.py @note This block is auto-generated and will be overwritten. */

#include "content/content_manager.h"

namespace themis {
namespace content {

/**
 * @brief Set Embedding Pipeline.
 * @param[in] pipeline Input parameter.
 * @details Calls: std::move().
 */
void ContentManager::setEmbeddingPipeline(std::shared_ptr<EmbeddingPipeline> pipeline) {
    embedding_pipeline_ = std::move(pipeline);
}

/**
 * @brief Generate Embedding.
 * @param[in] text Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 */
std::vector<float> ContentManager::generateEmbedding(
    const std::string& text,
    const std::string& /*model_name*/)
{
    // Prefer the attached pipeline when present and enabled.
    if (embedding_pipeline_ && embedding_pipeline_->isEnabled()) {
        return embedding_pipeline_->generateEmbedding(text);
    }

    // Fallback: delegate to the TEXT processor if registered.
    auto it = processors_.find(ContentCategory::TEXT);
    if (it != processors_.end() && it->second) {
        return it->second->generateEmbedding(text);
    }

    return {};
}

} // namespace content
} // namespace themis
