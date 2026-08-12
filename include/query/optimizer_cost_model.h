/**
 * @file optimizer_cost_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <thread>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <cstddef>
#include <cstdint>

namespace themis {

/**
 * @brief Workload classification used by SerializationStrategyAdvisor.
 *
 * Allows the query optimizer to tailor the serialization/execution path
 * to the dominant access pattern without inspecting query internals.
 */
enum class WorkloadType {
    DOCUMENT_CRUD,   ///< Point reads/writes, small payloads → JSON/CPU_SINGLE
    VECTOR_SEARCH,   ///< ANN / embedding queries → Arrow IPC, GPU-eligible
    ANALYTICS_OLAP,  ///< Full-scan aggregations → Arrow IPC + GPU (large batches)
    CDC_STREAM,      ///< Change-data-capture event streams → Binary/CPU threaded
    CACHE_REPL,      ///< Internal cache replication → Protobuf/CPU threaded
    TENSOR_RAG,      ///< TT-domain RAG/FLARE retrieval — see tensor_rag_cost_model.h
                     ///< Maps to Arrow IPC / GPU_VRAM when GgmlTensorBridge is active;
                     ///< falls back to VECTOR_SEARCH serialization path otherwise.
};

/**
 * Comprehensive Query Optimizer Cost Model
 * 
 * Provides accurate cost estimation for query planning decisions including:
 * - CPU costs (per-operation basis)
 * - I/O costs (disk reads/writes)
 * - Network costs (distributed queries)
 * - Memory costs
 * - Statistics-based calculations
 */
class OptimizerCostModel {
public:
    // =============================
    // Statistics Structures
    // =============================
    
    struct TableStatistics {
        std::string tableName;
        size_t rowCount = 0;
        size_t pageCount = 0;
        double avgRowSize = 0.0;  // bytes
        bool isStale = false;
        int64_t lastUpdated = 0;  // Unix timestamp in seconds (epoch time)
    };
    
    struct ColumnStatistics {
        std::string columnName;
        size_t distinctValues = 0;
        double nullFraction = 0.0;
        std::string minValue;
        std::string maxValue;
        std::map<std::string, double> valueDistribution;  // value -> frequency
    };
    
    struct IndexStatistics {
        std::string indexName;
        std::string indexType;  // "btree", "hash", "spatial", "vector"
        size_t entryCount = 0;
        size_t levels = 0;      // tree depth for btree
        double selectivity = 1.0;
    };
    
    // =============================
    // Cost Model Constants
    // =============================
    
    struct CostConstants {
        // CPU costs
        double cpuCostPerRow = 0.01;           // Base CPU cost per row
        double cpuCostPerPredicate = 0.001;    // Predicate evaluation cost
        double cpuCostPerHash = 0.005;         // Hash computation cost
        double cpuCostPerSort = 0.02;          // Sort comparison cost
        
        // I/O costs
        double pageReadCost = 1.0;             // Sequential page read
        double randomPageReadCost = 4.0;       // Random page read
        double pageWriteCost = 2.0;            // Page write cost
        double pageSize = 8192.0;              // Default page size in bytes
        
        // Memory costs
        double memoryPenaltyFactor = 0.1;      // Penalty when exceeding available memory
        size_t availableMemory = 1024 * 1024 * 1024;  // 1GB default
        double memoryThresholdRatio = 0.8;     // Trigger external sort at 80% memory usage
        
        // Network costs
        double networkBandwidth = 1000.0;      // MB/s
        double networkLatency = 0.5;           // ms per hop
        size_t defaultHops = 1;
        
        // Operation overhead
        double scanOverhead = 10.0;
        double joinOverhead = 50.0;
        double aggregationOverhead = 20.0;
        double sortOverhead = 30.0;
        
        // Hash table constants
        size_t hashTableKeySize = 8;           // Default hash key size in bytes
        size_t hashTablePointerSize = 8;       // Pointer size (use sizeof(void*) on target platform)
        size_t hashTableGroupOverhead = 64;    // Overhead per group in aggregation

