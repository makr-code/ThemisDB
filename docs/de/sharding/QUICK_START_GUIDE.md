# ThemisDB v1.4 Distributed Sharding - Quick Start Guide

## Overview

This guide helps you get started with ThemisDB v1.4's new distributed sharding capabilities, including pluggable consensus modules and cross-shard transactions.

## Prerequisites

- ThemisDB v1.4+ compiled with sharding support
- At least 3 nodes for consensus (recommended)
- Network connectivity between nodes

## Step 1: Basic Configuration

Create a `sharding_config.yaml` for each node:

### Node 1 Configuration

```yaml
# Node 1: shard1.example.com
node:
  id: "node1"
  listen_address: "0.0.0.0:8080"
  data_dir: "/var/lib/themisdb/node1"

sharding:
  enabled: true
  
  # Consensus configuration (choose Raft, Gossip, or Paxos)
  consensus:
    type: raft  # or gossip, paxos
    node_id: "node1"
    cluster_nodes:
      - "node1"
      - "node2"
      - "node3"
    heartbeat_interval_ms: 500
    election_timeout_min_ms: 1000
    election_timeout_max_ms: 2000
    enable_partition_detection: true
    enable_split_brain_prevention: true
  
  # Cross-shard transaction configuration
  transactions:
    protocol: two_phase_commit  # or three_phase_commit, saga, percolator
    isolation_level: serializable
    prepare_timeout_ms: 5000
    commit_timeout_ms: 5000
    abort_timeout_ms: 5000
    enable_deadlock_detection: true
    deadlock_detection_interval_ms: 1000
    transaction_timeout_ms: 30000
  
  # Metadata sharding
  metadata:
    num_shards: 3
    enable_cache: true
    cache_size: 10000
    cache_ttl_seconds: 300
    replication_factor: 3
  
  # Routing configuration
  router:
    scatter_timeout_ms: 30000
    max_concurrent_shards: 10
    enable_query_pushdown: true
```

For strict cross-shard invariants, keep `serializable` as the default.
Use `snapshot_isolation` only for workloads where write-skew and phantom-read anomalies are acceptable.

### Node 2 & Node 3 Configuration

Similar to Node 1, but update:
- `node.id` to "node2" and "node3"
- `node.listen_address` to appropriate IPs
- `consensus.node_id` to "node2" and "node3"

## Step 2: Start the Cluster

Start each node:

```bash
# Node 1
./themis_server --config sharding_config.yaml

# Node 2
./themis_server --config sharding_config.yaml

# Node 3
./themis_server --config sharding_config.yaml
```

Monitor logs for consensus leader election:

```
[INFO] Consensus module initialized: Raft
[INFO] Starting consensus protocol
[INFO] Node node1 became leader
[INFO] Cluster has quorum (3/3 nodes reachable)
```

## Step 3: Verify Cluster Status

### Check Consensus Status

```bash
curl http://localhost:8080/api/consensus/status
```

Expected response:

```json
{
  "type": "Raft",
  "node_id": "node1",
  "is_leader": true,
  "leader_id": "node1",
  "state": 2,
  "current_term": 5,
  "cluster_size": 3,
  "reachable_nodes": 3
}
```

### Check Sharding Status

```bash
curl http://localhost:8080/api/sharding/status
```

## Step 4: Execute Cross-Shard Transactions

### Example: Distributed Write

```cpp
#include "sharding/consensus_factory.h"
#include "sharding/cross_shard_transaction.h"

// Initialize consensus
ConsensusConfig consensus_config;
consensus_config.type = ConsensusType::RAFT;
consensus_config.node_id = "node1";
consensus_config.cluster_nodes = {"node1", "node2", "node3"};
auto consensus = ConsensusFactory::create(consensus_config);
consensus->initialize(consensus_config.node_id, consensus_config.cluster_nodes);
consensus->start();

// Initialize transaction coordinator
CrossShardTransactionConfig txn_config;
txn_config.default_protocol = TransactionProtocol::TWO_PHASE_COMMIT;
txn_config.default_isolation = IsolationLevel::SNAPSHOT_ISOLATION;
auto coordinator = std::make_shared<CrossShardTransactionCoordinator>(
    txn_config, consensus
);
coordinator->initialize();
coordinator->start();

// Begin cross-shard transaction
std::string txn_id = "txn_" + generateUUID();
coordinator->beginTransaction(txn_id);

// Add participants (shards involved)
coordinator->addParticipant(txn_id, "shard1", "node1:8080", 
    {"PUT /users/alice value1"});
coordinator->addParticipant(txn_id, "shard2", "node2:8080", 
    {"PUT /orders/order123 value2"});

// Execute transaction
if (coordinator->prepare(txn_id)) {
    if (coordinator->commit(txn_id)) {
        std::cout << "Transaction committed successfully" << std::endl;
    } else {
        std::cout << "Transaction commit failed" << std::endl;
    }
} else {
    coordinator->abort(txn_id);
    std::cout << "Transaction aborted (prepare failed)" << std::endl;
}
```

### Example: SAGA Transaction

```cpp
// Configure for SAGA protocol
txn_config.default_protocol = TransactionProtocol::SAGA;
auto saga_coordinator = std::make_shared<CrossShardTransactionCoordinator>(
    txn_config, consensus
);

// Define SAGA steps
std::vector<nlohmann::json> steps = {
    {{"action", "reserve_inventory"}, {"item", "product_123"}, {"qty", 5}},
    {{"action", "charge_payment"}, {"amount", 99.99}},
    {{"action", "create_shipment"}, {"address", "123 Main St"}}
};

// Define compensations (reverse operations)
std::vector<nlohmann::json> compensations = {
    {{"action", "release_inventory"}, {"item", "product_123"}, {"qty", 5}},
    {{"action", "refund_payment"}, {"amount", 99.99}},
    {{"action", "cancel_shipment"}}
};

// Execute SAGA
std::string saga_id = "saga_" + generateUUID();
coordinator->beginTransaction(saga_id, TransactionProtocol::SAGA);
bool success = coordinator->executeSaga(saga_id, steps, compensations);
```

