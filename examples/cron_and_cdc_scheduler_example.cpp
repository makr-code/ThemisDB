/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cron_and_cdc_scheduler_example.cpp                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     317                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ae4751578e  2026-03-09  fix(scheduler): address remaining documentation and examp... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cron_and_cdc_scheduler_example.cpp
 * @brief Example demonstrating cron-based and CDC event-based task scheduling
 * 
 * This example shows how to use the enhanced TaskScheduler with:
 * - Cron expressions for time-based scheduling
 * - CDC events for reactive task execution
 * - Hybrid scheduling (combining time and events)
 * - Manual task execution
 * - Priority-based task ordering
 */

#include "scheduler/task_scheduler.h"
#include "cdc/changefeed.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <iostream>
#include <thread>

using namespace themis;

void example_cron_based_tasks(TaskScheduler& scheduler) {
    std::cout << "\n=== Example 1: Cron-Based Tasks ===" << std::endl;
    
    // Task 1: Daily backup at 2 AM
    ScheduledTask backup_task;
    backup_task.name = "daily_backup";
    backup_task.description = "Perform daily database backup";
    backup_task.type = ScheduledTask::TaskType::FUNCTION;
    backup_task.function_name = "perform_backup";
    backup_task.trigger_type = ScheduledTask::TriggerType::CRON;
    backup_task.cron_expression = "0 2 * * *";  // Daily at 2:00 AM
    
    std::string backup_id = scheduler.registerTask(backup_task);
    std::cout << "Registered daily backup task: " << backup_id << std::endl;
    
    // Task 2: Weekday business hours monitoring
    ScheduledTask monitoring_task;
    monitoring_task.name = "business_hours_monitor";
    monitoring_task.description = "Monitor system during business hours";
    monitoring_task.type = ScheduledTask::TaskType::FUNCTION;
    monitoring_task.function_name = "monitor_system";
    monitoring_task.trigger_type = ScheduledTask::TriggerType::CRON;
    monitoring_task.cron_expression = "*/15 9-17 * * 1-5";  // Every 15 min, Mon-Fri 9-5
    monitoring_task.priority = ScheduledTask::Priority::HIGH;
    
    std::string monitor_id = scheduler.registerTask(monitoring_task);
    std::cout << "Registered business hours monitoring: " << monitor_id << std::endl;
    
    // Task 3: Monthly report generation
    ScheduledTask report_task;
    report_task.name = "monthly_report";
    report_task.description = "Generate monthly analytics report";
    report_task.type = ScheduledTask::TaskType::FUNCTION;
    report_task.function_name = "generate_report";
    report_task.trigger_type = ScheduledTask::TriggerType::CRON;
    report_task.cron_expression = "0 0 1 * *";  // First day of month at midnight
    
    std::string report_id = scheduler.registerTask(report_task);
    std::cout << "Registered monthly report task: " << report_id << std::endl;
}

void example_cdc_event_tasks(TaskScheduler& scheduler) {
    std::cout << "\n=== Example 2: CDC Event-Based Tasks ===" << std::endl;
    
    // Task 1: Process new user registrations
    ScheduledTask user_task;
    user_task.name = "new_user_processor";
    user_task.description = "Process new user registrations";
    user_task.type = ScheduledTask::TaskType::FUNCTION;
    user_task.function_name = "process_new_user";
    user_task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    user_task.cdc_trigger.key_prefix = "users:";
    user_task.cdc_trigger.event_types.insert(0);  // EVENT_PUT
    user_task.cdc_trigger.debounce_ms = 100;      // 100ms debounce
    user_task.priority = ScheduledTask::Priority::HIGH;
    
    std::string user_id = scheduler.registerTask(user_task);
    std::cout << "Registered user registration processor: " << user_id << std::endl;
    
    // Task 2: Cleanup deleted orders
    ScheduledTask cleanup_task;
    cleanup_task.name = "order_cleanup";
    cleanup_task.description = "Clean up deleted order data";
    cleanup_task.type = ScheduledTask::TaskType::FUNCTION;
    cleanup_task.function_name = "cleanup_order";
    cleanup_task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    cleanup_task.cdc_trigger.key_prefix = "orders:";
    cleanup_task.cdc_trigger.event_types.insert(1);  // EVENT_DELETE
    cleanup_task.priority = ScheduledTask::Priority::LOW;
    
    std::string cleanup_id = scheduler.registerTask(cleanup_task);
    std::cout << "Registered order cleanup task: " << cleanup_id << std::endl;
    
    // Task 3: Audit log for all changes
    ScheduledTask audit_task;
    audit_task.name = "audit_logger";
    audit_task.description = "Log all database changes";
    audit_task.type = ScheduledTask::TaskType::FUNCTION;
    audit_task.function_name = "log_change";
    audit_task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    audit_task.cdc_trigger.key_prefix = "*";  // All keys
    audit_task.cdc_trigger.event_types.insert(0);  // EVENT_PUT
    audit_task.cdc_trigger.event_types.insert(1);  // EVENT_DELETE
    
    std::string audit_id = scheduler.registerTask(audit_task);
    std::cout << "Registered audit logger: " << audit_id << std::endl;
}

