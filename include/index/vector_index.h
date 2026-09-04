/**
 * @file vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include "index/ann_index.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <utility>
#include <memory>
#include <cstdint>
#include <mutex>

namespace themis {

class BaseEntity;
class SecondaryIndexManager;
class ProductQuantizer;
class IExpressionEvaluator;
class HnswLayerOptimizer;
class RotaryEmbedding;
struct HnswOptimizationConfig;
struct RotationConfig;

namespace utils {
    class AuditLogger;
}

/// @brief Manages a vector index namespace backed by RocksDB with optional HNSW ANN acceleration.
///
/// VectorIndexManager supports:
/// - Optional HNSWlib-Unterstützung (compile-time)
/// - Fallback: Brute-Force (L2/Cosine) über in-memory Cache oder RocksDB-Scan
/// - Persistenz: Vektoren liegen in RocksDB unter Namespace objectName:pk als BaseEntity
/// - Atomare Operationen via WriteBatch (analog zu Secondary/Graph-Indizes)
/// - In-Memory Cache für schnellen Zugriff, optional HNSW-Index für ANN
/// - Optional: Audit Logging für Vector-Operationen (Phase 1 Knowledge Graph Protection)
///
/// Sources:
/// - HNSW Algorithm: Malkov, Y. A., & Yashunin, D. A. (2018).
///   "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs"
///   IEEE Transactions on Pattern Analysis and Machine Intelligence
/// - Library: hnswlib - https://github.com/nmslib/hnswlib
/// - License: Apache 2.0
/// - ThemisDB Integration: Transactional updates, RocksDB persistence, audit logging
class VectorIndexManager {
public:
    enum class Metric { L2, COSINE, DOT };

    /// @brief Result of a vector index operation; carries ok/error state and message.
    struct Status {
        bool ok = true;
        std::string message = {};
        /// @brief Returns a successful Status.
        static Status OK() { return {}; }
        /// @brief Returns an error Status with the given message.
        /// @param msg Human-readable error description.
        /// @return Status with ok=false and the provided message.
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /// @brief A single KNN result entry holding the primary key and distance to the query vector.
    struct Result {
        std::string pk;
        float distance = 0.0f; // kleiner = besser (für COSINE: 1 - cosine)
    };

    /// @brief Constructs a VectorIndexManager bound to the given RocksDB wrapper.
    /// @param db Reference to the RocksDB wrapper used for persistence.
    explicit VectorIndexManager(RocksDBWrapper& db);
    /// @brief Destructor; saves the index if auto-save is enabled.
    ~VectorIndexManager() noexcept;
    
    /// @brief Sets the optional audit logger for tracking vector operations.
    /// @param logger Shared audit logger instance; pass nullptr to disable.
    /// @param user_context User identifier attached to audit log entries.
    void setAuditLogger(std::shared_ptr<utils::AuditLogger> logger, std::string user_context = "system");
    
    /// @brief Sets the user context used in audit log entries.
    /// @param user_id Identifier of the acting user.
    void setUserContext(std::string user_id);
    
    /// @brief Sets the optional expression evaluator used for advanced candidate filtering.
    /// @param evaluator Shared evaluator instance; pass nullptr to disable.
    void setExpressionEvaluator(std::shared_ptr<IExpressionEvaluator> evaluator);
    
    /// @brief Returns the currently configured expression evaluator, or nullptr if none is set.
    std::shared_ptr<IExpressionEvaluator> getExpressionEvaluator() const;
    
    /// @brief Configuration for the optional advanced (FAISS/DiskANN/ScaNN) index backend.
    ///
    /// Enable FAISS-based advanced indexing (IVF+PQ/HNSW) for large-scale datasets.
    /// @note Requires THEMIS_GPU_ENABLED for FAISS/DiskANN support.
    struct AdvancedIndexConfig {
        bool enabled = false;           // Enable advanced indexing
        size_t nlist = 1024;           // Number of IVF clusters
        size_t nprobe = 64;            // Number of clusters to search
        bool use_pq = true;            // Enable Product Quantization
        size_t pq_m = 8;               // Number of sub-quantizers
        size_t pq_nbits = 8;           // Bits per sub-quantizer
        bool use_gpu = false;          // Use GPU acceleration
        int gpu_device = 0;            // GPU device ID
        size_t train_size = 100000;    // Training set size
        enum class Type {
            IVF_FLAT,     // IVF without compression
            IVF_PQ,       // IVF + Product Quantization (default)
            HNSW_FLAT,    // HNSW without IVF
            IVF_HNSW_PQ,  // IVF + HNSW + PQ
            SCANN,        // ScaNN: tree-AH partitioned ANN (pure C++, no extra deps)
            DISKANN       // DiskANN: SSD-resident graph index (requires THEMIS_ENABLE_DISKANN)
        } index_type = Type::IVF_PQ;

        // ScaNN-specific parameters (used when index_type == SCANN)
        size_t scann_num_leaves           = 1000; // Voronoi cells
        size_t scann_leaves_to_search     = 100;  // Cells probed per query
        size_t scann_reorder_num_neighbors = 200; // Re-ranking candidates

        // DiskANN-specific parameters (used when index_type == DISKANN)
        std::string diskann_index_path;      // On-disk graph file path (required)
        size_t      diskann_cache_mb = 1024; // RAM cache budget in MiB
    };
    
    /// @brief Applies an advanced index configuration.
    /// @param config Configuration to activate; must be called before init() to take effect.
    Status setAdvancedIndexConfig(const AdvancedIndexConfig& config);
    
    /// @brief Returns the current advanced index configuration.
    AdvancedIndexConfig getAdvancedIndexConfig() const { return advanced_config_; }
    
    /// @brief Returns true when the advanced ANN backend is enabled and initialised.
    bool isAdvancedIndexEnabled() const {
        return (advanced_config_.enabled && advanced_index_ != nullptr) ||
               ann_backend_ != nullptr;
    }


    /// @brief Initialises an index namespace with the given parameters.
    /// @param objectName Namespace prefix used as the RocksDB key prefix.
    /// @param dim Vector dimension.
    /// @param metric Distance metric (L2, COSINE, DOT).
    /// @param M HNSW M parameter (graph connections per node).
    /// @param efConstruction HNSW efConstruction parameter.
    /// @param efSearch HNSW efSearch parameter.
    /// @param savePath Optional on-disk directory for auto-save.
    Status init(std::string_view objectName, int dim, Metric metric = Metric::COSINE,
                int M = 16, int efConstruction = 200, int efSearch = 64,
                const std::string& savePath = "");

    /// @brief Sets the auto-save path and enables or disables automatic saving on shutdown.
    /// @param savePath Directory path for index persistence.
    /// @param autoSave When true, the index is saved automatically during shutdown().
        void setAutoSavePath(const std::string& savePath, bool autoSave = true);
    /// @brief Shuts down the index, saving it if auto-save is enabled.
    Status shutdown(); // Speichert Index wenn auto_save aktiviert

    /// @brief Adjusts the efSearch parameter at runtime (without rebuilding the index).
    /// @param efSearch New efSearch value; larger values improve recall at the cost of speed.
    Status setEfSearch(int efSearch);

    /// @brief Rebuilds the HNSW index from storage by scanning the objectName: key prefix.
    /// @return Status indicating success or failure of the rebuild.
    Status rebuildFromStorage();

    // ===== Incremental Re-indexing =====

    /// Statistics returned by incrementalReindex().
    struct IncrementalReindexStats {
        size_t added     = 0; ///< Vectors added to HNSW (new in storage)
        size_t removed   = 0; ///< Vectors marked deleted in HNSW (gone from storage)
        size_t updated   = 0; ///< Vectors updated in-place in HNSW (data changed)
        size_t unchanged = 0; ///< Vectors already up-to-date (no action taken)
        size_t total_scanned = 0; ///< Total storage entries scanned
        bool   full_rebuild_triggered = false; ///< True when auto full-rebuild ran
    };

    /// @brief Incrementally re-indexes the HNSW index by syncing with current storage state
    /// without performing a full rebuild.
    ///
    /// Compares in-memory index state against storage and:
    ///   - Adds new vectors found in storage but missing from index
    ///   - Marks deleted vectors present in index but removed from storage
    ///   - Updates vectors whose data has changed in storage
    ///   - Skips unchanged vectors (preserves HNSW graph connectivity)
    ///
    /// If the ratio of soft-deleted HNSW labels exceeds @p rebuild_threshold,
    /// a full rebuild is triggered automatically and
    /// IncrementalReindexStats::full_rebuild_triggered is set to true.
    ///
    /// @param rebuild_threshold  Deleted-label fraction that triggers full rebuild (0.0–1.0).
    ///                           Pass 0 to disable automatic full rebuild.
    /// @param vectorField        Name of the vector field in entities (default "embedding").
    /// @return {Status, IncrementalReindexStats}
    std::pair<Status, IncrementalReindexStats> incrementalReindex(
        float rebuild_threshold = 0.20f,
        std::string_view vectorField = "embedding");

    /// @brief Persists the HNSW index, mapping, and metadata to the given directory.
    /// @param directory Target directory; created if it does not exist.
    /// @return Status indicating success or failure of the save operation.
    Status saveIndex(const std::string& directory) const;
    /// @brief Loads an HNSW index from the given directory.
    /// @param directory Source directory containing index files written by saveIndex().
    /// @return Status indicating success or failure of the load operation.
    Status loadIndex(const std::string& directory);

    /// @brief Adds an entity to the index using a direct commit.
    /// @param e Entity whose vector field is indexed.
    /// @param vectorField Name of the vector field within the entity (default: "embedding").
    Status addEntity(const BaseEntity& e, std::string_view vectorField = "embedding");
    /// @brief Updates an existing entity in the index using a direct commit.
    /// @param e Entity with updated vector data.
    /// @param vectorField Name of the vector field within the entity.
    Status updateEntity(const BaseEntity& e, std::string_view vectorField = "embedding");
    /// @brief Removes an entity from the index by primary key using a direct commit.
    /// @param pk Primary key of the entity to remove.
    /// @return Status indicating success or failure of the removal.
    Status removeByPk(std::string_view pk);
    
    /// @brief Adds an entity to the index within an existing WriteBatch transaction.
    /// @param e Entity whose vector field is indexed.
    /// @param batch WriteBatch to accumulate the write into.
    /// @param vectorField Name of the vector field within the entity.
    Status addEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch, 
                     std::string_view vectorField = "embedding");
    /// @brief Updates an entity in the index within an existing WriteBatch transaction.
    /// @param e Entity with updated vector data.
    /// @param batch WriteBatch to accumulate the write into.
    /// @param vectorField Name of the vector field within the entity.
    Status updateEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch,
                        std::string_view vectorField = "embedding");
    /// @brief Removes an entity from the index within an existing WriteBatch transaction.
    /// @param pk Primary key of the entity to remove.
    /// @param batch WriteBatch to accumulate the delete into.
    /// @return Status indicating success or failure of the removal.
    Status removeByPk(std::string_view pk, RocksDBWrapper::WriteBatchWrapper& batch);

    /// @brief Adds an entity to the index within an MVCC TransactionWrapper.
    /// @param e Entity whose vector field is indexed.
    /// @param txn Active MVCC transaction.
    /// @param vectorField Name of the vector field within the entity.
    Status addEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                     std::string_view vectorField = "embedding");
    /// @brief Updates an entity in the index within an MVCC TransactionWrapper.
    /// @param e Entity with updated vector data.
    /// @param txn Active MVCC transaction.
    /// @param vectorField Name of the vector field within the entity.
    Status updateEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                        std::string_view vectorField = "embedding");
    /// @brief Removes an entity from the index within an MVCC TransactionWrapper.
    /// @param pk Primary key of the entity to remove.
    /// @param txn Active MVCC transaction.
    /// @return Status indicating success or failure of the removal.
    Status removeByPk(std::string_view pk, RocksDBWrapper::TransactionWrapper& txn);

    /// @brief Finds the k nearest neighbours of the query vector.
    /// @param query Query vector; must match the index dimension.
    /// @param k Number of results to return.
    /// @param whitelistPks Optional set of PKs to restrict the search to (pre-filtering).
    std::pair<Status, std::vector<Result>> searchKnn(
        const std::vector<float>& query,
        size_t k,
        const std::vector<std::string>* whitelistPks = nullptr
    ) const;

    /// KNN-Suche mit optionalem Evaluator-basiertem Kandidatenfilter.
    ///
    /// Der Evaluator wird nur fuer den JSON-Kontextvertrag
    /// `themis_json_context_v1` angewendet (Kontext: `const nlohmann::json*`).
    /// Fuer andere Evaluator-Typen faellt die Methode auf `searchKnn(...)`
    /// ohne Evaluatorfilter zurueck.
    std::pair<Status, std::vector<Result>> searchKnnEvaluated(
        const std::vector<float>& query,
        size_t k,
        const IExpressionEvaluator* evaluator,
        size_t candidateMultiplier = 4,
        const std::vector<std::string>* whitelistPks = nullptr
    ) const;

    /// @brief Simple post-filter applied to KNN results based on entity attribute equality.
    struct AttributeFilter {
        std::string field;
        std::string value;
        enum class Op { EQUALS, NOT_EQUALS, CONTAINS } op = Op::EQUALS;
    };
    
    // Extended AttributeFilter für Pre-Filtering via SecondaryIndex
    // Hinweis: Windows headers definieren ggf. ein Makro IN. Konflikt vermeiden.
    #ifdef IN
    #undef IN
    #endif
    /// @brief Extended attribute filter with range, set, and comparison operators for pre-filtering via SecondaryIndex.
    struct AttributeFilterV2 {
        std::string field = {};
        enum class Op { 
            EQUALS,           // field == value
            NOT_EQUALS,       // field != value
            CONTAINS,         // string contains substring
            GREATER_THAN,     // field > value (numeric/string)
            LESS_THAN,        // field < value
            GREATER_EQUAL,    // field >= value
            LESS_EQUAL,       // field <= value
            IN,               // field in [values]
            RANGE             // value_min <= field <= value_max
        } op = Op::EQUALS;
        
        std::string value;              // For EQUALS, NOT_EQUALS, CONTAINS, GT, LT, GTE, LTE
        std::vector<std::string> values; // For IN operator
        std::string value_min;          // For RANGE operator
        std::string value_max;          // For RANGE operator
        
        /// @brief Creates an equality filter matching @p field == @p value.
        /// @param field Attribute field name to match against.
        /// @param value Expected value for the equality check.
        /// @return AttributeFilterV2 configured for equality matching.
        static AttributeFilterV2 Equals(std::string field, std::string value) {
            return {std::move(field), Op::EQUALS, std::move(value), {}, "", ""};
        }
        /// @brief Creates a range filter matching @p min <= @p field <= @p max.
        /// @param field Attribute field name to apply the range to.
        /// @param min Lower bound of the range (inclusive).
        /// @param max Upper bound of the range (inclusive).
        /// @return AttributeFilterV2 configured for range matching.
        static AttributeFilterV2 Range(std::string field, std::string min, std::string max) {
            return {std::move(field), Op::RANGE, "", {}, std::move(min), std::move(max)};
        }
        /// @brief Creates a set-membership filter matching @p field in @p vals.
        /// @param field Attribute field name to check membership for.
        /// @param vals Set of accepted values.
        /// @return AttributeFilterV2 configured for set-membership matching.
        static AttributeFilterV2 In(std::string field, std::vector<std::string> vals) {
            return {std::move(field), Op::IN, "", std::move(vals), "", ""};
        }
    };
    
    /// @brief KNN search with post-filtering based on entity attributes.
    /// @param query Query vector.
    /// @param k Number of results to return.
    /// @param filters Attribute filters applied after HNSW search.
    /// @param candidateMultiplier Fetch k*multiplier candidates from HNSW before filtering.
    std::pair<Status, std::vector<Result>> searchKnnFiltered(
        const std::vector<float>& query,
        size_t k,
        const std::vector<AttributeFilter>& filters,
        size_t candidateMultiplier = 3  // Fetch k*multiplier from HNSW, then filter
    ) const;
    
    /// @brief KNN search with pre-filtering via SecondaryIndexManager.
    /// @param query Query vector.
    /// @param k Number of results to return.
    /// @param filters Attribute filters used to generate a PK whitelist.
    /// @param secondaryIdx Optional SecondaryIndexManager for whitelist generation.
    std::pair<Status, std::vector<Result>> searchKnnPreFiltered(
        const std::vector<float>& query,
        size_t k,
        const std::vector<AttributeFilterV2>& filters,
        SecondaryIndexManager* secondaryIdx = nullptr
    ) const;

    // ===== Radius Search (Epsilon Neighbors) =====
    
    /// Radius search: alle Nachbarn innerhalb Distanzschwelle epsilon
    /// Optional mit k als Obergrenze (verhindert Riesige Result Sets)
    std::pair<Status, std::vector<Result>> searchKnnRadius(
        const std::vector<float>& query,
        float epsilon,
        size_t max_results = 0,  // 0 = unbegrenzt
        const std::vector<std::string>* whitelistPks = nullptr
    ) const;

    /// Radius-Suche mit optionalem Evaluator-basiertem Kandidatenfilter.
    ///
    /// Der Evaluator wird nur fuer den JSON-Kontextvertrag
    /// `themis_json_context_v1` angewendet (Kontext: `const nlohmann::json*`).
    /// Fuer andere Evaluator-Typen faellt die Methode auf
    /// `searchKnnRadius(...)` ohne Evaluatorfilter zurueck.
    std::pair<Status, std::vector<Result>> searchKnnRadiusEvaluated(
        const std::vector<float>& query,
        float epsilon,
        size_t max_results,
        const IExpressionEvaluator* evaluator,
        const std::vector<std::string>* whitelistPks = nullptr
    ) const;
    
    /// Radius search mit Pre-Filtering via SecondaryIndexManager
    std::pair<Status, std::vector<Result>> searchKnnRadiusPreFiltered(
        const std::vector<float>& query,
        float epsilon,
        size_t max_results,
        const std::vector<AttributeFilterV2>& filters,
        SecondaryIndexManager* secondaryIdx = nullptr
    ) const;

    // ===== Batch Operations =====
    
    /// Add multiple entities in single batch (more efficient than individual adds)
    Status addBatch(const std::vector<BaseEntity>& entities, std::string_view vectorField = "embedding");
    
    /// Update multiple entities in single batch
    Status updateBatch(const std::vector<BaseEntity>& entities, std::string_view vectorField = "embedding");
    
    /// @brief Removes multiple entities by primary key in a single batch.
    /// @param pks List of primary keys to remove.
    /// @return Status indicating success or failure of the batch removal.
    Status removeBatch(const std::vector<std::string>& pks);

    // ===== Vector Statistics & Aggregation =====

    /// @brief Distance distribution and count statistics for the index.
    struct Statistics {
        size_t vector_count = 0;
        int dimension = 0;
        float min_distance = 0.0f;
        float max_distance = 0.0f;
        float mean_distance = 0.0f;
        float std_dev_distance = 0.0f;
        std::string metric_name;
    };
    
    /// Get index statistics (distance distribution, vector count, etc.)
    std::pair<Status, Statistics> getStatistics() const;
    
    /// @brief Computes the centroid (mean vector) of all vectors in the index.
    /// @return Pair of Status and the centroid vector; Status is error if the index is empty.
    std::pair<Status, std::vector<float>> computeCentroid() const;
    
    /// @brief Computes per-dimension variance across all vectors in the index.
    /// @return Pair of Status and the per-dimension variance vector.
    std::pair<Status, std::vector<float>> computeVariance() const;
    
    /// Find outlier vectors (those far from centroid)
    /// Returns PKs of vectors with distance > threshold * std_dev from centroid
    std::pair<Status, std::vector<std::string>> findOutliers(float threshold = 3.0f) const;

    // ===== Vector Quantization (Feature #7) =====
    
    /// Enable/disable product quantization for memory compression
    /// Must be called before adding vectors or after training
    Status enableQuantization(bool enable, int num_subquantizers = 8);
    
    /// Train quantizer with existing vectors or provided training set
    /// If training_vectors is empty, uses existing vectors from cache
    Status trainQuantizer(const std::vector<std::vector<float>>& training_vectors = {});
    
    /// Check if quantization is enabled and trained
    bool isQuantizationEnabled() const { return quantization_enabled_; }
    bool isQuantizerTrained() const;
    
    /// @brief Product quantization state and compression statistics.
    struct QuantizationStats {
        bool enabled = false;
        bool trained = false;
        int num_subquantizers = 0;
        float compression_ratio = 0.0f;
        size_t memory_usage_bytes = 0;
    };
    QuantizationStats getQuantizationStats() const;

    /// @brief Returns the index namespace (object name).
    const std::string& getObjectName() const { return objectName_; }
    /// @brief Returns the vector dimension.
    int getDimension() const { return dim_; }
    /// @brief Returns the configured distance metric.
    Metric getMetric() const { return metric_; }
    /// @brief Returns the current efSearch parameter.
    int getEfSearch() const { return efSearch_; }
    /// @brief Returns the HNSW M parameter.
    int getM() const { return m_; }
    /// @brief Returns the HNSW efConstruction parameter.
    int getEfConstruction() const { return efConstruction_; }
    /// @brief Returns the number of indexed vectors.
    size_t getVectorCount() const {
        if (useHnsw_ || ann_backend_ != nullptr) {
            return pkToId_.size();
        }
        return cache_.size();
    }
    /// @brief Returns true when the HNSW index is active.
    bool isHnswEnabled() const { return useHnsw_; }
    /// @brief Returns the configured on-disk save path.
    const std::string& getSavePath() const { return savePath_; }
    
    /// Get vector by primary key (for searchById support)
    /// Returns nullopt if vector doesn't exist
    std::optional<std::vector<float>> getVectorByPk(std::string_view pk) const;
    
    /// @brief Returns true when per-vector encryption is enabled.
    bool isVectorEncryptionEnabled() const;
    /// @brief Enables or disables per-vector encryption.
    void setVectorEncryptionEnabled(bool enabled);
    /// @brief Returns the key ID used for vector encryption.
    const std::string& getVectorKeyId() const { return vectorKeyId_; }
    /// @brief Sets the key ID used for vector encryption.
    void setVectorKeyId(const std::string& keyId) { vectorKeyId_ = keyId; }
    
    /// @brief Returns true when HNSW index encryption is enabled.
    bool isHnswEncryptionEnabled() const;
    /// @brief Enables or disables HNSW index encryption.
    void setHnswEncryptionEnabled(bool enabled);
    /// @brief Returns the key ID used for HNSW index encryption.
    const std::string& getHnswKeyId() const { return hnswKeyId_; }
    /// @brief Sets the key ID used for HNSW index encryption.
    void setHnswKeyId(const std::string& keyId) { hnswKeyId_ = keyId; }
    
    /// @brief Returns the HNSW layer optimizer, or nullptr if not configured.
    HnswLayerOptimizer* getHnswOptimizer() const { return hnsw_optimizer_.get(); }
    
    /// @brief Flushes any pending encrypted writes from the internal batch buffer.
    void flushEncryptedWrites() const;
    
    // ===== Rotary Embeddings Support =====
    
    /// Enable/disable rotary embeddings with configuration
    Status setRotaryEmbeddingConfig(const struct RotationConfig& config);

    /**
     * @brief Disable rotary embeddings for subsequent vector operations.
     *
     * Clears the active RoPE configuration and resets runtime counters. Calls
     * that require rotary embeddings will fail closed after this method returns.
     *
     * @return OK when RoPE was disabled, or an error when RoPE is already off.
     */
    Status disableRotaryEmbedding();
    
    /// Check if rotary embeddings are enabled
    bool isRotaryEmbeddingEnabled() const { return rotary_enabled_; }
    
    /// Get current rotary embedding configuration
    /// Returns nullopt if rotary embeddings are not enabled
    std::optional<struct RotationConfig> getRotaryEmbeddingConfig() const;

    struct RotaryEmbeddingStats {
        uint64_t total_rotated_entities = 0;
        uint64_t total_relational_rotations = 0;
        double avg_rotation_time_us = 0.0;
    };

    /// Get runtime RoPE stats. Returns nullopt when RoPE is disabled.
    std::optional<RotaryEmbeddingStats> getRotaryEmbeddingStats() const;
    
    /// @brief Adds an entity with automatic positional rotation applied to its embedding.
    /// @param e Entity whose vector field is indexed.
    /// @param vectorField Name of the vector field within the entity.
    /// @param position Position index used to compute the rotation angle.
    /// @return Status indicating success or failure.
    Status addEntityWithRotation(
        const BaseEntity& e,
        std::string_view vectorField,
        size_t position
    );
    
    /// @brief Adds an entity with relational rotation for Knowledge Graph edges.
    /// @param e Entity whose vector field is indexed.
    /// @param vectorField Name of the vector field within the entity.
    /// @param relation_type Relation type identifier used to compute the rotation.
    /// @return Status indicating success or failure.
    Status addEntityWithRelationalRotation(
        const BaseEntity& e,
        std::string_view vectorField,
        const std::string& relation_type
    );

    /**
     * @brief Runtime counters for rotary embedding operations.
     *
     * Incremented atomically by addEntityWithRotation (total_rotated_entities)
     * and addEntityWithRelationalRotation (relational_rotations).
     */
    struct RotaryStats {
        uint64_t total_rotated_entities{0};  ///< Cumulative positional-rotation adds
        uint64_t relational_rotations{0};    ///< Cumulative relational-rotation adds
    };

    /// Return a snapshot of the rotary embedding counters.
    RotaryStats getRotaryStats() const {
        return { rotary_positional_rotations_.load(std::memory_order_relaxed),
                 rotary_relational_rotations_.load(std::memory_order_relaxed) };
    }
    
    /// KNN search with rotation-aware query
    /// Rotates the query vector before search
    std::pair<Status, std::vector<Result>> searchWithRotation(
        const std::vector<float>& query,
        int k,
        size_t query_position,
        const std::vector<std::string>* whitelistPks = nullptr
    ) const;

