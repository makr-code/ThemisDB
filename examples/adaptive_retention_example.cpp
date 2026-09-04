/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_retention_example.cpp                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     535                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adaptive_retention_example.cpp
 * @brief Example: Adaptive Data Retention Based on Variance
 * 
 * This example demonstrates variance-based adaptive retention where
 * time periods with high fluctuations are preserved at higher resolution
 * while stable periods can be aggressively downsampled.
 * 
 * Key Concept: Coefficient of Variation (CV) = stddev / mean × 100%
 * - Low CV (<5%): Stable data → Aggressive downsampling (1h)
 * - Medium CV (5-20%): Moderate variance → Medium downsampling (15m)
 * - High CV (>20%): High variance/anomalies → Keep detailed (1m or 1s)
 * 
 * Benefits over time-based retention:
 * - Preserves important anomalies and events
 * - Reduces storage while maintaining analytical value
 * - Intelligent per-period decisions
 */

#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include <iostream>

namespace themis {
namespace examples {

/**
 * Configuration for adaptive retention
 */
struct AdaptiveRetentionConfig {
    double low_cv_threshold = 5.0;      // CV < 5%: low variance
    double medium_cv_threshold = 20.0;   // CV 5-20%: medium variance
    std::string low_cv_resolution = "1h";
    std::string medium_cv_resolution = "15m";
    std::string high_cv_resolution = "1m";
    int retention_days = 365;            // Apply to data older than this
};

/**
 * Example 1: Simple Adaptive Retention Based on Variance
 * 
 * Analyzes each hour's data and decides resolution based on variance.
 */
void example_simple_adaptive_retention(TaskScheduler& scheduler) {
    std::cout << "=== Example 1: Simple Adaptive Retention ===" << std::endl;
    
    ScheduledTask adaptive_task;
    adaptive_task.id = "adaptive_retention_simple";
    adaptive_task.name = "Variance-Based Adaptive Retention";
    adaptive_task.description = "Downsample based on coefficient of variation";
    adaptive_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    // Multi-step AQL query:
    // 1. Analyze variance in each hour
    // 2. Calculate coefficient of variation (CV)
    // 3. Decide target resolution based on CV
    // 4. Aggregate accordingly
    adaptive_task.aql_query = R"(
        // Step 1: Analyze each hour's variance
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp < DATE_SUB(NOW(), 90, 'days')
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            hour = DATE_TRUNC(d.timestamp, 'hour')
        AGGREGATE
            avg = AVG(d.value),
            stddev = STDDEV(d.value),
            min_val = MIN(d.value),
            max_val = MAX(d.value),
            count = COUNT(d)
        
        // Step 2: Calculate coefficient of variation (CV)
        LET cv = (stddev / ABS(avg)) * 100
        
        // Step 3: Classify variance level
        LET variance_level = (
            cv < 5 ? 'low' :
            cv < 20 ? 'medium' :
            'high'
        )
        
        // Step 4: Determine target resolution
        LET target_resolution = (
            cv < 5 ? '1h' :      // Low variance: aggressive downsampling
            cv < 20 ? '15m' :    // Medium variance: moderate downsampling
            '1m'                 // High variance: preserve detail
        )
        
        // Step 5: For low variance, we can use the hour aggregate directly
        // For medium/high, we need to re-aggregate at finer resolution
        FILTER cv < 5  // Only process low-variance periods in this task
        
        // Step 6: Insert adaptive aggregate
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: hour,
            resolution: target_resolution,
            value: avg,
            statistics: {
                avg: avg,
                stddev: stddev,
                cv: cv,
                min: min_val,
                max: max_val,
                count: count,
                variance_level: variance_level,
                original_resolution: '1s'
            },
            retention_strategy: 'adaptive',
            created_at: DATE_NOW()
        } INTO timeseries_adaptive
        
        RETURN {
            metric: metric,
            entity: entity,
            hour: hour,
            cv: cv,
            variance_level: variance_level,
            target_resolution: target_resolution,
            points_processed: count
        }
    )";
    
    adaptive_task.interval = std::chrono::hours(6);  // Run every 6 hours
    adaptive_task.timeout = std::chrono::hours(1);
    
    std::string task_id = scheduler.registerTask(adaptive_task);
    std::cout << "Registered adaptive retention task: " << task_id << std::endl;
}

/**
 * Example 2: Adaptive Retention with Anomaly Detection
 * 
 * Combines variance analysis with anomaly detection to preserve
 * important events even if they occur in otherwise stable periods.
 */
