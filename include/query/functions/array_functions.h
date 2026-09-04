/**
 * @file array_functions.h
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
#include <set>
#include <numeric>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Array Functions
// ============================================================================

/**
 * @brief FIRST(array) - Get first element
 */
class FirstFunction : public IFunction {
public:
    ~FirstFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FIRST",
            .category = "Array",
            .description = "Returns the first element of an array",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {R"(FIRST([1, 2, 3]) // 1)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        if (arr.empty()) {
          return nullptr;
        }
        return arr[0];
    }
};

/**
 * @brief LAST(array) - Get last element
 */
class LastFunction : public IFunction {
public:
    ~LastFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LAST",
            .category = "Array",
            .description = "Returns the last element of an array",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {R"(LAST([1, 2, 3]) // 3)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        if (arr.empty()) {
          return nullptr;
        }
        return arr[arr.size() - 1];
    }
};

/**
 * @brief NTH(array, index) - Get element at index
 */
class NthFunction : public IFunction {
public:
    ~NthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "NTH",
            .category = "Array",
            .description = "Returns the element at a specific index (0-based)",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"index", ArgType::INTEGER, true, nullptr, "Index (0-based)"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {R"(NTH([1, 2, 3], 1) // 2)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        int64_t idx = args[1].get<int64_t>();
        
        if (idx < 0) {
          idx = static_cast<int64_t>(arr.size()) + idx;
        }
        if (idx < 0 || idx >= static_cast<int64_t>(arr.size())) {
          return nullptr;
        }
        
        return arr[static_cast<size_t>(idx)];
    }
};

/**
 * @brief PUSH(array, value [, unique]) - Append element
 */
class PushFunction : public IFunction {
public:
    ~PushFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "PUSH",
            .category = "Array",
            .description = "Appends a value to an array and returns the new array",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"value", ArgType::ANY, true, nullptr, "Value to append"},
                {"unique", ArgType::BOOLEAN, false, false, "Only add if not already present"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(PUSH([1, 2], 3) // [1, 2, 3])",
                R"(PUSH([1, 2], 2, true) // [1, 2])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = args[0];
        const auto& value = args[1];
        bool unique = args.size() > 2 ? toBool(args[2]) : false;
        
        if (unique) {
            for (const auto& elem : result) {
                if (elem == value) {
                  return result;
                }
            }
        }
        
        result.push_back(value);
        return result;
    }
};

/**
 * @brief POP(array) - Remove last element
 */
class PopFunction : public IFunction {
public:
    ~PopFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "POP",
            .category = "Array",
            .description = "Returns the array with the last element removed",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(POP([1, 2, 3]) // [1, 2])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = args[0];
        if (!result.empty()) {
            result.erase(result.size() - 1);
        }
        return result;
    }
};

/**
 * @brief SHIFT(array) - Remove first element
 */
class ShiftFunction : public IFunction {
public:
    ~ShiftFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SHIFT",
            .category = "Array",
            .description = "Returns the array with the first element removed",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(SHIFT([1, 2, 3]) // [2, 3])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = args[0];
        if (!result.empty()) {
            result.erase(0);
        }
        return result;
    }
};

/**
 * @brief UNSHIFT(array, value) - Prepend element
 */
class UnshiftFunction : public IFunction {
public:
    ~UnshiftFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNSHIFT",
            .category = "Array",
            .description = "Prepends a value to an array and returns the new array",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"value", ArgType::ANY, true, nullptr, "Value to prepend"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(UNSHIFT([2, 3], 1) // [1, 2, 3])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        result.push_back(args[1]);
        for (const auto& elem : args[0]) {
            result.push_back(elem);
        }
        return result;
    }
};

/**
 * @brief SLICE(array, start [, end]) - Get subarray
 */