        // GPU / Serialization thresholds (kalibrierbar via calibrateCosts / updateConstant)
        size_t gpu_row_threshold_low  = 50'000;   ///< Rows ≥ this value → consider GPU path
        size_t gpu_row_threshold_high = 500'000;  ///< Rows ≥ this value → GPU+Parquet (OLAP)
        double vram_safety_factor     = 1.5;      ///< payload_bytes × factor must fit in free VRAM
        size_t cpu_batch_thread_low   = 4;        ///< Threads for medium row counts (1k–50k)
        size_t cpu_batch_thread_high  = 0;        ///< 0 = std::thread::hardware_concurrency()
        size_t msgpack_row_threshold  = 1'000;    ///< Rows ≥ this value → binary wire format
    };

    // =============================
    // Serialization Strategy Advisor
    // =============================

    /**
     * @brief Recommended wire format and execution path for a query result set.
     *
     * Produced by adviseSerializationStrategy() and embedded in QueryOptimizer::Plan.
     * Consumers use wire_format and exec_path to decide how to serialize the result
     * and how many CPU threads (or the GPU) to use for parallelism.
     */
    struct SerializationAdvice {
        // On-wire encoding for result rows.
        enum class Format {
            SF_JSON_TEXT,      // Standard JSON
            SF_BINARY_CUSTOM,  // Compact custom binary
            SF_MSGPACK_CBOR,   // MessagePack / CBOR
            SF_ARROW_IPC,      // Apache Arrow IPC stream
            SF_PROTOBUF_WIRE   // Protocol Buffers for internal/gRPC paths
        };

        // Execution path for compute.
        enum class ExecutionPath {
            CPU_SINGLE,
            CPU_THREADED_BATCH,
            GPU_VRAM
        };

        Format wire_format = Format::SF_JSON_TEXT;
        ExecutionPath exec_path = ExecutionPath::CPU_SINGLE;
        size_t recommended_batch_size = 1;
        size_t recommended_thread_count = 1;
        bool use_vram_pinned_memory = false;
        std::string rationale;
    };
    
    // =============================
    // Cost Estimation Results
    // =============================
    
    struct ScanCost {
        enum class ScanType { TABLE_SCAN, INDEX_SCAN, INDEX_ONLY_SCAN };
        
        ScanType type;
        double cpuCost = 0.0;
        double ioCost = 0.0;
        double totalCost = 0.0;
        size_t estimatedRows = 0;
        size_t pagesRead = 0;
    };
    
    struct FilterCost {
        double cpuCost = 0.0;
        double selectivity = 1.0;  // 0.0 to 1.0
        size_t inputRows = 0;
        size_t outputRows = 0;
    };
    
    struct JoinCost {
        enum class JoinType { NESTED_LOOP, HASH_JOIN, SORT_MERGE };
        
        JoinType type;
        double cpuCost = 0.0;
        double ioCost = 0.0;
        double memoryCost = 0.0;
        double totalCost = 0.0;
        size_t estimatedRows = 0;
        size_t leftRows = 0;
        size_t rightRows = 0;
    };
    
    struct AggregationCost {
        double cpuCost = 0.0;
        double memoryCost = 0.0;
        double totalCost = 0.0;
        size_t inputRows = 0;
        size_t outputRows = 0;
        size_t numGroups = 0;
    };
    
    struct SortCost {
        double cpuCost = 0.0;
        double ioCost = 0.0;  // For external sort
        double memoryCost = 0.0;
        double totalCost = 0.0;
        size_t rowCount = 0;
        bool isExternalSort = false;
    };
    
    struct NetworkCost {
        double transferCost = 0.0;
        double latencyCost = 0.0;
        double totalCost = 0.0;
        size_t dataSize = 0;  // bytes
        size_t numHops = 0;
    };
    
    // =============================
    // Public API
    // =============================
    
    OptimizerCostModel();
    explicit OptimizerCostModel(const CostConstants& constants);
    
    // Cost estimation methods
    ScanCost estimateTableScan(const TableStatistics& table) const;
    ScanCost estimateIndexScan(const TableStatistics& table, 
                               const IndexStatistics& index,
                               double selectivity) const;
    
    FilterCost estimateFilter(size_t inputRows, 
                             const std::vector<std::string>& predicates,
                             const std::map<std::string, ColumnStatistics>& columnStats) const;
    
