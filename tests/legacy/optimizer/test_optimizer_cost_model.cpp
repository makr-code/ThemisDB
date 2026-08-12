#include <gtest/gtest.h>
#include "query/optimizer_cost_model.h"

using namespace themis;

// =============================
// Test Fixtures
// =============================

class OptimizerCostModelTest : public ::testing::Test {
protected:
    OptimizerCostModel model_;
    OptimizerCostModel::CostConstants constants_;
    
    void SetUp() override {
        // Use default constants
        constants_ = model_.getConstants();
    }
};

// =============================
// Table Scan Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, TableScanCostBasic) {
    OptimizerCostModel::TableStatistics table;
    table.tableName = "users";
    table.rowCount = 10000;
    table.pageCount = 100;
    table.avgRowSize = 200.0;
    
    auto cost = model_.estimateTableScan(table);
    
    EXPECT_EQ(cost.type, OptimizerCostModel::ScanCost::ScanType::TABLE_SCAN);
    EXPECT_EQ(cost.estimatedRows, 10000);
    EXPECT_EQ(cost.pagesRead, 100);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_GT(cost.ioCost, 0.0);
    EXPECT_GT(cost.totalCost, 0.0);
    EXPECT_DOUBLE_EQ(cost.totalCost, cost.cpuCost + cost.ioCost);
}

TEST_F(OptimizerCostModelTest, TableScanLargerTableHasHigherCost) {
    OptimizerCostModel::TableStatistics smallTable;
    smallTable.rowCount = 1000;
    smallTable.pageCount = 10;
    
    OptimizerCostModel::TableStatistics largeTable;
    largeTable.rowCount = 100000;
    largeTable.pageCount = 1000;
    
    auto smallCost = model_.estimateTableScan(smallTable);
    auto largeCost = model_.estimateTableScan(largeTable);
    
    EXPECT_LT(smallCost.totalCost, largeCost.totalCost);
}

// =============================
// Index Scan Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, IndexScanCostBasic) {
    OptimizerCostModel::TableStatistics table;
    table.tableName = "users";
    table.rowCount = 10000;
    table.pageCount = 100;
    
    OptimizerCostModel::IndexStatistics index;
    index.indexName = "users_id_idx";
    index.indexType = "btree";
    index.entryCount = 10000;
    index.levels = 3;
    
    double selectivity = 0.1;  // 10% of rows
    auto cost = model_.estimateIndexScan(table, index, selectivity);
    
    EXPECT_EQ(cost.type, OptimizerCostModel::ScanCost::ScanType::INDEX_SCAN);
    EXPECT_EQ(cost.estimatedRows, 1000);
    EXPECT_GT(cost.pagesRead, 0);
    EXPECT_GT(cost.totalCost, 0.0);
}

TEST_F(OptimizerCostModelTest, IndexScanBetterThanTableScanForHighSelectivity) {
    OptimizerCostModel::TableStatistics table;
    table.rowCount = 100000;
    table.pageCount = 1000;
    
    OptimizerCostModel::IndexStatistics index;
    index.levels = 4;
    
    double highSelectivity = 0.01;  // 1% of rows
    
    auto tableScanCost = model_.estimateTableScan(table);
    auto indexScanCost = model_.estimateIndexScan(table, index, highSelectivity);
    
    EXPECT_LT(indexScanCost.totalCost, tableScanCost.totalCost);
}

// =============================
// Filter Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, FilterCostBasic) {
    size_t inputRows = 10000;
    std::vector<std::string> predicates = {"age", "status"};
    std::map<std::string, OptimizerCostModel::ColumnStatistics> columnStats;
    
    auto cost = model_.estimateFilter(inputRows, predicates, columnStats);
    
    EXPECT_EQ(cost.inputRows, 10000);
    EXPECT_LT(cost.outputRows, cost.inputRows);
    EXPECT_GT(cost.selectivity, 0.0);
    EXPECT_LE(cost.selectivity, 1.0);
    EXPECT_GT(cost.cpuCost, 0.0);
}

TEST_F(OptimizerCostModelTest, MorePredicatesReduceOutputRows) {
    size_t inputRows = 10000;
    std::map<std::string, OptimizerCostModel::ColumnStatistics> columnStats;
    
    std::vector<std::string> oneFilter = {"age"};
    std::vector<std::string> twoFilters = {"age", "status"};
    
    auto cost1 = model_.estimateFilter(inputRows, oneFilter, columnStats);
    auto cost2 = model_.estimateFilter(inputRows, twoFilters, columnStats);
    
    EXPECT_LE(cost2.outputRows, cost1.outputRows);
    EXPECT_LE(cost2.selectivity, cost1.selectivity);
}

