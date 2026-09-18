/**
 * @file schema_validator.h
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
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class DetectedFieldType {
    BOOLEAN,  ///< "true" / "false" (case-insensitive)
    INTEGER,  ///< Whole number (no decimal point)
    DOUBLE,   ///< Floating-point number
    STRING    ///< Everything else (or empty / null)
};

struct DetectedSchema {
    std::string              table_name;
    std::vector<std::string> columns;
    std::map<std::string, DetectedFieldType> column_types;
};

enum class NullHandlingPolicy {
    NULLABLE,           ///< NULL accepted, transformed to sentinel value
    NON_NULLABLE        ///< NULL rejected (error IMPORT_ROW_INVALID)
};

enum class ConstraintViolationType {
    TYPE_MISMATCH,      ///< Value type does not match column type
    LENGTH_VIOLATION,   ///< String/binary exceeds max length
    FOREIGN_KEY_VIOLATION,  ///< Value does not match referenced table
    UNIQUE_VIOLATION,   ///< Value violates unique constraint
    NONE                ///< No violation
};

struct SchemaValidationError {
    std::string       column;             ///< Column name where the mismatch occurred
    std::string       value;              ///< Actual string value that failed validation
    DetectedFieldType expected_type;      ///< Type declared in the schema
    std::string       message;            ///< Human-readable description
    
    // PHASE-2-HARDENING: Constraint violation categorization
    ConstraintViolationType violation_type{ConstraintViolationType::NONE};
    std::string       error_code;         ///< Structured error code (e.g., IMPORT_ROW_INVALID)
};

struct TypeCoercionConfig {
    // String constraints
    static constexpr size_t kMaxStringFieldLength = 4096;  ///< Max 4KB per string field
    
    // Numeric constraints
    double numeric_min_value{-1e308};     ///< Minimum numeric value (IEEE double)
    double numeric_max_value{1e308};      ///< Maximum numeric value (IEEE double)
    
    // Date format validation
    bool validate_iso8601{true};          ///< Enforce ISO8601 or RFC3339 for dates
};

// ============================================================================
// PHASE-3-ERROR-HANDLING: Schema Validation Levels & Reporting
// ============================================================================

enum class SchemaValidationLevel {
    STRICT,        ///< Enforce all rules (NULL types, complex cycles rejected)
    LENIENT,       ///< Allow NULL types, warn on cycles, truncate oversized identifiers
    AUTO_REPAIR    ///< Attempt to fix: coerce types, break cycles, truncate
};

struct SchemaError {
    std::string error_type;      ///< Error type (NULL_TABLE_NAME, CIRCULAR_FK, etc.)
    std::string message;          ///< Human-readable error message
    std::string affected_item;    ///< What caused error (table/column name)
    std::string suggestion;       ///< How to fix (e.g., "provide table name")
};

struct SchemaValidationReport {
    bool is_valid = {};

    SchemaValidationLevel level;

    std::vector<SchemaError> errors;

    std::vector<std::string> warnings;

    std::vector<std::string> suggestions;

    DetectedSchema repaired_schema;

    uint64_t validation_time_ms = {};

    json toJson() const {
        json errors_json = json::array();
        for (const auto& err : errors) {
            errors_json.push_back({
                {"error_type", err.error_type},
                {"message", err.message},
                {"affected_item", err.affected_item},
                {"suggestion", err.suggestion}
            });
        }

        json warnings_json = json::array();
        for (const auto& w : warnings) {
            warnings_json.push_back(w);
        }

        json suggestions_json = json::array();
        for (const auto& s : suggestions) {
            suggestions_json.push_back(s);
        }

        std::string level_str = {};
        switch (level) {
            case SchemaValidationLevel::STRICT:
                level_str = "STRICT";
                break;
            case SchemaValidationLevel::LENIENT:
                level_str = "LENIENT";
                break;
            case SchemaValidationLevel::AUTO_REPAIR:
                level_str = "AUTO_REPAIR";
                break;
        }

        return json{
            {"is_valid", is_valid},
            {"level", level_str},
            {"errors", errors_json},
            {"warnings", warnings_json},
            {"suggestions", suggestions_json},
            {"validation_time_ms", validation_time_ms}
        };
    }
};

/**
 * @brief Validate Schema With Report.
 * @param[in] schema Input parameter.
 * @param[in] level Input parameter.
 * @return Return value.
 */
SchemaValidationReport validateSchemaWithReport(
    const DetectedSchema& schema,
    SchemaValidationLevel level);

static std::pair<std::string, std::string> mapViolationToErrorCode(
    ConstraintViolationType violation_type
);

class SchemaAutoDetector {
public:
    SchemaAutoDetector() = default;

    /**
     * @brief Feed Row.
     * @param[in] columns Input parameter.
     * @param[in] values Input parameter.
     */
    void feedRow(const std::vector<std::string>& columns,
                 const std::vector<std::string>& values);

    /**
     * @brief Get Schema.
     * @param[in] table_name Name of the table.
     * @return Return value.
     */
    DetectedSchema getSchema(const std::string& table_name) const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Infer Type.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static DetectedFieldType inferType(const std::string& value);

    /**
     * @brief Widen Type.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static DetectedFieldType widenType(DetectedFieldType a, DetectedFieldType b);

    /**
     * @brief Type Name.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    static std::string typeName(DetectedFieldType t);

    /**
     * @brief Parse Type Name.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    static DetectedFieldType parseTypeName(const std::string& name);

    /**
     * @brief Schema To Json.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static json schemaToJson(const DetectedSchema& schema);

    /**
     * @brief Validate Row.
     * @param[in] columns Input parameter.
     * @param[in] values Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static std::vector<SchemaValidationError> validateRow(
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values,
        const DetectedSchema& schema);

    // -------------------------------------------------------------------------
    // PHASE-2-HARDENING: Constraint violation categorization and null handling
    // -------------------------------------------------------------------------

    static std::pair<std::string, std::string> mapViolationToErrorCode(
        ConstraintViolationType violation_type
    );

    /**
     * @brief Check Null Handling.
     * @param[in] value Input parameter.
     * @param[in] column_name Name of the column.
     * @param[in] null_policy Input parameter.
     * @return Return value.
     */
    static std::string checkNullHandling(
        const std::string& value,
        const std::string& column_name,
        NullHandlingPolicy null_policy
    );

    /**
     * @brief Validate Numeric Coercion.
     * @param[in] value Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::string validateNumericCoercion(
        const std::string& value,
        const TypeCoercionConfig& config
    );

    /**
     * @brief Validate String Coercion.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static std::string validateStringCoercion(const std::string& value);

private:
    std::vector<std::string>                 columns_;
    std::map<std::string, DetectedFieldType> widest_types_;

    /**
     * @brief Type Rank.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    static int typeRank(DetectedFieldType t);
};

} // namespace importers
} // namespace themis