private:
    RocksDBWrapper& db_;
    std::string objectName_;
    int dim_ = 0;
    Metric metric_ = Metric::COSINE;
    int efSearch_ = 64;
    int m_ = 16;
    int efConstruction_ = 200;
        std::string savePath_; // Verzeichnis für saveIndex/loadIndex
        bool autoSave_ = false; // Automatisches Speichern bei shutdown()
    
    // Phase 1: Vector encryption configuration
    std::string vectorKeyId_ = "vector_embeddings";  // Key ID for vector encryption
    
    // Phase 2: HNSW index encryption configuration
    std::string hnswKeyId_ = "hnsw_index";  // Key ID for HNSW index encryption

    // In-Memory Mapping PK <-> Label-ID (für HNSW) und Cache für Fallback
    mutable std::recursive_mutex index_state_mutex_;
    mutable std::unordered_map<std::string, size_t> pkToId_;
    mutable std::vector<std::string> idToPk_;
    mutable std::unordered_map<std::string, std::vector<float>> cache_; // für Fallback/Whitelist

    // Product Quantization (Feature #7)
    bool quantization_enabled_ = false;
    std::unique_ptr<ProductQuantizer> quantizer_;
    mutable std::unordered_map<std::string, std::vector<uint8_t>> quantized_cache_;  // PK -> codes

    // HNSWlib Index (wenn verfügbar)
