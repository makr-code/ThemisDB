/**
 * @file diagnostics.cpp
 * @brief Implementation of structured failure diagnostics
 * @version 0.0.1
 * @note PHASE-3-ERROR-HANDLING: Unified diagnostics implementation
 * @date 2026-08-02
 */

#include "importers/diagnostics.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace importers {

// ============================================================================
// Utility Functions
// ============================================================================

std::string failureCategoryToString(FailureCategory cat) {
    // PHASE-3-ERROR-HANDLING: Convert enum to string for diagnostics
    switch (cat) {
        case FailureCategory::SCHEMA_FAILURE:
            return "SCHEMA_FAILURE";
        case FailureCategory::CONFLICT_FAILURE:
            return "CONFLICT_FAILURE";
        case FailureCategory::CONNECTOR_FAILURE:
            return "CONNECTOR_FAILURE";
        case FailureCategory::CAPACITY_FAILURE:
            return "CAPACITY_FAILURE";
        case FailureCategory::INTEGRITY_FAILURE:
            return "INTEGRITY_FAILURE";
        default:
            return "UNKNOWN_FAILURE";
    }
}

/**
 * Get current nanosecond-precision timestamp
 * Deterministic: always returns current time
 */
static uint64_t getCurrentTimestampNs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    return static_cast<uint64_t>(nanos);
}

// ============================================================================
// Diagnostic Producers - Schema Failures
// ============================================================================

DiagnosticRecord produceSchemaDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context) {
    // PHASE-3-ERROR-HANDLING: Schema failure diagnosis
    
    DiagnosticRecord diag;
    diag.category = FailureCategory::SCHEMA_FAILURE;
    diag.timestamp_ns = getCurrentTimestampNs();
    diag.error_code = error_code;
    diag.context = context;

    // Extract context
    std::string table_name = context.count("table_name") ? 
        context.at("table_name") : "unknown";
    std::string column_name = context.count("column_name") ? 
        context.at("column_name") : "unknown";
    std::string error_detail = context.count("error_detail") ? 
        context.at("error_detail") : "schema validation failed";

    // Determine root cause and remediation based on error code
    if (error_code == ImportErrorCode::IMPORT_SCHEMA_MISMATCH) {
        diag.message = "Schema mismatch for table '" + table_name + "'";
        diag.root_cause = "Schema has changed since import started. The target schema "
                         "no longer matches the imported data structure. This can occur "
                         "when concurrent migrations modify the schema during an active import.";
        diag.remediation_steps = {
            "1. Verify that no concurrent schema migrations are running on table '" + table_name + "'",
            "2. Restart the import to re-fetch the current schema",
            "3. If schema changes are expected, wait for migrations to complete and retry",
            "4. If problem persists, check database connectivity and permissions"
        };
        diag.logs = {
            "INFO: Schema mismatch detected at table '" + table_name + "'",
            "DEBUG: Error detail: " + error_detail
        };
    } else if (error_code == ImportErrorCode::COLUMN_COUNT_MISMATCH) {
        diag.message = "Column count mismatch for table '" + table_name + "'";
        diag.root_cause = "The number of columns in the source data does not match "
                         "the target table schema. This typically occurs when source "
                         "data has been altered or schema inference sampled insufficient rows.";
        diag.remediation_steps = {
            "1. Verify source data has correct column structure matching '" + table_name + "'",
            "2. Check that all columns are present and in expected order",
            "3. If source data format is inconsistent, use schema override option",
            "4. Re-run schema inference with larger sample size (increase sample_rows)"
        };
        diag.logs = {
            "INFO: Column mismatch detected for table '" + table_name + "'",
            "DEBUG: " + error_detail
        };
    } else if (error_code == ImportErrorCode::UNKNOWN_TABLE) {
        diag.message = "Target table '" + table_name + "' does not exist";
        diag.root_cause = "The import target table '" + table_name + "' was not found "
                         "in the destination database. The table may have been dropped, "
                         "or the table name mapping is incorrect.";
        diag.remediation_steps = {
            "1. Verify that table '" + table_name + "' exists in the destination database",
            "2. Check table name case sensitivity (verify exact name match)",
            "3. If table is missing, create it with matching schema before retry",
            "4. Verify database user has access to create/modify tables if needed"
        };
        diag.logs = {
            "ERROR: Table '" + table_name + "' not found in destination",
            "DEBUG: " + error_detail
        };
    } else if (error_code == ImportErrorCode::SCHEMA_VALIDATION_FAILED) {
        diag.message = "Schema validation failed for table '" + table_name + 
                      "', column '" + column_name + "'";
        diag.root_cause = "Data value for column '" + column_name + "' does not match "
                         "the expected type or constraints. " + error_detail;
        diag.remediation_steps = {
            "1. Check value for column '" + column_name + "' in source data",
            "2. Verify column type definition matches actual data type",
            "3. If type mismatch, use type coercion or override configuration",
            "4. Consider using lenient validation mode if strict mode is too restrictive",
            "5. Quarantine invalid rows and continue import (enable continue_on_error)"
        };
        diag.logs = {
            "WARNING: Validation failed for " + table_name + "." + column_name,
            "DEBUG: " + error_detail
        };
    } else {
        diag.message = "Schema error: " + error_detail;
        diag.root_cause = "An unspecified schema validation error occurred. " + error_detail;
        diag.remediation_steps = {
            "1. Review error details for specific validation issue",
            "2. Check schema definition and data format match",
            "3. Retry import with verbose logging enabled",
            "4. Contact support if issue persists"
        };
        diag.logs = {"ERROR: " + error_detail};
    }

    return diag;
}

