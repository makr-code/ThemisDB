# ThemisDB RAID Shard Referencing Architecture

## Document Purpose

This document provides a comprehensive technical overview of ThemisDB's RAID sharding architecture, shard referencing mechanisms, and cluster coordination protocols.

---

## Table of Contents

1. [Overview](#overview)
2. [Shard Addressing and Discovery](#shard-addressing-and-discovery)
3. [RAID Modes and Data Distribution](#raid-modes-and-data-distribution)
4. [Shard Communication Protocols](#shard-communication-protocols)
5. [Consistency and Coordination](#consistency-and-coordination)
6. [Failure Detection and Recovery](#failure-detection-and-recovery)
7. [Performance Considerations](#performance-considerations)
8. [Configuration Reference](#configuration-reference)

---

## Overview

### What is Shard Referencing?

Shard referencing is the mechanism by which ThemisDB nodes discover, communicate with, and coordinate operations across multiple database shards in a RAID cluster.

### Key Concepts

**Shard**: An independent database node that stores a subset of the total dataset
**RAID Group**: A collection of shards operating under a specific RAID mode
**Shard ID**: Unique identifier for each shard within a cluster
**Peer Discovery**: Process by which shards find and connect to each other
**Routing**: Mechanism to determine which shard(s) handle a given operation

---

## Shard Addressing and Discovery

### Shard Identification Schema

Each shard has a unique identifier composed of:

```
<raid_mode>-<sequence_number>
```

**Examples:**
- `raid0-1` - First shard in RAID0 stripe group
- `raid1-primary` - Primary node in RAID1 mirror group
- `raid5-3` - Third shard in RAID5 parity group

### Network Addressing

Shards communicate using two address formats:

#### 1. Docker Internal Addressing
```
themis-<raid_group>-<shard_name>:<port>
```

**Examples:**
```
themis-raid0-shard1:18765    # RAID0 first shard
themis-raid1-primary:18765   # RAID1 primary node
themis-raid5-shard2:18765    # RAID5 second shard
```

#### 2. External Host Addressing
```
<host_ip>:<external_port>
```

**Examples:**
```
localhost:18765    # External access to RAID0 shard1
localhost:18768    # External access to RAID1 primary
192.168.1.10:8080  # External REST API access
```

### Discovery Mechanisms

#### Static Configuration (Current Implementation)

Shards are configured with explicit peer lists via environment variables:

```yaml
environment:
  THEMIS_SHARDS: "themis-raid0-shard1:18765,themis-raid0-shard2:18765,themis-raid0-shard3:18765"
  THEMIS_MIRROR_PEER: "themis-raid1-secondary:18765"
```

**Advantages:**
- Simple and predictable
- No additional service dependencies
- Works well for stable clusters

**Disadvantages:**
- Requires manual configuration
- Difficult to scale dynamically
- No automatic discovery

#### Future: Dynamic Discovery (Roadmap)

Potential implementation using service discovery:

```yaml
# Using Consul/etcd/ZooKeeper
discovery:
  service: "consul"
  key_prefix: "/themis/shards/"
  health_check_interval: 10s
```

---

## RAID Modes and Data Distribution

### RAID 0 (Striping)

**Purpose:** Maximum performance through data distribution

**Architecture:**
```
┌─────────────────────────────────────────────┐
│         Client Write Request                │
│              (100 KB data)                   │
└────────────────┬────────────────────────────┘
                 │
                 ▼
         ┌───────────────┐
         │  Hash(key) % n │  n = number of shards
         └───────┬───────┘
                 │
        ┌────────┼────────┐
        ▼        ▼        ▼
    ┌──────┐ ┌──────┐ ┌──────┐
    │Shard1│ │Shard2│ │Shard3│
    │ 33KB │ │ 33KB │ │ 33KB │
    └──────┘ └──────┘ └──────┘
```

**Shard Selection Algorithm:**
```cpp
uint32_t selectShard(const std::string& key, size_t num_shards) {
    uint32_t hash = murmurHash3(key);
    return hash % num_shards;
}
```

**Data Placement:**
- Records distributed by key hash
- Each shard stores ~1/n of total data
- No data redundancy

**Read Operation:**
```cpp
// Client requests key "user:12345"
// 1. Calculate target shard
uint32_t target = selectShard("user:12345", 3);  // Returns 1 (shard2)

// 2. Route to specific shard
auto result = shards[target]->get("user:12345");

// 3. Return result directly
return result;
```

**Write Operation:**
```cpp
// Client writes key "user:12345" with value
// 1. Calculate target shard
uint32_t target = selectShard("user:12345", 3);

// 2. Write to single shard
shards[target]->put("user:12345", value);

// 3. No replication needed
```

**Failover Behavior:**
- ⚠️ Data loss if any shard fails
- No automatic recovery
- Requires manual intervention

---

### RAID 1 (Mirroring)

**Purpose:** High availability and redundancy

**Architecture:**
```
┌─────────────────────────────────────────────┐
│         Client Write Request                │
│              (100 KB data)                   │
└────────────────┬────────────────────────────┘
                 │
                 ▼
         ┌───────────────┐
         │  Primary Node  │
         └───────┬───────┘
                 │
        ┌────────┴────────┐
        │ Synchronous     │
        │ Replication     │
        ▼                 ▼
    ┌──────┐         ┌──────┐
    │Primary│────────►│Secondary│
    │100KB │         │100KB │
    └──────┘         └──────┘
```

**Shard Configuration:**
```yaml
# Primary Node
environment:
  THEMIS_SHARD_ID: "raid1-primary"
  THEMIS_RAID_MODE: "mirror"
  THEMIS_RAID_GROUP: "raid1"
  THEMIS_MIRROR_PEER: "themis-raid1-secondary:18765"
  
# Secondary Node
environment:
  THEMIS_SHARD_ID: "raid1-secondary"
  THEMIS_RAID_MODE: "mirror"
  THEMIS_RAID_GROUP: "raid1"
  THEMIS_MIRROR_PEER: "themis-raid1-primary:18765"
```

**Data Placement:**
- Complete data copy on both nodes
- Storage efficiency: 50% (n/2)
- Full redundancy

**Write Operation (Synchronous):**
```cpp
bool mirrorWrite(const std::string& key, const std::string& value) {
    // Phase 1: Write to primary
    bool primary_ok = local_storage->put(key, value);
    if (!primary_ok) return false;
    
    // Phase 2: Replicate to secondary (synchronous)
    bool secondary_ok = mirror_peer->replicate(key, value);
    
    // Phase 3: Wait for acknowledgment
    if (!secondary_ok) {
        // Rollback or mark as inconsistent
        handleReplicationFailure(key);
        return false;
    }
    
    return true;
}
```

**Read Operation (Load Balanced):**
```cpp
std::string mirrorRead(const std::string& key) {
    // Strategy 1: Always read from primary
    return primary->get(key);
    
    // Strategy 2: Load balance between primary and secondary
    if (random() % 2 == 0) {
        return primary->get(key);
    } else {
        return secondary->get(key);
    }
}
```

**Failover Behavior:**
```cpp
void handlePrimaryFailure() {
    // 1. Detect primary failure
    if (!primary->isHealthy()) {
        // 2. Promote secondary to primary
        secondary->promoteToPrimary();
        
        // 3. Update routing
        updatePrimaryReference(secondary);
        
        // 4. Continue operations
        LOG(INFO) << "Failover complete: Secondary promoted to primary";
    }
}
```

---

### RAID 5 (Striping + Parity)

**Purpose:** Balance between performance and redundancy

**Architecture:**
```
┌─────────────────────────────────────────────┐
│         Client Write Request                │
│              (90 KB data)                    │
└────────────────┬────────────────────────────┘
                 │
                 ▼
         ┌───────────────┐
         │ Stripe + Parity│
         └───────┬───────┘
                 │
        ┌────────┼────────┐
        ▼        ▼        ▼
    ┌──────┐ ┌──────┐ ┌──────┐
    │Shard1│ │Shard2│ │Shard3│
    │ 30KB │ │ 30KB │ │ 30KB │
    │ Data │ │ Data │ │Parity│
    └──────┘ └──────┘ └──────┘
```

**Parity Calculation:**
```cpp
std::vector<uint8_t> calculateParity(
    const std::vector<std::vector<uint8_t>>& data_blocks
) {
    // XOR all data blocks to create parity
    std::vector<uint8_t> parity(data_blocks[0].size(), 0);
    
    for (const auto& block : data_blocks) {
        for (size_t i = 0; i < block.size(); i++) {
            parity[i] ^= block[i];
        }
    }
    
    return parity;
}
```

**Write Operation:**
```cpp
bool raid5Write(const std::string& key, const std::string& value) {
    size_t num_data_shards = shards.size() - 1;  // Last shard is parity
    
    // 1. Split data into stripes
    auto stripes = splitIntoStripes(value, num_data_shards);
    
    // 2. Calculate parity stripe
    auto parity = calculateParity(stripes);
    
    // 3. Write data stripes to shards
    for (size_t i = 0; i < stripes.size(); i++) {
        shards[i]->put(key + ":stripe:" + std::to_string(i), stripes[i]);
    }
    
    // 4. Write parity to last shard
    shards[num_data_shards]->put(key + ":parity", parity);
    
    return true;
}
```

**Read Operation (Normal):**
```cpp
std::string raid5Read(const std::string& key) {
    size_t num_data_shards = shards.size() - 1;
    
    // 1. Read all data stripes
    std::vector<std::string> stripes;
    for (size_t i = 0; i < num_data_shards; i++) {
        auto stripe = shards[i]->get(key + ":stripe:" + std::to_string(i));
        stripes.push_back(stripe);
    }
    
    // 2. Reconstruct data (no parity needed if all shards healthy)
    return concatenateStripes(stripes);
}
```

**Read Operation (Degraded - One Shard Failed):**
```cpp
std::string raid5ReadDegraded(const std::string& key, size_t failed_shard) {
    size_t num_data_shards = shards.size() - 1;
    
    // 1. Read available data stripes
    std::vector<std::string> stripes;
    for (size_t i = 0; i < num_data_shards; i++) {
        if (i == failed_shard) continue;
        auto stripe = shards[i]->get(key + ":stripe:" + std::to_string(i));
        stripes.push_back(stripe);
    }
    
    // 2. Read parity
    auto parity = shards[num_data_shards]->get(key + ":parity");
    
    // 3. Reconstruct missing stripe using XOR
    auto missing_stripe = reconstructStripe(stripes, parity, failed_shard);
    
    // 4. Insert missing stripe at correct position
    stripes.insert(stripes.begin() + failed_shard, missing_stripe);
    
    // 5. Concatenate all stripes
    return concatenateStripes(stripes);
}
```

**Parity Distribution Strategy:**

Traditional RAID5 rotates parity across shards to avoid hotspots:

```
Stripe 0: | Data1 | Data2 | Parity |
Stripe 1: | Data1 | Parity | Data2 |
Stripe 2: | Parity | Data1 | Data2 |
Stripe 3: | Data1 | Data2 | Parity |  (repeat)
```

```cpp
size_t getParityShard(size_t stripe_id, size_t num_shards) {
    return stripe_id % num_shards;
}
```

---

## Shard Communication Protocols

### Wire Protocol (Port 18765)

ThemisDB uses a custom binary wire protocol for inter-shard communication.

**Message Format:**
```
+--------+--------+----------+-------------+
| Magic  | OpCode | Length   | Payload     |
| 4 bytes| 2 bytes| 4 bytes  | N bytes     |
+--------+--------+----------+-------------+
```

**Operation Codes:**
```cpp
enum class OpCode : uint16_t {
    GET = 0x0001,           // Read request
    PUT = 0x0002,           // Write request
    DELETE = 0x0003,        // Delete request
    REPLICATE = 0x0004,     // Replication message
    HEALTH_CHECK = 0x0005,  // Health probe
    PEER_DISCOVERY = 0x0006,// Shard discovery
    FAILOVER = 0x0007,      // Failover notification
    SYNC_REQUEST = 0x0008,  // Sync data request
    SYNC_RESPONSE = 0x0009, // Sync data response
};
```

**Example GET Operation:**
```cpp
struct GetRequest {
    std::string shard_id;
    std::string key;
    uint64_t timestamp;
};

// Wire format
// Magic: 0x54484D53 ("THMS")
// OpCode: 0x0001 (GET)
// Length: sizeof(GetRequest)
// Payload: serialized GetRequest
```

### REST API (Port 8080)

HTTP-based API for external clients and metrics.

**Endpoints:**

```
GET  /health                    # Health check
GET  /metrics                   # Prometheus metrics
GET  /api/v1/keys/:key          # Read key
PUT  /api/v1/keys/:key          # Write key
DELETE /api/v1/keys/:key        # Delete key
GET  /api/v1/cluster/status     # Cluster status
GET  /api/v1/shards             # List shards
```

**Example Metrics Endpoint Response:**
```
# HELP themis_raid_io_bytes_total Total RAID I/O bytes
# TYPE themis_raid_io_bytes_total counter
themis_raid_io_bytes_total{shard_id="raid0-1",operation="write"} 1234567890

# HELP themis_operation_duration_seconds Operation duration
# TYPE themis_operation_duration_seconds histogram
themis_operation_duration_seconds_bucket{operation="get",le="0.001"} 1523
themis_operation_duration_seconds_bucket{operation="get",le="0.01"} 2456
themis_operation_duration_seconds_bucket{operation="get",le="0.1"} 2498
themis_operation_duration_seconds_bucket{operation="get",le="+Inf"} 2500

# HELP themis_shard_health_status Shard health (1=healthy, 0=unhealthy)
# TYPE themis_shard_health_status gauge
themis_shard_health_status{shard_id="raid0-1"} 1
themis_shard_health_status{shard_id="raid0-2"} 1
themis_shard_health_status{shard_id="raid0-3"} 0
```

---

## Consistency and Coordination

### Consistency Models

#### RAID 0 (Eventual Consistency)
- No replication, no consistency concerns
- Single copy of data per key

#### RAID 1 (Strong Consistency)

Writes across RAID-1 mirrors use the ThemisDB Two-Phase Commit (2PC) protocol.
Each Raft-leader shard runs a `TwoPhaseCommitParticipant`; the `DistributedTransactionCoordinator`
orchestrates PREPARE and COMMIT across all participants atomically.

```cpp
#include "sharding/two_phase_commit_participant.h"
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"

// ── Shard-side setup (runs once per Raft-group leader) ───────────────────────

TwoPhaseCommitParticipant::Config pcfg;
pcfg.prepare_timeout_ms = 10000;
pcfg.sync_wal_writes    = true;   // Durability

// Callbacks wired to the storage engine
auto participant = std::make_shared<TwoPhaseCommitParticipant>(
    "raid1-primary", pcfg,
    // validate & acquire row locks in PREPARE phase
    [&storage](const std::string& txn_id, const nlohmann::json& ops) {
        return storage.validateAndLock(txn_id, ops);
    },
    // apply operations in COMMIT phase
    [&storage](const std::string& txn_id, const nlohmann::json& ops, int64_t ts) {
        return storage.applyWithTimestamp(txn_id, ops, ts);
    },
    // release locks after COMMIT or ABORT
    [&storage](const std::string& txn_id) {
        storage.releaseLocks(txn_id);
    }
);
rpc_server.setRequestHandler(participant.get()); // attach to RPC listener

// ── Coordinator-side setup (runs on the client / routing tier) ───────────────

auto truetime    = std::make_shared<TrueTime>();
auto coordinator = std::make_shared<DistributedTransactionCoordinator>(truetime);

// Begin a transaction across the two RAID-1 mirrors
std::string txn_id = coordinator->beginTransaction({"raid1-primary", "raid1-secondary"});

// Stage per-shard write operations
coordinator->addOperation(txn_id, "raid1-primary",   {{"type","update"},{"key","k1"},{"value","v1"}});
coordinator->addOperation(txn_id, "raid1-secondary", {{"type","update"},{"key","k1"},{"value","v1"}});

// commit() runs Phase 1 (PREPARE) then Phase 2 (COMMIT or ABORT) atomically
bool ok = coordinator->commit(txn_id);
// ok == true  → both mirrors have applied the write; WAL flushed on both
// ok == false → one mirror rejected; coordinator sent ABORT to both (no change)
```

For crash-recovery semantics, in-doubt transactions (PREPARE logged but no COMMIT/ABORT)
are automatically resolved by the coordinator's `recoverTransactions()` on restart.
See [`docs/DISTRIBUTED_TRANSACTIONS.md`](../../DISTRIBUTED_TRANSACTIONS.md) for
the full protocol description, REST API reference (`/dtxn/*`), and configuration options.

#### RAID 5 (Read-Your-Writes Consistency)
- Writes complete only after all stripes (including parity) are written
- Reads may serve stale data during rebuild

### Conflict Resolution

**Last-Write-Wins (LWW):**
```cpp
struct VersionedValue {
    std::string value;
    uint64_t timestamp;
    std::string writer_id;
};

bool isNewer(const VersionedValue& a, const VersionedValue& b) {
    if (a.timestamp != b.timestamp) {
        return a.timestamp > b.timestamp;
    }
    // Tie-breaker: use writer ID
    return a.writer_id > b.writer_id;
}
```

**Vector Clocks (Advanced):**
```cpp
class VectorClock {
    std::map<std::string, uint64_t> clocks_;
    
public:
    void increment(const std::string& shard_id) {
        clocks_[shard_id]++;
    }
    
    Ordering compare(const VectorClock& other) const {
        // Returns: BEFORE, AFTER, CONCURRENT
        // Implementation details...
    }
};
```

---

## Failure Detection and Recovery

### Health Monitoring

**Heartbeat Protocol:**
```cpp
class HealthMonitor {
    std::chrono::seconds heartbeat_interval_{10};
    std::chrono::seconds failure_threshold_{30};
    
    void monitorShards() {
        while (running_) {
            for (auto& shard : shards_) {
                if (!sendHeartbeat(shard)) {
                    auto last_seen = shard.last_heartbeat_time;
                    auto elapsed = now() - last_seen;
                    
                    if (elapsed > failure_threshold_) {
                        handleShardFailure(shard);
                    }
                }
            }
            std::this_thread::sleep_for(heartbeat_interval_);
        }
    }
};
```

### Failure Scenarios

#### RAID 0 Shard Failure
```
Before:  [S1] [S2] [S3]
After:   [S1] [--] [S3]
Impact:  33% data loss, cluster non-functional
Action:  Manual intervention required
```

#### RAID 1 Primary Failure
```
Before:  [Primary] ←→ [Secondary]
After:   [-------]     [Secondary (promoted)]
Impact:  No data loss, brief service interruption
Action:  Automatic failover, secondary becomes primary
```

#### RAID 5 Single Shard Failure
```
Before:  [S1:Data] [S2:Data] [S3:Parity]
After:   [S1:Data] [-------] [S3:Parity]
Impact:  Degraded performance, no data loss
Action:  Reconstruct S2 from S1 XOR S3
```

### Recovery Process

**RAID 5 Rebuild:**
```cpp
class RAID5Rebuilder {
public:
    void rebuildShard(size_t failed_shard_idx) {
        LOG(INFO) << "Starting rebuild of shard " << failed_shard_idx;
        
        // 1. List all keys in remaining shards
        auto keys = gatherAllKeys(failed_shard_idx);
        
        // 2. For each key, reconstruct missing data
        for (const auto& key : keys) {
            auto reconstructed = reconstructKey(key, failed_shard_idx);
            
            // 3. Write to replacement shard
            replacement_shard_->put(key, reconstructed);
        }
        
        LOG(INFO) << "Rebuild complete. Processed " << keys.size() << " keys";
    }
    
private:
    std::string reconstructKey(const std::string& key, size_t failed_idx) {
        // Read all stripes except failed one
        std::vector<std::string> stripes;
        for (size_t i = 0; i < shards_.size() - 1; i++) {
            if (i == failed_idx) continue;
            stripes.push_back(shards_[i]->get(key + ":stripe:" + std::to_string(i)));
        }
        
        // Read parity
        auto parity = shards_.back()->get(key + ":parity");
        
        // XOR to reconstruct
        return xorReconstruct(stripes, parity, failed_idx);
    }
};
```

---

## Performance Considerations

### Read Performance

| RAID Mode | Single Key Read | Range Scan | Full Table Scan |
|-----------|----------------|------------|-----------------|
| RAID 0    | 1x (single shard) | Nx parallel | Nx parallel |
| RAID 1    | 2x (load balanced) | 2x parallel | 2x parallel |
| RAID 5    | 1x (single shard) | (N-1)x parallel | (N-1)x parallel |

### Write Performance

| RAID Mode | Single Write | Batch Write | Overhead |
|-----------|-------------|-------------|----------|
| RAID 0    | 1x          | Nx parallel | 0% |
| RAID 1    | 0.5x        | 0.5x        | 100% (replication) |
| RAID 5    | 0.7x        | 0.8x        | ~40% (parity) |

### Network Traffic

**RAID 0:**
- Minimal inter-shard traffic
- Only coordination messages

**RAID 1:**
- High replication traffic (2x writes)
- Heartbeat messages

**RAID 5:**
- Moderate traffic for parity distribution
- Higher traffic during rebuild

### Latency Impact

```
Average Latency = Base Latency + Coordination Overhead

RAID 0: Base + 1ms  (minimal coordination)
RAID 1: Base + 10ms (synchronous replication)
RAID 5: Base + 5ms  (parity calculation)
```

---

## Configuration Reference

### Environment Variables

```bash
# Shard Identification
THEMIS_SHARD_ID="raid0-1"              # Unique shard identifier
THEMIS_ROLE="shard"                    # Node role: shard|coordinator|proxy

# RAID Configuration
THEMIS_RAID_MODE="stripe"              # stripe|mirror|parity
THEMIS_RAID_GROUP="raid0"              # RAID group name
THEMIS_SHARDS="host1:port,host2:port"  # Peer shard list
THEMIS_MIRROR_PEER="host:port"         # Mirror peer (RAID1 only)

# Network Configuration
THEMIS_PORT="18765"                    # Wire protocol port
THEMIS_API_PORT="8080"                 # REST API port
THEMIS_BIND_ADDRESS="0.0.0.0"          # Bind address

# Performance Tuning
THEMIS_IO_THREADS="4"                  # I/O thread count
THEMIS_NETWORK_BUFFER_SIZE="134217728" # 128 MB
THEMIS_REPLICATION_TIMEOUT="30"        # Seconds

# Monitoring
THEMIS_ENABLE_METRICS="true"           # Enable Prometheus metrics
THEMIS_METRICS_PORT="9090"             # Metrics port
THEMIS_LOG_LEVEL="INFO"                # DEBUG|INFO|WARN|ERROR

# Storage
THEMIS_DATA_DIR="/var/lib/themisdb"    # Data directory
THEMIS_CACHE_SIZE="2147483648"         # 2 GB cache
```

### Docker Compose Example

```yaml
version: "3.8"

services:
  raid0-shard1:
    image: themisdb/themisdb:metrics-enabled
    container_name: themis-raid0-shard1
    environment:
      THEMIS_SHARD_ID: "raid0-1"
      THEMIS_RAID_MODE: "stripe"
      THEMIS_RAID_GROUP: "raid0"
      THEMIS_SHARDS: "themis-raid0-shard1:18765,themis-raid0-shard2:18765,themis-raid0-shard3:18765"
      THEMIS_PORT: "18765"
      THEMIS_ENABLE_METRICS: "true"
    ports:
      - "18765:18765"
      - "8080:8080"
    volumes:
      - raid0_shard1_data:/var/lib/themisdb
    networks:
      - themis-network
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 10s
      timeout: 5s
      retries: 3

volumes:
  raid0_shard1_data:
    driver: local

networks:
  themis-network:
    driver: bridge
```

---

## Troubleshooting Guide

### Shard Cannot Discover Peers

**Symptom:** Shard logs show "Unable to connect to peer"

**Diagnosis:**
```bash
# Check network connectivity
docker exec themis-raid0-shard1 ping themis-raid0-shard2

# Verify DNS resolution
docker exec themis-raid0-shard1 nslookup themis-raid0-shard2

# Check if peer is listening
docker exec themis-raid0-shard2 netstat -ln | grep 18765
```

**Solution:**
- Verify all containers are in same Docker network
- Check firewall rules
- Ensure `THEMIS_SHARDS` configuration is correct

### High Replication Lag (RAID1)

**Symptom:** Secondary significantly behind primary

**Diagnosis:**
```bash
# Check replication metrics
curl http://localhost:8083/metrics | grep replication_lag

# Monitor network bandwidth
docker stats themis-raid1-primary themis-raid1-secondary
```

**Solution:**
- Increase `THEMIS_NETWORK_BUFFER_SIZE`
- Add more network bandwidth
- Consider async replication for non-critical data

### Slow RAID5 Reads After Failure

**Symptom:** Read latency increases significantly after shard failure

**Explanation:** Degraded mode requires XOR reconstruction

**Solution:**
- Rebuild failed shard as soon as possible
- Consider RAID6 (dual parity) for better degraded performance
- Add read caching layer

---

## References

### Internal Documentation
- [DISTRIBUTED_TRANSACTIONS.md](../../DISTRIBUTED_TRANSACTIONS.md) – 2PC protocol, REST API reference, configuration
- [DOCKER_RAID_IMPLEMENTATION_SUMMARY.md](../benchmarks/DOCKER_RAID_IMPLEMENTATION_SUMMARY.md)
- [RAID_SHARDING_QUICKSTART.md](../benchmarks/RAID_SHARDING_QUICKSTART.md)
- [PROMETHEUS_INTEGRATION_COMPLETE.md](../PROMETHEUS_INTEGRATION_COMPLETE.md)

### External Resources
- [RAID Concepts (Wikipedia)](https://en.wikipedia.org/wiki/RAID)
- [Consistent Hashing](https://en.wikipedia.org/wiki/Consistent_hashing)
- [Two-Phase Commit](https://en.wikipedia.org/wiki/Two-phase_commit_protocol)
- [Paxos and Raft Consensus](https://raft.github.io/)

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-06  
**Author:** ThemisDB Team  
**Status:** Complete

