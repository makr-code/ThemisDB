/**
 * @file result_type_annotation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/result_type_annotation.h"

#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// resultFieldTypeName
// ---------------------------------------------------------------------------

std::string resultFieldTypeName(ResultFieldType t) {
    switch (t) {
        case ResultFieldType::UNKNOWN:   return "UNKNOWN";
        case ResultFieldType::NULL_TYPE: return "NULL";
        case ResultFieldType::BOOL:      return "BOOL";
        case ResultFieldType::INT:       return "INT";
        case ResultFieldType::FLOAT:     return "FLOAT";
        case ResultFieldType::STRING:    return "STRING";
        case ResultFieldType::ARRAY:     return "ARRAY";
        case ResultFieldType::OBJECT:    return "OBJECT";
        case ResultFieldType::VECTOR:    return "VECTOR";
    }
    return "UNKNOWN"; // unreachable, but suppresses warnings
}

// ---------------------------------------------------------------------------
// ResultFieldAnnotation::toJson
// ---------------------------------------------------------------------------

nlohmann::json ResultFieldAnnotation::toJson() const {
    nlohmann::json j;
    j["name"]     = name;
    j["type"]     = resultFieldTypeName(type);
    j["nullable"] = nullable;
    if (is_array) {
        j["element_type"] = resultFieldTypeName(element_type);
    }
    return j;
}

// ---------------------------------------------------------------------------
// QueryResultSchema
// ---------------------------------------------------------------------------

const ResultFieldAnnotation* QueryResultSchema::find(const std::string& n) const {
    for (const auto& f : fields) {
        if (f.name == n) return &f;
    }
    return nullptr;
}

nlohmann::json QueryResultSchema::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& f : fields) {
        arr.push_back(f.toJson());
    }
    return nlohmann::json{{"query_type", query_type}, {"fields", arr}};
}

// ---------------------------------------------------------------------------
// inferFieldType
// ---------------------------------------------------------------------------

ResultFieldType inferFieldType(const nlohmann::json& value) {
    if (value.is_null())    return ResultFieldType::NULL_TYPE;
    if (value.is_boolean()) return ResultFieldType::BOOL;
    if (value.is_string())  return ResultFieldType::STRING;
    if (value.is_object())  return ResultFieldType::OBJECT;

    if (value.is_number()) {
        if (value.is_number_integer()) return ResultFieldType::INT;
        // Floating-point: classify as INT when the fractional part is zero
        double d = value.get<double>();
        if (std::isfinite(d) && d == std::floor(d) &&
            d >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
            d <= static_cast<double>(std::numeric_limits<int64_t>::max()))
        {
            return ResultFieldType::INT;
        }
        return ResultFieldType::FLOAT;
    }

    if (value.is_array()) {
        // A homogeneous numeric array with >1 elements is a VECTOR (embedding)
        if (!value.empty() && value.size() > 1) {
            bool all_numeric = true;
            for (const auto& elem : value) {
                if (!elem.is_number()) { all_numeric = false; break; }
            }
            if (all_numeric) return ResultFieldType::VECTOR;
        }
        return ResultFieldType::ARRAY;
    }

    return ResultFieldType::UNKNOWN;
}

// ---------------------------------------------------------------------------
// inferResultSchema  (helper: merge / promote types)
// ---------------------------------------------------------------------------

namespace {

/**
 * Promote two types to their common supertype.
 * Rules (in priority order):
 *   - VECTOR beats ARRAY (more specific)
 *   - FLOAT beats INT (widening)
 *   - anything beats UNKNOWN
 *   - anything beats NULL_TYPE (marks nullable)
 *   - different non-null types → UNKNOWN (mixed type)
 */
ResultFieldType promoteType(ResultFieldType a, ResultFieldType b) {
    if (a == b)                         return a;
    if (a == ResultFieldType::UNKNOWN)  return b;
    if (b == ResultFieldType::UNKNOWN)  return a;
    if (a == ResultFieldType::NULL_TYPE) return b;
    if (b == ResultFieldType::NULL_TYPE) return a;
    // INT + FLOAT → FLOAT
    if ((a == ResultFieldType::INT  && b == ResultFieldType::FLOAT) ||
        (a == ResultFieldType::FLOAT && b == ResultFieldType::INT))
        return ResultFieldType::FLOAT;
    // ARRAY + VECTOR → VECTOR
    if ((a == ResultFieldType::ARRAY  && b == ResultFieldType::VECTOR) ||
        (a == ResultFieldType::VECTOR && b == ResultFieldType::ARRAY))
        return ResultFieldType::VECTOR;
    // Incompatible → fall back to UNKNOWN
    return ResultFieldType::UNKNOWN;
}

/**
 * Infer element type for array fields by scanning the first non-null row that
 * has the field as an array with at least one element.
 */
ResultFieldType inferElementType(const nlohmann::json& rows, const std::string& field) {
    for (const auto& row : rows) {
        if (!row.is_object()) continue;
        auto it = row.find(field);
        if (it == row.end() || !it->is_array() || it->empty()) continue;
        return inferFieldType((*it)[0]);
    }
    return ResultFieldType::UNKNOWN;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// inferResultSchema
// ---------------------------------------------------------------------------

QueryResultSchema inferResultSchema(const nlohmann::json& rows,
                                    const std::string&    query_type) {
    QueryResultSchema schema;
    schema.query_type = query_type;

    if (!rows.is_array() || rows.empty()) {
        return schema;
    }

    // Pass 1: collect field names and accumulate types
    // We use a vector (insertion-ordered) to preserve column order.
    std::vector<std::string>                        field_order;
    std::unordered_map<std::string, ResultFieldType> field_types;
    std::unordered_set<std::string>                  nullable_fields;
    std::unordered_set<std::string>                  seen_fields;

    for (const auto& row : rows) {
        if (!row.is_object()) continue;

        for (auto it = row.begin(); it != row.end(); ++it) {
            const std::string& fname = it.key();

            if (seen_fields.find(fname) == seen_fields.end()) {
                seen_fields.insert(fname);
                field_order.push_back(fname);
                field_types[fname] = ResultFieldType::UNKNOWN;
            }

            ResultFieldType vt = inferFieldType(it.value());
            if (vt == ResultFieldType::NULL_TYPE) {
                nullable_fields.insert(fname);
            }
            field_types[fname] = promoteType(field_types[fname], vt);
        }
    }

    // Pass 2: mark fields that are absent in some rows as nullable
    for (const auto& row : rows) {
        if (!row.is_object()) continue;
        for (const auto& fname : field_order) {
            if (row.find(fname) == row.end()) {
                nullable_fields.insert(fname);
            }
        }
    }

    // Pass 3: build ResultFieldAnnotation list
    for (const auto& fname : field_order) {
        ResultFieldAnnotation ann;
        ann.name     = fname;
        ann.type     = field_types[fname];
        ann.nullable = (nullable_fields.count(fname) > 0);

        if (ann.type == ResultFieldType::ARRAY ||
            ann.type == ResultFieldType::VECTOR) {
            ann.is_array    = true;
            ann.element_type = inferElementType(rows, fname);
        }

        schema.fields.push_back(std::move(ann));
    }

    return schema;
}

} // namespace query
} // namespace themis
