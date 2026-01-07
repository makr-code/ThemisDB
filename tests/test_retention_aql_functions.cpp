/**
 * @file test_retention_aql_functions.cpp
 * @brief Unit tests for AQL retention functions
 */

#include <gtest/gtest.h>
#include "query/functions/retention_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

class RetentionAQLFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_unique<FunctionRegistry>();
        registerRetentionFunctions(*registry);
    }
    
    std::unique_ptr<FunctionRegistry> registry;
    FunctionContext ctx;
};

// ============================================================================
// Statistical Functions Tests
// ============================================================================

TEST_F(RetentionAQLFunctionsTest, CoefficientOfVariation_LowVariance) {
    auto func = registry->getFunction("CV");
    ASSERT_NE(func, nullptr);
    
    std::vector<json> args = {2.5, 50.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_NEAR(result.get<double>(), 5.0, 0.01);
}

TEST_F(RetentionAQLFunctionsTest, CoefficientOfVariation_HighVariance) {
    auto func = registry->getFunction("CV");
    
    std::vector<json> args = {15.0, 50.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_NEAR(result.get<double>(), 30.0, 0.01);
}

TEST_F(RetentionAQLFunctionsTest, CoefficientOfVariation_ZeroMean) {
    auto func = registry->getFunction("CV");
    
    std::vector<json> args = {5.0, 0.0};
    auto result = func->execute(args, ctx);
    
    // Should handle division by zero gracefully
    EXPECT_EQ(result.get<double>(), 0.0);
}

TEST_F(RetentionAQLFunctionsTest, CoefficientOfVariation_NegativeValues) {
    auto func = registry->getFunction("CV");
    
    std::vector<json> args = {10.0, -50.0};
    auto result = func->execute(args, ctx);
    
    // Should use absolute value
    EXPECT_NEAR(result.get<double>(), 20.0, 0.01);
}

TEST_F(RetentionAQLFunctionsTest, VarianceLevel_Low) {
    auto func = registry->getFunction("VARIANCE_LEVEL");
    
    std::vector<json> args = {3.5};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "low");
}

TEST_F(RetentionAQLFunctionsTest, VarianceLevel_Medium) {
    auto func = registry->getFunction("VARIANCE_LEVEL");
    
    std::vector<json> args = {15.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "medium");
}

TEST_F(RetentionAQLFunctionsTest, VarianceLevel_High) {
    auto func = registry->getFunction("VARIANCE_LEVEL");
    
    std::vector<json> args = {25.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "high");
}

TEST_F(RetentionAQLFunctionsTest, VarianceLevel_CustomThresholds) {
    auto func = registry->getFunction("VARIANCE_LEVEL");
    
    // CV=7 with custom thresholds (low=3, medium=15)
    std::vector<json> args = {7.0, 3.0, 15.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "medium");
}

TEST_F(RetentionAQLFunctionsTest, RetentionResolution_Low) {
    auto func = registry->getFunction("RETENTION_RESOLUTION");
    
    std::vector<json> args = {3.5};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "1h");
}

TEST_F(RetentionAQLFunctionsTest, RetentionResolution_Medium) {
    auto func = registry->getFunction("RETENTION_RESOLUTION");
    
    std::vector<json> args = {15.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "15m");
}

TEST_F(RetentionAQLFunctionsTest, RetentionResolution_High) {
    auto func = registry->getFunction("RETENTION_RESOLUTION");
    
    std::vector<json> args = {25.0};
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result.get<std::string>(), "1m");
}

// ============================================================================
// Date Function Tests
// ============================================================================

TEST_F(RetentionAQLFunctionsTest, DateSub_Years) {
    auto func = registry->getFunction("DATE_SUB");
    
    int64_t now = 1700000000000LL;  // November 2023
    std::vector<json> args = {now, 1, "year"};
    auto result = func->execute(args, ctx);
    
    int64_t expected = now - (365LL * 24 * 60 * 60 * 1000);
    EXPECT_EQ(result.get<int64_t>(), expected);
}

