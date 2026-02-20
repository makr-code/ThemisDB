#include "utils/input_validator.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <regex>

namespace themis {
namespace utils {

InputValidator::InputValidator(std::string schema_dir)
    : schema_dir_(std::move(schema_dir)) {}

std::optional<nlohmann::json> InputValidator::loadSchema(const std::string& schema_name) const {
    try {
        std::string path = schema_dir_;
        if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
        path += schema_name + ".json";
        std::ifstream in(path);
        if (!in.good()) {
            return std::nullopt; // schema optional
        }
        std::stringstream buf;
        buf << in.rdbuf();
        auto j = nlohmann::json::parse(buf.str());
        return j;
    } catch (...) {
        return std::nullopt;
    }
}

static bool isAsciiControl(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return (uc < 0x20) || (uc == 0x7F);
}

std::string InputValidator::sanitizeForLogs(const std::string& input, size_t max_len) const {
    std::string out;
    out.reserve(std::min(input.size(), max_len));
    for (char c : input) {
        if (out.size() >= max_len) break;
        if (!isAsciiControl(c)) out.push_back(c);
    }
    return out;
}

bool InputValidator::validatePathSegment(const std::string& segment) const {
    if (segment.empty()) return false;
    if (segment.size() > 1024) return false; // arbitrary sane limit
    // Reject traversal or separators
    if (segment.find("..") != std::string::npos) return false;
    if (segment.find('/') != std::string::npos) return false;
    if (segment.find('\\') != std::string::npos) return false;
    if (segment.find('%') != std::string::npos) {
        // rudimentary: block encoded traversal attempts
        std::string lower = segment;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("%2e") != std::string::npos) return false;
    }
    // No control chars
    for (char c : segment) {
        if (isAsciiControl(c)) return false;
    }
    return true;
}

// Validate a single JSON value against a JSON Schema property descriptor.
// Supports: type, minLength, maxLength, minimum, maximum, exclusiveMinimum,
//           exclusiveMaximum, pattern, enum.
// Returns an error message on failure, std::nullopt on success.
static std::optional<std::string> validatePropertyConstraints(
    const std::string& field_name,
    const nlohmann::json& value,
    const nlohmann::json& prop)
{
    // --- type check ---
    if (prop.contains("type") && prop["type"].is_string()) {
        const std::string t = prop["type"].get<std::string>();
        bool type_ok = false;
        if      (t == "string")  type_ok = value.is_string();
        else if (t == "object")  type_ok = value.is_object();
        else if (t == "number")  type_ok = value.is_number();
        else if (t == "integer") type_ok = value.is_number_integer();
        else if (t == "boolean") type_ok = value.is_boolean();
        else if (t == "array")   type_ok = value.is_array();
        else if (t == "null")    type_ok = value.is_null();
        else                     type_ok = true; // unknown type – accept
        if (!type_ok) {
            return "field '" + field_name + "' must be " + t;
        }
    }

    // --- enum check ---
    if (prop.contains("enum") && prop["enum"].is_array()) {
        bool found = false;
        for (const auto& allowed : prop["enum"]) {
            if (value == allowed) { found = true; break; }
        }
        if (!found) {
            return "field '" + field_name + "' value not in allowed enum list";
        }
    }

    // --- string-specific constraints ---
    if (value.is_string()) {
        const std::string& s = value.get_ref<const std::string&>();
        if (prop.contains("minLength") && prop["minLength"].is_number_integer()) {
            auto min_len = prop["minLength"].get<size_t>();
            if (s.size() < min_len) {
                return "field '" + field_name + "' is shorter than minLength " +
                       std::to_string(min_len);
            }
        }
        if (prop.contains("maxLength") && prop["maxLength"].is_number_integer()) {
            auto max_len = prop["maxLength"].get<size_t>();
            if (s.size() > max_len) {
                return "field '" + field_name + "' exceeds maxLength " +
                       std::to_string(max_len);
            }
        }
        if (prop.contains("pattern") && prop["pattern"].is_string()) {
            try {
                std::regex re(prop["pattern"].get<std::string>(),
                              std::regex::ECMAScript | std::regex::optimize);
                if (!std::regex_search(s, re)) {
                    return "field '" + field_name + "' does not match required pattern";
                }
            } catch (const std::regex_error&) {
                return "schema error: invalid pattern for field '" + field_name + "'";
            }
        }
    }

    // --- numeric constraints ---
    if (value.is_number()) {
        double v = value.get<double>();
        if (prop.contains("minimum") && prop["minimum"].is_number()) {
            double mn = prop["minimum"].get<double>();
            if (v < mn) {
                return "field '" + field_name + "' is less than minimum " +
                       std::to_string(mn);
            }
        }
        if (prop.contains("maximum") && prop["maximum"].is_number()) {
            double mx = prop["maximum"].get<double>();
            if (v > mx) {
                return "field '" + field_name + "' exceeds maximum " +
                       std::to_string(mx);
            }
        }
        if (prop.contains("exclusiveMinimum") && prop["exclusiveMinimum"].is_number()) {
            double emn = prop["exclusiveMinimum"].get<double>();
            if (v <= emn) {
                return "field '" + field_name + "' must be > " + std::to_string(emn);
            }
        }
        if (prop.contains("exclusiveMaximum") && prop["exclusiveMaximum"].is_number()) {
            double emx = prop["exclusiveMaximum"].get<double>();
            if (v >= emx) {
                return "field '" + field_name + "' must be < " + std::to_string(emx);
            }
        }
    }

    return std::nullopt;
}

std::optional<std::string> InputValidator::validateJsonStub(
    const nlohmann::json& payload,
    const std::string& schema_name
) const {
    auto schema = loadSchema(schema_name);
    if (!schema.has_value()) {
        return std::nullopt; // no schema file present -> accept
    }
    try {
        if (!schema->is_object()) return std::string("invalid schema format");
        if (schema->contains("type") && (*schema)["type"].is_string()) {
            if ((*schema)["type"].get<std::string>() != "object") {
                return std::string("only top-level object schemas are supported");
            }
        }
        if (!payload.is_object()) return std::string("payload must be object");

        // --- required fields ---
        if (schema->contains("required") && (*schema)["required"].is_array()) {
            for (const auto& k : (*schema)["required"]) {
                if (!k.is_string()) continue;
                auto key = k.get<std::string>();
                if (!payload.contains(key)) {
                    return std::string("missing required field: ") + key;
                }
            }
        }

        // --- per-property constraints ---
        if (schema->contains("properties") && (*schema)["properties"].is_object()) {
            for (auto it = (*schema)["properties"].begin();
                 it != (*schema)["properties"].end(); ++it) {
                const std::string key = it.key();
                const auto& prop = it.value();
                if (!payload.contains(key)) continue;
                if (auto err = validatePropertyConstraints(key, payload.at(key), prop)) {
                    return err;
                }
            }
        }

        // --- additionalProperties: false ---
        if (schema->contains("additionalProperties") &&
            schema->at("additionalProperties").is_boolean() &&
            !schema->at("additionalProperties").get<bool>()) {
            if (schema->contains("properties") && (*schema)["properties"].is_object()) {
                const auto& props = (*schema)["properties"];
                for (const auto& [key, _] : payload.items()) {
                    if (!props.contains(key)) {
                        return "additional property not allowed: '" + key + "'";
                    }
                }
            }
        }

        return std::nullopt;
    } catch (...) {
        return std::string("schema validation error");
    }
}

std::optional<std::string> InputValidator::validateAqlRequest(const nlohmann::json& payload) const {
    // Basic structure
    if (!payload.is_object()) return std::string("AQL request must be a JSON object");
    if (!payload.contains("query") || !payload.at("query").is_string()) {
        return std::string("AQL request requires string field 'query'");
    }
    const std::string q = payload.at("query").get<std::string>();
    if (q.empty()) return std::string("AQL query must not be empty");
    if (q.size() > 100000) return std::string("AQL query too large (>100k)");

    // Disallow control characters (including NUL bytes)
    for (char c : q) {
        if (isAsciiControl(c) && c != '\n' && c != '\t' && c != '\r') {
            return std::string("AQL query contains control characters");
        }
    }
    // Very conservative blacklist (injection & multiple statements patterns)
    std::string lower = q; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find(";;") != std::string::npos) return std::string("multiple statement separator not allowed");
    
    // Disallow obvious DDL/DML tokens that don't belong in read-only endpoints
    static const char* forbidden[] = { "drop ", "truncate ", "alter ", "grant ", "revoke ", "create table", "insert ", "update ", "delete " };
    for (auto* f : forbidden) {
        if (lower.find(f) != std::string::npos) {
            return std::string("forbidden token in AQL query: '") + f + "'";
        }
    }
    // Optional: require bind variables to be an object if present
    if (payload.contains("bindVars") && !payload.at("bindVars").is_object()) {
        return std::string("'bindVars' must be an object");
    }

    // Pass minimal stub schema if available
    if (auto err = validateJsonStub(payload, "aql_request")) {
        return err;
    }

    return std::nullopt;
}

} // namespace utils
} // namespace themis
