/**
 * @file ingestion_sinks.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include "document/document_store.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace themis {
class RocksDBWrapper;
class GraphIndexManager;
class VectorIndexManager;

namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// IGraphWriter — sink for graph nodes and edges
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for writing a `BaseEntitySet` to the graph store.
 *
 * Implementations can target:
 *  - ThemisDB internal graph store (production)
 *  - In-memory store (tests / development)
 *  - External graph DB (neo4j, ArangoDB — via plugins)
 *
 * Thread-safety: `writeEntities()` and `writeRelations()` MUST be thread-safe.
 */
class IGraphWriter {
public:
    virtual ~IGraphWriter() = default;

    /**
     * @brief Upsert a batch of `BaseEntity` nodes.
     *
     * Nodes are keyed by `BaseEntity::id`; an existing node with the same ID
     * is updated with any new/changed properties.
     *
     * @param nodes  Entities to write as graph nodes.
     * @return Error on persistent I/O failure.
     */
    virtual Result<void> writeEntities(const std::vector<BaseEntity>& nodes) = 0;

    /**
     * @brief Upsert a batch of `EntityRelation` edges.
     *
     * An edge is identified by (from_id, to_id, relation_type); duplicate
     * edges with the same key are merged (properties updated).
     *
     * @param edges  Relations to write as graph edges.
     * @return Error on persistent I/O failure.
     */
    virtual Result<void> writeRelations(const std::vector<EntityRelation>& edges) = 0;

    /**
     * @brief Convenience method: write an entire `BaseEntitySet`.
     *
     * Calls `writeEntities()` + `writeRelations()` in order.
     */
    virtual Result<void> write(const BaseEntitySet& entity_set);

    /**
     * @brief Returns the number of nodes currently stored.
     */
    virtual std::size_t nodeCount() const = 0;

    /**
     * @brief Returns the number of edges currently stored.
     */
    virtual std::size_t edgeCount() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IVectorWriter — sink for vector index entries
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for writing `VectorRecord` entries to a vector index.
 *
 * Implementations:
 *  - ThemisDB HNSW vector index (production)
 *  - In-memory linear-scan index (tests / development)
 *
 * Thread-safety: `writeVectors()` MUST be thread-safe.
 */
class IVectorWriter {
public:
    virtual ~IVectorWriter() = default;

    /**
     * @brief Insert or update a batch of `VectorRecord` entries.
     *
     * Records are keyed by `VectorRecord::chunk_id`; an existing record is
     * replaced when a new record with the same key is written.
     *
     * @param records  Vector records to upsert.
     * @return Error on capacity overflow or I/O failure.
     */
    virtual Result<void> writeVectors(const std::vector<VectorRecord>& records) = 0;

    /**
     * @brief Returns the number of vectors currently indexed.
     */
    virtual std::size_t vectorCount() const = 0;

    /**
     * @brief Retrieve a stored vector record by chunk_id.
     *
     * Returns nullptr when not found.
     */
    virtual const VectorRecord* findByChunkId(const std::string& chunk_id) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocWriter — sink for raw document storage
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for persisting a `BaseEntitySet` as a document.
 *
 * The default implementation serialises the entity set to JSON and stores it
 * via `IDocumentStore`.
 *
 * Thread-safety: `writeDocument()` MUST be thread-safe.
 */
class IDocWriter {
public:
    virtual ~IDocWriter() = default;

    /**
     * @brief Store the entity set as a document.
     *
     * @param entity_set  Assembled entity set to persist.
     * @param collection  Target collection name in the document store.
     * @return Assigned document ID on success, or an error.
     */
    virtual Result<std::string> writeDocument(const BaseEntitySet& entity_set,
                                               const std::string& collection = "ingested") = 0;

