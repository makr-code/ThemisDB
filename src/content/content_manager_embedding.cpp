/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            content_manager_embedding.cpp                      ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:40:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • efa41f4324  2026-02-24  feat(content): implement embedding generation pipeline (I... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
