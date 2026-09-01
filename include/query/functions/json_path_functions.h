/**
 * @file json_path_functions.h
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
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// JSONPath Parser
// ============================================================================

/**
 * @brief Simple JSONPath parser for basic path expressions
 * 
 * Supports:
 * - $.field - root field access
 * - $.field.nested - nested field access
 * - $.array[0] - array index access
 * - $.field[0].nested - mixed access
 */
class JSONPath {
public:
    enum class SegmentType {
        FIELD,
        INDEX
    };
    
    struct Segment {
        SegmentType type;
        std::string field;
        int index;
        
        Segment(const std::string& f) : type(SegmentType::FIELD), field(f), index(-1) {}
        Segment(int idx) : type(SegmentType::INDEX), field(""), index(idx) {}
    };
    
    /**
     * @brief Parse a JSONPath expression
     * @param path JSONPath string (e.g., "$.field.nested[0]")
     * @return vector of path segments
     */
    static std::vector<Segment> parse(const std::string& path) {
        std::vector<Segment> segments;
        
        // Remove leading $. or $
        std::string working_path = path;
        if (working_path.starts_with("$.")) {
            working_path = working_path.substr(2);
        } else if (working_path.starts_with("$")) {
            working_path = working_path.substr(1);
        }
        
        if (working_path.empty()) {
            return segments; // Root path
        }
        
        // Parse segments
        size_t pos = 0;
        std::string current_field;
        
        while (pos < working_path.length()) {
            char c = working_path[pos];
            
            if (c == '.') {
                // Field separator
                if (!current_field.empty()) {
                    segments.push_back(Segment(current_field));
                    current_field.clear();
                }
                pos++;
            } else if (c == '[') {
                // Array index
                if (!current_field.empty()) {
                    segments.push_back(Segment(current_field));
                    current_field.clear();
                }
                
                // Find closing bracket
                size_t close = working_path.find(']', pos);
                if (close == std::string::npos) {
                    throw std::runtime_error("Invalid JSONPath: missing closing bracket");
                }
                
                std::string index_str = working_path.substr(pos + 1, close - pos - 1);
                // REL-22: wrap stoi() — index_str comes from user input and may be
                // non-numeric or out-of-range (e.g. "[abc]", "[999999999999]").
                int index;
                try {
                    index = std::stoi(index_str);
                } catch (const std::exception&) {
                    throw std::runtime_error("Invalid JSONPath: array index '" + index_str + "' is not a valid integer");
                }
                segments.push_back(Segment(index));
                pos = close + 1;
            } else {
                // Field character
                current_field += c;
                pos++;
            }
        }
        
        // Add final field if present
        if (!current_field.empty()) {
            segments.push_back(Segment(current_field));
        }
        
        return segments;
    }
    
    /**
     * @brief Extract value at JSONPath
     * @param root The root JSON object
     * @param path The JSONPath string
     * @return The value at the path, or null if not found
     */
    static nlohmann::json extract(const nlohmann::json& root, const std::string& path) {
        auto segments = parse(path);
        
        nlohmann::json current = root;
        for (const auto& segment : segments) {
            if (segment.type == SegmentType::FIELD) {
                if (!current.is_object() || !current.contains(segment.field)) {
                    return nullptr;
                }
                current = current[segment.field];
            } else if (segment.type == SegmentType::INDEX) {
                if (!current.is_array() || segment.index < 0 || 
                    static_cast<size_t>(segment.index) >= current.size()) {
                    return nullptr;
                }
                current = current[segment.index];
            }
        }
        
        return current;
    }
    
