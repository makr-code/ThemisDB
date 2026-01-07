# Scheduler Module

⚠️ **SECURITY WARNING**: This module allows arbitrary AQL query and function execution. See security documentation before deploying to production.

## Overview

The Scheduler module provides cron-like task scheduling capabilities for ThemisDB, enabling periodic execution of AQL queries and custom functions for post-processing operations.

## Components

- **TaskScheduler** - Core scheduling engine with configurable intervals
- **ScheduledTask** - Task definition with type, schedule, and statistics
- **TaskSchedulerApiHandler** - HTTP REST API for task management

## Key Features

- ✅ Cron-like scheduling with fixed intervals
- ✅ AQL query execution support
- ✅ Custom function registration and execution
- ✅ Concurrent task execution with resource limits
- ✅ Task persistence for recovery after restart
- ✅ Comprehensive statistics and monitoring
- ✅ Graceful shutdown with task completion

## Use Cases

1. **Periodic Data Compression** - Apply Gorilla compression to batched IoT data
2. **Data Aggregation** - Roll up high-frequency data to lower resolutions
3. **Data Cleanup** - Remove old raw data after aggregation
4. **Anomaly Detection** - Detect statistical anomalies in real-time data
5. **Maintenance Tasks** - Periodic database optimization and cleanup

## Documentation

- [Task Scheduler Guide (DE)](../../docs/de/scheduler/TASK_SCHEDULER.md) - Complete documentation in German
- [API Reference](../../include/scheduler/task_scheduler.h) - Header documentation
- [Unit Tests](../../tests/test_task_scheduler.cpp) - Usage examples

## Security

⚠️ **CRITICAL**: This module has significant security implications:

- Arbitrary code execution via AQL queries and functions
- Potential for SQL injection-like attacks
- Resource exhaustion risks (DoS)
- Data exfiltration possibilities
- Privilege escalation concerns

**Production deployments MUST implement:**
- Strong authentication and authorization (RBAC)
- Input validation and query sanitization
- Resource limits (CPU, memory, I/O)
- Comprehensive audit logging
- Encryption at rest for task definitions
- Network isolation and sandboxing

See [Security Documentation](../../docs/de/scheduler/TASK_SCHEDULER.md#sicherheitsrisiken) for details.

## Quick Start

```cpp
#include "scheduler/task_scheduler.h"

// Create scheduler
TaskScheduler scheduler(query_engine);

// Define a task
ScheduledTask task;
task.name = "Compress Old Data";
task.type = ScheduledTask::TaskType::AQL_QUERY;
task.aql_query = "FOR d IN timeseries FILTER d.old == true UPDATE d IN timeseries";
task.interval = std::chrono::minutes(10);

// Register and start
scheduler.registerTask(task);
scheduler.start();

// ... scheduler runs in background ...

scheduler.stop();
```

## Configuration

```cpp
TaskScheduler::Config config;
config.max_concurrent_tasks = 4;
config.check_interval = std::chrono::seconds(10);
config.persist_tasks = true;
config.persistence_path = "data/tasks";
config.allow_task_overlap = false;

TaskScheduler scheduler(query_engine, config);
```

## System Impact

| Aspect | Impact |
|--------|--------|
| CPU | 1-5% overhead for scheduler loop |
| Memory | ~1KB per task + execution memory |
| I/O | Depends on scheduled tasks |
| Write Path | No direct impact (async execution) |
| Concurrency | Configurable limits (default: 4 concurrent tasks) |

## Testing

Run unit tests:
```bash
./build/test_task_scheduler
```

## License

See main project LICENSE file.