// =============================
// Join Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, NestedLoopJoinCost) {
    size_t leftRows = 1000;
    size_t rightRows = 100;
    double selectivity = 0.01;
    
    auto cost = model_.estimateNestedLoopJoin(leftRows, rightRows, selectivity);
    
    EXPECT_EQ(cost.type, OptimizerCostModel::JoinCost::JoinType::NESTED_LOOP);
    EXPECT_EQ(cost.leftRows, leftRows);
    EXPECT_EQ(cost.rightRows, rightRows);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_GT(cost.estimatedRows, 0);
}

TEST_F(OptimizerCostModelTest, HashJoinCost) {
    size_t leftRows = 1000;
    size_t rightRows = 10000;
    double selectivity = 0.01;
    
    auto cost = model_.estimateHashJoin(leftRows, rightRows, selectivity);
    
    EXPECT_EQ(cost.type, OptimizerCostModel::JoinCost::JoinType::HASH_JOIN);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_GE(cost.memoryCost, 0.0);
}

TEST_F(OptimizerCostModelTest, HashJoinBetterThanNestedLoopForLargeTables) {
    size_t leftRows = 10000;
    size_t rightRows = 10000;
    double selectivity = 0.001;
    
    auto nestedLoopCost = model_.estimateNestedLoopJoin(leftRows, rightRows, selectivity);
    auto hashJoinCost = model_.estimateHashJoin(leftRows, rightRows, selectivity);
    
    EXPECT_LT(hashJoinCost.totalCost, nestedLoopCost.totalCost);
}

TEST_F(OptimizerCostModelTest, SortMergeJoinCost) {
    size_t leftRows = 10000;
    size_t rightRows = 10000;
    double selectivity = 0.01;
    
    auto cost = model_.estimateSortMergeJoin(leftRows, rightRows, selectivity);
    
    EXPECT_EQ(cost.type, OptimizerCostModel::JoinCost::JoinType::SORT_MERGE);
    EXPECT_GT(cost.cpuCost, 0.0);
}

// =============================
// Aggregation Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, AggregationCostBasic) {
    size_t inputRows = 10000;
    size_t estimatedGroups = 100;
    size_t numAggregates = 3;
    
    auto cost = model_.estimateAggregation(inputRows, estimatedGroups, numAggregates);
    
    EXPECT_EQ(cost.inputRows, inputRows);
    EXPECT_EQ(cost.outputRows, estimatedGroups);
    EXPECT_EQ(cost.numGroups, estimatedGroups);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_GE(cost.memoryCost, 0.0);
}

TEST_F(OptimizerCostModelTest, MoreGroupsIncreaseAggregationCost) {
    size_t inputRows = 10000;
    size_t numAggregates = 2;
    
    auto lowGroups = model_.estimateAggregation(inputRows, 10, numAggregates);
    auto highGroups = model_.estimateAggregation(inputRows, 1000, numAggregates);
    
    EXPECT_LT(lowGroups.memoryCost, highGroups.memoryCost);
}

// =============================
// Sort Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, InMemorySortCost) {
    size_t rowCount = 1000;
    size_t rowSize = 100;
    
    auto cost = model_.estimateSort(rowCount, rowSize);
    
    EXPECT_EQ(cost.rowCount, rowCount);
    EXPECT_FALSE(cost.isExternalSort);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_EQ(cost.ioCost, 0.0);
}

TEST_F(OptimizerCostModelTest, ExternalSortForLargeData) {
    // Large enough to trigger external sort
    size_t rowCount = 10000000;
    size_t rowSize = 200;
    
    auto cost = model_.estimateSort(rowCount, rowSize);
    
    EXPECT_TRUE(cost.isExternalSort);
    EXPECT_GT(cost.cpuCost, 0.0);
    EXPECT_GT(cost.ioCost, 0.0);
}

TEST_F(OptimizerCostModelTest, LargerDatasetHasHigherSortCost) {
    size_t rowSize = 100;
    
    auto smallSort = model_.estimateSort(100, rowSize);
    auto largeSort = model_.estimateSort(10000, rowSize);
    
    EXPECT_LT(smallSort.totalCost, largeSort.totalCost);
}

// =============================
// Network Cost Tests
// =============================

