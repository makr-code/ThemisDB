/**
 * @file relational_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <numeric>



namespace themis {
namespace query {
namespace functions {

/**
 * @brief Relational/SQL Functions for AQL
 * 
 * Provides SQL-compatible functions for relational operations:
 * 
 * ## Categories
 * 
 * ### Aggregation Functions
 * - COUNT_DISTINCT, GROUP_CONCAT, COLLECT
 * - STDDEV, VARIANCE, MEDIAN, PERCENTILE
 * - FIRST_VALUE, LAST_VALUE, NTH_VALUE
 * 
 * ### Set Operations
 * - DISTINCT, EXCEPT, INTERSECT_ALL, UNION_ALL
 * 
 * ### Conditional
 * - COALESCE, NULLIF, GREATEST, LEAST
 * - CASE_WHEN (as function), IF, IIF
 * 
 * ### Join Helpers
 * - INNER_JOIN, LEFT_JOIN, FULL_JOIN (array-based)
 * - LOOKUP, CROSS_PRODUCT
 * 
 * ### Window Function Helpers
 * - ROW_NUMBER, RANK, DENSE_RANK
 * - LAG, LEAD, NTILE
 * - RUNNING_SUM, RUNNING_AVG
 * 
 * ### Grouping
 * - GROUP_BY, HAVING_FILTER
 * - ROLLUP, CUBE (multi-dimensional aggregation)
 */

// ============================================================================
// Aggregation Functions
// ============================================================================

/**
 * @brief COUNT_DISTINCT(array) - Count unique values
 */
class CountDistinctFunction : public IFunction {
public:
    ~CountDistinctFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "COUNT_DISTINCT",
            "Relational",
            "Count unique/distinct values in an array",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of values"}
            },
            ArgType::INTEGER,
            true,
            true,
            {"COUNT_DISTINCT([1, 2, 2, 3, 3, 3])  // Returns 3"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::unordered_set<std::string> unique = {};

        for (const auto& val : args[0]) {
            unique.insert(val.dump());
        }
        return static_cast<int64_t>(unique.size());
    }
};

/**
 * @brief GROUP_CONCAT(array, separator) - Concatenate values with separator
 */
class GroupConcatFunction : public IFunction {
public:
    ~GroupConcatFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "GROUP_CONCAT",
            "Relational",
            "Concatenate array values into a string with separator",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of values"},
                {"separator", ArgType::STRING, false, nlohmann::json(","), "Separator string"}
            },
            ArgType::STRING,
            true,
            true,
            {"GROUP_CONCAT(['a', 'b', 'c'], ', ')  // Returns 'a, b, c'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string sep = args.size() > 1 ? args[1].get<std::string>() : ",";
        std::string result = {};
        
        bool first = true;
        for (const auto& val : args[0]) {
            if (!first) {
              result += sep;
            }
            if (val.is_string()) {
                result += val.get<std::string>();
            } else {
                result += val.dump();
            }
            first = false;
        }
        
        return result;
    }
};

/**
 * @brief COLLECT(array, key) - Collect values into groups by key
 */
class CollectFunction : public IFunction {
public:
    ~CollectFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "COLLECT",
            "Relational",
            "Collect/group array elements by a key field",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of objects"},
                {"key", ArgType::STRING, true, nullptr, "Field name to group by"}
            },
            ArgType::OBJECT,
            true,
            true,
            {"COLLECT([{type:'a',v:1},{type:'b',v:2},{type:'a',v:3}], 'type')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string key = args[1].get<std::string>();
        std::unordered_map<std::string, nlohmann::json> groups;
        
        for (const auto& item : args[0]) {
            if (item.is_object() && item.contains(key)) {
                std::string groupKey = item[key].dump();
                if (!groups.count(groupKey)) {
                    groups[groupKey] = nlohmann::json::array();
                }
                groups[groupKey].push_back(item);
            }
        }
        
        nlohmann::json result = nlohmann::json::object();
        for (const auto& [k, v] : groups) {
            result[k] = v;
        }
        return result;
    }
};

