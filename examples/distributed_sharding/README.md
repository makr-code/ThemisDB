> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Distributed Sharding Example

This example demonstrates ThemisDB v1.4's distributed sharding capabilities, including:

- Pluggable consensus modules (Raft, Gossip, Paxos)
- Cross-shard transactions with 2PC
- SAGA transactions with compensation
- Monitoring and observability

## Building the Example

```bash
# From ThemisDB root directory
mkdir build && cd build
cmake .. -DTHEMIS_BUILD_EXAMPLES=ON
make distributed_sharding_example
```

## Running the Example

```bash
./build/examples/distributed_sharding/distributed_sharding_example
```

## What the Example Demonstrates

### Example 1: Consensus Setup
- Creates a Raft consensus module
- Initializes with 3-node cluster
- Performs leader election
- Displays cluster statistics

### Example 2: Two-Phase Commit Transaction
- Sets up cross-shard transaction coordinator
- Begins a distributed transaction
- Adds multiple shard participants
- Executes 2PC protocol (prepare + commit)

## Expected Output

```
==================================================
ThemisDB v1.4 Distributed Sharding Examples
==================================================

=== Example 1: Consensus Setup ===
Waiting for leader election...
Is leader: yes
Leader ID: node1
Current state: 2
Cluster size: 3
Reachable nodes: 3
Current term: 1
Consensus stopped

=== Example 2: Two-Phase Commit ===
Beginning transaction: txn_1
Added 2 participants to transaction
Transaction committed successfully!

==================================================
Examples completed!
==================================================
```

## Key Concepts

### Consensus Module
The consensus module ensures that all nodes in the cluster agree on the order of operations. ThemisDB supports three algorithms:

- **Raft**: Leader-based, optimal for single-DC
- **Gossip**: Leaderless, good for membership
- **Paxos**: Quorum-based, optimal for multi-DC

### Cross-Shard Transactions
When a transaction spans multiple shards, the coordinator ensures ACID properties using protocols like:

- **2PC**: Two-phase commit (blocking, strongly consistent)
- **3PC**: Three-phase commit (non-blocking)
- **SAGA**: Compensating transactions (eventual consistency)
- **Percolator**: Optimistic concurrency (high throughput)

## Extending the Example

### Try Different Consensus Algorithms

```cpp
// Use Paxos instead of Raft
config.type = ConsensusType::PAXOS;
```

### Try Different Transaction Protocols

```cpp
// Use SAGA instead of 2PC
txn_config.default_protocol = TransactionProtocol::SAGA;
```

### Add More Shards

```cpp
coordinator->addParticipant(txn_id, "shard3", "node3:8080", operations);
coordinator->addParticipant(txn_id, "shard4", "node4:8080", operations);
```

## Related Documentation

- [Quick Start Guide](../../docs/de/sharding/QUICK_START_GUIDE.md)
- [Consensus Module Architecture](../../docs/de/sharding/CONSENSUS_MODULE.md)
- [Distributed Sharding Architecture](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)

## Troubleshooting

### Consensus not starting
- Ensure all node IDs in `cluster_nodes` are reachable
- Check network connectivity
- Verify no port conflicts

### Transaction failures
- Check shard endpoints are valid
- Ensure consensus has elected a leader
- Review timeout settings

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
