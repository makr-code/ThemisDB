/**
 * @file schema_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: schema_validator.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 176
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Detected field type for schema auto-detection.
 *
 * Types are ordered by specificity (rank): BOOLEAN < INTEGER < DOUBLE < STRING.
 * When merging types across multiple sample rows the widest (highest-rank) type
 * wins so that a column initially inferred as INTEGER that later has a float
 * value is widened to DOUBLE, and one that has a non-numeric value becomes STRING.
 */
enum class DetectedFieldType {
    BOOLEAN,  ///< "true" / "false" (case-insensitive)
    INTEGER,  ///< Whole number (no decimal point)
    DOUBLE,   ///< Floating-point number
    STRING    ///< Everything else (or empty / null)
};

/**
 * @brief Schema detected (or declared) for a single logical table.
 */
struct DetectedSchema {
    std::string              table_name;
    std::vector<std::string> columns;
    std::map<std::string, DetectedFieldType> column_types;
};

/**
 * @brief Null handling policy for columns during validation.
 *
 * PHASE-2-HARDENING: Deterministic null handling across all validators.
 */
enum class NullHandlingPolicy {
    NULLABLE,           ///< NULL accepted, transformed to sentinel value
    NON_NULLABLE        ///< NULL rejected (error IMPORT_ROW_INVALID)
};

/**
 * @brief Category of constraint violation during validation.
 *
 * PHASE-2-HARDENING: Constraint violation categorization for structured error reporting.
 */
enum class ConstraintViolationType {
    TYPE_MISMATCH,      ///< Value type does not match column type
    LENGTH_VIOLATION,   ///< String/binary exceeds max length
    FOREIGN_KEY_VIOLATION,  ///< Value does not match referenced table
    UNIQUE_VIOLATION,   ///< Value violates unique constraint
    NONE                ///< No violation
};

/**
 * @brief Describes a type mismatch for one field in a validated row.
 *
 * PHASE-2-HARDENING: Extended with constraint violation categorization.
 */
struct SchemaValidationError {
    std::string       column;             ///< Column name where the mismatch occurred
    std::string       value;              ///< Actual string value that failed validation
    DetectedFieldType expected_type;      ///< Type declared in the schema
    std::string       message;            ///< Human-readable description
    
    // PHASE-2-HARDENING: Constraint violation categorization
    ConstraintViolationType violation_type{ConstraintViolationType::NONE};
    std::string       error_code;         ///< Structured error code (e.g., IMPORT_ROW_INVALID)
};

/**
 * @brief Configuration for type coercion during validation.
 *
 * PHASE-2-HARDENING: Type coercion with strict bounds checking.
 */
struct TypeCoercionConfig {
    // String constraints
    static constexpr size_t kMaxStringFieldLength = 4096;  ///< Max 4KB per string field
    
    // Numeric constraints
    double numeric_min_value{-1e308};     ///< Minimum numeric value (IEEE double)
    double numeric_max_value{1e308};      ///< Maximum numeric value (IEEE double)
    
    // Date format validation
    bool validate_iso8601{true};          ///< Enforce ISO8601 or RFC3339 for dates
};


/**
 * @brief Auto-detects column types from sampled string values.
 *
 * Feed data rows via feedRow(); then call getSchema() to retrieve the
 * inferred schema.  Type inference follows the widening rule:
 *   BOOLEAN → INTEGER → DOUBLE → STRING
 * Empty / null values do not affect the inferred type of a column.
 *
 * Example:
 * @code
 *   SchemaAutoDetector det;
 *   det.feedRow({"id", "name", "score"}, {"1",  "Alice", "9.5"});
 *   det.feedRow({"id", "name", "score"}, {"2",  "Bob",   "7"});
 *   auto schema = det.getSchema("users");
 *   // schema.column_types["id"]    == DetectedFieldType::INTEGER
 *   // schema.column_types["score"] == DetectedFieldType::DOUBLE
 *   // schema.column_types["name"]  == DetectedFieldType::STRING
 * @endcode
 */
class SchemaAutoDetector {
public:
    SchemaAutoDetector() = default;

    /**
     * @brief Feed one data row to update the running type inference.
     *
     * The first call that contains columns seeds the column list; subsequent
     * calls widen the per-column type when necessary.
     *
     * @param columns  Column names in the same order as @p values.
     * @param values   Raw string values for each column.
     */
    void feedRow(const std::vector<std::string>& columns,
                 const std::vector<std::string>& values);