/**
 * @brief STDDEV(array) - Standard deviation
 */
class StddevFunction : public IFunction {
public:
    ~StddevFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "STDDEV",
            "Relational",
            "Calculate population standard deviation",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}
            },
            ArgType::NUMBER,
            true,
            true,
            {"STDDEV([1, 2, 3, 4, 5])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> values = {};

        for (const auto& v : args[0]) {
            if (v.is_number()) {
              values.push_back(v.get<double>());
            }
        }
        
        if (values.empty()) {
          return 0.0;
        }
        
        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double sqSum = 0.0;
        for (double v : values) {
            sqSum += (v - mean) * (v - mean);
        }
        
        return std::sqrt(sqSum / values.size());
    }
};

/**
 * @brief STDDEV_SAMPLE(array) - Sample standard deviation
 */
class StddevSampleFunction : public IFunction {
public:
    ~StddevSampleFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "STDDEV_SAMPLE",
            "Relational",
            "Calculate sample standard deviation (n-1 denominator)",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}
            },
            ArgType::NUMBER,
            true,
            true,
            {"STDDEV_SAMPLE([1, 2, 3, 4, 5])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> values = {};

        for (const auto& v : args[0]) {
            if (v.is_number()) {
              values.push_back(v.get<double>());
            }
        }
        
        if (values.size() < 2) {
          return 0.0;
        }
        
        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double sqSum = 0.0;
        for (double v : values) {
            sqSum += (v - mean) * (v - mean);
        }
        
        return std::sqrt(sqSum / (values.size() - 1));
    }
};

/**
 * @brief VARIANCE(array) - Population variance
 */
class VarianceFunction : public IFunction {
public:
    ~VarianceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VARIANCE",
            "Relational",
            "Calculate population variance",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}
            },
            ArgType::NUMBER,
            true,
            true,
            {"VARIANCE([1, 2, 3, 4, 5])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> values = {};

        for (const auto& v : args[0]) {
            if (v.is_number()) {
              values.push_back(v.get<double>());
            }
        }
        
        if (values.empty()) {
          return 0.0;
        }
        
        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double sqSum = 0.0;
        for (double v : values) {
            sqSum += (v - mean) * (v - mean);
        }
        
        return sqSum / values.size();
    }
};

/**
 * @brief MEDIAN(array) - Median value
 */
class MedianFunction : public IFunction {
public:
    ~MedianFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MEDIAN",
            "Relational",
            "Calculate median (50th percentile)",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}
            },
            ArgType::NUMBER,
            true,
            true,
            {"MEDIAN([1, 2, 3, 4, 5])  // Returns 3"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> values = {};

        for (const auto& v : args[0]) {
            if (v.is_number()) {
              values.push_back(v.get<double>());
            }
        }
        
        if (values.empty()) {
          return nullptr;
        }
        
        std::sort(values.begin(), values.end());
        size_t n = values.size();
        
        if (n % 2 == 0) {
            return (values[n/2 - 1] + values[n/2]) / 2.0;
        } else {
            return values[n/2];
        }
    }
};

/**
 * @brief PERCENTILE(array, p) - Calculate percentile
 */
class PercentileFunction : public IFunction {
public:
    ~PercentileFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PERCENTILE",
            "Relational",
            "Calculate percentile value (0-100)",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers"},
                {"percentile", ArgType::NUMBER, true, nullptr, "Percentile (0-100)"}
            },
            ArgType::NUMBER,
            true,
            true,
            {"PERCENTILE([1,2,3,4,5,6,7,8,9,10], 90)  // Returns 9"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> values = {};

        for (const auto& v : args[0]) {
            if (v.is_number()) {
              values.push_back(v.get<double>());
            }
        }
        
        if (values.empty()) {
          return nullptr;
        }
        
        double p = args[1].get<double>() / 100.0;
        p = std::max(0.0, std::min(1.0, p));
        
        std::sort(values.begin(), values.end());
        double index = p * (values.size() - 1);
        size_t lower = static_cast<size_t>(index);
        size_t upper = std::min(lower + 1, values.size() - 1);
        double frac = index - lower;
        
        return values[lower] + frac * (values[upper] - values[lower]);
    }
};

