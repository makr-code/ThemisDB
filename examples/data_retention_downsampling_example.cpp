/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_retention_downsampling_example.cpp            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     471                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file data_retention_downsampling_example.cpp
 * @brief Example: Automated Time-Series Data Retention and Downsampling
 * 
 * This example demonstrates how to use TaskScheduler to automatically replace
 * high-resolution time-series data with lower-resolution aggregated data after
 * a configurable retention period.
 * 
 * Use Case: After 1 year, replace 1-second data points with 1-hour aggregates
 * (average, standard deviation, min, max) to maintain analytical capability
 * while significantly reducing storage requirements.
 * 
 * Benefits:
 * - Reduces storage by ~99% (3600:1 ratio for 1s->1h)
 * - Maintains statistical properties for analysis
 * - Configurable retention periods and resolutions
 * - Fully automated with TaskScheduler
 */

#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include <iostream>
#include <chrono>

namespace themis {
namespace examples {

/**
 * Configuration for data retention and downsampling
 */
struct RetentionPolicy {
    std::string source_resolution;      // e.g., "1s"
    std::string target_resolution;      // e.g., "1h"
    std::chrono::hours retention_period; // e.g., 1 year = 8760 hours
    std::string metric_pattern;         // e.g., "sensor_*" or "*"
};

/**
 * Example 1: Simple 1s -> 1h Downsampling After 1 Year
 * 
 * This task automatically aggregates 1-second data points into 1-hour
 * aggregates after 1 year, then deletes the raw 1s data.
 */
void example_simple_downsampling(TaskScheduler& scheduler) {
    std::cout << "=== Example 1: Simple Downsampling (1s -> 1h after 1 year) ===" << std::endl;
    
    ScheduledTask downsample_task;
    downsample_task.id = "downsample_1s_to_1h_yearly";
    downsample_task.name = "Yearly Downsampling: 1s to 1h";
    downsample_task.description = "Aggregate 1s data older than 1 year to 1h resolution";
    downsample_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    // Multi-step AQL query:
    // 1. Find 1s data older than 1 year
    // 2. Aggregate to 1h buckets with statistics
    // 3. Insert aggregates into target table
    // 4. Delete original 1s data
    downsample_task.aql_query = R"(
        // Step 1: Aggregate 1s data to 1h buckets
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp < DATE_SUB(NOW(), 1, 'year')
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            hour = DATE_TRUNC(d.timestamp, 'hour')
        AGGREGATE
            avg_value = AVG(d.value),
            stddev_value = STDDEV(d.value),
            min_value = MIN(d.value),
            max_value = MAX(d.value),
            count = COUNT(d),
            sum_value = SUM(d.value)
        
        // Step 2: Insert aggregated data
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: hour,
            resolution: '1h',
            value: avg_value,
            statistics: {
                avg: avg_value,
                stddev: stddev_value,
                min: min_value,
                max: max_value,
                count: count,
                sum: sum_value
            },
            aggregated_from: '1s',
            aggregated_at: DATE_NOW()
        } INTO timeseries_aggregates
        
        // Return summary for logging
        RETURN {
            metric: metric,
            entity: entity,
            hour: hour,
            points_aggregated: count,
            value_range: [min_value, max_value]
        }
    )";
    
    // Run daily (once per day is sufficient for yearly retention)
    downsample_task.interval = std::chrono::hours(24);
    downsample_task.timeout = std::chrono::hours(2);
    
    std::string task_id = scheduler.registerTask(downsample_task);
    std::cout << "Registered downsampling task: " << task_id << std::endl;
    
