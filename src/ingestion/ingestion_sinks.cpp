/**
 * @file ingestion_sinks.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_sinks.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace ingestion {

using json = nlohmann::json;

namespace {

themis::BaseEntity toStorageNode(const BaseEntity& node) {
    themis::BaseEntity::FieldMap fields;
    fields["id"] = node.id;
    fields["entity_type"] = static_cast<int64_t>(node.entity_type);
    fields["source_file_id"] = node.source_file_id;
    fields["source_text_ref"] = node.source_text_ref;
    fields["text"] = node.text;
    if (!node.embeddings.empty()) {
        fields["embedding"] = node.embeddings;
    }
    for (const auto& [key, value] : node.properties) {
        fields["prop." + key] = value;
    }
    fields["provenance.step_name"] = node.provenance.step_name;
    fields["provenance.plugin_name"] = node.provenance.plugin_name;
    fields["provenance.confidence"] = node.provenance.confidence;
    fields["provenance.extracted_at"] = node.provenance.extracted_at;
    return themis::BaseEntity::fromFields(node.id, fields);
}

themis::BaseEntity toStorageEdge(const EntityRelation& edge) {
    const auto relation_tag = std::to_string(static_cast<int>(edge.relation_type));
    const auto edge_id = edge.from_id + "->" + edge.to_id + ":" + relation_tag;
    themis::BaseEntity::FieldMap fields;
    fields["id"] = edge_id;
    fields["_from"] = edge.from_id;
    fields["_to"] = edge.to_id;
    fields["_type"] = relation_tag;
    for (const auto& [key, value] : edge.properties) {
        fields[key] = value;
    }
    return themis::BaseEntity::fromFields(edge_id, fields);
}

themis::BaseEntity toVectorEntity(const VectorRecord& record) {
    themis::BaseEntity::FieldMap fields;
    fields["chunk_id"] = record.chunk_id;
    fields["source_file_id"] = record.source_file_id;
    fields["text_snippet"] = record.text_snippet;
    fields["embedding"] = record.embedding;
    for (const auto& [key, value] : record.metadata) {
        fields["metadata." + key] = value;
    }
    return themis::BaseEntity::fromFields(record.chunk_id, fields);
}

} // namespace

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

GraphStoreSinkAdapter::GraphStoreSinkAdapter(
    std::shared_ptr<themis::RocksDBWrapper> db,
    std::shared_ptr<themis::GraphIndexManager> graph_index,
    std::string node_key_prefix)
    : db_(std::move(db))
    , graph_index_(std::move(graph_index))
    , node_key_prefix_(std::move(node_key_prefix)) {
    if (!db_) {
        throw std::invalid_argument("GraphStoreSinkAdapter: db must not be null");
    }
    if (!graph_index_) {
        throw std::invalid_argument("GraphStoreSinkAdapter: graph_index must not be null");
    }
}

Result<void> GraphStoreSinkAdapter::writeEntities(const std::vector<BaseEntity>& nodes) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& node : nodes) {
        const auto storage_node = toStorageNode(node);
        if (!db_->put(node_key_prefix_ + node.id, storage_node.serialize())) {
            return tl::make_unexpected(
                Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                      "failed to persist graph node '" + node.id + "'"));
        }
        written_node_ids_.insert(node.id);
    }
    return {};
}

Result<void> GraphStoreSinkAdapter::writeRelations(const std::vector<EntityRelation>& edges) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& edge : edges) {
        const auto status = graph_index_->addEdge(toStorageEdge(edge));
        if (!status.ok) {
            return tl::make_unexpected(
                Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                      "failed to persist edge '" + edge.from_id + "->" + edge.to_id +
                      "': " + status.message));
        }

        written_edge_ids_.insert(
            edge.from_id + "\n" + edge.to_id + "\n" +
            std::to_string(static_cast<int>(edge.relation_type)));
    }
    return {};
}

std::size_t GraphStoreSinkAdapter::nodeCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return written_node_ids_.size();
}

std::size_t GraphStoreSinkAdapter::edgeCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return written_edge_ids_.size();
}

VectorIndexSinkAdapter::VectorIndexSinkAdapter(
    std::shared_ptr<themis::VectorIndexManager> vector_index,
    std::string object_name,
    std::size_t dimension,
    std::string vector_field)
    : vector_index_(std::move(vector_index))
    , object_name_(std::move(object_name))
    , dimension_(dimension)
    , vector_field_(std::move(vector_field)) {
    if (!vector_index_) {
        throw std::invalid_argument("VectorIndexSinkAdapter: vector_index must not be null");
    }
    if (object_name_.empty()) {
        throw std::invalid_argument("VectorIndexSinkAdapter: object_name must not be empty");
    }
    if (dimension_ == 0) {
        throw std::invalid_argument("VectorIndexSinkAdapter: dimension must be > 0");
    }
}

Result<void> VectorIndexSinkAdapter::ensureInitialized() const {
    // ensureInitialized() is const because lazy initialization is logically
    // const from the caller's perspective: the observable state of the adapter
    // (the data it can write) does not change.  Only the internal `initialized_`
    // flag and the vector index registration are mutated, both of which are
    // declared `mutable` in the header to allow this pattern.
    std::lock_guard<std::mutex> lock(mtx_);
    if (initialized_) {
        return {};
    }
    if (dimension_ > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return tl::make_unexpected(
            Error(errors::ErrorCode::ERR_INDEX_INVALID_TYPE,
                  "vector dimension exceeds VectorIndexManager int range"));
    }
    const auto status = vector_index_->init(
        object_name_,
        static_cast<int>(dimension_),
        themis::VectorIndexManager::Metric::COSINE);
    if (!status.ok) {
        return tl::make_unexpected(
            Error(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED, status.message));
    }
    initialized_ = true;
    return {};
}

Result<void> VectorIndexSinkAdapter::writeVectors(const std::vector<VectorRecord>& records) {
    auto init_result = ensureInitialized();
    if (!init_result) {
        return init_result;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& record : records) {
        if (record.embedding.size() != dimension_) {
            return tl::make_unexpected(
                Error(errors::ErrorCode::ERR_INDEX_INVALID_TYPE,
                      "chunk '" + record.chunk_id + "' embedding dimension mismatch"));
        }
        const auto status = vector_index_->addEntity(toVectorEntity(record), vector_field_);
        if (!status.ok) {
            return tl::make_unexpected(
                Error(errors::ErrorCode::ERR_INDEX_CREATION_FAILED, status.message));
        }
        last_written_records_[record.chunk_id] = record;
    }
    return {};
}

std::size_t VectorIndexSinkAdapter::vectorCount() const {
    return vector_index_->getVectorCount();
}

const VectorRecord* VectorIndexSinkAdapter::findByChunkId(const std::string& chunk_id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    const auto it = last_written_records_.find(chunk_id);
    return it != last_written_records_.end() ? &it->second : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryTensorCoreBridge
// ─────────────────────────────────────────────────────────────────────────────

std::string InMemoryTensorCoreBridge::makeKey(const std::string& tenant_id,
                                             const std::string& chunk_id) {
    return tenant_id + ":" + chunk_id;
}

Result<void> InMemoryTensorCoreBridge::write(const TensorCoreRecord& record,
                                            const std::string&      tenant_id) {
    // Validate tenant_id: non-empty and no path-separator characters.
    if (tenant_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "InMemoryTensorCoreBridge::write: tenant_id is empty");
    }
    if (tenant_id.find('/') != std::string::npos ||
        std::any_of(tenant_id.begin(), tenant_id.end(),
                    [](unsigned char c) { return c == '\0'; })) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "InMemoryTensorCoreBridge::write: tenant_id contains "
                       "illegal characters ('/' or '\\0')");
    }
    if (record.chunk_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "InMemoryTensorCoreBridge::write: chunk_id is empty");
    }
    if (record.serialized_train.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "InMemoryTensorCoreBridge::write: serialized_train is empty");
    }

    std::lock_guard<std::mutex> lk(mtx_);
    records_[makeKey(tenant_id, record.chunk_id)] = record;
    ++write_count_;
    return {};
}

std::size_t InMemoryTensorCoreBridge::writeCount() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return write_count_;
}

const std::unordered_map<std::string, TensorCoreRecord>&
InMemoryTensorCoreBridge::records() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return records_;
}

const TensorCoreRecord* InMemoryTensorCoreBridge::find(
    const std::string& tenant_id, const std::string& chunk_id) const
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = records_.find(makeKey(tenant_id, chunk_id));
    return (it == records_.end()) ? nullptr : &it->second;
}

void InMemoryTensorCoreBridge::clear() {
    std::lock_guard<std::mutex> lk(mtx_);
    records_.clear();
    write_count_ = 0;
}

} // namespace ingestion
} // namespace themis