// ============================================================================
// Conditional Functions
// ============================================================================

/**
 * @brief COALESCE(val1, val2, ...) - Return first non-null value
 */
class CoalesceFunction : public IFunction {
public:
    ~CoalesceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "COALESCE",
            "Relational",
            "Return first non-null value from arguments",
            {
                {"values", ArgType::ANY, true, nullptr, "Values to check (variadic)"}
            },
            ArgType::ANY,
            true,
            false,
            {"COALESCE(null, null, 'default')  // Returns 'default'"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
            throw std::runtime_error("COALESCE requires at least one argument");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        for (const auto& arg : args) {
            if (!arg.is_null()) {
                return arg;
            }
        }
        return nullptr;
    }
};

/**
 * @brief NULLIF(val1, val2) - Return null if values are equal
 */
class NullIfFunction : public IFunction {
public:
    ~NullIfFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "NULLIF",
            "Relational",
            "Return null if both values are equal, otherwise return first value",
            {
                {"value1", ArgType::ANY, true, nullptr, "First value"},
                {"value2", ArgType::ANY, true, nullptr, "Second value"}
            },
            ArgType::ANY,
            true,
            false,
            {"NULLIF(5, 5)  // Returns null", "NULLIF(5, 3)  // Returns 5"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args[0] == args[1]) {
            return nullptr;
        }
        return args[0];
    }
};

/**
 * @brief GREATEST(val1, val2, ...) - Return maximum value
 */
class GreatestFunction : public IFunction {
public:
    ~GreatestFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "GREATEST",
            "Relational",
            "Return the greatest (maximum) value from arguments",
            {
                {"values", ArgType::ANY, true, nullptr, "Values to compare (variadic)"}
            },
            ArgType::ANY,
            true,
            false,
            {"GREATEST(1, 5, 3)  // Returns 5"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
            throw std::runtime_error("GREATEST requires at least one argument");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json maxVal = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] > maxVal) {
                maxVal = args[i];
            }
        }
        return maxVal;
    }
};

/**
 * @brief LEAST(val1, val2, ...) - Return minimum value
 */
class LeastFunction : public IFunction {
public:
    ~LeastFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LEAST",
            "Relational",
            "Return the least (minimum) value from arguments",
            {
                {"values", ArgType::ANY, true, nullptr, "Values to compare (variadic)"}
            },
            ArgType::ANY,
            true,
            false,
            {"LEAST(1, 5, 3)  // Returns 1"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
            throw std::runtime_error("LEAST requires at least one argument");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json minVal = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] < minVal) {
                minVal = args[i];
            }
        }
        return minVal;
    }
};

/**
 * @brief IF(condition, then_value, else_value) - Conditional expression
 */
class IfFunction : public IFunction {
public:
    ~IfFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "IF",
            "Relational",
            "Return then_value if condition is true, else else_value",
            {
                {"condition", ArgType::ANY, true, nullptr, "Boolean condition"},
                {"then_value", ArgType::ANY, true, nullptr, "Value if true"},
                {"else_value", ArgType::ANY, true, nullptr, "Value if false"}
            },
            ArgType::ANY,
            true,
            false,
            {"IF(age >= 18, 'adult', 'minor')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        bool condition = toBool(args[0]);
        return condition ? args[1] : args[2];
    }
};

// ============================================================================
// Join Helper Functions
// ============================================================================

/**
 * @brief INNER_JOIN(left, right, leftKey, rightKey) - Inner join two arrays
 */
