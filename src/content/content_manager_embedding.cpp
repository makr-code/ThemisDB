/*
 * ThemisDB | File: content_manager_embedding.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 54
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4241 feat(content): Embedding Ge... (2026-03-15) | #3619 fix(content): build system ... (2026-03-12) | #3167 fix(content): add content_m... (2026-03-12) | #2812 feat(content): implement em... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file content_manager_embedding.cpp
 * @brief ContentManager embedding pipeline integration (Issue #1697)
 *
 * Implements:
 *  - ContentManager::setEmbeddingPipeline() — attaches an EmbeddingPipeline
 *  - ContentManager::generateEmbedding()    — per-text embedding via pipeline
 *
 * The pipeline is invoked automatically inside importContent() for every text
 * chunk that does not yet carry a pre-computed embedding, when a pipeline with
 * a non-empty model_name is attached.
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