// ============================================================================
// Diagnostic Producers - Conflict Failures
// ============================================================================

DiagnosticRecord produceConflictDiagnostic(
    const std::string& reason,
    const std::map<std::string, std::string>& context) {
    // PHASE-3-ERROR-HANDLING: Conflict failure diagnosis
    
    DiagnosticRecord diag;
    diag.category = FailureCategory::CONFLICT_FAILURE;
    diag.timestamp_ns = getCurrentTimestampNs();
    diag.error_code = ImportErrorCode::CONFLICT_ERROR;
    diag.context = context;

    // Extract context
    std::string table_name = context.count("table_name") ? 
        context.at("table_name") : "unknown";
    std::string row_id = context.count("row_id") ? 
        context.at("row_id") : "unknown";
    std::string conflict_strategy = context.count("conflict_strategy") ? 
        context.at("conflict_strategy") : "unknown";
    std::string key_value = context.count("key_value") ? 
        context.at("key_value") : "unknown";

    diag.message = "Conflict detected in table '" + table_name + "' (row " + row_id + ")";
    diag.root_cause = "Unresolvable conflict occurred: " + reason + ". A conflict key "
                     "was encountered multiple times or a quality gate check failed. "
                     "Current conflict strategy: " + conflict_strategy;

    diag.remediation_steps = {
        "1. Review conflicting record(s) with key value: " + key_value,
        "2. Decide on resolution strategy (OVERWRITE, SKIP, MERGE, or ERROR)",
        "3. If using ERROR strategy, change to OVERWRITE or SKIP and retry",
        "4. If using MERGE, review protected_fields configuration",
        "5. Investigate root cause: is source data duplicated or schema key incorrect?",
        "6. For persistent conflicts, enable quality gate bypass (with audit trail)"
    };

    diag.logs = {
        "WARNING: Conflict detected in " + table_name + ", row " + row_id,
        "DEBUG: Reason: " + reason,
        "DEBUG: Key value: " + key_value,
        "DEBUG: Conflict strategy: " + conflict_strategy
    };

    return diag;
}

// ============================================================================
// Diagnostic Producers - Connector Failures
// ============================================================================

DiagnosticRecord produceConnectorDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context) {
    // PHASE-3-ERROR-HANDLING: Connector failure diagnosis
    
    DiagnosticRecord diag;
    diag.category = FailureCategory::CONNECTOR_FAILURE;
    diag.timestamp_ns = getCurrentTimestampNs();
    diag.error_code = error_code;
    diag.context = context;

    // Extract context
    std::string connector_name = context.count("connector_name") ? 
        context.at("connector_name") : "unknown";
    std::string connection_string = context.count("connection_string") ? 
        context.at("connection_string") : "unknown";
    std::string error_detail = context.count("error_detail") ? 
        context.at("error_detail") : "connection failed";

    if (error_code == ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE) {
        diag.message = "Connector '" + connector_name + "' is unavailable";
        diag.root_cause = "The source connector (" + connector_name + ") is not "
                         "reachable or connection pool is exhausted. " + error_detail;
        diag.remediation_steps = {
            "1. Verify connector '" + connector_name + "' is online and accepting connections",
            "2. Check network connectivity to " + connection_string,
            "3. Verify authentication credentials are current and have not expired",
            "4. Check if connection pool is exhausted (max_connections limit reached)",
            "5. Review firewall/security group rules to allow access",
            "6. Wait for any ongoing maintenance to complete and retry",
            "7. If using CDC fallback, verify connector supports fallback mode"
        };
        diag.logs = {
            "ERROR: Connector '" + connector_name + "' unavailable",
            "DEBUG: Connection string: " + connection_string,
            "DEBUG: " + error_detail
        };
    } else if (error_code == ImportErrorCode::DEADLINE_EXCEEDED) {
        diag.message = "Connector '" + connector_name + "' operation timeout";
        diag.root_cause = "Operation against " + connector_name + " exceeded the "
                         "configured timeout. Network latency or slow query may be the cause.";
        diag.remediation_steps = {
            "1. Increase import_timeout_ms configuration",
            "2. Optimize source database queries (add indexes if needed)",
            "3. Check network latency to " + connection_string,
            "4. Reduce batch_size to lower per-operation load",
            "5. Verify source database is not under heavy load",
            "6. Try import during off-peak hours"
        };
        diag.logs = {
            "ERROR: Timeout on connector '" + connector_name + "'",
            "DEBUG: " + error_detail
        };
    } else {
        diag.message = "Connector error: " + error_detail;
        diag.root_cause = "An error occurred communicating with connector '" + 
                         connector_name + "'. " + error_detail;
        diag.remediation_steps = {
            "1. Verify connector '" + connector_name + "' is available and responding",
            "2. Check connector logs for error details",
            "3. Verify connection parameters and credentials",
            "4. Test connectivity manually before retrying import",
            "5. Enable fallback mode if available for this connector"
        };
        diag.logs = {
            "ERROR: Connector error: " + error_detail,
            "DEBUG: Connector: " + connector_name
        };
    }

    return diag;
}

