/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_sinks.cpp                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-15 18:49:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     325                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/ingestion_sinks.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace ingestion {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// IGraphWriter — default write() implementation
// ─────────────────────────────────────────────────────────────────────────────

Result<void> IGraphWriter::write(const BaseEntitySet& entity_set) {
    auto r1 = writeEntities(entity_set.nodes);
    if (!r1) return r1;
    return writeRelations(entity_set.edges);
}

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryGraphWriter
// ─────────────────────────────────────────────────────────────────────────────

Result<void> InMemoryGraphWriter::writeEntities(const std::vector<BaseEntity>& nodes) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& n : nodes) {
        auto it = nodes_.find(n.id);
        if (it == nodes_.end()) {
            nodes_.emplace(n.id, n);
        } else {
            // Merge properties: new values overwrite existing
            auto& existing = it->second;
            for (const auto& [k, v] : n.properties) {
                existing.properties[k] = v;
            }
            // Update embedding if provided
            if (!n.embeddings.empty()) {
                existing.embeddings = n.embeddings;
            }
            // Update confidence if higher
            if (n.provenance.confidence > existing.provenance.confidence) {
                existing.provenance = n.provenance;
            }
        }
    }
    return {};
}

Result<void> InMemoryGraphWriter::writeRelations(const std::vector<EntityRelation>& edges) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& e : edges) {
        // Upsert: find existing edge with same (from, to, type)
        auto it = std::find_if(edges_.begin(), edges_.end(),
            [&](const EntityRelation& r) {
                return r.from_id == e.from_id
                    && r.to_id   == e.to_id
                    && r.relation_type == e.relation_type;
            });
        if (it == edges_.end()) {
            edges_.push_back(e);
        } else {
            // Merge properties
            for (const auto& [k, v] : e.properties)
                (*it).properties[k] = v;
        }
    }
    return {};
}

std::size_t InMemoryGraphWriter::nodeCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return nodes_.size();
}

std::size_t InMemoryGraphWriter::edgeCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return edges_.size();
}

void InMemoryGraphWriter::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    nodes_.clear();
    edges_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryVectorWriter
// ─────────────────────────────────────────────────────────────────────────────

Result<void> InMemoryVectorWriter::writeVectors(const std::vector<VectorRecord>& records) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& rec : records) {
        records_[rec.chunk_id] = rec;
    }
    return {};
}

std::size_t InMemoryVectorWriter::vectorCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return records_.size();
}

const VectorRecord* InMemoryVectorWriter::findByChunkId(const std::string& chunk_id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = records_.find(chunk_id);
    return it != records_.end() ? &it->second : nullptr;
}

void InMemoryVectorWriter::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    records_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocWriter
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Serialize a BaseEntitySet to a compact JSON string.
std::string serializeEntitySet(const BaseEntitySet& es) {
    json j;
    j["source_file_id"] = es.source_file_id;
    j["quality_score"]  = es.quality_score;

    json nodes_arr = json::array();
    for (const auto& n : es.nodes) {
        json nj;
        nj["id"]          = n.id;
        nj["entity_type"] = static_cast<int>(n.entity_type);
        nj["text"]        = n.text;
        nj["properties"]  = n.properties;
        nj["confidence"]  = n.provenance.confidence;
        nj["step"]        = n.provenance.step_name;
        nodes_arr.push_back(std::move(nj));
    }
    j["nodes"] = std::move(nodes_arr);

    json edges_arr = json::array();
    for (const auto& e : es.edges) {
        json ej;
        ej["from"]  = e.from_id;
        ej["to"]    = e.to_id;
        ej["type"]  = static_cast<int>(e.relation_type);
        ej["props"] = e.properties;
        edges_arr.push_back(std::move(ej));
    }
    j["edges"] = std::move(edges_arr);

    j["chunk_count"] = es.chunks.size();

    return j.dump();
}

} // anonymous namespace

Result<std::string> InMemoryDocWriter::writeDocument(const BaseEntitySet& entity_set,
                                                       const std::string& collection) {
    std::lock_guard<std::mutex> lock(mtx_);
    const std::string doc_id = collection + "/" + entity_set.source_file_id
                               + "/" + std::to_string(next_id_++);
    docs_[doc_id] = serializeEntitySet(entity_set);
    return doc_id;
}

std::size_t InMemoryDocWriter::documentCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return docs_.size();
}

std::string InMemoryDocWriter::getDocument(const std::string& doc_id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = docs_.find(doc_id);
    return it != docs_.end() ? it->second : std::string{};
}

void InMemoryDocWriter::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    docs_.clear();
    next_id_ = 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// IngestionSinkBundle
