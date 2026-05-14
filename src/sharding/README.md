> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# sharding module

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../include/sharding/README.md · ./ARCHITECTURE.md · ./ROADMAP.md · ./FUTURE_ENHANCEMENTS.md -->

Status: active hardening and production-readiness work for distributed sharding.

## Current Implementation Layout

| Component | Implementation Location | Runtime Role |
|----------|--------------------------|--------------|
| Topology and routing core | `src/sharding/shard_router.cpp`, `src/sharding/shard_topology.cpp`, `src/sharding/consistent_hash.cpp` | Maps keys/queries to shards and keeps routing decisions aligned with topology updates |
| Consensus selection + adapters | `src/sharding/consensus_factory.cpp`, `src/sharding/raft_consensus_adapter.cpp`, `src/sharding/gossip_consensus_adapter.cpp` | Selects and starts Raft/Gossip/Paxos implementations at runtime |
| Consensus engines | `src/sharding/raft_consensus.cpp`, `src/sharding/gossip_protocol.cpp`, `src/sharding/paxos_consensus.cpp` | Coordinates distributed writes and membership decisions |
| Cross-shard transactions | `src/sharding/cross_shard_transaction.cpp`, `src/sharding/two_phase_commit_coordinator.cpp` | Executes 2PC/3PC/SAGA/Percolator-like flows for multi-shard operations |
| Repair + anti-entropy | `src/sharding/shard_repair_engine.cpp`, `src/sharding/redundancy_strategy.cpp` | Detects degraded shards, schedules repair jobs, and reconstructs lost shards/chunks |
| Rebalancing and migration | `src/sharding/auto_rebalancer.cpp`, `src/sharding/data_migrator.cpp`, `src/sharding/hardware_migration_manager.cpp` | Moves data and endpoints during scaling or hardware replacement |
| Operational APIs and metrics | `src/sharding/admin_api.cpp`, `src/sharding/operational_metrics.cpp`, `src/sharding/prometheus_metrics.cpp` | Exposes admin controls and observability endpoints for sharding state |

## Runtime Behavior, Error Cases, and Limits

### Routing + topology

- Hash-based and locality-aware routing are used to select target shards.
- Topology changes are eventually propagated; during transitions, requests can be re-routed or retried by higher layers.
- Invalid shard IDs, missing topology entries, or unresolved URNs return failure results instead of silently falling back.

### Consensus

- Runtime consensus choice is configured via `ConsensusFactory` (`Raft`, `Gossip`, `Paxos`).
- Quorum-based operation requires enough healthy peers; insufficient quorum causes proposals/commits to fail.
- WAL/persistence failures are treated as hard errors in consensus write paths.

### Cross-shard transactions

- The transaction coordinator supports 2PC, 3PC, SAGA, and Percolator-style paths.
- Prepare/commit/abort timeouts and deadlock detection windows are bounded by configuration.
- Participant unavailability and timeout conditions lead to abort/recovery paths.

### Repair and durability

- `ShardRepairEngine` can run periodic scans and on-demand repair jobs (`triggerRepair`, `triggerFullScan`, `triggerDocumentRepair`).
- Jobs expose status via `job_id` polling; failed jobs report explicit failure states.
- Erasure coding recovery can fail for irrecoverable failure sets (for example when missing shards exceed parity/recovery capability).

## Usage Snippets

### 1) Select and start a consensus module

```cpp
#include "sharding/consensus_factory.h"
#include "sharding/consensus_module.h"

themis::sharding::ConsensusConfig cfg;
cfg.type = themis::sharding::ConsensusType::RAFT;
cfg.node_id = "node-a";
cfg.cluster_nodes = {"node-a", "node-b", "node-c"};

auto consensus = themis::sharding::ConsensusFactory::create(cfg);
consensus->initialize(cfg.node_id, cfg.cluster_nodes);
consensus->start();
```

### 2) Run anti-entropy repair

```cpp
#include "sharding/shard_repair_engine.h"

themis::sharding::RepairConfig cfg;
cfg.enable_auto_repair = true;
cfg.scan_interval = std::chrono::seconds(300);

auto engine = std::make_shared<themis::sharding::ShardRepairEngine>(
    cfg, strategy, ring, topology, read_handler, write_handler);

engine->start();
const std::string job_id = engine->triggerRepair("shard-3");
auto status = engine->getJobStatus(job_id);
```

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Troubleshooting

- Consensus does not elect a leader or commit proposals: verify cluster size/quorum and peer reachability.
- Frequent cross-shard aborts: inspect participant timeouts and deadlock detection settings.
- Repair jobs stay in non-terminal states: check document-list providers and read/write handler wiring.
- Unexpected hotspotting on a subset of shards: inspect hash-ring and rebalancer telemetry before forcing migrations.

## Related Docs

- Public headers: [`../../include/sharding/README.md`](../../include/sharding/README.md)
- Architecture: [`./ARCHITECTURE.md`](./ARCHITECTURE.md)
- Security notes: [`./SECURITY.md`](./SECURITY.md)
- Audit log: [`./AUDIT.md`](./AUDIT.md)
- Module roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Module future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Distributed architecture overview: [`../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md`](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)
- Consensus details: [`../../docs/de/sharding/CONSENSUS_MODULE.md`](../../docs/de/sharding/CONSENSUS_MODULE.md)
- Quick start guide: [`../../docs/de/sharding/QUICK_START_GUIDE.md`](../../docs/de/sharding/QUICK_START_GUIDE.md)
- Shard repair deep dive: [`../../docs/de/sharding/SHARD_REPAIR_ENGINE.md`](../../docs/de/sharding/SHARD_REPAIR_ENGINE.md)
- Root roadmap: [`../../ROADMAP.md`](../../ROADMAP.md)
- Root future enhancements: [`../../FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md)