    /**
     * @brief Set value at JSONPath
     * @param root The root JSON object (modified in place)
     * @param path The JSONPath string
     * @param value The value to set
     * @return true if successful, false otherwise
     */
    static bool set(nlohmann::json& root, const std::string& path, const nlohmann::json& value) {
        auto segments = parse(path);
        
        if (segments.empty()) {
            root = value;
            return true;
        }
        
        // Navigate to parent
        nlohmann::json* current = &root;
        for (size_t i = 0; i < segments.size() - 1; i++) {
            const auto& segment = segments[i];
            
            if (segment.type == SegmentType::FIELD) {
                if (!current->is_object()) {
                    *current = nlohmann::json::object();
                }
                if (!current->contains(segment.field)) {
                    // Check next segment to decide what to create
                    if (i + 1 < segments.size() - 1 && segments[i + 1].type == SegmentType::INDEX) {
                        (*current)[segment.field] = nlohmann::json::array();
                    } else {
                        (*current)[segment.field] = nlohmann::json::object();
                    }
                }
                current = &(*current)[segment.field];
            } else if (segment.type == SegmentType::INDEX) {
                if (!current->is_array()) {
                    *current = nlohmann::json::array();
                }
                // Expand array if needed
                while (static_cast<size_t>(segment.index) >= current->size()) {
                    current->push_back(nullptr);
                }
                current = &(*current)[segment.index];
            }
        }
        
        // Set the final value
        const auto& last = segments.back();
        if (last.type == SegmentType::FIELD) {
            if (!current->is_object()) {
                *current = nlohmann::json::object();
            }
            (*current)[last.field] = value;
        } else if (last.type == SegmentType::INDEX) {
            if (!current->is_array()) {
                *current = nlohmann::json::array();
            }
            // Expand array if needed
            while (static_cast<size_t>(last.index) >= current->size()) {
                current->push_back(nullptr);
            }
            (*current)[last.index] = value;
        }
        
        return true;
    }
    