## Step 5: Monitor and Observe

### Consensus Metrics

```bash
curl http://localhost:8080/metrics | grep consensus
```

Key metrics:
- `themis_consensus_proposals_total` - Total proposals submitted
- `themis_consensus_commits_total` - Successfully committed proposals
- `themis_consensus_leader_elections_total` - Number of leader elections

### Transaction Metrics

```bash
curl http://localhost:8080/metrics | grep transaction
```

Key metrics:
- `themis_transactions_total{protocol="2pc"}` - Total 2PC transactions
- `themis_transactions_committed_total` - Successfully committed
- `themis_transactions_aborted_total` - Aborted transactions
- `themis_transactions_deadlocked_total` - Deadlocks detected

### Get Transaction Statistics

```bash
curl http://localhost:8080/api/transactions/statistics
```

Response:
```json
{
  "total_transactions": 1523,
  "committed_transactions": 1498,
  "aborted_transactions": 25,
  "deadlocked_transactions": 3,
  "active_transactions": 2
}
```

## Consensus Algorithm Selection Guide

### When to Use Raft

**Best for:**
- Single datacenter deployments
- Low-latency requirements
- Simpler operational model

**Characteristics:**
- Leader-based consensus
- 1-2 RTT latency for commits
- Well-understood failure modes

**Configuration:**
```yaml
consensus:
  type: raft
  heartbeat_interval_ms: 500
  election_timeout_min_ms: 1000
```

### When to Use Paxos

**Best for:**
- Multi-datacenter deployments
- High availability requirements
- No single point of failure tolerance

**Characteristics:**
- Quorum-based consensus
- 2-3 RTT latency for commits
- Works well across WAN

**Configuration:**
```yaml
consensus:
  type: paxos
  paxos_quorum_size: 0  # auto-calculate
  paxos_enable_fast_path: true
  paxos_prepare_timeout_ms: 1000
  paxos_accept_timeout_ms: 500
```

### When to Use Gossip

**Best for:**
- Cluster membership management
- Failure detection
- Eventually consistent metadata

**Characteristics:**
- Leaderless, decentralized
- Eventually consistent
- Low overhead

**Configuration:**
```yaml
consensus:
  type: gossip
  gossip_fanout: 3
  gossip_interval_ms: 1000
```

## Transaction Protocol Selection Guide

### Two-Phase Commit (2PC)

**Use for:** Standard ACID transactions, strong consistency

**Pros:**
- Industry standard
- Well-understood
- Strong consistency guarantees

**Cons:**
- Blocking during coordinator failure
- ~10ms latency overhead

### Three-Phase Commit (3PC)

**Use for:** High availability, non-blocking transactions

**Pros:**
- Non-blocking variant of 2PC
- Better availability during failures

**Cons:**
- ~15ms latency overhead
- More complex

### SAGA

**Use for:** Long-running workflows, microservices

**Pros:**
- Non-blocking
- Works for long transactions
- Supports compensation

**Cons:**
- Requires compensation logic
- Eventual consistency

### Percolator

**Use for:** High-throughput write workloads

**Pros:**
- Optimistic concurrency
- Low latency (~5ms)
- High throughput

**Cons:**
- May retry on conflicts
- Not suitable for all workloads

## Troubleshooting

### Consensus Not Reaching Quorum

**Symptoms:** Logs show "No quorum available"

**Solutions:**
1. Check network connectivity between nodes
2. Verify all nodes are running
3. Check firewall rules
4. Review `cluster_nodes` configuration

### High Transaction Abort Rate

**Symptoms:** Many transactions aborting

**Solutions:**
1. Check for deadlocks in logs
2. Reduce transaction scope
3. Adjust timeout settings
4. Consider optimistic protocols (Percolator)

### Slow Cross-Shard Queries

**Symptoms:** High latency for distributed queries

**Solutions:**
1. Enable query pushdown: `enable_query_pushdown: true`
2. Increase `max_concurrent_shards`
3. Review shard distribution
4. Consider data locality

## Performance Tuning

### Optimize for Throughput

```yaml
consensus:
  type: raft
  raft_enable_pipelining: true  # Pipeline log replication
  
transactions:
  protocol: percolator  # Optimistic concurrency
  enable_deadlock_detection: false  # Reduce overhead
  
router:
  max_concurrent_shards: 20  # More parallelism
```

### Optimize for Latency

```yaml
consensus:
  heartbeat_interval_ms: 250  # Faster heartbeats
  election_timeout_min_ms: 500
  
transactions:
  prepare_timeout_ms: 2000  # Tighter timeouts
  commit_timeout_ms: 2000
```

### Optimize for Geo-Distribution

```yaml
consensus:
  type: paxos  # Better for WAN
  paxos_enable_fast_path: true
  
transactions:
  protocol: three_phase_commit  # Non-blocking
  transaction_timeout_ms: 60000  # Longer for WAN
```

## Next Steps

- Review [Distributed Sharding Architecture](../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)
- Read [Consensus Module Documentation](../docs/de/sharding/CONSENSUS_MODULE.md)
- Check [Migration Guide](../docs/de/migration/DATA_MIGRATION_COMPATIBILITY.md)

## Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
- Enterprise Support: enterprise@themisdb.com
