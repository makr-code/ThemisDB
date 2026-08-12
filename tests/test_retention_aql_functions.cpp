/**
 * @file test_retention_aql_functions.cpp
 * @brief Unit tests for AQL retention and scheduling functions defined in
 *        include/query/functions/retention_functions.h
 *
 * Tests cover:
 *  - CoefficientOfVariationFunction (CV)
 *  - VarianceLevelFunction (VARIANCE_LEVEL)
 *  - RetentionResolutionFunction (RETENTION_RESOLUTION)
 *  - DateSubFunction (DATE_SUB)
 *  - EstimateStorageSavingsFunction (ESTIMATE_STORAGE_SAVINGS)
 *  - registerRetentionFunctions: all functions registered in FunctionRegistry
 */

#include <gtest/gtest.h>
#include "query/functions/retention_functions.h"
#include "query/functions/function_registry.h"
#include <cmath>
#include <chrono>

using namespace themis::query::functions;
using nlohmann::json;

// Shared empty context used across all tests
static const FunctionContext kCtx;

// ─── CoefficientOfVariationFunction (CV) ─────────────────────────────────────

TEST(RetentionAqlFunctions, CV_Basic) {
    CoefficientOfVariationFunction fn;
    // CV = (stddev / mean) * 100 = (5 / 100) * 100 = 5.0
    auto result = fn.execute({json(5.0), json(100.0)}, kCtx);
    ASSERT_TRUE(result.is_number());
    EXPECT_NEAR(result.get<double>(), 5.0, 1e-9);
}

TEST(RetentionAqlFunctions, CV_ZeroMeanReturnsZero) {
    CoefficientOfVariationFunction fn;
    auto result = fn.execute({json(2.5), json(0.0)}, kCtx);
    ASSERT_TRUE(result.is_number());
    EXPECT_NEAR(result.get<double>(), 0.0, 1e-9);
}

TEST(RetentionAqlFunctions, CV_NegativeMeanUsesAbsValue) {
    CoefficientOfVariationFunction fn;
    // mean = -50, stddev = 5 -> CV = (5/50)*100 = 10
    auto result = fn.execute({json(5.0), json(-50.0)}, kCtx);
    ASSERT_TRUE(result.is_number());
    EXPECT_NEAR(result.get<double>(), 10.0, 1e-9);
}

TEST(RetentionAqlFunctions, CV_NegativeStddevUsesAbsValue) {
    CoefficientOfVariationFunction fn;
    // stddev = -2.5, mean = 50 -> CV = (2.5/50)*100 = 5
    auto result = fn.execute({json(-2.5), json(50.0)}, kCtx);
    ASSERT_TRUE(result.is_number());
    EXPECT_NEAR(result.get<double>(), 5.0, 1e-9);
}

TEST(RetentionAqlFunctions, CV_SignatureNameIsCV) {
    CoefficientOfVariationFunction fn;
    EXPECT_EQ(fn.signature().name, "CV");
}

// ─── VarianceLevelFunction (VARIANCE_LEVEL) ───────────────────────────────────

TEST(RetentionAqlFunctions, VarianceLevel_LowDefault) {
    VarianceLevelFunction fn;
    auto result = fn.execute({json(3.5)}, kCtx);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "low");
}

TEST(RetentionAqlFunctions, VarianceLevel_MediumDefault) {
    VarianceLevelFunction fn;
    auto result = fn.execute({json(15.0)}, kCtx);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "medium");
}

TEST(RetentionAqlFunctions, VarianceLevel_HighDefault) {
    VarianceLevelFunction fn;
    auto result = fn.execute({json(25.0)}, kCtx);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "high");
}

TEST(RetentionAqlFunctions, VarianceLevel_ExactlyAtLowThreshold_IsMedium) {
    // cv == lowThreshold (5.0): not < 5, so "medium"
    VarianceLevelFunction fn;
    auto result = fn.execute({json(5.0)}, kCtx);
    EXPECT_EQ(result.get<std::string>(), "medium");
}

TEST(RetentionAqlFunctions, VarianceLevel_CustomThresholds) {
    VarianceLevelFunction fn;
    // low < 3, medium < 15, high >= 15
    auto low    = fn.execute({json(2.0), json(3.0), json(15.0)}, kCtx);
    auto medium = fn.execute({json(10.0), json(3.0), json(15.0)}, kCtx);
    auto high   = fn.execute({json(20.0), json(3.0), json(15.0)}, kCtx);
    EXPECT_EQ(low.get<std::string>(),    "low");
    EXPECT_EQ(medium.get<std::string>(), "medium");
    EXPECT_EQ(high.get<std::string>(),   "high");
}