    /**
     * @brief Return the schema inferred from all rows fed so far.
     * @param table_name  Logical table name to embed in the result.
     */
    DetectedSchema getSchema(const std::string& table_name) const;

    /**
     * @brief Reset the detector state so it can be reused for a new table.
     */
    void reset();

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Infer the DetectedFieldType of a single string value.
     *
     * Empty string returns STRING (cannot determine type from empty input).
     */
    static DetectedFieldType inferType(const std::string& value);

    /**
     * @brief Return the wider of two types.
     *
     * Widening order: BOOLEAN < INTEGER < DOUBLE < STRING.
     */
    static DetectedFieldType widenType(DetectedFieldType a, DetectedFieldType b);

    /**
     * @brief Convert a DetectedFieldType to its lowercase name string.
     *
     * Returns "boolean", "integer", "double", or "string".
     */
    static std::string typeName(DetectedFieldType t);

    /**
     * @brief Parse a type name string back to a DetectedFieldType.
     *
     * Accepts "boolean", "integer", "double", "float", "string".
     * Unknown values map to STRING.
     */
    static DetectedFieldType parseTypeName(const std::string& name);

    /**
     * @brief Serialize a DetectedSchema to JSON in the standard importer format.
     *
     * Output shape (matches IImporter::getSourceSchema()):
     * @code
     *   {
     *     "name":         "table",
     *     "columns":      ["col1", "col2"],
     *     "column_types": {"col1": "integer", "col2": "string"},
     *     "primary_keys": []
     *   }
     * @endcode
     */
    static json schemaToJson(const DetectedSchema& schema);

    /**
     * @brief Validate a data row against a DetectedSchema.
     *
     * A field passes validation when its inferred type has a rank ≤ the
     * declared column type (e.g. an INTEGER value is valid for a DOUBLE column).
     * Empty values are always considered valid (null semantics).
     *
     * @param columns  Column names.
     * @param values   Raw string values in the same order as @p columns.
     * @param schema   Schema to validate against.
     * @return         List of per-field errors; empty list means all valid.
     */
    static std::vector<SchemaValidationError> validateRow(
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values,
        const DetectedSchema& schema);

    // -------------------------------------------------------------------------
    // PHASE-2-HARDENING: Constraint violation categorization and null handling
    // -------------------------------------------------------------------------

    /**
     * @brief Map a constraint violation to an error code and description.
     *
     * @param violation_type  Type of constraint violation.
     * @return Pair of (error_code, description).
     * PHASE-2-HARDENING
     */
    static std::pair<std::string, std::string> mapViolationToErrorCode(
        ConstraintViolationType violation_type
    );

    /**
     * @brief Check and apply null handling policy to a field value.
     *
     * PHASE-2-HARDENING: Deterministic null handling.
     *
     * @param value           The string value to check (empty = NULL).
     * @param column_name     Column name for error messages.
     * @param null_policy     Null handling policy (NULLABLE or NON_NULLABLE).
     * @return Empty string if null handling is acceptable; otherwise error message.
     */
    static std::string checkNullHandling(
        const std::string& value,
        const std::string& column_name,
        NullHandlingPolicy null_policy
    );

    /**
     * @brief Validate numeric type coercion with bounds checking.
     *
     * PHASE-2-HARDENING: Type coercion with strict bounds.
     *
     * @param value   String representation of numeric value.
     * @param config  Type coercion configuration with bounds.
     * @return Empty string if valid; otherwise error message.
     */
    static std::string validateNumericCoercion(
        const std::string& value,
        const TypeCoercionConfig& config
    );

    /**
     * @brief Validate string type coercion with length limits.
     *
     * PHASE-2-HARDENING: String length enforcement.
     *
     * @param value   String value to validate.
     * @return Empty string if valid; otherwise error message.
     */
    static std::string validateStringCoercion(const std::string& value);

private:
    std::vector<std::string>                 columns_;
    std::map<std::string, DetectedFieldType> widest_types_;

    /// Numeric rank of a type for widening comparisons (higher = wider).
    static int typeRank(DetectedFieldType t);
};

} // namespace importers
} // namespace themis
