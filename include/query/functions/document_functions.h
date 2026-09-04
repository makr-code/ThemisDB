/**
 * @file document_functions.h
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

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Document/Object Functions
// ============================================================================

/**
 * @brief DOCUMENT(collection, key) - Load document by key
 */
class DocumentFunction : public IFunction {
public:
    ~DocumentFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DOCUMENT",
            .category = "Document",
            .description = "Loads a document from a collection by key",
            .arguments = {
                {"collection", ArgType::STRING, true, nullptr, "Collection name"},
                {"key", ArgType::STRING, true, nullptr, "Document key"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = false,
            .examples = {R"(DOCUMENT("users", "user123"))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext& context) const override {
        std::string collection = args[0].get<std::string>();
        std::string key = args[1].get<std::string>();
        return context.loadDocument(collection, key);
    }
};

/**
 * @brief MERGE(obj1, obj2, ...) - Merge objects
 */
class MergeFunction : public IFunction {
public:
    ~MergeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MERGE",
            .category = "Document",
            .description = "Merges multiple objects into one (later values override)",
            .arguments = {{"objects", ArgType::OBJECT, true, nullptr, "Objects to merge (variadic)"}},
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(MERGE({a: 1}, {b: 2}) // {a: 1, b: 2})"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        for (const auto& arg : args) {
            if (!arg.is_object() && !arg.is_null()) {
                throw std::runtime_error("MERGE: all arguments must be objects");
            }
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::object();
        for (const auto& obj : args) {
            if (obj.is_object()) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    result[it.key()] = it.value();
                }
            }
        }
        return result;
    }
};

/**
 * @brief MERGE_RECURSIVE(obj1, obj2, ...) - Deep merge objects
 */
class MergeRecursiveFunction : public IFunction {
public:
    ~MergeRecursiveFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MERGE_RECURSIVE",
            .category = "Document",
            .description = "Deep merges multiple objects (nested objects are merged)",
            .arguments = {{"objects", ArgType::OBJECT, true, nullptr, "Objects to merge (variadic)"}},
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(MERGE_RECURSIVE({a: {x: 1}}, {a: {y: 2}}) // {a: {x: 1, y: 2}})"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        for (const auto& arg : args) {
            if (!arg.is_object() && !arg.is_null()) {
                throw std::runtime_error("MERGE_RECURSIVE: all arguments must be objects");
            }
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::object();
        for (const auto& obj : args) {
            if (obj.is_object()) {
                mergeRecursive(result, obj);
            }
        }
        return result;
    }

private:
    static void mergeRecursive(nlohmann::json& target, const nlohmann::json& source) {
        for (auto it = source.begin(); it != source.end(); ++it) {
            if (it.value().is_object() && target.contains(it.key()) && target[it.key()].is_object()) {
                mergeRecursive(target[it.key()], it.value());
            } else {
                target[it.key()] = it.value();
            }
        }
    }
};

/**
 * @brief UNSET(obj, keys) - Remove keys from object
 */