class SliceFunction : public IFunction {
public:
    ~SliceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SLICE",
            .category = "Array",
            .description = "Returns a portion of an array",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"start", ArgType::INTEGER, true, nullptr, "Start index (inclusive)"},
                {"end", ArgType::INTEGER, false, nullptr, "End index (exclusive)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(SLICE([1, 2, 3, 4], 1, 3) // [2, 3])",
                R"(SLICE([1, 2, 3, 4], 2) // [3, 4])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        int64_t start = args[1].get<int64_t>();
        int64_t end = args.size() > 2 ? args[2].get<int64_t>() : static_cast<int64_t>(arr.size());
        
        // Handle negative indices
        int64_t size = static_cast<int64_t>(arr.size());
        if (start < 0) start = std::max(int64_t{0}, size + start);
        if (end < 0) {
          end = size + end;
        }
        start = std::max(int64_t{0}, std::min(start, size));
        end = std::max(int64_t{0}, std::min(end, size));
        
        nlohmann::json result = nlohmann::json::array();
        for (int64_t i = start; i < end; i++) {
            result.push_back(arr[static_cast<size_t>(i)]);
        }
        return result;
    }
};

/**
 * @brief FLATTEN(array [, depth]) - Flatten nested arrays
 */
class FlattenFunction : public IFunction {
public:
    ~FlattenFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FLATTEN",
            .category = "Array",
            .description = "Flattens nested arrays to a single level",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"depth", ArgType::INTEGER, false, 1, "Maximum depth to flatten"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(FLATTEN([[1, 2], [3, 4]]) // [1, 2, 3, 4])",
                R"(FLATTEN([[[1]], [[2]]], 2) // [1, 2])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t depth = args.size() > 1 ? args[1].get<int64_t>() : 1;
        return flattenRecursive(args[0], depth);
    }
    
private:
    static nlohmann::json flattenRecursive(const nlohmann::json& arr, int64_t depth) {
        nlohmann::json result = nlohmann::json::array();
        for (const auto& elem : arr) {
            if (elem.is_array() && depth > 0) {
                auto sub = flattenRecursive(elem, depth - 1);
                for (const auto& subElem : sub) {
                    result.push_back(subElem);
                }
            } else {
                result.push_back(elem);
            }
        }
        return result;
    }
};

/**
 * @brief UNIQUE(array) - Remove duplicates
 */
class UniqueFunction : public IFunction {
public:
    ~UniqueFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNIQUE",
            .category = "Array",
            .description = "Returns an array with duplicate values removed",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(UNIQUE([1, 2, 1, 3, 2]) // [1, 2, 3])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        std::set<std::string> seen;
        
        for (const auto& elem : args[0]) {
            std::string key = elem.dump();
            if (seen.find(key) == seen.end()) {
                seen.insert(key);
                result.push_back(elem);
            }
        }
        return result;
    }
};

/**
 * @brief SORTED(array [, direction]) - Sort array
 */
class SortedFunction : public IFunction {
public:
    ~SortedFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SORTED",
            .category = "Array",
            .description = "Returns a sorted copy of the array",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"direction", ArgType::STRING, false, "ASC", "Sort direction: ASC or DESC"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(SORTED([3, 1, 2]) // [1, 2, 3])",
                R"(SORTED([1, 2, 3], "DESC") // [3, 2, 1])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::vector<nlohmann::json> vec = {};

        for (const auto& elem : args[0]) {
            vec.push_back(elem);
        }
        
        bool desc = args.size() > 1 && 
                    (args[1].get<std::string>() == "DESC" || args[1].get<std::string>() == "desc");
        
        std::sort(vec.begin(), vec.end(), [desc](const auto& a, const auto& b) {
            // Compare by dump for consistent ordering
            if (desc) {
              return a.dump() > b.dump();
            }
            return a.dump() < b.dump();
        });
        
        nlohmann::json result = nlohmann::json::array();
        for (const auto& elem : vec) {
            result.push_back(elem);
        }
        return result;
    }
};

/**
 * @brief REVERSE(array) - Reverse array
 */
