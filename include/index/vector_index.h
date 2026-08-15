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

/// VectorIndexManager
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

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    struct Result {
        std::string pk;
        float distance = 0.0f; // kleiner = besser (für COSINE: 1 - cosine)
    };

    explicit VectorIndexManager(RocksDBWrapper& db);
    ~VectorIndexManager() noexcept;
    
    // Phase 1: Set optional audit logger for tracking vector operations
    void setAuditLogger(std::shared_ptr<utils::AuditLogger> logger, std::string user_context = "system");
    
    // Set user context for audit logging
    void setUserContext(std::string user_id);
    
    // Phase 4: Set optional expression evaluator for advanced filtering
    void setExpressionEvaluator(std::shared_ptr<IExpressionEvaluator> evaluator);
    
    // Get expression evaluator
    std::shared_ptr<IExpressionEvaluator> getExpressionEvaluator() const;
    
    // Advanced Vector Index Integration (v1.5.0+)
    // Enable FAISS-based advanced indexing (IVF+PQ/HNSW) for large-scale datasets
    // Note: Requires THEMIS_GPU_ENABLED for FAISS/DiskANN support
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
    
    // Enable advanced indexing with specified configuration
    // Must be called before init() to take effect
    Status setAdvancedIndexConfig(const AdvancedIndexConfig& config);
    
    // Get current advanced index configuration
    AdvancedIndexConfig getAdvancedIndexConfig() const { return advanced_config_; }
    
    // Check if advanced indexing is enabled and available
    bool isAdvancedIndexEnabled() const {
        return (advanced_config_.enabled && advanced_index_ != nullptr) ||
               ann_backend_ != nullptr;
    }


    // Initialisierung eines Index-Namespace (z. B. "documents"): Dimension, M/ef, Metrik
    Status init(std::string_view objectName, int dim, Metric metric = Metric::COSINE,
                int M = 16, int efConstruction = 200, int efSearch = 64,
                const std::string& savePath = "");

    // Lifecycle-Management
        void setAutoSavePath(const std::string& savePath, bool autoSave = true);
    Status shutdown(); // Speichert Index wenn auto_save aktiviert

    // HNSW Parameter zur Laufzeit anpassen (nur efSearch; M/efConstruction erfordern Rebuild)
    Status setEfSearch(int efSearch);

    // Index aus Storage aufbauen (scannt Prefix objectName:) — optional
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

    /// Incremental re-index: sync the HNSW index with current storage state
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

    // Persistenz (optional, nur wenn HNSW aktiv): speichert Index + Mapping + Metadaten im Verzeichnis
    Status saveIndex(const std::string& directory) const;
    Status loadIndex(const std::string& directory);

    // CRUD (Standard: direktes Commit)
    Status addEntity(const BaseEntity& e, std::string_view vectorField = "embedding");
    Status updateEntity(const BaseEntity& e, std::string_view vectorField = "embedding");
    Status removeByPk(std::string_view pk);
    
    // CRUD für Transaktionen: nutzen bestehende WriteBatch
    Status addEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch, 
                     std::string_view vectorField = "embedding");
    Status updateEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch,
                        std::string_view vectorField = "embedding");
    Status removeByPk(std::string_view pk, RocksDBWrapper::WriteBatchWrapper& batch);

    // MVCC Transaction Varianten
    Status addEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                     std::string_view vectorField = "embedding");
    Status updateEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                        std::string_view vectorField = "embedding");
    Status removeByPk(std::string_view pk, RocksDBWrapper::TransactionWrapper& txn);

    // KNN-Suche; optional Whitelist von PKs für hybrides Pre-Filtering
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

    // KNN-Suche mit Attribut-Filter (Post-Filtering)
    // Filtert Ergebnisse basierend auf Entity-Attributen nach HNSW-Suche
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
    struct AttributeFilterV2 {
        std::string field;
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
        
        // Convenience constructors
        static AttributeFilterV2 Equals(std::string field, std::string value) {
            return {std::move(field), Op::EQUALS, std::move(value), {}, "", ""};
        }
        static AttributeFilterV2 Range(std::string field, std::string min, std::string max) {
            return {std::move(field), Op::RANGE, "", {}, std::move(min), std::move(max)};
        }
        static AttributeFilterV2 In(std::string field, std::vector<std::string> vals) {
            return {std::move(field), Op::IN, "", std::move(vals), "", ""};
        }
    };
    
    std::pair<Status, std::vector<Result>> searchKnnFiltered(
        const std::vector<float>& query,
        size_t k,
        const std::vector<AttributeFilter>& filters,
        size_t candidateMultiplier = 3  // Fetch k*multiplier from HNSW, then filter
    ) const;
    
    // KNN-Suche mit Pre-Filtering via SecondaryIndexManager
    // Generiert Whitelist aus SecondaryIndex-Scans, dann HNSW mit Whitelist
    // Benötigt SecondaryIndexManager-Pointer (optional dependency)
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
    
    /// Remove multiple entities by PKs in single batch
    Status removeBatch(const std::vector<std::string>& pks);

    // ===== Vector Statistics & Aggregation =====
    
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
    
    /// Compute centroid (mean vector) of all vectors in index
    std::pair<Status, std::vector<float>> computeCentroid() const;
    
    /// Compute variance per dimension
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
    
    /// Get quantization statistics
    struct QuantizationStats {
        bool enabled = false;
        bool trained = false;
        int num_subquantizers = 0;
        float compression_ratio = 0.0f;
        size_t memory_usage_bytes = 0;
    };
    QuantizationStats getQuantizationStats() const;

    // Getter für Konfiguration & Statistiken
    const std::string& getObjectName() const { return objectName_; }
    int getDimension() const { return dim_; }
    Metric getMetric() const { return metric_; }
    int getEfSearch() const { return efSearch_; }
    int getM() const { return m_; }
    int getEfConstruction() const { return efConstruction_; }
    size_t getVectorCount() const {
        if (useHnsw_ || ann_backend_ != nullptr) {
            return pkToId_.size();
        }
        return cache_.size();
    }
    bool isHnswEnabled() const { return useHnsw_; }
    const std::string& getSavePath() const { return savePath_; }
    
    /// Get vector by primary key (for searchById support)
    /// Returns nullopt if vector doesn't exist
    std::optional<std::vector<float>> getVectorByPk(std::string_view pk) const;
    
    // Encryption configuration (Phase 1)
    bool isVectorEncryptionEnabled() const;
    void setVectorEncryptionEnabled(bool enabled);
    const std::string& getVectorKeyId() const { return vectorKeyId_; }
    void setVectorKeyId(const std::string& keyId) { vectorKeyId_ = keyId; }
    
    // Phase 2: HNSW index encryption
    bool isHnswEncryptionEnabled() const;
    void setHnswEncryptionEnabled(bool enabled);
    const std::string& getHnswKeyId() const { return hnswKeyId_; }
    void setHnswKeyId(const std::string& keyId) { hnswKeyId_ = keyId; }
    
    // Phase 4: HNSW Layer Optimizer access
    HnswLayerOptimizer* getHnswOptimizer() const { return hnsw_optimizer_.get(); }
    
    // Flush pending encrypted writes (Phase 1 batching)
    void flushEncryptedWrites() const;
    
    // ===== Rotary Embeddings Support =====
    
    /// Enable/disable rotary embeddings with configuration
    Status setRotaryEmbeddingConfig(const struct RotationConfig& config);
    
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
    
    /// Add entity with automatic positional rotation
    /// The embedding is rotated based on the position parameter before storage
    Status addEntityWithRotation(
        const BaseEntity& e,
        std::string_view vectorField,
        size_t position
    );
    
    /// Add entity with relational rotation (for Knowledge Graph edges)
    /// The embedding is rotated based on the relation type
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
