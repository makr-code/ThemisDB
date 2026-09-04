/**
 * @file retention_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "query/functions/function_registry.h"
#include "scheduler/task_scheduler.h"
#include "scheduler/hybrid_retention_manager.h"
#include <cmath>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Statistical Functions for Adaptive Retention
// ============================================================================

/**
 * @brief CV(stddev, mean) - Calculate Coefficient of Variation
 * 
 * CV = (stddev / mean) × 100%
 * Used in adaptive retention to determine data variance.
 */
class CoefficientOfVariationFunction : public IFunction {
public:
    ~CoefficientOfVariationFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CV",
            .category = "Statistics",
            .description = "Calculates Coefficient of Variation (CV = stddev/mean × 100%)",
            .arguments = {
                {"stddev", ArgType::NUMBER, true, nullptr, "Standard deviation"},
                {"mean", ArgType::NUMBER, true, nullptr, "Mean value"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(CV(2.5, 50) // 5.0 (low variance))",
                R"(CV(15, 50) // 30.0 (high variance))",
                R"(COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
                   AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
                   LET cv = CV(stddev, avg)
                   RETURN {hour, cv, variance_level: cv < 5 ? 'low' : cv < 20 ? 'medium' : 'high'})"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double stddev = args[0].get<double>();
        double mean = args[1].get<double>();
        
        if (mean == 0.0) {
            return 0.0;  // Avoid division by zero
        }
        
        return (std::abs(stddev) / std::abs(mean)) * 100.0;
    }
};

/**
 * @brief VARIANCE_LEVEL(cv, lowThreshold?, mediumThreshold?) - Classify variance level
 * 
 * Returns 'low', 'medium', or 'high' based on CV thresholds.
 */
class VarianceLevelFunction : public IFunction {
public:
    ~VarianceLevelFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "VARIANCE_LEVEL",
            .category = "Statistics",
            .description = "Classifies variance level based on CV thresholds",
            .arguments = {
                {"cv", ArgType::NUMBER, true, nullptr, "Coefficient of Variation"},
                {"lowThreshold", ArgType::NUMBER, false, nullptr, "Low threshold (default: 5.0)"},
                {"mediumThreshold", ArgType::NUMBER, false, nullptr, "Medium threshold (default: 20.0)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(VARIANCE_LEVEL(3.5) // 'low')",
                R"(VARIANCE_LEVEL(15) // 'medium')",
                R"(VARIANCE_LEVEL(25) // 'high')",
                R"(VARIANCE_LEVEL(cv, 3, 15) // Custom thresholds)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double cv = args[0].get<double>();
        double lowThreshold = args.size() > 1 ? args[1].get<double>() : 5.0;
        double mediumThreshold = args.size() > 2 ? args[2].get<double>() : 20.0;
        
        if (cv < lowThreshold) {
            return "low";
        } else if (cv < mediumThreshold) {
            return "medium";
        } else {
            return "high";
        }
    }
};

/**
 * @brief RETENTION_RESOLUTION(cv, lowThreshold?, mediumThreshold?) - Suggest retention resolution
 * 
 * Suggests appropriate resolution based on variance level.
 */
class RetentionResolutionFunction : public IFunction {
public:
    ~RetentionResolutionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RETENTION_RESOLUTION",
            .category = "Retention",
            .description = "Suggests data retention resolution based on variance (CV)",
            .arguments = {
                {"cv", ArgType::NUMBER, true, nullptr, "Coefficient of Variation"},
                {"lowThreshold", ArgType::NUMBER, false, nullptr, "Low CV threshold (default: 5.0)"},
                {"mediumThreshold", ArgType::NUMBER, false, nullptr, "Medium CV threshold (default: 20.0)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(RETENTION_RESOLUTION(3.5) // '1h' (low variance))",
                R"(RETENTION_RESOLUTION(15) // '15m' (medium variance))",
                R"(RETENTION_RESOLUTION(25) // '1m' (high variance))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double cv = args[0].get<double>();
        double lowThreshold = args.size() > 1 ? args[1].get<double>() : 5.0;
        double mediumThreshold = args.size() > 2 ? args[2].get<double>() : 20.0;
        
        if (cv < lowThreshold) {
            return "1h";
        } else if (cv < mediumThreshold) {
            return "15m";
        } else {
            return "1m";
        }
    }
};

// ============================================================================
// Date Convenience Aliases for Retention
// ============================================================================

/**
 * @brief DATE_SUB(timestamp, amount, unit) - Alias for DATE_SUBTRACT
 * 
 * Provides a shorter, more convenient alias for DATE_SUBTRACT.
 */
class DateSubFunction : public IFunction {
public:
    ~DateSubFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_SUB",
            .category = "Date",
            .description = "Alias for DATE_SUBTRACT - subtracts time from a timestamp",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"amount", ArgType::INTEGER, true, nullptr, "Amount to subtract"},
                {"unit", ArgType::STRING, true, nullptr, "Unit: year, month, day, hour, minute, second"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(DATE_SUB(NOW(), 1, 'year') // One year ago)",
                R"(DATE_SUB(NOW(), 7, 'days') // One week ago)",
                R"(DATE_SUB(NOW(), 90, 'days') // Three months ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, [[maybe_unused]] const FunctionContext& ctx) const override {
        // Delegate to DATE_SUBTRACT with negative amount
        std::vector<nlohmann::json> subtractArgs = {
            args[0],
            -args[1].get<int64_t>(),
            args[2]
        };
        
        // Use DATE_ADD with negative amount (same as DATE_SUBTRACT)
        int64_t ts = args[0].get<int64_t>();
        int64_t amount = -args[1].get<int64_t>();
        std::string unit = args[2].get<std::string>();
        
        int64_t ms = 0;
        if (unit == "millisecond" || unit == "ms") {
            ms = amount;
        } else if (unit == "second" || unit == "s") {
            ms = amount * 1000;
        } else if (unit == "minute" || unit == "m") {
            ms = amount * 60 * 1000;
        } else if (unit == "hour" || unit == "h") {
            ms = amount * 60 * 60 * 1000;
        } else if (unit == "day" || unit == "d" || unit == "days") {
            ms = amount * 24 * 60 * 60 * 1000;
        } else if (unit == "week" || unit == "w" || unit == "weeks") {
            ms = amount * 7 * 24 * 60 * 60 * 1000;
        } else if (unit == "month" || unit == "M" || unit == "months") {
            // Approximate: 30 days
            ms = amount * 30 * 24 * 60 * 60 * 1000;
        } else if (unit == "year" || unit == "y" || unit == "years") {
            // Approximate: 365 days
            ms = amount * 365 * 24 * 60 * 60 * 1000;
        } else {
            throw std::runtime_error("DATE_SUB: unknown unit '" + unit + "'");
        }
        
        return ts + ms;
    }
};

// ============================================================================
// Task Scheduling Functions
// ============================================================================

/**
 * @brief SCHEDULE_TASK(taskConfig) - Create a scheduled task
 * 
 * ⚠️ SECURITY: Requires admin privileges. Can execute arbitrary AQL.
 * 
 * Creates a new scheduled task from AQL. The task will be registered
 * with the TaskScheduler and executed periodically.
 */
class ScheduleTaskFunction : public IFunction {
public:
    ~ScheduleTaskFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SCHEDULE_TASK",
            .category = "Scheduling",
            .description = "⚠️ ADMIN ONLY: Creates a scheduled task that runs periodically",
            .arguments = {
                {"config", ArgType::OBJECT, true, nullptr, "Task configuration object"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = false,
            .examples = {
                R"(SCHEDULE_TASK({
                    name: 'Daily Cleanup',
                    type: 'aql',
                    query: 'FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 30, "days") REMOVE d IN timeseries',
                    interval_hours: 24
                }))",
                R"(SCHEDULE_TASK({
                    name: 'Hourly Aggregation',
                    type: 'aql',
                    query: 'FOR d IN timeseries ... INSERT INTO aggregates',
                    interval_minutes: 60
                }))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext& ctx) const override {
        const auto& config = args[0];
        
        // Validate required fields
        if (!config.contains("name") || !config.contains("type") || !config.contains("query")) {
            throw std::runtime_error("SCHEDULE_TASK: config must contain 'name', 'type', and 'query'");
        }
        
        // Extract task configuration
        std::string name = config["name"].get<std::string>();
        std::string type = config["type"].get<std::string>();
        std::string query = config["query"].get<std::string>();
        
        // Calculate interval in milliseconds
        int64_t interval_ms = 60000; // Default: 1 minute
        if (config.contains("interval_hours")) {
            interval_ms = static_cast<int64_t>(config["interval_hours"].get<int>()) * 3600000LL;
        } else if (config.contains("interval_minutes")) {
            interval_ms = static_cast<int64_t>(config["interval_minutes"].get<int>()) * 60000LL;
        } else if (config.contains("interval_seconds")) {
            interval_ms = static_cast<int64_t>(config["interval_seconds"].get<int>()) * 1000LL;
        } else if (config.contains("interval_ms")) {
            interval_ms = config["interval_ms"].get<int64_t>();
        }

        // Use injected TaskScheduler bridge when available
        if (const auto& fn = ctx.registerTaskFn()) {
            nlohmann::json task_config{
                {"name",        name},
                {"type",        type},
                {"query",       query},
                {"interval_ms", interval_ms}
            };
            std::string task_id = fn(task_config);
            return nlohmann::json{
                {"status",      "created"},
                {"task_id",     task_id},
                {"name",        name},
                {"interval_ms", interval_ms}
            };
        }

        // Fallback when no scheduler context is injected
        return nlohmann::json{
            {"status",      "pending"},
            {"task_id",     "unregistered_" + name},
            {"name",        name},
            {"interval_ms", interval_ms},
            {"note",        "Task scheduler not configured; inject via FunctionContext::setRegisterTaskFn()"}
        };
    }
};