    JoinCost estimateNestedLoopJoin(size_t leftRows, size_t rightRows, 
                                    double selectivity) const;
    JoinCost estimateHashJoin(size_t leftRows, size_t rightRows, 
                             double selectivity,
                             size_t hashKeySize = 8) const;
    JoinCost estimateSortMergeJoin(size_t leftRows, size_t rightRows,
                                   double selectivity) const;
    
    AggregationCost estimateAggregation(size_t inputRows, 
                                       size_t estimatedGroups,
                                       size_t numAggregates) const;
    
    SortCost estimateSort(size_t rowCount, size_t rowSize) const;
    
    NetworkCost estimateNetworkTransfer(size_t dataSize, size_t numHops = 1) const;
    
    // Selectivity estimation
    double estimateSelectivity(const std::string& predicate,
                              const ColumnStatistics& columnStats) const;
    double estimateJoinSelectivity(const ColumnStatistics& leftCol,
                                  const ColumnStatistics& rightCol) const;
    
    // Cost calibration
    void calibrateCosts(const std::map<std::string, double>& measurements);
    void updateConstant(const std::string& name, double value);

    /**
     * @brief Choose the optimal serialization format and execution path.
     *
     * Decision tree (thresholds from CostConstants; all overridable via calibrateCosts):
     *  - row_count < msgpack_row_threshold  OR  workload==DOCUMENT_CRUD
     *      → JSON_TEXT / CPU_SINGLE
     *  - msgpack_row_threshold ≤ row_count < gpu_row_threshold_low
     *      → MSGPACK_CBOR / CPU_THREADED_BATCH  (cpu_batch_thread_low threads)
     *  - CDC_STREAM workload
     *      → BINARY_CUSTOM / CPU_THREADED_BATCH  (any row count)
     *  - CACHE_REPL workload
     *      → PROTOBUF / CPU_THREADED_BATCH
     *  - row_count ≥ gpu_row_threshold_low, GPU available, VRAM fits
     *      → ARROW_IPC / GPU_VRAM  (use_vram_pinned_memory=true)
     *  - row_count ≥ gpu_row_threshold_low, no GPU (or VRAM too small)
     *      → ARROW_IPC / CPU_THREADED_BATCH  (hardware_concurrency threads)
     *
     * @param estimated_row_count  Expected rows in result set.
     * @param avg_row_bytes        Average encoded row size in bytes.
     * @param gpu_available        True when a GPU with sufficient VRAM is present.
     * @param vram_free_bytes      Free VRAM bytes at time of planning.
     * @param workload             Dominant access pattern hint.
     * @return SerializationAdvice filled with wire_format, exec_path, thread count, etc.
     *
     * ### Performance Targets
     *
     * The following speedup targets relative to a baseline JSON_TEXT / CPU_SINGLE pipeline
     * are expected at the default CostConstants thresholds.  All figures assume 100-byte
     * average row size on a host with ≥ 4 cores; GPU figures require RTX-class hardware
     * (≥ 8 GB VRAM, PCIe 4.0 x16).
     *
     * | Path selected                          | Condition                        | Expected throughput gain | Payload size reduction |
     * |----------------------------------------|----------------------------------|-------------------------:|------------------------|
     * | MSGPACK_CBOR / CPU_THREADED_BATCH (4T)  | 1 k–50 k rows, non-CDC           |              1.3–2.5×    | 20–50 %                |
     * | BINARY_CUSTOM / CPU_THREADED_BATCH (4T) | CDC_STREAM (any row count)       |              1.5–3×      | 30–60 %                |
     * | ARROW_IPC / CPU_THREADED_BATCH (N_hw)   | ≥ 50 k rows, no GPU              |              2–4×        | 40–65 %                |
     * | ARROW_IPC / GPU_VRAM                    | ≥ 50 k rows, GPU + VRAM ≥ 1.5× payload |       3–10×      | 40–65 %                |
     * | PROTOBUF / CPU_THREADED_BATCH           | CACHE_REPL workload              |         30–70 % smaller payload | —              |
     *
     * Decision overhead: ≤ 1 µs per call (no I/O, pure arithmetic).
     *
     * These targets are tracked in `PERFORMANCE_EXPECTATIONS.md` §2.5.
     */
    SerializationAdvice adviseSerializationStrategy(
        size_t       estimated_row_count,
        size_t       avg_row_bytes,
        bool         gpu_available,
        size_t       vram_free_bytes,
        WorkloadType workload) const;
    
