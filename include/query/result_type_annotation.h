/**
 * @file result_type_annotation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Data type of a single field in a query result row.
 *
 * Maps to the common type vocabulary used by client SDK generators
 * (TypeScript, Python, Go, etc.).
 */
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

/** @brief Human-readable name for a ResultFieldType (used in JSON schema). */
std::string resultFieldTypeName(ResultFieldType t);

// ---------------------------------------------------------------------------
// ResultFieldAnnotation
// ---------------------------------------------------------------------------

/**
 * @brief Type annotation for a single output field of a query.
 *
 * Records the field name, its inferred data type, whether it can be null,
 * and – for ARRAY fields – the element type.
 */
struct ResultFieldAnnotation {
    std::string        name;                              ///< Field / column name
    ResultFieldType    type       = ResultFieldType::UNKNOWN; ///< Inferred type
    bool               nullable   = false;                ///< True if null seen in any row
    bool               is_array   = false;                ///< True when type == ARRAY
    ResultFieldType    element_type = ResultFieldType::UNKNOWN; ///< Element type for arrays

    /**
     * @brief Serialise this annotation to a JSON object.
     *
     * Format:
     * @code
     * { "name": "age", "type": "INT", "nullable": false }
     * @endcode
     */
    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// QueryResultSchema
// ---------------------------------------------------------------------------

/**
 * @brief Complete type schema for a query result set.
 *
 * Contains one @ref ResultFieldAnnotation per output field together with
 * the query type string (e.g. "conjunctive", "or", "vector_geo").
 *
 * Clients can serialise this to JSON and feed it into a code generator to
 * produce language-specific typed result classes.
 */
struct QueryResultSchema {
    std::string                          query_type;  ///< e.g. "conjunctive", "or"
    std::vector<ResultFieldAnnotation>   fields;      ///< One entry per output field

    /**
     * @brief Find a field annotation by name.
     * @return Pointer to the annotation, or nullptr if not found.
     */
    [[nodiscard]] const ResultFieldAnnotation* find(const std::string& name) const;

    /**
     * @brief Serialise the full schema to JSON.
     *
     * Format:
     * @code
     * {
     *   "query_type": "conjunctive",
     *   "fields": [
     *     { "name": "id",   "type": "STRING",  "nullable": false },
     *     { "name": "age",  "type": "INT",     "nullable": false },
     *     { "name": "score","type": "FLOAT",   "nullable": true  }
     *   ]
     * }
     * @endcode
     */
    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// AnnotatedQueryResult
// ---------------------------------------------------------------------------

/**
 * @brief A query result bundled with its inferred type schema.
 *
 * Returned by @ref executeAqlAnnotated().  The @c result member holds the
 * same JSON payload that @ref executeAql() would return; @c schema carries
 * the type annotations suitable for client SDK code generation.
 */
struct AnnotatedQueryResult {
    nlohmann::json    result;  ///< The raw query result JSON
    QueryResultSchema schema;  ///< Inferred type schema
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Infer the type of a single JSON value.
 *
 * A numeric JSON value is classified as INT when it is an integer or a float
 * whose fractional part is zero; otherwise it is FLOAT.  A homogeneous
 * all-numeric array with more than one element is tagged as VECTOR.
 */
[[nodiscard]] ResultFieldType inferFieldType(const nlohmann::json& value);

/**
 * @brief Infer a QueryResultSchema from an array of result rows.
 *
 * @param rows       JSON array where each element is an object (result row).
 * @param query_type String label for the query type (default "unknown").
 * @return The inferred schema.  Fields that appear in some rows but not
 *         others are marked nullable.
 *
 * @note If @p rows is empty or contains non-object rows the returned schema
 *       will have no fields.
 */
[[nodiscard]] QueryResultSchema inferResultSchema(
    const nlohmann::json& rows,
    const std::string&    query_type = "unknown"
);

} // namespace query
} // namespace themis
