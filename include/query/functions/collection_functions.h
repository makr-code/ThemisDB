/**
 * @file collection_functions.h
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
#include "query/functions/holiday_provider.h"
#include <algorithm>
#include <sstream>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Collection Constructor Functions with JSON-Native Support
// ============================================================================
// 
// **IMPORTANT: Native Syntax vs Function Aliases**
// 
// AQL supports native literal syntax for creating collections:
//   LET arr = [1, 2, 3]           -- Native array literal
//   LET obj = { name: "Alice" }  -- Native object literal
//   LET nested = [1, [2, 3]]     -- Native nested structures
//
// The functions ARRAY(), DICT(), TUPLE(), SET() are **ALIASES** that provide:
// 1. **JSON-Native Parsing** - Parse JSON strings into collections
//    ARRAY('[1, 2, 3]')  =>  [1, 2, 3]  (parses JSON string)
// 2. **Explicit Type Coercion** - Ensure a value is a specific type
//    ARRAY(singleValue)  =>  [singleValue]  (wrap in array)
// 3. **Dynamic Construction** - Build from function results
//    DICT(ENTRIES(otherObj))  =>  reconstructed object
//
// **Recommendation:**
// - Use native syntax `[...]` and `{...}` for static literals
// - Use functions when parsing JSON strings or need type coercion
//
// **Equivalence Table:**
// | Native Syntax           | Function Alias                    |
// |-------------------------|-----------------------------------|
// | [1, 2, 3]               | ARRAY(1, 2, 3)                    |
// | { name: "Alice" }       | DICT("name", "Alice")             |
// | [[1], [2]]              | ARRAY([1], [2])                   |
// | (JSON string)           | ARRAY('[1,2,3]') - parses string! |
//
// ============================================================================

/**
 * @brief Helper to detect and parse JSON strings
 */
