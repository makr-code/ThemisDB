# Module Integration Guide

## Overview

This guide explains how existing ThemisDB modules register their maintenance
capabilities with the `DatabaseMaintenanceOrchestrator`.

The orchestrator uses two integration points:

1. **Health Probes** – modules supply a lightweight status callback that is
   called synchronously when `GET /api/v1/maintenance/health` is requested.

2. **Task Execution** – the orchestrator calls module entry points inside
   `executeTask()`.  Adding a new task type requires a one-line addition there.

---

## 1. Registering a Health Probe

A `HealthProbe` is a `std::function<ModuleHealthSignal()>`.  Register it
**once** at server start-up, after constructing the orchestrator.

```cpp
#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_health_report.h"

// Example: Storage module health probe
orchestrator->registerHealthProbe(
    "storage",
    []() -> themis::maintenance::ModuleHealthSignal {
        ModuleHealthSignal sig;
        sig.module_name   = "storage";
        sig.checked_at_ms = nowMs();

        // Query whatever health indicator your module exposes
        if (rocksdb_wrapper->isHealthy()) {
            sig.status  = ModuleHealthStatus::OK;
            sig.message = "RocksDB healthy";
        } else {
            sig.status  = ModuleHealthStatus::CRITICAL;
            sig.message = "RocksDB not responding";
        }

        // Optionally add detail key-value pairs
        sig.details["compaction_pending_bytes"] =
            std::to_string(rocksdb_wrapper->getPendingCompactionBytes());

        return sig;
    }
);
```

### Rules for HealthProbe callbacks

- **Must be fast** – < 10 ms.  No blocking I/O; use cached / atomic values.
- **Must not throw** – uncaught exceptions are caught by the orchestrator and
  surfaced as `UNKNOWN` status with the exception message.
- **Must be thread-safe** – the probe may be called from any thread.

---

## 2. Adding a New Task Type

### Step 1 – Add the enum value

`include/maintenance/maintenance_task.h`:

```cpp
enum class MaintenanceTaskType {
    // ... existing values ...
    MY_NEW_TASK,   ///< Brief description
};
```

### Step 2 – Add string conversions

Same file, in `taskTypeToString` and `taskTypeFromString`:

```cpp
// taskTypeToString
case MaintenanceTaskType::MY_NEW_TASK: return "my_new_task";

// taskTypeFromString
if (s == "my_new_task") return MaintenanceTaskType::MY_NEW_TASK;
```

### Step 3 – Implement execution

`src/maintenance/database_maintenance_orchestrator.cpp`,
`executeTask()` switch:

```cpp
case MaintenanceTaskType::MY_NEW_TASK: {
    if (!my_module_) {
        job.state         = MaintenanceJobState::FAILED;
        job.error_message = "MyModule not available";
        return;
    }
    auto result = my_module_->runMyMaintenanceOperation();
    if (!result) {
        job.state         = MaintenanceJobState::FAILED;
        job.error_message = result.error();
    } else {
        job.state          = MaintenanceJobState::SUCCEEDED;
        job.result_summary = "My operation completed";
    }
    break;
}
```

### Step 4 – Expose the module dependency

Add the module pointer as a constructor parameter or via a setter on the
`DatabaseMaintenanceOrchestrator`, similar to how `index_maintenance_` is wired.

---

## 3. Using the Registry Helpers

`src/maintenance/maintenance_registry.cpp` provides factory functions for the
**default schedule bundles**:

```cpp
#include "maintenance/database_maintenance_orchestrator.h"

// Register all four default schedules + IndexMaintenance probe in one call:
themis::maintenance::registerDefaultMaintenanceSetup(
    *orchestrator,
    index_maintenance_shared_ptr);
```

All four default schedules are inserted in **disabled state** so operators
must explicitly enable them.

---

## 4. Example: Adding a Replica Consistency Module

```cpp
// in server start-up

// 1. Health probe
orchestrator->registerHealthProbe(
    "replica_consistency",
    [replica_mgr]() -> ModuleHealthSignal {
        ModuleHealthSignal s;
        s.module_name   = "replica_consistency";
        s.checked_at_ms = nowMs();
        auto lag = replica_mgr->getMaxLagMs();
        s.details["max_lag_ms"] = std::to_string(lag);
        if (lag > 5000) {
            s.status  = ModuleHealthStatus::CRITICAL;
            s.message = "Replica lag " + std::to_string(lag) + " ms exceeds threshold";
        } else if (lag > 1000) {
            s.status  = ModuleHealthStatus::DEGRADED;
            s.message = "Replica lag elevated: " + std::to_string(lag) + " ms";
        } else {
            s.status  = ModuleHealthStatus::OK;
            s.message = "Replica lag " + std::to_string(lag) + " ms";
        }
        return s;
    });

// 2. Create a custom schedule
MaintenanceScheduleEntry replica_sched;
replica_sched.name      = "Replica Consistency Check";
replica_sched.frequency = ScheduleFrequency::WEEKLY;
replica_sched.tasks     = {MaintenanceTaskType::REPLICA_VALIDATION};
replica_sched.enabled   = true;
orchestrator->createSchedule(replica_sched);
```
