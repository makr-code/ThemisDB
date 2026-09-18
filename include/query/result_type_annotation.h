/**
 * @file result_type_annotation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// ResultFieldType
// ---------------------------------------------------------------------------

enum class ResultFieldType {
    UNKNOWN,   ///< Type could not be determined (empty result set)
    NULL_TYPE, ///< Field is always null
    BOOL,      ///< Boolean (true/false)
    INT,       ///< Integer number (int64-compatible)
    FLOAT,     ///< Floating-point number
    STRING,    ///< UTF-8 string
    ARRAY,     ///< JSON array
    OBJECT,    ///< JSON object / document
    VECTOR,    ///< Numeric array representing an embedding vector
};

/**
 * @brief Result Field Type Name.
 * @param[in] t Input parameter.
 * @return Return value.
 */
std::string resultFieldTypeName(ResultFieldType t);

// ---------------------------------------------------------------------------
// ResultFieldAnnotation
// ---------------------------------------------------------------------------

struct ResultFieldAnnotation {
    std::string        name;                              ///< Field / column name
    ResultFieldType    type       = ResultFieldType::UNKNOWN; ///< Inferred type
    bool               nullable   = false;                ///< True if null seen in any row
    bool               is_array   = false;                ///< True when type == ARRAY
    ResultFieldType    element_type = ResultFieldType::UNKNOWN; ///< Element type for arrays

    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// QueryResultSchema
// ---------------------------------------------------------------------------

struct QueryResultSchema {
    std::string                          query_type;  ///< e.g. "conjunctive", "or"
    std::vector<ResultFieldAnnotation>   fields;      ///< One entry per output field

    [[nodiscard]] const ResultFieldAnnotation* find(const std::string& name) const;

    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// AnnotatedQueryResult
// ---------------------------------------------------------------------------

struct AnnotatedQueryResult {
    nlohmann::json    result;  ///< The raw query result JSON
    QueryResultSchema schema;  ///< Inferred type schema
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

[[nodiscard]] ResultFieldType inferFieldType(const nlohmann::json& value);

[[nodiscard]] QueryResultSchema inferResultSchema(
    const nlohmann::json& rows,
    const std::string&    query_type = "unknown"
);

} // namespace query
} // namespace themis
