#pragma once

#include <string>
#include <vector>

namespace themis::rag {

/**
 * @brief Model-independent evidence shared by retrieval and optional RAG stages.
 * @note Scores are higher-is-better retrieval/rerank values, not probabilities.
 * Identity is the pair (source_namespace, doc_id). This value owns its data.
 */
struct RAGCandidate {
    std::string doc_id;
    std::string content;
    float score = 0.0f;
    std::string source_namespace;
    std::vector<std::string> provenance_tags;
};

} // namespace themis::rag