/**
 * @brief LIST_SCHEDULED_TASKS() - List all scheduled tasks
 * 
 * Returns a list of all tasks registered in the TaskScheduler.
 */
class ListScheduledTasksFunction : public IFunction {
public:
    ~ListScheduledTasksFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LIST_SCHEDULED_TASKS",
            .category = "Scheduling",
            .description = "Lists all scheduled tasks",
            .arguments = {},
            .return_type = ArgType::ARRAY,
            .is_deterministic = false,
            .examples = {
                R"(LIST_SCHEDULED_TASKS() // Returns array of task objects)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& /*args*/,
                           const FunctionContext& ctx) const override {
        if (const auto& fn = ctx.listTasksFn()) {
            return fn();
        }
        // Fallback when no scheduler context is injected
        return nlohmann::json::array();
    }
};

/**
 * @brief CANCEL_TASK(taskId) - Cancel a scheduled task
 * 
 * ⚠️ SECURITY: Requires admin privileges.
 */
class CancelTaskFunction : public IFunction {
public:
    ~CancelTaskFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CANCEL_TASK",
            .category = "Scheduling",
            .description = "⚠️ ADMIN ONLY: Cancels a scheduled task",
            .arguments = {
                {"taskId", ArgType::STRING, true, nullptr, "Task ID to cancel"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = false,
            .examples = {
                R"(CANCEL_TASK('task_12345') // Returns true if cancelled)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext& ctx) const override {
        std::string taskId = args[0].get<std::string>();
        if (const auto& fn = ctx.cancelTaskFn()) {
            return fn(taskId);
        }
        // Fallback when no scheduler context is injected
        return false;
    }
};

/**
 * @brief ESTIMATE_STORAGE_SAVINGS(sourceResolution, targetResolution, dataPoints) - Estimate savings
 * 
 * Calculates estimated storage savings from downsampling.
 */
class EstimateStorageSavingsFunction : public IFunction {
public:
    ~EstimateStorageSavingsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ESTIMATE_STORAGE_SAVINGS",
            .category = "Retention",
            .description = "Estimates storage savings from downsampling",
            .arguments = {
                {"sourceResolution", ArgType::STRING, true, nullptr, "Source resolution (e.g., '1s')"},
                {"targetResolution", ArgType::STRING, true, nullptr, "Target resolution (e.g., '1h')"},
                {"dataPoints", ArgType::INTEGER, true, nullptr, "Number of data points"}
            },
            .return_type = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {
                R"(ESTIMATE_STORAGE_SAVINGS('1s', '1h', 31536000) // 1 year of 1s data -> 1h)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string sourceRes = args[0].get<std::string>();
        std::string targetRes = args[1].get<std::string>();
        int64_t dataPoints = args[2].get<int64_t>();
        
        // Parse resolutions to seconds
        auto parseResolution = [](const std::string& res) -> int64_t {
            if (res == "1s") {
              return 1;
            }
            if (res == "1m") {
              return 60;
            }
            if (res == "5m") {
              return 300;
            }
            if (res == "15m") {
              return 900;
            }
            if (res == "1h") {
              return 3600;
            }
            if (res == "1d") {
              return 86400;
            }
            return 1;
        };
        
        int64_t sourceSec = parseResolution(sourceRes);
        int64_t targetSec = parseResolution(targetRes);
        
        int64_t compressionRatio = targetSec / sourceSec;
        int64_t targetPoints = dataPoints / compressionRatio;
        
        // Assume 100 bytes per point, 150 bytes per aggregate (with stats)
        int64_t sourceBytes = dataPoints * 100;
        int64_t targetBytes = targetPoints * 150;
        int64_t savedBytes = sourceBytes - targetBytes;
        double savingsPercent = (double)savedBytes / sourceBytes * 100.0;
        
        return nlohmann::json{
            {"source_resolution", sourceRes},
            {"target_resolution", targetRes},
            {"source_data_points", dataPoints},
            {"target_data_points", targetPoints},
            {"compression_ratio", compressionRatio},
            {"source_storage_bytes", sourceBytes},
            {"target_storage_bytes", targetBytes},
            {"storage_saved_bytes", savedBytes},
            {"storage_savings_percent", savingsPercent},
            {"storage_saved_mb", savedBytes / (1024 * 1024)}
        };
    }
};

// ============================================================================
// Register Retention Functions
// ============================================================================

inline void registerRetentionFunctions(FunctionRegistry& reg) {
    // Statistical functions for adaptive retention
    reg.registerFunction(std::make_unique<CoefficientOfVariationFunction>());
    reg.registerFunction(std::make_unique<VarianceLevelFunction>());
    reg.registerFunction(std::make_unique<RetentionResolutionFunction>());
    
    // Date convenience aliases
    reg.registerFunction(std::make_unique<DateSubFunction>());
    
    // Task scheduling functions
    reg.registerFunction(std::make_unique<ScheduleTaskFunction>());
    reg.registerFunction(std::make_unique<ListScheduledTasksFunction>());
    reg.registerFunction(std::make_unique<CancelTaskFunction>());
    
    // Utility functions
    reg.registerFunction(std::make_unique<EstimateStorageSavingsFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

