/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            optimizer_cost_model.cpp                           ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     591                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Query Optimizer Cost Model Implementation

#include "query/optimizer_cost_model.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace themis {

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
    size_t totalSize = rowCount * rowSize;
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
    
    cost.estimatedRows = static_cast<size_t>(leftRows * rightRows * selectivity);
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
    
    cost.estimatedRows = static_cast<size_t>(leftRows * rightRows * selectivity);
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
        size_t totalSize = rowCount * rowSize;
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
    cost.transferCost = (dataSizeMB / constants_.networkBandwidth) * 1000.0;
    
    // Latency cost: number of network hops
    cost.latencyCost = static_cast<double>(cost.numHops) * constants_.networkLatency;
    
    cost.totalCost = cost.transferCost + cost.latencyCost;
    
    return cost;
}

// =============================
// Selectivity Estimation
// =============================

double OptimizerCostModel::estimateSelectivity(
    const std::string& predicate,
    const ColumnStatistics& columnStats) const {
    
    // Simple heuristic-based selectivity estimation
    
    if (columnStats.distinctValues == 0) {
        return 0.1;  // Default 10% selectivity
    }
    
    // For equality predicates: 1 / distinctValues
    // This assumes uniform distribution
    double selectivity = 1.0 / static_cast<double>(columnStats.distinctValues);
    
    // Adjust for null fraction
    selectivity *= (1.0 - columnStats.nullFraction);
    
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
    double nonNullFraction = (1.0 - leftCol.nullFraction) * (1.0 - rightCol.nullFraction);
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
    if (name == "cpuCostPerRow") {
        constants_.cpuCostPerRow = value;
    } else if (name == "cpuCostPerPredicate") {
        constants_.cpuCostPerPredicate = value;
    } else if (name == "cpuCostPerHash") {
        constants_.cpuCostPerHash = value;
    } else if (name == "cpuCostPerSort") {
        constants_.cpuCostPerSort = value;
    } else if (name == "pageReadCost") {
        constants_.pageReadCost = value;
    } else if (name == "randomPageReadCost") {
        constants_.randomPageReadCost = value;
    } else if (name == "pageWriteCost") {
        constants_.pageWriteCost = value;
    } else if (name == "networkBandwidth") {
        constants_.networkBandwidth = value;
    } else if (name == "networkLatency") {
        constants_.networkLatency = value;
    }
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
    // Placeholder implementation
    // In a real system, this would query the storage engine
    OptimizerCostModel::TableStatistics stats;
    stats.tableName = tableName;
    stats.rowCount = 0;
    stats.pageCount = 0;
    stats.avgRowSize = 0.0;
    stats.isStale = false;
    stats.lastUpdated = getCurrentTimestamp();
    
    tableStats_[tableName] = stats;
}

void StatisticsManager::collectColumnStatistics(
    const std::string& tableName,
    const std::string& columnName) {
    
    // Placeholder implementation
    OptimizerCostModel::ColumnStatistics stats;
    stats.columnName = columnName;
    stats.distinctValues = 0;
    stats.nullFraction = 0.0;
    
    columnStats_[tableName][columnName] = stats;
}

void StatisticsManager::collectIndexStatistics(const std::string& indexName) {
    // Placeholder implementation
    OptimizerCostModel::IndexStatistics stats;
    stats.indexName = indexName;
    stats.indexType = "btree";
    stats.entryCount = 0;
    stats.levels = 3;  // Default tree depth
    stats.selectivity = 1.0;
    
    indexStats_[indexName] = stats;
}

void StatisticsManager::refreshAllStatistics() {
    // Refresh statistics for all tables
    for (auto& [tableName, stats] : tableStats_) {
        collectTableStatistics(tableName);
    }
}

void StatisticsManager::refreshStaleStatistics() {
    int64_t threshold = 3600;  // 1 hour in seconds
    
    for (auto& [tableName, stats] : tableStats_) {
        if (areStatisticsStale(tableName, threshold)) {
            collectTableStatistics(tableName);
        }
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