void example_adaptive_with_anomaly_detection(TaskScheduler& scheduler) {
    std::cout << "=== Example 2: Adaptive with Anomaly Detection ===" << std::endl;
    
    ScheduledTask anomaly_aware_task;
    anomaly_aware_task.id = "adaptive_retention_anomaly_aware";
    anomaly_aware_task.name = "Adaptive Retention with Anomaly Preservation";
    anomaly_aware_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    anomaly_aware_task.aql_query = R"(
        // Step 1: Analyze variance and detect anomalies
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp < DATE_SUB(NOW(), 90, 'days')
        
        // Calculate rolling statistics for anomaly detection
        LET window_stats = (
            FOR w IN timeseries
            FILTER w.metric == d.metric
            AND w.entity == d.entity
            AND w.timestamp BETWEEN DATE_SUB(d.timestamp, 1, 'hour') 
                                AND DATE_ADD(d.timestamp, 1, 'hour')
            RETURN w.value
        )
        
        LET window_avg = AVG(window_stats)
        LET window_stddev = STDDEV(window_stats)
        
        // Detect outliers (>3 sigma)
        LET is_anomaly = ABS(d.value - window_avg) > 3 * window_stddev
        
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            hour = DATE_TRUNC(d.timestamp, 'hour')
        AGGREGATE
            avg = AVG(d.value),
            stddev = STDDEV(d.value),
            min_val = MIN(d.value),
            max_val = MAX(d.value),
            count = COUNT(d),
            anomaly_count = SUM(is_anomaly ? 1 : 0)
        
        LET cv = (stddev / ABS(avg)) * 100
        LET has_anomalies = anomaly_count > 0
        
        // If period has anomalies, always keep higher resolution
        LET target_resolution = (
            has_anomalies ? '1s' :      // Preserve anomalous periods
            cv < 5 ? '1h' :              // Low variance
            cv < 20 ? '15m' :            // Medium variance
            '1m'                         // High variance
        )
        
        INSERT {
            metric: metric,
            entity: entity,
            timestamp: hour,
            resolution: target_resolution,
            value: avg,
            statistics: {
                avg: avg,
                stddev: stddev,
                cv: cv,
                min: min_val,
                max: max_val,
                count: count,
                anomaly_count: anomaly_count,
                has_anomalies: has_anomalies
            },
            retention_strategy: 'adaptive_anomaly_aware'
        } INTO timeseries_adaptive
        
        RETURN {
            hour: hour,
            cv: cv,
            anomaly_count: anomaly_count,
            resolution: target_resolution
        }
    )";
    
    anomaly_aware_task.interval = std::chrono::hours(12);
    scheduler.registerTask(anomaly_aware_task);
    std::cout << "Registered anomaly-aware adaptive task" << std::endl;
}

/**
 * Example 3: Hybrid Strategy - Gorilla + Adaptive + Time-Based
 * 
 * Implements a three-stage strategy:
 * - Stage 1 (0-7 days): Gorilla compression (lossless)
 * - Stage 2 (7-365 days): Adaptive retention (variance-based)
 * - Stage 3 (>365 days): Time-based retention (1d aggregates)
 */
