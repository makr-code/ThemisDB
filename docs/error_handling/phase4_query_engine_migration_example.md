# Phase 4 Query Engine: Statistical Aggregator Migration Example

**Date:** 2026-01-20  
**Migration Type:** `nullptr` → `Result<nlohmann::json>`  
**Module:** Query Engine - Statistical Aggregator

---

## 🎯 Migration Summary

Successfully migrated `StatisticalAggregator` class from returning `nullptr` on error to returning `Result<nlohmann::json>`. This serves as the reference example for migrating the remaining query engine components.

### Changes Made

**Files Modified:**
1. `include/utils/error_registry.h` - Added 4 new query error codes
2. `src/utils/error_registry.cpp` - Registered error codes with metadata
3. `include/query/statistical_aggregator.h` - Function signature updates
4. `src/query/statistical_aggregator.cpp` - Implementation migration (10 nullptr sites)
5. `tests/test_statistical_aggregations.cpp` - Test updates for Result<T> pattern

**Total Migration Points:** 10 nullptr returns → Result<T>

---

## 📝 Error Codes Added

### ERR_QUERY_AGGREGATION_FAILED (6105)

**When:** Statistical aggregation function fails due to invalid input or insufficient data  
**Category:** Query  
**Severity:** Error  

**Causes:**
- Empty value sets
- Invalid parameter ranges (e.g., percentile outside 0-100)
- Insufficient data points (e.g., variance needs ≥2 values, IQR needs ≥4)

**Solutions:**
1. Verify input values are numeric
2. Ensure sufficient data points
3. Check for valid parameter ranges
4. Verify data types are compatible
5. Check for NULL or empty value sets

**Related Codes:**
- `ERR_QUERY_TYPE_MISMATCH (6106)` - For type compatibility issues
- `ERR_QUERY_RESOURCE_EXHAUSTED (6107)` - For resource limits

---

## 📝 Migration Pattern

### Before (Legacy Pattern)

**Function Signature:**
```cpp
static nlohmann::json calculatePercentile(
    std::vector<double> values,
    double percentile
);
```

**Implementation:**
```cpp
nlohmann::json StatisticalAggregator::calculatePercentile(
    std::vector<double> values,
    double percentile
) {
    if (values.empty()) {
        return nullptr;  // ❌ Lost error context
    }
    
    if (percentile < 0.0 || percentile > 100.0) {
        return nullptr;  // ❌ Lost error context
    }
    
    // ... calculation ...
    return result;
}
```

**Call Site (legacy):**
```cpp
auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
if (p50.is_null()) {
    // ❌ Cannot distinguish error types
    EXPECT_TRUE(p50.is_null());
}
```

**Problems:**
- ❌ Callers cannot distinguish between "empty values" vs "invalid percentile"
- ❌ Error context (value count, percentile value) lost at call site
- ❌ Requires nullptr checks everywhere
- ❌ No structured error handling

---

### After (Unified Error Handling)

**Function Signature:**
```cpp
static Result<nlohmann::json> calculatePercentile(
    std::vector<double> values,
    double percentile
);
```

**Implementation:**
```cpp
Result<nlohmann::json> StatisticalAggregator::calculatePercentile(
    std::vector<double> values,
    double percentile
) {
    if (values.empty()) {
        return Err<nlohmann::json>(
            ErrorCode::ERR_QUERY_AGGREGATION_FAILED,
            "Empty value set for percentile calculation"
        );  // ✅ Structured error with context
    }
    
    if (percentile < 0.0 || percentile > 100.0) {
        return Err<nlohmann::json>(
            ErrorCode::ERR_QUERY_AGGREGATION_FAILED,
            fmt::format("Invalid percentile value: {} (must be 0-100)", percentile)
        );  // ✅ Structured error with full context
    }
    
    // ... calculation ...
    return Ok(nlohmann::json(result));  // ✅ Explicit success
}
```

**Call Site (Result pattern):**
```cpp
auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
if (p50.has_value()) {
    // ✅ Success: extract value
    EXPECT_TRUE(doubleEquals(p50->get<double>(), 55.0));
} else {
    // ✅ Error: inspect error code and message
    EXPECT_EQ(p50.error().code(), ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
    spdlog::error("Aggregation failed: {}", p50.error().message());
}
```

**Benefits:**
- ✅ Callers can distinguish error types via error codes
- ✅ Full error context preserved (value count, percentile, etc.)
- ✅ Type-safe error checking (compiler enforced)
- ✅ Consistent error handling pattern across codebase
- ✅ Machine-readable error codes for programmatic handling

---

## 🔍 Test Migration Patterns

### Pattern 1: Success Case