// ============================================================================
// Diagnostic Producers - Capacity Failures
// ============================================================================

DiagnosticRecord produceCapacityDiagnostic(
    const std::string& limit_name,
    uint64_t limit_value,
    uint64_t used_value,
    const std::map<std::string, std::string>& context) {
    // PHASE-3-ERROR-HANDLING: Capacity failure diagnosis
    
    DiagnosticRecord diag;
    diag.category = FailureCategory::CAPACITY_FAILURE;
    diag.timestamp_ns = getCurrentTimestampNs();
    diag.error_code = ImportErrorCode::FILE_READ_FAILED;  // Reuse for capacity failures
    diag.context = context;

    // Calculate percentage
    double percent = limit_value > 0 ? 
        (static_cast<double>(used_value) / static_cast<double>(limit_value)) * 100.0 : 0.0;

    std::ostringstream oss;
    oss << std::fixed << percent;
    std::string percent_str = oss.str();

    diag.message = "Capacity limit '" + limit_name + "' exceeded (" + 
                  percent_str + "% of limit)";
    diag.root_cause = "Resource limit for '" + limit_name + "' has been exceeded. "
                     "Used: " + std::to_string(used_value) + ", Limit: " + 
                     std::to_string(limit_value);

    if (limit_name.find("quota") != std::string::npos) {
        diag.remediation_steps = {
            "1. Check current quota usage with quota status command",
            "2. Identify which data contributed to quota consumption",
            "3. Delete or archive non-essential data to free quota",
            "4. Request quota increase from administrator",
            "5. Split large imports into smaller batches",
            "6. Prioritize most critical tables and retry after cleanup"
        };
    } else if (limit_name.find("buffer") != std::string::npos) {
        diag.remediation_steps = {
            "1. Reduce batch_size to lower memory usage per iteration",
            "2. Check for memory leaks in custom plugins",
            "3. Increase available system memory if possible",
            "4. Enable disk-based buffering if available",
            "5. Run import on system with more available memory",
            "6. Monitor memory usage during import"
        };
    } else {
        diag.remediation_steps = {
            "1. Check what resource is exhausted: " + limit_name,
            "2. Review current usage vs. configured limits",
            "3. Free up resources or increase limits",
            "4. Split import into smaller batches",
            "5. Run import during lower-usage periods"
        };
    }

    diag.logs = {
        "ERROR: Capacity limit exceeded: " + limit_name,
        "DEBUG: Used " + std::to_string(used_value) + " / " + 
               std::to_string(limit_value) + " (" + percent_str + "%)"
    };

    return diag;
}

// ============================================================================
// Diagnostic Producers - Integrity Failures
// ============================================================================