class UnsetFunction : public IFunction {
public:
    ~UnsetFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNSET",
            .category = "Document",
            .description = "Returns a copy of the object with specified keys removed",
            .arguments = {
                {"obj", ArgType::OBJECT, true, nullptr, "Input object"},
                {"keys", ArgType::ARRAY, true, nullptr, "Array of keys to remove"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(UNSET({a: 1, b: 2, c: 3}, ["b"]) // {a: 1, c: 3})"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = args[0];
        for (const auto& key : args[1]) {
            if (key.is_string()) {
                result.erase(key.get<std::string>());
            }
        }
        return result;
    }
};

/**
 * @brief KEEP(obj, keys) - Keep only specified keys
 */
class KeepFunction : public IFunction {
public:
    ~KeepFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "KEEP",
            .category = "Document",
            .description = "Returns a copy of the object with only specified keys",
            .arguments = {
                {"obj", ArgType::OBJECT, true, nullptr, "Input object"},
                {"keys", ArgType::ARRAY, true, nullptr, "Array of keys to keep"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(KEEP({a: 1, b: 2, c: 3}, ["a", "c"]) // {a: 1, c: 3})"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::object();
        const auto& obj = args[0];
        for (const auto& key : args[1]) {
            if (key.is_string()) {
                std::string k = key.get<std::string>();
                if (obj.contains(k)) {
                    result[k] = obj[k];
                }
            }
        }
        return result;
    }
};

/**
 * @brief HAS(obj, key) - Check if key exists
 */
class HasFunction : public IFunction {
public:
    ~HasFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "HAS",
            .category = "Document",
            .description = "Checks if an object has a specific key",
            .arguments = {
                {"obj", ArgType::OBJECT, true, nullptr, "Input object"},
                {"key", ArgType::STRING, true, nullptr, "Key to check"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(HAS({a: 1, b: 2}, "a") // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].contains(args[1].get<std::string>());
    }
};

/**
 * @brief ATTRIBUTES(obj) - Get all keys
 */
class AttributesFunction : public IFunction {
public:
    ~AttributesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ATTRIBUTES",
            .category = "Document",
            .description = "Returns an array of all keys in an object",
            .arguments = {{"obj", ArgType::OBJECT, true, nullptr, "Input object"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(ATTRIBUTES({a: 1, b: 2}) // ["a", "b"])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        for (auto it = args[0].begin(); it != args[0].end(); ++it) {
            result.push_back(it.key());
        }
        return result;
    }
};

/**
 * @brief VALUES(obj) - Get all values
 */
class ValuesFunction : public IFunction {
public:
    ~ValuesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "VALUES",
            .category = "Document",
            .description = "Returns an array of all values in an object",
            .arguments = {{"obj", ArgType::OBJECT, true, nullptr, "Input object"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(VALUES({a: 1, b: 2}) // [1, 2])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::array();
        for (auto it = args[0].begin(); it != args[0].end(); ++it) {
            result.push_back(it.value());
        }
        return result;
    }
};

/**
 * @brief ZIP(keys, values) - Create object from arrays
 */
class ZipFunction : public IFunction {
public:
    ~ZipFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ZIP",
            .category = "Document",
            .description = "Creates an object from arrays of keys and values",
            .arguments = {
                {"keys", ArgType::ARRAY, true, nullptr, "Array of keys"},
                {"values", ArgType::ARRAY, true, nullptr, "Array of values"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(ZIP(["a", "b"], [1, 2]) // {a: 1, b: 2})"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json result = nlohmann::json::object();
        const auto& keys = args[0];
        const auto& values = args[1];
        
        size_t len = std::min(keys.size(), values.size());
        for (size_t i = 0; i < len; i++) {
            if (keys[i].is_string()) {
                result[keys[i].get<std::string>()] = values[i];
            }
        }
        return result;
    }
};

/**
 * @brief UNZIP(obj) - Split object into keys and values
 */
class UnzipFunction : public IFunction {
public:
    ~UnzipFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNZIP",
            .category = "Document",
            .description = "Splits an object into arrays of keys and values",
            .arguments = {{"obj", ArgType::OBJECT, true, nullptr, "Input object"}},
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {R"(UNZIP({a: 1, b: 2}) // {keys: ["a", "b"], values: [1, 2]})"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        nlohmann::json keys = nlohmann::json::array();
        nlohmann::json values = nlohmann::json::array();
        
        for (auto it = args[0].begin(); it != args[0].end(); ++it) {
            keys.push_back(it.key());
            values.push_back(it.value());
        }
        
        return {{"keys", keys}, {"values", values}};
    }
};

// ============================================================================
// Type Functions
// ============================================================================

/**
 * @brief TYPENAME(value) - Get type name
 */
class TypenameFunction : public IFunction {
public:
    ~TypenameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TYPENAME",
            .category = "Type",
            .description = "Returns the type name of a value",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Any value"}},
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(TYPENAME("hello") // "string")",
                R"(TYPENAME(123) // "number")",
                R"(TYPENAME([1,2]) // "array")"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        if (val.is_null()) {
          return "null";
        }
        if (val.is_boolean()) {
          return "bool";
        }
        if (val.is_number_integer()) {
          return "int";
        }
        if (val.is_number_float()) {
          return "number";
        }
        if (val.is_string()) {
          return "string";
        }
        if (val.is_array()) {
          return "array";
        }
        if (val.is_object()) {
          return "object";
        }
        return "unknown";
    }
};

/**
 * @brief IS_NULL/IS_BOOL/IS_NUMBER/IS_STRING/IS_ARRAY/IS_OBJECT
 */
class IsNullFunction : public IFunction {
public:
    ~IsNullFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_NULL",
            .category = "Type",
            .description = "Checks if value is null",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_NULL(null) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_null();
    }
};

/** @brief Is bool query function. */
class IsBoolFunction : public IFunction {
public:
    ~IsBoolFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_BOOL",
            .category = "Type",
            .description = "Checks if value is a boolean",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_BOOL(true) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_boolean();
    }
};

/** @brief Is number query function. */
class IsNumberFunction : public IFunction {
public:
    ~IsNumberFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_NUMBER",
            .category = "Type",
            .description = "Checks if value is a number",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_NUMBER(123) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_number();
    }
};

/** @brief Is string query function. */
class IsStringFunction : public IFunction {
public:
    ~IsStringFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_STRING",
            .category = "Type",
            .description = "Checks if value is a string",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_STRING("hello") // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_string();
    }
};

/** @brief Is array query function. */
class IsArrayFunction : public IFunction {
public:
    ~IsArrayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_ARRAY",
            .category = "Type",
            .description = "Checks if value is an array",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_ARRAY([1, 2]) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_array();
    }
};

/** @brief Is object query function. */
class IsObjectFunction : public IFunction {
public:
    ~IsObjectFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_OBJECT",
            .category = "Type",
            .description = "Checks if value is an object",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to check"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_OBJECT({a: 1}) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].is_object();
    }
};

/**
 * @brief TO_NUMBER/TO_STRING/TO_BOOL/TO_ARRAY - Type conversion
 */
class ToNumberFunction : public IFunction {
public:
    ~ToNumberFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TO_NUMBER",
            .category = "Type",
            .description = "Converts a value to a number",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to convert"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(TO_NUMBER("123") // 123)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return toNumber(args[0]);
    }
};

/** @brief To string query function. */
class ToStringFunction : public IFunction {
public:
    ~ToStringFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TO_STRING",
            .category = "Type",
            .description = "Converts a value to a string",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to convert"}},
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(TO_STRING(123) // "123")"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return toString(args[0]);
    }
};

/** @brief To bool query function. */
class ToBoolFunction : public IFunction {
public:
    ~ToBoolFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TO_BOOL",
            .category = "Type",
            .description = "Converts a value to a boolean",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to convert"}},
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(TO_BOOL(1) // true)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return toBool(args[0]);
    }
};

/** @brief To array query function. */
class ToArrayFunction : public IFunction {
public:
    ~ToArrayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TO_ARRAY",
            .category = "Type",
            .description = "Converts a value to an array",
            .arguments = {{"value", ArgType::ANY, true, nullptr, "Value to convert"}},
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {
                R"(TO_ARRAY("hello") // ["hello"])",
                R"(TO_ARRAY([1, 2]) // [1, 2])"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        if (val.is_array()) {
          return val;
        }
        if (val.is_null()) {
          return nlohmann::json::array();
        }
        return nlohmann::json::array({val});
    }
};

// ============================================================================
// Register Document Functions
// ============================================================================

inline void registerDocumentFunctions(FunctionRegistry& reg) {
    // Document functions
    reg.registerFunction(std::make_unique<DocumentFunction>());
    reg.registerFunction(std::make_unique<MergeFunction>());
    reg.registerFunction(std::make_unique<MergeRecursiveFunction>());
    reg.registerFunction(std::make_unique<UnsetFunction>());
    reg.registerFunction(std::make_unique<KeepFunction>());
    reg.registerFunction(std::make_unique<HasFunction>());
    reg.registerFunction(std::make_unique<AttributesFunction>());
    reg.registerFunction(std::make_unique<ValuesFunction>());
    reg.registerFunction(std::make_unique<ZipFunction>());
    reg.registerFunction(std::make_unique<UnzipFunction>());
    
    // Type functions
    reg.registerFunction(std::make_unique<TypenameFunction>());
    reg.registerFunction(std::make_unique<IsNullFunction>());
    reg.registerFunction(std::make_unique<IsBoolFunction>());
    reg.registerFunction(std::make_unique<IsNumberFunction>());
    reg.registerFunction(std::make_unique<IsStringFunction>());
    reg.registerFunction(std::make_unique<IsArrayFunction>());
    reg.registerFunction(std::make_unique<IsObjectFunction>());
    reg.registerFunction(std::make_unique<ToNumberFunction>());
    reg.registerFunction(std::make_unique<ToStringFunction>());
    reg.registerFunction(std::make_unique<ToBoolFunction>());
    reg.registerFunction(std::make_unique<ToArrayFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
