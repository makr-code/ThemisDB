#pragma once

#include "rag/rag_candidate.h"

#include <cmath>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis::rag {

/**
 * @brief Convert an already materialized entity to evidence without performing I/O.
 * @param id Stable entity key; empty identifiers are rejected.
 * @param collection Source collection used with the key as evidence identity.
 * @param entity Materialized object; content/text/body is preferred, otherwise real
 * structured fields are serialized. Empty or identifier-only objects are rejected.
 * @param score Finite, higher-is-better score; not necessarily normalized.
 * @param provenance Retrieval method annotation.
 * @return Owned evidence, or nullopt for unusable materialized input.
 * @note Pure transformation; no storage access, logging, model calls, or retries.
 */
[[nodiscard]] inline std::optional<RAGCandidate> makeRAGCandidate(
    const std::string& id, const std::string& collection,
    const nlohmann::json& entity, float score, const std::string& provenance) {
    if (id.empty() || !entity.is_object() || entity.empty() || !std::isfinite(score)) {
        return std::nullopt;
    }

    RAGCandidate candidate;
    candidate.doc_id = id;
    candidate.source_namespace = collection;
    candidate.score = score;
    candidate.provenance_tags.push_back(provenance);
    for (const auto* field : {"content", "text", "body"}) {
        const auto it = entity.find(field);
        if (it != entity.end() && it->is_string()) {
            const auto& text = it->get_ref<const std::string&>();
            if (text.find_first_not_of(" \t\r\n") != std::string::npos) {
                candidate.content = text;
                return candidate;
            }
        }
    }

    auto fields = entity;
    for (const auto* field : {"_key", "_id", "id", "source", "content", "text", "body"}) {
        fields.erase(field);
    }
    if (fields.empty()) {
        return std::nullopt;
    }
    candidate.content = entity.dump();
    return candidate;
}

} // namespace themis::rag
