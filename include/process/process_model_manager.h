/**
 * @file process_model_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/process_graph.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <shared_mutex>
#include <atomic>

namespace themis {
class RocksDBWrapper;
class InvertedIndex;
class VectorIndexManager;

namespace process {

/**
 * @brief Supported process model notation formats.
 */
enum class ProcessNotation {
    BPMN_2_0,   ///< Business Process Model and Notation 2.0 (ISO/IEC 19510)
    EPK,         ///< Ereignisgesteuerte Prozesskette (Event-driven Process Chain)
    VCC_VPB,     ///< VCC-VPB YAML process definition format
    CMMN_1_1,   ///< Case Management Model and Notation 1.1 (OMG)
    DMN_1_5,    ///< Decision Model and Notation 1.5 (OMG)
};

/**
 * @brief Domain classification for a process model.
 *
 * Used for filtering, LLM context generation, and compliance tagging.
 */
enum class ProcessDomain {
    ADMINISTRATION,     ///< Verwaltungsprozess (public administration)
    BUSINESS,           ///< Geschäftsprozess (general business)
    IT_SERVICE,         ///< IT-Service-Management (ITIL)
    HEALTHCARE,         ///< Gesundheitswesen
    FINANCE,            ///< Finanzen / Rechnungswesen
    CUSTOMER_SERVICE,   ///< Kundenservice
    CUSTOM,             ///< Anwenderdefiniert
};

/**
 * @brief Lifecycle state of a process model stored in the DB.
 */
enum class ProcessModelState {
    DRAFT,      ///< In Bearbeitung – not yet deployable
    ACTIVE,     ///< Deployed and executable
    DEPRECATED, ///< Superseded by a newer version
    ARCHIVED,   ///< Read-only, retired model
};

/**
 * @brief Metadata record that accompanies each process model stored as a base-entity.
 *
 * All fields map to reserved or well-known JSON keys so that AQL queries can filter
 * and sort process models without additional joins.
 */
struct ProcessModelRecord {
    // Identity
    std::string id;           ///< Unique model ID (e.g. "bauantrag_standard")
    std::string name;         ///< Human-readable name (DE)
    std::string name_en;      ///< Human-readable name (EN)
    std::string version;      ///< Semantic version string ("1.0.0")
    int         revision{0};  ///< Auto-incremented on every save

    // Classification
    ProcessNotation notation{ProcessNotation::BPMN_2_0};
    ProcessDomain   domain{ProcessDomain::BUSINESS};
    ProcessModelState state{ProcessModelState::DRAFT};

    // Description (used for LLM context and semantic search)
    std::string description;     ///< Short DE description
    std::string description_en;  ///< Short EN description
    std::string long_description; ///< Detailed description for LLM context

    // Compliance / regulatory tags (e.g. "DSGVO", "GWB", "§34 BauO")
    std::vector<std::string> compliance_tags;

    // Ownership
    std::string owner;        ///< Responsible team or role
    std::string created_by;
    std::string updated_by;
    int64_t     created_at_ms{0};
    int64_t     updated_at_ms{0};

    // Embedded model payload (raw format bytes / YAML / XML)
    std::string raw_payload;  ///< Original format payload (BPMN XML, EPK text, YAML)

    // Normalised internal representation (built by import)
    nlohmann::json normalized; ///< Canonical JSON process graph (nodes + edges + metadata)

    // Vector embedding of the long_description for semantic search
    std::vector<float> embedding; ///< Optional: pre-computed text embedding

    /**
     * @brief Serialize the record to a BaseEntity-compatible JSON document.
     *
     * Uses reserved ThemisDB field names so that the document is
     * indexable by the process graph engine.
     */
    [[nodiscard]] nlohmann::json toDocument() const;

    /**
     * @brief Deserialize a JSON document back into a ProcessModelRecord.
     */
    static ProcessModelRecord fromDocument(const nlohmann::json& doc);
};

// ---------------------------------------------------------------------------
// Operation result type
// ---------------------------------------------------------------------------

/**
 * @brief Result type used by all ProcessModelManager operations.
 */
struct ProcessModelResult {
    bool        ok{false};
    std::string message;
    std::string model_id; ///< Populated on successful write operations

    static ProcessModelResult success(std::string_view id = "");
    static ProcessModelResult failure(std::string_view msg);
};

// ---------------------------------------------------------------------------
// ProcessModelManager
// ---------------------------------------------------------------------------

