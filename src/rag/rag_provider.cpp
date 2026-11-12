#include "rag/rag_provider.h"
#include "rag/rag_retrieval_helpers.h"
#include "storage/base_entity.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

namespace themis::rag {
namespace {

Result<void> checkExecution(const RetrievalExecutionContext& context) {
    if (context.cancelled && context.cancelled->load(std::memory_order_acquire)) {
        return ErrVoid(errors::ErrorCode::ERR_QUERY_CANCELLED, "RAG retrieval cancelled");
    }
    if (std::chrono::steady_clock::now() >= context.deadline) {
        return ErrVoid(errors::ErrorCode::ERR_QUERY_TIMEOUT, "RAG retrieval deadline exceeded");
    }
    return OkVoid();
}

Result<void> checkCollectionAccess(
    const RetrievalExecutionContext& context, const std::string& collection) {
    if (!context.authorize_collection) {
        return OkVoid();
    }
    try {
        if (context.authorize_collection(collection)) {
            return OkVoid();
        }
    } catch (...) {
    }
    return ErrVoid(errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
                   "RAG collection access denied");
}

} // namespace

Result<RetrievalResult> RAGProvider::retrieve(
    const RetrievalRequest& request, const RetrievalExecutionContext& context) const {
    if (request.max_candidates == 0 || request.max_candidates > 1000 ||
        request.max_content_bytes == 0 || request.max_content_bytes > 1024 * 1024) {
        return Err<RetrievalResult>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                                   "Invalid RAG candidate or content-byte budget");
    }
    auto execution = checkExecution(context);
    if (!execution) {
        return tl::unexpected(execution.error());
    }

    RetrievalResult result;
    std::size_t content_bytes = 0;
    std::set<std::pair<std::string, std::string>> seen;
    auto append = [&](const std::string& id, const std::string& collection,
                      const nlohmann::json& entity, float score,
                      const std::string& provenance) -> Result<void> {
        auto active = checkExecution(context);
        if (!active) {
            return active;
        }
        auto authorized = checkCollectionAccess(context, collection);
        if (!authorized) {
            return authorized;
        }
        nlohmann::json materialized_entity;
        const auto* candidate_entity = &entity;
        const auto source_type = entity.find("source_ref_type");
        if (source_type != entity.end() && source_type->is_string() &&
            source_type->get_ref<const std::string&>() == "content_chunk") {
            const auto source_collection = entity.find("source_ref_collection");
            const auto source_key = entity.find("source_ref_key");
            const auto source_version = entity.find("source_ref_version");
            const auto content_id = entity.find("content_id");
            if (source_collection == entity.end() || !source_collection->is_string() ||
                source_collection->get_ref<const std::string&>() != collection ||
                source_key == entity.end() || !source_key->is_string() ||
                source_key->get_ref<const std::string&>().empty() ||
                content_id == entity.end() || !content_id->is_string() ||
                content_id->get_ref<const std::string&>().empty() ||
                (source_version != entity.end() &&
                 !source_version->is_string() && !source_version->is_number_integer())) {
                ++result.rejected_documents;
                return OkVoid();
            }
            std::optional<std::string> expected_version;
            if (source_version != entity.end()) {
                if (source_version->is_string()) {
                    expected_version = source_version->get<std::string>();
                } else {
                    try {
                        expected_version = std::to_string(
                            source_version->get<std::int64_t>());
                    } catch (const nlohmann::json::exception&) {
                        ++result.rejected_documents;
                        return OkVoid();
                    }
                }
            }
            auto source_text = engine_.readContentChunkText(
                source_key->get_ref<const std::string&>(),
                content_id->get_ref<const std::string&>());
            if (!source_text) {
                return tl::unexpected(source_text.error());
            }
            if (!*source_text) {
                ++result.rejected_documents;
                return OkVoid();
            }
            if (expected_version &&
                *expected_version !=
                    std::to_string((*source_text)->second)) {
                ++result.rejected_documents;
                return OkVoid();
            }
            materialized_entity = entity;
            materialized_entity["content"] = std::move((*source_text)->first);
            candidate_entity = &materialized_entity;
        }
        auto candidate = makeRAGCandidate(
            id, collection, *candidate_entity, score, provenance);
        if (!candidate) {
            ++result.rejected_documents;
            return OkVoid();
        }
        if (!seen.emplace(collection, id).second) {
            return OkVoid();
        }
        if (candidate->content.size() > request.max_content_bytes - content_bytes) {
            return ErrVoid(errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
                           "RAG evidence exceeds the content-byte budget");
        }
        content_bytes += candidate->content.size();
        result.candidates.push_back(std::move(*candidate));
        return OkVoid();
    };