DiagnosticRecord produceIntegrityDiagnostic(
    const std::string& violation_type,
    const std::map<std::string, std::string>& context) {
    // PHASE-3-ERROR-HANDLING: Integrity failure diagnosis
    
    DiagnosticRecord diag;
    diag.category = FailureCategory::INTEGRITY_FAILURE;
    diag.timestamp_ns = getCurrentTimestampNs();
    diag.error_code = ImportErrorCode::VALUE_OUT_OF_RANGE;  // Reuse for integrity failures
    diag.context = context;

    // Extract context
    std::string table_name = context.count("table_name") ? 
        context.at("table_name") : "unknown";
    std::string column_name = context.count("column_name") ? 
        context.at("column_name") : "unknown";
    std::string row_id = context.count("row_id") ? 
        context.at("row_id") : "unknown";
    std::string value = context.count("value") ? 
        context.at("value") : "unknown";

    diag.message = "Integrity violation in table '" + table_name + 
                  "', row " + row_id + " (" + violation_type + ")";

    if (violation_type.find("FOREIGN_KEY") != std::string::npos ||
        violation_type.find("FK") != std::string::npos) {
        diag.root_cause = "Foreign key constraint violation. The value '" + value + 
                         "' for column '" + column_name + "' does not exist in the "
                         "referenced table. This typically indicates missing parent records.";
        diag.remediation_steps = {
            "1. Verify parent records exist in referenced table",
            "2. Check if parent data is being imported before child data",
            "3. Ensure import order respects foreign key dependencies",
            "4. Temporarily disable foreign key constraints (if safe)",
            "5. Use ignore_fk option to skip FK validation during import",
            "6. Identify and fix data inconsistencies in source"
        };
    } else if (violation_type.find("UNIQUE") != std::string::npos) {
        diag.root_cause = "Unique constraint violation. The value '" + value + 
                         "' for column '" + column_name + "' duplicates an existing record. "
                         "This indicates duplicate data in the source or an attempt to "
                         "violate uniqueness rules.";
        diag.remediation_steps = {
            "1. Identify which existing record has the duplicate value",
            "2. Decide on resolution: merge, skip, or use conflict strategy",
            "3. Configure conflict resolution strategy (MERGE/SKIP/OVERWRITE)",
            "4. Deduplicate source data before import",
            "5. Check if this is expected (e.g., upsert scenario)"
        };
    } else if (violation_type.find("TYPE") != std::string::npos) {
        diag.root_cause = "Type mismatch. Value '" + value + "' for column '" + 
                         column_name + "' cannot be converted to the expected type.";
        diag.remediation_steps = {
            "1. Review column type definition for '" + column_name + "'",
            "2. Check if value needs formatting (date, number, etc.)",
            "3. Use type coercion configuration to handle conversion",
            "4. Set type override for this column",
            "5. Quarantine rows with type mismatches and fix separately"
        };
    } else {
        diag.root_cause = "Data integrity violation: " + violation_type + 
                         ". Value '" + value + "' for column '" + column_name + 
                         "' violates constraints.";
        diag.remediation_steps = {
            "1. Review the constraint that was violated: " + violation_type,
            "2. Check if source data conforms to constraints",
            "3. Determine if constraint is necessary or can be relaxed",
            "4. Modify data to meet constraints before retry"
        };
    }

    diag.logs = {
        "ERROR: Integrity violation in " + table_name + ", row " + row_id,
        "DEBUG: Violation type: " + violation_type,
        "DEBUG: Column: " + column_name + ", Value: " + value
    };

    return diag;
}

// ============================================================================
// Diagnostic Aggregation
// ============================================================================

DiagnosticSummary aggregateDiagnostics(
    const std::string& import_id,
    uint64_t import_duration_ms,
    uint64_t total_records_attempted,
    const std::vector<DiagnosticRecord>& all_diagnostics) {
    // PHASE-3-ERROR-HANDLING: Aggregate diagnostics into summary
    
    DiagnosticSummary summary;
    summary.import_id = import_id;
    summary.import_duration_ms = import_duration_ms;
    summary.total_records_attempted = total_records_attempted;
    summary.all_diagnostics = all_diagnostics;

    // Count failures and warnings
    summary.failure_count = 0;
    summary.warning_count = 0;

    // Count by category and track root causes
    std::unordered_map<std::string, uint32_t> root_cause_counts;

    for (const auto& diag : all_diagnostics) {
        // Increment failure/warning count based on severity
        if (diag.error_code == ImportErrorCode::SUCCESS) {
            ++summary.warning_count;
        } else {
            ++summary.failure_count;
        }

        // Count failures by category
        summary.failures_by_category[diag.category]++;

        // Track root cause frequency
        if (!diag.root_cause.empty()) {
            root_cause_counts[diag.root_cause]++;
        }
    }

    // Extract top 5 root causes
    std::vector<std::pair<std::string, uint32_t>> sorted_causes(
        root_cause_counts.begin(), 
        root_cause_counts.end());

    std::sort(sorted_causes.begin(), sorted_causes.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (sorted_causes.size() > 5) {
        sorted_causes.resize(5);
    }

    summary.top_5_root_causes = sorted_causes;

    // Deduplicate and collect remediation steps
    std::unordered_map<std::string, uint32_t> remediation_counts;

    for (const auto& diag : all_diagnostics) {
        for (const auto& step : diag.remediation_steps) {
            remediation_counts[step]++;
        }
    }

    // Sort by frequency
    std::vector<std::pair<std::string, uint32_t>> sorted_remediation(
        remediation_counts.begin(),
        remediation_counts.end());

    std::sort(sorted_remediation.begin(), sorted_remediation.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [step, count] : sorted_remediation) {
        summary.common_remediation.push_back(step);
    }

    return summary;
}

}  // namespace importers
}  // namespace themis