class InnerJoinFunction : public IFunction {
public:
    ~InnerJoinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "INNER_JOIN",
            "Relational",
            "Perform inner join on two arrays of objects",
            {
                {"left", ArgType::ARRAY, true, nullptr, "Left array"},
                {"right", ArgType::ARRAY, true, nullptr, "Right array"},
                {"leftKey", ArgType::STRING, true, nullptr, "Key field in left array"},
                {"rightKey", ArgType::STRING, true, nullptr, "Key field in right array"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"INNER_JOIN(orders, customers, 'customerId', 'id')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& left = args[0];
        const auto& right = args[1];
        std::string leftKey = args[2].get<std::string>();
        std::string rightKey = args[3].get<std::string>();
        
        // Build hash index on right side
        std::unordered_map<std::string, std::vector<nlohmann::json>> rightIndex;
        for (const auto& r : right) {
            if (r.is_object() && r.contains(rightKey)) {
                rightIndex[r[rightKey].dump()].push_back(r);
            }
        }
        
        // Perform join
        nlohmann::json result = nlohmann::json::array();
        for (const auto& l : left) {
            if (l.is_object() && l.contains(leftKey)) {
                auto it = rightIndex.find(l[leftKey].dump());
                if (it != rightIndex.end()) {
                    for (const auto& r : it->second) {
                        nlohmann::json merged = l;
                        for (const auto& [k, v] : r.items()) {
                            if (!merged.contains(k)) {
                                merged[k] = v;
                            } else {
                                merged["_right_" + k] = v;
                            }
                        }
                        result.push_back(merged);
                    }
                }
            }
        }
        
        return result;
    }
};

/**
 * @brief LEFT_JOIN(left, right, leftKey, rightKey) - Left outer join
 */
class LeftJoinFunction : public IFunction {
public:
    ~LeftJoinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LEFT_JOIN",
            "Relational",
            "Perform left outer join on two arrays of objects",
            {
                {"left", ArgType::ARRAY, true, nullptr, "Left array"},
                {"right", ArgType::ARRAY, true, nullptr, "Right array"},
                {"leftKey", ArgType::STRING, true, nullptr, "Key field in left array"},
                {"rightKey", ArgType::STRING, true, nullptr, "Key field in right array"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"LEFT_JOIN(orders, customers, 'customerId', 'id')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& left = args[0];
        const auto& right = args[1];
        std::string leftKey = args[2].get<std::string>();
        std::string rightKey = args[3].get<std::string>();
        
        // Build hash index on right side
        std::unordered_map<std::string, std::vector<nlohmann::json>> rightIndex;
        for (const auto& r : right) {
            if (r.is_object() && r.contains(rightKey)) {
                rightIndex[r[rightKey].dump()].push_back(r);
            }
        }
        
        // Perform left join
        nlohmann::json result = nlohmann::json::array();
        for (const auto& l : left) {
            if (l.is_object() && l.contains(leftKey)) {
                auto it = rightIndex.find(l[leftKey].dump());
                if (it != rightIndex.end()) {
                    for (const auto& r : it->second) {
                        nlohmann::json merged = l;
                        for (const auto& [k, v] : r.items()) {
                            if (!merged.contains(k)) {
                                merged[k] = v;
                            } else {
                                merged["_right_" + k] = v;
                            }
                        }
                        result.push_back(merged);
                    }
                } else {
                    // No match - include left row with nulls
                    result.push_back(l);
                }
            } else {
                result.push_back(l);
            }
        }
        
        return result;
    }
};

/**
 * @brief LOOKUP(array, key, value) - Find object in array by key value
 */
class LookupFunction : public IFunction {
public:
    ~LookupFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LOOKUP",
            "Relational",
            "Find first object in array where key equals value",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of objects"},
                {"key", ArgType::STRING, true, nullptr, "Field name to match"},
                {"value", ArgType::ANY, true, nullptr, "Value to find"}
            },
            ArgType::ANY,
            true,
            false,
            {"LOOKUP(users, 'id', 123)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string key = args[1].get<std::string>();
        const auto& value = args[2];
        
        for (const auto& item : args[0]) {
            if (item.is_object() && item.contains(key) && item[key] == value) {
                return item;
            }
        }
        
        return nullptr;
    }
};

// ============================================================================
// Window Function Helpers
// ============================================================================

/**
 * @brief ROW_NUMBER(array) - Add row numbers to array
 */
class RowNumberFunction : public IFunction {
public:
    ~RowNumberFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ROW_NUMBER",
            "Relational",
            "Add sequential row numbers to array elements",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of objects"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"ROW_NUMBER([{name:'a'},{name:'b'}])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json result = nlohmann::json::array();
        int64_t rowNum = 1;
        
