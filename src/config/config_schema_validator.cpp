#include "config/config_schema_validator.h"
#include "config/config_errors.h"
#include "config/config_path_resolver.h"
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <regex>

namespace themis {
namespace config {

// ═══════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════

namespace {

// Convert a yaml-cpp Node to nlohmann::json (forward declaration).
nlohmann::json yamlNodeToJsonImpl(const YAML::Node& node) {
    switch (node.Type()) {
        case YAML::NodeType::Null:
            return nullptr;

        case YAML::NodeType::Scalar: {
            // Try numeric types first, then boolean, then keep as string.
            const std::string s = node.Scalar();
            if (s == "true" || s == "yes" || s == "on")  return true;
            if (s == "false" || s == "no"  || s == "off") return false;
            if (s == "null" || s == "~")                  return nullptr;
            // Integer
            try {
                std::size_t pos = 0;
                long long i = std::stoll(s, &pos);
                if (pos == s.size()) return i;
            } catch (...) {}
            // Float
            try {
                std::size_t pos = 0;
                double d = std::stod(s, &pos);
                if (pos == s.size()) return d;
            } catch (...) {}
            return s;
        }

        case YAML::NodeType::Sequence: {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& child : node) {
                arr.push_back(yamlNodeToJsonImpl(child));
            }
            return arr;
        }

        case YAML::NodeType::Map: {
            nlohmann::json obj = nlohmann::json::object();
            for (const auto& kv : node) {
                obj[kv.first.as<std::string>()] = yamlNodeToJsonImpl(kv.second);
            }
            return obj;
        }

        default:
            return nullptr;
    }
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// loadAsJson
// ═══════════════════════════════════════════════════════════

nlohmann::json ConfigSchemaValidator::loadAsJson(const std::string& file_path) {
    // Check extension to choose parser.
    bool is_yaml = false;
    if (file_path.size() >= 5) {
        std::string ext = file_path.substr(file_path.size() - 5);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext == ".yaml") is_yaml = true;
    }
    if (!is_yaml && file_path.size() >= 4) {
        std::string ext = file_path.substr(file_path.size() - 4);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext == ".yml") is_yaml = true;
    }