    /**
     * @brief Returns the number of documents written.
     */
    virtual std::size_t documentCount() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryGraphWriter — reference implementation for tests
// ─────────────────────────────────────────────────────────────────────────────

// STUB/SIMULATION NOTE:
// Purpose: Provide a fully functional but non-persistent IGraphWriter for use
//   in unit tests, integration tests, and dry-run ingestion scenarios.
// Activation: Explicitly instantiated by test code or by callers that do not
//   have a real graph database available at test time.
// Production Delta: Data exists only in process memory and is lost on
//   destruction.  No ACID guarantees, no replication, no persistence.
// Removal Plan: Not removed — retained as the canonical test double and for
//   future in-memory graph scenarios (e.g. ephemeral batch imports).

/**
 * @brief Thread-safe in-memory `IGraphWriter` implementation.
 *
 * Suitable for unit tests and dry-run scenarios.  Not persistent.
 */
class InMemoryGraphWriter : public IGraphWriter {
public:
    Result<void> writeEntities(const std::vector<BaseEntity>& nodes) override;
    Result<void> writeRelations(const std::vector<EntityRelation>& edges) override;
    std::size_t  nodeCount() const override;
    std::size_t  edgeCount() const override;

    /// Direct read access for test assertions.
    const std::unordered_map<std::string, BaseEntity>& nodes() const { return nodes_; }
    const std::vector<EntityRelation>&                 edges() const { return edges_; }

    /// Clear all stored data.
    void clear();

private:
    mutable std::mutex                           mtx_;
    std::unordered_map<std::string, BaseEntity>  nodes_;
    std::vector<EntityRelation>                  edges_;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryVectorWriter — reference implementation for tests
// ─────────────────────────────────────────────────────────────────────────────

// STUB/SIMULATION NOTE:
// Purpose: Provide a fully functional but non-persistent IVectorWriter for
//   unit tests and dry-run ingestion scenarios.
// Activation: Explicitly instantiated by test code or callers without a real
//   vector store available.
// Production Delta: Data exists only in process memory with no ANN index,
//   no persistence, and no similarity-search capability.
// Removal Plan: Not removed — retained as the canonical test double.

/**
 * @brief Thread-safe in-memory `IVectorWriter` implementation.
 */
class InMemoryVectorWriter : public IVectorWriter {
public:
    Result<void>       writeVectors(const std::vector<VectorRecord>& records) override;
    std::size_t        vectorCount() const override;
    const VectorRecord* findByChunkId(const std::string& chunk_id) const override;

    /// Direct read access for test assertions.
    const std::unordered_map<std::string, VectorRecord>& records() const {
        return records_;
    }

    /// Clear all stored data.
    void clear();

private:
    mutable std::mutex                              mtx_;
    std::unordered_map<std::string, VectorRecord>  records_;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocWriter — reference implementation for tests
// ─────────────────────────────────────────────────────────────────────────────

// STUB/SIMULATION NOTE:
// Purpose: Provide a fully functional but non-persistent IDocWriter for unit
//   tests and dry-run ingestion scenarios; also used by DocumentStoreSinkAdapter
//   tests when no real IDocumentStore is wired.
// Activation: Explicitly instantiated by test code or dry-run callers.
// Production Delta: Data exists only in process memory, serialised as JSON
//   snapshots.  No schema evolution, no versioning, no encryption support.
// Removal Plan: Not removed — retained as the canonical test double.
//   Production persistence is provided by DocumentStoreSinkAdapter below.

/**
 * @brief Thread-safe in-memory `IDocWriter` implementation for tests/dry-runs.
 *
 * Stores `BaseEntitySet` snapshots serialised to JSON.
 * For production persistence use `DocumentStoreSinkAdapter`.
 */
class InMemoryDocWriter : public IDocWriter {
public:
    Result<std::string> writeDocument(const BaseEntitySet& entity_set,
                                       const std::string& collection) override;
    std::size_t documentCount() const override;

    /// Return stored JSON for a given document ID (empty string if not found).
    std::string getDocument(const std::string& doc_id) const;