TEST(RetentionAqlFunctions, VarianceLevel_SignatureName) {
    EXPECT_EQ(VarianceLevelFunction{}.signature().name, "VARIANCE_LEVEL");
}

// ─── RetentionResolutionFunction (RETENTION_RESOLUTION) ──────────────────────

TEST(RetentionAqlFunctions, RetentionResolution_LowCvReturns1h) {
    RetentionResolutionFunction fn;
    auto result = fn.execute({json(3.5)}, kCtx);
    EXPECT_EQ(result.get<std::string>(), "1h");
}

TEST(RetentionAqlFunctions, RetentionResolution_MediumCvReturns15m) {
    RetentionResolutionFunction fn;
    auto result = fn.execute({json(15.0)}, kCtx);
    EXPECT_EQ(result.get<std::string>(), "15m");
}

TEST(RetentionAqlFunctions, RetentionResolution_HighCvReturns1m) {
    RetentionResolutionFunction fn;
    auto result = fn.execute({json(25.0)}, kCtx);
    EXPECT_EQ(result.get<std::string>(), "1m");
}

TEST(RetentionAqlFunctions, RetentionResolution_CustomThresholds) {
    RetentionResolutionFunction fn;
    // cv=2, lowThr=3, medThr=10 → "1h"
    auto r = fn.execute({json(2.0), json(3.0), json(10.0)}, kCtx);
    EXPECT_EQ(r.get<std::string>(), "1h");
}

TEST(RetentionAqlFunctions, RetentionResolution_SignatureName) {
    EXPECT_EQ(RetentionResolutionFunction{}.signature().name, "RETENTION_RESOLUTION");
}

// ─── DateSubFunction (DATE_SUB) ───────────────────────────────────────────────