void example_hybrid_strategy(TaskScheduler& scheduler) {
    std::cout << "=== Example 3: Hybrid Strategy (Gorilla + Adaptive + Time) ===" << std::endl;
    
    // Stage 1: Apply Gorilla compression to hot data (0-7 days)
    scheduler.registerFunction("apply_gorilla_compression",
        [](const nlohmann::json& params) -> nlohmann::json {
            // This would interface with the Gorilla compression system
            std::string metric = params.value("metric", "");
            int days = params.value("days", 7);
            
            // Pseudo-code: Compress recent data with Gorilla
            // auto data = query_recent_data(metric, days);
            // auto compressed = gorilla_compress(data);
            // store_compressed(compressed);
            
            return nlohmann::json{
                {"status", "success"},
                {"stage", "gorilla_compression"},
                {"metric", metric},
                {"days", days},
                {"compression_ratio", 10.5}  // Typical Gorilla ratio
            };
        }
    );
    
    ScheduledTask gorilla_task;
    gorilla_task.name = "Stage 1: Gorilla Compression (Hot Data)";
    gorilla_task.type = ScheduledTask::TaskType::FUNCTION;
    gorilla_task.function_name = "apply_gorilla_compression";
    gorilla_task.parameters = {{"days", 7}};
    gorilla_task.interval = std::chrono::hours(24);
    scheduler.registerTask(gorilla_task);
    
    // Stage 2: Adaptive retention for warm data (7-365 days)
    ScheduledTask adaptive_task;
    adaptive_task.name = "Stage 2: Adaptive Retention (Warm Data)";
    adaptive_task.type = ScheduledTask::TaskType::AQL_QUERY;
    adaptive_task.aql_query = R"(
        FOR d IN timeseries
        FILTER d.resolution == '1s'
        AND d.timestamp BETWEEN DATE_SUB(NOW(), 365, 'days') 
                           AND DATE_SUB(NOW(), 7, 'days')
        COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
        AGGREGATE
            avg = AVG(d.value),
            stddev = STDDEV(d.value)
        LET cv = (stddev / ABS(avg)) * 100
        LET resolution = cv < 5 ? '1h' : cv < 20 ? '15m' : '1m'
        INSERT {
            timestamp: hour,
            resolution: resolution,
            value: avg,
            statistics: {avg: avg, stddev: stddev, cv: cv},
            stage: 'adaptive'
        } INTO timeseries_adaptive
    )";
    adaptive_task.interval = std::chrono::hours(12);
    scheduler.registerTask(adaptive_task);
    
    // Stage 3: Time-based retention for cold data (>365 days)
    ScheduledTask timebased_task;
    timebased_task.name = "Stage 3: Time-Based Retention (Cold Data)";
    timebased_task.type = ScheduledTask::TaskType::AQL_QUERY;
    timebased_task.aql_query = R"(
        FOR d IN timeseries_adaptive
        FILTER d.timestamp < DATE_SUB(NOW(), 1, 'year')
        COLLECT day = DATE_TRUNC(d.timestamp, 'day')
        AGGREGATE
            avg = AVG(d.value),
            min_val = MIN(d.statistics.min),
            max_val = MAX(d.statistics.max)
        INSERT {
            timestamp: day,
            resolution: '1d',
            value: avg,
            statistics: {avg: avg, min: min_val, max: max_val},
            stage: 'time_based_cold'
        } INTO timeseries_longterm
    )";
    timebased_task.interval = std::chrono::hours(24);
    scheduler.registerTask(timebased_task);
    
    std::cout << "Registered 3-stage hybrid strategy" << std::endl;
}

/**
 * Example 4: Configurable Adaptive Retention with Custom Thresholds
 * 
 * Allows per-metric configuration of variance thresholds.
 */
