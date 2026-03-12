<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Transaction Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- SAGA orchestration engine with fully-implemented compensating actions (relational, secondary-index, graph, vector)
- Serializable Snapshot Isolation (SSI) via predicate locking
- Git-like transaction branching and merging
- Named snapshots for point-in-time isolation
- Deadlock detection with wait-for graph cycle detection
- CDC changefeed integration for transaction events
- Transaction EXPLAIN: lock inspection and write-set analysis
- Multi-region Global Transaction Manager with TrueTime 2PC
- Distributed 2PC across shards via Raft coordination

## [1.0.0] — 2024-01-01
### Added
- ACID transactions on RocksDB via `TransactionDB`
- MVCC snapshot isolation
- Optimistic concurrency with write-conflict detection
- Savepoints within transactions