class ReverseArrayFunction : public IFunction {
public:
    ~ReverseArrayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REVERSE_ARRAY",
            .category = "Array",
            .description = "Returns a reversed copy of the array",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Input array"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(REVERSE_ARRAY([1, 2, 3]) // [3, 2, 1])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        const auto& arr = args[0];
        for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
            result.push_back(*it);
        }
        return result;
    }
};

/**
 * @brief UNION(arr1, arr2, ...) - Union of arrays
 */
class UnionFunction : public IFunction {
public:
    ~UnionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNION",
            .category = "Array",
            .description = "Returns the union of all arrays (unique values)",
            .arguments = {{"arrays", ArgType::ARRAY, true, nullptr, "Arrays to combine (variadic)"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(UNION([1, 2], [2, 3]) // [1, 2, 3])"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
          throw std::runtime_error("UNION requires at least 1 array");
        }
        for (const auto& arg : args) {
            if (!arg.is_array()) {
              throw std::runtime_error("UNION: all arguments must be arrays");
            }
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::set<std::string> seen;
        nlohmann::json result = nlohmann::json::array();
        
        for (const auto& arr : args) {
            for (const auto& elem : arr) {
                std::string key = elem.dump();
                if (seen.find(key) == seen.end()) {
                    seen.insert(key);
                    result.push_back(elem);
                }
            }
        }
        return result;
    }
};

/**
 * @brief INTERSECTION(arr1, arr2, ...) - Intersection of arrays
 */
class IntersectionFunction : public IFunction {
public:
    ~IntersectionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "INTERSECTION",
            .category = "Array",
            .description = "Returns the intersection of all arrays (common values)",
            .arguments = {{"arrays", ArgType::ARRAY, true, nullptr, "Arrays to intersect (variadic)"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(INTERSECTION([1, 2, 3], [2, 3, 4]) // [2, 3])"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
          throw std::runtime_error("INTERSECTION requires at least 1 array");
        }
        for (const auto& arg : args) {
            if (!arg.is_array()) {
              throw std::runtime_error("INTERSECTION: all arguments must be arrays");
            }
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.empty()) {
          return nlohmann::json::array();
        }
        if (args.size() == 1) {
          return args[0];
        }
        
        // Start with first array
        std::set<std::string> current = {};

        for (const auto& elem : args[0]) {
            current.insert(elem.dump());
        }
        
        // Intersect with remaining arrays
        for (size_t i = 1; i < args.size(); i++) {
            std::set<std::string> next = {};

            for (const auto& elem : args[i]) {
                std::string key = elem.dump();
                if (current.find(key) != current.end()) {
                    next.insert(key);
                }
            }
            current = std::move(next);
        }
        
        // Build result preserving original elements
        nlohmann::json result = nlohmann::json::array();
        std::set<std::string> added = {};

        for (const auto& elem : args[0]) {
            std::string key = elem.dump();
            if (current.find(key) != current.end() && added.find(key) == added.end()) {
                added.insert(key);
                result.push_back(elem);
            }
        }
        return result;
    }
};

/**
 * @brief MINUS(arr1, arr2) - Difference of arrays
 */
class MinusFunction : public IFunction {
public:
    ~MinusFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MINUS",
            .category = "Array",
            .description = "Returns elements in first array that are not in second",
            .arguments = {
                {"arr1", ArgType::ARRAY, true, nullptr, "First array"},
                {"arr2", ArgType::ARRAY, true, nullptr, "Second array"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(MINUS([1, 2, 3], [2, 4]) // [1, 3])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::set<std::string> exclude = {};

        for (const auto& elem : args[1]) {
            exclude.insert(elem.dump());
        }
        
        nlohmann::json result = nlohmann::json::array();
        std::set<std::string> added = {};

        for (const auto& elem : args[0]) {
            std::string key = elem.dump();
            if (exclude.find(key) == exclude.end() && added.find(key) == added.end()) {
                added.insert(key);
                result.push_back(elem);
            }
        }
        return result;
    }
};

/**
 * @brief POSITION(array, value) - Find index of element
 */
class PositionFunction : public IFunction {
public:
    ~PositionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "POSITION",
            .category = "Array",
            .description = "Returns the index of a value in an array (-1 if not found)",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Input array"},
                {"value", ArgType::ANY, true, nullptr, "Value to find"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(POSITION([1, 2, 3], 2) // 1)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        const auto& value = args[1];
        
        for (size_t i = 0; i < arr.size(); i++) {
            if (arr[i] == value) {
              return static_cast<int64_t>(i);
            }
        }
        return static_cast<int64_t>(-1);
    }
};

/**
 * @brief COUNT(array) - Count elements
 */
class CountFunction : public IFunction {
public:
    ~CountFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "COUNT",
            .category = "Array",
            .description = "Returns the number of elements in an array or 1 for non-arrays",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Array or value"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .is_aggregate = true,
            .examples = {
                R"(COUNT([1, 2, 3]) // 3)",
                R"(COUNT(null) // 0)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        if (val.is_null()) {
          return static_cast<int64_t>(0);
        }
        if (val.is_array()) {
          return static_cast<int64_t>(val.size());
        }
        return static_cast<int64_t>(1);
    }
};

/**
 * @brief RANGE(start, end [, step]) - Generate number range
 */
class RangeFunction : public IFunction {
public:
    ~RangeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RANGE",
            .category = "Array",
            .description = "Generates an array of numbers from start to end",
            .arguments = {
                {"start", ArgType::INTEGER, true, nullptr, "Start value (inclusive)"},
                {"end", ArgType::INTEGER, true, nullptr, "End value (inclusive)"},
                {"step", ArgType::INTEGER, false, 1, "Step size"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(RANGE(1, 5) // [1, 2, 3, 4, 5])",
                R"(RANGE(0, 10, 2) // [0, 2, 4, 6, 8, 10])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t start = args[0].get<int64_t>();
        int64_t end = args[1].get<int64_t>();
        int64_t step = args.size() > 2 ? args[2].get<int64_t>() : 1;
        
        if (step == 0) {
          throw std::runtime_error("RANGE: step cannot be 0");
        }
        
        nlohmann::json result = nlohmann::json::array();
        
        if (step > 0) {
            for (int64_t i = start; i <= end; i += step) {
                result.push_back(i);
            }
        } else {
            for (int64_t i = start; i >= end; i += step) {
                result.push_back(i);
            }
        }
        
        return result;
    }
};

// ============================================================================
// Register Array Functions
// ============================================================================

inline void registerArrayFunctions(FunctionRegistry& reg) {
    reg.registerFunction(std::make_unique<FirstFunction>());
    reg.registerFunction(std::make_unique<LastFunction>());
    reg.registerFunction(std::make_unique<NthFunction>());
    reg.registerFunction(std::make_unique<PushFunction>());
    reg.registerFunction(std::make_unique<PopFunction>());
    reg.registerFunction(std::make_unique<ShiftFunction>());
    reg.registerFunction(std::make_unique<UnshiftFunction>());
    reg.registerFunction(std::make_unique<SliceFunction>());
    reg.registerFunction(std::make_unique<FlattenFunction>());
    reg.registerFunction(std::make_unique<UniqueFunction>());
    reg.registerFunction(std::make_unique<SortedFunction>());
    reg.registerFunction(std::make_unique<ReverseArrayFunction>());
    reg.registerFunction(std::make_unique<UnionFunction>());
    reg.registerFunction(std::make_unique<IntersectionFunction>());
    reg.registerFunction(std::make_unique<MinusFunction>());
    reg.registerFunction(std::make_unique<PositionFunction>());
    reg.registerFunction(std::make_unique<CountFunction>());
    reg.registerFunction(std::make_unique<RangeFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
