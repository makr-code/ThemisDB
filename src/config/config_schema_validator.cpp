/**
 * @file config_schema_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "config/config_schema_validator.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include "config/config_errors.h"
#include "config/config_path_resolver.h"

namespace themis {
namespace config {

// ═══════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════

namespace {

// Convert a yaml-cpp Node to nlohmann::json (forward declaration).
nlohmann::json yamlNodeToJsonImpl(const YAML::Node &node) {
    switch (node.Type()) {
        case YAML::NodeType::Null:
            return nullptr;

        case YAML::NodeType::Scalar: {
            // Try numeric types first, then boolean, then keep as string.
            const std::string s = node.Scalar();
            if (s == "true" || s == "yes" || s == "on") {
                return true;
            }
            if (s == "false" || s == "no" || s == "off") {
                return false;
            }
            if (s == "null" || s == "~") {
                return nullptr;
            }
            // Integer
            try {
                std::size_t pos = 0;
                long long i = std::stoll(s, &pos);
                if (pos == s.size()) {
                  return i;
                }
            } catch (const std::invalid_argument &) {
            } catch (const std::out_of_range &) {
            } catch (const std::string &) {
            } catch (const char *) {
            }
            // Float
            try {
                std::size_t pos = 0;
                double d = std::stod(s, &pos);
                if (pos == s.size()) {
                  return d;
                }
            } catch (const std::invalid_argument &) {
            } catch (const std::out_of_range &) {
            } catch (const std::string &) {
            } catch (const char *) {
            }
            return s;
        }

        case YAML::NodeType::Sequence: {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &child : node) {
                arr.push_back(yamlNodeToJsonImpl(child));
            }
            return arr;
        }

        case YAML::NodeType::Map: {
            nlohmann::json obj = nlohmann::json::object();
            for (const auto &kv : node) {
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

nlohmann::json ConfigSchemaValidator::loadAsJson(const std::string &file_path) {
    // Check extension to choose parser.
    bool is_yaml = false;
    if (static_cast<int>(file_path.size()) > = 5) {
        std::string ext = file_path.substr(file_path.size() - 5);
        for (auto &c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (ext == ".yaml") {
            is_yaml = true;
        }
    }
    if (!is_yaml && file_path.size() >= 4) {
        std::string ext = file_path.substr(file_path.size() - 4);
        for (auto &c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (ext == ".yml") {
            is_yaml = true;
        }
    }

    if (is_yaml) {
        try {
            YAML::Node root = YAML::LoadFile(file_path);
            return yamlNodeToJsonImpl(root);
        } catch (const YAML::Exception &e) {
            throw SchemaValidationException(file_path, std::string("YAML parse error: ") + e.what());
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
        } catch (const nlohmann::json::exception &e) {
            throw SchemaValidationException(file_path, std::string("JSON parse error: ") + e.what());
        }
    }
}

nlohmann::json ConfigSchemaValidator::loadAsJson(const std::string &content, bool is_yaml) {
    if (is_yaml) {
        try {
            YAML::Node root = YAML::Load(content);
            return yamlNodeToJsonImpl(root);
        } catch (const YAML::Exception &e) {
            throw SchemaValidationException("<string>", std::string("YAML parse error: ") + e.what());
        }
    } else {
        try {
            return nlohmann::json::parse(content);
        } catch (const nlohmann::json::exception &e) {
            throw SchemaValidationException("<string>", std::string("JSON parse error: ") + e.what());
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateFromString
// ═══════════════════════════════════════════════════════════

ConfigSchemaValidator::ValidationResult
ConfigSchemaValidator::validateFromString(const std::string &content, bool is_yaml, const nlohmann::json &schema) {
    ValidationResult result;
    result.config_path = "<string>";

    nlohmann::json data;
    try {
        data = loadAsJson(content, is_yaml);
    } catch (const SchemaValidationException &e) {
        result.addError(e.what());
        return result;
    } catch (const std::exception &e) {
        result.addError(std::string("Unexpected error parsing config string: ") + e.what());
        return result;
    }

    validateValue(data, schema, "#", result);
    return result;
}

// ═══════════════════════════════════════════════════════════
// validate
// ═══════════════════════════════════════════════════════════

ConfigSchemaValidator::ValidationResult ConfigSchemaValidator::validate(const std::string &config_path,
                                                                        const nlohmann::json &schema) {
    ValidationResult result;
    result.config_path = config_path;

    nlohmann::json data;
    try {
        data = loadAsJson(config_path);
    } catch (const SchemaValidationException &e) {
        result.addError(e.what());
        return result;
    } catch (const std::exception &e) {
        result.addError(std::string("Unexpected error loading config: ") + e.what());
        return result;
    }

    validateValue(data, schema, "#", result);
    return result;
}

// ═══════════════════════════════════════════════════════════
// validateWithSchemaFile
// ═══════════════════════════════════════════════════════════

ConfigSchemaValidator::ValidationResult ConfigSchemaValidator::validateWithSchemaFile(const std::string &config_path,
                                                                                      const std::string &schema_path) {
    ValidationResult result;
    result.config_path = config_path;

    // Resolve schema path via ConfigPathResolver so legacy-to-new mapping applies.
    std::string resolved_schema = schema_path;
    auto maybe                  = ConfigPathResolver::tryResolve(schema_path);
    if (maybe.has_value()) {
        resolved_schema = *maybe;
    }
    result.schema_path = resolved_schema;

    nlohmann::json schema;
    try {
        schema = loadAsJson(resolved_schema);
    } catch (const SchemaValidationException &e) {
        result.addError(std::string("Schema file error: ") + e.what());
        return result;
    }

    nlohmann::json data;
    try {
        data = loadAsJson(config_path);
    } catch (const SchemaValidationException &e) {
        result.addError(e.what());
        return result;
    }

    validateValue(data, schema, "#", result);
    return result;
}

// ═══════════════════════════════════════════════════════════
// resolveRef  (RFC 6901 JSON Pointer over local '#/...' refs)
// ═══════════════════════════════════════════════════════════

const nlohmann::json *ConfigSchemaValidator::resolveRef(const std::string &ref, const nlohmann::json &root_schema) {
    // Only document-internal refs beginning with '#' are supported.
    if (ref.empty() || ref[0] != '#') {
        return nullptr;
    }

    // "#" alone refers to the root schema.
    if (ref == "#") {
        return &root_schema;
    }

    // After '#' there must be a '/'.
    if (ref.size() < 2 || ref[1] != '/') {
        return nullptr;
    }

    // Walk the JSON Pointer path (RFC 6901).
    const nlohmann::json *node = &root_schema;
    const std::string path     = ref.substr(2); // strip leading "#/"

    std::size_t pos = 0;
    while (pos <= path.size()) {
        const std::size_t slash     = path.find('/', pos);
        const std::string raw_token = (slash == std::string::npos) ? path.substr(pos) : path.substr(pos, slash - pos);
        pos                         = (slash == std::string::npos) ? path.size() + 1 : slash + 1;

        // RFC 6901: unescape '~1' → '/' and '~0' → '~' (in that order).
        std::string key = {};
        key.reserve(raw_token.size());
        for (std::size_t i = 0; i < raw_token.size(); ++i) {
            if (raw_token[i] == '~' && i + 1 < raw_token.size()) {
                if (raw_token[i + 1] == '1') {
                    key += '/';
                    ++i;
                } else if (raw_token[i + 1] == '0') {
                    key += '~';
                    ++i;
                } else {
                    key += raw_token[i];
                }
            } else {
                key += raw_token[i];
            }
        }

        if (node->is_object()) {
            auto it = node->find(key);
            if (it == node->end()) {
                return nullptr;
            }
            node = &(*it);
        } else if (node->is_array()) {
            // RFC 6901 §4: array index must be "0" or a positive decimal
            // integer with no leading zeros.
            if (key.empty() || (key[0] == '0' && key.size() > 1)) {
                return nullptr;
            }
            try {
                const std::size_t idx = std::stoull(key);
                if (idx >= node->size()) {
                    return nullptr;
                }
                node = &((*node)[idx]);
            } catch (const std::invalid_argument &) {
                return nullptr;
            } catch (const std::out_of_range &) {
                return nullptr;
            } catch (const std::string &) {
                return nullptr;
            } catch (const char *) {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }
    return node;
}

// ═══════════════════════════════════════════════════════════
// matchesType
// ═══════════════════════════════════════════════════════════

bool ConfigSchemaValidator::matchesType(const nlohmann::json &value, const std::string &type) {
    if (type == "null") {
        return value.is_null();
    }
    if (type == "boolean") {
        return value.is_boolean();
    }
    if (type == "integer") {
        return value.is_number_integer();
    }
    if (type == "number") {
        return value.is_number();
    }
    if (type == "string") {
        return value.is_string();
    }
    if (type == "array") {
        return value.is_array();
    }
    if (type == "object") {
        return value.is_object();
    }
    return false;
}

// ═══════════════════════════════════════════════════════════
// validateValue  (entry-point wrapper)
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateValue(const nlohmann::json &value, const nlohmann::json &schema,
                                          const std::string &json_path, ValidationResult &result) {
    std::vector<std::string> visited;
    validateValueImpl(value, schema, json_path, result, schema, visited);
}

// ═══════════════════════════════════════════════════════════
// validateValueImpl  (dispatcher, carries root schema and visited-refs)
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateValueImpl(const nlohmann::json &value, const nlohmann::json &schema,
                                              const std::string &json_path, ValidationResult &result,
                                              const nlohmann::json &root_schema,
                                              std::vector<std::string> &visited_refs) {
    if (!schema.is_object()) {
        return;
    }

    // --- $ref ---
    // In JSON Schema Draft 7, $ref replaces sibling keywords; resolve and
    // delegate to the referenced schema.
    if (schema.contains("$ref") && schema["$ref"].is_string()) {
        const std::string &ref = schema["$ref"].get<std::string>();

        // Detect and reject cycles before they can recurse infinitely.
        for (const auto &v : visited_refs) {
            if (v == ref) {
                result.addError("Cyclic $ref detected at '" + json_path + "': " + ref);
                return;
            }
        }

        // External URI resolution is out of scope to prevent SSRF.
        if (ref.empty() || ref[0] != '#') {
            result.addError("External $ref is not supported at '" + json_path + "': " + ref);
            return;
        }

        const nlohmann::json *resolved = resolveRef(ref, root_schema);
        if (!resolved) {
            result.addError("Cannot resolve $ref '" + ref + "' at '" + json_path + "'");
            return;
        }

        visited_refs.push_back(ref);
        validateValueImpl(value, *resolved, json_path, result, root_schema, visited_refs);
        visited_refs.pop_back();
        return; // $ref replaces sibling keywords (Draft 7 §8.3)
    }

    // --- enum ---
    if (schema.contains("enum") && schema["enum"].is_array()) {
        bool found = false;
        for (const auto &e : schema["enum"]) {
            if (value == e) {
                found = true;
                break;
            }
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
        const auto &type_node = schema["type"];
        if (type_node.is_string()) {
            validateType(value, type_node.get<std::string>(), json_path, result);
        } else if (type_node.is_array()) {
            bool matched = false;
            for (const auto &t : type_node) {
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
        validateObject(value, schema, json_path, result, root_schema, visited_refs);
    } else if (value.is_array()) {
        validateArray(value, schema, json_path, result, root_schema, visited_refs);
    } else if (value.is_string()) {
        validateString(value, schema, json_path, result);
    } else if (value.is_number()) {
        validateNumber(value, schema, json_path, result);
    }

    // --- allOf ---
    if (schema.contains("allOf") && schema["allOf"].is_array()) {
        validateAllOf(value, schema["allOf"], json_path, result, root_schema, visited_refs);
    }

    // --- anyOf ---
    if (schema.contains("anyOf") && schema["anyOf"].is_array()) {
        validateAnyOf(value, schema["anyOf"], json_path, result, root_schema, visited_refs);
    }

    // --- oneOf ---
    if (schema.contains("oneOf") && schema["oneOf"].is_array()) {
        validateOneOf(value, schema["oneOf"], json_path, result, root_schema, visited_refs);
    }

    // --- not ---
    if (schema.contains("not") && schema["not"].is_object()) {
        validateNot(value, schema["not"], json_path, result, root_schema, visited_refs);
    }
}

// ═══════════════════════════════════════════════════════════
// validateType
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateType(const nlohmann::json &value, const std::string &expected_type,
                                         const std::string &json_path, ValidationResult &result) {
    if (!matchesType(value, expected_type)) {
        result.addError("Type mismatch at '" + json_path + "': expected '" + expected_type + "', got '"
                        + std::string(value.type_name()) + "'");
    }
}

// ═══════════════════════════════════════════════════════════
// validateObject
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateObject(const nlohmann::json &value, const nlohmann::json &schema,
                                           const std::string &json_path, ValidationResult &result,
                                           const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    // --- required ---
    if (schema.contains("required") && schema["required"].is_array()) {
        for (const auto &req : schema["required"]) {
            if (req.is_string() && !value.contains(req.get<std::string>())) {
                result.addError("Missing required property '" + req.get<std::string>() + "' at '" + json_path + "'");
            }
        }
    }

    // --- properties ---
    if (schema.contains("properties") && schema["properties"].is_object()) {
        const auto &props = schema["properties"];
        for (const auto &[key, prop_schema] : props.items()) {
            if (value.contains(key)) {
                validateValueImpl(value[key], prop_schema, json_path + "/" + key, result, root_schema, visited_refs);
            }
        }
    }

    // --- additionalProperties ---
    if (schema.contains("additionalProperties")) {
        const auto &ap = schema["additionalProperties"];
        std::vector<std::string> known_keys = {};

        if (schema.contains("properties") && schema["properties"].is_object()) {
            for (const auto &[k, _] : schema["properties"].items()) {
                known_keys.push_back(k);
            }
        }
        for (const auto &[key, val] : value.items()) {
            bool known = false;
            for (const auto &k : known_keys) {
                if (k == key) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                if (ap.is_boolean() && !ap.get<bool>()) {
                    result.addError("Additional property '" + key + "' is not allowed at '" + json_path + "'");
                } else if (ap.is_object()) {
                    validateValueImpl(val, ap, json_path + "/" + key, result, root_schema, visited_refs);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateArray
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateArray(const nlohmann::json &value, const nlohmann::json &schema,
                                          const std::string &json_path, ValidationResult &result,
                                          const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    // --- minItems ---
    if (schema.contains("minItems") && schema["minItems"].is_number_integer()) {
        std::size_t min = schema["minItems"].get<std::size_t>();
        if (value.size() < min) {
            result.addError("Array at '" + json_path + "' has " + std::to_string(value.size()) + " items, minimum is "
                            + std::to_string(min));
        }
    }

    // --- maxItems ---
    if (schema.contains("maxItems") && schema["maxItems"].is_number_integer()) {
        std::size_t max = schema["maxItems"].get<std::size_t>();
        if (static_cast<int>(value.size()) > max) {
            result.addError("Array at '" + json_path + "' has " + std::to_string(value.size()) + " items, maximum is "
                            + std::to_string(max));
        }
    }

    // --- items ---
    if (schema.contains("items") && schema["items"].is_object()) {
        const auto &item_schema = schema["items"];
        std::size_t idx         = 0;
        for (const auto &item : value) {
            validateValueImpl(item, item_schema, json_path + "/" + std::to_string(idx), result, root_schema,
                              visited_refs);
            ++idx;
        }
    }

    // --- uniqueItems ---
    if (schema.contains("uniqueItems") && schema["uniqueItems"].is_boolean() && schema["uniqueItems"].get<bool>()) {
        for (std::size_t i = 0; i < value.size(); ++i) {
            for (std::size_t j = i + 1; j < value.size(); ++j) {
                if (value[i] == value[j]) {
                    result.addError("Array at '" + json_path
                                    + "' must have unique items "
                                      "(duplicate at index "
                                    + std::to_string(i) + " and " + std::to_string(j) + ")");
                    return;
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateString
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateString(const nlohmann::json &value, const nlohmann::json &schema,
                                           const std::string &json_path, ValidationResult &result) {
    const std::string &s = value.get<std::string>();

    // --- minLength ---
    if (schema.contains("minLength") && schema["minLength"].is_number_integer()) {
        std::size_t min = schema["minLength"].get<std::size_t>();
        if (s.size() < min) {
            result.addError("String at '" + json_path + "' is too short (length " + std::to_string(s.size())
                            + ", minimum " + std::to_string(min) + ")");
        }
    }

    // --- maxLength ---
    if (schema.contains("maxLength") && schema["maxLength"].is_number_integer()) {
        std::size_t max = schema["maxLength"].get<std::size_t>();
        if (static_cast<int>(s.size()) > max) {
            result.addError("String at '" + json_path + "' is too long (length " + std::to_string(s.size())
                            + ", maximum " + std::to_string(max) + ")");
        }
    }

    // --- pattern ---
    if (schema.contains("pattern") && schema["pattern"].is_string()) {
        const std::string &pattern = schema["pattern"].get<std::string>();
        try {
            std::regex re(pattern);
            if (!std::regex_search(s, re)) {
                result.addError("String at '" + json_path + "' does not match pattern '" + pattern + "'");
            }
        } catch (const std::regex_error &e) {
            result.addWarning("Invalid pattern '" + pattern + "' in schema at '" + json_path + "': " + e.what());
        }
    }

    // --- format ---
    if (schema.contains("format") && schema["format"].is_string()) {
        const std::string &fmt = schema["format"].get<std::string>();
        bool format_valid      = true;
        try {
            if (fmt == "date") {
                // YYYY-MM-DD
                static const std::regex re_date(R"(^\d{4}-\d{2}-\d{2}$)");
                format_valid = std::regex_match(s, re_date);
            } else if (fmt == "date-time") {
                // RFC 3339 / ISO 8601: YYYY-MM-DDTHH:mm:ss with optional fractional seconds
                // and UTC offset.  Only 'T' is accepted as the separator (not space).
                static const std::regex re_datetime(
                    R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+\-]\d{2}:\d{2})?$)");
                format_valid = std::regex_match(s, re_datetime);
            } else if (fmt == "email") {
                // Simplified RFC 5321 local@domain check
                static const std::regex re_email(R"(^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$)");
                format_valid = std::regex_match(s, re_email);
            } else if (fmt == "uri") {
                // scheme ":" hier-part; must start with a valid scheme
                static const std::regex re_uri(R"(^[a-zA-Z][a-zA-Z0-9+\-.]*:.+$)");
                format_valid = std::regex_match(s, re_uri);
            } else if (fmt == "ipv4") {
                // Dotted-decimal, each octet 0-255
                static const std::regex re_ipv4(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
                std::smatch m = {};
                if (std::regex_match(s, m, re_ipv4)) {
                    for (int i = 1; i <= 4; ++i) {
                        if (std::stoi(m[i].str()) > 255) {
                            format_valid = false;
                            break;
                        }
                    }
                } else {
                    format_valid = false;
                }
            } else if (fmt == "ipv6") {
                // Simplified check: requires at least one colon and only hex digits plus colons.
                // Does not validate segment count, leading/trailing :: compression,
                // or embedded IPv4 addresses.  Rejects non-hex characters and plain IPv4.
                static const std::regex re_ipv6(R"(^[0-9a-fA-F:]+:[0-9a-fA-F:]*$)");
                format_valid = std::regex_match(s, re_ipv6);
            }
            // Unknown formats are silently accepted (informational keyword).
        } catch (const std::regex_error &) {
            // Internal pattern error — treat as valid to avoid false positives.
        }
        if (!format_valid) {
            result.addError("String at '" + json_path + "' does not conform to format '" + fmt + "'");
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateNumber
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateNumber(const nlohmann::json &value, const nlohmann::json &schema,
                                           const std::string &json_path, ValidationResult &result) {
    const double v = value.get<double>();

    // --- minimum ---
    if (schema.contains("minimum") && schema["minimum"].is_number()) {
        double min = schema["minimum"].get<double>();
        if (v < min) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) + ") is less than minimum ("
                            + std::to_string(min) + ")");
        }
    }

    // --- maximum ---
    if (schema.contains("maximum") && schema["maximum"].is_number()) {
        double max = schema["maximum"].get<double>();
        if (v > max) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v) + ") exceeds maximum ("
                            + std::to_string(max) + ")");
        }
    }

    // --- exclusiveMinimum (Draft 7: numeric) ---
    if (schema.contains("exclusiveMinimum") && schema["exclusiveMinimum"].is_number()) {
        double emin = schema["exclusiveMinimum"].get<double>();
        if (v <= emin) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v)
                            + ") must be strictly greater than exclusiveMinimum (" + std::to_string(emin) + ")");
        }
    }

    // --- exclusiveMaximum (Draft 7: numeric) ---
    if (schema.contains("exclusiveMaximum") && schema["exclusiveMaximum"].is_number()) {
        double emax = schema["exclusiveMaximum"].get<double>();
        if (v >= emax) {
            result.addError("Value at '" + json_path + "' (" + std::to_string(v)
                            + ") must be strictly less than exclusiveMaximum (" + std::to_string(emax) + ")");
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateAllOf
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateAllOf(const nlohmann::json &value, const nlohmann::json &schemas,
                                          const std::string &json_path, ValidationResult &result,
                                          const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    for (const auto &sub : schemas) {
        if (sub.is_object()) {
            validateValueImpl(value, sub, json_path, result, root_schema, visited_refs);
        }
    }
}

// ═══════════════════════════════════════════════════════════
// validateAnyOf
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateAnyOf(const nlohmann::json &value, const nlohmann::json &schemas,
                                          const std::string &json_path, ValidationResult &result,
                                          const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    for (const auto &sub : schemas) {
        if (sub.is_object()) {
            ValidationResult sub_result;
            validateValueImpl(value, sub, json_path, sub_result, root_schema, visited_refs);
            if (sub_result.valid) {
                return; // at least one sub-schema matched
            }
        }
    }
    result.addError("Value at '" + json_path + "' does not match any of the anyOf schemas");
}

// ═══════════════════════════════════════════════════════════
// validateOneOf
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateOneOf(const nlohmann::json &value, const nlohmann::json &schemas,
                                          const std::string &json_path, ValidationResult &result,
                                          const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    int matched = 0;
    for (const auto &sub : schemas) {
        if (sub.is_object()) {
            ValidationResult sub_result;
            validateValueImpl(value, sub, json_path, sub_result, root_schema, visited_refs);
            if (sub_result.valid) {
                ++matched;
            }
        }
    }
    if (matched != 1) {
        result.addError("Value at '" + json_path + "' must match exactly one of the oneOf schemas, but matched "
                        + std::to_string(matched));
    }
}

// ═══════════════════════════════════════════════════════════
// validateNot
// ═══════════════════════════════════════════════════════════

void ConfigSchemaValidator::validateNot(const nlohmann::json &value, const nlohmann::json &not_schema,
                                        const std::string &json_path, ValidationResult &result,
                                        const nlohmann::json &root_schema, std::vector<std::string> &visited_refs) {
    ValidationResult sub_result;
    validateValueImpl(value, not_schema, json_path, sub_result, root_schema, visited_refs);
    if (sub_result.valid) {
        result.addError("Value at '" + json_path + "' must NOT be valid against the 'not' schema");
    }
}

} // namespace config
} // namespace themis