    /// Clear all stored data.
    void clear();

private:
    mutable std::mutex                        mtx_;
    std::unordered_map<std::string, std::string> docs_;
    std::size_t                               next_id_{1};
};

// ─────────────────────────────────────────────────────────────────────────────
// IngestionSinkBundle — convenience wrapper for all three sinks
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Groups all three sinks for convenient passing through the pipeline.
 *
 * The `WorkflowEngine` / `IngestionManager` can optionally accept a
 * `IngestionSinkBundle` and write the assembled `BaseEntitySet` to it after
 * workflow execution completes.
 */
struct IngestionSinkBundle {
    std::shared_ptr<IGraphWriter>  graph;   ///< may be nullptr (skip graph write)
    std::shared_ptr<IVectorWriter> vector;  ///< may be nullptr (skip vector write)
    std::shared_ptr<IDocWriter>    doc;     ///< may be nullptr (skip doc write)

    /**
     * @brief Write `entity_set` to all configured (non-null) sinks.
     *
     * @return First error encountered, or success when all sinks succeed.
     */
    Result<void> writeAll(const BaseEntitySet& entity_set,
                           const std::string& collection = "ingested") const;
};

// ─────────────────────────────────────────────────────────────────────────────
// DocumentStoreSinkAdapter — IDocWriter backed by IDocumentStore (Phase 5)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Production `IDocWriter` that persists `BaseEntitySet` records to a
 *        real `IDocumentStore` backend (ThemisDB document module).
 *
 * Each `writeDocument()` call serialises the `BaseEntitySet` to JSON and calls
 * `IDocumentStore::put()`.  The document ID is taken from
 * `BaseEntitySet::source_doc_id` when non-empty, or auto-generated as
 * `"ingested-<uuid4>"` otherwise.
 *
 * Thread-safety: all methods are thread-safe.
 */
class DocumentStoreSinkAdapter : public IDocWriter {
public:
    /**
     * @brief Construct adapter around an existing `IDocumentStore`.
     *
     * @param store    Backing store (must not be nullptr).
     * @throws std::invalid_argument when store is nullptr.
     */
    explicit DocumentStoreSinkAdapter(
        std::shared_ptr<themis::document::IDocumentStore> store);

    Result<std::string> writeDocument(const BaseEntitySet& entity_set,
                                       const std::string& collection) override;
    std::size_t documentCount() const override;

    /// Access the underlying store for inspection.
    const themis::document::IDocumentStore& store() const { return *store_; }

private:
    std::shared_ptr<themis::document::IDocumentStore> store_;
    mutable std::mutex                                 mtx_;
    std::size_t                                        count_{0};

    /// Serialise a `BaseEntitySet` to a JSON body for the document store.
    static nlohmann::json serialise(const BaseEntitySet& es);
};

/**
 * @brief Production `IGraphWriter` backed by RocksDB + GraphIndexManager.
 *
 * Nodes are persisted as canonical `BaseEntity` blobs under a configurable key
 * prefix; edges are upserted through `GraphIndexManager`.
 */
class GraphStoreSinkAdapter : public IGraphWriter {
public:
    GraphStoreSinkAdapter(std::shared_ptr<themis::RocksDBWrapper> db,
                          std::shared_ptr<themis::GraphIndexManager> graph_index,
                          std::string node_key_prefix = "ingestion:graph:node:");

    Result<void> writeEntities(const std::vector<BaseEntity>& nodes) override;
    Result<void> writeRelations(const std::vector<EntityRelation>& edges) override;
    std::size_t nodeCount() const override;
    std::size_t edgeCount() const override;

private:
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::string node_key_prefix_;
    mutable std::mutex mtx_;
    std::unordered_set<std::string> written_node_ids_;
    std::unordered_set<std::string> written_edge_ids_;
};

/**
 * @brief Production `IVectorWriter` backed by `VectorIndexManager`.
 */
class VectorIndexSinkAdapter : public IVectorWriter {
public:
    VectorIndexSinkAdapter(std::shared_ptr<themis::VectorIndexManager> vector_index,
                           std::string object_name,
                           std::size_t dimension,
                           std::string vector_field = "embedding");