void example_configurable_adaptive(TaskScheduler& scheduler, QueryEngine* query_engine) {
    std::cout << "=== Example 4: Configurable Adaptive Retention ===" << std::endl;
    
    scheduler.registerFunction("adaptive_retention_configurable",
        [query_engine](const nlohmann::json& params) -> nlohmann::json {
            std::string metric = params.value("metric", "*");
            double low_cv = params.value("low_cv_threshold", 5.0);
            double medium_cv = params.value("medium_cv_threshold", 20.0);
            std::string low_res = params.value("low_cv_resolution", "1h");
            std::string medium_res = params.value("medium_cv_resolution", "15m");
            std::string high_res = params.value("high_cv_resolution", "1m");
            int retention_days = params.value("retention_days", 90);
            
            // Build AQL query with dynamic thresholds
            std::ostringstream aql = {};
            aql << "FOR d IN timeseries "
                << "FILTER d.resolution == '1s' ";
            
            if (metric != "*") {
                aql << "AND d.metric == '" << metric << "' ";
            }
            
            aql << "AND d.timestamp < DATE_SUB(NOW(), " << retention_days << ", 'days') "
                << "COLLECT hour = DATE_TRUNC(d.timestamp, 'hour') "
                << "AGGREGATE "
                << "  avg = AVG(d.value), "
                << "  stddev = STDDEV(d.value), "
                << "  count = COUNT(d) "
                << "LET cv = (stddev / ABS(avg)) * 100 "
                << "LET resolution = ( "
                << "  cv < " << low_cv << " ? '" << low_res << "' : "
                << "  cv < " << medium_cv << " ? '" << medium_res << "' : "
                << "  '" << high_res << "' "
                << ") "
                << "INSERT { "
                << "  timestamp: hour, "
                << "  resolution: resolution, "
                << "  value: avg, "
                << "  statistics: {avg: avg, stddev: stddev, cv: cv, count: count} "
                << "} INTO timeseries_adaptive "
                << "RETURN {hour: hour, cv: cv, resolution: resolution}";
            
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
    
    // Temperature sensors: Very stable, aggressive downsampling
    ScheduledTask temp_policy;
    temp_policy.name = "Temperature Adaptive Policy";
    temp_policy.type = ScheduledTask::TaskType::FUNCTION;
    temp_policy.function_name = "adaptive_retention_configurable";
    temp_policy.parameters = {
        {"metric", "temperature"},
        {"low_cv_threshold", 3.0},     // More aggressive
        {"medium_cv_threshold", 10.0},
        {"low_cv_resolution", "1h"},
        {"medium_cv_resolution", "30m"},
        {"high_cv_resolution", "5m"},
        {"retention_days", 90}
    };
    temp_policy.interval = std::chrono::hours(12);
    scheduler.registerTask(temp_policy);
    
    // Vibration sensors: Highly variable, preserve detail
    ScheduledTask vibration_policy;
    vibration_policy.name = "Vibration Adaptive Policy";
    vibration_policy.type = ScheduledTask::TaskType::FUNCTION;
    vibration_policy.function_name = "adaptive_retention_configurable";
    vibration_policy.parameters = {
        {"metric", "vibration"},
        {"low_cv_threshold", 10.0},    // Less aggressive
        {"medium_cv_threshold", 30.0},
        {"low_cv_resolution", "15m"},
        {"medium_cv_resolution", "5m"},
        {"high_cv_resolution", "1s"},  // Keep high resolution!
        {"retention_days", 90}
    };
    vibration_policy.interval = std::chrono::hours(6);
    scheduler.registerTask(vibration_policy);
    
    std::cout << "Registered configurable adaptive policies for 2 metrics" << std::endl;
}

/**
 * Example 5: Variance Analysis and Reporting
 * 
 * Analyzes historical variance patterns to help calibrate thresholds.
 */
void example_variance_analysis(TaskScheduler& scheduler) {
    std::cout << "=== Example 5: Variance Analysis & Reporting ===" << std::endl;
    
    scheduler.registerFunction("analyze_variance_patterns",
        [](const nlohmann::json& params) -> nlohmann::json {
            // This would analyze actual variance patterns
            // For demonstration, return sample analysis
            
            return nlohmann::json{
                {"analysis_period", "last_30_days"},
                {"metrics", {
                    {
                        {"metric", "temperature"},
                        {"avg_cv", 4.2},
                        {"cv_distribution", {
                            {"low_cv_percent", 78.5},
                            {"medium_cv_percent", 18.3},
                            {"high_cv_percent", 3.2}
                        }},
                        {"recommended_thresholds", {
                            {"low_cv", 5.0},
                            {"medium_cv", 15.0}
                        }},
                        {"estimated_storage_reduction", "99.2%"}
                    },
                    {
                        {"metric", "vibration"},
                        {"avg_cv", 28.7},
                        {"cv_distribution", {
                            {"low_cv_percent", 12.1},
                            {"medium_cv_percent", 35.4},
                            {"high_cv_percent", 52.5}
                        }},
                        {"recommended_thresholds", {
                            {"low_cv", 10.0},
                            {"medium_cv", 30.0}
                        }},
                        {"estimated_storage_reduction", "85.3%"}
                    }
                }}
            };
        }
    );
    
    ScheduledTask analysis_task;
    analysis_task.name = "Variance Analysis Report";
    analysis_task.type = ScheduledTask::TaskType::FUNCTION;
    analysis_task.function_name = "analyze_variance_patterns";
    analysis_task.interval = std::chrono::hours(168);  // Weekly
    
    analysis_task.on_success = [](const std::string& task_id, const nlohmann::json& result) {
        std::cout << "\n=== Variance Analysis Report ===" << std::endl;
        std::cout << result.dump(2) << std::endl;
        std::cout << "=================================\n" << std::endl;
    };
    
    scheduler.registerTask(analysis_task);
    std::cout << "Registered variance analysis task (runs weekly)" << std::endl;
}

/**
 * Main example runner
 */
void run_adaptive_retention_examples(QueryEngine* query_engine) {
    std::cout << "====================================================" << std::endl;
    std::cout << "  Adaptive Retention Examples" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    TaskScheduler::Config config;
    config.max_concurrent_tasks = 4;
    config.check_interval = std::chrono::seconds(10);
    config.persist_tasks = true;
    
    TaskScheduler scheduler(query_engine, config);
    
    // Run all examples
    example_simple_adaptive_retention(scheduler);
    example_adaptive_with_anomaly_detection(scheduler);
    example_hybrid_strategy(scheduler);
    example_configurable_adaptive(scheduler, query_engine);
    example_variance_analysis(scheduler);
    
    std::cout << "\n====================================================" << std::endl;
    std::cout << "All adaptive retention tasks registered!" << std::endl;
    std::cout << "====================================================" << std::endl;
}

} // namespace examples
} // namespace themis