    // Separate cleanup task to delete old 1s data after aggregation
    ScheduledTask cleanup_task;
    cleanup_task.id = "cleanup_downsampled_1s_data";
    cleanup_task.name = "Cleanup Downsampled 1s Data";
    cleanup_task.description = "Remove 1s data after successful 1h aggregation";
    cleanup_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    cleanup_task.aql_query = R"(
        // Only delete 1s data if corresponding 1h aggregates exist
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp < DATE_SUB(NOW(), 1, 'year')
        LET hour_bucket = DATE_TRUNC(d.timestamp, 'hour')
        LET aggregate_exists = (
            FOR a IN timeseries_aggregates
            FILTER a.metric == d.metric
            AND a.entity == d.entity
            AND a.timestamp == hour_bucket
            AND a.resolution == '1h'
            LIMIT 1
            RETURN 1
        )
        FILTER LENGTH(aggregate_exists) > 0
        REMOVE d IN timeseries
        RETURN OLD
    )";
    
    cleanup_task.interval = std::chrono::hours(24);
    cleanup_task.timeout = std::chrono::hours(2);
    
    std::string cleanup_id = scheduler.registerTask(cleanup_task);
    std::cout << "Registered cleanup task: " << cleanup_id << std::endl;
}

/**
 * Example 2: Multi-Tier Retention Policy
 * 
 * Implements a cascading retention policy:
 * - Keep 1s data for 7 days
 * - Keep 1m data for 3 months (aggregate from 1s)
 * - Keep 1h data for 1 year (aggregate from 1m)
 * - Keep 1d data forever (aggregate from 1h)
 */
void example_multi_tier_retention(TaskScheduler& scheduler) {
    std::cout << "=== Example 2: Multi-Tier Retention Policy ===" << std::endl;
    
    // Tier 1: 1s -> 1m after 7 days
    ScheduledTask tier1_task;
    tier1_task.id = "retention_tier1_1s_to_1m";
    tier1_task.name = "Tier 1: 1s to 1m (7 days)";
    tier1_task.type = ScheduledTask::TaskType::AQL_QUERY;
    tier1_task.aql_query = R"(
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp BETWEEN DATE_SUB(NOW(), 8, 'days') AND DATE_SUB(NOW(), 7, 'days')
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            minute = DATE_TRUNC(d.timestamp, 'minute')
        AGGREGATE
            avg = AVG(d.value),
            stddev = STDDEV(d.value),
            min = MIN(d.value),
            max = MAX(d.value),
            count = COUNT(d)
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: minute,
            resolution: '1m',
            value: avg,
            statistics: {avg: avg, stddev: stddev, min: min, max: max, count: count},
            tier: 1
        } INTO timeseries_aggregates
        RETURN {metric: metric, entity: entity, minute: minute, count: count}
    )";
    tier1_task.interval = std::chrono::hours(6); // Run every 6 hours
    scheduler.registerTask(tier1_task);
    
    // Tier 2: 1m -> 1h after 3 months
    ScheduledTask tier2_task;
    tier2_task.id = "retention_tier2_1m_to_1h";
    tier2_task.name = "Tier 2: 1m to 1h (3 months)";
    tier2_task.type = ScheduledTask::TaskType::AQL_QUERY;
    tier2_task.aql_query = R"(
        FOR d IN timeseries_aggregates
        FILTER d.resolution == '1m'
        AND d.timestamp < DATE_SUB(NOW(), 3, 'months')
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            hour = DATE_TRUNC(d.timestamp, 'hour')
        AGGREGATE
            avg = AVG(d.value),
            min = MIN(d.statistics.min),
            max = MAX(d.statistics.max),
            count = SUM(d.statistics.count)
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: hour,
            resolution: '1h',
            value: avg,
            statistics: {avg: avg, min: min, max: max, count: count},
            tier: 2
        } INTO timeseries_aggregates
        RETURN {metric: metric, entity: entity, hour: hour}
    )";
    tier2_task.interval = std::chrono::hours(24);
    scheduler.registerTask(tier2_task);
    
    // Tier 3: 1h -> 1d after 1 year
    ScheduledTask tier3_task;
    tier3_task.id = "retention_tier3_1h_to_1d";
    tier3_task.name = "Tier 3: 1h to 1d (1 year)";
    tier3_task.type = ScheduledTask::TaskType::AQL_QUERY;
    tier3_task.aql_query = R"(
        FOR d IN timeseries_aggregates
        FILTER d.resolution == '1h'
        AND d.timestamp < DATE_SUB(NOW(), 1, 'year')
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            day = DATE_TRUNC(d.timestamp, 'day')
        AGGREGATE
            avg = AVG(d.value),
            min = MIN(d.statistics.min),
            max = MAX(d.statistics.max),
            count = SUM(d.statistics.count)
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: day,
            resolution: '1d',
            value: avg,
            statistics: {avg: avg, min: min, max: max, count: count},
            tier: 3
        } INTO timeseries_aggregates
        RETURN {metric: metric, entity: entity, day: day}
    )";
    tier3_task.interval = std::chrono::hours(24);
    scheduler.registerTask(tier3_task);
    
    std::cout << "Registered 3-tier retention policy tasks" << std::endl;
}

