/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            optimizer_cost_model.h                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     273                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstddef>

namespace themis {

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

        // Serialization strategy thresholds
        size_t gpuRowThresholdLow  = 50'000;   // Rows above which GPU Arrow IPC is preferred
        size_t msgpackRowThreshold = 10'000;   // Rows above which Arrow CPU-parallel beats msgpack
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
    
    // Accessors
    const CostConstants& getConstants() const { return constants_; }
    void setConstants(const CostConstants& constants) { constants_ = constants; }

    // =============================
    // Serialization Strategy Advisor
    // =============================

    /// Workload context for serialization strategy selection.
    enum class SerializationWorkloadType {
        UNKNOWN,
        CPU_BATCH,          ///< General CPU batch processing
        CPU_PARALLEL,       ///< Parallel CPU execution (multiple threads)
        GPU_VRAM,           ///< GPU execution, data resides in VRAM
    };

    /// Recommended serialization path for a query plan.
    struct SerializationAdvice {
        enum class Format {
            BINARY_BATCH_CPU,    ///< msgpack / binary row format for CPU batches
            ARROW_CPU_PARALLEL,  ///< Apache Arrow IPC for multi-threaded CPU
            ARROW_GPU_VRAM,      ///< Apache Arrow IPC optimised for GPU memory transfer
        };

        Format              format      = Format::BINARY_BATCH_CPU;
        std::string         description;    ///< Human-readable rationale
        bool                gpu_capable = false; ///< Indicates GPU-accelerated path
    };

    /**
     * @brief Advise the best serialization strategy given workload and row count.
     *
     * Decision tree:
     *  - GPU workload AND rows >= gpuRowThresholdLow  → ARROW_GPU_VRAM
     *  - rows >= msgpackRowThreshold (non-GPU)        → ARROW_CPU_PARALLEL
     *  - otherwise                                    → BINARY_BATCH_CPU
     *
     * @param workload  Workload classification for the planned query.
     * @param row_count Expected number of rows to serialise.
     * @return          Populated SerializationAdvice struct.
     */
    SerializationAdvice adviseSerializationStrategy(
        SerializationWorkloadType workload,
        size_t                    row_count) const;

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
    StatisticsManager() = default;
    
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
    
    int64_t getCurrentTimestamp() const;
};

} // namespace themis