TEST_F(OptimizerCostModelTest, NetworkCostBasic) {
    size_t dataSize = 1024 * 1024;  // 1 MB
    size_t numHops = 2;
    
    auto cost = model_.estimateNetworkTransfer(dataSize, numHops);
    
    EXPECT_EQ(cost.dataSize, dataSize);
    EXPECT_EQ(cost.numHops, numHops);
    EXPECT_GT(cost.transferCost, 0.0);
    EXPECT_GT(cost.latencyCost, 0.0);
    EXPECT_GT(cost.totalCost, 0.0);
}

TEST_F(OptimizerCostModelTest, MoreDataIncreaseNetworkCost) {
    size_t numHops = 1;
    
    auto smallTransfer = model_.estimateNetworkTransfer(1024 * 1024, numHops);
    auto largeTransfer = model_.estimateNetworkTransfer(100 * 1024 * 1024, numHops);
    
    EXPECT_LT(smallTransfer.transferCost, largeTransfer.transferCost);
}

TEST_F(OptimizerCostModelTest, MoreHopsIncreaseLatency) {
    size_t dataSize = 1024 * 1024;
    
    auto oneHop = model_.estimateNetworkTransfer(dataSize, 1);
    auto threeHops = model_.estimateNetworkTransfer(dataSize, 3);
    
    EXPECT_LT(oneHop.latencyCost, threeHops.latencyCost);
}

// =============================
// Selectivity Estimation Tests
// =============================

TEST_F(OptimizerCostModelTest, SelectivityEstimationBasic) {
    OptimizerCostModel::ColumnStatistics colStats;
    colStats.columnName = "status";
    colStats.distinctValues = 10;
    colStats.nullFraction = 0.0;
    
    double selectivity = model_.estimateSelectivity("status = 'active'", colStats);
    
    EXPECT_GT(selectivity, 0.0);
    EXPECT_LE(selectivity, 1.0);
}

TEST_F(OptimizerCostModelTest, MoreDistinctValuesLowerSelectivity) {
    OptimizerCostModel::ColumnStatistics lowCardinality;
    lowCardinality.distinctValues = 10;
    lowCardinality.nullFraction = 0.0;
    
    OptimizerCostModel::ColumnStatistics highCardinality;
    highCardinality.distinctValues = 1000;
    highCardinality.nullFraction = 0.0;
    
    double lowSel = model_.estimateSelectivity("col = value", lowCardinality);
    double highSel = model_.estimateSelectivity("col = value", highCardinality);
    
    EXPECT_GT(lowSel, highSel);
}

TEST_F(OptimizerCostModelTest, JoinSelectivityEstimation) {
    OptimizerCostModel::ColumnStatistics leftCol;
    leftCol.distinctValues = 100;
    leftCol.nullFraction = 0.0;
    
    OptimizerCostModel::ColumnStatistics rightCol;
    rightCol.distinctValues = 100;
    rightCol.nullFraction = 0.0;
    
    double selectivity = model_.estimateJoinSelectivity(leftCol, rightCol);
    
    EXPECT_GT(selectivity, 0.0);
    EXPECT_LE(selectivity, 1.0);
}

// =============================
// Cost Calibration Tests
// =============================

TEST_F(OptimizerCostModelTest, CostCalibration) {
    std::map<std::string, double> measurements;
    measurements["cpuCostPerRow"] = 0.02;
    measurements["pageReadCost"] = 2.0;
    
    model_.calibrateCosts(measurements);
    
    auto newConstants = model_.getConstants();
    EXPECT_DOUBLE_EQ(newConstants.cpuCostPerRow, 0.02);
    EXPECT_DOUBLE_EQ(newConstants.pageReadCost, 2.0);
}

TEST_F(OptimizerCostModelTest, UpdateIndividualConstant) {
    model_.updateConstant("networkBandwidth", 500.0);
    
    auto constants = model_.getConstants();
    EXPECT_DOUBLE_EQ(constants.networkBandwidth, 500.0);
}

// =============================
// StatisticsManager Tests
// =============================

TEST(StatisticsManagerTest, CollectTableStatistics) {
    StatisticsManager manager;
    
    manager.collectTableStatistics("users");
    auto stats = manager.getTableStatistics("users");
    
    EXPECT_EQ(stats.tableName, "users");
}

TEST(StatisticsManagerTest, CollectColumnStatistics) {
    StatisticsManager manager;
    
    manager.collectColumnStatistics("users", "age");
    auto stats = manager.getColumnStatistics("users", "age");
    
    EXPECT_EQ(stats.columnName, "age");
}

