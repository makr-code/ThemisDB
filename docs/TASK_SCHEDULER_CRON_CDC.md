# Task Scheduler: Cron and CDC Event Triggers

## Overview

The ThemisDB TaskScheduler has been enhanced with advanced scheduling capabilities including:
- **Cron Expression Support**: Schedule tasks using standard 5-field cron syntax
- **CDC Event Triggers**: Execute tasks in response to database changes
- **Hybrid Scheduling**: Combine time-based and event-based triggers
- **Priority Levels**: Control task execution order
- **Manual Execution**: On-demand task execution

## Trigger Types

### 1. CRON - Time-Based Scheduling

Execute tasks based on cron expressions following standard 5-field syntax:

```
┌───────────── minute (0 - 59)
│ ┌───────────── hour (0 - 23)
│ │ ┌───────────── day of month (1 - 31)
│ │ │ ┌───────────── month (1 - 12)
│ │ │ │ ┌───────────── day of week (0 - 6) (Sunday=0)
│ │ │ │ │
* * * * *
```

**Supported Syntax:**
- Wildcards: `*` (any value)
- Ranges: `0-5` (values from 0 to 5)
- Lists: `1,3,5` (specific values)
- Steps: `*/15` (every 15 units)
- Step ranges: `0-30/5` (every 5 from 0 to 30)

**Examples:**
```cpp
// Daily at 2 AM
task.cron_expression = "0 2 * * *";

// Every 15 minutes during business hours (Mon-Fri 9-5)
task.cron_expression = "*/15 9-17 * * 1-5";

// First day of every month at midnight
task.cron_expression = "0 0 1 * *";

// Every Sunday at 3:30 AM
task.cron_expression = "30 3 * * 0";
```

### 2. INTERVAL - Fixed Interval (Legacy)

Traditional interval-based scheduling (backward compatible):

```cpp
task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
task.interval = std::chrono::minutes(5);
```

### 3. CDC_EVENT - Database Change Events

Execute tasks when database changes occur:

```cpp
task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
task.cdc_trigger.key_prefix = "users:";
task.cdc_trigger.event_types.insert(0);  // EVENT_PUT
task.cdc_trigger.debounce_ms = 100;
```

**Configuration Options:**
- `key_prefix`: Filter by key prefix (e.g., "users:", "orders:", "*")
- `event_types`: Set of event types to trigger on
  - `0` = EVENT_PUT (insert/update)
  - `1` = EVENT_DELETE
  - `2` = EVENT_TRANSACTION_COMMIT
  - `3` = EVENT_TRANSACTION_ROLLBACK
- `condition`: Optional AQL filter (future feature)
- `debounce_ms`: Minimum milliseconds between triggers

### 4. WEBHOOK - External HTTP Events (Future)

Reserved for external webhook triggers.

### 5. MANUAL - On-Demand Execution

Tasks that only run when explicitly executed:

```cpp
task.trigger_type = ScheduledTask::TriggerType::MANUAL;
scheduler.executeTaskNow(task_id);
```

## Priority Levels

Control task execution order with priority levels:

```cpp
enum class Priority {
    LOW = 0,      // Background tasks, cleanup
    NORMAL = 1,   // Default priority
    HIGH = 2      // Critical, time-sensitive tasks
};

task.priority = ScheduledTask::Priority::HIGH;
```

## Hybrid Scheduling

Combine multiple trigger types with AND/OR logic:

```cpp
// Cache refresh: hourly OR on data changes
task.trigger_type = ScheduledTask::TriggerType::CRON;
task.cron_expression = "0 * * * *";  // Every hour

task.cdc_trigger.key_prefix = "products:";
task.cdc_trigger.event_types.insert(0);  // Also trigger on PUT

task.trigger_logic = ScheduledTask::TriggerLogic::OR;  // Time OR Event
```

**Trigger Logic:**
- `OR`: Execute when ANY trigger condition is met (default)
- `AND`: Execute only when ALL trigger conditions are met

## Usage Examples

### Example 1: Daily Backup

