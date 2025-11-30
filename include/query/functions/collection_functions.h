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
// These functions provide explicit type constructors for creating
// arrays, dictionaries (objects), and sets. They support:
// - JSON-native parsing from strings
// - Explicit type conversion
// - Creating complex nested structures
// - Loading external data
// - Ensuring type safety in queries
//
// **JSON-Native Examples:**
//   ARRAY('[1, 2, 3]')                => [1, 2, 3]
//   DICT('{"name": "Alice"}')         => {"name": "Alice"}
//   ARRAY(1, 2, ARRAY('[3, 4]'))      => [1, 2, [3, 4]]
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
 * @brief ARRAY(...) - Create an array from arguments (JSON-native)
 * 
 * Creates an array containing all passed arguments.
 * **Supports JSON-native parsing from strings.**
 * 
 * Examples:
 *   ARRAY(1, 2, 3)                    => [1, 2, 3]
 *   ARRAY('[1, 2, 3]')                => [1, 2, 3]  (JSON string parsed)
 *   ARRAY("a", "b", "c")              => ["a", "b", "c"]
 *   ARRAY({a: 1}, {b: 2})             => [{a: 1}, {b: 2}]
 *   ARRAY(singleValue)                => [singleValue]
 *   ARRAY()                           => []
 *   ARRAY('[1, 2]', '[3, 4]')         => [[1, 2], [3, 4]]  (nested)
 */
class ArrayConstructorFunction : public IFunction {
public:
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
class JsonParseFunction : public IFunction {
public:
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
            nlohmann::json::parse(str);
            return true;
        } catch (...) {
            return false;
        }
    }
};

/**
 * @brief JSON_TYPE(value) - Get JSON type as string
 */
class JsonTypeFunction : public IFunction {
public:
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
        std::set<int64_t> holidays = provider.loadHolidays(name);
        
        return HolidayProvider::toJsonArray(holidays);
    }
};

/**
 * @brief LIST_CALENDARS() - List available holiday calendars
 */
class ListCalendarsFunction : public IFunction {
public:
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
    reg.registerFunction(std::make_unique<JsonParseFunction>());
    reg.registerFunction(std::make_unique<ToJsonFunction>());
    reg.registerFunction(std::make_unique<JsonValidFunction>());
    reg.registerFunction(std::make_unique<JsonTypeFunction>());
    
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
}

} // namespace functions
} // namespace query
} // namespace themis
