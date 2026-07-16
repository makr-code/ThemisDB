### Context

This issue implements the roadmap item 'Distributed Ingestion Coordinator' for the ingestion domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Distributed Ingestion Coordinator

### Goal

Deliver the scoped changes for Distributed Ingestion Coordinator in src/ingestion/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Distributed Ingestion Coordinator
**Priority:** Medium
**Target Version:** v1.8.0

Enable `IngestionManager` to distribute source processing across multiple ThemisDB nodes so that large ingestion jobs are not bottlenecked on a single instance. The coordinator partitions sources and page ranges across worker nodes and aggregates `IngestionReport` results.

**Implementation Notes:**
- Add `IngestionCoordinator` class in `ingestion_coordinator.cpp`; acts as the leader that partitions work via consistent hashing of `source_id` across available worker nodes.
- Workers receive their assigned sources via a gRPC `IngestRequest` (new proto definition in `proto/ingestion_coordinator.proto`); they run the existing `IngestionManager::ingestAll()` locally and stream progress events back to the coordinator.
- `IngestionCheckpointStore` must switch to a shared backend (Redis or the ThemisDB checkpoint collection) so that all workers see the same incremental progress state.
- Leader election uses a lightweight lease mechanism (TTL-based lock in the checkpoint collection) to avoid split-brain during coordinator failover.

**Performance Targets:**
- Linear throughput scaling to at least 4 worker nodes (≥ 3.5× aggregate throughput vs single node) for API and filesystem sources.
- Coordinator overhead (partitioning + progress aggregation) ≤ 5 % of total ingestion wall-clock time.

---

### Acceptance Criteria

- [x] Add `IngestionCoordinator` class in `ingestion_coordinator.cpp`; acts as the leader that partitions work via consistent hashing of `source_id` across available worker nodes.
- [x] Workers receive their assigned sources via a gRPC `IngestRequest` (new proto definition in `proto/ingestion_coordinator.proto`); they run the existing `IngestionManager::ingestAll()` locally and stream progress events back to the coordinator.
- [x] `IngestionCheckpointStore` must switch to a shared backend (Redis or the ThemisDB checkpoint collection) so that all workers see the same incremental progress state.
- [x] Leader election uses a lightweight lease mechanism (TTL-based lock in the checkpoint collection) to avoid split-brain during coordinator failover.
- [x] Linear throughput scaling to at least 4 worker nodes (≥ 3.5× aggregate throughput vs single node) for API and filesystem sources.
- [x] Coordinator overhead (partitioning + progress aggregation) ≤ 5 % of total ingestion wall-clock time.

### Relationships

- Roadmap row: #179 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/ingestion/FUTURE_ENHANCEMENTS.md#distributed-ingestion-coordinator
- Source key: roadmap:179:ingestion:v1.8.0:distributed-ingestion-coordinator

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:179:ingestion:v1.8.0:distributed-ingestion-coordinator -->
<!-- roadmap-ref: row=179;module=ingestion;target=v1.8.0 -->
<!-- roadmap-detail: src/ingestion/FUTURE_ENHANCEMENTS.md#distributed-ingestion-coordinator -->
