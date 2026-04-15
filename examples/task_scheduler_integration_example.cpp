/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_scheduler_integration_example.cpp             ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file task_scheduler_integration_example.cpp
 * @brief Example of integrating TaskScheduler with TSAutoBuffer for post-processing
 * 
 * This example demonstrates how to use the TaskScheduler for periodic
 * compression and aggregation of IoT time-series data stored via TSAutoBuffer.
 */

#include "scheduler/task_scheduler.h"
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/tsstore.h"
#include "timeseries/gorilla.h"
#include "query/query_engine.h"
#include <iostream>

namespace themis {
namespace examples {

/**
 * Example 1: Periodic Gorilla Compression of Batched Data
 * 
 * This function shows how to compress old time-series data that was
 * initially stored uncompressed via TSAutoBuffer.
 */
void example_periodic_compression(TaskScheduler& scheduler) {
    std::cout << "=== Example 1: Periodic Gorilla Compression ===" << std::endl;
    
    ScheduledTask compression_task;
    compression_task.id = "iot_data_compression";
    compression_task.name = "IoT Data Gorilla Compression";
    compression_task.description = "Compress IoT time-series data older than 1 hour using Gorilla algorithm";
    compression_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    // AQL query to find and compress old data
    // In production, you would implement a custom compression function
    compression_task.aql_query = R"(
        FOR d IN timeseries
        FILTER d.timestamp < DATE_SUB(NOW(), 1, 'hour')
        AND d.compressed == false
        COLLECT metric = d.metric, entity = d.entity INTO batch = d
        RETURN {
            metric: metric,
            entity: entity,
            batch_size: LENGTH(batch),
            action: 'compress_with_gorilla'
        }
    )";
    
    compression_task.interval = std::chrono::minutes(10);
    compression_task.timeout = std::chrono::minutes(5);
    
    std::string task_id = scheduler.registerTask(compression_task);
    std::cout << "Registered compression task: " << task_id << std::endl;
}

/**
 * Example 2: Custom Function for Batch Compression
 * 
 * This shows how to register a custom function that directly compresses
 * batched data using Gorilla compression.
 */
void example_custom_compression_function(TaskScheduler& scheduler, TSStore* tsstore) {
    std::cout << "=== Example 2: Custom Compression Function ===" << std::endl;
    
    // Register a custom function for batch compression
    scheduler.registerFunction("compress_iot_batch", 
        [tsstore](const nlohmann::json& params) -> nlohmann::json {
            std::string metric = params.value("metric", "");
            std::string entity = params.value("entity", "");
            int64_t cutoff_ms = params.value("cutoff_ms", 0);
            
            // Query time-series data
            auto points = tsstore->query(metric, entity, 0, cutoff_ms);
            
            if (points.empty()) {
                return nlohmann::json{
                    {"status", "skipped"},
                    {"reason", "no data to compress"}
                };
            }
            
            // Apply Gorilla compression
            GorillaEncoder encoder;
            for (const auto& point : points) {
                encoder.encode(point.timestamp_ms, point.value);
            }
            auto compressed = encoder.finish();
            
            // Calculate compression ratio
            size_t original_size = points.size() * (sizeof(int64_t) + sizeof(double));
            size_t compressed_size = compressed.size();
            double ratio = static_cast<double>(original_size) / compressed_size;
            
            // In production: Store compressed data back to RocksDB
            // For now, just return statistics
            
            return nlohmann::json{
                {"status", "success"},
                {"metric", metric},
                {"entity", entity},
                {"points_compressed", points.size()},
                {"original_size_bytes", original_size},
                {"compressed_size_bytes", compressed_size},
                {"compression_ratio", ratio}
            };
        }
    );
    
    // Create task using the custom function
    ScheduledTask func_task;
    func_task.name = "Batch Compression Task";
    func_task.type = ScheduledTask::TaskType::FUNCTION;
    func_task.function_name = "compress_iot_batch";
    func_task.parameters = {
        {"metric", "temperature"},
        {"entity", "sensor_001"},
        {"cutoff_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch() - 
            std::chrono::hours(1)
        ).count()}
    };
    func_task.interval = std::chrono::minutes(5);
    
    std::string task_id = scheduler.registerTask(func_task);
    std::cout << "Registered custom function task: " << task_id << std::endl;
}

/**
 * Example 3: Data Aggregation and Downsampling
 * 
 * Aggregates high-frequency IoT data to lower resolutions for efficient querying.
 */
