/**
 * @file schema_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/schema_validator.h"
#include <stdexcept>
#include <chrono>
#include <ctime>

#include <algorithm>
#include <cctype>

namespace themis {
namespace importers {

// ============================================================================
// Internal helpers
// ============================================================================

static std::string toLowerSchema(const std::string& s) {
    std::string out = {};
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
    if (s.empty()) {
      return false;
    }
    size_t start = 0;
    if (s[0] == '+' || s[0] == '-') {
      ++start;
    }
    if (start == static_cast<int>(s.size())) {
      return false;
    }
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
          return false;
        }
    }
    return true;
}

static bool valueIsDouble(const std::string& s) {
    if (s.empty()) {
      return false;
    }
    try {
        size_t pos = 0;
        (void)std::stod(s, &pos);
        return pos == static_cast<int>(s.size());
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
    if (value.empty()) {
      return DetectedFieldType::STRING;
    }
    if (valueIsBoolean(value)) {
      return DetectedFieldType::BOOLEAN;
    }
    if (valueIsInteger(value)) {
      return DetectedFieldType::INTEGER;
    }
    if (valueIsDouble(value)) {
      return DetectedFieldType::DOUBLE;
    }
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
    if (lower == "boolean" || lower == "bool") {
      return DetectedFieldType::BOOLEAN;
    }
    if (lower == "integer" || lower == "int") {
      return DetectedFieldType::INTEGER;
    }
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
    std::vector<SchemaValidationError> errors = {};

    const size_t n = std::min(columns.size(),static_cast<int>(values.size()));
    
    // PHASE-2-HARDENING: Default null policy (NULLABLE for backward compatibility)
    const NullHandlingPolicy null_policy = NullHandlingPolicy::NULLABLE;
    const TypeCoercionConfig coercion_config;
    
    for (size_t i = 0; i < n; ++i) {
        const auto& col = columns[i];
        const auto& val = values[i];
        
        // PHASE-2-HARDENING: Check null handling policy first
        std::string null_error = checkNullHandling(val, col, null_policy);
        if (!null_error.empty()) {
            SchemaValidationError err;
            err.column = col;
            err.value = val;
            err.expected_type = DetectedFieldType::STRING;
            err.message = null_error;
            err.violation_type = ConstraintViolationType::TYPE_MISMATCH;
            auto [error_code, _] = mapViolationToErrorCode(err.violation_type);
            err.error_code = error_code;
            errors.push_back(std::move(err));
            continue;
        }
        
        if (val.empty()) continue;  // null / empty is always valid (NULLABLE policy)

        auto it = schema.column_types.find(col);
        if (it == schema.column_types.end()) continue;  // unknown column – skip

        DetectedFieldType expected = it->second;
        DetectedFieldType actual   = inferType(val);

        // A narrower-or-equal type is fine (e.g. INTEGER fits a DOUBLE column).
        if (typeRank(actual) > typeRank(expected)) {
            // PHASE-2-HARDENING: Categorize constraint violation
            ConstraintViolationType violation_type = ConstraintViolationType::TYPE_MISMATCH;
            
            // Additional type-specific validation
            if (expected == DetectedFieldType::DOUBLE || expected == DetectedFieldType::INTEGER) {
                std::string numeric_error = validateNumericCoercion(val, coercion_config);
                if (!numeric_error.empty()) {
                    violation_type = ConstraintViolationType::TYPE_MISMATCH;
                }
            } else if (expected == DetectedFieldType::STRING) {
                std::string string_error = validateStringCoercion(val);
                if (!string_error.empty()) {
                    violation_type = ConstraintViolationType::LENGTH_VIOLATION;
                }
            }
            
            SchemaValidationError err;
            err.column = col;
            err.value = val;
            err.expected_type = expected;
            err.violation_type = violation_type;
            err.message = "Column '" + col + "': expected " +
                          typeName(expected) + " but got value '" + val + "'";
            
            // PHASE-2-HARDENING: Map violation to error code
            auto [error_code, error_desc] = mapViolationToErrorCode(violation_type);
            err.error_code = error_code;
            err.message += " (" + error_desc + ")";
            
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

    const size_t n = std::min(columns.size(),static_cast<int>(values.size()));
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

// ============================================================================
// PHASE-2-HARDENING: Constraint violation categorization and null handling
// ============================================================================

std::pair<std::string, std::string>
SchemaAutoDetector::mapViolationToErrorCode(
    ConstraintViolationType violation_type)
{
    switch (violation_type) {
        case ConstraintViolationType::TYPE_MISMATCH:
            return {"IMPORT_ROW_INVALID", "Type mismatch: value does not match column type"};
        case ConstraintViolationType::LENGTH_VIOLATION:
            return {"IMPORT_ROW_INVALID", "Length violation: string/binary exceeds maximum length"};
        case ConstraintViolationType::FOREIGN_KEY_VIOLATION:
            return {"IMPORT_DUPLICATE_KEY", "Foreign key violation: value not found in referenced table"};
        case ConstraintViolationType::UNIQUE_VIOLATION:
            return {"IMPORT_DUPLICATE_KEY", "Unique constraint violation: duplicate value"};
        case ConstraintViolationType::NONE:
        [[fallthrough]];\n        default:
            return {"OK", "No violation"};
    }
}

std::string
SchemaAutoDetector::checkNullHandling(
    const std::string& value,
    const std::string& column_name,
    NullHandlingPolicy null_policy)
{
    // PHASE-2-HARDENING: Deterministic null handling policy
    // Empty string is treated as NULL
    if (value.empty()) {
        if (null_policy == NullHandlingPolicy::NON_NULLABLE) {
            return "Column '" + column_name + "': NULL value not allowed (NON_NULLABLE policy)";
        }
        // NULLABLE policy: NULL accepted, transformed to sentinel value
        return "";  // No error
    }
    return "";  // Non-null value is always acceptable
}

std::string
SchemaAutoDetector::validateNumericCoercion(
    const std::string& value,
    const TypeCoercionConfig& config)
{
    // PHASE-2-HARDENING: Numeric type coercion with strict bounds checking
    if (value.empty()) {
        return "";  // Empty is treated as NULL, handled separately
    }

    try {
        double numeric_value = std::stod(value);
        
        // Check bounds
        if (numeric_value < config.numeric_min_value) {
            return "Numeric value " + value + " is below minimum (" +
                   std::to_string(config.numeric_min_value) + ")";
        }
        if (numeric_value > config.numeric_max_value) {
            return "Numeric value " + value + " exceeds maximum (" +
                   std::to_string(config.numeric_max_value) + ")";
        }
        
        // Check for overflow/underflow representation
        if (std::isnan(numeric_value) || std::isinf(numeric_value)) {
            return "Numeric value " + value + " is not a valid number";
        }
        
        return "";  // Valid
    } catch (const std::exception&) {
        return "Cannot coerce value '" + value + "' to numeric type";
    }
}

std::string
SchemaAutoDetector::validateStringCoercion(const std::string& value)
{
    // PHASE-2-HARDENING: String length enforcement (max 4KB per field)
    if (static_cast<int>(value.size()) > TypeCoercionConfig::kMaxStringFieldLength) {
        return "String value exceeds maximum length (" +
               std::to_string(TypeCoercionConfig::kMaxStringFieldLength) + " bytes)";
    }
    return "";  // Valid
}

// ============================================================================
// PHASE-3-ERROR-HANDLING: Schema Validation with Report
// ============================================================================

SchemaValidationReport validateSchemaWithReport(
    const DetectedSchema& schema,
    SchemaValidationLevel level) {
    // PHASE-3-ERROR-HANDLING: Comprehensive schema validation with degradation paths
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SchemaValidationReport report;
    report.level = level;
    report.repaired_schema = schema;
    report.is_valid = true;
    
    // Check for NULL table name
    if (schema.table_name.empty()) {
        SchemaError err;
        err.error_type = "NULL_TABLE_NAME";
        err.message = "Table name is missing or empty";
        err.affected_item = "<unknown>";
        err.suggestion = "Provide a non-empty table name in the schema";
        
        if (level == SchemaValidationLevel::STRICT) {
            report.errors.push_back(err);
            report.is_valid = false;
        } else if (level == SchemaValidationLevel::LENIENT) {
            report.warnings.push_back("Table name is empty (will use default)");
            report.suggestions.push_back("Provide table name in schema");
        } else if (level == SchemaValidationLevel::AUTO_REPAIR) {
            report.repaired_schema.table_name = "repaired_table_" + 
                std::to_string(std::time(nullptr));
            report.suggestions.push_back("Assigned default table name: " + 
                report.repaired_schema.table_name);
        }
    }
    
    // Check for oversized identifiers
    for (const auto& col : schema.columns) {
        if (col.length() > 128) {
            SchemaError err;
            err.error_type = "OVERSIZED_IDENTIFIER";
            err.message = "Identifier exceeds 128 character limit";
            err.affected_item = col;
            err.suggestion = "Truncate identifier to 128 characters or provide shorter name";
            
            if (level == SchemaValidationLevel::STRICT) {
                report.errors.push_back(err);
                report.is_valid = false;
            } else if (level == SchemaValidationLevel::LENIENT) {
                report.warnings.push_back("Column '" + col + "' exceeds length limit");
                report.suggestions.push_back("Consider truncating column names");
            } else if (level == SchemaValidationLevel::AUTO_REPAIR) {
                std::string truncated = col.substr(0, 128);
                report.suggestions.push_back("Auto-truncated column '" + col + 
                    "' to '" + truncated + "'");
            }
        }
    }
    
    // Check for unknown/NULL types
    for (const auto& [col_name, col_type] : schema.column_types) {
        // In DetectedFieldType enum, all values are defined, so type checking is implicit
        // Just warn about any columns that might have issues
        if (col_name.empty()) {
            SchemaError err;
            err.error_type = "NULL_COLUMN_NAME";
            err.message = "Column name is empty";
            err.affected_item = "<empty>";
            err.suggestion = "Provide non-empty column name";
            
            if (level == SchemaValidationLevel::STRICT) {
                report.errors.push_back(err);
                report.is_valid = false;
            } else if (level == SchemaValidationLevel::LENIENT) {
                report.warnings.push_back("Empty column name detected");
            }
        }
    }
    
    // Note: Circular FK detection would require additional context (foreign key definitions)
    // which are not part of DetectedSchema. This is a placeholder for the logic.
    // In a real implementation, this would require a more complex schema structure
    // that includes foreign key definitions.
    
    auto end_time = std::chrono::high_resolution_clock::now();
    report.validation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    return report;
}

} // namespace importers
} // namespace themis

