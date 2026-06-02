/*
 * ThemisDB | File: schema_validator.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 207
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "importers/schema_validator.h"
#include <stdexcept>

#include <algorithm>
#include <cctype>

namespace themis {
namespace importers {

// ============================================================================
// Internal helpers
// ============================================================================

static std::string toLowerSchema(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
        out += static_cast<char>(std::tolower(c));
    return out;
}

static bool valueIsBoolean(const std::string& s) {
    std::string lower = toLowerSchema(s);
    return lower == "true" || lower == "false";
}

static bool valueIsInteger(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '+' || s[0] == '-') ++start;
    if (start == s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

static bool valueIsDouble(const std::string& s) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        (void)std::stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

// ============================================================================
// SchemaAutoDetector – static helpers
// ============================================================================

int SchemaAutoDetector::typeRank(DetectedFieldType t) {
    switch (t) {
        case DetectedFieldType::BOOLEAN: return 0;
        case DetectedFieldType::INTEGER: return 1;
        case DetectedFieldType::DOUBLE:  return 2;
        case DetectedFieldType::STRING:  return 3;
    }
    return 3;
}

DetectedFieldType SchemaAutoDetector::inferType(const std::string& value) {
    if (value.empty())          return DetectedFieldType::STRING;
    if (valueIsBoolean(value))  return DetectedFieldType::BOOLEAN;
    if (valueIsInteger(value))  return DetectedFieldType::INTEGER;
    if (valueIsDouble(value))   return DetectedFieldType::DOUBLE;
    return DetectedFieldType::STRING;
}

DetectedFieldType SchemaAutoDetector::widenType(DetectedFieldType a,
                                                  DetectedFieldType b) {
    return typeRank(b) > typeRank(a) ? b : a;
}

std::string SchemaAutoDetector::typeName(DetectedFieldType t) {
    switch (t) {
        case DetectedFieldType::BOOLEAN: return "boolean";
        case DetectedFieldType::INTEGER: return "integer";
        case DetectedFieldType::DOUBLE:  return "double";
        case DetectedFieldType::STRING:  return "string";
    }
    return "string";
}

DetectedFieldType SchemaAutoDetector::parseTypeName(const std::string& name) {
    std::string lower = toLowerSchema(name);
    if (lower == "boolean" || lower == "bool") return DetectedFieldType::BOOLEAN;
    if (lower == "integer" || lower == "int")  return DetectedFieldType::INTEGER;
    if (lower == "double"  || lower == "float" ||
        lower == "number"  || lower == "real")  return DetectedFieldType::DOUBLE;
    return DetectedFieldType::STRING;
}

json SchemaAutoDetector::schemaToJson(const DetectedSchema& schema) {
    json cols  = json::array();
    json types = json::object();
    for (const auto& col : schema.columns) {
        cols.push_back(col);
        auto it = schema.column_types.find(col);
        types[col] = (it != schema.column_types.end())
                         ? typeName(it->second)
                         : "string";
    }
    return json{
        {"name",         schema.table_name},
        {"columns",      cols},
        {"column_types", types},
        {"primary_keys", json::array()}
    };
}

std::vector<SchemaValidationError> SchemaAutoDetector::validateRow(
    const std::vector<std::string>& columns,
    const std::vector<std::string>& values,
    const DetectedSchema& schema)
{
    std::vector<SchemaValidationError> errors;
    const size_t n = std::min(columns.size(), values.size());
    for (size_t i = 0; i < n; ++i) {
        const auto& col = columns[i];
        const auto& val = values[i];
        if (val.empty()) continue;  // null / empty is always valid

        auto it = schema.column_types.find(col);
        if (it == schema.column_types.end()) continue;  // unknown column – skip

        DetectedFieldType expected = it->second;
        DetectedFieldType actual   = inferType(val);

        // A narrower-or-equal type is fine (e.g. INTEGER fits a DOUBLE column).
        if (typeRank(actual) > typeRank(expected)) {
            SchemaValidationError err;
            err.column        = col;
            err.value         = val;
            err.expected_type = expected;
            err.message = "Column '" + col + "': expected " +
                          typeName(expected) + " but got value '" + val + "'";
            errors.push_back(std::move(err));
        }
    }
    return errors;
}

// ============================================================================
// SchemaAutoDetector – instance methods
// ============================================================================

void SchemaAutoDetector::feedRow(const std::vector<std::string>& columns,
                                  const std::vector<std::string>& values) {
    // Initialise column list on first call
    if (columns_.empty()) {
        columns_ = columns;
        for (const auto& col : columns_) {
            // Start every column at BOOLEAN (narrowest) so the first real value
            // immediately establishes the actual type.
            widest_types_[col] = DetectedFieldType::BOOLEAN;
        }
    }

    const size_t n = std::min(columns.size(), values.size());
    for (size_t i = 0; i < n; ++i) {
        const auto& col = columns[i];
        const auto& val = values[i];
        if (val.empty()) continue;  // empty values don't narrow the type

        DetectedFieldType inferred = inferType(val);
        auto it = widest_types_.find(col);
        if (it == widest_types_.end()) {
            widest_types_[col] = inferred;
        } else {
            it->second = widenType(it->second, inferred);
        }
    }
}

DetectedSchema SchemaAutoDetector::getSchema(const std::string& table_name) const {
    DetectedSchema schema;
    schema.table_name   = table_name;
    schema.columns      = columns_;
    schema.column_types = widest_types_;

    // Columns with no non-empty values default to STRING
    for (const auto& col : columns_) {
        if (schema.column_types.find(col) == schema.column_types.end()) {
            schema.column_types[col] = DetectedFieldType::STRING;
        }
    }
    return schema;
}

void SchemaAutoDetector::reset() {
    columns_.clear();
    widest_types_.clear();
}

} // namespace importers
} // namespace themis