/**
 * @brief High-level manager for process model definitions stored in ThemisDB.
 *
 * ## Responsibilities
 *
 * - Import process models from BPMN 2.0 XML, EPK text, or VCC-VPB YAML.
 * - Persist models as base-entity documents in the `_process_definitions` system
 *   collection, making them queryable via AQL just like any other document.
 * - Export models to BPMN 2.0 XML, EPK, or the LLM-optimised JSON descriptor.
 * - Bridge to `ProcessGraphManager` for execution (deploy / undeploy).
 * - Generate LLM-readable descriptions for use in RAG pipelines.
 *
 * ## Layer over Base-Entities
 *
 * Every process model is stored as a regular ThemisDB document with a well-known
 * `_type = "process_definition"` field.  This means:
 *
 * ```aql
 * FOR m IN _process_definitions
 *   FILTER m.domain == "ADMINISTRATION"
 *   RETURN m
 * ```
 *
 * works without any special query language extension.
 *
 * ## Thread Safety
 *
 * All public methods are thread-safe when called on separate instances backed by the
 * same RocksDB database (RocksDB itself is thread-safe for concurrent reads and writes).
 */
class ProcessModelManager {
public:
    /**
     * @brief Construct a ProcessModelManager with a RocksDB backend.
     *
     * @param db Reference to a RocksDBWrapper instance that will be used for all model storage
     *           and retrieval operations. The ProcessModelManager does not own this reference;
     *           the caller is responsible for keeping the RocksDBWrapper alive for the entire
     *           lifetime of this manager instance.
     *
     * @note Thread-safe: Multiple ProcessModelManager instances can be created with the same
     *       RocksDB backend, and their operations will properly synchronize via RocksDB's
     *       internal locking mechanisms.
     *
     * @note The constructor does not perform any I/O; initialization is lazy (performed
     *       when the first import/retrieval operation is called).
     *
     * @see ~ProcessModelManager() for cleanup semantics
     */
    explicit ProcessModelManager(::themis::RocksDBWrapper& db);
    ~ProcessModelManager();

    // Prevent accidental copy
    ProcessModelManager(const ProcessModelManager&)            = delete;
    ProcessModelManager& operator=(const ProcessModelManager&) = delete;

    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    /**
     * @brief Import a BPMN 2.0 XML document and store it as a process model.
     *
     * @param bpmn_xml  Full BPMN 2.0 XML string.
     * @param meta      Optional metadata overrides (name, domain, owner, …).
     * @return ProcessModelResult with the assigned model_id on success.
     */
    ProcessModelResult importBpmn(
        std::string_view bpmn_xml,
        const ProcessModelRecord& meta = {}
    );

    /**
     * @brief Import an EPK (Ereignisgesteuerte Prozesskette) definition.
     *
     * Accepts a lightweight text-based EPK description (one node per line,
     * edges indicated by `->` notation, or JSON format).
     *
     * @param epk_text  EPK definition string.
     * @param meta      Metadata overrides.
     */
    ProcessModelResult importEpk(
        std::string_view epk_text,
        const ProcessModelRecord& meta = {}
    );

    /**
     * @brief Import a VCC-VPB YAML process definition.
     *
     * VCC-VPB is the native format used by the Visual Change Control –
     * Visual Process Builder tool.  The YAML schema is documented in
     * `config/process_models/README.md`.
     *
     * @param yaml_text  Raw YAML content.
     * @param meta       Metadata overrides.
     */
    ProcessModelResult importVccVpb(
        std::string_view yaml_text,
        const ProcessModelRecord& meta = {}
    );

    /**
     * @brief Import an EPK model from an ARIS Markup Language (AML) XML document.
     *
     * Parses the first EPK `<Model>` found in the AML file produced by
     * ARIS Designer 9.x / 10.x.  ARIS TypeNum values are mapped to the
     * corresponding EPKNodeType values (see EpkArisXmlImporter for the
     * full mapping table).
     *
     * @param aml_xml  Full AML XML string.
     * @param meta     Optional metadata overrides (name, domain, owner, …).
     * @return ProcessModelResult with the assigned model_id on success.
     */
    ProcessModelResult importArisXml(
        std::string_view aml_xml,
        const ProcessModelRecord& meta = {}
    );

    // -------------------------------------------------------------------------
    // CRUD
    // -------------------------------------------------------------------------

    /**
     * @brief Save or update a ProcessModelRecord in the DB.
     *
     * If a record with the same `id` already exists, a new revision is written
     * and the old one is kept under its versioned key (audit trail).
     */
    ProcessModelResult save(const ProcessModelRecord& record);