    Result<void> writeVectors(const std::vector<VectorRecord>& records) override;
    std::size_t vectorCount() const override;
    const VectorRecord* findByChunkId(const std::string& chunk_id) const override;

private:
    Result<void> ensureInitialized() const;

    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::string object_name_;
    std::size_t dimension_;
    std::string vector_field_;
    mutable std::mutex mtx_;
    mutable bool initialized_{false};
    std::unordered_map<std::string, VectorRecord> last_written_records_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ITensorCoreBridge — bridge for TT-core records produced by chunk_tt_decompose
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for persisting `TensorCoreRecord`s.
 *
 * The `builtin.tensor_core_bridge` ingestion step calls this interface for every
 * `TensorCoreRecord` in `ExtractionContext::tensor_cores`.
 *
 * Implementations:
 *  - `InMemoryTensorCoreBridge`   — in-memory map, for unit tests
 *  - `TensorCoreStorageBridge`    — writes to `ITensorStorageBackend` (in `tensor/`)
 *
 * Key contract (implemented by all concrete bridges):
 *  - Upsert semantics: re-ingesting the same `chunk_id` under the same `tenant_id`
 *    replaces the existing record rather than creating a duplicate.
 *  - `write()` MUST be thread-safe.
 *  - A non-empty `TensorCoreRecord::serialized_train` is required;
 *    records with empty train bytes are rejected with ERR_DOC_INVALID_ARGUMENT.
 */
class ITensorCoreBridge {
public:
    virtual ~ITensorCoreBridge() = default;

    /**
     * @brief Persist one Tensor-Train core record.
     *
     * @param record     Pre-computed `TensorCoreRecord` from the decompose step.
     * @param tenant_id  Tenant scope for isolation (validated: non-empty, no '/').
     * @return Error on invalid input or I/O failure.
     */
    virtual Result<void> write(const TensorCoreRecord& record,
                               const std::string&      tenant_id) = 0;

    /**
     * @brief Number of records successfully written since construction.
     */
    virtual std::size_t writeCount() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryTensorCoreBridge — thread-safe in-memory sink for unit tests
// ─────────────────────────────────────────────────────────────────────────────

// STUB/SIMULATION NOTE:
// Purpose: Provide a fully functional but non-persistent ITensorCoreBridge for
//   unit tests and development environments where no storage backend is available.
// Activation: Any consumer that injects InMemoryTensorCoreBridge instead of
//   TensorCoreStorageBridge (the production RocksDB-backed impl).
// Production Delta: Data is NOT persisted; all records are lost on process exit.
// Removal Plan: Not removed — remains the no-persistence default for tests.

/**
 * @brief Thread-safe in-memory implementation of `ITensorCoreBridge`.
 *
 * Records are stored in `std::unordered_map<tenant_id+":"+chunk_id, TensorCoreRecord>`.
 * Suitable for unit tests and in-process integration tests.
 */
class InMemoryTensorCoreBridge : public ITensorCoreBridge {
public:
    Result<void> write(const TensorCoreRecord& record,
                       const std::string& tenant_id) override;

    std::size_t writeCount() const override;

    /// Return all stored records (keyed by `<tenant>:<chunk_id>`).
    const std::unordered_map<std::string, TensorCoreRecord>& records() const;

    /// Retrieve a record by tenant_id + chunk_id (returns nullptr if absent).
    const TensorCoreRecord* find(const std::string& tenant_id,
                                 const std::string& chunk_id) const;

    /// Remove all records (for test cleanup).
    void clear();

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, TensorCoreRecord> records_;
    std::size_t write_count_{0};

    static std::string makeKey(const std::string& tenant_id,
                               const std::string& chunk_id);
};

} // namespace ingestion
} // namespace themis
