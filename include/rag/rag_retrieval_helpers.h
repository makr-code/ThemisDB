#pragma once

#include "rag/rag_candidate.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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
    const auto source_type = entity.find("source_ref_type");
    const auto source_collection = entity.find("source_ref_collection");
    const auto source_key = entity.find("source_ref_key");
    if (source_type != entity.end() && source_type->is_string() &&
        source_collection != entity.end() && source_collection->is_string() &&
        source_key != entity.end() && source_key->is_string() &&
        !source_type->get_ref<const std::string&>().empty() &&
        !source_collection->get_ref<const std::string&>().empty() &&
        !source_key->get_ref<const std::string&>().empty()) {
        RAGSourceReference reference;
        reference.type = source_type->get<std::string>();
        reference.collection = source_collection->get<std::string>();
        reference.key = source_key->get<std::string>();
        const auto source_provenance = entity.find("source_ref_provenance");
        if (source_provenance != entity.end() && source_provenance->is_string()) {
            reference.provenance = source_provenance->get<std::string>();
        }
        const auto source_version = entity.find("source_ref_version");
        if (source_version != entity.end()) {
            if (source_version->is_string()) {
                reference.version = source_version->get<std::string>();
            } else if (source_version->is_number_integer()) {
                reference.version = std::to_string(source_version->get<std::int64_t>());
            }
        }
        candidate.source_reference = std::move(reference);
    }

    for (const auto* field : {"content", "text", "body", "text_snippet"}) {
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
    for (auto it = fields.begin(); it != fields.end();) {
        const auto& field = it.key();
        const bool identity_field =
            field == "_key" || field == "_id" || field == "id" || field == "source" ||
            field == "chunk_id" || field == "source_file_id" || field == "content_id" ||
            field == "seq_num" || field == "content" || field == "text" || field == "body" ||
            field == "text_snippet" || field == "source_ref_type" ||
            field == "source_ref_collection" || field == "source_ref_key" ||
            field == "source_ref_provenance" || field == "source_ref_version";
        const bool metadata_or_vector_field =
            field == "metadata" || field.starts_with("metadata.") ||
            field.starts_with("embedding") || field.starts_with("vector");
        if (identity_field || metadata_or_vector_field) {
            it = fields.erase(it);
        } else {
            ++it;
        }
    }
    if (fields.empty()) {
        return std::nullopt;
    }
    candidate.content = fields.dump();
    return candidate;
}

} // namespace themis::rag
