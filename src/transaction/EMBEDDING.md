---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h`, `src/transaction/transaction_manager.cpp`, and `src/transaction/distributed_transaction_manager.cpp`. Related issue: #7390.

# Transaction Embedding and Runtime Placement

## Purpose

This document explains where the transaction module sits in ThemisDB's runtime and which responsibilities it owns versus which responsibilities remain in adjacent modules.

## Upstream dependencies

The transaction module depends on lower-level durable and indexing systems, including:

| Dependency area | Representative source | Role |
|---|---|---|
| storage | `include/storage/rocksdb_wrapper.h`, `src/transaction/transaction_manager.cpp` | durable writes and persistence for transactional mutations |
| index | `include/index/secondary_index.h`, `include/index/graph_index.h`, `include/index/vector_index.h` | index update tracking during commit and rollback |
| sharding | `include/sharding/wal_manager.h`, `include/sharding/truetime.h` | distributed ordering and WAL-backed coordination |
| utils | `utils/logger.h`, `utils/retry_contract.h` | status surfaces, logging, and retry semantics |

## Downstream exposure

The transaction module is consumed by modules that need coherent write boundaries, including:

- query execution paths that need atomic multi-step mutation,
- server-facing APIs that execute DML through transaction lifecycles,
- sharding and distributed coordination flows,
- snapshot, branch, and merge workflows that depend on correct versioning semantics.

The module therefore sits near the core of ThemisDB's correctness plane, not at the outermost request boundary.

## Critical runtime edges

### Local write boundary

`TransactionManager` coordinates a local RocksDB-backed transaction lifecycle. This does not by itself establish atomicity for arbitrary external side effects or every separately managed index path; TODO: cite the specific integration contract before claiming those guarantees.

### Lock and contention boundary

`LockManager` is the admission control layer for conflicts. It prevents incompatible lock grants and records waiters so timed-out or deadlocked transactions are visible to the transaction manager.

### Compensation boundary

`Saga` stores and invokes compensation callbacks. Application-defined callbacks determine the effect of compensation; this class does not guarantee that external side effects are fully undone.

### Distributed coordination boundary

`DistributedTransactionManager` reaches beyond the local machine. It governs the prepare/commit/abort flow that makes multi-node transactional coordination a legal part of the runtime model.

## Runtime flow

A typical transaction path looks like this:

1. client or caller requests a transaction from `TransactionManager`,
2. the manager stores transaction metadata and isolation settings,
3. mutation requests are added to the transaction context,
4. lock checks guard incompatible access,
5. commit performs a durable atomic update or triggers rollback,
6. for distributed cases, participants vote and commit/abort under coordinator control.

## Guardrails

- keep local transactional state ownership narrow and unambiguous,
- prefer explicit timeout and retry semantics over silent recovery,
- keep compensation steps ordered and monitorable,
- avoid widening the transaction module into query-planning or networking responsibilities,
- ensure distributed state transitions remain observable and replayable.

## Summary

The transaction module is the correctness backbone between client-visible writes and lower-level storage/index durability. It is most useful when its boundaries remain precise: lock safety, commit semantics, compensation, and recovery are the real runtime responsibilities of this module.