void example_data_aggregation(TaskScheduler& scheduler) {
    std::cout << "=== Example 3: Data Aggregation ===" << std::endl;
    
    ScheduledTask agg_task;
    agg_task.name = "IoT Data Aggregation";
    agg_task.description = "Aggregate 1-second data to 1-minute resolution";
    agg_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    // Aggregate high-frequency data
    agg_task.aql_query = R"(
        FOR d IN timeseries
        FILTER d.timestamp < DATE_SUB(NOW(), 5, 'minutes')
        AND d.resolution == '1s'
        COLLECT 
            metric = d.metric,
            entity = d.entity,
            minute = DATE_TRUNC(d.timestamp, 'minute')
        AGGREGATE 
            avg_value = AVG(d.value),
            min_value = MIN(d.value),
            max_value = MAX(d.value),
            count = COUNT(d)
        RETURN {
            metric: metric,
            entity: entity,
            timestamp: minute,
            resolution: '1m',
            avg: avg_value,
            min: min_value,
            max: max_value,
            count: count
        }
    )";
    
    agg_task.interval = std::chrono::minutes(5);
    
    std::string task_id = scheduler.registerTask(agg_task);
    std::cout << "Registered aggregation task: " << task_id << std::endl;
}

/**
 * Example 4: Data Cleanup
 * 
 * Removes old raw data after aggregates are created.
 */
void example_data_cleanup(TaskScheduler& scheduler) {
    std::cout << "=== Example 4: Data Cleanup ===" << std::endl;
    
    ScheduledTask cleanup_task;
    cleanup_task.name = "Old Data Cleanup";
    cleanup_task.description = "Remove raw data older than 30 days (keep aggregates)";
    cleanup_task.type = ScheduledTask::TaskType::AQL_QUERY;
    
    cleanup_task.aql_query = R"(
        FOR d IN timeseries
        FILTER d.timestamp < DATE_SUB(NOW(), 30, 'days')
        AND d.resolution == '1s'
        REMOVE d IN timeseries
        RETURN OLD
    )";
    
    // Run daily at off-peak hours
    cleanup_task.interval = std::chrono::hours(24);
    cleanup_task.timeout = std::chrono::hours(1);
    
    std::string task_id = scheduler.registerTask(cleanup_task);
    std::cout << "Registered cleanup task: " << task_id << std::endl;
}

/**
 * Example 5: Monitoring TSAutoBuffer and Triggering Compression
 * 
 * Monitors buffer statistics and triggers compression when thresholds are met.
 */
void example_buffer_monitoring(TaskScheduler& scheduler, TSAutoBuffer* buffer) {
    std::cout << "=== Example 5: Buffer Monitoring ===" << std::endl;
    
    scheduler.registerFunction("monitor_buffer", 
        [buffer](const nlohmann::json& params) -> nlohmann::json {
            auto stats = buffer->getStats();
            
            // Check if buffer is getting full
            double fill_ratio = static_cast<double>(stats.current_buffer_size) / 
                               stats.max_buffer_size;
            
            nlohmann::json result{
                {"current_size", stats.current_buffer_size},
                {"max_size", stats.max_buffer_size},
                {"fill_ratio", fill_ratio},
                {"points_buffered", stats.points_buffered},
                {"points_flushed", stats.points_flushed}
            };
            
            // If buffer is > 80% full, trigger flush
            if (fill_ratio > 0.8) {
                buffer->flush();
                result["action"] = "triggered_flush";
            } else {
                result["action"] = "no_action";
            }
            
            return result;
        }
    );
    
    ScheduledTask monitor_task;
    monitor_task.name = "Buffer Monitor";
    monitor_task.type = ScheduledTask::TaskType::FUNCTION;
    monitor_task.function_name = "monitor_buffer";
    monitor_task.interval = std::chrono::seconds(30);
    
    std::string task_id = scheduler.registerTask(monitor_task);
    std::cout << "Registered buffer monitoring task: " << task_id << std::endl;
}

/**
 * Main example function
 */
void run_integration_examples(QueryEngine* query_engine, 
                               TSStore* tsstore, 
                               TSAutoBuffer* buffer) {
    std::cout << "====================================================" << std::endl;
    std::cout << "  TaskScheduler Integration Examples" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Create scheduler
    TaskScheduler::Config config;
    config.max_concurrent_tasks = 4;
    config.check_interval = std::chrono::seconds(10);
    config.persist_tasks = true;
    config.persistence_path = "data/tasks";
    
    TaskScheduler scheduler(query_engine, config);
    
    // Run examples
    example_periodic_compression(scheduler);
    example_custom_compression_function(scheduler, tsstore);
    example_data_aggregation(scheduler);
    example_data_cleanup(scheduler);
    example_buffer_monitoring(scheduler, buffer);
    
    // Start scheduler
    std::cout << "\nStarting scheduler..." << std::endl;
    scheduler.start();
    
    // Print initial statistics
    auto stats = scheduler.getStats();
    std::cout << "\nScheduler Statistics:" << std::endl;
    std::cout << "  Registered tasks: " << stats.registered_tasks << std::endl;
    std::cout << "  Active tasks: " << stats.active_tasks << std::endl;
    
    // In production, the scheduler would run indefinitely
    // For this example, we'll run for a short time
    std::cout << "\nScheduler is running. Press Ctrl+C to stop." << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // scheduler.stop() would be called on shutdown
}

} // namespace examples
} // namespace themis