void example_manual_tasks(TaskScheduler& scheduler) {
    std::cout << "\n=== Example 3: Manual Tasks ===" << std::endl;
    
    // Task: On-demand data migration
    ScheduledTask migration_task;
    migration_task.name = "data_migration";
    migration_task.description = "Manual data migration";
    migration_task.type = ScheduledTask::TaskType::FUNCTION;
    migration_task.function_name = "migrate_data";
    migration_task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::string migration_id = scheduler.registerTask(migration_task);
    std::cout << "Registered manual migration task: " << migration_id << std::endl;
    
    // Execute manually when needed
    std::cout << "Executing migration manually..." << std::endl;
    auto result = scheduler.executeTaskNow(migration_id);
    std::cout << "Migration result: " << result.dump(2) << std::endl;
}

void example_hybrid_scheduling(TaskScheduler& scheduler) {
    std::cout << "\n=== Example 4: Hybrid Scheduling ===" << std::endl;
    
    // Task: Cache refresh - both time-based and event-based
    // Refresh cache every hour OR when data changes
    ScheduledTask cache_task;
    cache_task.name = "cache_refresh";
    cache_task.description = "Refresh cache periodically and on data changes";
    cache_task.type = ScheduledTask::TaskType::FUNCTION;
    cache_task.function_name = "refresh_cache";
    
    // Set up both cron and CDC triggers
    cache_task.trigger_type = ScheduledTask::TriggerType::CRON;
    cache_task.cron_expression = "0 * * * *";  // Every hour
    
    // Also trigger on data changes
    cache_task.cdc_trigger.key_prefix = "products:";
    cache_task.cdc_trigger.event_types.insert(0);  // EVENT_PUT
    cache_task.cdc_trigger.event_types.insert(1);  // EVENT_DELETE
    cache_task.cdc_trigger.debounce_ms = 5000;     // 5 second debounce
    
    // Use OR logic - trigger on time OR event
    cache_task.trigger_logic = ScheduledTask::TriggerLogic::OR;
    
    std::string cache_id = scheduler.registerTask(cache_task);
    std::cout << "Registered hybrid cache refresh task: " << cache_id << std::endl;
    std::cout << "  - Triggers: hourly OR on product changes" << std::endl;
}

void register_task_functions(TaskScheduler& scheduler) {
    // Register all the functions used by tasks
    
    scheduler.registerFunction("perform_backup", [](const nlohmann::json& params) {
        std::cout << "  [BACKUP] Performing database backup..." << std::endl;
        // Simulate backup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return nlohmann::json{{"status", "success"}, {"backup_size_mb", 1024}};
    });
    
    scheduler.registerFunction("monitor_system", [](const nlohmann::json& params) {
        std::cout << "  [MONITOR] Checking system health..." << std::endl;
        return nlohmann::json{{"status", "healthy"}, {"cpu_usage", 45.2}};
    });
    
    scheduler.registerFunction("generate_report", [](const nlohmann::json& params) {
        std::cout << "  [REPORT] Generating monthly report..." << std::endl;
        return nlohmann::json{{"status", "success"}, {"records_processed", 100000}};
    });
    
    scheduler.registerFunction("process_new_user", [](const nlohmann::json& params) {
        std::cout << "  [USER] Processing new user registration..." << std::endl;
        return nlohmann::json{{"status", "processed"}};
    });
    
    scheduler.registerFunction("cleanup_order", [](const nlohmann::json& params) {
        std::cout << "  [CLEANUP] Cleaning up deleted order..." << std::endl;
        return nlohmann::json{{"status", "cleaned"}};
    });
    
    scheduler.registerFunction("log_change", [](const nlohmann::json& params) {
        std::cout << "  [AUDIT] Logging database change..." << std::endl;
        return nlohmann::json{{"status", "logged"}};
    });
    
    scheduler.registerFunction("migrate_data", [](const nlohmann::json& params) {
        std::cout << "  [MIGRATION] Migrating data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return nlohmann::json{{"status", "completed"}, {"records_migrated", 50000}};
    });
    
    scheduler.registerFunction("refresh_cache", [](const nlohmann::json& params) {
        std::cout << "  [CACHE] Refreshing cache..." << std::endl;
        return nlohmann::json{{"status", "refreshed"}, {"cache_entries", 10000}};
    });
}

