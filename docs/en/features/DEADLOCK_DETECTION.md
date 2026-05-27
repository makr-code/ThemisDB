# Deadlock Detection in ThemisDB

## Overview

ThemisDB includes built-in deadlock detection for transaction processing at two levels:

- **Local deadlock detection** (`TransactionManager`): scans circular wait dependencies
  within a single node in a background thread.
- **Cluster-wide deadlock detection** (`CrossShardTransactionCoordinator`): builds a
  distributed Wait-For Graph (WFG) across all shards and detects cycles that span shard
  boundaries (see [Cluster-Wide Detection](#cluster-wide-cross-shard-deadlock-detection)).

## Architecture

The deadlock detection system consists of:

1. **Wait-For Graph**: Tracks which transactions are waiting for resources held by other transactions
2. **Cycle Detection**: Uses depth-first search (DFS) to detect cycles in the wait-for graph
3. **Victim Selection**: When a deadlock is detected, the youngest transaction (highest ID) is chosen as the victim and aborted
4. **Background Thread**: Runs periodically to check for deadlocks at configurable intervals

## API Usage

### Enable/Disable Deadlock Detection

```cpp
#include "transaction/transaction_manager.h"

// Enable deadlock detection
transactionManager.setDeadlockDetection(true);

// Disable deadlock detection
transactionManager.setDeadlockDetection(false);
```

### Configure Detection Timeout

```cpp
// Set deadlock detection interval to 500ms
transactionManager.setDeadlockTimeout(std::chrono::milliseconds(500));

// Set deadlock detection interval to 2 seconds
transactionManager.setDeadlockTimeout(std::chrono::milliseconds(2000));
```

### Query Deadlock Statistics

```cpp
// Get total number of deadlocks detected
uint64_t deadlock_count = transactionManager.getDeadlockCount();
std::cout << "Total deadlocks detected: " << deadlock_count << std::endl;

// Get recent deadlocks (last 24 hours)
auto deadlocks = transactionManager.getDeadlocks(std::chrono::hours(24));

for (const auto& deadlock : deadlocks) {
    std::cout << "Deadlock detected at: " << /* format timestamp */ << std::endl;
    std::cout << "  Victim transaction: " << deadlock.victim_id << std::endl;
    std::cout << "  Transactions involved: ";
    for (auto txn_id : deadlock.cycle) {
        std::cout << txn_id << " ";
    }
    std::cout << std::endl;
}
```

## How It Works

### Lock Tracking

When deadlock detection is enabled, the TransactionManager tracks:
- **Held Locks**: Which transaction currently holds a lock on each resource (key)
- **Waiting Transactions**: Which transactions are waiting for specific resources

### Deadlock Detection Algorithm

1. **Build Wait-For Graph**: 
   - For each waiting transaction, determine which other transactions hold the locks it needs
   - Create edges: `waiting_txn -> holding_txn`

2. **Cycle Detection**:
   - Perform DFS traversal starting from each transaction
   - If we encounter a transaction already in the recursion stack, we've found a cycle

3. **Deadlock Resolution**:
   - Choose the youngest transaction (highest transaction ID) as the victim
   - Abort the victim transaction to break the cycle
   - Log the deadlock event for monitoring

### Example Deadlock Scenario

```
Transaction 1: Holds lock on key "A", waits for lock on key "B"
Transaction 2: Holds lock on key "B", waits for lock on key "A"

Wait-for graph:
  T1 -> T2 (T1 waits for T2 to release "B")
  T2 -> T1 (T2 waits for T1 to release "A")
  
Cycle detected: [T1, T2]
Victim selected: T2 (higher ID)
Action: Abort T2, allowing T1 to proceed
```

## Configuration Recommendations

### Development Environment
```cpp
// Aggressive detection for catching issues early
transactionManager.setDeadlockDetection(true);
transactionManager.setDeadlockTimeout(std::chrono::milliseconds(100));
```

### Production Environment
```cpp
// Conservative detection to minimize overhead
transactionManager.setDeadlockDetection(true);
transactionManager.setDeadlockTimeout(std::chrono::milliseconds(1000));
```

### High-Throughput Workloads
```cpp
// Less frequent checks to reduce overhead
transactionManager.setDeadlockDetection(true);
transactionManager.setDeadlockTimeout(std::chrono::milliseconds(5000));
```

## Best Practices

### Preventing Deadlocks

1. **Consistent Lock Ordering**
   ```cpp
   // Always acquire locks in the same order
   // Good: Always lock A before B
   txn1.putEntity("table", entityA);  // Locks A
   txn1.putEntity("table", entityB);  // Locks B
   
   txn2.putEntity("table", entityA);  // Locks A
   txn2.putEntity("table", entityB);  // Locks B
   ```

2. **Short Transactions**
   ```cpp
   // Keep transactions short to minimize lock hold time
   auto txn_id = transactionManager.beginTransaction();
   auto txn = transactionManager.getTransaction(txn_id);
   
   // Quick operations
   txn->putEntity("users", user);
   txn->commit();  // Release locks ASAP
   ```

3. **Timeout Handling**
   ```cpp
   // Handle potential deadlock aborts gracefully
   auto txn_id = transactionManager.beginTransaction();
   auto txn = transactionManager.getTransaction(txn_id);
   
   try {
       txn->putEntity("users", user);
       auto status = txn->commit();
       
       if (!status.ok) {
           // Transaction was aborted (possibly due to deadlock)
           // Implement retry logic
           std::cerr << "Transaction failed: " << status.message << std::endl;
       }
   } catch (const std::exception& e) {
       std::cerr << "Transaction error: " << e.what() << std::endl;
       txn->rollback();
   }
   ```

## Monitoring

### Metrics to Track

1. **Deadlock Rate**: `getDeadlockCount() / time_period`
2. **Transaction Abort Rate**: Check if increased due to deadlock resolution
3. **Average Transaction Duration**: Longer durations may increase deadlock probability

### Alerting

```cpp
// Periodic monitoring
void monitorDeadlocks() {
    static uint64_t last_count = 0;
    uint64_t current_count = transactionManager.getDeadlockCount();
    
    if (current_count > last_count + 10) {
        // Alert: Significant number of deadlocks detected
        logAlert("High deadlock rate detected: " + 
                 std::to_string(current_count - last_count) + 
                 " deadlocks in monitoring period");
    }
    
    last_count = current_count;
}
```

## Performance Considerations

### Overhead

- **Disabled**: No overhead
- **Enabled**: Small overhead from:
  - Lock tracking data structures
  - Background thread checking every N milliseconds
  - DFS traversal of wait-for graph

### Tuning

- **Faster Detection**: Lower timeout (100-500ms) - catches deadlocks quickly but higher overhead
- **Lower Overhead**: Higher timeout (2000-5000ms) - less frequent checks but deadlocks persist longer

## Limitations

1. **RocksDB Internal Locking**: The current implementation tracks high-level transaction operations. For full visibility into lock contention, deeper integration with RocksDB's lock manager would be needed.

2. **False Positives**: Very rare, but possible in high-contention scenarios

3. **Detection Latency**: Deadlocks are detected at interval boundaries, not immediately

## Future Enhancements

- [ ] Integration with RocksDB's native lock tracking
- [x] ~~Configurable victim selection strategies~~ — youngest-transaction strategy implemented
- [ ] Deadlock prediction based on historical patterns
- [ ] Visualization of wait-for graph in monitoring UI
- [ ] Automatic retry of aborted transactions

## Cluster-Wide (Cross-Shard) Deadlock Detection

Deadlocks involving locks on **different shards** require a distributed Wait-For Graph.
`CrossShardTransactionCoordinator` implements this via a pull+push hybrid approach.

### How It Works

1. **Push edges** — any component reports `coordinator.reportDistributedWait(waiter, blocker, shard_id)` when it knows a transaction is blocked.
2. **Pull edges** — the background `deadlockDetectionThread` polls every endpoint in `config.shard_endpoints` via `ShardRPCClient::collectWaitForEdges()` once per `deadlock_detection_interval`.
3. Both edge sources are merged into a single in-memory WFG.
4. **Cycle detection** runs Tarjan's SCC algorithm; SCC size > 1 signals a deadlock.
5. The **youngest** transaction in the cycle (most recent `start_time`) is selected as victim and aborted.

### Configuration

```cpp
CrossShardTransactionConfig config;
config.shard_endpoints["shard-1"] = "shard1.internal:50051";
config.shard_endpoints["shard-2"] = "shard2.internal:50051";
config.deadlock_detection_interval = std::chrono::milliseconds(500);

auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
coordinator->start();

// When a transaction is blocked cross-shard, report the wait edge:
coordinator->reportDistributedWait("txn-waiting", "txn-blocking", "shard-1");
```

### Statistics

```cpp
auto stats = coordinator->getStatistics();
uint64_t count = stats["deadlocked_transactions"].get<uint64_t>();
```

### Example: Classic Cross-Shard Deadlock

```
T1 holds lock on Shard-A/key-X, waits for Shard-B/key-Y
T2 holds lock on Shard-B/key-Y, waits for Shard-A/key-X

WFG:  T1 → T2  (from Shard-A polling or push)
      T2 → T1  (from Shard-B polling or push)

Cycle [T1, T2] detected → youngest aborted → remaining transaction proceeds
```

### Testing Hook

For deterministic tests, assign `config.polled_wait_for_edge_collector`:

```cpp
config.polled_wait_for_edge_collector =
    [](const std::string& shard_id, const std::string&) {
        if (shard_id == "shard-remote") {
            return std::vector<CrossShardTransactionConfig::PolledWaitForEdge>{
                {"txn-a", "txn-b"}
            };
        }
        return {};
    };
```

## See Also

- [Transaction Best Practices](../features/TRANSACTION_BEST_PRACTICES.md)
- [MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)
- [Troubleshooting Guide](../operations/TROUBLESHOOTING_GUIDE.md)