/**
 * Example 3: Configurable Retention with Custom Function
 * 
 * Uses a custom function to make retention policies configurable
 * and support multiple metrics with different requirements.
 */
void example_configurable_retention(TaskScheduler& scheduler, QueryEngine* query_engine) {
    std::cout << "=== Example 3: Configurable Retention Policy ===" << std::endl;
    
    // Register a flexible retention function
    scheduler.registerFunction("apply_retention_policy",
        [query_engine](const nlohmann::json& params) -> nlohmann::json {
            std::string metric = params.value("metric", "*");
            std::string source_res = params.value("source_resolution", "1s");
            std::string target_res = params.value("target_resolution", "1h");
            int retention_days = params.value("retention_days", 365);
            
            // Build AQL query dynamically
            std::ostringstream aql = {};
            aql << "FOR d IN timeseries "
                << "FILTER d.resolution == '" << source_res << "' ";
            
            if (metric != "*") {
                aql << "AND d.metric == '" << metric << "' ";
            }
            
            aql << "AND d.timestamp < DATE_SUB(NOW(), " << retention_days << ", 'days') "
                << "COLLECT "
                << "  metric = d.metric, "
                << "  entity = d.entity, "
                << "  bucket = DATE_TRUNC(d.timestamp, '" << target_res << "') "
                << "AGGREGATE "
                << "  avg = AVG(d.value), "
                << "  stddev = STDDEV(d.value), "
                << "  min = MIN(d.value), "
                << "  max = MAX(d.value), "
                << "  count = COUNT(d) "
                << "INSERT { "
                << "  metric: metric, "
                << "  entity: entity, "
                << "  timestamp: bucket, "
                << "  resolution: '" << target_res << "', "
                << "  value: avg, "
                << "  statistics: {avg: avg, stddev: stddev, min: min, max: max, count: count} "
                << "} INTO timeseries_aggregates "
                << "RETURN {metric: metric, count: count}";
            
            // Execute query
            auto result = executeAql(aql.str(), *query_engine);
            
            if (!result) {
                return nlohmann::json{
                    {"status", "error"},
                    {"message", result.error().message()}
                };
            }
            
            return nlohmann::json{
                {"status", "success"},
                {"policy", params},
                {"result", *result}
            };
        }
    );
    
    // Create tasks with different policies for different metrics
    
    // Policy 1: Temperature sensors - 1s to 1h after 1 year
    ScheduledTask temp_policy;
    temp_policy.name = "Temperature Retention Policy";
    temp_policy.type = ScheduledTask::TaskType::FUNCTION;
    temp_policy.function_name = "apply_retention_policy";
    temp_policy.parameters = {
        {"metric", "temperature"},
        {"source_resolution", "1s"},
        {"target_resolution", "1h"},
        {"retention_days", 365}
    };
    temp_policy.interval = std::chrono::hours(24);
    scheduler.registerTask(temp_policy);
    
    // Policy 2: Pressure sensors - 1s to 15m after 90 days
    ScheduledTask pressure_policy;
    pressure_policy.name = "Pressure Retention Policy";
    pressure_policy.type = ScheduledTask::TaskType::FUNCTION;
    pressure_policy.function_name = "apply_retention_policy";
    pressure_policy.parameters = {
        {"metric", "pressure"},
        {"source_resolution", "1s"},
        {"target_resolution", "15m"},
        {"retention_days", 90}
    };
    pressure_policy.interval = std::chrono::hours(12);
    scheduler.registerTask(pressure_policy);
    
    // Policy 3: All other metrics - 1s to 1h after 6 months
    ScheduledTask default_policy;
    default_policy.name = "Default Retention Policy";
    default_policy.type = ScheduledTask::TaskType::FUNCTION;
    default_policy.function_name = "apply_retention_policy";
    default_policy.parameters = {
        {"metric", "*"},
        {"source_resolution", "1s"},
        {"target_resolution", "1h"},
        {"retention_days", 180}
    };
    default_policy.interval = std::chrono::hours(24);
    scheduler.registerTask(default_policy);
    
    std::cout << "Registered 3 configurable retention policies" << std::endl;
}

