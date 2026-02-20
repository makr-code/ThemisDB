# Sharding Module

Horizontal scaling and sharding implementation for ThemisDB v1.4+.

## Components

### Core Infrastructure
- Shard manager and topology
- Data distribution strategies
- Consistent hashing
- Shard rebalancing
- TrueTime integration for global consistency

### NEW in v1.4+ - Pluggable Consensus Architecture
- **Consensus Module Interface** - Abstract interface for pluggable consensus
- **Raft Consensus Adapter** - Adapter for existing Raft implementation
- **Gossip Consensus Adapter** - Adapter for Gossip protocol
- **Paxos Consensus** - New Multi-Paxos implementation
- **Consensus Factory** - Runtime consensus selection

### NEW in v1.4+ - Enhanced Transaction Support
- **Cross-Shard Transaction Coordinator** - Pluggable transaction protocols
  - Two-Phase Commit (2PC)
  - Three-Phase Commit (3PC)
  - SAGA (compensating transactions)
  - Percolator (optimistic concurrency)
- **Distributed Deadlock Detection**
- **Snapshot Isolation** across shards

### NEW in v1.4+ - Metadata Sharding
- **Metadata Shard** - Horizontally partitioned metadata
- **Metadata Shard Router** - Consistent hashing for metadata routing
- Partitioned by type: SCHEMA, INDEX, SHARD_MAP, TRANSACTION_LOG, etc.

### NEW in v1.5+ - Repair / Anti-Entropy Engine
- **ShardRepairEngine** (`include/sharding/shard_repair_engine.h`) – automated
  self-healing for Parity (RAID-5/6) and Mirror shard setups.
- **Improved Reed-Solomon Decoder** – Vandermonde matrix-based erasure recovery
  supporting up to `parity_shards` simultaneous chunk failures (previously
  limited to 1).

#### Key capabilities
| Capability | Details |
|---|---|
| Background scan | Configurable periodic anti-entropy scan across all shards |
| Auto-repair | Degraded documents detected during scan are queued for recovery |
| On-demand triggers | `triggerRepair(shard_id)`, `triggerFullScan()`, `triggerDocumentRepair(doc_id)` |
| Per-shard health | `ShardHealthReport` with status enum: `HEALTHY` / `DEGRADED` / `FAILED` / `REBUILDING` |
| Job tracking | Every trigger returns a `job_id`; `getJobStatus(job_id)` polls progress |
| Prometheus metrics | `exportPrometheusMetrics()` or via `ShardingMetricsHandler::getRepairMetrics()` |

#### Prometheus metrics exposed
| Metric | Type | Description |
|---|---|---|
| `themis_shard_repair_scans_total` | counter | Anti-entropy scans performed |
| `themis_shard_repair_attempts_total` | counter | Repair attempts |
| `themis_shard_repair_successes_total` | counter | Successful repairs |
| `themis_shard_repair_failures_total` | counter | Failed repair attempts |
| `themis_shard_repair_documents_scanned_total` | counter | Documents checked |
| `themis_shard_repair_avg_duration_ms` | gauge | Rolling average repair time (ms) |
| `themis_shard_health{shard="..."}` | gauge | Per-shard health (0–3) |
| `themis_shard_degraded_documents{shard="..."}` | gauge | Degraded document count |

#### Admin API endpoints
| Method | Path | Description |
|---|---|---|
| `POST` | `/admin/repair` | Trigger repair (body: `{"shard_id":"..."}` or `{}` for all) |
| `POST` | `/admin/repair/scan` | Trigger full anti-entropy scan |
| `GET`  | `/admin/repair/{job_id}` | Poll repair job status |

#### Quick start
```cpp
#include "sharding/shard_repair_engine.h"

themis::sharding::RepairConfig cfg;
cfg.scan_interval = std::chrono::seconds(300);   // scan every 5 min
cfg.enable_auto_repair = true;

auto engine = std::make_shared<themis::sharding::ShardRepairEngine>(
    cfg, strategy, ring, topology, read_handler, write_handler);

// Provide document list so the scanner knows what to check
engine->setDocumentListProvider([](const std::string& shard_id) {
    return myStorage.listDocuments(shard_id);
});

engine->start();

// On-demand repair
std::string job_id = engine->triggerRepair("shard_3");
auto status = engine->getJobStatus(job_id);

// Wire up to existing Prometheus scrape endpoint
metricsHandler->setRepairEngine(engine);
```

## Features

### Scalability
- Horizontal data partitioning
- Consistent hashing for shard assignment
- Dynamic shard rebalancing
- Metadata sharding prevents bottlenecks

### Consistency
- **Pluggable consensus algorithms** (Raft, Gossip, Paxos)
- **Multiple transaction protocols** (2PC, 3PC, SAGA, Percolator)
- **ACID guarantees across multiple shards**
- **TrueTime-based external consistency**
- **Snapshot isolation** for distributed reads

### Availability
- Automatic failover with hot spares
- Partition detection and split-brain prevention
- Deadlock detection and resolution
- Multi-datacenter support
- **Self-healing via ShardRepairEngine** (v1.5+)

## Implementation Status

### ✅ Completed (v1.4)
- Pluggable consensus module architecture
- Raft, Gossip, and Paxos consensus implementations
- Cross-shard transaction coordinator
- Transaction protocol abstraction (2PC, 3PC, SAGA, Percolator)
- Deadlock detection framework
- Metadata sharding design
- Comprehensive documentation

### ✅ Completed (v1.5)
- **ShardRepairEngine** – anti-entropy background scan + repair queue
- **Vandermonde-based Reed-Solomon decoder** – full multi-chunk recovery
- **Prometheus metrics integration** for repair health
- **Admin API repair endpoints** (POST /admin/repair, /admin/repair/scan, GET /admin/repair/{id})

### 🚧 Partial (requires integration)
- Full RPC integration for cross-shard operations
- Persistent state management for Paxos
- Complete metadata shard implementation
- Advanced query optimization

## Documentation

For comprehensive sharding documentation, see:
- **[Distributed Sharding Architecture](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)** - NEW v1.4!
- **[Consensus Module Architecture](../../docs/de/sharding/CONSENSUS_MODULE.md)** - NEW v1.4!
- **[Data Migration Guide](../../docs/de/migration/DATA_MIGRATION_COMPATIBILITY.md)** - NEW v1.4!
- [Distributed Transactions with 2PC](../../docs/DISTRIBUTED_TRANSACTIONS.md)
- [Sharding Implementation Summary](../../docs/SHARDING_IMPLEMENTATION_SUMMARY.md)
- [Sharding Phase 1 Report](../../docs/SHARDING_PHASE1_REPORT.md)
- [Sharding Phases 1-3 Summary](../../docs/SHARDING_PHASES_1-3_SUMMARY.md)
- [Horizontal Scaling Strategy](../../docs/horizontal_scaling_implementation_strategy.md)

## Quick Start

See usage examples in the architecture documentation above.