TEST(StatisticsManagerTest, CollectIndexStatistics) {
    StatisticsManager manager;
    
    manager.collectIndexStatistics("users_id_idx");
    auto stats = manager.getIndexStatistics("users_id_idx");
    
    EXPECT_EQ(stats.indexName, "users_id_idx");
}

TEST(StatisticsManagerTest, InvalidateStatistics) {
    StatisticsManager manager;
    
    manager.collectTableStatistics("users");
    manager.invalidateStatistics("users");
    
    auto stats = manager.getTableStatistics("users");
    EXPECT_TRUE(stats.isStale);
}

TEST(StatisticsManagerTest, CheckStaleStatistics) {
    StatisticsManager manager;
    
    manager.collectTableStatistics("users");
    
    // Statistics should not be stale immediately
    bool isStale = manager.areStatisticsStale("users", 3600);
    EXPECT_FALSE(isStale);
    
    // Non-existent table should be stale
    bool nonExistentStale = manager.areStatisticsStale("nonexistent", 3600);
    EXPECT_TRUE(nonExistentStale);
}

TEST(StatisticsManagerTest, UpdateTableStatistics) {
    StatisticsManager manager;
    
    OptimizerCostModel::TableStatistics stats;
    stats.tableName = "products";
    stats.rowCount = 50000;
    stats.pageCount = 500;
    
    manager.updateTableStatistics("products", stats);
    
    auto retrieved = manager.getTableStatistics("products");
    EXPECT_EQ(retrieved.rowCount, 50000);
    EXPECT_EQ(retrieved.pageCount, 500);
}

// ====================================================
// updateConstant negative-value guard (issue #5177)
// ====================================================

TEST(OptimizerCostModelTest, UpdateConstantNegativeValueClampsToZero) {
    OptimizerCostModel model;
    // A negative value must clamp to 0 rather than wrap to SIZE_MAX (UB).
    model.updateConstant("gpu_row_threshold_low",  -1.0);
    model.updateConstant("gpu_row_threshold_high", -500.0);
    model.updateConstant("cpu_batch_thread_low",   -2.0);
    model.updateConstant("cpu_batch_thread_high",  -3.0);
    model.updateConstant("msgpack_row_threshold",  -99.0);

    const auto& c = model.getConstants();
    EXPECT_EQ(c.gpu_row_threshold_low,  0u);
    EXPECT_EQ(c.gpu_row_threshold_high, 0u);
    EXPECT_EQ(c.cpu_batch_thread_low,   0u);
    EXPECT_EQ(c.cpu_batch_thread_high,  0u);
    EXPECT_EQ(c.msgpack_row_threshold,  0u);
}

TEST(OptimizerCostModelTest, UpdateConstantPositiveValueUnchanged) {
    OptimizerCostModel model;
    model.updateConstant("gpu_row_threshold_low", 10000.0);
    EXPECT_EQ(model.getConstants().gpu_row_threshold_low, 10000u);
}

// ====================================================
// REL-02/REL-03: estimatedRows overflow safety (issue #5177)
// ====================================================

TEST(OptimizerCostModelTest, HashJoinEstimatedRowsNoOverflowOnHugeInputs) {
    OptimizerCostModel model;
    // leftRows * rightRows would wrap around if computed as size_t*size_t.
    const size_t hugeRows = std::numeric_limits<size_t>::max() / 2 + 1;
    // With selectivity=1.0 the product exceeds SIZE_MAX — must clamp to SIZE_MAX.
    auto cost = model.estimateHashJoin(hugeRows, hugeRows, 1.0);
    EXPECT_EQ(cost.estimatedRows, std::numeric_limits<size_t>::max());
}

TEST(OptimizerCostModelTest, SortMergeJoinEstimatedRowsNoOverflowOnHugeInputs) {
    OptimizerCostModel model;
    const size_t hugeRows = std::numeric_limits<size_t>::max() / 2 + 1;
    auto cost = model.estimateSortMergeJoin(hugeRows, hugeRows, 1.0);
    EXPECT_EQ(cost.estimatedRows, std::numeric_limits<size_t>::max());
}

TEST(OptimizerCostModelTest, HashJoinEstimatedRowsNormalSelectivity) {
    OptimizerCostModel model;
    // 1000 * 2000 * 0.01 = 20 — no overflow, result must be accurate.
    auto cost = model.estimateHashJoin(1000, 2000, 0.01);
    EXPECT_EQ(cost.estimatedRows, 20u);
}