    auto retrieved = std::visit([&](const auto& specification) -> Result<void> {
        using Query = std::decay_t<decltype(specification)>;
        if constexpr (std::is_same_v<Query, query::ConjunctiveQuery>) {
            if (specification.table.empty() || specification.table.find(':') != std::string::npos) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "RAG collection is required");
            }
            if (specification.pk_eq &&
                (specification.pk_eq->empty() || specification.pk_eq->find(':') != std::string::npos)) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "Invalid RAG primary key");
            }
            if (specification.pk_eq &&
                (!specification.predicates.empty() || !specification.rangePredicates.empty() ||
                 specification.orderBy || specification.fulltextPredicate || specification.phrasePredicate ||
                 specification.fuzzyPredicate || specification.spatialPredicate)) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                               "Primary-key lookup cannot bypass additional RAG predicates");
            }
            auto authorized = checkCollectionAccess(context, specification.table);
            if (!authorized) {
                return authorized;
            }
            auto keys = engine_.executeAndKeys(specification);
            if (!keys) {
                return tl::unexpected(keys.error());
            }
            result.truncated = keys->size() > request.max_candidates;
            if (result.truncated) {
                keys->resize(request.max_candidates);
            }
            for (const auto& key : *keys) {
                auto active = checkExecution(context);
                if (!active) {
                    return active;
                }
                query::ConjunctiveQuery lookup;
                lookup.table = specification.table;
                lookup.pk_eq = key;
                auto entities = engine_.executeAndEntities(lookup);
                if (!entities) {
                    return tl::unexpected(entities.error());
                }
                if (entities->empty()) {
                    ++result.rejected_documents;
                }
                for (const auto& entity : *entities) {
                    auto added = append(entity.getPrimaryKey(), specification.table,
                                        nlohmann::json::parse(entity.toJson()), 1.0f,
                                        specification.spatialPredicate ? "retrieve:geo" : "retrieve:relational");
                    if (!added) {
                        return added;
                    }
                }
            }
        } else if constexpr (std::is_same_v<Query, query::VectorGeoQuery>) {
            if (specification.table.empty() || specification.table.find(':') != std::string::npos ||
                specification.vector_field.empty() ||
                specification.query_vector.empty() ||
                !std::all_of(specification.query_vector.begin(), specification.query_vector.end(),
                             [](float value) { return std::isfinite(value); }) ||
                std::all_of(specification.query_vector.begin(), specification.query_vector.end(),
                            [](float value) { return value == 0.0f; })) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                               "RAG vector query requires a collection, field and finite nonzero embedding");
            }
            auto authorized = checkCollectionAccess(context, specification.table);
            if (!authorized) {
                return authorized;
            }
            auto query = specification;
            query.k = request.max_candidates;
            auto matches = engine_.executeVectorGeoQuery(query);
            if (!matches) {
                return tl::unexpected(matches.error());
            }
            for (const auto& match : *matches) {
                auto added = append(match.pk, query.table, match.entity,
                                    1.0f - match.vector_distance, "retrieve:vector");
                if (!added) {
                    return added;
                }
            }
        } else if constexpr (std::is_same_v<Query, query::ContentGeoQuery>) {
            if (specification.table.empty() || specification.table.find(':') != std::string::npos ||
                specification.text_field.empty() || specification.fulltext_query.empty() ||
                !specification.spatial_filter) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                               "RAG content query requires collection, text field and search text");
            }
            auto authorized = checkCollectionAccess(context, specification.table);
            if (!authorized) {
                return authorized;
            }
            auto query = specification;
            query.limit = request.max_candidates;
            auto matches = engine_.executeContentGeoQuery(query);
            if (!matches) {
                return tl::unexpected(matches.error());
            }
            for (const auto& match : *matches) {
                auto added = append(match.pk, query.table, match.entity,
                                    static_cast<float>(match.bm25_score), "retrieve:content_geo");
                if (!added) {
                    return added;
                }
            }
        } else {
            if (specification.start_vertex.empty() || specification.start_vertex.front() == '/' ||
                specification.start_vertex.find(':') != std::string::npos || specification.min_depth < 0 ||
                specification.max_depth < specification.min_depth || specification.max_depth > 16) {
                return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                               "RAG graph query requires a start vertex and depth range within 0..16");
            }
            const auto start_slash = specification.start_vertex.find('/');
            const auto start_collection = start_slash == std::string::npos
                ? std::string{"vertices"}
                : specification.start_vertex.substr(0, start_slash);
            auto authorized = checkCollectionAccess(context, start_collection);
            if (!authorized) {
                return authorized;
            }
            auto matches = engine_.executeGeneralTraversal(
                specification.start_vertex, specification.min_depth, specification.max_depth,
                specification.direction, specification.graph_id, specification.edge_type,
                context.authorize_collection);
            if (!matches) {
                return tl::unexpected(matches.error());
            }
            result.truncated = matches->size() > request.max_candidates;
            if (result.truncated) {
                matches->resize(request.max_candidates);
            }
            for (const auto& match : *matches) {
                const auto slash = match.vertex_pk.find('/');
                const auto collection = slash == std::string::npos
                    ? std::string{"vertices"} : match.vertex_pk.substr(0, slash);
                auto added = append(match.vertex_pk, collection, match.vertex_data,
                                    1.0f / (1.0f + match.depth), "retrieve:graph");
                if (!added) {
                    return added;
                }
            }
        }
        return checkExecution(context);
    }, request.query);
    if (!retrieved) {
        return tl::unexpected(retrieved.error());
    }

    std::stable_sort(result.candidates.begin(), result.candidates.end(),
        [](const RAGCandidate& left, const RAGCandidate& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            return std::tie(left.source_namespace, left.doc_id) <
                   std::tie(right.source_namespace, right.doc_id);
        });
    execution = checkExecution(context);
    if (!execution) {
        return tl::unexpected(execution.error());
    }
    return Ok(std::move(result));
}

} // namespace themis::rag
