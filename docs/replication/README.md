# Replication Documentation Index

Welcome to the ThemisDB replication documentation. This directory serves as the central hub for all replication and high-availability (HA) documentation.

## 📚 Core Documentation

### User Guides

- **[replication-ha-guide.md](../replication-ha-guide.md)** - **START HERE** ⭐
  - Complete guide to High Availability replication
  - Deployment topologies (Active-Passive, Active-Active, Multi-DC)
  - Configuration examples and best practices
  - Monitoring, alerting, and operational procedures
  - Performance tuning and troubleshooting

### Implementation Documentation

- **[REPLICATION_IMPLEMENTATION_STATUS.md](../reports/REPLICATION_IMPLEMENTATION_STATUS.md)** (German)
  - Detailed implementation status (~85% complete)
  - Component breakdown and file locations
  - Write-path flow diagrams
  - Build & test status
  - Prometheus metrics reference

- **[replication_raid_plan.md](../replication_raid_plan.md)**
  - RAID 1/10 replication readiness plan
  - Current findings and implementation steps
  - Acceptance criteria and next actions
  - Integration status

## 🏗️ Architecture Overview

### Module Organization

ThemisDB's replication system is organized across two main modules:

#### `replication/` Module (High-Level Orchestration)
- **Location**: `include/replication/`, `src/replication/`
- **Components**:
  - `ReplicationManager` - Lifecycle and configuration management
  - `MultiMasterReplicationManager` - Multi-master coordination (see `include/replication/multi_master_replication.h`)
- **Responsibility**: High-level replication strategies and orchestration

#### `sharding/` Module (Low-Level Infrastructure)
- **Location**: `include/sharding/`, `src/sharding/`
- **Components**:
  - `WALManager` - Write-Ahead Log with LSN tracking
  - `WALShipper` - Batch-based WAL shipping to replicas
  - `WALApplier` - Idempotent WAL application
  - `ReplicationCoordinator` - Write concern enforcement (ONE/MAJORITY/ALL)
  - `ReplicaTopology` - Shard-to-replica mapping (RAID 1/10/5/6)
  - Consensus modules (Raft, Gossip, Paxos)
  - `HealthMonitor` - Replica health tracking
- **Responsibility**: WAL-based replication mechanics, distributed consensus, topology management

**Design Rationale**: This separation allows the `replication/` module to focus on business logic while `sharding/` handles the complex distributed systems infrastructure needed for both replication and horizontal scaling.

### Key Features

✅ **Completed**:
- WAL-based replication with LSN tracking
- Write concern support (ONE/MAJORITY/ALL)
- RAID 1/10 topology support
- Idempotent WAL application
- Prometheus metrics integration
- HTTP and gRPC replication endpoints
- Leader election (Raft/Gossip/Paxos)
- Health monitoring and failure detection

🚧 **In Progress**:
- Multi-node endurance testing
- RAID 5/6 implementation
- Automatic failover enhancement

## 🔗 Related Documentation

### Sharding & Distribution
- [Distributed Sharding Architecture](../de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)
- [Consensus Module Architecture](../de/sharding/CONSENSUS_MODULE.md)
- [Distributed Transactions](../DISTRIBUTED_TRANSACTIONS.md)

### System Architecture
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - System overview
- [SECURITY.md](../../SECURITY.md) - Security configuration
- [MONITORING.md](../production/MONITORING.md) - Monitoring setup

### Operations
- [Disaster Recovery](../en/guides/disaster_recovery.md) - DR procedures

## 🚀 Quick Start

1. **Start here**: Read [replication-ha-guide.md](../replication-ha-guide.md) for a comprehensive overview
2. **Configuration**: Check the configuration examples in the HA guide
3. **Implementation details**: See [REPLICATION_IMPLEMENTATION_STATUS.md](../reports/REPLICATION_IMPLEMENTATION_STATUS.md) for component-level details
4. **Deployment planning**: Review [replication_raid_plan.md](../replication_raid_plan.md) for RAID configuration

## 📊 Implementation Status

**Overall Progress**: ~85% Complete

### Completed Components ✅
- WAL Manager, Shipper, Applier
- Replication Coordinator
- Replica Topology
- HTTP/gRPC endpoints
- Integration tests (8/8 passing)
- Prometheus metrics

### Remaining Work
- Multi-node endurance testing
- RAID 5/6 parity implementation
- Automatic failover enhancements

## 🤝 Contributing

When updating replication documentation:
1. Keep all three core documents in sync (HA guide, implementation status, RAID plan)
2. Update cross-references when adding new documentation
3. Follow the established structure for consistency
4. Update this index when adding new replication docs

## 📝 License

See [LICENSE](../../LICENSE) for licensing information.
