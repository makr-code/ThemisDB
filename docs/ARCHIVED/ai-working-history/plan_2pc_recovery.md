# 2PC Recovery Unification Plan

- affected files:
  - `include/transaction/recoverable_two_phase_coordinator.h`
  - `include/transaction/two_phase_commit_wal_recovery.h`
  - `include/sharding/two_phase_commit_coordinator.h`
  - `src/sharding/two_phase_commit_coordinator.cpp`
  - `include/sharding/distributed_transaction.h`
  - `src/sharding/distributed_transaction.cpp`
  - `include/sharding/cross_shard_transaction.h`
  - `src/sharding/cross_shard_transaction.cpp`
  - focused recovery tests + transaction recovery docs

- acceptance checks:
  - all three coordinator types expose one common recoverable-2PC contract
  - standalone 2PC recovery reconstructs shard targets from WAL and replays phase 2
  - distributed transaction coordinator writes BEGIN/decision/complete WAL records and can replay durable decisions
  - global recovery report can aggregate pre/post in-doubt counts across coordinators
  - regression tests cover commit-decision replay and global recovery aggregation

- verification scope:
  - capture current configure failures for `linux-release` / `community-release`
  - run focused file-level review plus WAL/recovery tests where sandbox dependencies allow