TEST_F(RetentionAQLFunctionsTest, DateSub_Days) {
    auto func = registry->getFunction("DATE_SUB");
    
    int64_t now = 1700000000000LL;
    std::vector<json> args = {now, 7, "days"};
    auto result = func->execute(args, ctx);
    
    int64_t expected = now - (7LL * 24 * 60 * 60 * 1000);
    EXPECT_EQ(result.get<int64_t>(), expected);
}

TEST_F(RetentionAQLFunctionsTest, DateSub_Hours) {
    auto func = registry->getFunction("DATE_SUB");
    
    int64_t now = 1700000000000LL;
    std::vector<json> args = {now, 12, "hour"};
    auto result = func->execute(args, ctx);
    
    int64_t expected = now - (12LL * 60 * 60 * 1000);
    EXPECT_EQ(result.get<int64_t>(), expected);
}

TEST_F(RetentionAQLFunctionsTest, DateSub_PluralUnits) {
    auto func = registry->getFunction("DATE_SUB");
    
    int64_t now = 1700000000000LL;
    std::vector<json> args = {now, 90, "days"};
    auto result = func->execute(args, ctx);
    
    int64_t expected = now - (90LL * 24 * 60 * 60 * 1000);
    EXPECT_EQ(result.get<int64_t>(), expected);
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST_F(RetentionAQLFunctionsTest, EstimateStorageSavings_1sTo1h) {
    auto func = registry->getFunction("ESTIMATE_STORAGE_SAVINGS");
    
    // 1 hour of 1s data
    std::vector<json> args = {"1s", "1h", 3600};
    auto result = func->execute(args, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["source_resolution"].get<std::string>(), "1s");
    EXPECT_EQ(result["target_resolution"].get<std::string>(), "1h");
    EXPECT_EQ(result["source_data_points"].get<int64_t>(), 3600);
    EXPECT_EQ(result["target_data_points"].get<int64_t>(), 1);
    EXPECT_EQ(result["compression_ratio"].get<int64_t>(), 3600);
    
    // Check storage calculations
    int64_t sourceBytes = 3600 * 100;
    int64_t targetBytes = 1 * 150;
    int64_t savedBytes = sourceBytes - targetBytes;
    
    EXPECT_EQ(result["source_storage_bytes"].get<int64_t>(), sourceBytes);
    EXPECT_EQ(result["target_storage_bytes"].get<int64_t>(), targetBytes);
    EXPECT_EQ(result["storage_saved_bytes"].get<int64_t>(), savedBytes);
    EXPECT_NEAR(result["storage_savings_percent"].get<double>(), 99.96, 0.01);
}

TEST_F(RetentionAQLFunctionsTest, EstimateStorageSavings_1sTo15m) {
    auto func = registry->getFunction("ESTIMATE_STORAGE_SAVINGS");
    
    std::vector<json> args = {"1s", "15m", 31536000};  // 1 year
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result["compression_ratio"].get<int64_t>(), 900);
    EXPECT_TRUE(result["storage_savings_percent"].get<double>() > 90.0);
}