// ─────────────────────────────────────────────────────────────────────────────

Result<void> IngestionSinkBundle::writeAll(const BaseEntitySet& entity_set,
                                            const std::string& collection) const {
    if (graph) {
        auto r = graph->write(entity_set);
        if (!r) return r;
    }
    if (vector && !entity_set.chunks.empty()) {
        auto r = vector->writeVectors(entity_set.chunks);
        if (!r) return r;
    }
    if (doc) {
        auto r = doc->writeDocument(entity_set, collection);
        if (!r) return Result<void>{tl::make_unexpected(r.error())};
    }
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentStoreSinkAdapter — Phase 5: IDocumentStore wiring
// ─────────────────────────────────────────────────────────────────────────────

DocumentStoreSinkAdapter::DocumentStoreSinkAdapter(
    std::shared_ptr<themis::document::IDocumentStore> store)
    : store_(std::move(store))
{
    if (!store_) {
        throw std::invalid_argument("DocumentStoreSinkAdapter: store must not be null");
    }
}

nlohmann::json DocumentStoreSinkAdapter::serialise(const BaseEntitySet& es) {
    nlohmann::json j;
    j["source_file_id"] = es.source_file_id;
    j["quality_score"]  = es.quality_score;

    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& e : es.nodes) {
        nlohmann::json ej;
        ej["id"]             = e.id;
        ej["entity_type"]    = static_cast<int>(e.entity_type);
        ej["text"]           = e.text;
        ej["source_file_id"] = e.source_file_id;
        ej["source_text_ref"] = e.source_text_ref;
        ej["embeddings"]     = e.embeddings;
        ej["provenance"] = {
            {"step_name", e.provenance.step_name},
            {"plugin_name", e.provenance.plugin_name},
            {"confidence", e.provenance.confidence},
            {"extracted_at", e.provenance.extracted_at}
        };
        nlohmann::json props;
        for (const auto& [k, v] : e.properties) props[k] = v;
        ej["properties"] = std::move(props);
        nodes.push_back(std::move(ej));
    }
    j["nodes"] = std::move(nodes);

    nlohmann::json edges = nlohmann::json::array();
    for (const auto& r : es.edges) {
        nlohmann::json rj;
        rj["from_id"]       = r.from_id;
        rj["to_id"]         = r.to_id;
        rj["relation_type"] = static_cast<int>(r.relation_type);
        const auto weight_it = r.properties.find("weight");
        if (weight_it != r.properties.end()) {
            rj["weight"] = weight_it->second;
        }
        nlohmann::json rprops;
        for (const auto& [k, v] : r.properties) rprops[k] = v;
        rj["properties"] = std::move(rprops);
        edges.push_back(std::move(rj));
    }
    j["edges"] = std::move(edges);

    nlohmann::json chunks = nlohmann::json::array();
    for (const auto& c : es.chunks) {
        nlohmann::json cj;
        cj["chunk_id"]       = c.chunk_id;
        cj["source_file_id"] = c.source_file_id;
        cj["text_snippet"]   = c.text_snippet;
        cj["embedding"]      = c.embedding;
        cj["metadata"]       = c.metadata;
        const auto chunk_index_it = c.metadata.find("chunk_index");
        if (chunk_index_it != c.metadata.end()) {
            cj["chunk_index"] = chunk_index_it->second;
        }
        chunks.push_back(std::move(cj));
    }
    j["chunks"] = std::move(chunks);
    return j;
}

Result<std::string> DocumentStoreSinkAdapter::writeDocument(
    const BaseEntitySet& entity_set, const std::string& collection)
{
    // Generate a stable document ID from source_file_id when available.
    std::string doc_id = entity_set.source_file_id.empty()
        ? ("ingested-" + std::to_string(
               std::chrono::steady_clock::now().time_since_epoch().count()))
        : ("ingested-" + entity_set.source_file_id);

    themis::document::DocumentRecord rec;
    rec.id            = doc_id;
    rec.collection_id = collection;
    rec.body          = serialise(entity_set);

    auto result = store_->put(rec);
    if (!result) {
        // If already exists, attempt update
        if (result.error().code() == errors::ErrorCode::ERR_DOC_ALREADY_EXISTS) {
            auto upd = store_->update(collection, doc_id, rec.body);
            if (!upd) return tl::make_unexpected(upd.error());
        } else {
            return tl::make_unexpected(result.error());
        }
    }
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ++count_;
    }
    return doc_id;
}

std::size_t DocumentStoreSinkAdapter::documentCount() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return count_;
}

} // namespace ingestion
} // namespace themis