int main() {
    std::cout << "=== Task Scheduler with Cron and CDC Triggers Example ===" << std::endl;
    
    try {
        // Setup storage and changefeed
        RocksDBWrapper::Config storage_config;
        storage_config.db_path = "data/scheduler_example";
        auto storage = std::make_shared<RocksDBWrapper>(storage_config);
        
        if (!storage->open()) {
            std::cerr << "Failed to open database" << std::endl;
            return 1;
        }
        
        auto changefeed = std::make_unique<Changefeed>(storage->getRawDB());
        auto idx = std::make_unique<SecondaryIndexManager>(*storage);
        auto query_engine = std::make_unique<QueryEngine>(*storage, *idx);
        
        // Create scheduler with CDC support
        TaskScheduler::Config scheduler_config;
        scheduler_config.max_concurrent_tasks = 4;
        scheduler_config.check_interval = std::chrono::seconds(10);
        scheduler_config.persist_tasks = true;
        scheduler_config.persistence_path = "data/scheduler_example";
        
        TaskScheduler scheduler(query_engine.get(), scheduler_config, changefeed.get());
        
        // Register all task functions
        register_task_functions(scheduler);
        
        // Run examples
        example_cron_based_tasks(scheduler);
        example_cdc_event_tasks(scheduler);
        example_manual_tasks(scheduler);
        example_hybrid_scheduling(scheduler);
        
        // Start scheduler
        std::cout << "\n=== Starting Scheduler ===" << std::endl;
        scheduler.start();
        
        // Simulate some CDC events
        std::cout << "\n=== Simulating CDC Events ===" << std::endl;
        
        Changefeed::ChangeEvent user_event;
        user_event.type = Changefeed::ChangeEventType::EVENT_PUT;
        user_event.key = "users:12345";
        user_event.value = "{\"name\": \"John Doe\", \"email\": \"john@example.com\"}";
        user_event.timestamp_ms = 1000;
        changefeed->recordEvent(user_event);
        std::cout << "Recorded user registration event" << std::endl;
        
        Changefeed::ChangeEvent product_event;
        product_event.type = Changefeed::ChangeEventType::EVENT_PUT;
        product_event.key = "products:789";
        product_event.value = "{\"name\": \"Widget\", \"price\": 19.99}";
        product_event.timestamp_ms = 2000;
        changefeed->recordEvent(product_event);
        std::cout << "Recorded product update event" << std::endl;
        
        // Let scheduler run for a bit
        std::cout << "\nScheduler running... (Ctrl+C to stop)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Show statistics
        auto stats = scheduler.getStats();
        std::cout << "\n=== Scheduler Statistics ===" << std::endl;
        std::cout << "Registered tasks: " << stats.registered_tasks << std::endl;
        std::cout << "Active tasks: " << stats.active_tasks << std::endl;
        std::cout << "Running tasks: " << stats.running_tasks << std::endl;
        std::cout << "Total executions: " << stats.total_executions << std::endl;
        std::cout << "Failed executions: " << stats.failed_executions << std::endl;
        
        // Stop scheduler
        std::cout << "\n=== Stopping Scheduler ===" << std::endl;
        scheduler.stop();
        
        storage->close();
        
        std::cout << "\nExample completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