    if (is_yaml) {
        try {
            YAML::Node root = YAML::LoadFile(file_path);
            return yamlNodeToJsonImpl(root);
        } catch (const YAML::Exception& e) {
            throw SchemaValidationException(file_path,
                std::string("YAML parse error: ") + e.what());
        }
    } else {
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            throw SchemaValidationException(file_path, "cannot open file");
        }
        try {
            nlohmann::json j;
            ifs >> j;
            return j;
        } catch (const nlohmann::json::exception& e) {
            throw SchemaValidationException(file_path,
                std::string("JSON parse error: ") + e.what());
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validate
// ═══════════════════════════════════════════════════════════

ConfigSchemaValidator::ValidationResult
ConfigSchemaValidator::validate(const std::string& config_path,
                                const nlohmann::json& schema) {
    ValidationResult result;
    result.config_path = config_path;

    nlohmann::json data;
    try {
        data = loadAsJson(config_path);
    } catch (const SchemaValidationException& e) {
        result.addError(e.what());
        return result;
    } catch (const std::exception& e) {
        result.addError(std::string("Unexpected error loading config: ") + e.what());
        return result;
    }

    validateValue(data, schema, "#", result);
    return result;
}

// ═══════════════════════════════════════════════════════════
// validateWithSchemaFile
// ═══════════════════════════════════════════════════════════

ConfigSchemaValidator::ValidationResult
ConfigSchemaValidator::validateWithSchemaFile(const std::string& config_path,
                                              const std::string& schema_path) {
    ValidationResult result;
    result.config_path = config_path;

    // Resolve schema path via ConfigPathResolver so legacy-to-new mapping applies.
    std::string resolved_schema = schema_path;
    auto maybe = ConfigPathResolver::tryResolve(schema_path);
    if (maybe.has_value()) {
        resolved_schema = *maybe;
    }
    result.schema_path = resolved_schema;

    nlohmann::json schema;
    try {
        schema = loadAsJson(resolved_schema);
    } catch (const SchemaValidationException& e) {
        result.addError(std::string("Schema file error: ") + e.what());
        return result;
    }

    nlohmann::json data;
    try {
        data = loadAsJson(config_path);
    } catch (const SchemaValidationException& e) {
        result.addError(e.what());
        return result;
    }

    validateValue(data, schema, "#", result);
    return result;
}

// ═══════════════════════════════════════════════════════════
// matchesType
// ═══════════════════════════════════════════════════════════

bool ConfigSchemaValidator::matchesType(const nlohmann::json& value,
                                        const std::string& type) {
    if (type == "null")    return value.is_null();
    if (type == "boolean") return value.is_boolean();
    if (type == "integer") return value.is_number_integer();
    if (type == "number")  return value.is_number();
    if (type == "string")  return value.is_string();
    if (type == "array")   return value.is_array();
    if (type == "object")  return value.is_object();
    return false;
}

// ═══════════════════════════════════════════════════════════
// validateValue  (dispatcher)
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateValue(const nlohmann::json& value,
                                          const nlohmann::json& schema,
                                          const std::string& json_path,
                                          ValidationResult& result) {
    if (!schema.is_object()) return;

    // --- enum ---
    if (schema.contains("enum") && schema["enum"].is_array()) {
        bool found = false;
        for (const auto& e : schema["enum"]) {
            if (value == e) { found = true; break; }
        }
        if (!found) {
            result.addError("Value at '" + json_path + "' is not one of the allowed enum values");
        }
        // When enum is present other keywords still apply per spec but we
        // return early after this check to keep complexity manageable.
        return;
    }

    // --- const ---
    if (schema.contains("const")) {
        if (value != schema["const"]) {
            result.addError("Value at '" + json_path + "' does not match the required const value");
        }
        return;
    }

    // --- type ---
    if (schema.contains("type")) {
        const auto& type_node = schema["type"];
        if (type_node.is_string()) {
            validateType(value, type_node.get<std::string>(), json_path, result);
        } else if (type_node.is_array()) {
            bool matched = false;
            for (const auto& t : type_node) {
                if (t.is_string() && matchesType(value, t.get<std::string>())) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                result.addError("Value at '" + json_path + "' does not match any of the allowed types");
            }
        }
    }

    // --- type-specific keywords ---
    if (value.is_object()) {
        validateObject(value, schema, json_path, result);
    } else if (value.is_array()) {
        validateArray(value, schema, json_path, result);
    } else if (value.is_string()) {
        validateString(value, schema, json_path, result);
    } else if (value.is_number()) {
        validateNumber(value, schema, json_path, result);
    }
}

// ═══════════════════════════════════════════════════════════
// validateType
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateType(const nlohmann::json& value,
                                         const std::string& expected_type,
                                         const std::string& json_path,
                                         ValidationResult& result) {
    if (!matchesType(value, expected_type)) {
        result.addError("Type mismatch at '" + json_path + "': expected '" + expected_type +
                        "', got '" + std::string(value.type_name()) + "'");
    }
}

// ═══════════════════════════════════════════════════════════
// validateObject
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateObject(const nlohmann::json& value,
                                           const nlohmann::json& schema,
                                           const std::string& json_path,
                                           ValidationResult& result) {
    // --- required ---
    if (schema.contains("required") && schema["required"].is_array()) {
        for (const auto& req : schema["required"]) {
            if (req.is_string() && !value.contains(req.get<std::string>())) {
                result.addError("Missing required property '" + req.get<std::string>() +
                                "' at '" + json_path + "'");
            }
        }
    }

    // --- properties ---
    if (schema.contains("properties") && schema["properties"].is_object()) {
        const auto& props = schema["properties"];
        for (const auto& [key, prop_schema] : props.items()) {
            if (value.contains(key)) {
                validateValue(value[key], prop_schema, json_path + "/" + key, result);
            }
        }
    }

    // --- additionalProperties ---
    if (schema.contains("additionalProperties")) {
        const auto& ap = schema["additionalProperties"];
        std::vector<std::string> known_keys;
        if (schema.contains("properties") && schema["properties"].is_object()) {
            for (const auto& [k, _] : schema["properties"].items()) {
                known_keys.push_back(k);
            }
        }
        for (const auto& [key, val] : value.items()) {
            bool known = false;
            for (const auto& k : known_keys) {
                if (k == key) { known = true; break; }
            }
            if (!known) {
                if (ap.is_boolean() && !ap.get<bool>()) {
                    result.addError("Additional property '" + key +
                                    "' is not allowed at '" + json_path + "'");
                } else if (ap.is_object()) {
                    validateValue(val, ap, json_path + "/" + key, result);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateArray
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateArray(const nlohmann::json& value,
                                          const nlohmann::json& schema,
                                          const std::string& json_path,
                                          ValidationResult& result) {
    // --- minItems ---
    if (schema.contains("minItems") && schema["minItems"].is_number_integer()) {
        std::size_t min = schema["minItems"].get<std::size_t>();
        if (value.size() < min) {
            result.addError("Array at '" + json_path + "' has " + std::to_string(value.size()) +
                            " items, minimum is " + std::to_string(min));
        }
    }

    // --- maxItems ---
    if (schema.contains("maxItems") && schema["maxItems"].is_number_integer()) {
        std::size_t max = schema["maxItems"].get<std::size_t>();
        if (value.size() > max) {
            result.addError("Array at '" + json_path + "' has " + std::to_string(value.size()) +
                            " items, maximum is " + std::to_string(max));
        }
    }

    // --- items ---
    if (schema.contains("items") && schema["items"].is_object()) {
        const auto& item_schema = schema["items"];
        std::size_t idx = 0;
        for (const auto& item : value) {
            validateValue(item, item_schema, json_path + "/" + std::to_string(idx), result);
            ++idx;
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateString
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateString(const nlohmann::json& value,
                                           const nlohmann::json& schema,
                                           const std::string& json_path,
                                           ValidationResult& result) {
    const std::string& s = value.get<std::string>();

    // --- minLength ---
    if (schema.contains("minLength") && schema["minLength"].is_number_integer()) {
        std::size_t min = schema["minLength"].get<std::size_t>();
        if (s.size() < min) {
            result.addError("String at '" + json_path + "' is too short (length " +
                            std::to_string(s.size()) + ", minimum " + std::to_string(min) + ")");
        }
    }

    // --- maxLength ---
    if (schema.contains("maxLength") && schema["maxLength"].is_number_integer()) {
        std::size_t max = schema["maxLength"].get<std::size_t>();
        if (s.size() > max) {
            result.addError("String at '" + json_path + "' is too long (length " +
                            std::to_string(s.size()) + ", maximum " + std::to_string(max) + ")");
        }
    }

    // --- pattern ---
    if (schema.contains("pattern") && schema["pattern"].is_string()) {
        const std::string& pattern = schema["pattern"].get<std::string>();
        try {
            std::regex re(pattern);
            if (!std::regex_search(s, re)) {
                result.addError("String at '" + json_path + "' does not match pattern '" +
                                pattern + "'");
            }
        } catch (const std::regex_error& e) {
            result.addWarning("Invalid pattern '" + pattern + "' in schema at '" +
                              json_path + "': " + e.what());
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateNumber
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateNumber(const nlohmann::json& value,
                                           const nlohmann::json& schema,
                                           const std::string& json_path,
                                           ValidationResult& result) {
    const double v = value.get<double>();

    // --- minimum ---
    if (schema.contains("minimum") && schema["minimum"].is_number()) {
        double min = schema["minimum"].get<double>();
        if (v < min) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) +
                            ") is less than minimum (" + std::to_string(min) + ")");
        }
    }

    // --- maximum ---
    if (schema.contains("maximum") && schema["maximum"].is_number()) {
        double max = schema["maximum"].get<double>();
        if (v > max) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) +
                            ") exceeds maximum (" + std::to_string(max) + ")");
        }
    }

    // --- exclusiveMinimum (Draft 7: numeric) ---
    if (schema.contains("exclusiveMinimum") && schema["exclusiveMinimum"].is_number()) {
        double emin = schema["exclusiveMinimum"].get<double>();
        if (v <= emin) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) +
                            ") must be strictly greater than exclusiveMinimum (" +
                            std::to_string(emin) + ")");
        }
    }

    // --- exclusiveMaximum (Draft 7: numeric) ---
    if (schema.contains("exclusiveMaximum") && schema["exclusiveMaximum"].is_number()) {
        double emax = schema["exclusiveMaximum"].get<double>();
        if (v >= emax) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) +
                            ") must be strictly less than exclusiveMaximum (" +
                            std::to_string(emax) + ")");
        }
    }
}

} // namespace config
} // namespace themis
