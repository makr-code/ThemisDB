> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->

# Maintenance Module — Public Headers

Scheduled database maintenance orchestration for ThemisDB: compaction, replica validation, MVCC cleanup, health reporting, RocksDB persistence, DAG dependency graph, distributed lock coordination, and multi-tenant isolation.

## Header Listing

| Header | Purpose |
|--------|---------|
| `database_maintenance_orchestrator.h` | Central maintenance coordinator; `TenantMaintenanceConfig` |
| `i_distributed_lock.h` | Distributed lock interface; `InProcessDistributedLock` |
| `i_maintenance_task_handler.h` | Task handler base interface |
| `maintenance_task_handler_impls.h` | Built-in handlers (compaction, replica, MVCC, function) |
| `maintenance_task.h` | Task and dependency types |
| `maintenance_schedule.h` | Cron-based schedule entry with DAG support |
| `maintenance_schedule_store.h` | RocksDB-backed schedule CRUD |
| `maintenance_health_report.h` | Aggregated health reporting |

## Links

- Implementation: `../../src/maintenance/`
- ARCHITECTURE.md · AUDIT.md · CHANGELOG.md · ROADMAP.md · SECURITY.md · FUTURE_ENHANCEMENTS.md

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "maintenance/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