    /**
     * @brief Remove value at JSONPath
     * @param root The root JSON object (modified in place)
     * @param path The JSONPath string
     * @return true if something was removed, false otherwise
     */
    static bool remove(nlohmann::json& root, const std::string& path) {
        auto segments = parse(path);
        
        if (segments.empty()) {
            return false; // Can't remove root
        }
        
        // Navigate to parent
        nlohmann::json* current = &root;
        for (size_t i = 0; i < segments.size() - 1; i++) {
            const auto& segment = segments[i];
            
            if (segment.type == SegmentType::FIELD) {
                if (!current->is_object() || !current->contains(segment.field)) {
                    return false;
                }
                current = &(*current)[segment.field];
            } else if (segment.type == SegmentType::INDEX) {
                if (!current->is_array() || segment.index < 0 || 
                    static_cast<size_t>(segment.index) >= current->size()) {
                    return false;
                }
                current = &(*current)[segment.index];
            }
        }
        
        // Remove the final element
        const auto& last = segments.back();
        if (last.type == SegmentType::FIELD) {
            if (!current->is_object()) {
                return false;
            }
            return current->erase(last.field) > 0;
        } else if (last.type == SegmentType::INDEX) {
            if (!current->is_array() || last.index < 0 || 
                static_cast<size_t>(last.index) >= current->size()) {
                return false;
            }
            current->erase(last.index);
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Get the depth of a JSON structure (iterative with max depth limit)
     */
    static int depth(const nlohmann::json& root) {
        if (!root.is_object() && !root.is_array()) {
            return 0;
        }
        
        // Use iterative approach with stack to avoid stack overflow
        // and support early termination
        constexpr int MAX_DEPTH = 1000; // Reasonable limit
        
        struct StackItem {
            const nlohmann::json* node;
            int level;
        };
        
        std::vector<StackItem> stack;
        stack.push_back({&root, 1});
        int max_depth = 1;
        
        while (!stack.empty()) {
            auto item = stack.back();
            stack.pop_back();
            
            max_depth = std::max(max_depth, item.level);
            
            if (item.level >= MAX_DEPTH) {
                // Hit depth limit, return current max
                continue;
            }
            
            if (item.node->is_object()) {
                for (auto it = item.node->begin(); it != item.node->end(); ++it) {
                    if (it.value().is_object() || it.value().is_array()) {
                        stack.push_back({&it.value(), item.level + 1});
                    }
                }
            } else if (item.node->is_array()) {
                for (const auto& elem : *item.node) {
                    if (elem.is_object() || elem.is_array()) {
                        stack.push_back({&elem, item.level + 1});
                    }
                }
            }
        }
        
        return max_depth;
    }
    
    /**
     * @brief Check if a value exists in JSON
     */
    static bool contains(const nlohmann::json& root, const nlohmann::json& value) {
        if (root == value) {
            return true;
        }
        
        if (root.is_object()) {
            for (auto it = root.begin(); it != root.end(); ++it) {
                if (contains(it.value(), value)) {
                    return true;
                }
            }
        } else if (root.is_array()) {
            for (const auto& elem : root) {
                if (contains(elem, value)) {
                    return true;
                }
            }
        }
        
        return false;
    }
};

// ============================================================================
// JSON Path Functions
// ============================================================================

/**
 * @brief JSON_EXTRACT(doc, path) - Extract value using JSONPath
 */
class JsonExtractFunction : public IFunction {
public:
    ~JsonExtractFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_EXTRACT",
            .category = "JSON",
            .description = "Extracts a value from a JSON document using a JSONPath expression",
            .arguments = {
                {"document", ArgType::OBJECT, true, nullptr, "JSON document"},
                {"path", ArgType::STRING, true, nullptr, "JSONPath expression (e.g., '$.field.nested[0]')"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(JSON_EXTRACT({a: {b: 1}}, "$.a.b") // 1)",
                R"(JSON_EXTRACT({arr: [1,2,3]}, "$.arr[0]") // 1)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        try {
            return JSONPath::extract(args[0], args[1].get<std::string>());
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON_EXTRACT: ") + e.what());
        }
    }
};

/**
 * @brief JSON_SET(doc, path, value) - Set value at JSONPath
 */
class JsonSetFunction : public IFunction {
public:
    ~JsonSetFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_SET",
            .category = "JSON",
            .description = "Sets a value in a JSON document at the specified path",
            .arguments = {
                {"document", ArgType::OBJECT, true, nullptr, "JSON document"},
                {"path", ArgType::STRING, true, nullptr, "JSONPath expression"},
                {"value", ArgType::ANY, true, nullptr, "Value to set"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {
                R"(JSON_SET({a: 1}, "$.b", 2) // {a: 1, b: 2})",
                R"(JSON_SET({arr: []}, "$.arr[0]", "hello") // {arr: ["hello"]})"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        try {
            nlohmann::json result = args[0];
            JSONPath::set(result, args[1].get<std::string>(), args[2]);
            return result;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON_SET: ") + e.what());
        }
    }
};

/**
 * @brief JSON_REMOVE(doc, path) - Remove value at JSONPath
 */
class JsonRemoveFunction : public IFunction {
public:
    ~JsonRemoveFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_REMOVE",
            .category = "JSON",
            .description = "Removes a value from a JSON document at the specified path",
            .arguments = {
                {"document", ArgType::OBJECT, true, nullptr, "JSON document"},
                {"path", ArgType::STRING, true, nullptr, "JSONPath expression"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {
                R"(JSON_REMOVE({a: 1, b: 2}, "$.b") // {a: 1})",
                R"(JSON_REMOVE({arr: [1,2,3]}, "$.arr[1]") // {arr: [1,3]})"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        try {
            nlohmann::json result = args[0];
            JSONPath::remove(result, args[1].get<std::string>());
            return result;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON_REMOVE: ") + e.what());
        }
    }
};

/**
 * @brief JSON_TYPE(doc, path) - Get type at JSONPath
 */
class JsonTypeFunction : public IFunction {
public:
    ~JsonTypeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_TYPE",
            .category = "JSON",
            .description = "Returns the type of the value at the specified path",
            .arguments = {
                {"document", ArgType::OBJECT, true, nullptr, "JSON document"},
                {"path", ArgType::STRING, true, nullptr, "JSONPath expression"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(JSON_TYPE({a: 123}, "$.a") // "number")",
                R"(JSON_TYPE({arr: []}, "$.arr") // "array")"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        try {
            auto value = JSONPath::extract(args[0], args[1].get<std::string>());
            if (value.is_null()) return "null";
            if (value.is_boolean()) return "boolean";
            if (value.is_number_integer()) return "integer";
            if (value.is_number_float()) return "number";
            if (value.is_string()) return "string";
            if (value.is_array()) return "array";
            if (value.is_object()) return "object";
            return "unknown";
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON_TYPE: ") + e.what());
        }
    }
};

/**
 * @brief JSON_CONTAINS(doc, value) - Check if value exists
 */
class JsonContainsFunction : public IFunction {
public:
    ~JsonContainsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_CONTAINS",
            .category = "JSON",
            .description = "Checks if a JSON document contains a specific value",
            .arguments = {
                {"document", ArgType::OBJECT, true, nullptr, "JSON document"},
                {"value", ArgType::ANY, true, nullptr, "Value to search for"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(JSON_CONTAINS({a: 1, b: {c: 2}}, 2) // true)",
                R"(JSON_CONTAINS([1, 2, 3], 4) // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return JSONPath::contains(args[0], args[1]);
    }
};

/**
 * @brief JSON_DEPTH(doc) - Get maximum depth
 */
class JsonDepthFunction : public IFunction {
public:
    ~JsonDepthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_DEPTH",
            .category = "JSON",
            .description = "Returns the maximum depth of a JSON structure",
            .arguments = {
                {"document", ArgType::ANY, true, nullptr, "JSON document"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(JSON_DEPTH({a: {b: {c: 1}}}) // 3)",
                R"(JSON_DEPTH([1, [2, [3]]]) // 3)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return JSONPath::depth(args[0]);
    }
};

/**
 * @brief JSON_PARSE(str) - Parse JSON string
 */
class JsonParseFunction : public IFunction {
public:
    ~JsonParseFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_PARSE",
            .category = "JSON",
            .description = "Parses a JSON string into a JSON object",
            .arguments = {
                {"json_string", ArgType::STRING, true, nullptr, "JSON string to parse"}
            },
            .return_type = ArgType::ANY,
            .is_deterministic = true,
            .examples = {
                R"(JSON_PARSE('{"a": 1, "b": 2}') // {a: 1, b: 2})",
                R"(JSON_PARSE('[1, 2, 3]') // [1, 2, 3])"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        try {
            return nlohmann::json::parse(args[0].get<std::string>());
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON_PARSE: ") + e.what());
        }
    }
};

/**
 * @brief JSON_STRINGIFY(value) - Convert to JSON string
 */
class JsonStringifyFunction : public IFunction {
public:
    ~JsonStringifyFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "JSON_STRINGIFY",
            .category = "JSON",
            .description = "Converts a value to a JSON string",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "Value to stringify"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(JSON_STRINGIFY({a: 1, b: 2}) // '{"a":1,"b":2}')",
                R"(JSON_STRINGIFY([1, 2, 3]) // '[1,2,3]')"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].dump();
    }
};

// ============================================================================
// Register JSON Path Functions
// ============================================================================

inline void registerJsonPathFunctions(FunctionRegistry& reg) {
    reg.registerFunction(std::make_unique<JsonExtractFunction>());
    reg.registerFunction(std::make_unique<JsonSetFunction>());
    reg.registerFunction(std::make_unique<JsonRemoveFunction>());
    reg.registerFunction(std::make_unique<JsonTypeFunction>());
    reg.registerFunction(std::make_unique<JsonContainsFunction>());
    reg.registerFunction(std::make_unique<JsonDepthFunction>());
    reg.registerFunction(std::make_unique<JsonParseFunction>());
    reg.registerFunction(std::make_unique<JsonStringifyFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