TEST(RetentionAqlFunctions, DateSub_Seconds) {
    DateSubFunction fn;
    // 1000 ms - 1 second = 0 ms
    auto result = fn.execute({json(1000LL), json(1LL), json("second")}, kCtx);
    ASSERT_TRUE(result.is_number_integer());
    EXPECT_EQ(result.get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, DateSub_Minutes) {
    DateSubFunction fn;
    // 60000 ms - 1 minute = 0 ms
    auto result = fn.execute({json(60000LL), json(1LL), json("minute")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, DateSub_Hours) {
    DateSubFunction fn;
    int64_t one_hour_ms = 60 * 60 * 1000LL;
    auto result = fn.execute({json(one_hour_ms * 2), json(1LL), json("hour")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), one_hour_ms);
}

TEST(RetentionAqlFunctions, DateSub_Days) {
    DateSubFunction fn;
    int64_t one_day_ms = 24LL * 60 * 60 * 1000;
    auto result = fn.execute({json(one_day_ms * 7), json(7LL), json("day")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, DateSub_Weeks) {
    DateSubFunction fn;
    int64_t one_week_ms = 7LL * 24 * 60 * 60 * 1000;
    auto result = fn.execute({json(one_week_ms * 2), json(1LL), json("week")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), one_week_ms);
}

TEST(RetentionAqlFunctions, DateSub_Months_Approximate) {
    DateSubFunction fn;
    // 1 month ≈ 30 days = 2,592,000,000 ms
    int64_t one_month_ms = 30LL * 24 * 60 * 60 * 1000;
    auto result = fn.execute({json(one_month_ms * 3), json(3LL), json("month")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, DateSub_Years_Approximate) {
    DateSubFunction fn;
    // 1 year ≈ 365 days
    int64_t one_year_ms = 365LL * 24 * 60 * 60 * 1000;
    auto result = fn.execute({json(one_year_ms), json(1LL), json("year")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, DateSub_Milliseconds) {
    DateSubFunction fn;
    auto result = fn.execute({json(500LL), json(250LL), json("ms")}, kCtx);
    EXPECT_EQ(result.get<int64_t>(), 250LL);
}

TEST(RetentionAqlFunctions, DateSub_UnknownUnitThrows) {
    DateSubFunction fn;
    EXPECT_THROW(
        fn.execute({json(1000LL), json(1LL), json("fortnight")}, kCtx),
        std::runtime_error);
}

TEST(RetentionAqlFunctions, DateSub_SignatureName) {
    EXPECT_EQ(DateSubFunction{}.signature().name, "DATE_SUB");
}

// ─── EstimateStorageSavingsFunction (ESTIMATE_STORAGE_SAVINGS) ───────────────

TEST(RetentionAqlFunctions, EstimateStorageSavings_Basic_1sTo1h) {
    EstimateStorageSavingsFunction fn;
    // 1s → 1h: ratio = 3600, 3600 source points
    auto result = fn.execute({json("1s"), json("1h"), json(3600LL)}, kCtx);
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["source_data_points"].get<int64_t>(), 3600LL);
    EXPECT_EQ(result["compression_ratio"].get<int64_t>(), 3600LL);
    EXPECT_EQ(result["target_data_points"].get<int64_t>(), 1LL);
    EXPECT_GT(result["storage_savings_percent"].get<double>(), 0.0);
    EXPECT_GT(result["storage_saved_bytes"].get<int64_t>(), 0LL);
}

TEST(RetentionAqlFunctions, EstimateStorageSavings_SameResolutionNoSavings) {
    EstimateStorageSavingsFunction fn;
    // 1m → 1m: ratio = 1, no compression
    auto result = fn.execute({json("1m"), json("1m"), json(1000LL)}, kCtx);
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["compression_ratio"].get<int64_t>(), 1LL);
    EXPECT_EQ(result["source_data_points"].get<int64_t>(), 1000LL);
    EXPECT_EQ(result["target_data_points"].get<int64_t>(), 1000LL);
}

TEST(RetentionAqlFunctions, EstimateStorageSavings_AllResolutionKeys) {
    EstimateStorageSavingsFunction fn;
    auto result = fn.execute({json("1s"), json("1d"), json(86400LL)}, kCtx);
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("source_resolution"));
    EXPECT_TRUE(result.contains("target_resolution"));
    EXPECT_TRUE(result.contains("source_data_points"));
    EXPECT_TRUE(result.contains("target_data_points"));
    EXPECT_TRUE(result.contains("compression_ratio"));
    EXPECT_TRUE(result.contains("source_storage_bytes"));
    EXPECT_TRUE(result.contains("target_storage_bytes"));
    EXPECT_TRUE(result.contains("storage_saved_bytes"));
    EXPECT_TRUE(result.contains("storage_savings_percent"));
    EXPECT_TRUE(result.contains("storage_saved_mb"));
}

TEST(RetentionAqlFunctions, EstimateStorageSavings_SignatureName) {
    EXPECT_EQ(EstimateStorageSavingsFunction{}.signature().name, "ESTIMATE_STORAGE_SAVINGS");
}

// ─── registerRetentionFunctions: all functions registered ────────────────────

TEST(RetentionAqlFunctions, RegisterRetentionFunctions_AllRegistered) {
    auto& reg = FunctionRegistry::instance();
    registerRetentionFunctions(reg);

    EXPECT_TRUE(reg.hasFunction("CV"));
    EXPECT_TRUE(reg.hasFunction("VARIANCE_LEVEL"));
    EXPECT_TRUE(reg.hasFunction("RETENTION_RESOLUTION"));
    EXPECT_TRUE(reg.hasFunction("DATE_SUB"));
    EXPECT_TRUE(reg.hasFunction("SCHEDULE_TASK"));
    EXPECT_TRUE(reg.hasFunction("LIST_SCHEDULED_TASKS"));
    EXPECT_TRUE(reg.hasFunction("CANCEL_TASK"));
    EXPECT_TRUE(reg.hasFunction("ESTIMATE_STORAGE_SAVINGS"));
}

TEST(RetentionAqlFunctions, RegisterRetentionFunctions_CallViaRegistry) {
    auto& reg = FunctionRegistry::instance();
    registerRetentionFunctions(reg);

    // Call CV(10, 100) → 10.0 via registry
    auto result = reg.call("CV", {json(10.0), json(100.0)}, kCtx);
    ASSERT_TRUE(result.is_number());
    EXPECT_NEAR(result.get<double>(), 10.0, 1e-9);
}

TEST(RetentionAqlFunctions, RegisterRetentionFunctions_ListScheduledTasksReturnsArray) {
    auto& reg = FunctionRegistry::instance();
    registerRetentionFunctions(reg);

    auto result = reg.call("LIST_SCHEDULED_TASKS", {}, kCtx);
    ASSERT_TRUE(result.is_array());
}

TEST(RetentionAqlFunctions, RegisterRetentionFunctions_UnknownFunctionNotRegistered) {
    auto& reg = FunctionRegistry::instance();
    registerRetentionFunctions(reg);
    EXPECT_FALSE(reg.hasFunction("THIS_DOES_NOT_EXIST"));
}