**Before:**
```cpp
TEST_F(StatisticalAggregatorTest, PercentileBasic) {
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    ASSERT_FALSE(p50.is_null());
    EXPECT_TRUE(doubleEquals(p50.get<double>(), 55.0));
}
```

**After:**
```cpp
TEST_F(StatisticalAggregatorTest, PercentileBasic) {
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    ASSERT_TRUE(p50.has_value());
    EXPECT_TRUE(doubleEquals(p50->get<double>(), 55.0));
}
```

---

### Pattern 2: Error Case with Code Validation

**Before:**
```cpp
TEST_F(StatisticalAggregatorTest, PercentileEmpty) {
    std::vector<double> values = {};
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    EXPECT_TRUE(p50.is_null());  // ❌ Cannot check error reason
}
```

**After:**
```cpp
TEST_F(StatisticalAggregatorTest, PercentileEmpty) {
    std::vector<double> values = {};
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    EXPECT_FALSE(p50.has_value());
    EXPECT_EQ(p50.error().code(), ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
    // ✅ Can validate specific error code
}
```

---

### Pattern 3: Chained Functions

**Before:**
```cpp
nlohmann::json calculateStdDev(const std::vector<double>& values) {
    auto variance = calculateVariance(values);
    if (variance.is_null()) {
        return nullptr;  // ❌ Lost original error context
    }
    return std::sqrt(variance.get<double>());
}
```

**After:**
```cpp
Result<nlohmann::json> calculateStdDev(const std::vector<double>& values) {
    auto variance = calculateVariance(values);
    if (!variance) {
        // ✅ Propagate error with original context preserved
        return Err<nlohmann::json>(variance.error().code(), variance.error().context());
    }
    return Ok(nlohmann::json(std::sqrt(variance->get<double>())));
}
```

---

## 📊 Specific Error Messages

### Empty Value Set
```cpp
"Empty value set for percentile calculation"
"Empty value set for range calculation"
"Empty value set for MAD calculation"
"Empty value set for population variance calculation"
```

### Invalid Parameters
```cpp
fmt::format("Invalid percentile value: {} (must be 0-100)", percentile)
```

### Insufficient Data
```cpp
fmt::format("Insufficient data for variance calculation: {} values (need ≥2)", values.size())
fmt::format("Insufficient data for IQR calculation: {} values (need ≥4)", values.size())
```

---

## 📈 Migration Checklist

For each function to migrate:

- [ ] Update function signature in header to `Result<T>`
- [ ] Add `#include "utils/expected.h"` in header if not present
- [ ] Replace `return nullptr` with `Err<T>(error_code, context_message)`
- [ ] Replace successful returns with `Ok(value)`
- [ ] Use `fmt::format` for context-rich error messages
- [ ] Update all call sites to check `has_value()` instead of `is_null()`
- [ ] Update call sites to use `->` or `*` to access value
- [ ] Update tests to validate error codes using `error().code()`
- [ ] Add test cases for all error scenarios
- [ ] Build and run tests to verify

---

## ✅ Completion Metrics

### Statistical Aggregator Migration

| Metric | Target | Status |
|--------|--------|--------|
| nullptr Sites Migrated | 10 / 10 | ✅ 100% |
| Functions Updated | 9 / 9 | ✅ 100% |
| Tests Updated | ~15 tests | ✅ Complete |
| New Error Codes | 4 | ✅ Added |
| Call Sites Updated | 0 external | ✅ N/A |

---

## 🔄 Next Steps for Query Engine Migration

### Priority Order

1. **CTE Subquery** (Week 1-2)
   - 8 nullptr returns + 32 Status returns
   - Use `ERR_QUERY_CTE_CYCLE_DETECTED` for cycles
   - Follow same pattern as StatisticalAggregator

2. **AQL Translator** (Week 1-2)
   - 96 Status returns
   - Use `ERR_QUERY_PARSE_FAILED` and `ERR_QUERY_INVALID_SYNTAX`
   - Focus on parse error context (line, column)

3. **Query Engine Core** (Week 2-3)
   - 5 nullptr + 67 Status returns
   - Use `ERR_QUERY_EXECUTION_FAILED`, `ERR_QUERY_TIMEOUT`
   - Critical for query execution path

---

## 📚 Related Documents

- **Foundation:** `phase4_migration_matrix.md` - Overall migration plan
- **Example:** `phase4_week2_getOrCreateColumnFamily_example.md` - RocksDB migration
- **Infrastructure:** `include/utils/expected.h` - Result<T> implementation
- **Error Registry:** `include/utils/error_registry.h` - All error codes

---

**Migration Completed:** 2026-01-20  
**Status:** ✅ SUCCESS  
**Next Module:** CTE Subquery or AQL Translator
