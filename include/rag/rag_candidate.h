#pragma once

#include <string>
#include <vector>
#include <optional>

namespace themis::rag {

/**
 * @brief Identifies the authoritative source represented by indexed evidence.
 * @note Keys are exact source keys; consumers must not infer them from IDs or prefixes.
 */
struct RAGSourceReference {
    std::string type;                    ///< Source kind, e.g. "content_chunk".
    std::string collection;              ///< Authoritative source collection.
    std::string key;                     ///< Exact source key; never inferred.
    std::string provenance;              ///< Ingestion or source provenance.
    std::optional<std::string> version;   ///< Existing source version/freshness value.
};

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
    std::optional<RAGSourceReference> source_reference;
};

} // namespace themis::rag
