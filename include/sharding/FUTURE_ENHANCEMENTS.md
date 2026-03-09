# Sharding Module - Future Header Enhancements

## Scope

- `IShardRouter` interface extensions for adaptive routing and cross-shard query planning
- Adaptive rebalancer API (`IAdaptiveRebalancer`) for batched shard migration planning
- Distributed transaction coordinator interface (`IDistributedTxCoordinator`) with Percolator-style 2PC
- Raft snapshot API (`IRaftSnapshotManager`) for async snapshot initiation and compaction
- Consistent hashing ring interface (`IConsistentHashRing`) with immutable-during-rebalance semantics
- Cross-shard query routing API for fan-out and merge of multi-shard query plans

## Design Constraints

- `[x]` `IShardRouter` is fully thread-safe; all routing lookups must be lock-free on the read path
- `[x]` Adaptive rebalancer batches migration plans; no live data migration occurs while active queries reference the affected shards
- `[x]` `IConsistentHashRing` is immutable during a rebalance lock; callers observe the pre-rebalance ring until the lock is released
- `[x]` `IRaftSnapshotManager` snapshot initiation is async; `initiateSnapshot()` returns `std::future<SnapshotHandle>`
- `[x]` Distributed transaction coordinator exposes a typed error for each 2PC phase failure; no silent rollbacks
- `[x]` Cross-shard query routing overhead is accounted for in the query plan cost model exposed through the public header

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IShardRouter` | Query planner, write path | Thread-safe; exposes `route(const ShardKey&) -> ShardId`, `routeAll(std::span<const ShardKey>) -> std::vector<ShardId>` |
| `IAdaptiveRebalancer` | Cluster manager, admin API | Exposes `planRebalance() -> RebalancePlan`, `applyPlan(const RebalancePlan&) -> std::future<RebalanceResult>` |
| `IDistributedTxCoordinator` | Transaction manager, write path | Exposes `begin() -> TxHandle`, `prepare(TxHandle) -> PrepareResult`, `commit(TxHandle)`, `abort(TxHandle)` |
| `IRaftSnapshotManager` | Raft consensus layer, compaction | Exposes `initiateSnapshot(ShardId) -> std::future<SnapshotHandle>`, `verifySnapshot(SnapshotHandle) -> bool` |
| `IConsistentHashRing` | Shard router, rebalancer | Exposes `getNode(const ShardKey&) -> NodeId`, `nodes() -> std::span<const NodeId>`; immutable under rebalance lock |
| `ICrossShardQueryRouter` | Query planner, scan engine | Exposes `fanOut(const QueryPlan&) -> std::vector<ShardQueryPlan>`, `merge(std::span<ShardResult>) -> ResultSet` |

## Planned Features

### Adaptive Shard Rebalancer Interface

- `[x]` Define `IAdaptiveRebalancer` with `planRebalance(const ClusterStats&) -> RebalancePlan` — pure computation, no side effects
- `[x]` `RebalancePlan` carries `migrations` (`std::vector<ShardMigration>`), `estimatedDurationMs`, and `affectedShards`
- `[x]` Add `applyPlan(const RebalancePlan&) -> std::future<RebalanceResult>` for async execution
- `[x]` Rebalancer exposes `cancel(RebalancePlanId) -> bool`; cancellation is best-effort and safe to call from any thread

### Distributed Transaction Coordinator API

- `[x]` Define `IDistributedTxCoordinator` with Percolator-style phases: `begin() -> TxHandle`, `prepare(TxHandle) -> PrepareResult`, `commit(TxHandle)`, `abort(TxHandle)`
- `[x]` `PrepareResult` is a typed variant: `Prepared`, `ConflictError(TxId conflicting)`, `TimeoutError`
- `[x]` `TxHandle` is move-only RAII; destructor calls `abort()` if neither `commit()` nor explicit `abort()` was called
- `[x]` Coordinator exposes `maxConcurrentTx() -> size_t` and `activeTxCount() -> size_t` for backpressure signalling

### Raft Snapshot Compaction Interface

- `[x]` Define `IRaftSnapshotManager` with `initiateSnapshot(ShardId) -> std::future<SnapshotHandle>`
- `[x]` `SnapshotHandle` exposes `id()`, `sizeBytes()`, `createdAt()`, `verifyIntegrity() -> bool`
- `[x]` Add `compactLog(ShardId, SnapshotHandle) -> std::future<CompactionResult>` to truncate log up to snapshot index
- `[x]` Snapshot API is async throughout; no blocking calls on the public interface

### Consistent Hash Ring API

- `[x]` Define `IConsistentHashRing` with `getNode(const ShardKey&) -> NodeId` and `getNodes(const ShardKey&, size_t replicationFactor) -> std::vector<NodeId>`
- `[x]` Add `acquireRebalanceLock() -> RebalanceLockHandle`; ring is immutable while any lock handle is alive
- `[x]` `RebalanceLockHandle` is RAII move-only; destruction releases the lock
- `[x]` Ring exposes `virtualNodes() -> size_t` and `physicalNodes() -> std::vector<NodeId>` for introspection

### Cross-Shard Query Routing Interface

- `[x]` Define `ICrossShardQueryRouter` with `fanOut(const QueryPlan&) -> std::vector<ShardQueryPlan>`
- `[x]` Add `merge(std::span<const ShardResult>) -> ResultSet` with configurable merge strategy (enum: `Union`, `Intersect`, `Sorted`)
- `[x]` Query router exposes `estimateCost(const QueryPlan&) -> QueryCostEstimate` returning `shardCount`, `estimatedRows`, `networkHops`
- `[x]` All methods are `const` and thread-safe; routing is stateless with respect to ongoing transactions

## Test Strategy

- Thread-safety tests for `IShardRouter`: 64 concurrent threads performing `route()` and `routeAll()` with no data races
- Rebalancer plan computation tests: verify `RebalancePlan` is conflict-free and does not migrate shards referenced by active queries
- Distributed transaction coordinator tests cover all `PrepareResult` variants including conflict and timeout paths
- Raft snapshot integrity tests: corrupt a snapshot byte and assert `verifyIntegrity()` returns `false`
- Consistent hash ring immutability tests: acquire rebalance lock, mutate ring in background thread, assert readers see old ring
- Cross-shard query routing tests verify fan-out produces one `ShardQueryPlan` per affected shard and merge produces correct row counts

## Performance Targets

- `IShardRouter::route()` single-key lookup (lock-free): **≤ 200 ns**
- `IAdaptiveRebalancer::planRebalance()` for a 256-shard cluster: **≤ 100 ms**
- `IRaftSnapshotManager::initiateSnapshot()` round-trip to future ready: **≤ 1 s**
- `ICrossShardQueryRouter` routing overhead per query (excluding shard execution): **≤ 2 ms**
- `IDistributedTxCoordinator::prepare()` under no contention: **≤ 5 ms**
- `IConsistentHashRing::getNode()` lookup with 1,024 virtual nodes: **≤ 300 ns**

## Security / Reliability

- All cross-shard RPC calls are mandated to use mTLS; the coordinator interface exposes `requiresMTLS() -> bool` returning `true` always
- Shard admin operations (`applyPlan`, `compactLog`) require a cluster-admin capability token passed via `AdminContext`; missing token throws `PermissionDeniedError`
- Snapshot files are verified via HMAC-SHA-256 before any `compactLog` call proceeds; tampered snapshots are rejected
- Rebalancer cannot begin data migration without quorum approval; `applyPlan()` internally verifies quorum before the first migration step
- `IDistributedTxCoordinator` enforces transaction timeout; abandoned `TxHandle` objects are aborted by the coordinator after a configurable TTL
- `IConsistentHashRing` exposes no mechanism to enumerate all keys on a node; only per-key routing is permitted to limit data enumeration attacks