```cpp
ScheduledTask backup;
backup.name = "daily_backup";
backup.type = ScheduledTask::TaskType::FUNCTION;
backup.function_name = "backup_database";
backup.trigger_type = ScheduledTask::TriggerType::CRON;
backup.cron_expression = "0 2 * * *";  // 2 AM daily

scheduler.registerTask(backup);
```

### Example 2: Real-time User Processing

```cpp
ScheduledTask user_processor;
user_processor.name = "process_new_users";
user_processor.type = ScheduledTask::TaskType::FUNCTION;
user_processor.function_name = "process_user";
user_processor.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
user_processor.cdc_trigger.key_prefix = "users:";
user_processor.cdc_trigger.event_types.insert(0);  // PUT events
user_processor.cdc_trigger.debounce_ms = 100;
user_processor.priority = ScheduledTask::Priority::HIGH;

scheduler.registerTask(user_processor);
```

### Example 3: Business Hours Monitoring

```cpp
ScheduledTask monitor;
monitor.name = "system_monitor";
monitor.type = ScheduledTask::TaskType::FUNCTION;
monitor.function_name = "check_system_health";
monitor.trigger_type = ScheduledTask::TriggerType::CRON;
monitor.cron_expression = "*/5 9-17 * * 1-5";  // Every 5 min, weekdays
monitor.priority = ScheduledTask::Priority::HIGH;

scheduler.registerTask(monitor);
```

### Example 4: Manual Data Migration

```cpp
ScheduledTask migration;
migration.name = "data_migration";
migration.type = ScheduledTask::TaskType::FUNCTION;
migration.function_name = "migrate_data";
migration.trigger_type = ScheduledTask::TriggerType::MANUAL;

std::string task_id = scheduler.registerTask(migration);

// Execute when ready
auto result = scheduler.executeTaskNow(task_id);
```

## Cron Expression Reference

### Common Patterns

| Pattern | Description | Example |
|---------|-------------|---------|
| `* * * * *` | Every minute | - |
| `*/5 * * * *` | Every 5 minutes | 0, 5, 10, 15... |
| `0 * * * *` | Every hour | 0:00, 1:00, 2:00... |
| `0 */2 * * *` | Every 2 hours | 0:00, 2:00, 4:00... |
| `0 0 * * *` | Daily at midnight | 00:00 |
| `0 2 * * *` | Daily at 2 AM | 02:00 |
| `0 9-17 * * *` | Every hour 9-5 | 9:00, 10:00...17:00 |
| `0 9 * * 1-5` | Weekdays at 9 AM | Mon-Fri 9:00 |
| `0 0 1 * *` | First of month | 1st at midnight |
| `0 0 * * 0` | Every Sunday | Sunday midnight |
| `30 2 * * 0` | Sunday at 2:30 AM | Sunday 02:30 |

### Field Ranges

| Field | Range | Special Values |
|-------|-------|----------------|
| Minute | 0-59 | - |
| Hour | 0-23 | - |
| Day | 1-31 | - |
| Month | 1-12 | - |
| Weekday | 0-6 | 0=Sunday, 6=Saturday |

## CDC Event Filtering

### Key Prefix Patterns

```cpp
// Exact prefix
cdc_trigger.key_prefix = "users:";        // Matches: users:123, users:abc

// Wildcard (all keys)
cdc_trigger.key_prefix = "*";             // Matches: all keys

// Prefix with wildcard
cdc_trigger.key_prefix = "users:*";       // Matches: users:*
```

### Event Type Combinations

```cpp
// Single event type
cdc_trigger.event_types.insert(0);  // Only PUT events

// Multiple event types
cdc_trigger.event_types.insert(0);  // PUT
cdc_trigger.event_types.insert(1);  // DELETE

// All change events
cdc_trigger.event_types.insert(0);  // PUT
cdc_trigger.event_types.insert(1);  // DELETE
cdc_trigger.event_types.insert(2);  // TRANSACTION_COMMIT
cdc_trigger.event_types.insert(3);  // TRANSACTION_ROLLBACK
```

### Debouncing

Prevent excessive task execution for high-frequency events:

```cpp
// 1 second debounce - max 1 execution per second
cdc_trigger.debounce_ms = 1000;

// 5 second debounce - max 1 execution per 5 seconds
cdc_trigger.debounce_ms = 5000;

// No debouncing
cdc_trigger.debounce_ms = 0;
```

