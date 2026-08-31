/**
 * @file input_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/input_validator.h"
#include <stdexcept>
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <array>
#include <regex>
#include <mutex>
#include <unordered_set>

namespace themis {
namespace utils {

InputValidator::InputValidator()
    : schema_dir_() {}

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
    } catch (const nlohmann::json::exception &) {
        return std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    } catch (const std::string &) {
        return std::nullopt;
    } catch (const char *) {
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

    // Enforce conservative identifier format for path-like IDs.
    // Allowed: ASCII alnum, underscore, hyphen.
    for (char c : segment) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (!(std::isalnum(uc) || c == '_' || c == '-')) {
            return false;
        }
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

// static
std::optional<std::string> InputValidator::validateJson(
    const nlohmann::json& payload,
    const nlohmann::json& schema
) {
    try {
        if (!schema.is_object()) return std::string("invalid schema format");
        if (schema.contains("type") && schema["type"].is_string()) {
            if (schema["type"].get<std::string>() != "object") {
                return std::string("only top-level object schemas are supported");
            }
        }
        if (!payload.is_object()) return std::string("payload must be object");

        // --- required fields ---
        if (schema.contains("required") && schema["required"].is_array()) {
            for (const auto& k : schema["required"]) {
                if (!k.is_string()) continue;
                auto key = k.get<std::string>();
                if (!payload.contains(key)) {
                    return std::string("missing required field: ") + key;
                }
            }
        }

        // --- per-property constraints ---
        if (schema.contains("properties") && schema["properties"].is_object()) {
            for (auto it = schema["properties"].begin();
                 it != schema["properties"].end(); ++it) {
                const std::string key = it.key();
                const auto& prop = it.value();
                if (!payload.contains(key)) continue;
                if (auto err = validatePropertyConstraints(key, payload.at(key), prop)) {
                    return err;
                }
            }
        }

        // --- additionalProperties: false ---
        if (schema.contains("additionalProperties") &&
            schema.at("additionalProperties").is_boolean() &&
            !schema.at("additionalProperties").get<bool>()) {
            if (schema.contains("properties") && schema["properties"].is_object()) {
                const auto& props = schema["properties"];
                for (const auto& [key, _] : payload.items()) {
                    if (!props.contains(key)) {
                        return "additional property not allowed: '" + key + "'";
                    }
                }
            }
        }

        return std::nullopt;
    } catch (const nlohmann::json::exception &) {
        return std::string("schema validation error");
    } catch (const std::exception &) {
        return std::string("schema validation error");
    } catch (const std::string &) {
        return std::string("schema validation error");
    } catch (const char *) {
        return std::string("schema validation error");
    }
}

std::optional<std::string> InputValidator::validateJsonSchema(
    const nlohmann::json& payload,
    const std::string& schema_name
) const {
    auto schema = loadSchema(schema_name);
    if (!schema.has_value()) {
        // Fail-closed: reject the request.  Warn once per unique schema_name to
        // avoid log spam when schemas are intentionally not deployed in an env.
        // Thread-safety: C++17 §9.7[stmt.dcl]p4 (formerly C++11 §6.7[stmt.dcl]p4)
        // guarantees that the initialization of a block-scope static variable is
        // performed exactly once, even under concurrent access ("magic statics").
        // The mutex then serializes all subsequent accesses to s_warned_schemas.
        // emplace() combines lookup and insert atomically under the lock.
        {
            static std::mutex s_warned_mutex;
            static std::unordered_set<std::string> s_warned_schemas;
            std::lock_guard<std::mutex> lock(s_warned_mutex);
            if (s_warned_schemas.emplace(schema_name).second) {
                THEMIS_WARN("InputValidator::validateJsonStub: schema '{}' not found — "
                            "expected file: '{}/{}.json'.  "
                            "Place the JSON Schema file in that directory or set "
                            "THEMIS_SCHEMA_DIR to the correct path.  "
                            "Request rejected (fail-closed).  "
                            "(Subsequent occurrences for this schema are suppressed.)",
                            schema_name, schema_dir_, schema_name);
            }

        }
        return std::string("schema '" + schema_name + "' not found in '" +
                           schema_dir_ + "' — validation failed");
    }
    return validateJson(payload, *schema);
}

std::optional<std::string> InputValidator::validateJsonStub(
    const nlohmann::json& payload,
    const std::string& schema_name
) const {
    return validateJsonSchema(payload, schema_name);
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
    if (auto err = validateJsonSchema(payload, "aql_request")) {
        return err;
    }

    return std::nullopt;
}

// =============================================================================
// Security-focused validation methods
// =============================================================================

// Named limits used across multiple security validators
static constexpr size_t kMaxAqlQuerySize  = 100000;  // 100 KB
static constexpr size_t kMaxFilePathSize  =   4096;  // POSIX PATH_MAX
static constexpr size_t kMaxFilenameSize  =    255;  // POSIX NAME_MAX
static constexpr size_t kMaxUrlSize       =   2048;  // common browser limit

// O(1) lookup table for shell-injection metacharacter detection.
// Flags: ; | & ` $ ( ) { } [ ] < > ! ? * " \ '
static const std::array<bool, 256>& filenameMetacharTable() {
    static const std::array<bool, 256> tbl = [] {
        std::array<bool, 256> t{};
        for (unsigned char c : std::string(";|&`$(){}[]<>!?*\"\\'")) {
            t[c] = true;
        }
        return t;
    }();
    return tbl;
}

bool InputValidator::validateAQLQuery(const std::string& query) const {
    if (query.empty() || query.size() > kMaxAqlQuerySize) return false;

    std::string lower = query;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Reject comment markers used to truncate query conditions
    if (lower.find("--") != std::string::npos) return false;
    if (lower.find("/*") != std::string::npos) return false;

    // Reject double-slash comment markers (not valid AQL, signals injection attempt)
    // Note: avoid rejecting valid http:// in string literals by checking for ' //'
    if (lower.find("; //") != std::string::npos) return false;

    // Reject dangerous write/DDL operations
    static const char* const dangerous_ops[] = {
        " drop ", "; drop", "' drop", " remove ", "; remove", "' remove",
        " truncate ", " insert ", " update ", " delete ",
        " union ", "' union",
    };
    for (const char* op : dangerous_ops) {
        if (lower.find(op) != std::string::npos) return false;
    }
    // Also catch patterns like '; DROP ... (leading semicolon)
    if (lower.rfind("';", 0) != std::string::npos ||
        lower.find("'; drop") != std::string::npos ||
        lower.find("'; remove") != std::string::npos ||
        lower.find("'; ") != std::string::npos) {
        return false;
    }

    // Detect boolean injection: OR '...'='...' pattern
    {
        static const std::regex bool_inject_str(
            R"([\s']or\s+'[^']*'\s*=\s*'[^']*')", std::regex::icase);
        if (std::regex_search(lower, bool_inject_str)) return false;
    }
    // Detect boolean injection: OR <num>=<num> pattern
    {
        static const std::regex bool_inject_num(
            R"(\bor\s+\d+\s*=\s*\d+)", std::regex::icase);
        if (std::regex_search(lower, bool_inject_num)) return false;
    }

    return true;
}

bool InputValidator::validateFilePath(const std::string& path) const {
    if (path.empty() || path.size() > kMaxFilePathSize) return false;

    // Reject directory traversal sequences
    if (path.find("..") != std::string::npos) return false;

    // Reject double-slash (also catches file://)
    if (path.find("//") != std::string::npos) return false;

    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Reject URL-encoded traversal (%2e = '.', %2f = '/')
    if (lower.find("%2e") != std::string::npos) return false;
    if (lower.find("%2f") != std::string::npos) return false;
    // Reject double-encoded percent sign (e.g. %252f)
    if (lower.find("%25") != std::string::npos) return false;

    // Reject dangerous protocol schemes
    if (lower.find("file:") != std::string::npos) return false;

    // Reject access to sensitive kernel/OS paths
    if (lower.find("/proc/") != std::string::npos) return false;
    if (lower.find("/sys/") != std::string::npos) return false;

    return true;
}

std::string InputValidator::sanitizeForHTML(const std::string& input) const {
    // Step 1: remove dangerous protocol schemes (javascript:, vbscript:)
    std::string tmp = input;
    {
        static const std::regex js_proto(R"(javascript\s*:)", std::regex::icase);
        tmp = std::regex_replace(tmp, js_proto, "");
    }
    {
        static const std::regex vbs_proto(R"(vbscript\s*:)", std::regex::icase);
        tmp = std::regex_replace(tmp, vbs_proto, "");
    }
    // Step 2: remove on* event-handler attributes (onerror=, onload=, etc.)
    {
        static const std::regex event_handler(R"(\bon\w+\s*=)", std::regex::icase);
        tmp = std::regex_replace(tmp, event_handler, "");
    }
    // Step 3: HTML-encode remaining special characters
    std::string result;
    result.reserve(static_cast<size_t>(tmp.size() * 1.2));
    for (char c : tmp) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#x27;"; break;
            case '/':  result += "&#x2F;"; break;
            default:   result += c;        break;
        }
    }
    return result;
}

bool InputValidator::validateFilename(const std::string& filename) const {
    if (filename.empty() || filename.size() > kMaxFilenameSize) return false;

    // Reject shell metacharacters that enable command injection (O(1) lookup)
    const auto& meta = filenameMetacharTable();
    for (char c : filename) {
        if (meta[static_cast<unsigned char>(c)]) return false;
        if (static_cast<unsigned char>(c) < 0x20) return false; // control chars
    }

    // Reject traversal sequences
    if (filename.find("..") != std::string::npos) return false;

    return true;
}

bool InputValidator::validateJSON(const std::string& input) const {
    // Reject MongoDB operator injection patterns
    static const char* const mongo_ops[] = {
        "$gt", "$gte", "$lt", "$lte", "$ne", "$in", "$nin",
        "$or", "$and", "$not", "$nor", "$where", "$regex",
        "$exists", "$type", "$mod", "$text", "$elemMatch"
    };
    for (const char* op : mongo_ops) {
        if (input.find(op) != std::string::npos) return false;
    }

    // Reject JavaScript injection patterns (case-insensitive)
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("return true") != std::string::npos) return false;
    if (input.find("|| ") != std::string::npos) return false;

    return true;
}

bool InputValidator::validateXML(const std::string& input) const {
    std::string upper = input;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // Reject DOCTYPE and ENTITY declarations used for XXE
    if (upper.find("<!DOCTYPE") != std::string::npos) return false;
    if (upper.find("<!ENTITY") != std::string::npos) return false;

    // Reject external resource references
    if (upper.find("SYSTEM") != std::string::npos) return false;
    if (upper.find("PUBLIC") != std::string::npos) return false;

    return true;
}

bool InputValidator::validateLDAPFilter(const std::string& input) const {
    // Reject unescaped LDAP filter metacharacters: * ( ) \ NUL
    for (char c : input) {
        if (c == '*' || c == '(' || c == ')' || c == '\\' || c == '\0') {
            return false;
        }
    }
    return true;
}

bool InputValidator::validateEmail(const std::string& email) const {
    // Reject CRLF injection sequences that enable header injection
    if (email.find('\n') != std::string::npos) return false;
    if (email.find('\r') != std::string::npos) return false;

    // Reject URL-encoded CRLF
    std::string lower = email;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("%0a") != std::string::npos) return false;
    if (lower.find("%0d") != std::string::npos) return false;

    // Basic structural check: must have exactly one '@' not at start/end
    size_t at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0) return false;
    if (at_pos == email.size() - 1) return false;
    if (email.find('@', at_pos + 1) != std::string::npos) return false;

    // Must have a '.' after the '@'
    if (email.find('.', at_pos) == std::string::npos) return false;

    return true;
}

bool InputValidator::validateURL(const std::string& url,
                                  const std::vector<std::string>& allowed_schemes) const {
    if (url.empty() || url.size() > kMaxUrlSize) return false;

    // Reject protocol-relative URLs
    if (url.size() >= 2 && url[0] == '/' && url[1] == '/') return false;

    // Extract scheme
    size_t scheme_sep = url.find("://");
    std::string scheme;
    std::string rest_after_scheme;

    if (scheme_sep != std::string::npos) {
        scheme = url.substr(0, scheme_sep);
        rest_after_scheme = url.substr(scheme_sep + 3);
    } else {
        // No "://" – look for a bare colon (e.g. "javascript:alert()")
        size_t colon = url.find(':');
        if (colon == std::string::npos) return false; // no scheme at all
        scheme = url.substr(0, colon);
        rest_after_scheme = url.substr(colon + 1);
    }

    // Lower-case the scheme for comparison
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

    // Validate scheme against whitelist
    bool scheme_ok = false;
    for (const auto& s : allowed_schemes) {
        std::string ls = s;
        std::transform(ls.begin(), ls.end(), ls.begin(), ::tolower);
        if (scheme == ls) { scheme_ok = true; break; }
    }
    if (!scheme_ok) return false;

    // If we had "://", check the authority part
    if (scheme_sep != std::string::npos) {
        size_t path_start = rest_after_scheme.find('/');
        size_t query_start = rest_after_scheme.find('?');
        size_t auth_end = std::min(path_start, query_start);

        std::string authority = (auth_end != std::string::npos)
                                    ? rest_after_scheme.substr(0, auth_end)
                                    : rest_after_scheme;

        // Reject user-info in authority (phishing / credential confusion)
        if (authority.find('@') != std::string::npos) return false;

        // Check query string for embedded redirect URLs (open redirect)
        if (query_start != std::string::npos) {
            std::string query = rest_after_scheme.substr(query_start + 1);
            std::string qlower = query;
            std::transform(qlower.begin(), qlower.end(), qlower.begin(), ::tolower);
            if (qlower.find("=http://")  != std::string::npos) return false;
            if (qlower.find("=https://") != std::string::npos) return false;
            if (qlower.find("=%2f%2f")   != std::string::npos) return false;
        }
    }

    return true;
}

bool InputValidator::validateStringLength(const std::string& input, size_t max_len) const {
    return input.size() <= max_len;
}

bool InputValidator::validateIntegerRange(int64_t value,
                                           int64_t min_val,
                                           int64_t max_val) const {
    return value >= min_val && value <= max_val;
}

std::string InputValidator::sanitizeLogMessage(const std::string& input) const {
    // Remove %n and %N format specifiers – these can write to arbitrary memory
    // if the string is ever passed as a printf format argument.
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == 'n' || next == 'N') {
                ++i; // skip the 'n'/'N' character; don't emit either byte
                continue;
            }
        }
        result += input[i];
    }
    return result;
}

std::string InputValidator::normalizeUnicode(const std::string& input) const {
    // Normalize full-width Unicode characters (U+FF01..U+FF5E) to their ASCII
    // equivalents (U+0021..U+007E) so that hidden injection patterns become
    // visible.  These characters are encoded in UTF-8 as three bytes:
    //   U+FF01..U+FF3F -> EF BC 81..EF BC BF  (maps to ASCII 0x21..0x5F)
    //   U+FF40..U+FF5E -> EF BD 80..EF BD 9E  (maps to ASCII 0x60..0x7E)
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ) {
        auto byte = static_cast<unsigned char>(input[i]);

        if (byte == 0xEF && i + 2 < input.size()) {
            auto b2 = static_cast<unsigned char>(input[i + 1]);
            auto b3 = static_cast<unsigned char>(input[i + 2]);

            if (b2 == 0xBC && b3 >= 0x81 && b3 <= 0xBF) {
                // U+FF01..U+FF3F -> ASCII 0x21..0x5F
                result += static_cast<char>(b3 - 0x81 + 0x21);
                i += 3;
                continue;
            }
            if (b2 == 0xBD && b3 >= 0x80 && b3 <= 0x9E) {
                // U+FF40..U+FF5E -> ASCII 0x60..0x7E
                result += static_cast<char>(b3 - 0x80 + 0x60);
                i += 3;
                continue;
            }
        }

        result += input[i];
        ++i;
    }

    return result;
}

bool InputValidator::validateHeaderValue(const std::string& value) const {
    // Reject raw CRLF characters
    if (value.find('\n') != std::string::npos) return false;
    if (value.find('\r') != std::string::npos) return false;

    // Reject null bytes.  std::string::find() searches by value within the
    // full string length (not limited by NUL termination), so embedded NULs
    // in a std::string are correctly detected here.
    if (value.find('\0') != std::string::npos) return false;

    // Reject URL-encoded CRLF sequences
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("%0d") != std::string::npos) return false;
    if (lower.find("%0a") != std::string::npos) return false;

    return true;
}

} // namespace utils
} // namespace themis
