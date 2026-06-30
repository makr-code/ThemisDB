# Distributed Transaction Coordinator with 2PC

## Overview

ThemisDB's Distributed Transaction Coordinator implements a Two-Phase Commit (2PC) protocol enhanced with TrueTime for providing ACID guarantees across multiple shards. This enables atomic, consistent, isolated, and durable transactions that span multiple database shards.

## Architecture

### Components

1. **DistributedTransactionCoordinator**: Orchestrates distributed transactions
2. **TrueTime**: Provides globally consistent timestamps
3. **ShardRPCClient**: Handles communication with participant shards
4. **TransactionParticipant**: Represents a shard participating in a transaction

### Key Features

- ✅ **Two-Phase Commit Protocol**: Guarantees atomicity across shards
- ✅ **TrueTime Integration**: External consistency for distributed transactions
- ✅ **Snapshot Isolation**: Read-only transactions without locks
- ✅ **Parallel Execution**: Concurrent prepare/commit operations
- ✅ **Fault Tolerance**: Handles participant failures gracefully
- ✅ **Configurable Timeouts**: Customizable RPC and phase timeouts
- ✅ **Cluster-Wide Deadlock Detection**: Distributed Wait-For Graph with cycle detection and victim abort
- ✅ **Cross-Shard Foreign Key Validation**: Referential integrity enforced at prepare-phase boundary (Issue #5390)

## Two-Phase Commit Protocol

### Phase 1: Prepare

1. Coordinator sends **PREPARE** request to all participants
2. Each participant:
   - Validates operations can be performed
   - Locks required resources
   - Writes undo/redo logs
   - Votes: **COMMIT** (prepared) or **ABORT** (cannot prepare)
3. Coordinator waits for all votes (with timeout)
4. If all vote COMMIT → proceed to Phase 2
5. If any vote ABORT or timeout → abort transaction

### Phase 2: Commit/Abort

#### Commit Path
1. Coordinator assigns commit timestamp using TrueTime
2. Wait until commit timestamp is in the past (TrueTime guarantee)
3. Send **COMMIT** request to all participants with timestamp
4. Participants apply changes and release locks
5. Transaction completes successfully

#### Abort Path
1. Send **ABORT** request to all participants
2. Participants rollback changes and release locks
3. Transaction fails

## TrueTime Integration

TrueTime provides external consistency guarantees:

```cpp
// Get commit timestamp (latest bound ensures visibility)
txn.commit_time = truetime_->now().latest;

// Wait until commit timestamp is definitely in the past
// This ensures all subsequent reads see this transaction
truetime_->waitUntil(txn.commit_time);
```

### Benefits

- **External Consistency**: Transactions are globally ordered
- **Snapshot Isolation**: Read-only transactions use snapshot timestamps
- **Wait-Free Reads**: No locks needed for read-only transactions

## API Usage

### Basic Transaction Flow

```cpp
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"

// Initialize
auto truetime = std::make_shared<TrueTime>();
DistributedTransactionCoordinator::Config config;
config.prepare_timeout_ms = 10000;
config.commit_timeout_ms = 10000;

auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
    truetime, config
);

// Begin distributed transaction
std::vector<std::string> shard_ids = {"shard1", "shard2", "shard3"};
std::string txn_id = coordinator->beginTransaction(shard_ids);

// Add operations to transaction
nlohmann::json op1 = {{"type", "insert"}, {"key", "user:123"}, {"value", {...}}};
coordinator->addOperation(txn_id, "shard1", op1);

nlohmann::json op2 = {{"type", "update"}, {"key", "account:456"}, {"value", {...}}};
coordinator->addOperation(txn_id, "shard2", op2);

// Commit with 2PC
bool success = coordinator->commit(txn_id);
if (success) {
    // Transaction committed atomically across all shards
} else {
    // Transaction aborted - no changes applied
}
```

### Read-Only Transactions

Read-only transactions are optimized with TrueTime:

```cpp
// No 2PC needed for reads!
std::vector<std::string> shard_ids = {"shard1", "shard2"};
nlohmann::json operations = {
    {"queries", {
        {{"shard", "shard1"}, {"key", "user:123"}},
        {{"shard", "shard2"}, {"key", "account:456"}}
    }}
};

// Execute snapshot read at consistent timestamp
auto results = coordinator->executeReadOnly(shard_ids, operations);
```

### Abort Transaction

```cpp
// Explicitly abort a transaction
bool aborted = coordinator->abort(txn_id);
```

### Check Transaction State

```cpp
auto state = coordinator->getTransactionState(txn_id);
if (state) {
    switch (*state) {
        case TransactionState::ACTIVE:
            // Transaction is active
            break;
        case TransactionState::PREPARING:
            // In prepare phase
            break;
        case TransactionState::COMMITTED:
            // Successfully committed
            break;
        case TransactionState::ABORTED:
            // Aborted or failed
            break;
        // ... other states
    }
}
```

### Statistics

```cpp
auto stats = coordinator->getStatistics();
// Returns:
// {
//   "total_transactions": 1000,
//   "committed_transactions": 950,
//   "aborted_transactions": 50,
//   "readonly_transactions": 5000,
//   "active_transactions": 10
// }
```

## Configuration

```cpp
DistributedTransactionCoordinator::Config config;

// Timeout for prepare phase (default: 10 seconds)
config.prepare_timeout_ms = 10000;

// Timeout for commit phase (default: 10 seconds)
config.commit_timeout_ms = 10000;

// Maximum concurrent transactions (default: 1000)
config.max_concurrent_txns = 1000;

// Enable read-only optimization (default: true)
config.enable_read_only_opt = true;

// RPC timeout per shard call (default: 5 seconds)
config.rpc_timeout_ms = 5000;

// Maximum RPC retry attempts (default: 3)
config.max_retries = 3;

// Maximum commit phase retries (default: 5, higher for durability)
config.max_commit_retries = 5;

// Base delay for exponential backoff (default: 100ms)
config.retry_backoff_base_ms = 100;

// Maximum backoff delay (default: 5 seconds)
config.max_backoff_ms = 5000;

// Enable transaction recovery logging (default: true)
config.enable_recovery_log = true;
```

## Transaction States

| State | Description |
|-------|-------------|
| `ACTIVE` | Transaction is active, accepting operations |
| `PREPARING` | Phase 1: Sending prepare requests |
| `PREPARED` | All participants prepared successfully |
| `COMMITTING` | Phase 2: Sending commit requests |
| `COMMITTED` | Transaction committed successfully |
| `ABORTING` | Sending abort requests |
| `ABORTED` | Transaction aborted |

## Error Handling

### Prepare Phase Failures

If any participant votes ABORT or times out:
- Coordinator sends ABORT to all participants
- Transaction state → ABORTED
- No changes are applied
- Detailed error context collected from all failed participants

### Commit Phase Failures

Enhanced commit phase with automatic retry logic:

1. **Automatic Retry**: If commit fails on any participant:
   - Coordinator automatically retries with exponential backoff
   - Default: up to 5 retry attempts
   - Backoff: 100ms → 200ms → 400ms → 800ms → 1600ms (capped at 5s)
   
2. **Error Context**: Detailed error information collected:
   - Which participants failed
   - Error messages from each participant
   - Retry attempt counts
   
3. **Recovery Logging**: For committed transactions:
   - Transaction state persisted to recovery log
   - Enables recovery on coordinator restart
   
4. **Persistent Failures**: On retry exhaustion:
   - Transaction marked as ABORTED
   - Detailed error logged for manual investigation
   - Recovery mechanism can replay commit based on coordinator log

**Example Configuration:**

```cpp
DistributedTransactionCoordinator::Config config;
config.max_commit_retries = 5;         // Retry commit up to 5 times
config.retry_backoff_base_ms = 100;    // Start with 100ms delay
config.max_backoff_ms = 5000;          // Cap delay at 5 seconds
config.enable_recovery_log = true;      // Enable recovery logging
```

### Network Partitions

- Participants with prepared state will wait for coordinator decision
- Coordinator timeout will trigger abort if participants unreachable
- Recovery protocol resolves in-doubt transactions on reconnection
- Recovery log enables coordinator to resume interrupted transactions

## Performance Characteristics

### Write Transactions

- **Latency**: ~2x single-shard transaction (2 network round trips)
- **Throughput**: Limited by coordinator capacity and network bandwidth
- **Scalability**: Horizontal scaling of coordinator instances possible

### Read-Only Transactions

- **Latency**: ~1x single-shard read (parallel reads)
- **Throughput**: Very high (no locking, no 2PC overhead)
- **Scalability**: Linear with number of shards

## Best Practices

### 1. Minimize Transaction Scope

```cpp
// ❌ BAD: Large transaction across many shards
auto txn_id = coordinator->beginTransaction({"shard1", "shard2", ..., "shard50"});

// ✅ GOOD: Focused transaction with minimal shards
auto txn_id = coordinator->beginTransaction({"shard1", "shard2"});
```

### 2. Use Read-Only Optimization

```cpp
// ❌ BAD: Using full 2PC for reads
auto txn_id = coordinator->beginTransaction(shard_ids);
// ... read operations ...
coordinator->commit(txn_id);

// ✅ GOOD: Use read-only optimization
auto results = coordinator->executeReadOnly(shard_ids, operations);
```

### 3. Handle Errors Gracefully

```cpp
try {
    auto txn_id = coordinator->beginTransaction(shard_ids);
    // ... operations ...
    
    if (!coordinator->commit(txn_id)) {
        // Handle commit failure
        LOG_ERROR("Transaction commit failed");
        // Possibly retry or compensate
    }
} catch (const std::exception& e) {
    LOG_ERROR("Transaction error: {}", e.what());
    // Handle exception
}
```

### 4. Configure Appropriate Timeouts

```cpp
// For operations with predictable latency
config.prepare_timeout_ms = 5000;   // 5 seconds
config.commit_timeout_ms = 5000;

// For operations with variable latency
config.prepare_timeout_ms = 30000;  // 30 seconds
config.commit_timeout_ms = 30000;
```

## Testing

### Unit Tests

Located in `tests/test_distributed_transactions.cpp`:

```bash
# Run distributed transaction tests
cd build
ctest -R test_distributed_transactions -V
```

### Integration Tests

```bash
# Run full integration test suite
cd build
ctest -R distributed -V
```

### Benchmarks

Located in `benchmarks/bench_distributed_coordinator.cpp`:

```bash
# Run benchmarks
cd build
./benchmarks/bench_distributed_coordinator
```

## Monitoring and Observability

### Metrics

The coordinator exposes metrics via `getStatistics()`:

- `total_transactions`: Total transactions started
- `committed_transactions`: Successfully committed
- `aborted_transactions`: Aborted or failed
- `readonly_transactions`: Read-only transactions executed
- `active_transactions`: Currently active transactions

### Logging

Transaction lifecycle events are logged:

```
[DEBUG] PREPARE shard1: vote=COMMIT
[DEBUG] PREPARE shard2: vote=COMMIT
[DEBUG] COMMIT shard1: success=true
[DEBUG] COMMIT shard2: success=true
```

## Limitations and Future Work

### Current Limitations

1. **Coordinator Single Point of Failure**: No coordinator replication (planned for future)
2. **Blocking Protocol**: 2PC is blocking if coordinator fails (3PC planned)

### Recent Enhancements (v1.5.x)

✅ **Automatic Commit Phase Retry**: Exponential backoff with configurable retry limits  
✅ **Enhanced Error Context**: Detailed error messages from all participants  
✅ **Recovery Logging**: Transaction states (PREPARED and COMMITTED) persisted to WAL  
✅ **Recovery Audit Trail**: Coordinator can verify successfully committed transactions on restart  
✅ **Improved Observability**: Better logging and error tracking throughout 2PC phases  
✅ **In-Doubt Transaction Recovery**: Coordinator scans `PREPARE_TX` WAL entries on restart and
   safely aborts any transactions that reached PREPARED state but were never resolved  
✅ **Dedicated WALEntryType::PREPARE_TX**: Semantically correct WAL entry type for 2PC PREPARE phase  
✅ **TwoPhaseCommitParticipant**: Shard-side participant handler with idempotent message handling,
   WAL-backed durability, prepare-timeout auto-abort, and crash recovery (`recoverFromWAL`)  
✅ **HTTP API**: REST endpoints for distributed transactions (`/dtxn/*`, see below)  
✅ **Cross-Shard Foreign Key Validation** (Issue #5390): Referential integrity checks at prepare-phase
   boundary via `CrossShardForeignKeyValidator` — see section below

## Cross-Shard Foreign Key Validation

### Problem

Per-shard schema constraints (NOT NULL, UNIQUE, CHECK) can only be enforced within a single shard.
Foreign key constraints that reference parent rows on *different* shards are structurally impossible
to enforce locally, enabling orphaned child records when the parent shard does not hold a matching row.

### Solution: Prepare-Phase FK Validation

`CrossShardTransactionCoordinator::prepare()` now supports an injected
`CrossShardForeignKeyValidator` that is called **before** any PREPARE RPC is
sent to participants.  If any FK constraint is violated, `prepare()` returns
`false` immediately and no participant ever enters the PREPARED state.

**Protocol extension (Phase 1):**

```
1. Coordinator receives prepare() call.
2. If a CrossShardForeignKeyValidator is configured:
   a. Merge all shard operations into a unified list.
   b. For each INSERT/UPDATE on a registered child table:
      - Extract FK column value.
      - Query parent shard: does the parent row exist? (via ParentKeyLookupFn)
      - If lookup returns false or throws → FK violation detected (fail-closed).
   c. If any violation → return false, transaction stays ACTIVE (no PREPARE sent).
3. If no violation → proceed with normal 2PC PREPARE phase.
```

### Architecture

```cpp
// include/sharding/cross_shard_fk_validator.h

struct CrossShardFKConstraint {
    std::string constraint_name;   // e.g. "fk_orders_user_id"
    std::string child_table;       // e.g. "orders"
    std::string child_column;      // e.g. "user_id"
    std::string parent_table;      // e.g. "users"
    std::string parent_column;     // e.g. "id"
    std::string parent_shard_id;   // shard that holds the parent table
};

class CrossShardForeignKeyValidator {
public:
    // Callback: (shard_id, parent_table, parent_column, fk_value) -> bool
    using ParentKeyLookupFn = std::function<bool(...)>;

    void registerConstraint(CrossShardFKConstraint constraint);
    void removeConstraint(const std::string& constraint_name);
    void setParentKeyLookup(ParentKeyLookupFn fn);

    // Called by prepare() — returns violations; empty = OK.
    std::vector<CrossShardFKViolation> validateTransaction(
        const nlohmann::json& operations) const;
};
```

### Usage

```cpp
// 1. Create and configure validator.
auto fkv = std::make_shared<CrossShardForeignKeyValidator>();

CrossShardFKConstraint fk;
fk.constraint_name  = "fk_orders_user_id";
fk.child_table      = "orders";
fk.child_column     = "user_id";
fk.parent_table     = "users";
fk.parent_column    = "id";
fk.parent_shard_id  = "shard_users";
fkv->registerConstraint(std::move(fk));

// 2. Inject a lookup callback (production: real gRPC lookup).
fkv->setParentKeyLookup(
    [&rpc_client](const std::string& shard, const std::string& table,
                  const std::string& column, const std::string& value) {
        return rpc_client.rowExists(shard, table, column, value);
    });

// 3. Wire into coordinator.
coordinator.setForeignKeyValidator(fkv);

// 4. Use normally — prepare() now blocks on FK violations.
auto txn_id = coordinator.begin("my-txn");
coordinator.addParticipant(txn_id, "shard_orders", "localhost:9001",
                            {insert_op.dump()});
bool ok = coordinator.prepare(txn_id);  // returns false if FK violated
if (!ok) coordinator.abort(txn_id);
```

### Fail-Closed Guarantee

- **No lookup callback**: treated as violation for every FK constraint.
- **Lookup throws exception**: caught, treated as violation.
- **Lookup returns false (network partition / shard unreachable)**: treated as violation.
- **NULL FK value**: skipped (nullable FK semantics — no parent required).

### Audit and Compliance

Each FK violation is logged at `WARN` level with full constraint metadata
(constraint name, child table/column, parent shard, FK value) and serialised
to JSON via `CrossShardFKViolation::toJSON()` for audit trail integration.

### Limitations and Migration Notes

- The `ParentKeyLookupFn` is called **synchronously** during prepare; high-latency
  remote lookups increase prepare-phase duration.  Keep lookup timeouts short.
- The validator checks FK values **as presented in the operation data**.  If the
  production storage layer encodes values differently, the lookup callback must
  normalise accordingly.
- Cross-shard FK constraints are advisory at the data model level; the validator
  enforces them at the coordinator boundary but does not prevent schema changes
  on individual shards.
- For DSGVO/GDPR and PCI DSS audit compliance, persist `CrossShardFKViolation`
  records to the audit log before returning from the FK check callback.

### HTTP API Reference

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/dtxn/begin` | Begin a distributed transaction across shards |
| POST | `/dtxn/operation` | Append an operation to an active transaction |
| POST | `/dtxn/commit` | Commit the transaction (runs 2PC) |
| POST | `/dtxn/abort` | Abort the transaction |
| POST | `/dtxn/readonly` | Execute a read-only (snapshot) query |
| GET | `/dtxn/status/{id}` | Query transaction state |
| GET | `/dtxn/stats` | Coordinator statistics |

> ✅ Isolation Hinweis: `/dtxn/begin` verwendet standardmäßig `serializable`.
> Für weniger strikte Workloads kann optional explizit `snapshot_isolation` gesetzt werden.
> ⚠️ `snapshot_isolation` kann Write-Skew- und Phantom-Read-Anomalien zulassen.
> Der Server-Default kann per Environment-Variable
> `THEMIS_DTXN_DEFAULT_ISOLATION` auf `snapshot_isolation` oder `serializable` gesetzt werden.

**Example — multi-shard transfer:**

```bash
# 1. Begin
curl -X POST http://localhost:8080/dtxn/begin \
     -H 'Content-Type: application/json' \
     -d '{"shards": ["shard1", "shard2"], "isolation_level": "serializable"}'
# → {"transaction_id": "txn-abc123", "status": "active", "shards": ["shard1","shard2"], "isolation_level":"serializable"}

# 2. Add operations
curl -X POST http://localhost:8080/dtxn/operation \
     -H 'Content-Type: application/json' \
     -d '{"transaction_id":"txn-abc123","shard_id":"shard1","operation":{"type":"update","key":"balance","delta":-100}}'
curl -X POST http://localhost:8080/dtxn/operation \
     -H 'Content-Type: application/json' \
     -d '{"transaction_id":"txn-abc123","shard_id":"shard2","operation":{"type":"update","key":"balance","delta":100}}'

# 3. Commit (runs 2PC internally)
curl -X POST http://localhost:8080/dtxn/commit \
     -H 'Content-Type: application/json' \
     -d '{"transaction_id":"txn-abc123"}'
# → {"transaction_id":"txn-abc123","status":"committed"}
```

### Future Enhancements

- [ ] Three-Phase Commit (3PC) for non-blocking guarantee
- [ ] Coordinator replication and failover
- [ ] Participant health monitoring with heartbeat mechanism
- [ ] Optimistic concurrency control
- [x] ~~Distributed deadlock detection~~ — implemented (see below)
- [ ] Saga pattern support for long-running transactions
- [ ] Circuit breaker pattern for participant health monitoring

## Cluster-Wide Deadlock Detection

`CrossShardTransactionCoordinator` implements a distributed Wait-For Graph (WFG) to detect
circular lock-wait dependencies that span multiple shards.

### Architecture

```
┌──────────────────────────────────────────────────────┐
│            CrossShardTransactionCoordinator           │
│                                                      │
│  ┌──────────────┐   push edges    ┌──────────────┐   │
│  │ reportDist-  │ ─────────────►  │  wait-for    │   │
│  │ ributedWait  │                 │  graph       │   │
│  └──────────────┘                 │  (in-memory) │   │
│                                   └──────┬───────┘   │
│  ┌──────────────┐   pull edges           │ SCC       │
│  │ deadlock-    │ ─────────────►  ┌──────▼───────┐   │
│  │ Detection-   │  (per-interval) │ cycle detect │   │
│  │ Thread       │ ◄─ ShardRPC ─── │ + victim     │   │
│  └──────────────┘    per shard    │   selection  │   │
│                                   └──────────────┘   │
└──────────────────────────────────────────────────────┘
```

### Configuration

```cpp
CrossShardTransactionConfig config;

// Register shard endpoints to poll (shard_id → gRPC address)
config.shard_endpoints["shard-1"] = "shard1.internal:50051";
config.shard_endpoints["shard-2"] = "shard2.internal:50051";

// How often to poll each shard and run cycle detection
config.deadlock_detection_interval = std::chrono::milliseconds(500);

auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
```

### Edge Sources

**Push (explicit reporting):** any component that knows a transaction is waiting calls
`coordinator.reportDistributedWait(waiting_txn_id, blocking_txn_id, shard_id)`.

**Pull (periodic polling):** the background detection thread calls
`ShardRPCClient::collectWaitForEdges()` on every endpoint in `shard_endpoints` once per
`deadlock_detection_interval`.  The `CollectWaitForEdges` RPC is defined in
`proto/sharding/shard_rpc.proto` and served by `ShardRPCServer`.

Both edge sources are merged into the same in-memory WFG before cycle detection.

### Deadlock Resolution

1. Cycle detected via Tarjan's SCC algorithm.
2. All transactions inside the SCC are identified as deadlocked.
3. The **youngest** transaction (most recent `start_time`) is chosen as victim and aborted.
   Younger transactions have invested less work; aborting them minimises wasted effort.
4. Remaining transactions in the cycle become unblocked.

### Testing Hook

For deterministic unit tests or custom deployments, set
`CrossShardTransactionConfig::polled_wait_for_edge_collector` to an `std::function`
that returns `PolledWaitForEdge` vectors without making a real RPC:

```cpp
config.polled_wait_for_edge_collector =
    [](const std::string& shard_id, const std::string& /*endpoint*/) {
        if (shard_id == "shard-remote") {
            return std::vector<CrossShardTransactionConfig::PolledWaitForEdge>{
                {"txn-a", "txn-b"}
            };
        }
        return {};
    };
```

## References

- Gray, J., & Reuter, A. (1992). Transaction Processing: Concepts and Techniques
- Corbett, J. C., et al. (2013). Spanner: Google's Globally Distributed Database
- Bernstein, P. A., & Newcomer, E. (2009). Principles of Transaction Processing

## See Also

- [TransactionManager](../src/transaction/transaction_manager.h) - Local transaction support
- [TrueTime](../include/sharding/truetime.h) - Distributed timestamp service
- [ShardRPCClient](../include/sharding/shard_rpc_client.h) - RPC communication
- [CrossShardForeignKeyValidator](../include/sharding/cross_shard_fk_validator.h) - Cross-shard FK referential integrity
- [Sharding Documentation](SHARDING_IMPLEMENTATION_SUMMARY.md)
