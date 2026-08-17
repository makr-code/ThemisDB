/**
 * @file provenance_store.h
 * @brief Post-generation provenance persistence and queryability.
 *
 * Enables audit-ready retrieval of complete provenance chains for retrieval steps:
 * ANN frontdoor → Tensor Mid-Layer → Graph Validator → Final Layer.
 *
 * Backed by RocksDB for production-grade persistence, crash recovery, and range queries.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis::observability {

/**
 * @brief Generalized provenance record for one retrieval step.
 *
 * Stores query execution lineage independent of end-to-end
 * TensorRAG flow. Useful for auditing, debugging, and SLO validation
 * across distributed retrieval layers.
 */
struct ProvenanceStepRecord {
    // ── identity ────────────────────────────────────────────────────────────
    std::string query_id;
    int         step_number = 0;
    std::string correlation_id;
    int64_t     timestamp_ms = 0;

    // ── layer/layer flow ───────────────────────────────────────────────────
    std::string layer_name;          ///< e.g. "AnnFrontdoor", "TensorMidLayer"
    std::string source_layer;        ///< Upstream layer (if part of a chain)

    // ── candidates/evidence ─────────────────────────────────────────────────
    int64_t  num_candidates = 0;
    int64_t  num_selected   = 0;
    std::string input_vector_hash;   ///< Hash of input query vector/embedding
    std::string shard_id;            ///< If applicable (for distributed)
    std::string backend_name;        ///< Backend used (e.g. "VectorIndexManager")

    // ── routing/fallback ────────────────────────────────────────────────────
    std::string routing_reason_code;        ///< Decision code (e.g. kDistributedFanout)
    std::string fallback_mode;              ///< "CONTINUE_DEGRADED", "FAIL_CLOSED", "NONE"
    std::string confidence_policy_version;

    // ── performance ─────────────────────────────────────────────────────────
    int64_t decision_duration_us = 0;   ///< Step duration in microseconds
};

/**
 * @brief Abstract interface for persistent provenance storage.
 *
 * Supports ACID semantics for individual record storage and
 * efficient range queries on (query_id, timestamp).
 */
class IProvenanceStore {
public:
    virtual ~IProvenanceStore() = default;

    /**
     * @brief Store a provenance record in the index.
     *
     * @param query_id Unique query identifier.
     * @param step_number Step ordinal within the query execution.
     * @param record Provenance record to persist.
     * @return true if record was stored successfully, false on error.
     */
    [[nodiscard]] virtual bool storeRecord(const std::string& query_id,
                                           int step_number,
                                           const ProvenanceStepRecord& record) = 0;

    /**
     * @brief Retrieve a single provenance record by query_id and step.
     *
     * @param query_id Query identifier.
     * @param step_number Step ordinal.
     * @return Record if found; empty optional if not found or on error.
     */
    [[nodiscard]] virtual std::optional<ProvenanceStepRecord> getRecord(
        const std::string& query_id, int step_number) = 0;

    /**
     * @brief Retrieve the complete provenance chain for a query.
     *
     * @param query_id Query identifier.
     * @return Vector of records ordered by step_number (ascending).
     *         Empty vector if query_id not found.
     */
    [[nodiscard]] virtual std::vector<ProvenanceStepRecord> getProvenanceChain(
        const std::string& query_id) = 0;

    /**
     * @brief Query provenance records by timestamp range.
     *
     * Useful for audit windows and forensic analysis.
     *
     * @param start_ts_ms Start timestamp (milliseconds since epoch).
     * @param end_ts_ms End timestamp (milliseconds since epoch).
     * @return Vector of all records within the range, sorted by timestamp.
     */
    [[nodiscard]] virtual std::vector<ProvenanceStepRecord> getRecordsByTimeRange(
        int64_t start_ts_ms, int64_t end_ts_ms) = 0;

    /**
     * @brief List all unique query_ids in the store.
     *
     * @return Vector of query identifiers.
     */
    [[nodiscard]] virtual std::vector<std::string> listQueryIds() = 0;

    /**
     * @brief Delete all provenance records for a query.
     *
     * @param query_id Query identifier to purge.
     * @return true if purge succeeded, false on error.
     */
    [[nodiscard]] virtual bool deleteQuery(const std::string& query_id) = 0;
};

/**
 * @brief RocksDB-backed provenance store implementation.
 *
 * Provides persistent storage of provenance records with crash recovery,
 * efficient range scans, and configurable retention policies.
 *
 * Key schema: "provenance:<query_id>:<step_number:08d>"
 * Time index: "provenance_ts:<timestamp_ms:016x>:<query_id>"
 */
class RocksDBProvenanceStore final : public IProvenanceStore {
public:
    struct Config {
        /// Path to the RocksDB data directory.
        std::string db_path;

        /// Maximum number of records to keep in memory cache before flush.
        size_t batch_flush_threshold = 1000;

        /// Enable compression (default: true).
        bool enable_compression = true;

        /// Maximum number of persisted provenance records.
        /// 0 means unlimited.
        std::size_t retention_max_records = 0;

        /// Maximum age of persisted provenance records in milliseconds.
        /// Records older than now - retention_max_age_ms are removed on writes.
        /// 0 means unlimited.
        int64_t retention_max_age_ms = 0;
    };

    /**
     * @brief Construct and open RocksDB provenance store.
     *
     * @param config Storage configuration.
     * @throws std::runtime_error if database cannot be opened.
     */
    explicit RocksDBProvenanceStore(Config config);

    /**
     * @brief Close the RocksDB handle and flush pending writes.
     *
     * Automatically called by destructor.
     */
    ~RocksDBProvenanceStore() override;

    // Disable copy; move is allowed.
    RocksDBProvenanceStore(const RocksDBProvenanceStore&) = delete;
    RocksDBProvenanceStore& operator=(const RocksDBProvenanceStore&) = delete;
    RocksDBProvenanceStore(RocksDBProvenanceStore&&)           noexcept = default;
    RocksDBProvenanceStore& operator=(RocksDBProvenanceStore&&) noexcept = default;

    // IProvenanceStore interface implementation
    [[nodiscard]] bool storeRecord(const std::string&                 query_id,
                                   int                                 step_number,
                                   const ProvenanceStepRecord& record) override;

    [[nodiscard]] std::optional<ProvenanceStepRecord> getRecord(
        const std::string& query_id, int step_number) override;

    [[nodiscard]] std::vector<ProvenanceStepRecord> getProvenanceChain(
        const std::string& query_id) override;

    [[nodiscard]] std::vector<ProvenanceStepRecord> getRecordsByTimeRange(
        int64_t start_ts_ms, int64_t end_ts_ms) override;

    [[nodiscard]] std::vector<std::string> listQueryIds() override;

    [[nodiscard]] bool deleteQuery(const std::string& query_id) override;

    /**
     * @brief Flush all pending writes to disk.
     *
     * @return true if flush succeeded.
     */
    [[nodiscard]] bool flush();

    /**
     * @brief Compact the database to reclaim space.
     *
     * @return true if compaction started successfully.
     */
    [[nodiscard]] bool compact();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis::observability