    // Accessors
    const CostConstants& getConstants() const { return constants_; }
    void setConstants(const CostConstants& constants) { constants_ = constants; }

private:
    CostConstants constants_;
    
    // Helper methods
    double calculateCpuCost(size_t rowsProcessed, double costPerRow) const;
    double calculateIoCost(size_t pagesRead, bool sequential = true) const;
    double calculateMemoryCost(size_t memoryUsed) const;
    double estimateCardinality(size_t baseRows, double selectivity) const;
    bool needsExternalSort(size_t rowCount, size_t rowSize) const;
};

/**
 * Statistics Manager
 * Collects and maintains statistics for cost-based optimization
 */
class StatisticsManager {
public:
    // ------------------------------------------------------------------
    // Provider injection — callers set these before the first collect/
    // refresh call so that collectTableStatistics() / collectColumnStatistics()
    // / collectIndexStatistics() can query real storage-engine statistics
    // instead of returning zero-initialised defaults.
    // ------------------------------------------------------------------

    /// Callback signature: given a table name, return live TableStatistics.
    using TableScanProvider =
        std::function<OptimizerCostModel::TableStatistics(const std::string&)>;

    /// Callback signature: given a (table, column) pair, return live ColumnStatistics.
    using ColumnScanProvider =
        std::function<OptimizerCostModel::ColumnStatistics(const std::string&, const std::string&)>;

    /// Callback signature: given an index name, return live IndexStatistics.
    using IndexScanProvider =
        std::function<OptimizerCostModel::IndexStatistics(const std::string&)>;

    StatisticsManager() = default;

    /// Inject a real table-statistics provider (e.g. from RocksDB property queries).
    /// Once set, collectTableStatistics() calls this provider instead of returning
    /// zero-initialised defaults.
    void setTableScanProvider(TableScanProvider fn) { table_scan_provider_ = std::move(fn); }

    /// Inject a real column-statistics provider (e.g. from sampled storage scans).
    void setColumnScanProvider(ColumnScanProvider fn) { column_scan_provider_ = std::move(fn); }

    /// Inject a real index-statistics provider (e.g. from index-subsystem metadata).
    void setIndexScanProvider(IndexScanProvider fn) { index_scan_provider_ = std::move(fn); }

    // Statistics collection
    void collectTableStatistics(const std::string& tableName);
    void collectColumnStatistics(const std::string& tableName, 
                                const std::string& columnName);
    void collectIndexStatistics(const std::string& indexName);
    
    // Statistics refresh
    void refreshAllStatistics();
    void refreshStaleStatistics();
    
    // Statistics retrieval
    OptimizerCostModel::TableStatistics getTableStatistics(const std::string& tableName) const;
    OptimizerCostModel::ColumnStatistics getColumnStatistics(const std::string& tableName,
                                                            const std::string& columnName) const;
    OptimizerCostModel::IndexStatistics getIndexStatistics(const std::string& indexName) const;
    
    // Statistics management
    void invalidateStatistics(const std::string& tableName);
    bool areStatisticsStale(const std::string& tableName, int64_t threshold) const;
    
    // Manual update
    void updateTableStatistics(const std::string& tableName,
                              const OptimizerCostModel::TableStatistics& stats);
    
private:
    std::map<std::string, OptimizerCostModel::TableStatistics> tableStats_;
    std::map<std::string, std::map<std::string, OptimizerCostModel::ColumnStatistics>> columnStats_;
    std::map<std::string, OptimizerCostModel::IndexStatistics> indexStats_;

    std::optional<TableScanProvider>  table_scan_provider_;
    std::optional<ColumnScanProvider> column_scan_provider_;
    std::optional<IndexScanProvider>  index_scan_provider_;

    int64_t getCurrentTimestamp() const;
};

} // namespace themis
