# Core HTTP transaction workload: blocked

No transaction benchmark is implemented. The smallest candidate was a
single-node `POST /transaction` containing multiple Entity puts, followed by
Entity API readback and owned-key cleanup. The request handler uses a real
`TransactionManager` and MVCC transaction, but its rollback and failure
lifecycle is not safe enough for a repeatable workload:

1. `src/server/transaction_api_handler.cpp` calls `txn->rollback()` directly
   when an operation reports an error, instead of
   `TransactionManager::rollbackTransaction(txn_id)`. The manager therefore
   retains that finished transaction in `active_transactions_`; it is not counted
   as aborted. `abortTimedOutTransactions()` skips finished transactions, and
   normal completed-transaction cleanup scans a different map.
2. `POST /transaction/rollback` ignores the boolean returned by
   `rollbackTransaction` and returns `rolled_back` even for an unknown ID.
   The response alone does not establish a rollback.
3. If an exception occurs after the batch handler starts a transaction, its
   outer catch returns an error without resolving the manager-owned session or
   returning its ID. Separately, `Transaction::commit()` marks the transaction
   finished before commit; its commit-failure branch does not explicitly roll
   back the still-active RocksDB transaction. `TransactionWrapper::rollback()`
   also discards the RocksDB rollback status. These source-level failure paths
   were not fault-injected in the live probe.

These are not reasons to retry, silently compensate, or treat a second rollback
call as a benchmark compatibility path. The server needs exception-safe session
finalization and truthful rollback/commit-failure reporting, with focused
production tests, before this workload can be added.

## Isolated functional diagnostic, 2026-11-23

A copied Community Release server executable (SHA-256
`26f3d9c85f8efbbcfa243ccc4217879e870ef5e1baac049c4396b25d621e3bbf`)
ran on a PID-verified loopback listener with a private database and working
directory, minimal profile, two threads, and a 100000/minute rate limit.
It was not rebuilt from the current, dirty worktree.

- A two-put request returned HTTP 200, `committed`, `applied: 2`; both
  expected entities were readable afterward and active count was zero.
- A request with a valid put followed by an invalid operation returned HTTP 409,
  `rolled_back`, `applied: 0`; both affected keys read as HTTP 404. However,
  transaction stats changed from `active_count: 0, total_aborted: 0` to
  `active_count: 1, total_aborted: 0`.
- Rolling back an unknown transaction ID returned HTTP 200 and
  `status: rolled_back`.
- All four attempted entity keys were batch-deleted and verified absent; an
  unrelated entity remained readable. The harness terminated its own server,
  without graceful shutdown. Evidence (server log and private DB) remains at
  `%TEMP%\themis-txn-probe-e0kfg0ce`.

This diagnostic observes final states in one run; it does not prove atomic
visibility under concurrency, durability, recovery, or performance. The unit
tests in `tests/transaction/test_transaction_manager.cpp` exercise manager
commit/rollback and index results, not the defective HTTP error lifecycle.
They were not run here: no shared build was touched, and the existing HTTP
test harnesses can reuse or terminate unrelated servers.
