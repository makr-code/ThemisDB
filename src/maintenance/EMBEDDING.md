---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Embedding and Vector-Reindex Boundary

## Purpose and scope

This page prevents a naming ambiguity for developers integrating vector
maintenance. The maintenance module coordinates scheduled tasks; it does not
generate text or image embeddings, select an embedding model, or own vector
index storage.

## What the source implements

`MaintenanceTaskType::VECTOR_REINDEX` is an operator-visible task token, whose
enum description calls it incremental HNSW re-indexing. In
`DatabaseMaintenanceOrchestrator::executeTask`, this task requires an
`IndexMaintenanceManager` and calls `triggerMaintenanceCheck()`. The
orchestrator reports a trigger result; actual index work and its embedding
semantics belong to the index/embedding subsystem.

The maintenance schedule contract therefore has no embedding payload, model
identifier, vector dimension, normalization rule, chunking policy, or embedding
quality metric. Do not add those fields to maintenance schedules to describe a
model lifecycle. Add or change the index/embedding API in its owning module and
keep maintenance as a typed dispatch/integration point.

## Integration and failure behavior

- **Operator guidance (not enforced by schedule validation):** enable a schedule
  containing `VECTOR_REINDEX` only after startup has wired the relevant
  index-maintenance capability.
- If `IndexMaintenanceManager` is unavailable, the task reports failure.
- If `triggerMaintenanceCheck()` returns an error, the task reports that error
  through the job result.
- If the delegated operation succeeds, the maintenance result says that vector
  re-index was triggered; this is not evidence that embeddings were regenerated
  or that a vector quality target was met.
- A schedule may contain other tasks before or after re-indexing. Use
  `task_dependencies` when order is required; completed earlier tasks are not
  rolled back if re-indexing fails.

## Evaluation ownership

Evaluate embedding quality, model/version compatibility, vector dimensions,
recall, and index freshness in the owning embedding and index modules. In this
module, test only the coordination contract: task serialization, ordering,
missing-manager/error propagation, handler registration where applicable, and
operator-visible dispatch/job outcomes.

## Sources

- [`maintenance_task.h`](../../include/maintenance/maintenance_task.h)
- [`database_maintenance_orchestrator.cpp`](database_maintenance_orchestrator.cpp)
- [`maintenance_schedule.h`](../../include/maintenance/maintenance_schedule.h)
- [Maintenance architecture](ARCHITECTURE.md)
- [Maintenance API](API.md)
- [Maintenance research/evaluation notes](RESEARCH.md)
