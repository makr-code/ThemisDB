/**
 * @file content_manager_embedding.cpp
 * @brief Core content management system orchestrating processors, validators, and storage.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 76/100
 * @note Gap Summary: total=12; TODO=2, Stub=1, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=3, M=6, L=0
 * @note Status: Beta; Embedding pipeline in progress; model loading and caching under test
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_manager.h"

namespace themis {
namespace content {

void ContentManager::setEmbeddingPipeline(std::shared_ptr<EmbeddingPipeline> pipeline) {
    embedding_pipeline_ = std::move(pipeline);
}

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