    /**
     * @brief Load a process model by ID.
     *
     * @param model_id  The unique model identifier.
     * @return The record, or std::nullopt when not found.
     */
    std::optional<ProcessModelRecord> load(std::string_view model_id) const;

    /**
     * @brief Delete a process model (soft-delete: marks state as ARCHIVED).
     */
    ProcessModelResult remove(std::string_view model_id);

    // -------------------------------------------------------------------------
    // Query / Search
    // -------------------------------------------------------------------------

    /**
     * @brief List all process models, optionally filtered by domain and state.
     *
     * @param domain  If set, only models of this domain are returned.
     * @param state   If set, only models with this lifecycle state are returned.
     * @param limit   Maximum number of results (0 = unlimited).
     */
    std::vector<ProcessModelRecord> list(
        std::optional<ProcessDomain>      domain = std::nullopt,
        std::optional<ProcessModelState>  state  = std::nullopt,
        size_t                            limit  = 0
    ) const;

    /**
     * @brief Full-text search across model names and descriptions.
     *
     * Simple keyword matching — for semantic search use findSimilar().
     *
     * @param query  Keyword or phrase.
     * @param limit  Maximum results.
     */
    std::vector<ProcessModelRecord> search(
        std::string_view query,
        size_t           limit = 20
    ) const;

    /**
     * @brief Vector-similarity search: find models semantically similar to a
     *        natural-language query or another model's embedding.
     *
     * @param query_embedding  Embedding vector for the query.
     * @param k                Number of nearest neighbours to return.
     * @param min_similarity   Minimum cosine similarity threshold [0, 1].
     */
    std::vector<std::pair<ProcessModelRecord, float>> findSimilar(
        const std::vector<float>& query_embedding,
        size_t                    k              = 10,
        float                     min_similarity = 0.7f
    ) const;

    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Export a stored model to BPMN 2.0 XML.
     *
     * @return BPMN XML string, or empty on failure.
     */
    std::string exportBpmn(std::string_view model_id) const;

    /**
     * @brief Export a stored model to EPK text format.
     */
    std::string exportEpk(std::string_view model_id) const;

    /**
     * @brief Generate an LLM-optimised JSON descriptor for a model.
     *
     * The descriptor is designed to be injected into an LLM system prompt or
     * RAG context.  It includes structured node/edge descriptions, compliance
     * tags, SLA information, and a natural-language summary.
     */
    nlohmann::json generateLlmDescriptor(std::string_view model_id) const;

    // -------------------------------------------------------------------------
    // Execution bridge
    // -------------------------------------------------------------------------

    /**
     * @brief Deploy a process model to the ProcessGraphManager for execution.
     *
     * Converts the stored model to ProcessNodeInfo / ProcessEdgeInfo objects
     * and registers them with the given engine.
     *
     * @param model_id  The model to deploy.
     * @param engine    Target execution engine.
     */
    ProcessModelResult deployToEngine(
        std::string_view      model_id,
        ProcessGraphManager&  engine
    ) const;

    /**
     * @brief Undeploy (unregister) a model from the execution engine.
     */
    ProcessModelResult undeployFromEngine(
        std::string_view     model_id,
        ProcessGraphManager& engine
    ) const;

    // -------------------------------------------------------------------------
    // Validation and Hardening
    // -------------------------------------------------------------------------

    /**
     * @brief Validate process model consistency and integrity.
     *
     * Checks:
     * - Required fields are non-empty (id, name, version)
     * - All referenced nodes in edges exist
     * - No dangling references
     * - Best-effort bounded cycle diagnostics (warnings only, non-fatal)
     * - Resource limits respected (max nodes/edges/depth)
     *
     * @param record The record to validate
     * @return ProcessModelResult with detailed validation errors on failure
     */
    ProcessModelResult validateModelConsistency(const ProcessModelRecord& record) const;

    /**
     * @brief Get internal state consistency checks for debugging.
     *
     * Returns diagnostic information about:
     * - Total models loaded
     * - Index coherency status (FTS/Vector)
     * - Orphaned or corrupted records
     * - Revision chain integrity
     *
     * @return JSON diagnostic object
     */
    [[nodiscard]] nlohmann::json getConsistencyDiagnostics() const;

    // -------------------------------------------------------------------------
    // Optional integrations
    // -------------------------------------------------------------------------

    /**
     * @brief Wire a text-embedding function.
     *
     * When set, @p embedder is called automatically inside save() whenever the
     * saved record has an empty embedding vector.  The concatenation of
     * `name + " " + description + " " + long_description` is used as input.
     *
     * @param embedder  Callable `(std::string_view text) → std::vector<float>`.
     *                  Pass an empty function to disable.
     */
    void setEmbedder(std::function<std::vector<float>(std::string_view)> embedder);

