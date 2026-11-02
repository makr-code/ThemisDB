---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/epoch_fencing.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_client_adapter.h`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
Related issues: #7093, #7167
Related milestone: DOC-WEEKLY-2026-W45

# Sharding implementation

## Purpose and boundary

For developers changing shard routing, cross-shard transaction handling, WAL/recovery, or migration. The frozen v1.x contract in `include/sharding/sharding_api_contract.h` is normative for the behaviors it specifies.

## Implementation map

- `shard_router.cpp`: routes point operations and query work using the configured resolver/executor.
- `cross_shard_transaction.cpp`: cross-shard transaction state and decision handling.
- `wal_manager.cpp`, `wal_applier.cpp`, and migration sources implement separate durability and topology-change paths; follow the matching public contract before editing.

## Invariants and failure cases

- The contract defines stable key placement for an unchanged ring and bounded fallback when a primary shard is unavailable.
- A transaction left `IN_DOUBT` must block conflicting writes until resolved; preserve that state instead of guessing commit or abort.
- WAL replay corruption is a contract failure, not a successful partial recovery.
- Migration is stateful (planned, transferring, verifying, complete); failed work must follow the contract’s rollback/fault path.
- Routing, transaction, WAL, and migration errors must remain distinguishable through their declared error taxonomy.

## Epoch fencing on shard write paths

- `EpochFencingManager::checkToken()` rejects zero epochs, empty or
  source-mismatched issuers, wrong shard scopes, stale epochs, and future
  epochs; only an exact current-epoch token is allowed. Issuer validation is
  string equality, not cryptographic signature verification or authenticated
  principal binding.
- `applyWithToken()` holds the epoch-transition shared guard from validation
  through the storage callback. `bumpEpoch()` takes the exclusive guard, so an
  epoch transition cannot pass an in-flight guarded write.
- 2PC write transactions require a configured per-shard token provider. The
  coordinator validates and serializes each target token in PREPARE; the RPC
  client transports it in the existing `transaction_data` field. The
  participant checks it before prepare-time locking, retains/persists it in its
  transaction record/WAL, and atomically rechecks it immediately before COMMIT
  applies storage changes. The two-argument RPC prepare overload returns false
  without making a request.
- `DistributedTransactionCoordinator` and `CrossShardTransactionConfig` expose
  token providers. Their distributed PREPARE and SAGA write paths reject absent
  or structurally invalid target-scoped credentials rather than dispatching
  unfenced writes.
- Phase 2 participant failure to acknowledge yields coordinator `ERROR` and
  leaves the decision unresolved; it is not logged as complete. When WAL
  persistence is configured, the decision record remains available for
  recovery. Remote abort adapters return the participant's actual ACK status.

Shared durable metadata for DR, authenticated issuer binding, and genuine
multi-process failover/recovery E2E evidence remain open limitations. Source-based
check coverage was added for this change but was not executed; no test-pass claim
is made here.

## Change and validation steps

1. Locate the relevant section in `sharding_api_contract.h` and the corresponding implementation.
2. Add focused coverage in `tests/sharding/`; the contract regression suite is `test_sharding_contract_hardening_focused.cpp`.
3. Run the affected target registered by `tests/sharding/CMakeLists.txt`; for routing or end-to-end behavior, select the closest sharding core/integration test.

See `API.md`, `MODULE_GUIDE.md`, and `ROADMAP.md` for contracts and module context.