inline bool tryParseJson(const std::string& str, nlohmann::json& out) {
    if (str.empty()) return false;
    
    // Check if it looks like JSON (starts with [ or {)
    char first = str[0];
    if (first != '[' && first != '{') return false;
    
    try {
        out = nlohmann::json::parse(str);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief ARRAY(...) - Alias/wrapper for array creation with JSON parsing
 * 
 * This function is an **ALIAS** for the native array syntax `[...]`.
 * Use it when you need JSON-native parsing or explicit type coercion.
 * 
 * **Native Syntax (preferred for static literals):**
 *   LET arr = [1, 2, 3]              -- Use this for static arrays
 * 
 * **Function Syntax (for JSON parsing/coercion):**
 *   ARRAY('[1, 2, 3]')               -- Parses JSON string
 *   ARRAY(singleValue)               -- Wraps in array
 * 
 * Examples:
 *   ARRAY(1, 2, 3)                    => [1, 2, 3]  (equivalent to [1, 2, 3])
 *   ARRAY('[1, 2, 3]')                => [1, 2, 3]  (JSON string parsed!)
 *   ARRAY("a", "b", "c")              => ["a", "b", "c"]
 *   ARRAY({a: 1}, {b: 2})             => [{a: 1}, {b: 2}]
 *   ARRAY(singleValue)                => [singleValue]  (type coercion)
 *   ARRAY()                           => []
 */
class ArrayConstructorFunction : public IFunction {
public:
    ~ArrayConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ARRAY",
            .category = "Collection",
            .description = "Creates an array from arguments. Supports JSON-native string parsing.",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values to include (variadic). JSON strings like '[1,2,3]' are parsed."}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(ARRAY(1, 2, 3) // [1, 2, 3])",
                R"(ARRAY('[1, 2, 3]') // [1, 2, 3] - JSON parsed)",
                R"(ARRAY("a", "b") // ["a", "b"])",
                R"(ARRAY() // [])",
                R"(ARRAY('[1,2]', '[3,4]') // [[1,2], [3,4]])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {
        // Accept any number of arguments
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        // Empty array
        if (args.empty()) {
            return nlohmann::json::array();
        }
        
        // Single string argument - try JSON parse
        if (args.size() == 1 && args[0].is_string()) {
            nlohmann::json parsed;
            if (tryParseJson(args[0].get<std::string>(), parsed)) {
                if (parsed.is_array()) {
                    return parsed;
                }
                // Parsed but not array - wrap it
                return nlohmann::json::array({parsed});
            }
        }
        
        // Single array argument - return as-is
        if (args.size() == 1 && args[0].is_array()) {
            return args[0];
        }
        
        // Create array from all arguments, parsing JSON strings
        nlohmann::json result = nlohmann::json::array();
        for (const auto& arg : args) {
            if (arg.is_string()) {
                nlohmann::json parsed;
                if (tryParseJson(arg.get<std::string>(), parsed)) {
                    result.push_back(parsed);
                } else {
                    result.push_back(arg);
                }
            } else {
                result.push_back(arg);
            }
        }
        return result;
    }
};

/**
 * @brief DICT(...) - Create a dictionary/object (JSON-native)
 * 
 * Creates an object from key-value pairs OR parses a JSON string.
 * **Supports JSON-native parsing from strings.**
 * 
 * Examples:
 *   DICT("name", "Alice", "age", 30)  => {"name": "Alice", "age": 30}
 *   DICT('{"name": "Alice"}')         => {"name": "Alice"}  (JSON parsed)
 *   DICT("a", 1, "b", 2, "c", 3)      => {"a": 1, "b": 2, "c": 3}
 *   DICT()                            => {}
 *   DICT("nested", DICT('{"x": 1}'))  => {"nested": {"x": 1}}
 */
class DictConstructorFunction : public IFunction {
public:
    ~DictConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DICT",
            .category = "Collection",
            .description = "Creates an object from key-value pairs or JSON string",
            .arguments = {
                {"pairs", ArgType::ANY, false, nullptr, "Key-value pairs OR a JSON string like '{\"key\": value}'"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {
                R"(DICT("name", "Alice", "age", 30) // {"name": "Alice", "age": 30})",
                R"(DICT('{"name": "Alice"}') // {"name": "Alice"} - JSON parsed)",
                R"(DICT("x", 1, "y", 2) // {"x": 1, "y": 2})",
                R"(DICT() // {})"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        // Empty is OK
        if (args.empty()) return;
        
        // Single string argument - will try JSON parse
        if (args.size() == 1 && args[0].is_string()) return;
        
        // Single object argument - will return as-is
        if (args.size() == 1 && args[0].is_object()) return;
        
        // Otherwise must be even number of args (key-value pairs)
        if (args.size() % 2 != 0) {
            throw std::runtime_error("DICT: requires either a JSON string, an object, or an even number of key-value pairs");
        }
        
        // Validate that keys are strings
        for (size_t i = 0; i < args.size(); i += 2) {
            if (!args[i].is_string()) {
                throw std::runtime_error("DICT: keys must be strings, got " + 
                    std::string(args[i].type_name()) + " at position " + std::to_string(i));
            }
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        // Empty object
        if (args.empty()) {
            return nlohmann::json::object();
        }
        
        // Single argument - try JSON parse or return object
        if (args.size() == 1) {
            if (args[0].is_object()) {
                return args[0];
            }
            if (args[0].is_string()) {
                nlohmann::json parsed;
                if (tryParseJson(args[0].get<std::string>(), parsed)) {
                    if (parsed.is_object()) {
                        return parsed;
                    }
                    throw std::runtime_error("DICT: JSON string must be an object, got " + 
                        std::string(parsed.type_name()));
                }
                // Not valid JSON - error
                throw std::runtime_error("DICT: single string argument must be valid JSON object");
            }
        }
        
        // Key-value pairs
        nlohmann::json result = nlohmann::json::object();
        
        for (size_t i = 0; i < args.size(); i += 2) {
            std::string key = args[i].get<std::string>();
            nlohmann::json value = args[i + 1];
            
            // If value is a JSON string, parse it
            if (value.is_string()) {
                nlohmann::json parsed;
                if (tryParseJson(value.get<std::string>(), parsed)) {
                    value = parsed;
                }
            }
            
            result[key] = value;
        }
        
        return result;
    }
};

/**
 * @brief JSON(string) - Parse JSON string
 * 
 * Parses a JSON string into a native value.
 * 
 * Examples:
 *   JSON('[1, 2, 3]')                 => [1, 2, 3]
 *   JSON('{"name": "Alice"}')         => {"name": "Alice"}
 *   JSON('null')                      => null
 *   JSON('123')                       => 123
 *   JSON('"text"')                    => "text"
 */
// NOTE: renamed to avoid clashing with JSON_PATH functions' JsonParseFunction
class JsonValueParseFunction : public IFunction {
public:
    ~JsonValueParseFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON",
            .category = "Collection",
            .description = "Parses a JSON string into a native value",
            .arguments = {
                {"jsonString", ArgType::STRING, true, nullptr, "JSON string to parse"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(JSON('[1, 2, 3]') // [1, 2, 3])",
                R"(JSON('{"name": "Alice"}') // {"name": "Alice"})",
                R"(JSON('null') // null)",
                R"(JSON('123') // 123)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        
        try {
            return nlohmann::json::parse(str);
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("JSON: invalid JSON - " + std::string(e.what()));
        }
    }
};

/**
 * @brief TO_JSON(value) - Convert value to JSON string
 * 
 * Serializes a value to a JSON string.
 * 
 * Examples:
 *   TO_JSON([1, 2, 3])               => "[1,2,3]"
 *   TO_JSON({name: "Alice"})         => '{"name":"Alice"}'
 */
class ToJsonFunction : public IFunction {
public:
    ~ToJsonFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TO_JSON",
            .category = "Collection",
            .description = "Serializes a value to a JSON string",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to serialize"},
                {"pretty", ArgType::BOOLEAN, false, nullptr, "Pretty-print with indentation (default: false)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(TO_JSON([1, 2, 3]) // "[1,2,3]")",
                R"(TO_JSON({name: "Alice"}) // '{"name":"Alice"}')",
                R"(TO_JSON({a: 1}, true) // Pretty-printed)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        bool pretty = args.size() > 1 && args[1].get<bool>();
        
        if (pretty) {
            return args[0].dump(2);  // 2-space indent
        }
        return args[0].dump();
    }
};

/**
 * @brief JSON_VALID(string) - Check if string is valid JSON
 */
class JsonValidFunction : public IFunction {
public:
    ~JsonValidFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_VALID",
            .category = "Collection",
            .description = "Returns true if the string is valid JSON",
            .arguments = {
                {"jsonString", ArgType::STRING, true, nullptr, "String to validate"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(JSON_VALID('[1, 2, 3]') // true)",
                R"(JSON_VALID('not json') // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        
        try {
            auto parsed = nlohmann::json::parse(str);
            return parsed.type() != nlohmann::json::value_t::discarded;
        } catch (...) {
            return false;
        }
    }
};

/**
 * @brief JSON_TYPE(value) - Get JSON type as string
 */
// NOTE: renamed to avoid clashing with JSON_PATH functions' JsonTypeFunction
class JsonValueTypeFunction : public IFunction {
public:
    ~JsonValueTypeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_TYPE",
            .category = "Collection",
            .description = "Returns the JSON type of a value as a string",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to check"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(JSON_TYPE([1, 2]) // "array")",
                R"(JSON_TYPE({a: 1}) // "object")",
                R"(JSON_TYPE(123) // "number")",
                R"(JSON_TYPE("text") // "string")",
                R"(JSON_TYPE(null) // "null")",
                R"(JSON_TYPE(true) // "boolean")"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        
        if (val.is_null()) return "null";
        if (val.is_boolean()) return "boolean";
        if (val.is_number_integer()) return "integer";
        if (val.is_number_float()) return "number";
        if (val.is_number()) return "number";
        if (val.is_string()) return "string";
        if (val.is_array()) return "array";
        if (val.is_object()) return "object";
        
        return "unknown";
    }
};

/**
 * @brief OBJECT(key1, value1, ...) - Alias for DICT
 */
class ObjectConstructorFunction : public DictConstructorFunction {
public:
    ~ObjectConstructorFunction() override = default;
    FunctionSignature signature() const override {
        auto sig = DictConstructorFunction::signature();
        sig.name = "OBJECT";
        sig.description = "Creates an object from key-value pairs (alias for DICT)";
        return sig;
    }
};

/**
 * @brief SET(...) - Create a set (unique values array)
 * 
 * Creates an array with unique values only.
 * 
 * Examples:
 *   SET(1, 2, 2, 3, 3, 3)             => [1, 2, 3]
 *   SET("a", "b", "a")                => ["a", "b"]
 *   SET([1, 2, 2, 3])                 => [1, 2, 3]
 */
class SetConstructorFunction : public IFunction {
public:
    ~SetConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SET",
            .category = "Collection",
            .description = "Creates an array with unique values only (like a mathematical set)",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values to include (duplicates removed)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(SET(1, 2, 2, 3) // [1, 2, 3])",
                R"(SET("a", "b", "a") // ["a", "b"])",
                R"(SET([1, 1, 2, 2]) // [1, 2])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::vector<nlohmann::json> unique;
        
        auto addUnique = [&unique](const nlohmann::json& val) {
            bool found = false;
            for (const auto& existing : unique) {
                if (existing == val) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unique.push_back(val);
            }
        };
        
        for (const auto& arg : args) {
            if (arg.is_array()) {
                // Flatten single array argument
                for (const auto& item : arg) {
                    addUnique(item);
                }
            } else {
                addUnique(arg);
            }
        }
        
        return nlohmann::json(unique);
    }
};

/**
 * @brief TUPLE(...) - Create a fixed-size tuple (array)
 * 
 * Creates an array representing a tuple. Semantically indicates
 * a fixed-size, ordered collection (vs. variable-length ARRAY).
 */
class TupleConstructorFunction : public IFunction {
public:
    ~TupleConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TUPLE",
            .category = "Collection",
            .description = "Creates a tuple (fixed-size array) from arguments",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values for the tuple"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(TUPLE(1, "a", true) // [1, "a", true])",
                R"(TUPLE(x, y) // [x, y])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return nlohmann::json(args);
    }
};

/**
 * @brief PAIR(key, value) - Create a key-value pair
 * 
 * Creates a two-element array representing a key-value pair.
 * Useful for building maps incrementally.
 */
class PairConstructorFunction : public IFunction {
public:
    ~PairConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "PAIR",
            .category = "Collection",
            .description = "Creates a key-value pair as a two-element array",
            .arguments = {
                {"key", ArgType::ANY, true, nullptr, "The key"},
                {"value", ArgType::ANY, true, nullptr, "The value"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(PAIR("name", "Alice") // ["name", "Alice"])",
                R"(PAIR(1, "one") // [1, "one"])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return nlohmann::json::array({args[0], args[1]});
    }
};

/**
 * @brief RANGE(start, end, step?) - Create a numeric range array
 * 
 * Creates an array of numbers from start to end (exclusive).
 */
class RangeConstructorFunction : public IFunction {
public:
    ~RangeConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RANGE",
            .category = "Collection",
            .description = "Creates an array of numbers from start to end (exclusive)",
            .arguments = {
                {"start", ArgType::NUMBER, true, nullptr, "Start value (inclusive)"},
                {"end", ArgType::NUMBER, true, nullptr, "End value (exclusive)"},
                {"step", ArgType::NUMBER, false, nullptr, "Step size (default: 1)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(RANGE(0, 5) // [0, 1, 2, 3, 4])",
                R"(RANGE(1, 10, 2) // [1, 3, 5, 7, 9])",
                R"(RANGE(10, 0, -1) // [10, 9, 8, 7, 6, 5, 4, 3, 2, 1])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.size() >= 3 && args[2].get<double>() == 0) {
            throw std::runtime_error("RANGE: step cannot be zero");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double start = args[0].get<double>();
        double end = args[1].get<double>();
        double step = args.size() > 2 ? args[2].get<double>() : 1.0;
        
        // Auto-adjust step direction
        if ((end > start && step < 0) || (end < start && step > 0)) {
            step = -step;
        }
        
        nlohmann::json result = nlohmann::json::array();
        
        // Limit to prevent infinite loops
        const size_t maxSize = 100000;
        
        if (step > 0) {
            for (double i = start; i < end && result.size() < maxSize; i += step) {
                if (i == std::floor(i)) {
                    result.push_back(static_cast<int64_t>(i));
                } else {
                    result.push_back(i);
                }
            }
        } else {
            for (double i = start; i > end && result.size() < maxSize; i += step) {
                if (i == std::floor(i)) {
                    result.push_back(static_cast<int64_t>(i));
                } else {
                    result.push_back(i);
                }
            }
        }
        
        return result;
    }
};

/**
 * @brief REPEAT(value, count) - Create array with repeated value
 */
class RepeatConstructorFunction : public IFunction {
public:
    ~RepeatConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REPEAT",
            .category = "Collection",
            .description = "Creates an array with a value repeated N times",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to repeat"},
                {"count", ArgType::INTEGER, true, nullptr, "Number of repetitions"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(REPEAT(0, 5) // [0, 0, 0, 0, 0])",
                R"(REPEAT("x", 3) // ["x", "x", "x"])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        int64_t count = args[1].get<int64_t>();
        if (count < 0) {
            throw std::runtime_error("REPEAT: count cannot be negative");
        }
        if (count > 100000) {
            throw std::runtime_error("REPEAT: count too large (max 100000)");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json value = args[0];
        int64_t count = args[1].get<int64_t>();
        
        nlohmann::json result = nlohmann::json::array();
        for (int64_t i = 0; i < count; i++) {
            result.push_back(value);
        }
        
        return result;
    }
};

// ============================================================================
// External Data Loading Functions
// ============================================================================

/**
 * @brief LOAD_HOLIDAYS(calendarName) - Load holidays from external file
 * 
 * Securely loads a holiday calendar from YAML or JSON file.
 * 
 * Security features:
 * - Path injection protection
 * - Whitelisted directories only
 * - File size limits
 * - Content validation
 * 
 * Examples:
 *   LOAD_HOLIDAYS("germany_2024")
 *   LOAD_HOLIDAYS("us_federal_2024")
 */
class LoadHolidaysFunction : public IFunction {
public:
    ~LoadHolidaysFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LOAD_HOLIDAYS",
            .category = "Collection",
            .description = "Securely loads a holiday calendar from external YAML/JSON file",
            .arguments = {
                {"calendarName", ArgType::STRING, true, nullptr, 
                 "Calendar name (e.g., 'germany_2024', 'us_federal_2024')"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = false,  // File content can change
            .examples = {
                R"(LET holidays = LOAD_HOLIDAYS("germany_2024"))",
                R"(WORKDAYS(start, end, LOAD_HOLIDAYS("company_holidays")))"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (!args[0].is_string()) {
            throw std::runtime_error("LOAD_HOLIDAYS: calendar name must be a string");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string name = args[0].get<std::string>();
        
        auto& provider = HolidayProvider::instance();
        std::set<int64_t> holidays = provider.getHolidays(name);
        
        return HolidayProvider::toJsonArray(holidays);
    }
};

/**
 * @brief LIST_CALENDARS() - List available holiday calendars
 */
class ListCalendarsFunction : public IFunction {
public:
    ~ListCalendarsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LIST_CALENDARS",
            .category = "Collection",
            .description = "Lists all available holiday calendar names",
            .arguments = {},
            .return_type = ArgType::ARRAY,
            .is_deterministic = false,
            .examples = {
                R"(LIST_CALENDARS() // ["germany_2024", "us_federal_2024", ...])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto& provider = HolidayProvider::instance();
        std::vector<std::string> calendars = provider.listCalendars();
        
        return nlohmann::json(calendars);
    }
};

/**
 * @brief HOLIDAYS_BETWEEN(calendar, startDate, endDate) - Get holidays in date range
 */
class HolidaysBetweenFunction : public IFunction {
public:
    ~HolidaysBetweenFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "HOLIDAYS_BETWEEN",
            .category = "Collection",
            .description = "Returns holidays from a calendar within a date range",
            .arguments = {
                {"calendarName", ArgType::STRING, true, nullptr, "Calendar name (e.g., 'DE_2024')"},
                {"startDate", ArgType::INTEGER, true, nullptr, "Start timestamp (ms)"},
                {"endDate", ArgType::INTEGER, true, nullptr, "End timestamp (ms)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(HOLIDAYS_BETWEEN("DE_2024", MAKE_DATE(2024,12,1), MAKE_DATE(2024,12,31)))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string name = args[0].get<std::string>();
        int64_t startMs = args[1].get<int64_t>();
        int64_t endMs = args[2].get<int64_t>();
        
        if (startMs > endMs) {
            std::swap(startMs, endMs);
        }
        
        auto& provider = HolidayProvider::instance();
        std::set<int64_t> allHolidays = provider.getHolidays(name);
        
        nlohmann::json result = nlohmann::json::array();
        for (int64_t h : allHolidays) {
            if (h >= startMs && h <= endMs) {
                result.push_back(h);
            }
        }
        
        return result;
    }
};

// ============================================================================
// Inline Holiday Definition Functions
// ============================================================================

/**
 * @brief HOLIDAYS(...) - Get holidays from calendar or create inline array
 * 
 * Can be used in two ways:
 * 1. Load from registered calendar: HOLIDAYS("DE_2024")
 * 2. Create inline from dates: HOLIDAYS("2024-12-25", "2024-12-26")
 * 3. Merge multiple calendars: HOLIDAYS("DE_2024", "company_holidays")
 * 
 * Examples:
 *   HOLIDAYS("DE_2024")  // Load German holidays 2024
 *   HOLIDAYS("DE_2024", "AT_2024")  // Merge German and Austrian
 *   HOLIDAYS("2024-12-25", "2024-12-26")  // Inline dates
 */
class HolidaysInlineFunction : public IFunction {
public:
    ~HolidaysInlineFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "HOLIDAYS",
            .category = "Collection",
            .description = "Load holidays from calendar(s) or create from date strings",
            .arguments = {
                {"sources", ArgType::ANY, false, nullptr, "Calendar names or date strings (YYYY-MM-DD)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(HOLIDAYS("DE_2024") // Load German holidays)",
                R"(HOLIDAYS("DE_2024", "AT_2024") // Merge calendars)",
                R"(HOLIDAYS("2024-12-25", "2024-12-26") // Inline dates)",
                R"(WORKDAYS(start, end, HOLIDAYS("DE_2024")))"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::set<int64_t> holidays;
        auto& provider = HolidayProvider::instance();
        
        for (const auto& arg : args) {
            if (arg.is_string()) {
                std::string str = arg.get<std::string>();
                
                // Check if it looks like a calendar name (uppercase, no dashes in date format)
                bool looksLikeCalendar = true;
                if (str.length() >= 10 && str[4] == '-' && str[7] == '-') {
                    looksLikeCalendar = false;  // Looks like YYYY-MM-DD
                }
                
                if (looksLikeCalendar) {
                    // Try to load as calendar
                    try {
                        auto calHolidays = provider.getHolidays(str);
                        holidays.insert(calHolidays.begin(), calHolidays.end());
                        continue;
                    } catch (...) {
                        // Not a calendar, try as date
                    }
                }
                
                // Parse as date
                try {
                    int64_t ts = HolidayProvider::parseDateToTimestamp(str);
                    holidays.insert(ts);
                } catch (...) {
                    throw std::runtime_error("HOLIDAYS: Invalid calendar name or date: " + str);
                }
            } else if (arg.is_number()) {
                int64_t ts = arg.get<int64_t>();
                // Normalize to day start
                ts = (ts / (24 * 60 * 60 * 1000)) * (24 * 60 * 60 * 1000);
                holidays.insert(ts);
            } else if (arg.is_array()) {
                // Flatten array
                for (const auto& item : arg) {
                    if (item.is_string()) {
                        int64_t ts = HolidayProvider::parseDateToTimestamp(item.get<std::string>());
                        holidays.insert(ts);
                    } else if (item.is_number()) {
                        int64_t ts = item.get<int64_t>();
                        ts = (ts / (24 * 60 * 60 * 1000)) * (24 * 60 * 60 * 1000);
                        holidays.insert(ts);
                    }
                }
            }
        }
        
        return HolidayProvider::toJsonArray(holidays);
    }
};

// ============================================================================
// Type Conversion Constructors
// ============================================================================

/**
 * @brief LIST(value) - Convert to array/list
 * 
 * Converts various types to array:
 * - String: splits by comma or newline
 * - Object: returns array of values
 * - Array: returns as-is
 * - Other: wraps in array
 */
class ListConstructorFunction : public IFunction {
public:
    ~ListConstructorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LIST",
            .category = "Collection",
            .description = "Converts a value to a list/array",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to convert"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(LIST("a,b,c") // ["a", "b", "c"])",
                R"(LIST({a: 1, b: 2}) // [1, 2])",
                R"(LIST([1, 2, 3]) // [1, 2, 3])"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        
        if (val.is_array()) {
            return val;
        }
        
        if (val.is_object()) {
            nlohmann::json result = nlohmann::json::array();
            for (auto it = val.begin(); it != val.end(); ++it) {
                result.push_back(it.value());
            }
            return result;
        }
        
        if (val.is_string()) {
            std::string str = val.get<std::string>();
            nlohmann::json result = nlohmann::json::array();
            
            // Try splitting by common delimiters
            std::string delimiter = ",";
            if (str.find('\n') != std::string::npos) {
                delimiter = "\n";
            } else if (str.find(';') != std::string::npos) {
                delimiter = ";";
            }
            
            size_t pos = 0;
            while ((pos = str.find(delimiter)) != std::string::npos) {
                std::string item = str.substr(0, pos);
                // Trim whitespace
                size_t start = item.find_first_not_of(" \t\r\n");
                size_t end = item.find_last_not_of(" \t\r\n");
                if (start != std::string::npos) {
                    result.push_back(item.substr(start, end - start + 1));
                }
                str.erase(0, pos + delimiter.length());
            }
            // Add last part
            size_t start = str.find_first_not_of(" \t\r\n");
            size_t end = str.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                result.push_back(str.substr(start, end - start + 1));
            }
            
            return result;
        }
        
        if (val.is_null()) {
            return nlohmann::json::array();
        }
        
        // Wrap single value in array
        return nlohmann::json::array({val});
    }
};

/**
 * @brief KEYS(object) - Get object keys as array
 */
class KeysFunction : public IFunction {
public:
    ~KeysFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "KEYS",
            .category = "Collection",
            .description = "Returns the keys of an object as an array",
            .arguments = {
                {"object", ArgType::OBJECT, true, nullptr, "Object to get keys from"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(KEYS({a: 1, b: 2, c: 3}) // ["a", "b", "c"])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        
        if (args[0].is_object()) {
            for (auto it = args[0].begin(); it != args[0].end(); ++it) {
                result.push_back(it.key());
            }
        }
        
        return result;
    }
};

/**
 * @brief ENTRIES(object) - Get object as array of [key, value] pairs
 */
class EntriesFunction : public IFunction {
public:
    ~EntriesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ENTRIES",
            .category = "Collection",
            .description = "Returns object entries as array of [key, value] pairs",
            .arguments = {
                {"object", ArgType::OBJECT, true, nullptr, "Object to convert"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(ENTRIES({a: 1, b: 2}) // [["a", 1], ["b", 2]])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        
        if (args[0].is_object()) {
            for (auto it = args[0].begin(); it != args[0].end(); ++it) {
                result.push_back(nlohmann::json::array({it.key(), it.value()}));
            }
        }
        
        return result;
    }
};

/**
 * @brief FROM_ENTRIES(entries) - Create object from [key, value] pairs
 */
class FromEntriesFunction : public IFunction {
public:
    ~FromEntriesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FROM_ENTRIES",
            .category = "Collection",
            .description = "Creates an object from an array of [key, value] pairs",
            .arguments = {
                {"entries", ArgType::ARRAY, true, nullptr, "Array of [key, value] pairs"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {
                R"(FROM_ENTRIES([["a", 1], ["b", 2]]) // {a: 1, b: 2})"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::object();
        
        if (args[0].is_array()) {
            for (const auto& entry : args[0]) {
                if (entry.is_array() && entry.size() >= 2) {
                    std::string key;
                    if (entry[0].is_string()) {
                        key = entry[0].get<std::string>();
                    } else {
                        key = entry[0].dump();
                    }
                    result[key] = entry[1];
                }
            }
        }
        
        return result;
    }
};

// ============================================================================
// Logical Array Functions (Excel-Style)
// ============================================================================
//
// These functions provide Excel-inspired logical operations on arrays and
// collections. They support AND, OR, NOT, XOR semantics for combining,
// filtering, and transforming data sets.
//
// **Excel-Style Examples:**
//   AND([true, true, true])           => true  (all true)
//   OR([false, true, false])          => true  (at least one true)
//   NOT([true, false])                => [false, true]
//   XOR([true, true])                 => false (exclusive or)
//   IF(condition, trueVal, falseVal)  => conditional value
//   IFS(cond1, val1, cond2, val2)     => multi-conditional
//   FILTER(array, condition)          => filtered array
//   MAP(array, expression)            => transformed array
//   REDUCE(array, initial, expr)      => aggregated value
//
// ============================================================================

/**
 * @brief AND(...) - Logical AND operation (Excel-compatible)
 * 
 * Returns true if ALL arguments are true or truthy.
 * For arrays: returns true if ALL elements are truthy.
 * 
 * Truthy values: true, non-zero numbers, non-empty strings, non-empty arrays/objects
 * Falsy values: false, 0, "", null, [], {}
 * 
 * Examples:
 *   AND(true, true, true)             => true
 *   AND(true, false, true)            => false
 *   AND([true, true, true])           => true (array form)
 *   AND(1, 2, 3)                      => true (all non-zero)
 *   AND(1, 0, 3)                      => false (zero is falsy)
 */
class AndFunction : public IFunction {
public:
    ~AndFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "AND",
            .category = "Logical",
            .description = "Returns true if all arguments are truthy (Excel-compatible)",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values or array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(AND(true, true, true) // true)",
                R"(AND(true, false) // false)",
                R"(AND([1, 2, 3]) // true - all non-zero)",
                R"(AND(1, 0, 3) // false - zero is falsy)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    static bool isTruthy(const nlohmann::json& val) {
        if (val.is_null()) return false;
        if (val.is_boolean()) return val.get<bool>();
        if (val.is_number()) return val.get<double>() != 0;
        if (val.is_string()) return !val.get<std::string>().empty();
        if (val.is_array()) return !val.empty();
        if (val.is_object()) return !val.empty();
        return false;
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.empty()) return true;
        
        // Single array argument - test all elements
        if (args.size() == 1 && args[0].is_array()) {
            for (const auto& elem : args[0]) {
                if (!isTruthy(elem)) return false;
            }
            return true;
        }
        
        // Multiple arguments
        for (const auto& arg : args) {
            if (!isTruthy(arg)) return false;
        }
        return true;
    }
};

/**
 * @brief OR(...) - Logical OR operation (Excel-compatible)
 * 
 * Returns true if AT LEAST ONE argument is true or truthy.
 * For arrays: returns true if ANY element is truthy.
 * 
 * Examples:
 *   OR(false, false, true)            => true
 *   OR(false, false, false)           => false
 *   OR([0, 0, 1])                     => true
 */
class OrFunction : public IFunction {
public:
    ~OrFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "OR",
            .category = "Logical",
            .description = "Returns true if at least one argument is truthy (Excel-compatible)",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values or array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(OR(false, true, false) // true)",
                R"(OR(false, false, false) // false)",
                R"(OR([0, 0, 1]) // true)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.empty()) return false;
        
        // Single array argument
        if (args.size() == 1 && args[0].is_array()) {
            for (const auto& elem : args[0]) {
                if (AndFunction::isTruthy(elem)) return true;
            }
            return false;
        }
        
        // Multiple arguments
        for (const auto& arg : args) {
            if (AndFunction::isTruthy(arg)) return true;
        }
        return false;
    }
};

/**
 * @brief NOT(value) - Logical NOT operation (Excel-compatible)
 * 
 * Inverts the truthiness of a value or all elements in an array.
 * 
 * Examples:
 *   NOT(true)                         => false
 *   NOT(false)                        => true
 *   NOT(0)                            => true
 *   NOT([true, false, true])          => [false, true, false]
 */
class NotFunction : public IFunction {
public:
    ~NotFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "NOT",
            .category = "Logical",
            .description = "Inverts truthiness of a value or array elements",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value or array to invert"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(NOT(true) // false)",
                R"(NOT(0) // true)",
                R"(NOT([true, false]) // [false, true])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        
        // Array - invert each element
        if (val.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto& elem : val) {
                result.push_back(!AndFunction::isTruthy(elem));
            }
            return result;
        }
        
        // Single value
        return !AndFunction::isTruthy(val);
    }
};

/**
 * @brief XOR(...) - Logical Exclusive OR operation (Excel-compatible)
 * 
 * Returns true if an ODD number of arguments are true.
 * For arrays: counts truthy elements.
 * 
 * Examples:
 *   XOR(true, false)                  => true  (1 true)
 *   XOR(true, true)                   => false (2 trues)
 *   XOR(true, true, true)             => true  (3 trues)
 *   XOR([true, false, true])          => false (2 trues)
 */
class XorFunction : public IFunction {
public:
    ~XorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "XOR",
            .category = "Logical",
            .description = "Returns true if an odd number of arguments are truthy (Excel-compatible)",
            .arguments = {
                {"values", ArgType::ANY, false, nullptr, "Values or array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(XOR(true, false) // true)",
                R"(XOR(true, true) // false)",
                R"(XOR(true, true, true) // true)",
                R"(XOR([1, 0, 1]) // false)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        size_t trueCount = 0;
        
        // Single array argument
        if (args.size() == 1 && args[0].is_array()) {
            for (const auto& elem : args[0]) {
                if (AndFunction::isTruthy(elem)) trueCount++;
            }
        } else {
            // Multiple arguments
            for (const auto& arg : args) {
                if (AndFunction::isTruthy(arg)) trueCount++;
            }
        }
        
        return (trueCount % 2) == 1;
    }
};

/**
 * @brief IF(condition, trueValue [, falseValue]) - Conditional expression (Excel-compatible)
 * 
 * Returns trueValue if condition is truthy, otherwise falseValue.
 * 
 * Examples:
 *   IF(true, "yes", "no")             => "yes"
 *   IF(1 > 0, "positive", "negative") => "positive"
 *   IF(false, "yes")                  => null (no else value)
 */
class IfFunction_Collection : public IFunction {
public:
    ~IfFunction_Collection() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IF",
            .category = "Logical",
            .description = "Returns one value if condition is true, another if false",
            .arguments = {
                {"condition", ArgType::ANY, true, nullptr, "Condition to test"},
                {"trueValue", ArgType::ANY, true, nullptr, "Value if true"},
                {"falseValue", ArgType::ANY, false, nullptr, "Value if false (default: null)"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(IF(true, "yes", "no") // "yes")",
                R"(IF(x > 0, "positive", "negative"))",
                R"(IF(false, "yes") // null)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        bool condition = AndFunction::isTruthy(args[0]);
        
        if (condition) {
            return args[1];
        }
        
        return args.size() > 2 ? args[2] : nlohmann::json(nullptr);
    }
};

/**
 * @brief IFS(cond1, val1, cond2, val2, ...) - Multiple conditions (Excel-compatible)
 * 
 * Evaluates conditions in order and returns the first matching value.
 * 
 * Examples:
 *   IFS(false, "a", true, "b", true, "c")  => "b"
 *   IFS(x < 0, "negative", x > 0, "positive", true, "zero")
 */
class IfsFunction : public IFunction {
public:
    ~IfsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IFS",
            .category = "Logical",
            .description = "Returns value for first true condition (Excel-compatible)",
            .arguments = {
                {"pairs", ArgType::ANY, false, nullptr, "Condition-value pairs"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(IFS(false, "a", true, "b") // "b")",
                R"(IFS(x < 0, "neg", x > 0, "pos", true, "zero"))"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.size() % 2 != 0) {
            throw std::runtime_error("IFS: requires an even number of arguments (condition-value pairs)");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        for (size_t i = 0; i < args.size(); i += 2) {
            if (AndFunction::isTruthy(args[i])) {
                return args[i + 1];
            }
        }
        return nullptr;
    }
};

/**
 * @brief SWITCH(expr, case1, val1, case2, val2, ..., [default]) - Switch/Case (Excel-compatible)
 * 
 * Matches expression against cases and returns corresponding value.
 * 
 * Examples:
 *   SWITCH(2, 1, "one", 2, "two", 3, "three")  => "two"
 *   SWITCH(x, "a", 1, "b", 2, 0)  => 0 (default if no match)
 */
class SwitchFunction : public IFunction {
public:
    ~SwitchFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SWITCH",
            .category = "Logical",
            .description = "Matches expression against cases (Excel-compatible)",
            .arguments = {
                {"expression", ArgType::ANY, true, nullptr, "Value to match"},
                {"cases", ArgType::ANY, false, nullptr, "Case-value pairs, optional default"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(SWITCH(2, 1, "one", 2, "two") // "two")",
                R"(SWITCH(x, "a", 1, "b", 2, 0) // 0 if no match)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.size() < 2) return nullptr;
        
        const auto& expr = args[0];
        
        // Check pairs
        size_t i = 1;
        while (i + 1 < args.size()) {
            if (args[i] == expr) {
                return args[i + 1];
            }
            i += 2;
        }
        
        // Default value (odd number of remaining args)
        if (i < args.size()) {
            return args[i];
        }
        
        return nullptr;
    }
};

/**
 * @brief CHOOSE(index, val1, val2, ...) - Choose by index (Excel-compatible)
 * 
 * Returns the value at the specified index (1-based like Excel).
 * 
 * Examples:
 *   CHOOSE(2, "a", "b", "c")          => "b"
 *   CHOOSE(1, 10, 20, 30)             => 10
 */
class ChooseFunction : public IFunction {
public:
    ~ChooseFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CHOOSE",
            .category = "Logical",
            .description = "Returns value at index (1-based, Excel-compatible)",
            .arguments = {
                {"index", ArgType::INTEGER, true, nullptr, "Index (1-based)"},
                {"values", ArgType::ANY, false, nullptr, "Values to choose from"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(CHOOSE(2, "a", "b", "c") // "b")",
                R"(CHOOSE(1, 10, 20, 30) // 10)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.size() < 2) return nullptr;
        
        int64_t index = args[0].get<int64_t>();
        
        // 1-based index like Excel
        if (index < 1 || static_cast<size_t>(index) >= args.size()) {
            return nullptr;
        }
        
        return args[static_cast<size_t>(index)];
    }
};

// ============================================================================
// Set/Array Logical Operations
// ============================================================================

/**
 * @brief ARRAY_AND(arr1, arr2) - Element-wise AND
 * 
 * Returns array with element-wise AND of two arrays.
 * 
 * Examples:
 *   ARRAY_AND([true, true, false], [true, false, false])  
 *   => [true, false, false]
 */
class ArrayAndFunction : public IFunction {
public:
    ~ArrayAndFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ARRAY_AND",
            .category = "Logical",
            .description = "Element-wise AND of two arrays",
            .arguments = {
                {"arr1", ArgType::ARRAY, true, nullptr, "First array"},
                {"arr2", ArgType::ARRAY, true, nullptr, "Second array"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(ARRAY_AND([true, true, false], [true, false, false]) // [true, false, false])",
                R"(ARRAY_AND([1, 2, 0], [1, 0, 1]) // [true, false, false])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr1 = args[0];
        const auto& arr2 = args[1];
        
        size_t len = std::min(arr1.size(), arr2.size());
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < len; i++) {
            result.push_back(AndFunction::isTruthy(arr1[i]) && AndFunction::isTruthy(arr2[i]));
        }
        
        return result;
    }
};

/**
 * @brief ARRAY_OR(arr1, arr2) - Element-wise OR
 */
class ArrayOrFunction : public IFunction {
public:
    ~ArrayOrFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ARRAY_OR",
            .category = "Logical",
            .description = "Element-wise OR of two arrays",
            .arguments = {
                {"arr1", ArgType::ARRAY, true, nullptr, "First array"},
                {"arr2", ArgType::ARRAY, true, nullptr, "Second array"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(ARRAY_OR([true, false, false], [false, false, true]) // [true, false, true])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr1 = args[0];
        const auto& arr2 = args[1];
        
        size_t len = std::min(arr1.size(), arr2.size());
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < len; i++) {
            result.push_back(AndFunction::isTruthy(arr1[i]) || AndFunction::isTruthy(arr2[i]));
        }
        
        return result;
    }
};

/**
 * @brief ARRAY_XOR(arr1, arr2) - Element-wise XOR
 */
class ArrayXorFunction : public IFunction {
public:
    ~ArrayXorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ARRAY_XOR",
            .category = "Logical",
            .description = "Element-wise XOR of two arrays",
            .arguments = {
                {"arr1", ArgType::ARRAY, true, nullptr, "First array"},
                {"arr2", ArgType::ARRAY, true, nullptr, "Second array"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(ARRAY_XOR([true, true, false], [true, false, false]) // [false, true, false])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr1 = args[0];
        const auto& arr2 = args[1];
        
        size_t len = std::min(arr1.size(), arr2.size());
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < len; i++) {
            bool a = AndFunction::isTruthy(arr1[i]);
            bool b = AndFunction::isTruthy(arr2[i]);
            result.push_back(a != b);
        }
        
        return result;
    }
};

/**
 * @brief ALL(array) - Check if all elements are truthy
 * 
 * Alias for AND with single array argument.
 */
class AllFunction : public IFunction {
public:
    ~AllFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ALL",
            .category = "Logical",
            .description = "Returns true if all array elements are truthy",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(ALL([true, true, true]) // true)",
                R"(ALL([1, 2, 3]) // true)",
                R"(ALL([1, 0, 3]) // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        for (const auto& elem : arr) {
            if (!AndFunction::isTruthy(elem)) return false;
        }
        return true;
    }
};

/**
 * @brief ANY(array) - Check if any element is truthy
 * 
 * Alias for OR with single array argument.
 */
class AnyFunction : public IFunction {
public:
    ~AnyFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ANY",
            .category = "Logical",
            .description = "Returns true if any array element is truthy",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(ANY([false, false, true]) // true)",
                R"(ANY([0, 0, 1]) // true)",
                R"(ANY([0, 0, 0]) // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        for (const auto& elem : arr) {
            if (AndFunction::isTruthy(elem)) return true;
        }
        return false;
    }
};

/**
 * @brief NONE(array) - Check if no elements are truthy
 */
class NoneFunction : public IFunction {
public:
    ~NoneFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "NONE",
            .category = "Logical",
            .description = "Returns true if no array elements are truthy",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Array to test"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(NONE([false, false, false]) // true)",
                R"(NONE([0, 0, 0]) // true)",
                R"(NONE([0, 1, 0]) // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        for (const auto& elem : arr) {
            if (AndFunction::isTruthy(elem)) return false;
        }
        return true;
    }
};

/**
 * @brief COUNT_IF(array, condition) - Count elements matching condition
 * 
 * Note: In AQL, condition is typically expressed via FILTER.
 * This counts truthy elements in the array.
 */
class CountIfFunction : public IFunction {
public:
    ~CountIfFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "COUNT_IF",
            .category = "Logical",
            .description = "Counts truthy elements in array (Excel COUNTIF style)",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Array to count"},
                {"value", ArgType::ANY, false, nullptr, "Specific value to count (optional)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(COUNT_IF([true, false, true]) // 2)",
                R"(COUNT_IF([1, 0, 2, 0, 3]) // 3)",
                R"(COUNT_IF([1, 2, 1, 3, 1], 1) // 3)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        int64_t count = 0;
        
        if (args.size() > 1) {
            // Count specific value
            const auto& value = args[1];
            for (const auto& elem : arr) {
                if (elem == value) count++;
            }
        } else {
            // Count truthy elements
            for (const auto& elem : arr) {
                if (AndFunction::isTruthy(elem)) count++;
            }
        }
        
        return count;
    }
};

/**
 * @brief SUM_IF(array, conditionArray) - Sum elements where condition is true
 */
class SumIfFunction : public IFunction {
public:
    ~SumIfFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SUM_IF",
            .category = "Logical",
            .description = "Sums elements where corresponding condition is truthy",
            .arguments = {
                {"values", ArgType::ARRAY, true, nullptr, "Array of numbers"},
                {"conditions", ArgType::ARRAY, true, nullptr, "Array of conditions"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(SUM_IF([10, 20, 30], [true, false, true]) // 40)",
                R"(SUM_IF([1, 2, 3, 4], [1, 0, 1, 0]) // 4)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& values = args[0];
        const auto& conditions = args[1];
        
        double sum = 0;
        size_t len = std::min(values.size(), conditions.size());
        
        for (size_t i = 0; i < len; i++) {
            if (AndFunction::isTruthy(conditions[i]) && values[i].is_number()) {
                sum += values[i].get<double>();
            }
        }
        
        return sum;
    }
};

/**
 * @brief FILTER_BY(array, conditionArray) - Filter array by condition array
 */
class FilterByFunction : public IFunction {
public:
    ~FilterByFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FILTER_BY",
            .category = "Logical",
            .description = "Filters array elements by corresponding condition array",
            .arguments = {
                {"array", ArgType::ARRAY, true, nullptr, "Array to filter"},
                {"conditions", ArgType::ARRAY, true, nullptr, "Condition array (parallel)"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(FILTER_BY([1, 2, 3, 4], [true, false, true, false]) // [1, 3])",
                R"(FILTER_BY(["a", "b", "c"], [1, 0, 1]) // ["a", "c"])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        const auto& conditions = args[1];
        
        nlohmann::json result = nlohmann::json::array();
        size_t len = std::min(arr.size(), conditions.size());
        
        for (size_t i = 0; i < len; i++) {
            if (AndFunction::isTruthy(conditions[i])) {
                result.push_back(arr[i]);
            }
        }
        
        return result;
    }
};

/**
 * @brief IFERROR(value, errorValue) - Return alternative on error/null
 * 
 * Excel-compatible error handling.
 */
class IfErrorFunction : public IFunction {
public:
    ~IfErrorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IFERROR",
            .category = "Logical",
            .description = "Returns alternative value if first value is null/error",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to test"},
                {"errorValue", ArgType::ANY, true, nullptr, "Value to return if null"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(IFERROR(null, 0) // 0)",
                R"(IFERROR(123, 0) // 123)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args[0].is_null()) {
            return args[1];
        }
        return args[0];
    }
};

/**
 * @brief IFNA(value, naValue) - Return alternative for N/A values
 * 
 * Same as IFERROR for our purposes.
 */
class IfNaFunction : public IfErrorFunction {
public:
    ~IfNaFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IFNA",
            .category = "Logical",
            .description = "Returns alternative value if first value is null/N/A",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to test"},
                {"naValue", ArgType::ANY, true, nullptr, "Value to return if N/A"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(IFNA(null, "N/A") // "N/A")",
                R"(IFNA("value", "N/A") // "value")"
            }
        };
    }
};

// ============================================================================
// Register Collection Functions
// ============================================================================

inline void registerCollectionFunctions(FunctionRegistry& reg) {
    // Array/Collection constructors
    reg.registerFunction(std::make_unique<ArrayConstructorFunction>());
    reg.registerFunction(std::make_unique<DictConstructorFunction>());
    reg.registerFunction(std::make_unique<ObjectConstructorFunction>());
    reg.registerFunction(std::make_unique<SetConstructorFunction>());
    reg.registerFunction(std::make_unique<TupleConstructorFunction>());
    reg.registerFunction(std::make_unique<PairConstructorFunction>());
    reg.registerFunction(std::make_unique<RangeConstructorFunction>());
    reg.registerFunction(std::make_unique<RepeatConstructorFunction>());
    
    // JSON functions
    reg.registerFunction(std::make_unique<JsonValueParseFunction>());
    reg.registerFunction(std::make_unique<ToJsonFunction>());
    reg.registerFunction(std::make_unique<JsonValidFunction>());
    reg.registerFunction(std::make_unique<JsonValueTypeFunction>());
    
    // Holiday/Calendar functions
    reg.registerFunction(std::make_unique<LoadHolidaysFunction>());
    reg.registerFunction(std::make_unique<ListCalendarsFunction>());
    reg.registerFunction(std::make_unique<HolidaysBetweenFunction>());
    reg.registerFunction(std::make_unique<HolidaysInlineFunction>());
    
    // Type conversion
    reg.registerFunction(std::make_unique<ListConstructorFunction>());
    reg.registerFunction(std::make_unique<KeysFunction>());
    reg.registerFunction(std::make_unique<EntriesFunction>());
    reg.registerFunction(std::make_unique<FromEntriesFunction>());
    
    // Logical functions (Excel-style)
    reg.registerFunction(std::make_unique<AndFunction>());
    reg.registerFunction(std::make_unique<OrFunction>());
    reg.registerFunction(std::make_unique<NotFunction>());
    reg.registerFunction(std::make_unique<XorFunction>());
    reg.registerFunction(std::make_unique<IfFunction_Collection>());
    reg.registerFunction(std::make_unique<IfsFunction>());
    reg.registerFunction(std::make_unique<SwitchFunction>());
    reg.registerFunction(std::make_unique<ChooseFunction>());
    
    // Array logical operations
    reg.registerFunction(std::make_unique<ArrayAndFunction>());
    reg.registerFunction(std::make_unique<ArrayOrFunction>());
    reg.registerFunction(std::make_unique<ArrayXorFunction>());
    reg.registerFunction(std::make_unique<AllFunction>());
    reg.registerFunction(std::make_unique<AnyFunction>());
    reg.registerFunction(std::make_unique<NoneFunction>());
    reg.registerFunction(std::make_unique<CountIfFunction>());
    reg.registerFunction(std::make_unique<SumIfFunction>());
    reg.registerFunction(std::make_unique<FilterByFunction>());
    reg.registerFunction(std::make_unique<IfErrorFunction>());
    reg.registerFunction(std::make_unique<IfNaFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