    /**
     * @brief Wire an InvertedIndex for BM25 full-text search.
     *
     * When set, save() automatically indexes the model name and description
     * fields, and remove() removes the posting entries.  The search() method
     * uses the BM25 index instead of the linear keyword scan.
     *
     * The index must be created for the logical table "process_definitions"
     * and the column "text" before the first save() call.
     *
     * @param fts  Shared pointer to an InvertedIndex instance (may be null to
     *             disable).
     */
    void setInvertedIndex(std::shared_ptr<InvertedIndex> fts);

    /**
     * @brief Wire a VectorIndexManager for HNSW-based findSimilar().
     *
     * When set, findSimilar() delegates to the HNSW index for O(log n)
     * approximate nearest-neighbour search instead of a linear cosine scan.
     * save() upserts the model embedding; remove() deletes it from the index.
     *
     * The index must be initialised for the object name "process_models"
     * with the correct embedding dimension before the first save() call.
     *
     * @param vi  Shared pointer to an initialised VectorIndexManager (may be
     *            null to disable).
     */
    void setVectorIndex(std::shared_ptr<VectorIndexManager> vi);

private:
    ::themis::RocksDBWrapper& db_;
    std::function<std::vector<float>(std::string_view)> embedder_;
    std::shared_ptr<InvertedIndex> fts_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;

    // Phase 2: Concurrency guards and determinism tracking
    mutable std::shared_mutex model_state_lock_;  ///< RWLock for model CRUD operations
    mutable std::shared_mutex linking_lock_;      ///< RWLock for linking state consistency
    std::atomic<uint64_t> operation_counter_{0};  ///< Track operation sequence for determinism

    /**
     * @brief Transaction context for multi-step operations requiring atomicity.
     * Used to detect conflicts and roll back on high-churn scenarios.
     */
    struct TransactionContext {
        uint64_t txn_id = 0;
        std::string model_id;
        int64_t start_time_ms;
        int revision_at_start;
        std::vector<std::string> modified_keys;
        bool is_active{true};
    };

    /**
     * @brief Guard for maintaining transaction semantics during concurrent operations.
     * Implements RAII-style automatic rollback on destruction if marked failed.
     */
    class TransactionGuard {
    public:
        TransactionGuard(ProcessModelManager& mgr, const TransactionContext& ctx)
            : manager_(mgr), context_(ctx), failed_(false) {}
        ~TransactionGuard();
        
        void markFailed() { failed_ = true; }
        const TransactionContext& getContext() const { return context_; }
        
        // Prevent copying
        TransactionGuard(const TransactionGuard&) = delete;
        TransactionGuard& operator=(const TransactionGuard&) = delete;
        
    private:
        ProcessModelManager& manager_;
        TransactionContext context_;
        bool failed_;
    };

    // Helpers
    std::string makeKey_(std::string_view model_id) const;
    std::string makeVersionedKey_(std::string_view model_id, int revision) const;

    // Internal: build normalised JSON from a raw import
    static nlohmann::json buildNormalizedGraph_(
        const std::vector<ProcessNodeInfo>&  nodes,
        const std::vector<ProcessEdgeInfo>&  edges,
        const ProcessModelRecord&            meta
    );

    // Phase 2: Determinism and churn detection helpers
    /**
     * @brief Detect if another operation modified the model during our transaction.
     * @param model_id The model being tracked.
     * @param expected_revision The revision we expect.
     * @return true if conflict detected.
     */
    bool detectConflict_(std::string_view model_id, int expected_revision) const;

    /**
     * @brief Rollback modifications to a model in case of conflict.
     * @param txn Transaction context describing what was modified.
     */
    void rollbackTransaction_(const TransactionContext& txn);

    /**
     * @brief Create a new transaction context for a multi-step operation.
     * @param model_id The model being modified.
     * @return TransactionContext with unique ID and timestamp.
     */
    TransactionContext createTransaction_(std::string_view model_id);
};

// ---------------------------------------------------------------------------
// Helper: human-readable names for enums
// ---------------------------------------------------------------------------

std::string_view toString(ProcessNotation n);
std::string_view toString(ProcessDomain   d);
std::string_view toString(ProcessModelState s);

ProcessNotation   notationFromString(std::string_view s);
ProcessDomain     domainFromString(std::string_view s);
ProcessModelState stateFromString(std::string_view s);

} // namespace process
} // namespace themis