#ifdef THEMIS_HNSW_ENABLED
    struct HnswDeleter { void operator()(void* /*p*/) const {} };
    // Wir verwenden Pointer-void, um hnswlib-Header-Dependency zu vermeiden, wenn nicht definiert
    void* hnswIndex_ = nullptr; // tatsächlich hnswlib::HierarchicalNSW<float>*
    void* hnswSpace_ = nullptr; // tatsächlich hnswlib::SpaceInterface<float>* (owned; freed with hnswIndex_)
    bool useHnsw_ = false;
#else
    bool useHnsw_ = false;
#endif

    // Hilfsfunktionen
    static float l2(const std::vector<float>& a, const std::vector<float>& b);
    static float cosineOneMinus(const std::vector<float>& a, const std::vector<float>& b);
    static float dotProduct(const std::vector<float>& a, const std::vector<float>& b);
    static void normalizeL2(std::vector<float>& v);
    float distance(const std::vector<float>& a, const std::vector<float>& b) const;

    // Storage Keys
    std::string makeObjectKey(std::string_view pk) const;

    // Interne Suche
    std::vector<Result> bruteForceSearch_(const std::vector<float>& query, size_t k,
                                          const std::vector<std::string>* whitelist) const;

    // Encryption insert batching (Phase 1): reduce per-vector DB commit overhead
    std::unique_ptr<RocksDBWrapper::WriteBatchWrapper> encBatch_;
    size_t encBatchCount_ = 0;
    size_t encBatchSize_ = 256; // commit every N encrypted inserts
    void flushEncBatch() const;
    
    // Phase 5: Safe HNSW resource cleanup (RAII safety fix)
    void releaseHnswResources_() noexcept;
    
    // Phase 1: Optional AuditLogger for knowledge graph protection
    std::shared_ptr<utils::AuditLogger> audit_logger_;
    std::string user_context_ = "system";  // Default user context
    
    // Phase 4: Optional ExpressionEvaluator for advanced filtering
    std::shared_ptr<IExpressionEvaluator> expression_evaluator_;
    
    // Phase 4: HNSW Layer Optimizer for vector index optimization
    std::unique_ptr<HnswLayerOptimizer> hnsw_optimizer_;
    
    // Rotary Embeddings support
    std::unique_ptr<RotaryEmbedding> rotary_embedding_;
    bool rotary_enabled_ = false;
    mutable std::atomic<uint64_t> rotary_positional_rotations_{0};
    mutable std::atomic<uint64_t> rotary_relational_rotations_{0};
    mutable std::atomic<uint64_t> rotary_query_rotations_{0};
    mutable std::atomic<uint64_t> rotary_total_rotation_time_us_{0};
    
    // Advanced Vector Index Integration (v1.5.0+)
    AdvancedIndexConfig advanced_config_;
    std::unique_ptr<class AdvancedVectorIndex> advanced_index_;
    // Alternative ANN backends (ScaNN / DiskANN) – active when index_type is SCANN or DISKANN
    std::unique_ptr<index::IAnnIndex> ann_backend_;
    
    // Helper: Log audit event if logger is set
    void logAuditEvent_(const std::string& event_type, const std::string& resource,
                       const std::string& operation, size_t count = 0) const;
    
    // Helper: Load HNSW optimization configuration from YAML
    void loadHnswOptimizationConfig_();
};

} // namespace themis