TEST_F(RetentionAQLFunctionsTest, EstimateStorageSavings_1sTo1d) {
    auto func = registry->getFunction("ESTIMATE_STORAGE_SAVINGS");
    
    std::vector<json> args = {"1s", "1d", 31536000};  // 1 year
    auto result = func->execute(args, ctx);
    
    EXPECT_EQ(result["compression_ratio"].get<int64_t>(), 86400);
    EXPECT_TRUE(result["storage_savings_percent"].get<double>() > 99.0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(RetentionAQLFunctionsTest, IntegrationTest_AdaptiveDecision) {
    // Simulate adaptive retention decision process
    auto cv_func = registry->getFunction("CV");
    auto level_func = registry->getFunction("VARIANCE_LEVEL");
    auto resolution_func = registry->getFunction("RETENTION_RESOLUTION");
    
    // Scenario 1: Low variance data
    {
        std::vector<json> cv_args = {2.0, 50.0};
        auto cv = cv_func->execute(cv_args, ctx);
        
        std::vector<json> level_args = {cv};
        auto level = level_func->execute(level_args, ctx);
        
        std::vector<json> res_args = {cv};
        auto resolution = resolution_func->execute(res_args, ctx);
        
        EXPECT_NEAR(cv.get<double>(), 4.0, 0.01);
        EXPECT_EQ(level.get<std::string>(), "low");
        EXPECT_EQ(resolution.get<std::string>(), "1h");
    }
    
    // Scenario 2: High variance data
    {
        std::vector<json> cv_args = {12.0, 50.0};
        auto cv = cv_func->execute(cv_args, ctx);
        
        std::vector<json> level_args = {cv};
        auto level = level_func->execute(level_args, ctx);
        
        std::vector<json> res_args = {cv};
        auto resolution = resolution_func->execute(res_args, ctx);
        
        EXPECT_NEAR(cv.get<double>(), 24.0, 0.01);
        EXPECT_EQ(level.get<std::string>(), "high");
        EXPECT_EQ(resolution.get<std::string>(), "1m");
    }
}

TEST_F(RetentionAQLFunctionsTest, IntegrationTest_StorageEstimation) {
    auto savings_func = registry->getFunction("ESTIMATE_STORAGE_SAVINGS");
    
    // Compare different retention strategies
    std::vector<std::string> resolutions = {"1m", "15m", "1h", "1d"};
    int64_t data_points = 31536000;  // 1 year of 1s data
    
    for (const auto& res : resolutions) {
        std::vector<json> args = {"1s", res, data_points};
        auto result = savings_func->execute(args, ctx);
        
        EXPECT_TRUE(result["storage_savings_percent"].get<double>() > 50.0);
        EXPECT_GT(result["storage_saved_mb"].get<int64_t>(), 0);
    }
}

TEST_F(RetentionAQLFunctionsTest, FunctionSignatures) {
    // Verify all functions are properly registered
    EXPECT_NE(registry->getFunction("CV"), nullptr);
    EXPECT_NE(registry->getFunction("VARIANCE_LEVEL"), nullptr);
    EXPECT_NE(registry->getFunction("RETENTION_RESOLUTION"), nullptr);
    EXPECT_NE(registry->getFunction("DATE_SUB"), nullptr);
    EXPECT_NE(registry->getFunction("SCHEDULE_TASK"), nullptr);
    EXPECT_NE(registry->getFunction("LIST_SCHEDULED_TASKS"), nullptr);
    EXPECT_NE(registry->getFunction("CANCEL_TASK"), nullptr);
    EXPECT_NE(registry->getFunction("ESTIMATE_STORAGE_SAVINGS"), nullptr);
}

TEST_F(RetentionAQLFunctionsTest, FunctionCategories) {
    auto cv_func = registry->getFunction("CV");
    auto sig = cv_func->signature();
    EXPECT_EQ(sig.category, "Statistics");
    
    auto date_func = registry->getFunction("DATE_SUB");
    auto date_sig = date_func->signature();
    EXPECT_EQ(date_sig.category, "Date");
    
    auto schedule_func = registry->getFunction("SCHEDULE_TASK");
    auto schedule_sig = schedule_func->signature();
    EXPECT_EQ(schedule_sig.category, "Scheduling");
}

TEST_F(RetentionAQLFunctionsTest, EdgeCases) {
    auto cv_func = registry->getFunction("CV");
    
    // Zero stddev (perfect stability)
    {
        std::vector<json> args = {0.0, 50.0};
        auto result = cv_func->execute(args, ctx);
        EXPECT_EQ(result.get<double>(), 0.0);
    }
    
    // Very large CV
    {
        std::vector<json> args = {100.0, 10.0};
        auto result = cv_func->execute(args, ctx);
        EXPECT_NEAR(result.get<double>(), 1000.0, 0.01);
    }
}