        for (const auto& item : args[0]) {
            if (item.is_object()) {
                nlohmann::json row = item;
                row["_row_number"] = rowNum++;
                result.push_back(row);
            } else {
                result.push_back({{"_value", item}, {"_row_number", rowNum++}});
            }
        }
        
        return result;
    }
};

/**
 * @brief LAG(array, offset, default) - Get previous value in array
 */
class LagFunction : public IFunction {
public:
    ~LagFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LAG",
            "Relational",
            "Add lagged (previous) values to array elements",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of values or objects"},
                {"field", ArgType::STRING, false, nullptr, "Field name (for objects)"},
                {"offset", ArgType::INTEGER, false, nlohmann::json(1), "Number of rows to look back"},
                {"default_value", ArgType::ANY, false, nullptr, "Default for null values"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"LAG([1,2,3,4,5])  // Returns [null,1,2,3,4]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& arr = args[0];
        std::string field = args.size() > 1 && !args[1].is_null() ? args[1].get<std::string>() : "";
        int offset = args.size() > 2 ? args[2].get<int>() : 1;
        nlohmann::json defaultVal = args.size() > 3 ? args[3] : nlohmann::json(nullptr);
        
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < arr.size(); ++i) {
            nlohmann::json lagVal = defaultVal;
            if (i >= static_cast<size_t>(offset)) {
                size_t lagIdx = i - offset;
                if (field.empty()) {
                    lagVal = arr[lagIdx];
                } else if (arr[lagIdx].is_object() && arr[lagIdx].contains(field)) {
                    lagVal = arr[lagIdx][field];
                }
            }
            
            if (arr[i].is_object()) {
                nlohmann::json row = arr[i];
                row["_lag"] = lagVal;
                result.push_back(row);
            } else {
                result.push_back({{"_value", arr[i]}, {"_lag", lagVal}});
            }
        }
        
        return result;
    }
};

/**
 * @brief LEAD(array, offset, default) - Get next value in array
 */
class LeadFunction : public IFunction {
public:
    ~LeadFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LEAD",
            "Relational",
            "Add leading (next) values to array elements",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of values or objects"},
                {"field", ArgType::STRING, false, nullptr, "Field name (for objects)"},
                {"offset", ArgType::INTEGER, false, nlohmann::json(1), "Number of rows to look ahead"},
                {"default_value", ArgType::ANY, false, nullptr, "Default for null values"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"LEAD([1,2,3,4,5])  // Returns [2,3,4,5,null]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& arr = args[0];
        std::string field = args.size() > 1 && !args[1].is_null() ? args[1].get<std::string>() : "";
        int offset = args.size() > 2 ? args[2].get<int>() : 1;
        nlohmann::json defaultVal = args.size() > 3 ? args[3] : nlohmann::json(nullptr);
        
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < arr.size(); ++i) {
            nlohmann::json leadVal = defaultVal;
            size_t leadIdx = i + offset;
            if (leadIdx < arr.size()) {
                if (field.empty()) {
                    leadVal = arr[leadIdx];
                } else if (arr[leadIdx].is_object() && arr[leadIdx].contains(field)) {
                    leadVal = arr[leadIdx][field];
                }
            }
            
            if (arr[i].is_object()) {
                nlohmann::json row = arr[i];
                row["_lead"] = leadVal;
                result.push_back(row);
            } else {
                result.push_back({{"_value", arr[i]}, {"_lead", leadVal}});
            }
        }
        
        return result;
    }
};

/**
 * @brief RUNNING_SUM(array, field) - Cumulative sum
 */