## Migration Guide

### From Old Interval-Based Tasks

Old code (still works):
```cpp
ScheduledTask task;
task.interval = std::chrono::minutes(5);
scheduler.registerTask(task);
```

New code (explicit):
```cpp
ScheduledTask task;
task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
task.interval = std::chrono::minutes(5);
scheduler.registerTask(task);
```

### Converting to Cron

Replace fixed intervals with cron expressions for more flexibility:

```cpp
// Old: Every 5 minutes
task.interval = std::chrono::minutes(5);

// New: Every 5 minutes (using cron)
task.trigger_type = ScheduledTask::TriggerType::CRON;
task.cron_expression = "*/5 * * * *";

// Benefit: Can now restrict to business hours
task.cron_expression = "*/5 9-17 * * 1-5";  // Only weekdays 9-5
```

## Task Persistence

Tasks with cron and CDC configurations are automatically persisted:

```cpp
TaskScheduler::Config config;
config.persist_tasks = true;
config.persistence_path = "data/tasks";

TaskScheduler scheduler(query_engine, config, changefeed);
```

Persisted fields include:
- Trigger type
- Cron expression
- CDC trigger configuration
- Priority
- Trigger logic

## Security Considerations

### Cron Injection Prevention

Cron expressions are validated and parsed safely:
- Field count validation
- Range validation for each field
- Syntax validation

### CDC Event Validation

Event trigger configurations are validated:
- Key prefix cannot be empty
- At least one event type must be specified
- Event types must be in valid range (0-3)

### Audit Logging

All trigger activations are logged:
- Task registration with trigger configuration
- Manual task executions
- CDC event triggers
- Cron-based executions

### Rate Limiting

Built-in rate limiting prevents abuse:
- Manual execution: max 10 per minute per task
- CDC debouncing: configurable per task
- Concurrent task limit: configurable globally

## Performance Impact

### Cron Scheduling
- CPU: Minimal (<0.1% overhead)
- Memory: ~100 bytes per task
- Latency: 1-second precision

### CDC Event Triggers
- CPU: ~0.5% overhead per active trigger
- Memory: ~1KB per trigger
- Latency: Sub-second event processing

### Recommendations
- Use debouncing for high-frequency CDC events
- Limit concurrent tasks to avoid resource contention
- Use appropriate priority levels for critical tasks

## Troubleshooting

### Cron Task Not Executing

1. Verify cron expression is valid:
   ```cpp
   auto validation = CronExpression::validate(expression);
   if (!validation.is_valid) {
       std::cout << validation.error_message << std::endl;
   }
   ```

2. Check task is enabled:
   ```cpp
   auto task = scheduler.getTask(task_id);
   if (!task->enabled) {
       scheduler.enableTask(task_id);
   }
   ```

3. Verify scheduler is running:
   ```cpp
   if (!scheduler.isRunning()) {
       scheduler.start();
   }
   ```

### CDC Trigger Not Firing

1. Verify changefeed is provided:
   ```cpp
   TaskScheduler scheduler(query_engine, config, changefeed.get());
   ```

2. Check event types match:
   ```cpp
   // Ensure event type is in trigger config
   task.cdc_trigger.event_types.insert(event_type);
   ```

3. Verify key prefix matches:
   ```cpp
   // Key must match prefix
   // Event key: "users:123"
   // Trigger prefix: "users:" ✓
   ```

### High CPU Usage

1. Reduce check interval:
   ```cpp
   config.check_interval = std::chrono::seconds(30);
   ```

2. Add CDC debouncing:
   ```cpp
   cdc_trigger.debounce_ms = 5000;
   ```

3. Limit concurrent tasks:
   ```cpp
   config.max_concurrent_tasks = 2;
   ```

## API Reference

See header files for complete API documentation:
- `include/utils/cron_parser.h` - Cron expression parsing
- `include/scheduler/event_trigger.h` - CDC event triggers
- `include/scheduler/task_scheduler.h` - Task scheduler

## Examples

Complete working examples can be found in:
- `examples/cron_and_cdc_scheduler_example.cpp`
- `tests/test_task_scheduler_triggers.cpp`