/**
 * Example 4: Storage Savings Calculator
 * 
 * Monitors and reports storage savings from retention policies.
 */
void example_storage_savings_monitor(TaskScheduler& scheduler) {
    std::cout << "=== Example 4: Storage Savings Monitor ===" << std::endl;
    
    scheduler.registerFunction("calculate_storage_savings",
        [](const nlohmann::json& params) -> nlohmann::json {
            // This would query actual storage statistics
            // For demonstration, we calculate theoretical savings
            
            int64_t seconds_per_year = 365 * 24 * 3600;
            int64_t raw_1s_records = seconds_per_year;  // 1 record per second
            int64_t aggregated_1h_records = 365 * 24;   // 1 record per hour
            
            // Assume ~100 bytes per record (simplified)
            int64_t raw_storage_bytes = raw_1s_records * 100;
            int64_t aggregated_storage_bytes = aggregated_1h_records * 150; // Slightly larger due to stats
            
            int64_t savings_bytes = raw_storage_bytes - aggregated_storage_bytes;
            double savings_percent = (double)savings_bytes / raw_storage_bytes * 100;
            
            return nlohmann::json{
                {"raw_records", raw_1s_records},
                {"aggregated_records", aggregated_1h_records},
                {"compression_ratio", (double)raw_1s_records / aggregated_1h_records},
                {"raw_storage_mb", raw_storage_bytes / (1024 * 1024)},
                {"aggregated_storage_mb", aggregated_storage_bytes / (1024 * 1024)},
                {"savings_mb", savings_bytes / (1024 * 1024)},
                {"savings_percent", savings_percent}
            };
        }
    );
    
    ScheduledTask monitor_task;
    monitor_task.name = "Storage Savings Monitor";
    monitor_task.type = ScheduledTask::TaskType::FUNCTION;
    monitor_task.function_name = "calculate_storage_savings";
    monitor_task.interval = std::chrono::hours(24);
    
    monitor_task.on_success = [](const std::string& task_id, const nlohmann::json& result) {
        std::cout << "=== Storage Savings Report ===" << std::endl;
        std::cout << "Compression Ratio: " << result["compression_ratio"] << ":1" << std::endl;
        std::cout << "Storage Savings: " << result["savings_mb"] << " MB ("
                  << result["savings_percent"] << "%)" << std::endl;
    };
    
    scheduler.registerTask(monitor_task);
    std::cout << "Registered storage savings monitor" << std::endl;
}

/**
 * Main example runner
 */
void run_retention_examples(QueryEngine* query_engine) {
    std::cout << "====================================================" << std::endl;
    std::cout << "  Data Retention & Downsampling Examples" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    TaskScheduler::Config config;
    config.max_concurrent_tasks = 4;
    config.check_interval = std::chrono::seconds(10);
    config.persist_tasks = true;
    
    TaskScheduler scheduler(query_engine, config);
    
    // Run all examples
    example_simple_downsampling(scheduler);
    example_multi_tier_retention(scheduler);
    example_configurable_retention(scheduler, query_engine);
    example_storage_savings_monitor(scheduler);
    
    std::cout << "\n====================================================" << std::endl;
    std::cout << "All retention policy tasks registered successfully!" << std::endl;
    std::cout << "====================================================" << std::endl;
}

} // namespace examples
} // namespace themis
