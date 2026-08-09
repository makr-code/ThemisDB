/**
 * @file optimizer_cost_model.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=3, Debt=0, C=6, H=8, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: optimizer_cost_model.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 784
 * Gap Summary: total=10; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=3, Debt=0, C=12, H=14, M=1, L=0
 * PR History (last 5): #1018 Complete cost optimization ... (2026-03-11) | #795 Implement comprehensive que... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Query Optimizer Cost Model Implementation

#include "query/optimizer_cost_model.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <chrono>
#include <thread>

namespace themis {
namespace {

size_t saturatingMultiply(size_t a, size_t b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > (std::numeric_limits<size_t>::max() / b)) {
        return std::numeric_limits<size_t>::max();
    }
    return a * b;
}

double clampUnitInterval(double value, double fallback = 0.0) {
    const double candidate = std::isfinite(value) ? value : fallback;
    return std::clamp(candidate, 0.0, 1.0);
}

} // namespace

// =============================
// OptimizerCostModel Implementation
// =============================

OptimizerCostModel::OptimizerCostModel() : constants_() {}

OptimizerCostModel::OptimizerCostModel(const CostConstants& constants) 
    : constants_(constants) {}

// CPU Cost Calculation
double OptimizerCostModel::calculateCpuCost(size_t rowsProcessed, double costPerRow) const {
    return static_cast<double>(rowsProcessed) * costPerRow;
}

// I/O Cost Calculation
double OptimizerCostModel::calculateIoCost(size_t pagesRead, bool sequential) const {
    double costPerPage = sequential ? constants_.pageReadCost : constants_.randomPageReadCost;
    return static_cast<double>(pagesRead) * costPerPage;
}

// Memory Cost Calculation
double OptimizerCostModel::calculateMemoryCost(size_t memoryUsed) const {
    if (memoryUsed <= constants_.availableMemory) {
        return 0.0;  // No penalty if within available memory
    }
    
    double excessMemory = static_cast<double>(memoryUsed - constants_.availableMemory);
    double memoryRatio = excessMemory / static_cast<double>(constants_.availableMemory);
    return memoryRatio * constants_.memoryPenaltyFactor;
}

// Cardinality Estimation
double OptimizerCostModel::estimateCardinality(size_t baseRows, double selectivity) const {
    return static_cast<double>(baseRows) * std::max(0.0, std::min(1.0, selectivity));
}

// Check if external sort is needed
bool OptimizerCostModel::needsExternalSort(size_t rowCount, size_t rowSize) const {
    size_t totalSize = saturatingMultiply(rowCount, rowSize);
    // External sort needed if data size exceeds threshold of available memory
    size_t memoryThreshold = static_cast<size_t>(
        constants_.availableMemory * constants_.memoryThresholdRatio
    );
    return totalSize > memoryThreshold;
}

// =============================
// Table Scan Cost Estimation
// =============================

OptimizerCostModel::ScanCost OptimizerCostModel::estimateTableScan(
    const TableStatistics& table) const {
    
    ScanCost cost;
    cost.type = ScanCost::ScanType::TABLE_SCAN;
    cost.estimatedRows = table.rowCount;
    cost.pagesRead = table.pageCount;
    
    // I/O cost: sequential read of all pages
    cost.ioCost = calculateIoCost(table.pageCount, true) + constants_.scanOverhead;
    
    // CPU cost: process each row
    cost.cpuCost = calculateCpuCost(table.rowCount, constants_.cpuCostPerRow);
    
    cost.totalCost = cost.ioCost + cost.cpuCost;
    return cost;
}

// =============================
// Index Scan Cost Estimation
// =============================

OptimizerCostModel::ScanCost OptimizerCostModel::estimateIndexScan(
    const TableStatistics& table,
    const IndexStatistics& index,
    double selectivity) const {
    
    ScanCost cost;
    cost.type = ScanCost::ScanType::INDEX_SCAN;
    
    // Estimated rows based on selectivity
    cost.estimatedRows = static_cast<size_t>(
        estimateCardinality(table.rowCount, selectivity)
    );
    
    // Index navigation cost (btree levels)
    double indexNavigationCost = static_cast<double>(index.levels) * constants_.randomPageReadCost;
    
    // Random I/O to fetch actual rows
    size_t estimatedPages = std::max(
        size_t(1),
        static_cast<size_t>(table.pageCount * selectivity)
    );
    cost.pagesRead = estimatedPages;
    
    // I/O cost: index navigation + random page reads
    cost.ioCost = indexNavigationCost + 
                  calculateIoCost(estimatedPages, false) +
                  constants_.scanOverhead;
    
    // CPU cost: process matched rows
    cost.cpuCost = calculateCpuCost(cost.estimatedRows, constants_.cpuCostPerRow);
    
    cost.totalCost = cost.ioCost + cost.cpuCost;
    return cost;
}

// =============================
// Filter Cost Estimation
// =============================

OptimizerCostModel::FilterCost OptimizerCostModel::estimateFilter(
    size_t inputRows,
    const std::vector<std::string>& predicates,
    const std::map<std::string, ColumnStatistics>& columnStats) const {
    
    FilterCost cost;
    cost.inputRows = inputRows;
    
    // Estimate combined selectivity (assuming independence)
    double combinedSelectivity = 1.0;
    for (const auto& pred : predicates) {
        // Simple heuristic: each predicate filters ~10% by default
        double predSelectivity = 0.1;
        
        // Check if we have column statistics
        auto it = columnStats.find(pred);
        if (it != columnStats.end()) {
            const auto& colStats = it->second;
            predSelectivity = estimateSelectivity(pred, colStats);
        }
        
        combinedSelectivity *= predSelectivity;
    }
    
    cost.selectivity = combinedSelectivity;
    cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
    
    // CPU cost: evaluate predicates for each input row
    double predicateCost = static_cast<double>(predicates.size()) * 
                          constants_.cpuCostPerPredicate;
    cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
    
    return cost;
}

// =============================
// Join Cost Estimation
// =============================

OptimizerCostModel::JoinCost OptimizerCostModel::estimateNestedLoopJoin(
    size_t leftRows,
    size_t rightRows,
    double selectivity) const {
    
    JoinCost cost;
    cost.type = JoinCost::JoinType::NESTED_LOOP;
    cost.leftRows = leftRows;
    cost.rightRows = rightRows;
    
    // Nested loop: for each left row, scan all right rows
    size_t comparisons = leftRows * rightRows;
    cost.cpuCost = calculateCpuCost(comparisons, constants_.cpuCostPerPredicate) +
                   constants_.joinOverhead;
    
    // I/O cost: read right table once for each left row (if not in memory)
    cost.ioCost = 0.0;  // Assume tables are in memory for nested loop
    
    cost.estimatedRows = static_cast<size_t>(comparisons * selectivity);
    cost.totalCost = cost.cpuCost + cost.ioCost;
    
    return cost;
}

OptimizerCostModel::JoinCost OptimizerCostModel::estimateHashJoin(
    size_t leftRows,
    size_t rightRows,
    double selectivity,
    size_t hashKeySize) const {
    
    JoinCost cost;
    cost.type = JoinCost::JoinType::HASH_JOIN;
    cost.leftRows = leftRows;
    cost.rightRows = rightRows;
    
    // Build phase: hash all rows from smaller table (assume left is smaller)
    double buildCost = calculateCpuCost(leftRows, constants_.cpuCostPerHash);
    
    // Probe phase: hash and lookup for each right row
    double probeCost = calculateCpuCost(rightRows, constants_.cpuCostPerHash);
    
    cost.cpuCost = buildCost + probeCost + constants_.joinOverhead;
    
    // Memory cost: hash table for left table
    size_t hashTableSize = leftRows * (hashKeySize + constants_.hashTablePointerSize);
    cost.memoryCost = calculateMemoryCost(hashTableSize);
    
    const double estimatedD = static_cast<double>(leftRows) * static_cast<double>(rightRows) * selectivity;
    cost.estimatedRows = estimatedD >= static_cast<double>(std::numeric_limits<size_t>::max()) ? std::numeric_limits<size_t>::max() : static_cast<size_t>(estimatedD);
    cost.totalCost = cost.cpuCost + cost.memoryCost;
    
    return cost;
}

OptimizerCostModel::JoinCost OptimizerCostModel::estimateSortMergeJoin(
    size_t leftRows,
    size_t rightRows,
    double selectivity) const {
    
    JoinCost cost;
    cost.type = JoinCost::JoinType::SORT_MERGE;
    cost.leftRows = leftRows;
    cost.rightRows = rightRows;
    
    // Sort both inputs
    double leftSortCost = leftRows * std::log2(static_cast<double>(leftRows) + 1.0) * 
                         constants_.cpuCostPerSort;
    double rightSortCost = rightRows * std::log2(static_cast<double>(rightRows) + 1.0) *
                          constants_.cpuCostPerSort;
    
    // Merge phase: single scan of both sorted inputs
    double mergeCost = calculateCpuCost(leftRows + rightRows, constants_.cpuCostPerPredicate);
    
    cost.cpuCost = leftSortCost + rightSortCost + mergeCost + constants_.joinOverhead;
    
    const double estimatedD = static_cast<double>(leftRows) * static_cast<double>(rightRows) * selectivity;
    cost.estimatedRows = estimatedD >= static_cast<double>(std::numeric_limits<size_t>::max()) ? std::numeric_limits<size_t>::max() : static_cast<size_t>(estimatedD);
    cost.totalCost = cost.cpuCost;
    
    return cost;
}

// =============================
// Aggregation Cost Estimation
// =============================

OptimizerCostModel::AggregationCost OptimizerCostModel::estimateAggregation(
    size_t inputRows,
    size_t estimatedGroups,
    size_t numAggregates) const {
    
    constexpr size_t AGGREGATE_VALUE_SIZE = 8;  // Size of each aggregate value
    
    AggregationCost cost;
    cost.inputRows = inputRows;
    cost.outputRows = estimatedGroups;
    cost.numGroups = estimatedGroups;
    
    // Hash-based aggregation
    // Cost: hash each input row and update aggregate
    double hashCost = calculateCpuCost(inputRows, constants_.cpuCostPerHash);
    double aggregateCost = static_cast<double>(inputRows * numAggregates) * 
                          constants_.cpuCostPerPredicate;
    
    cost.cpuCost = hashCost + aggregateCost + constants_.aggregationOverhead;
    
    // Memory cost: hash table for groups
    size_t hashTableSize = estimatedGroups * 
        (constants_.hashTableGroupOverhead + numAggregates * AGGREGATE_VALUE_SIZE);
    cost.memoryCost = calculateMemoryCost(hashTableSize);
    
    cost.totalCost = cost.cpuCost + cost.memoryCost;
    
    return cost;
}

// =============================
// Sort Cost Estimation
// =============================

OptimizerCostModel::SortCost OptimizerCostModel::estimateSort(
    size_t rowCount,
    size_t rowSize) const {
    
    SortCost cost;
    cost.rowCount = rowCount;
    cost.isExternalSort = needsExternalSort(rowCount, rowSize);
    
    if (cost.isExternalSort) {
        // External sort: multiple passes over data
        size_t totalSize = rowCount * rowSize;
        size_t numPasses = 1 + static_cast<size_t>(
            std::ceil(std::log2(static_cast<double>(totalSize) / constants_.availableMemory))
        );
        
        // I/O cost: read and write data multiple times
        size_t totalPages = (totalSize + static_cast<size_t>(constants_.pageSize) - 1) / 
                           static_cast<size_t>(constants_.pageSize);
        cost.ioCost = static_cast<double>(totalPages * numPasses * 2) * 
                     constants_.pageReadCost;
        
        // CPU cost: comparisons
        cost.cpuCost = rowCount * std::log2(static_cast<double>(rowCount) + 1.0) * 
                      constants_.cpuCostPerSort + constants_.sortOverhead;
        
        cost.memoryCost = 0.0;  // External sort manages memory explicitly
    } else {
        // In-memory sort
        cost.cpuCost = rowCount * std::log2(static_cast<double>(rowCount) + 1.0) * 
                      constants_.cpuCostPerSort + constants_.sortOverhead;
        cost.ioCost = 0.0;
        
        // Memory cost: need to hold all data in memory
        size_t totalSize = saturatingMultiply(rowCount, rowSize);
        cost.memoryCost = calculateMemoryCost(totalSize);
    }
    
    cost.totalCost = cost.cpuCost + cost.ioCost + cost.memoryCost;
    
    return cost;
}

// =============================
// Network Cost Estimation
// =============================

OptimizerCostModel::NetworkCost OptimizerCostModel::estimateNetworkTransfer(
    size_t dataSize,
    size_t numHops) const {
    
    NetworkCost cost;
    cost.dataSize = dataSize;
    cost.numHops = numHops > 0 ? numHops : constants_.defaultHops;
    
    // Transfer cost: data size / bandwidth (convert to milliseconds)
    double dataSizeMB = static_cast<double>(dataSize) / (1024.0 * 1024.0);
    const double safeBandwidth = (std::isfinite(constants_.networkBandwidth) &&
                                  constants_.networkBandwidth > 0.0)
                                     ? constants_.networkBandwidth
                                     : 1.0;
    cost.transferCost = (dataSizeMB / safeBandwidth) * 1000.0;
    
    // Latency cost: number of network hops
    const double safeLatency = std::isfinite(constants_.networkLatency)
                                   ? std::max(constants_.networkLatency, 0.0)
                                   : 0.0;
    cost.latencyCost = static_cast<double>(cost.numHops) * safeLatency;
    
    cost.totalCost = cost.transferCost + cost.latencyCost;
    
    return cost;
}

// =============================
// Selectivity Estimation
// =============================

double OptimizerCostModel::estimateSelectivity(
    [[maybe_unused]] const std::string& predicate,
    const ColumnStatistics& columnStats) const {
    
    // Simple heuristic-based selectivity estimation
    
    if (columnStats.distinctValues == 0) {
        return 0.1;  // Default 10% selectivity
    }
    
    // For equality predicates: 1 / distinctValues
    // This assumes uniform distribution
    double selectivity = 1.0 / static_cast<double>(columnStats.distinctValues);
    
    // Adjust for null fraction
    const double nullFraction = clampUnitInterval(columnStats.nullFraction);
    selectivity *= (1.0 - nullFraction);
    
    // Clamp between reasonable bounds
    return std::max(0.001, std::min(1.0, selectivity));
}

double OptimizerCostModel::estimateJoinSelectivity(
    const ColumnStatistics& leftCol,
    const ColumnStatistics& rightCol) const {
    
    // Estimate join selectivity based on distinct values
    // Using the formula: 1 / max(NDV_left, NDV_right)
    
    size_t maxDistinct = std::max(leftCol.distinctValues, rightCol.distinctValues);
    
    if (maxDistinct == 0) {
        return 0.1;  // Default selectivity
    }
    
    double selectivity = 1.0 / static_cast<double>(maxDistinct);
    
    // Adjust for null fractions (nulls don't match)
    const double leftNull = clampUnitInterval(leftCol.nullFraction);
    const double rightNull = clampUnitInterval(rightCol.nullFraction);
    double nonNullFraction = (1.0 - leftNull) * (1.0 - rightNull);
    selectivity *= nonNullFraction;
    
    return std::max(0.0001, std::min(1.0, selectivity));
}

// =============================
// Cost Calibration
// =============================

void OptimizerCostModel::calibrateCosts(
    const std::map<std::string, double>& measurements) {
    
    // Adjust cost constants based on actual measurements
    for (const auto& pair : measurements) {
        updateConstant(pair.first, pair.second);
    }
}

void OptimizerCostModel::updateConstant(const std::string& name, double value) {
    if (!std::isfinite(value)) {
        return;
    }
    if (name == "cpuCostPerRow") {
        constants_.cpuCostPerRow = std::max(value, 0.0);
    } else if (name == "cpuCostPerPredicate") {
        constants_.cpuCostPerPredicate = std::max(value, 0.0);
    } else if (name == "cpuCostPerHash") {
        constants_.cpuCostPerHash = std::max(value, 0.0);
    } else if (name == "cpuCostPerSort") {
        constants_.cpuCostPerSort = std::max(value, 0.0);
    } else if (name == "pageReadCost") {
        constants_.pageReadCost = std::max(value, 0.0);
    } else if (name == "randomPageReadCost") {
        constants_.randomPageReadCost = std::max(value, 0.0);
    } else if (name == "pageWriteCost") {
        constants_.pageWriteCost = std::max(value, 0.0);
    } else if (name == "networkBandwidth") {
        constants_.networkBandwidth = std::max(value, 1e-9);
    } else if (name == "networkLatency") {
        constants_.networkLatency = std::max(value, 0.0);
    } else if (name == "gpu_row_threshold_low") {
        constants_.gpu_row_threshold_low = static_cast<size_t>(std::max(value, 0.0));
    } else if (name == "gpu_row_threshold_high") {
        constants_.gpu_row_threshold_high = static_cast<size_t>(std::max(value, 0.0));
    } else if (name == "vram_safety_factor") {
        constants_.vram_safety_factor = std::max(value, 0.1);
    } else if (name == "cpu_batch_thread_low") {
        constants_.cpu_batch_thread_low = static_cast<size_t>(std::max(value, 1.0));
    } else if (name == "cpu_batch_thread_high") {
        constants_.cpu_batch_thread_high = static_cast<size_t>(std::max(value, 0.0));
    } else if (name == "msgpack_row_threshold") {
        constants_.msgpack_row_threshold = static_cast<size_t>(std::max(value, 0.0));
    }
}

// =============================
// Serialization Strategy Advisor
// =============================

OptimizerCostModel::SerializationAdvice
OptimizerCostModel::adviseSerializationStrategy(
        size_t       estimated_row_count,
        size_t       avg_row_bytes,
        bool         gpu_available,
        size_t       vram_free_bytes,
        WorkloadType workload) const {

    using Format        = SerializationAdvice::Format;
    using ExecutionPath = SerializationAdvice::ExecutionPath;

    SerializationAdvice advice;

    // Effective CPU thread count for batch paths
    const size_t hw_threads_raw = static_cast<size_t>(std::thread::hardware_concurrency());
    const size_t hw_threads = std::max<size_t>(hw_threads_raw, 1);
    const size_t threads_low = std::max<size_t>(constants_.cpu_batch_thread_low, 1);
    const size_t threads_high = (constants_.cpu_batch_thread_high > 0)
                                    ? constants_.cpu_batch_thread_high
                                    : hw_threads;

    // --- Override rules for specific workload classes ---

    if (workload == WorkloadType::CDC_STREAM) {
        // Change-data-capture: schema-versioned binary to keep per-event overhead low
        advice.wire_format              = Format::SF_BINARY_CUSTOM;
        advice.exec_path                = ExecutionPath::CPU_THREADED_BATCH;
        advice.recommended_batch_size   = 256;
        advice.recommended_thread_count = threads_low;
        advice.use_vram_pinned_memory   = false;
        advice.rationale                = "CDC_STREAM → BINARY_CUSTOM/CPU_THREADED_BATCH";
        return advice;
    }

    if (workload == WorkloadType::CACHE_REPL) {
        // Internal cache replication uses Protobuf for compact, schema-safe encoding
        advice.wire_format              = Format::SF_PROTOBUF_WIRE;
        advice.exec_path                = ExecutionPath::CPU_THREADED_BATCH;
        advice.recommended_batch_size   = 512;
        advice.recommended_thread_count = threads_low;
        advice.use_vram_pinned_memory   = false;
        advice.rationale                = "CACHE_REPL → PROTOBUF/CPU_THREADED_BATCH";
        return advice;
    }

    if (workload == WorkloadType::DOCUMENT_CRUD ||
        estimated_row_count < constants_.msgpack_row_threshold) {
        // Small payload or simple CRUD: JSON + single-threaded path is cheapest
        advice.wire_format              = Format::SF_JSON_TEXT;
        advice.exec_path                = ExecutionPath::CPU_SINGLE;
        advice.recommended_batch_size   = estimated_row_count > 0 ? estimated_row_count : 1;
        advice.recommended_thread_count = 1;
        advice.use_vram_pinned_memory   = false;
        advice.rationale                = "row_count<" +
            std::to_string(constants_.msgpack_row_threshold) + " or DOCUMENT_CRUD → JSON_TEXT/CPU_SINGLE";
        return advice;
    }

    // Below the GPU threshold: binary format + multi-threaded CPU
    if (estimated_row_count < constants_.gpu_row_threshold_low) {
        advice.wire_format              = Format::SF_MSGPACK_CBOR;
        advice.exec_path                = ExecutionPath::CPU_THREADED_BATCH;
        advice.recommended_batch_size   = 1024;
        advice.recommended_thread_count = constants_.cpu_batch_thread_low;
        advice.use_vram_pinned_memory   = false;
        advice.rationale                = std::to_string(constants_.msgpack_row_threshold) +
            "≤row_count<" + std::to_string(constants_.gpu_row_threshold_low) +
            " → MSGPACK_CBOR/CPU_THREADED_BATCH(" +
            std::to_string(constants_.cpu_batch_thread_low) + " threads)";
        return advice;
    }

    // At or above the GPU threshold — decide CPU vs GPU
    const size_t payload_bytes = estimated_row_count * (avg_row_bytes > 0 ? avg_row_bytes : 256);
    const size_t required_vram =
        static_cast<size_t>(static_cast<double>(payload_bytes) * constants_.vram_safety_factor);

    const bool vram_fits = gpu_available && (vram_free_bytes >= required_vram);

    if (vram_fits) {
        advice.wire_format              = Format::SF_ARROW_IPC;
        advice.exec_path                = ExecutionPath::GPU_VRAM;
        advice.recommended_batch_size   = 8192;
        advice.recommended_thread_count = 1;   // GPU handles internal parallelism
        advice.use_vram_pinned_memory   = true;
        advice.rationale                = "row_count=" + std::to_string(estimated_row_count) +
            "≥" + std::to_string(constants_.gpu_row_threshold_low) +
            " GPU+VRAM_OK → ARROW_IPC/GPU_VRAM";
    } else {
        advice.wire_format              = Format::SF_ARROW_IPC;
        advice.exec_path                = ExecutionPath::CPU_THREADED_BATCH;
        advice.recommended_batch_size   = 4096;
        advice.recommended_thread_count = threads_high;
        advice.use_vram_pinned_memory   = false;
        if (!gpu_available) {
            advice.rationale = "row_count=" + std::to_string(estimated_row_count) +
                "≥" + std::to_string(constants_.gpu_row_threshold_low) +
                " no_gpu → ARROW_IPC/CPU_THREADED_BATCH(" + std::to_string(threads_high) + ")";
        } else {
            advice.rationale = "row_count=" + std::to_string(estimated_row_count) +
                "≥" + std::to_string(constants_.gpu_row_threshold_low) +
                " VRAM_insufficient(need=" + std::to_string(required_vram) +
                ",free=" + std::to_string(vram_free_bytes) +
                ") → ARROW_IPC/CPU_THREADED_BATCH(" + std::to_string(threads_high) + ")";
        }
    }
    return advice;
}

// =============================
// StatisticsManager Implementation
// =============================

int64_t StatisticsManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

void StatisticsManager::collectTableStatistics(const std::string& tableName) {
    if (table_scan_provider_) {
        // Provider injected: call it to get live statistics from the storage engine.
        OptimizerCostModel::TableStatistics stats = (*table_scan_provider_)(tableName);
        stats.tableName   = tableName;
        stats.lastUpdated = getCurrentTimestamp();
        stats.isStale     = false;
        tableStats_[tableName] = stats;
        return;
    }
    // PERMANENT FALLBACK NOTE (StatisticsManager table scan — no provider injected):
    // Purpose: Creates an empty placeholder entry so that getTableStatistics()
    //   always returns a well-initialised struct for a known table, even before
    //   real stats are injected.
    // Activation: No TableScanProvider has been set via setTableScanProvider().
    // Production Delta: A production implementation queries the storage
    //   engine for live row count, page count, and average row size.  The
    //   actual stats must be injected via setTableScanProvider() (e.g. from
    //   a RocksDB property reader) or via updateTableStatistics().
    // Note: call setTableScanProvider() with a storage-engine scan function
    //   once StorageEngine is injectable into StatisticsManager.
    OptimizerCostModel::TableStatistics stats;
    stats.tableName   = tableName;
    stats.rowCount    = 0;
    stats.pageCount   = 0;
    stats.avgRowSize  = 0.0;
    stats.isStale     = false;
    stats.lastUpdated = getCurrentTimestamp();
    
    tableStats_[tableName] = stats;
}

void StatisticsManager::collectColumnStatistics(
    const std::string& tableName,
    const std::string& columnName) {
    
    if (column_scan_provider_) {
        // Provider injected: call it to get live column statistics.
        OptimizerCostModel::ColumnStatistics stats =
            (*column_scan_provider_)(tableName, columnName);
        stats.columnName = columnName;
        columnStats_[tableName][columnName] = stats;
        return;
    }
    // PERMANENT FALLBACK NOTE (StatisticsManager column scan — no provider injected):
    // Purpose: Creates a zero-initialised column stats entry so downstream
    //   callers always get a valid struct (distinctValues=0 → worst-case
    //   selectivity assumption).
    // Activation: No ColumnScanProvider has been set via setColumnScanProvider().
    // Production Delta: Should scan existing index or sample storage to derive
    //   distinctValues, nullFraction, min/max, and histogram.  Wire via
    //   setColumnScanProvider() once the index subsystem exposes a stats API.
    OptimizerCostModel::ColumnStatistics stats;
    stats.columnName    = columnName;
    stats.distinctValues = 0;
    stats.nullFraction   = 0.0;
    
    columnStats_[tableName][columnName] = stats;
}

void StatisticsManager::collectIndexStatistics(const std::string& indexName) {
    if (index_scan_provider_) {
        // Provider injected: call it to get live index statistics.
        OptimizerCostModel::IndexStatistics stats = (*index_scan_provider_)(indexName);
        stats.indexName = indexName;
        indexStats_[indexName] = stats;
        return;
    }
    // PERMANENT FALLBACK NOTE (StatisticsManager index scan — no provider injected):
    // Purpose: Registers a default btree-shaped index entry so that
    //   estimateIndexScan() always has a valid struct to work with.
    // Activation: No IndexScanProvider has been set via setIndexScanProvider().
    // Production Delta: Should query the index subsystem for actual entry
    //   count, tree depth, and selectivity histogram.  Wire via
    //   setIndexScanProvider() once the index subsystem exposes a stats API.
    OptimizerCostModel::IndexStatistics stats;
    stats.indexName  = indexName;
    stats.indexType  = "btree";
    stats.entryCount = 0;
    stats.levels     = 3;  // Default tree depth
    stats.selectivity = 1.0;
    
    indexStats_[indexName] = stats;
}

void StatisticsManager::refreshAllStatistics() {
    // F-023: Improvement over the old implementation that called
    // collectTableStatistics() (which zeros out all counts).  Now:
    //  • If a row_count_provider_ is registered, use it to populate real
    //    approximate row counts for each tracked table.
    //  • If no provider is registered, retain existing stats as-is rather
    //    than destroying them.  The only time an entry is zeroed is at
    //    first registration via collectTableStatistics().
    int64_t now = getCurrentTimestamp();
    for (auto& [tableName, stats] : tableStats_) {
        if (table_scan_provider_) {
            auto live = (*table_scan_provider_)(tableName);
            if (live.rowCount >= 0) {
                stats.rowCount = live.rowCount;
            }
        }
        // Mark as freshly updated so areStatisticsStale() returns false.
        stats.lastUpdated = now;
        stats.isStale     = false;
    }
}

void StatisticsManager::refreshStaleStatistics() {
    constexpr int64_t kStaleThresholdSeconds = 3600;  // 1 hour
    int64_t now = getCurrentTimestamp();
    for (auto& [tableName, stats] : tableStats_) {
        if (!areStatisticsStale(tableName, kStaleThresholdSeconds)) continue;
        if (table_scan_provider_) {
            auto live = (*table_scan_provider_)(tableName);
            if (live.rowCount >= 0) {
                stats.rowCount = live.rowCount;
            }
        }
        stats.lastUpdated = now;
        stats.isStale     = false;
    }
}

OptimizerCostModel::TableStatistics StatisticsManager::getTableStatistics(
    const std::string& tableName) const {
    
    auto it = tableStats_.find(tableName);
    if (it != tableStats_.end()) {
        return it->second;
    }
    
    // Return default statistics if not found
    OptimizerCostModel::TableStatistics stats;
    stats.tableName = tableName;
    return stats;
}

OptimizerCostModel::ColumnStatistics StatisticsManager::getColumnStatistics(
    const std::string& tableName,
    const std::string& columnName) const {
    
    auto tableIt = columnStats_.find(tableName);
    if (tableIt != columnStats_.end()) {
        auto colIt = tableIt->second.find(columnName);
        if (colIt != tableIt->second.end()) {
            return colIt->second;
        }
    }
    
    // Return default statistics if not found
    OptimizerCostModel::ColumnStatistics stats;
    stats.columnName = columnName;
    return stats;
}

OptimizerCostModel::IndexStatistics StatisticsManager::getIndexStatistics(
    const std::string& indexName) const {
    
    auto it = indexStats_.find(indexName);
    if (it != indexStats_.end()) {
        return it->second;
    }
    
    // Return default statistics if not found
    OptimizerCostModel::IndexStatistics stats;
    stats.indexName = indexName;
    return stats;
}

void StatisticsManager::invalidateStatistics(const std::string& tableName) {
    auto it = tableStats_.find(tableName);
    if (it != tableStats_.end()) {
        it->second.isStale = true;
    }
}

bool StatisticsManager::areStatisticsStale(
    const std::string& tableName,
    int64_t threshold) const {
    
    auto it = tableStats_.find(tableName);
    if (it == tableStats_.end()) {
        return true;  // No statistics means stale
    }
    
    if (it->second.isStale) {
        return true;
    }
    
    int64_t age = getCurrentTimestamp() - it->second.lastUpdated;
    return age > threshold;
}

void StatisticsManager::updateTableStatistics(
    const std::string& tableName,
    const OptimizerCostModel::TableStatistics& stats) {
    
    tableStats_[tableName] = stats;
}

} // namespace themis