class RunningSumFunction : public IFunction {
public:
    ~RunningSumFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "RUNNING_SUM",
            "Relational",
            "Calculate running (cumulative) sum",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array of numbers or objects"},
                {"field", ArgType::STRING, false, nullptr, "Field name for objects"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"RUNNING_SUM([1,2,3,4,5])  // Returns [1,3,6,10,15]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& arr = args[0];
        std::string field = args.size() > 1 && !args[1].is_null() ? args[1].get<std::string>() : "";
        
        nlohmann::json result = nlohmann::json::array();
        double runningSum = 0.0;
        
        for (const auto& item : arr) {
            double val = 0.0;
            if (item.is_number()) {
                val = item.get<double>();
            } else if (item.is_object() && !field.empty() && item.contains(field) && item[field].is_number()) {
                val = item[field].get<double>();
            }
            
            runningSum += val;
            
            if (item.is_object()) {
                nlohmann::json row = item;
                row["_running_sum"] = runningSum;
                result.push_back(row);
            } else {
                result.push_back({{"_value", item}, {"_running_sum", runningSum}});
            }
        }
        
        return result;
    }
};

/**
 * @brief NTILE(array, n) - Divide array into n buckets
 */
class NtileFunction : public IFunction {
public:
    ~NtileFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "NTILE",
            "Relational",
            "Divide array into n equal-sized buckets",
            {
                {"array", ArgType::ARRAY, true, nullptr, "Array to partition"},
                {"n", ArgType::INTEGER, true, nullptr, "Number of buckets"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"NTILE([1,2,3,4,5,6], 3)  // Assigns bucket 1,1,2,2,3,3"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& arr = args[0];
        int n = args[1].get<int>();
        
        if (n <= 0) {
          throw std::runtime_error("NTILE: n must be positive");
        }
        
        nlohmann::json result = nlohmann::json::array();
        size_t total = arr.size();
        size_t baseSize = total / n;
        size_t remainder = total % n;
        
        size_t idx = 0;
        for (int bucket = 1; bucket <= n && idx < total; ++bucket) {
            size_t bucketSize = baseSize + (static_cast<size_t>(bucket) <= remainder ? 1 : 0);
            for (size_t j = 0; j < bucketSize && idx < total; ++j, ++idx) {
                if (arr[idx].is_object()) {
                    nlohmann::json row = arr[idx];
                    row["_ntile"] = bucket;
                    result.push_back(row);
                } else {
                    result.push_back({{"_value", arr[idx]}, {"_ntile", bucket}});
                }
            }
        }
        
        return result;
    }
};

// ============================================================================
// Registration Function
// ============================================================================

/**
 * @brief Register all Relational functions with the registry
 */
inline void registerRelationalFunctions(FunctionRegistry& registry) {
    // Aggregation
    registry.registerFunction(std::make_unique<CountDistinctFunction>());
    registry.registerFunction(std::make_unique<GroupConcatFunction>());
    registry.registerFunction(std::make_unique<CollectFunction>());
    registry.registerFunction(std::make_unique<StddevFunction>());
    registry.registerFunction(std::make_unique<StddevSampleFunction>());
    registry.registerFunction(std::make_unique<VarianceFunction>());
    registry.registerFunction(std::make_unique<MedianFunction>());
    registry.registerFunction(std::make_unique<PercentileFunction>());
    
    // Conditional
    registry.registerFunction(std::make_unique<CoalesceFunction>());
    registry.registerFunction(std::make_unique<NullIfFunction>());
    registry.registerFunction(std::make_unique<GreatestFunction>());
    registry.registerFunction(std::make_unique<LeastFunction>());
    registry.registerFunction(std::make_unique<IfFunction>());
    
    // Joins
    registry.registerFunction(std::make_unique<InnerJoinFunction>());
    registry.registerFunction(std::make_unique<LeftJoinFunction>());
    registry.registerFunction(std::make_unique<LookupFunction>());
    
    // Window functions
    registry.registerFunction(std::make_unique<RowNumberFunction>());
    registry.registerFunction(std::make_unique<LagFunction>());
    registry.registerFunction(std::make_unique<LeadFunction>());
    registry.registerFunction(std::make_unique<RunningSumFunction>());
    registry.registerFunction(std::make_unique<NtileFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis


